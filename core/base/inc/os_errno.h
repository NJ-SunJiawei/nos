/************************************************************************
 *File name: os_errno.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_ERRNO_H
#define OS_ERRNO_H

OS_BEGIN_EXTERN_C

#if defined(_WIN32)

typedef DWORD os_err_t;

#define OS_EPERM                   ERROR_ACCESS_DENIED
#define OS_ENOMEM                  ERROR_NOT_ENOUGH_MEMORY
#define OS_EACCES                  ERROR_ACCESS_DENIED
#define OS_EEXIST                  ERROR_ALREADY_EXISTS
#define OS_EEXIST_FILE             ERROR_FILE_EXISTS
#define OS_ECONNRESET              WSAECONNRESET
#define OS_ETIMEDOUT               WSAETIMEDOUT
#define OS_ECONNREFUSED            WSAECONNREFUSED
#define OS_EBADF                   WSAEBADF
#define OS_EAGAIN                  WSAEWOULDBLOCK

#define os_errno                   GetLastError()
#define os_set_errno(err)          SetLastError(err)
#define os_socket_errno            WSAGetLastError()
#define os_set_socket_errno(err)   WSASetLastError(err)

#else

typedef int os_err_t;

#define OS_EPERM                   EPERM
#define OS_ENOMEM                  ENOMEM
#define OS_EACCES                  EACCES
#define OS_EEXIST                  EEXIST
#define OS_EEXIST_FILE             EEXIST
#define OS_ECONNRESET              ECONNRESET
#define OS_ETIMEDOUT               ETIMEDOUT
#define OS_ECONNREFUSED            ECONNREFUSED
#define OS_EBADF                   EBADF
#if (__hpux__)
#define OS_EAGAIN                  EWOULDBLOCK
#else
#define OS_EAGAIN                  EAGAIN
#endif

#define os_errno                   errno
#define os_socket_errno            errno
#define os_set_errno(err)          errno = err
#define os_set_socket_errno(err)   errno = err

#endif

#define OS_OK       0
#define OS_ERROR   -1
#define OS_RETRY   -2
#define OS_TIMEUP  -3
#define OS_DONE    -4

char *os_strerror(os_err_t err, char *buf, size_t size);

OS_END_EXTERN_C

#endif
