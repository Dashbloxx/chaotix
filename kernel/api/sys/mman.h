#pragma once

#define PROT_READ 0x2
#define PROT_WRITE 0x4

#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x4
#define MAP_ANONYMOUS 0x8
#define MAP_ANON MAP_ANONYMOUS

#define MAP_FAILED ((void*)-1)
