#pragma once

#define F_DUPFD 0
#define F_GETFL 3
#define F_SETFL 4

#define O_RDONLY 0x1
#define O_WRONLY 0x2
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_CREAT 0x8
#define O_EXCL 0x10
#define O_NONBLOCK 0x100
