/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
 *
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#pragma once

#define ENUMERATE_ERRNO(F)                                                     \
    F(ESUCCESS, "Success")                                                     \
    F(EPERM, "Operation not permitted")                                        \
    F(ENOENT, "No such file or directory")                                     \
    F(ESRCH, "No such process")                                                \
    F(EINTR, "Interrupted system call")                                        \
    F(EIO, "I/O error")                                                        \
    F(ENXIO, "No such device or address")                                      \
    F(E2BIG, "Arg list too long")                                              \
    F(ENOEXEC, "Exec format error")                                            \
    F(EBADF, "Bad file number")                                                \
    F(ECHILD, "No child processes")                                            \
    F(EAGAIN, "Try again")                                                     \
    F(ENOMEM, "Out of memory")                                                 \
    F(EACCES, "Permission denied")                                             \
    F(EFAULT, "Bad address")                                                   \
    F(ENOTBLK, "Block device required")                                        \
    F(EBUSY, "Device or resource busy")                                        \
    F(EEXIST, "File exists")                                                   \
    F(EXDEV, "Cross-device link")                                              \
    F(ENODEV, "No such device")                                                \
    F(ENOTDIR, "Not a directory")                                              \
    F(EISDIR, "Is a directory")                                                \
    F(EINVAL, "Invalid argument")                                              \
    F(ENFILE, "File table overflow")                                           \
    F(EMFILE, "Too many open files")                                           \
    F(ENOTTY, "Not a TTY")                                                     \
    F(ETXTBSY, "Text file busy")                                               \
    F(EFBIG, "File too large")                                                 \
    F(ENOSPC, "No space left on device")                                       \
    F(ESPIPE, "Illegal seek")                                                  \
    F(EROFS, "Read-only file system")                                          \
    F(EMLINK, "Too many links")                                                \
    F(EPIPE, "Broken pipe")                                                    \
    F(EDOM, "Math argument out of domain of function")                         \
    F(ERANGE, "Math result not representable")                                 \
    F(ENOMSG, "No message of desired type")                                    \
    F(EDEADLK, "Resource deadlock condition")                                  \
    F(ENOLCK, "No record locks available")                                     \
    F(EPROTO, "Protocol error")                                                \
    F(ENOSYS, "Function not implemented")                                      \
    F(ENOTEMPTY, "Directory not empty")                                        \
    F(ENAMETOOLONG, "File name too long")                                      \
    F(ELOOP, "Symbolic link loop")                                             \
    F(EOPNOTSUPP, "Operation not supported on transport end-point")            \
    F(EPFNOSUPPORT, "Protocol family not supported")                           \
    F(ECONNRESET, "Connection reset by peer")                                  \
    F(ENOBUFS, "No buffer space available")                                    \
    F(EAFNOSUPPORT, "Address family not supported by protocol")                \
    F(EPROTOTYPE, "Protocol wrong type for socket")                            \
    F(ENOTSOCK, "Socket operation on non-socket")                              \
    F(ENOPROTOOPT, "Protocol not available")                                   \
    F(ESHUTDOWN, "Cannot send after transport endpoint shutdown")              \
    F(ECONNREFUSED, "Connection refused")                                      \
    F(EADDRINUSE, "Address already in use")                                    \
    F(ECONNABORTED, "Software caused connection abort")                        \
    F(ENETUNREACH, "Network is unreachable")                                   \
    F(ENETDOWN, "Network is down")                                             \
    F(ETIMEDOUT, "Connection timed out")                                       \
    F(EHOSTDOWN, "Node is down")                                               \
    F(EHOSTUNREACH, "No route to node")                                        \
    F(EINPROGRESS, "Operation now in progress")                                \
    F(EALREADY, "Operation already in progress")                               \
    F(EDESTADDRREQ, "Destination address required")                            \
    F(EMSGSIZE, "Message too long")                                            \
    F(EPROTONOSUPPORT, "Protocol wrong type for socket")                       \
    F(ESOCKTNOSUPPORT, "Socket type not supported")                            \
    F(EADDRNOTAVAIL, "Cannot assign requested address")                        \
    F(ENETRESET, "Network dropped connection because of reset")                \
    F(EISCONN, "Transport endpoint is already connected")                      \
    F(ENOTCONN, "Transport endpoint is not connected")                         \
    F(ETOOMANYREFS, "Too many references")                                     \
    F(EDQUOT, "Disc quota exceeded")                                           \
    F(ENOTSUP, "Not supported")                                                \
    F(EILSEQ, "Illegal byte sequence")                                         \
    F(EOVERFLOW, "Value too large for defined data type")                      \
    F(ECANCELED, "Operation canceled")                                         \
    F(ENOTRECOVERABLE, "Lock is not recoverable")

#define ENUM_ITEM(I, MSG) I,
enum { ENUMERATE_ERRNO(ENUM_ITEM) EMAXERRNO };
#undef ENUM_ITEM
