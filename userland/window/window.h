#pragma once

#include <stdbool.h>

typedef unsigned short int wid_t;

struct window_properties {
    unsigned char enabled;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    char title[128];
    unsigned char visible;
};