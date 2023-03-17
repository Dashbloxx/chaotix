#pragma once

#include <kernel/api/sys/types.h>
#include <kernel/api/sys/wait.h>

pid_t waitpid(pid_t pid, int* wstatus, int options);
