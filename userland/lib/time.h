#pragma once

#include <kernel/api/time.h>

clock_t clock(void);

time_t time(time_t* tloc);

double difftime(time_t time1, time_t time0);

struct tm* gmtime_r(time_t const* t, struct tm* tm);
char* asctime_r(const struct tm* time_ptr, char* buf);

int nanosleep(const struct timespec* req, struct timespec* rem);

int clock_gettime(clockid_t clk_id, struct timespec* tp);
int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec* request, struct timespec* remain);
