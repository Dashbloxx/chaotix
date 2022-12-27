#include <calendar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    time_t now;
    if (time(&now) < 0) {
        perror("time");
        return EXIT_FAILURE;
    }
    struct tm tm;
    gmtime_r(&now, &tm);

    static const char* month_names[] = {"Jan", "Feb", "Mar", "Apr",
                                        "May", "Jun", "Jul", "Aug",
                                        "Sep", "Oct", "Nov", "Dec"};
    printf("      %s %d\nSu Mo Tu We Th Fr Sa\n", month_names[tm.tm_mon],
           tm.tm_year + 1900);

    int wday_of_first_day_of_this_month = tm.tm_wday - (tm.tm_mday - 1) % 7;
    for (int i = 0; i < wday_of_first_day_of_this_month; ++i)
        printf("   ");

    int days_in_this_month = days_in_month(tm.tm_year + 1900, tm.tm_mon + 1);
    int wday = wday_of_first_day_of_this_month;
    for (int mday = 1; mday <= days_in_this_month; ++mday) {
        if (++wday >= 7) {
            printf("%2d\n", mday);
            wday = 0;
        } else {
            printf("%2d ", mday);
        }
    }
    if (wday > 0)
        printf("\n");

    return EXIT_SUCCESS;
}
