#pragma once

#include <stddef.h>

int atoi(const char* str);
int abs(int i);

void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*));
