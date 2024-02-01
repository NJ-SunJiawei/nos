/************************************************************************
 *File name: os_sockpair.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SOCKPAIR_H
#define OS_SOCKPAIR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define AF_SOCKPAIR     AF_INET
#else
#define AF_SOCKPAIR     AF_UNIX
#endif

int os_socketpair(int family, int type, int protocol, os_socket_t fd[2]);

#ifdef __cplusplus
}
#endif

#endif
