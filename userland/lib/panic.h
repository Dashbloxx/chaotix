#pragma once

#include <stddef.h>
#include <stdnoreturn.h>

#define PANIC(msg) panic("PANIC: " msg, __FILE__, __LINE__)
#define UNREACHABLE() PANIC("Unreachable")
#define UNIMPLEMENTED() PANIC("Unimplemented")
#define ASSERT(cond) ((cond) ? (void)0 : PANIC("Assertion failed: " #cond))
#define ASSERT_OK(result) ASSERT((result) >= 0)
#define ASSERT_ERR(result) ASSERT((result) < 0)

noreturn void panic(const char* message, const char* file, size_t line);
