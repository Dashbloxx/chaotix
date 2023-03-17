#include "console.h"
#include "psf.h"
#include <common/extra.h>
#include <common/stdlib.h>
#include <common/string.h>
#include <kernel/api/fb.h>
#include <kernel/api/fcntl.h>
#include <kernel/api/hid.h>
#include <kernel/api/signum.h>
#include <kernel/api/sys/ioctl.h>
#include <kernel/api/sys/sysmacros.h>
#include <kernel/api/sys/types.h>
#include <kernel/fs/fs.h>
#include <kernel/interrupts.h>
#include <kernel/lock.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/ring_buf.h>
#include <kernel/scheduler.h>
//#include <string.h>

#define TAB_STOP 8

#define DEFAULT_FG_COLOR 7
#define DEFAULT_BG_COLOR 0

static const uint32_t palette[] = {
    0x191919, 0xcc0000, 0x4e9a06, 0xc4a000, 0x3465a4, 0x75507b,
    0x06989a, 0xd0d0d0, 0x555753, 0xef2929, 0x8ae234, 0xfce94f,
    0x729fcf, 0xad7fa8, 0x34e2e2, 0xeeeeec,
};

struct cell {
    char ch;
    uint8_t fg_color;
    uint8_t bg_color;
};

static struct font* font;
static uintptr_t fb_addr;
static struct fb_info fb_info;
static size_t console_width;
static size_t console_height;
static size_t cursor_x = 0;
static size_t cursor_y = 0;
static bool is_cursor_visible = true;
static enum { STATE_GROUND, STATE_ESC, STATE_CSI } state = STATE_GROUND;
static uint8_t fg_color = DEFAULT_FG_COLOR;
static uint8_t bg_color = DEFAULT_BG_COLOR;
static struct cell* cells;
static bool* line_is_dirty;
static bool whole_screen_should_be_cleared = false;
static bool stomp = false;

static void set_cursor(size_t x, size_t y) {
    stomp = false;
    line_is_dirty[cursor_y] = true;
    line_is_dirty[y] = true;
    cursor_x = x;
    cursor_y = y;
}

static void clear_line_at(size_t x, size_t y, size_t length) {
    struct cell* cell = cells + x + y * console_width;
    for (size_t i = 0; i < length; ++i) {
        cell->ch = ' ';
        cell->fg_color = fg_color;
        cell->bg_color = bg_color;
        ++cell;
    }
    line_is_dirty[y] = true;
}

static void clear_screen(void) {
    for (size_t y = 0; y < console_height; ++y)
        clear_line_at(0, y, console_width);
    whole_screen_should_be_cleared = true;
}

static void write_char_at(size_t x, size_t y, char c) {
    struct cell* cell = cells + x + y * console_width;
    cell->ch = c;
    cell->fg_color = fg_color;
    cell->bg_color = bg_color;
    line_is_dirty[y] = true;
}

static void scroll_up(void) {
    memmove(cells, cells + console_width,
            console_width * (console_height - 1) * sizeof(struct cell));
    for (size_t y = 0; y < console_height - 1; ++y)
        line_is_dirty[y] = true;
    clear_line_at(0, console_height - 1, console_width);
}

static void flush_cell_at(size_t x, size_t y, struct cell* cell) {
    bool is_cursor = is_cursor_visible && x == cursor_x && y == cursor_y;
    uint32_t fg = palette[is_cursor ? cell->bg_color : cell->fg_color];
    uint32_t bg = palette[is_cursor ? cell->fg_color : cell->bg_color];

    const unsigned char* glyph =
        font->glyphs +
        font->ascii_to_glyph[(size_t)cell->ch] * font->bytes_per_glyph;
    uintptr_t row_addr = fb_addr + x * font->glyph_width * sizeof(uint32_t) +
                         y * font->glyph_height * fb_info.pitch;
    for (size_t py = 0; py < font->glyph_height; ++py) {
        uint32_t* pixel = (uint32_t*)row_addr;
        for (size_t px = 0; px < font->glyph_width; ++px) {
            uint32_t val = *(const uint32_t*)glyph;
            uint32_t swapped = ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) |
                               ((val >> 8) & 0xff00) |
                               ((val << 24) & 0xff000000);
            *pixel++ = swapped & (1 << (32 - px - 1)) ? fg : bg;
        }
        glyph += font->bytes_per_glyph / font->glyph_height;
        row_addr += fb_info.pitch;
    }
}

static void flush(void) {
    if (whole_screen_should_be_cleared) {
        memset32((uint32_t*)fb_addr, palette[bg_color],
                 fb_info.pitch * fb_info.height / sizeof(uint32_t));
        whole_screen_should_be_cleared = false;
    }

    struct cell* row_cells = cells;
    bool* dirty = line_is_dirty;
    for (size_t y = 0; y < console_height; ++y) {
        if (*dirty) {
            struct cell* cell = row_cells;
            for (size_t x = 0; x < console_width; ++x)
                flush_cell_at(x, y, cell++);
            *dirty = false;
        }
        row_cells += console_width;
        ++dirty;
    }
}

static void handle_ground(char c) {
    switch (c) {
    case '\x1b':
        state = STATE_ESC;
        return;
    case '\r':
        set_cursor(0, cursor_y);
        break;
    case '\n':
        set_cursor(0, cursor_y + 1);
        break;
    case '\b':
        if (cursor_x > 0)
            set_cursor(cursor_x - 1, cursor_y);
        break;
    case '\t':
        set_cursor(round_up(cursor_x + 1, TAB_STOP), cursor_y);
        break;
    default:
        if ((unsigned)c > 127)
            return;
        if (stomp)
            set_cursor(0, cursor_y + 1);
        if (cursor_y >= console_height) {
            scroll_up();
            set_cursor(cursor_x, console_height - 1);
        }
        write_char_at(cursor_x, cursor_y, c);
        set_cursor(cursor_x + 1, cursor_y);
        break;
    }
    if (cursor_x >= console_width) {
        set_cursor(console_width - 1, cursor_y);

        // event if we reach at the right end of a screen, we don't proceed to
        // the next line until we write the next character
        stomp = true;
    }
}

static char param_buf[1024];
static size_t param_buf_idx = 0;

static void handle_state_esc(char c) {
    switch (c) {
    case '[':
        param_buf_idx = 0;
        state = STATE_CSI;
        return;
    }
    state = STATE_GROUND;
    handle_ground(c);
}

// Cursor Up
static void handle_csi_cuu(void) {
    unsigned dy = atoi(param_buf);
    if (dy == 0)
        dy = 1;
    if (dy > cursor_y)
        set_cursor(cursor_x, 0);
    else
        set_cursor(cursor_x, cursor_y - dy);
}

// Cursor Down
static void handle_csi_cud(void) {
    unsigned dy = atoi(param_buf);
    if (dy == 0)
        dy = 1;
    if (dy + cursor_y >= console_height)
        set_cursor(cursor_x, console_height - 1);
    else
        set_cursor(cursor_x, cursor_y + dy);
}

// Cursor Forward
static void handle_csi_cuf(void) {
    unsigned dx = atoi(param_buf);
    if (dx == 0)
        dx = 1;
    if (dx + cursor_x >= console_width)
        set_cursor(console_width - 1, cursor_y);
    else
        set_cursor(cursor_x + dx, cursor_y);
}

// Cursor Back
static void handle_csi_cub(void) {
    unsigned dx = atoi(param_buf);
    if (dx == 0)
        dx = 1;
    if (dx > cursor_x)
        set_cursor(0, cursor_y);
    else
        set_cursor(cursor_x - dx, cursor_y);
}

// Cursor Horizontal Absolute
static void handle_csi_cha(void) {
    unsigned x = atoi(param_buf);
    if (x > 0)
        --x;
    if (x >= console_width)
        x = console_width - 1;
    set_cursor(x, cursor_y);
}

// Cursor Position
static void handle_csi_cup(void) {
    size_t x = 0;
    size_t y = 0;

    static const char* sep = ";";
    char* saved_ptr;
    const char* param = strtok_r(param_buf, sep, &saved_ptr);
    for (size_t i = 0; param; ++i) {
        switch (i) {
        case 0:
            y = atoi(param);
            if (y > 0)
                --y;
            break;
        case 1:
            x = atoi(param);
            if (x > 0)
                --x;
            break;
        }
        param = strtok_r(NULL, sep, &saved_ptr);
    }

    if (x >= console_width)
        x = console_width - 1;
    if (y >= console_height)
        y = console_height - 1;
    set_cursor(x, y);
}

// Erase in Display
static void handle_csi_ed(void) {
    switch (atoi(param_buf)) {
    case 0:
        clear_line_at(cursor_x, cursor_y, console_width - cursor_x);
        for (size_t y = cursor_y + 1; y < console_height; ++y)
            clear_line_at(0, y, console_width);
        break;
    case 1:
        if (cursor_y > 0) {
            for (size_t y = 0; y < cursor_y; ++y)
                clear_line_at(0, y, console_width);
        }
        clear_line_at(0, cursor_y, cursor_x + 1);
        break;
    case 2:
        clear_screen();
        break;
    }
}

// Erase in Line
static void handle_csi_el(void) {
    switch (atoi(param_buf)) {
    case 0:
        clear_line_at(cursor_x, cursor_y, console_width - cursor_x);
        break;
    case 1:
        clear_line_at(0, cursor_y, cursor_x + 1);
        break;
    case 2:
        clear_line_at(0, cursor_y, console_width);
        break;
    }
}

// Select Graphic Rendition
static void handle_csi_sgr(void) {
    if (param_buf[0] == '\0') {
        fg_color = DEFAULT_FG_COLOR;
        bg_color = DEFAULT_BG_COLOR;
        return;
    }

    static const char* sep = ";";
    char* saved_ptr;
    bool bold = false;
    for (const char* param = strtok_r(param_buf, sep, &saved_ptr); param;
         param = strtok_r(NULL, sep, &saved_ptr)) {
        int num = atoi(param);
        if (num == 0) {
            fg_color = DEFAULT_FG_COLOR;
            bg_color = DEFAULT_BG_COLOR;
            bold = false;
        } else if (num == 1) {
            bold = true;
        } else if (num == 7) {
            uint8_t tmp = fg_color;
            fg_color = bg_color;
            bg_color = tmp;
        } else if (num == 22) {
            fg_color = DEFAULT_FG_COLOR;
            bold = false;
        } else if (30 <= num && num <= 37) {
            fg_color = num - 30 + (bold ? 8 : 0);
        } else if (num == 38) {
            fg_color = DEFAULT_FG_COLOR;
        } else if (40 <= num && num <= 47) {
            bg_color = num - 40 + (bold ? 8 : 0);
        } else if (num == 48) {
            bg_color = DEFAULT_BG_COLOR;
        } else if (90 <= num && num <= 97) {
            fg_color = num - 90 + 8;
        } else if (100 <= num && num <= 107) {
            bg_color = num - 100 + 8;
        }
    }
}

// Text Cursor Enable Mode
static void handle_csi_dectcem(char c) {
    if (strcmp(param_buf, "?25") != 0)
        return;
    switch (c) {
    case 'h':
        is_cursor_visible = true;
        line_is_dirty[cursor_y] = true;
        return;
    case 'l':
        is_cursor_visible = false;
        line_is_dirty[cursor_y] = true;
        return;
    }
}

static void handle_state_csi(char c) {
    if (c < 0x40) {
        param_buf[param_buf_idx++] = c;
        return;
    }
    param_buf[param_buf_idx] = '\0';

    switch (c) {
    case 'A':
        handle_csi_cuu();
        break;
    case 'B':
        handle_csi_cud();
        break;
    case 'C':
        handle_csi_cuf();
        break;
    case 'D':
        handle_csi_cub();
        break;
    case 'G':
        handle_csi_cha();
        break;
    case 'H':
        handle_csi_cup();
        break;
    case 'J':
        handle_csi_ed();
        break;
    case 'K':
        handle_csi_el();
        break;
    case 'm':
        handle_csi_sgr();
        break;
    case 'h':
    case 'l':
        handle_csi_dectcem(c);
        break;
    }

    state = STATE_GROUND;
}

static void on_char(char c) {
    switch (state) {
    case STATE_GROUND:
        handle_ground(c);
        return;
    case STATE_ESC:
        handle_state_esc(c);
        return;
    case STATE_CSI:
        handle_state_csi(c);
        return;
    }
    UNREACHABLE();
}

static bool initialized = false;
static ring_buf input_buf;
static mutex lock;

void fb_console_init(void) {
    font = load_psf("/usr/share/fonts/ter-u16n.psf");
    ASSERT_OK(font);

    file_description* desc = vfs_open("/dev/fb0", O_RDWR, 0);
    ASSERT_OK(desc);

    ASSERT_OK(file_description_ioctl(desc, FBIOGET_INFO, &fb_info));
    ASSERT(fb_info.bpp == 32);

    console_width = fb_info.width / font->glyph_width;
    console_height = fb_info.height / font->glyph_height;

    cells = kmalloc(console_width * console_height * sizeof(struct cell));
    ASSERT(cells);
    line_is_dirty = kmalloc(console_height * sizeof(bool));
    ASSERT(line_is_dirty);

    size_t fb_size = fb_info.pitch * fb_info.height;
    uintptr_t vaddr = range_allocator_alloc(&kernel_vaddr_allocator, fb_size);
    ASSERT_OK(vaddr);
    fb_addr = file_description_mmap(desc, vaddr, fb_size, 0,
                                    PAGE_WRITE | PAGE_SHARED | PAGE_GLOBAL);
    ASSERT_OK(fb_addr);

    ASSERT_OK(file_description_close(desc));

    clear_screen();
    flush();

    ASSERT_OK(ring_buf_init(&input_buf));

    initialized = true;
}

static void input_buf_write_str(const char* s) {
    bool int_flag = push_cli();
    ring_buf_write_evicting_oldest(&input_buf, s, strlen(s));
    pop_cli(int_flag);
}

static pid_t pgid;

void fb_console_on_key(const key_event* event) {
    if (!event->pressed)
        return;
    switch (event->keycode) {
    case KEYCODE_UP:
        input_buf_write_str("\x1b[A");
        return;
    case KEYCODE_DOWN:
        input_buf_write_str("\x1b[B");
        return;
    case KEYCODE_RIGHT:
        input_buf_write_str("\x1b[C");
        return;
    case KEYCODE_LEFT:
        input_buf_write_str("\x1b[D");
        return;
    case KEYCODE_HOME:
        input_buf_write_str("\x1b[H");
        return;
    case KEYCODE_END:
        input_buf_write_str("\x1b[F");
        return;
    case KEYCODE_DELETE:
        input_buf_write_str("\x1b[3~");
        return;
    default:
        break;
    }

    if (!event->key)
        return;
    char key = event->key;
    if (event->modifiers & KEY_MODIFIER_CTRL) {
        if ('a' <= key && key <= 'z')
            key -= '`';
        else if (key == '\\')
            key = 0x1c;
    }

    tty_maybe_send_signal(pgid, key);

    bool int_flag = push_cli();
    ring_buf_write_evicting_oldest(&input_buf, &key, 1);
    pop_cli(int_flag);
}

static bool read_should_unblock(file_description* desc) {
    (void)desc;
    bool int_flag = push_cli();
    bool should_unblock = !ring_buf_is_empty(&input_buf);
    pop_cli(int_flag);
    return should_unblock;
}

static ssize_t fb_console_device_read(file_description* desc, void* buffer,
                                      size_t count) {
    (void)desc;

    for (;;) {
        int rc = file_description_block(desc, read_should_unblock);
        if (IS_ERR(rc))
            return rc;

        bool int_flag = push_cli();
        if (ring_buf_is_empty(&input_buf)) {
            pop_cli(int_flag);
            continue;
        }

        ssize_t nread = ring_buf_read(&input_buf, buffer, count);
        pop_cli(int_flag);
        return nread;
    }
}

static ssize_t fb_console_device_write(file_description* desc,
                                       const void* buffer, size_t count) {
    (void)desc;
    const char* chars = (char*)buffer;
    mutex_lock(&lock);

    for (size_t i = 0; i < count; ++i)
        on_char(chars[i]);
    flush();

    mutex_unlock(&lock);
    return count;
}

static int fb_console_device_ioctl(file_description* desc, int request,
                                   void* argp) {
    (void)desc;
    switch (request) {
    case TIOCGPGRP:
        *(pid_t*)argp = pgid;
        return 0;
    case TIOCSPGRP: {
        pid_t new_pgid = *(pid_t*)argp;
        if (new_pgid < 0)
            return -EINVAL;
        pgid = new_pgid;
        return 0;
    }
    case TIOCGWINSZ: {
        struct winsize* winsize = (struct winsize*)argp;
        winsize->ws_col = console_width;
        winsize->ws_row = console_height;
        winsize->ws_xpixel = winsize->ws_ypixel = 0;
        return 0;
    }
    }
    return -EINVAL;
}

struct inode* fb_console_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = fb_console_device_read,
                            .write = fb_console_device_write,
                            .ioctl = fb_console_device_ioctl};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(5, 0),
                            .ref_count = 1};
    return inode;
}
