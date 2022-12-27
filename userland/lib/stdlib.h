#pragma once

#include <common/stdlib.h>
#include <stddef.h>
#include <stdnoreturn.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX 2147483647

noreturn void exit(int status);
noreturn void abort(void);

void* malloc(size_t size);
void* aligned_alloc(size_t alignment, size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);

char* getenv(const char* name);

int rand(void);
void srand(unsigned seed);
