/************************************************************************
 *File name: os_sockopt.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "os_init.h"

#undef OS_LOG_DOMAIN
#define OS_LOG_DOMAIN __os_sock_domain

void os_sockopt_init(os_sockopt_t *option)
{
    os_assert(option);

    memset(option, 0, sizeof *option);

    option->sctp.spp_hbinterval = 5000;         /* 5 seconds */
    option->sctp.spp_sackdelay = 200;           /* 200 ms */
    option->sctp.srto_initial = 3000;           /* 3 seconds */
    option->sctp.srto_min = 1000;               /* 1 seconds */
    option->sctp.srto_max = 5000;               /* 5 seconds */
    option->sctp.sinit_num_ostreams = OS_DEFAULT_SCTP_MAX_NUM_OF_OSTREAMS;
    option->sctp.sinit_max_instreams = 65535;
    option->sctp.sinit_max_attempts = 4;
    option->sctp.sinit_max_init_timeo = 8000;   /* 8 seconds */

    option->sctp_nodelay = true;
    option->tcp_nodelay = true;
}

int os_nonblocking(os_socket_t fd)
{
#ifdef _WIN32
    int rc;
    os_assert(fd != INVALID_SOCKET);

    u_long io_mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &io_mode);
    if (rc != OS_OK) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "ioctlsocket failed");
        return OS_ERROR;
    }
#else
    int rc;
    int flags;
    os_assert(fd != INVALID_SOCKET);

    flags = fcntl(fd, F_GETFL, NULL);
    if (flags < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "F_GETFL failed");
        return OS_ERROR;
    }
    if (!(flags & O_NONBLOCK)) {
        rc = fcntl(fd, F_SETFL, (flags | O_NONBLOCK));
        if (rc != OS_OK) {
            OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "F_SETFL failed");
            return OS_ERROR;
        }
    }
#endif

    return OS_OK;
}

int os_closeonexec(os_socket_t fd)
{
#ifndef _WIN32
    int rc;
    int flags;

    os_assert(fd != INVALID_SOCKET);
    flags = fcntl(fd, F_GETFD, NULL);
    if (flags < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "F_GETFD failed");
        return OS_ERROR;
    }
    if (!(flags & FD_CLOEXEC)) {
        rc = fcntl(fd, F_SETFD, (flags | FD_CLOEXEC));
        if (rc != OS_OK) {
            OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "F_SETFD failed");
            return OS_ERROR;
        }
    }
#endif

    return OS_OK;
}

int os_listen_reusable(os_socket_t fd, int on)
{
#if defined(SO_REUSEADDR) && !defined(_WIN32)
    int rc;

    os_assert(fd != INVALID_SOCKET);

    OS_DEBUG("Turn on SO_REUSEADDR");
    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(int));
    if (rc != OS_OK) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(SOL_SOCKET, SO_REUSEADDR) failed");
        return OS_ERROR;
    }
#endif

    return OS_OK;
}

int os_tcp_nodelay(os_socket_t fd, int on)
{
#if defined(TCP_NODELAY) && !defined(_WIN32)
    int rc;

    os_assert(fd != INVALID_SOCKET);

    OS_DEBUG("Turn on TCP_NODELAY");
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(int));
    if (rc != OS_OK) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(IPPROTO_TCP, TCP_NODELAY) failed");
        return OS_ERROR;
    }
#endif

    return OS_OK;
}

int os_so_linger(os_socket_t fd, int l_linger)
{
#if defined(SO_LINGER) && !defined(_WIN32)
    struct linger l;
    int rc;

    os_assert(fd != INVALID_SOCKET);

    memset(&l, 0, sizeof(l));
    l.l_onoff = 1;
    l.l_linger = l_linger;

    OS_DEBUG("SO_LINGER:[%d]", l_linger);
    rc = setsockopt(fd, SOL_SOCKET, SO_LINGER,
            (void *)&l, sizeof(struct linger));
    if (rc != OS_OK) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(SOL_SOCKET, SO_LINGER) failed");
        return OS_ERROR;
    }
#endif

    return OS_OK;
}

int os_bind_to_device(os_socket_t fd, const char *device)
{
#if defined(SO_BINDTODEVICE) && !defined(_WIN32)
    int rc;

    os_assert(fd != INVALID_SOCKET);
    os_assert(device);

    OS_DEBUG("SO_BINDTODEVICE:[%s]", device);
    rc = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device)+1);
    if (rc != OS_OK) {
        int err = os_errno;
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(SOL_SOCKET, SO_BINDTODEVICE, %s) failed", device);
        if (err == OS_EPERM)
            OS_ERR("You need to grant CAP_NET_RAW privileges to use SO_BINDTODEVICE.");
        return OS_ERROR;
    }
#endif

    return OS_OK;
}
