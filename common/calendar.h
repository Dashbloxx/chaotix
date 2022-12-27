#pragma once

#include <stdbool.h>

static inline bool is_leap_year(unsigned year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static inline unsigned days_in_year(unsigned year) {
    return is_leap_year(year) ? 366 : 365;
}

// `month`: January is 1

static inline unsigned days_in_month(unsigned year, unsigned month) {
    static const unsigned table[] = {31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year))
        return 29;
    return table[month - 1];
}

static inline unsigned day_of_year(unsigned year, unsigned month,
                                   unsigned day) {
    unsigned result = day - 1;
    for (unsigned m = 1; m < month; ++m)
        result += days_in_month(year, m);
    return result;
}

static inline unsigned day_of_week(unsigned year, unsigned month,
                                   unsigned day) {
    // Zeller's congruence
    if (month <= 2) {
        --year;
        month += 12;
    }
    return (day + (13 * month + 8) / 5 + year + year / 4 - year / 100 +
            year / 400) %
           7;
}
