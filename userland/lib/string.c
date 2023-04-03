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

#include "string.h"
#include "errno.h"
#include "signum.h"
#include "stdlib.h"

char* strdup(const char* src) {
    size_t len = strlen(src);
    char* buf = malloc((len + 1) * sizeof(char));
    if (!buf)
        return NULL;

    memcpy(buf, src, len);
    buf[len] = '\0';
    return buf;
}

#define ERRNO_MSG(I, MSG) MSG,
static const char* errno_msgs[EMAXERRNO] = {ENUMERATE_ERRNO(ERRNO_MSG)};
#undef ERRNO_MSG

char* strerror(int errnum) {
    if (0 <= errnum && errnum < EMAXERRNO)
        return (char*)errno_msgs[errnum];
    return "Unknown error";
}

const char* const sys_siglist[NSIG] = {
    "Invalid signal",
    "Hangup",
    "Interrupt",
    "Quit",
    "Illegal instruction",
    "Trace/breakpoint trap",
    "Aborted",
    "Bus error",
    "Floating point exception",
    "Killed",
    "User defined signal 1",
    "Segmentation violation",
    "User defined signal 2",
    "Broken pipe",
    "Alarm clock",
    "Terminated",
    "Stack fault",
    "Child exited",
    "Continued",
    "Stopped (signal)",
    "Stopped",
    "Stopped (tty input)",
    "Stopped (tty output)",
    "Urgent I/O condition)",
    "CPU time limit exceeded",
    "File size limit exceeded",
    "Virtual timer expired",
    "Profiling timer expired",
    "Window changed",
    "I/O possible",
    "Power failure",
    "Bad system call",
};

char* strsignal(int signum) {
    if (0 <= signum && signum < NSIG)
        return (char*)sys_siglist[signum];
    return "Unknown signal";
}
