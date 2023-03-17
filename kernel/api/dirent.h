#pragma once

#include "sys/types.h"

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

struct dirent {
    uint16_t d_reclen;
    uint8_t d_type;
    uint8_t d_namlen;
    char d_name[];
};
