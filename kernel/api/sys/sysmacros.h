#pragma once

#define makedev(major, minor)                                                  \
    (((minor)&0xffu) | ((major) << 8u) | (((minor) & ~0xffu) << 12u))
#define major(dev) (((dev)&0xfff00u) >> 8u)
#define minor(dev) (((dev)&0xffu) | (((dev) >> 12u) & 0xfff00u))
