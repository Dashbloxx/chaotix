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

#include <kernel/api/err.h>
#include <kernel/api/errno.h>
#include <kernel/api/time.h>
#include <kernel/scheduler.h>
#include <kernel/system.h>

int sys_clock_gettime(clockid_t clk_id, struct timespec* tp) {
    switch (clk_id) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
        time_now(tp);
        return 0;
    default:
        return -EINVAL;
    }
}

static void timespec_add(struct timespec* this, const struct timespec* other) {
    this->tv_sec += other->tv_sec;
    this->tv_nsec += other->tv_nsec;
    if (this->tv_nsec >= 1000000000) {
        ++this->tv_sec;
        this->tv_nsec -= 1000000000;
    }
}

static void timespec_saturating_sub(struct timespec* this, const struct timespec* other) {
    this->tv_sec -= other->tv_sec;
    this->tv_nsec -= other->tv_nsec;
    if (this->tv_nsec < 0) {
        --this->tv_sec;
        this->tv_nsec += 1000000000;
    }
    if (this->tv_sec < 0)
        this->tv_sec = this->tv_nsec = 0;
}

static bool sleep_should_unblock(const struct timespec* deadline) {
    struct timespec now = {0};
    time_now(&now);
    return now.tv_sec > deadline->tv_sec || (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec);
}

int sys_clock_nanosleep(clockid_t clockid, int flags, const struct timespec* request, struct timespec* remain) {
    switch (clockid) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
        break;
    default:
        return -EINVAL;
    }

    struct timespec deadline = {0};
    switch (flags) {
    case 0: {
        int rc = time_now(&deadline);
        if (IS_ERR(rc))
            return rc;
        timespec_add(&deadline, request);
        break;
    }
    case TIMER_ABSTIME:
        deadline = *request;
        break;
    default:
        return -EINVAL;
    }

    int rc =
        scheduler_block((should_unblock_fn)sleep_should_unblock, &deadline);
    if (IS_ERR(rc))
        return rc;
    if (remain) {
        *remain = deadline;
        struct timespec now;
        int rc = time_now(&now);
        if (IS_ERR(rc))
            return rc;
        timespec_saturating_sub(remain, &now);
    }
    return 0;
}
