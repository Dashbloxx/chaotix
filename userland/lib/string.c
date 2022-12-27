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
