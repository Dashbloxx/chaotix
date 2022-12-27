#pragma once

#include <common/string.h>

char* strdup(const char* src);

char* strerror(int errnum);

extern const char* const sys_siglist[];
char* strsignal(int signum);
