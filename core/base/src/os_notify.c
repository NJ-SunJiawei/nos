/************************************************************************
 *File name: os_notify.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#include "os_init.h"
#include "private/os_poll_priv.h"

PRIVATE void os_drain_pollset(short when, os_socket_t fd, void *data);

void os_notify_init(os_pollset_t *pollset)
{
#if !defined(HAVE_EVENTFD)
    int rc;
#endif
    os_assert(pollset);

#if defined(HAVE_EVENTFD)
    pollset->notify.fd[0] = eventfd(0, 0);
    os_assert(pollset->notify.fd[0] != INVALID_SOCKET);
#else
    rc = os_socketpair(AF_SOCKPAIR, SOCK_STREAM, 0, pollset->notify.fd);
    os_assert(rc == OS_OK);
#endif

    pollset->notify.poll = os_pollset_add(pollset, OS_POLLIN,
            pollset->notify.fd[0], os_drain_pollset, NULL);
    os_assert(pollset->notify.poll);
}

void os_notify_final(os_pollset_t *pollset)
{
    os_assert(pollset);

    os_pollset_remove(pollset->notify.poll);

    os_closesocket(pollset->notify.fd[0]);
#if !defined(HAVE_EVENTFD)
    os_closesocket(pollset->notify.fd[1]);
#endif
}

int os_notify_pollset(os_pollset_t *pollset)
{
    ssize_t r;
#if defined(HAVE_EVENTFD)
    uint64_t msg = 1;
#else
    char buf[1];
    buf[0] = 0;
#endif

    os_assert(pollset);

#if defined(HAVE_EVENTFD)
    r = write(pollset->notify.fd[0], (void*)&msg, sizeof(msg));
#else
    r = send(pollset->notify.fd[1], buf, 1, 0);
#endif

    if (r < 0) {
       	os_logsp(ERROR, ERRNOID, os_socket_errno, "notify failed");
        return OS_ERROR;
    }

    return OS_OK;
}

PRIVATE void os_drain_pollset(short when, os_socket_t fd, void *data)
{
    ssize_t r;
#if defined(HAVE_EVENTFD)
    uint64_t msg;
#else
    unsigned char buf[1024];
#endif

    os_assert(when == OS_POLLIN);

#if defined(HAVE_EVENTFD)
    r = read(fd, (char *)&msg, sizeof(msg));
#else
    r = recv(fd, (char *)buf, sizeof(buf), 0);
#endif
    if (r < 0) {
        os_logsp(ERROR, ERRNOID, os_socket_errno, "drain failed");
    }
}
