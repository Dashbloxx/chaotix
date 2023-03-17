#include "boot_defs.h"
#include "kprintf.h"
#include "multiboot.h"
#include "system.h"
#include <common/string.h>
//#include <string.h>

#define MAX_CMDLINE_LEN 1024
#define MAX_NUM_KEYS 1024

static char raw[MAX_CMDLINE_LEN];
static char buf[MAX_CMDLINE_LEN];
static size_t num_keys = 0;
static char* keys[MAX_NUM_KEYS];
static char* values[MAX_NUM_KEYS];

void cmdline_init(const multiboot_info_t* mb_info) {
    if (!(mb_info->flags & MULTIBOOT_INFO_CMDLINE))
        return;

    const char* str = (const char*)(mb_info->cmdline + KERNEL_VADDR);
    kprintf("Kernel cmdline: \"%s\"\n", str);
    strlcpy(raw, str, MAX_CMDLINE_LEN);
    strlcpy(buf, str, MAX_CMDLINE_LEN);

    char* saved_ptr;
    static const char* sep = " ";
    for (char* token = strtok_r(buf, sep, &saved_ptr); token;
         token = strtok_r(NULL, sep, &saved_ptr)) {
        keys[num_keys] = token;

        char* next_equal = strchr(token, '=');
        char* next_space = strchr(token, ' ');
        if (next_equal) {
            if (!next_space || next_equal < next_space)
                values[num_keys] = next_equal + 1;
        }

        ++num_keys;
    }

    // null terminate
    for (size_t i = 0; i < num_keys; ++i) {
        char* space = strchr(keys[i], ' ');
        if (space)
            *space = 0;
        if (values[i]) {
            char* equal = strchr(keys[i], '=');
            if (equal)
                *equal = 0;
        }
    }
}

const char* cmdline_get_raw(void) { return raw; }

const char* cmdline_lookup(const char* key) {
    if (num_keys == 0)
        return NULL;
    for (size_t i = 0; i < num_keys; ++i) {
        if (!strcmp(keys[i], key))
            return values[i];
    }
    return NULL;
}

bool cmdline_contains(const char* key) {
    if (num_keys == 0)
        return false;
    for (size_t i = 0; i < num_keys; ++i) {
        if (!strcmp(keys[i], key))
            return true;
    }
    return false;
}
