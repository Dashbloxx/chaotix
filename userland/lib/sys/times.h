#pragma once

#include <kernel/api/sys/times.h>

clock_t times(struct tms* buf);
