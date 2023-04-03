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
    printf("      %s %d\nSu Mo Tu We Th Fr Sa\n", month_names[tm.tm_mon], tm.tm_year + 1900);

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
