#pragma once

#include <stdarg.h>
#include <stddef.h>

int sprintf(char* buffer, const char* format, ...);
int snprintf(char* buffer, size_t bufsz, const char* format, ...);
int vsprintf(char* buffer, const char* format, va_list args);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
