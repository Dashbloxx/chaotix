#pragma once

#include "errno.h"

// error value to pointer
#define ERR_PTR(x) ((void*)x)

// pointer to error value
#define PTR_ERR(x) ((long)x)

#define IS_ERR(x) ((uintptr_t)x >= (uintptr_t)-EMAXERRNO)
#define IS_OK(x) (!IS_ERR(x))

// error-valued pointer to pointer of another type
#define ERR_CAST(x) ((void*)x)
