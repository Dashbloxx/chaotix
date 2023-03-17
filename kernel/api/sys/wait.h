#pragma once

#define WEXITSTATUS(status) (((status)&0xff00) >> 8)
#define WTERMSIG(status) ((status)&0x7f)
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#define WIFSIGNALED(status)                                                    \
    ((0 < WTERMSIG(status)) && (WTERMSIG(status) < 0x7f))

#define WNOHANG 0x1
