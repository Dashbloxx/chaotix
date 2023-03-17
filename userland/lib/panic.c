#include "panic.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

noreturn void panic(const char* message, const char* file, size_t line) {
    dprintf(STDERR_FILENO, "%s at %s:%u\n", message, file, line);
    abort();
}
