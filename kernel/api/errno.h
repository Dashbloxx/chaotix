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
