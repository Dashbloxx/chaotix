/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
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

#include "api/time.h"
#include "asm_wrapper.h"
#include "panic.h"
#include "system.h"
#include <common/calendar.h>

static uint8_t cmos_read(uint8_t idx) {
    /*
     *  Only i?86 uses instructions like `outb`, `outl`, `outw`, `inb`, `inl`, and `inw` (I believe). Therefore, we use a macro to isolate the code here to
     *  i?86 processors only.
     */
    #if defined(__i386__)
    out8(0x70, idx);
    return in8(0x71);
    #endif
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0xf) + ((bcd >> 4) * 10);
}

/* Return the amount of days since epoch. The epoch in this context, is since midnight of Janurary 1st, 1970. */
static unsigned days_since_epoch(unsigned year, unsigned month, unsigned day) {
    ASSERT(year >= 1970);
    unsigned days = day_of_year(year, month, day);
    for (unsigned y = 1970; y < year; ++y)
        days += days_in_year(y);
    return days;
}


static time_t rtc_now(void) {
    int timeout = 100;
    bool update_finished = false;
    while (--timeout >= 0) {
        if (!(cmos_read(0x0a) & 0x80)) {
            update_finished = true;
            break;
        }
        delay(1000);
    }

    unsigned year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;

    if (update_finished) {
        uint8_t status = cmos_read(0xb);
        second = cmos_read(0x0);
        minute = cmos_read(0x2);
        hour = cmos_read(0x4);
        day = cmos_read(0x7);
        month = cmos_read(0x8);
        year = cmos_read(0x9);

        if (!(status & 0x4)) {
            second = bcd_to_bin(second);
            minute = bcd_to_bin(minute);
            hour = bcd_to_bin(hour & 0x7f);
            day = bcd_to_bin(day);
            month = bcd_to_bin(month);
            year = bcd_to_bin(year);
        }
        if (!(status & 0x2)) {
            hour %= 12;
            if (hour & 0x80)
                hour += 12;
        }

        year += 2000;
    }

    time_t days = days_since_epoch(year, month, day);
    time_t hours = days * 24 + hour;
    time_t minutes = hours * 60 + minute;
    return minutes * 60 + second;
}

static struct timespec now;

void time_init(void) {
    now.tv_sec = rtc_now();
    now.tv_nsec = 0;
}

void time_tick(void) {
    static const long nanos = 1000000000;
    now.tv_nsec += nanos / CLK_TCK;
    if (now.tv_nsec >= nanos) {
        ++now.tv_sec;
        now.tv_nsec -= nanos;
    }
}

int time_now(struct timespec* tp) {
    *tp = now;
    return 0;
}
