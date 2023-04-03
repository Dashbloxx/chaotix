/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
 * 
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <ctype.h>
#include <errno.h>
#include <extra.h>
#include <fcntl.h>
#include <panic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 1024

struct line_editor {
    char input_buf[BUF_SIZE];
    size_t input_len;
    size_t cursor;
    enum { STATE_GROUND, STATE_ESC, STATE_CSI, STATE_ACCEPTED } state;
    bool dirty;
    char param_buf[BUF_SIZE];
    size_t param_len;
};

static void handle_ground(struct line_editor* ed, char c) {
    if (isprint(c)) {
        memmove(ed->input_buf + ed->cursor + 1, ed->input_buf + ed->cursor, ed->input_len - ed->cursor);
        ed->input_buf[ed->cursor++] = c;
        ++ed->input_len;
        ed->dirty = true;
        return;
    }
    switch (c) {
    case '\x1b':
        ed->state = STATE_ESC;
        return;
    case '\r':
    case '\n':
        ed->state = STATE_ACCEPTED;
        return;
    case '\b':
    case '\x7f': // ^H
        if (ed->cursor == 0)
            return;
        memmove(ed->input_buf + ed->cursor - 1, ed->input_buf + ed->cursor, ed->input_len - ed->cursor);
        ed->input_buf[--ed->input_len] = 0;
        --ed->cursor;
        ed->dirty = true;
        return;
    case 'U' - '@': // ^U
        memset(ed->input_buf, 0, ed->input_len);
        ed->input_len = ed->cursor = 0;
        ed->dirty = true;
        return;
    case 'D' - '@': // ^D
        if (ed->input_len == 0) {
            strcpy(ed->input_buf, "exit");
            ed->state = STATE_ACCEPTED;
            ed->dirty = true;
        }
        return;
    case 'L' - '@': // ^L
        dprintf(STDERR_FILENO, "\x1b[H\x1b[2J");
        ed->dirty = true;
        return;
    }
}

static void handle_state_esc(struct line_editor* ed, char c) {
    switch (c) {
    case '[':
        ed->param_len = 0;
        ed->state = STATE_CSI;
        return;
    }
    ed->state = STATE_GROUND;
    handle_ground(ed, c);
}

static void on_home(struct line_editor* ed) {
    ed->cursor = 0;
    ed->dirty = true;
}

static void on_end(struct line_editor* ed) {
    ed->cursor = ed->input_len;
    ed->dirty = true;
}

static void handle_csi_vt(struct line_editor* ed) {
    int code = atoi(ed->param_buf);
    switch (code) {
    case 1:
    case 7:
        on_home(ed);
        return;
    case 4:
    case 8:
        on_end(ed);
        return;
    case 3: // delete
        if (ed->cursor < ed->input_len) {
            memmove(ed->input_buf + ed->cursor, ed->input_buf + ed->cursor + 1, ed->input_len - ed->cursor - 1);
            ed->input_buf[--ed->input_len] = 0;
            ed->dirty = true;
        }
        return;
    }
}

static void handle_state_csi(struct line_editor* ed, char c) {
    if (c < 0x40) {
        ed->param_buf[ed->param_len++] = c;
        return;
    }
    ed->param_buf[ed->param_len] = '\0';

    switch (c) {
    case 'C': // right
        if (ed->cursor < ed->input_len) {
            ++ed->cursor;
            ed->dirty = true;
        }
        break;
    case 'D': // left
        if (ed->cursor > 0) {
            --ed->cursor;
            ed->dirty = true;
        }
        break;
    case 'H':
        on_home(ed);
        break;
    case 'F':
        on_end(ed);
        break;
    case '~':
        handle_csi_vt(ed);
        break;
    }

    ed->state = STATE_GROUND;
}

static void on_char(struct line_editor* ed, char c) {
    switch (ed->state) {
    case STATE_GROUND:
        handle_ground(ed, c);
        return;
    case STATE_ESC:
        handle_state_esc(ed, c);
        return;
    case STATE_CSI:
        handle_state_csi(ed, c);
        return;
    default:
        UNREACHABLE();
    }
}

static char* read_input(struct line_editor* ed, size_t terminal_width) {
    ed->state = STATE_GROUND;
    memset(ed->input_buf, 0, BUF_SIZE);
    ed->input_len = ed->cursor = 0;
    ed->dirty = true;

    char cwd_buf[BUF_SIZE];
    memset(cwd_buf, 0, BUF_SIZE);
    getcwd(cwd_buf, 1024);

    size_t prompt_len = strlen(cwd_buf) + 3;

    for (;;) {
        if (ed->dirty) {
            dprintf(STDERR_FILENO,
                    "\x1b[?25l"            // hide cursor
                    "\x1b[G"               // go to left end
                    "\x1b[36m%s\x1b[m $ ", // print prompt
                    cwd_buf);

            bool clear_needed = prompt_len + ed->input_len < terminal_width;
            size_t cursor_x = prompt_len + ed->cursor;

            if (cursor_x < terminal_width) {
                size_t len = MIN(ed->input_len, terminal_width - prompt_len);
                write(STDERR_FILENO, ed->input_buf, len);
            } else {
                const char* str = ed->input_buf + cursor_x - terminal_width + 1;
                size_t len =
                    MIN(ed->input_len, terminal_width - prompt_len + 1) - 1;
                if (ed->cursor == ed->input_len && cursor_x > terminal_width) {
                    --len;
                    clear_needed = true;
                }
                cursor_x = terminal_width;
                write(STDERR_FILENO, str, len);
            }

            if (clear_needed)
                dprintf(STDERR_FILENO, "\x1b[J");

            dprintf(STDERR_FILENO,
                    "\x1b[%uG"   // set cursor position
                    "\x1b[?25h", // show cursor
                    cursor_x + 1);
            ed->dirty = false;
        }

        char c;
        ssize_t nread = read(0, &c, 1);
        if (nread < 0)
            return NULL;
        if (nread == 0)
            continue;
        on_char(ed, c);

        if (ed->state == STATE_ACCEPTED)
            return ed->input_buf;
    }
}

#define MAX_ARGC 16

struct node {
    enum {
        CMD_EXECUTE,
        CMD_JUXTAPOSITION,
        CMD_PIPE,
        CMD_REDIRECT,
        CMD_BACKGROUND
    } type;

    union {
        struct execute_node {
            char* argv[MAX_ARGC];
            size_t lengths[MAX_ARGC];
        } execute;

        struct juxtaposition_node {
            struct node* left;
            struct node* right;
        } juxtaposition;

        struct pipe_node {
            struct node* left;
            struct node* right;
        } pipe;

        struct redirect_node {
            struct node* from;
            char* to;
            size_t to_length;
        } redirect;

        struct background_node {
            struct node* inner;
        } background;
    };
};

struct parser {
    char* cursor;
    enum {
        RESULT_SUCCESS,
        RESULT_EMPTY,
        RESULT_NOMEM_ERROR,
        RESULT_SYNTAX_ERROR,
        RESULT_TOO_MANY_ARGS_ERROR
    } result;
};

static char peek(struct parser* parser) { return *parser->cursor; }

static char consume(struct parser* parser) { return *parser->cursor++; }

static bool consume_if(struct parser* parser, char c) {
    if (*parser->cursor == c) {
        consume(parser);
        return true;
    }
    return false;
}

static char* consume_while(struct parser* parser, bool (*cond)(char)) {
    if (!*parser->cursor || !cond(*parser->cursor))
        return NULL;
    char* start = parser->cursor;
    do {
        ++parser->cursor;
    } while (cond(peek(parser)));
    return start;
}

static bool is_whitespace(char c) { return isspace(c); }

static void skip_whitespaces(struct parser* parser) {
    consume_while(parser, is_whitespace);
}

static bool is_valid_filename_character(char c) {
    switch (c) {
    case '>':
    case ' ':
    case '|':
    case '&':
    case ';':
        return false;
    }
    return ' ' <= c && c <= '~';
}

static char* parse_filename(struct parser* parser) {
    return consume_while(parser, is_valid_filename_character);
}

static struct node* parse_execute(struct parser* parser) {
    struct node* node = malloc(sizeof(struct node));
    if (!node) {
        parser->result = RESULT_NOMEM_ERROR;
        return NULL;
    }
    node->type = CMD_EXECUTE;

    struct execute_node* execute = &node->execute;
    size_t i = 0;
    for (;; ++i) {
        if (i >= MAX_ARGC) {
            parser->result = RESULT_TOO_MANY_ARGS_ERROR;
            return NULL;
        }
        char* arg = parse_filename(parser);
        if (!arg)
            break;
        char* saved_cursor = parser->cursor;
        skip_whitespaces(parser);
        execute->argv[i] = arg;
        execute->lengths[i] = saved_cursor - arg;
    }
    if (i == 0) {
        parser->result = RESULT_EMPTY;
        return NULL;
    }

    return node;
}

static struct node* parse_redirect(struct parser* parser) {
    struct node* from = parse_execute(parser);
    if (!from)
        return NULL;
    skip_whitespaces(parser);
    if (!consume_if(parser, '>'))
        return from;
    skip_whitespaces(parser);

    char* to = parse_filename(parser);
    if (!to) {
        parser->result = RESULT_SYNTAX_ERROR;
        return NULL;
    }

    struct node* node = malloc(sizeof(struct node));
    if (!node) {
        parser->result = RESULT_NOMEM_ERROR;
        return NULL;
    }
    node->type = CMD_REDIRECT;

    struct redirect_node* redirect = &node->redirect;
    redirect->from = from;
    redirect->to = to;
    redirect->to_length = parser->cursor - to;
    return node;
}

static struct node* parse_pipe(struct parser* parser) {
    struct node* left = parse_redirect(parser);
    if (!left)
        return NULL;
    skip_whitespaces(parser);
    if (!consume_if(parser, '|'))
        return left;
    skip_whitespaces(parser);

    struct node* right = parse_pipe(parser);
    if (!right) {
        if (parser->result == RESULT_EMPTY)
            parser->result = RESULT_SYNTAX_ERROR;
        return NULL;
    }

    struct node* node = malloc(sizeof(struct node));
    if (!node) {
        parser->result = RESULT_NOMEM_ERROR;
        return NULL;
    }
    node->type = CMD_PIPE;

    struct pipe_node* pipe = &node->pipe;
    pipe->left = left;
    pipe->right = right;
    return node;
}

static struct node* parse_juxtaposition(struct parser* parser) {
    struct node* left = parse_pipe(parser);
    if (!left)
        return NULL;
    skip_whitespaces(parser);

    if (consume_if(parser, '&')) {
        struct node* bg = malloc(sizeof(struct node));
        if (!bg) {
            parser->result = RESULT_NOMEM_ERROR;
            return NULL;
        }
        bg->type = CMD_BACKGROUND;
        bg->background.inner = left;
        left = bg;
    } else if (!consume_if(parser, ';')) {
        return left;
    }
    skip_whitespaces(parser);

    struct node* right = parse_juxtaposition(parser);
    if (!right) {
        if (parser->result == RESULT_EMPTY)
            parser->result = RESULT_SUCCESS;
        return left;
    }

    struct node* node = malloc(sizeof(struct node));
    if (!node) {
        parser->result = RESULT_NOMEM_ERROR;
        return NULL;
    }
    node->type = CMD_JUXTAPOSITION;

    struct juxtaposition_node* jux = &node->juxtaposition;
    jux->left = left;
    jux->right = right;
    return node;
}

static void null_terminate(struct node* node) {
    switch (node->type) {
    case CMD_EXECUTE: {
        char** argv = node->execute.argv;
        size_t* length = node->execute.lengths;
        while (*argv)
            (*argv++)[*length++] = 0;
        return;
    }
    case CMD_JUXTAPOSITION:
        null_terminate(node->juxtaposition.left);
        null_terminate(node->juxtaposition.right);
        return;
    case CMD_PIPE:
        null_terminate(node->pipe.left);
        null_terminate(node->pipe.right);
        return;
    case CMD_REDIRECT:
        null_terminate(node->redirect.from);
        (node->redirect.to)[node->redirect.to_length] = 0;
        return;
    case CMD_BACKGROUND:
        null_terminate(node->background.inner);
        return;
    }
    UNREACHABLE();
}

static struct node* parse(struct parser* parser, char* line) {
    parser->cursor = line;
    parser->result = RESULT_SUCCESS;

    skip_whitespaces(parser);
    struct node* node = parse_juxtaposition(parser);
    skip_whitespaces(parser);
    if (peek(parser) != 0) {
        parser->result = RESULT_SYNTAX_ERROR;
        return NULL;
    }
    if (!node)
        return NULL;

    if (node)
        null_terminate(node);
    return node;
}

enum {
    RUN_ERROR = -1,
    RUN_SIGNALED = -2,
};

struct run_context {
    char* const* envp;
    pid_t pgid;
    bool foreground;
};

static int run_command(const struct node* node, struct run_context ctx);

static int run_execute(const struct execute_node* node,
                       struct run_context ctx) {
    if (!strcmp(node->argv[0], "exit")) {
        dprintf(STDERR_FILENO, "exit\n");
        int status = node->argv[1] ? atoi(node->argv[1]) : 0;
        exit(status);
    }
    if (!strcmp(node->argv[0], "cd")) {
        if (node->argv[1])
            return chdir(node->argv[1]);
        const char* home = getenv("HOME");
        if (home)
            return chdir(home);
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0)
        return RUN_ERROR;
    if (pid == 0) {
        if (!ctx.pgid)
            ctx.pgid = getpid();
        if (setpgid(0, ctx.pgid) < 0) {
            perror("setpgpid");
            abort();
        }
        if (ctx.foreground)
            tcsetpgrp(STDERR_FILENO, ctx.pgid);
        if (execvpe(node->argv[0], node->argv, ctx.envp) < 0) {
            perror("execvpe");
            abort();
        }
        UNREACHABLE();
    }

    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0)
        return RUN_ERROR;
    if (WIFSIGNALED(wstatus))
        return RUN_SIGNALED;

    return 0;
}

static int run_juxtaposition(const struct juxtaposition_node* node,
                             struct run_context ctx) {
    int rc = run_command(node->left, ctx);
    if (rc < 0)
        return rc;
    return run_command(node->right, ctx);
}

static int run_pipe(const struct pipe_node* node, struct run_context ctx) {
    int pipefd[2];
    if (pipe(pipefd) < 0)
        return RUN_ERROR;

    pid_t pid = fork();
    if (pid < 0)
        return RUN_ERROR;
    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        if (!ctx.pgid)
            ctx.pgid = getpid();
        if (run_command(node->left, ctx) == RUN_ERROR) {
            perror("run_command");
            abort();
        }

        close(STDOUT_FILENO);
        exit(EXIT_SUCCESS);
    }

    if (!ctx.pgid)
        ctx.pgid = pid;

    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    int rc = run_command(node->right, ctx);
    if (rc < 0) {
        int saved_errno = errno;
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
        errno = saved_errno;
        return rc;
    }

    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);

    return waitpid(pid, NULL, 0);
}

static int run_redirect(const struct redirect_node* node,
                        struct run_context ctx) {
    int fd = open(node->to, O_WRONLY | O_CREAT, 0);
    if (fd < 0)
        return RUN_ERROR;
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    int rc = run_command(node->from, ctx);
    int saved_errno = errno;
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    errno = saved_errno;
    return rc;
}

static int run_background(const struct background_node* node,
                          struct run_context ctx) {
    pid_t pid = fork();
    if (pid < 0)
        return RUN_ERROR;
    if (pid == 0) {
        ctx.pgid = getpid();
        ctx.foreground = false;
        if (run_command(node->inner, ctx) == RUN_ERROR) {
            perror("run_command");
            abort();
        }
        exit(EXIT_SUCCESS);
    }
    return 0;
}

static int run_command(const struct node* node, struct run_context ctx) {
    switch (node->type) {
    case CMD_EXECUTE:
        return run_execute(&node->execute, ctx);
    case CMD_JUXTAPOSITION:
        return run_juxtaposition(&node->juxtaposition, ctx);
    case CMD_PIPE:
        return run_pipe(&node->pipe, ctx);
    case CMD_REDIRECT:
        return run_redirect(&node->redirect, ctx);
    case CMD_BACKGROUND:
        return run_background(&node->background, ctx);
    }
    UNREACHABLE();
}

int main(int argc, char* const argv[], char* const envp[]) {
    (void)argc;
    (void)argv;

    if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {
        perror("tcsetpgrp");
        return EXIT_FAILURE;
    }

    struct winsize winsize;
    if (ioctl(STDERR_FILENO, TIOCGWINSZ, &winsize) < 0) {
        winsize.ws_col = 80;
        winsize.ws_row = 25;
    }

    for (;;) {
        static struct line_editor editor;
        char* input = read_input(&editor, winsize.ws_col);
        if (!input) {
            perror("read_input");
            return EXIT_FAILURE;
        }
        dprintf(STDERR_FILENO, "\n");

        static struct parser parser;
        struct node* node = parse(&parser, input);
        switch (parser.result) {
        case RESULT_SUCCESS:
            ASSERT(node);
            break;
        case RESULT_EMPTY:
            continue;
        case RESULT_NOMEM_ERROR:
            dprintf(STDERR_FILENO, "Out of memory\n");
            return EXIT_FAILURE;
        case RESULT_SYNTAX_ERROR:
            dprintf(STDERR_FILENO, "Syntax error\n");
            continue;
        case RESULT_TOO_MANY_ARGS_ERROR:
            dprintf(STDERR_FILENO, "Too many arguments\n");
            continue;
        default:
            UNREACHABLE();
        }

        // reap previous background processes
        while (waitpid(-1, NULL, WNOHANG) >= 0)
            ;

        struct run_context ctx = {.envp = envp, .pgid = 0, .foreground = true};
        if (run_command(node, ctx) == RUN_ERROR) {
            perror("run_command");
            return EXIT_FAILURE;
        }

        if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {
            perror("tcsetpgrp");
            return EXIT_FAILURE;
        }

        // print 1 line worth of spaces so that we always ends up on a new line
        size_t num_spaces = winsize.ws_col - 1;
        char* spaces = malloc(num_spaces + 1);
        if (!spaces) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        memset(spaces, ' ', num_spaces);
        spaces[num_spaces] = 0;
        dprintf(STDERR_FILENO,
                "\x1b[?25l"            // hide cursor
                "\x1b[90;107m%%\x1b[m" // show end of line mark
                "%s"
                "\x1b[G"     // go to left end
                "\x1b[?25h", // show cursor
                spaces);
        free(spaces);
    }
    return EXIT_SUCCESS;
}
