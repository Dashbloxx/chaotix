#include "time.h"
#include "errno.h"
#include "stdio.h"
#include "sys/times.h"
#include <calendar.h>

clock_t clock(void) {
    struct tms tms;
    times(&tms);
    return tms.tms_utime + tms.tms_stime;
}

time_t time(time_t* tloc) {
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) < 0)
        return -1;
    if (tloc)
        *tloc = tp.tv_sec;
    return tp.tv_sec;
}

double difftime(time_t time1, time_t time0) { return (double)(time1 - time0); }

static int32_t divmodi64(int64_t a, int32_t b, int32_t* rem) {
    int32_t q;
    int32_t r;
    __asm__("idivl %[b]"
            : "=a"(q), "=d"(r)
            : "d"((int32_t)(a >> 32)),
              "a"((int32_t)(a & 0xffffffff)), [b] "rm"(b));
    *rem = r;
    return q;
}

struct tm* gmtime_r(const time_t* t, struct tm* tm) {
    static int const seconds_per_day = 60 * 60 * 24;

    time_t time = *t;

    unsigned year = 1970;
    for (;; ++year) {
        time_t seconds_in_this_year =
            (time_t)days_in_year(year) * seconds_per_day;
        if (time < seconds_in_this_year)
            break;
        time -= seconds_in_this_year;
    }
    tm->tm_year = year - 1900;

    int seconds;
    time_t days = divmodi64(time, seconds_per_day, &seconds);
    tm->tm_yday = days;
    tm->tm_sec = seconds % 60;

    int minutes = seconds / 60;
    tm->tm_hour = minutes / 60;
    tm->tm_min = minutes % 60;

    unsigned month;
    for (month = 1; month < 12; ++month) {
        time_t days_in_this_month = (time_t)days_in_month(year, month);
        if (days < days_in_this_month)
            break;
        days -= days_in_this_month;
    }

    tm->tm_mon = month - 1;
    tm->tm_mday = days + 1;
    tm->tm_wday = day_of_week(year, month, tm->tm_mday);

    return tm;
}

char* asctime_r(const struct tm* time_ptr, char* buf) {
    static const char* day_names[] = {"Sun", "Mon", "Tue", "Wed",
                                      "Thu", "Fri", "Sat"};
    static const char* month_names[] = {"Jan", "Feb", "Mar", "Apr",
                                        "May", "Jun", "Jul", "Aug",
                                        "Sep", "Oct", "Nov", "Dec"};
    int len = sprintf(
        buf, "%s %s %2d %02d:%02d:%02d %d", day_names[time_ptr->tm_wday],
        month_names[time_ptr->tm_mon], time_ptr->tm_mday, time_ptr->tm_hour,
        time_ptr->tm_min, time_ptr->tm_sec, time_ptr->tm_year + 1900);
    return len > 0 ? buf : NULL;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    int rc = clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
    if (rc > 0) {
        errno = -rc;
        return -1;
    }
    return 0;
}
