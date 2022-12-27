#pragma once

#include "types.h"

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
};
