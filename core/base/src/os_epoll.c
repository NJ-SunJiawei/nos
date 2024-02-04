/************************************************************************
 *File name: os_epoll.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/epoll.h>

#include "os_init.h"
#include "private/os_poll_priv.h"

PRIVATE void epoll_init(os_pollset_t *pollset);
PRIVATE void epoll_cleanup(os_pollset_t *pollset);
PRIVATE int epoll_add(os_poll_t *poll);
PRIVATE int epoll_remove(os_poll_t *poll);
PRIVATE int epoll_process(os_pollset_t *pollset, os_time_t timeout);

const os_pollset_actions_t os_epoll_actions = {
    epoll_init,
    epoll_cleanup,

    epoll_add,
    epoll_remove,
    epoll_process,

    os_notify_pollset,
};

struct epoll_map_s {
    os_poll_t *read;
    os_poll_t *write;
};

struct epoll_context_s {
    int epfd;

    os_hash_t *map_hash;
    struct epoll_event *event_list;
};

PRIVATE void epoll_init(os_pollset_t *pollset)
{
    struct epoll_context_s *context = NULL;
    os_assert(pollset);

    context = os_calloc(1, sizeof *context);
    os_assert(context);
    pollset->context = context;

    context->event_list = os_calloc(
            pollset->capacity, sizeof(struct epoll_event));
    os_assert(context->event_list);

    context->map_hash = os_hash_make();
    os_assert(context->map_hash);

    context->epfd = epoll_create(pollset->capacity);
    os_assert(context->epfd >= 0);

    os_notify_init(pollset);
}

PRIVATE void epoll_cleanup(os_pollset_t *pollset)
{
    struct epoll_context_s *context = NULL;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    os_notify_final(pollset);
    close(context->epfd);
    os_free(context->event_list);
    os_hash_destroy(context->map_hash);

    os_free(context);
}

PRIVATE int epoll_add(os_poll_t *poll)
{
    int rv, op;
    os_pollset_t *pollset = NULL;
    struct epoll_context_s *context = NULL;
    struct epoll_map_s *map = NULL;
    struct epoll_event ee;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    map = os_hash_get(context->map_hash, &poll->fd, sizeof(poll->fd));
    if (!map) {
        map = os_calloc(1, sizeof(*map));
        if (!map) {
            os_log0(ERROR, "os_calloc() failed");
            return OS_ERROR;
        }

        op = EPOLL_CTL_ADD;
        os_hash_set(context->map_hash, &poll->fd, sizeof(poll->fd), map);
    } else {
        op = EPOLL_CTL_MOD;
    }

    if (poll->when & OS_POLLIN)
        map->read = poll;
    if (poll->when & OS_POLLOUT)
        map->write = poll;

    memset(&ee, 0, sizeof ee);

    ee.events = 0;
    if (map->read)
        ee.events |= (EPOLLIN|EPOLLRDHUP);
    if (map->write)
        ee.events |= EPOLLOUT;
    ee.data.fd = poll->fd;

    rv = epoll_ctl(context->epfd, op, poll->fd, &ee);
    if (rv < 0) {
        os_logp1(ERROR, ERRNOID, os_socket_errno, "epoll_ctl[%d] failed", op);
        return OS_ERROR;
    }

    return OS_OK;
}

PRIVATE int epoll_remove(os_poll_t *poll)
{
    int rv, op;
    os_pollset_t *pollset = NULL;
    struct epoll_context_s *context = NULL;
    struct epoll_map_s *map = NULL;
    struct epoll_event ee;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    map = os_hash_get(context->map_hash, &poll->fd, sizeof(poll->fd));
    os_assert(map);

    if (poll->when & OS_POLLIN)
        map->read = NULL;
    if (poll->when & OS_POLLOUT)
        map->write = NULL;

    memset(&ee, 0, sizeof ee);

    ee.events = 0;
    if (map->read)
        ee.events |= (EPOLLIN|EPOLLRDHUP);
    if (map->write)
        ee.events |= EPOLLOUT;

    if (map->read || map->write) {
        op = EPOLL_CTL_MOD;
        ee.data.fd = poll->fd;
    } else {
        op = EPOLL_CTL_DEL;
        ee.data.fd = INVALID_SOCKET;

        os_hash_set(context->map_hash, &poll->fd, sizeof(poll->fd), NULL);
        os_free(map);
    }

    rv = epoll_ctl(context->epfd, op, poll->fd, &ee);
    if (rv < 0) {
        os_logp1(ERROR, ERRNOID, os_socket_errno, "epoll_remove[%d] failed", op);
        return OS_ERROR;
    }

    return OS_OK;
}

PRIVATE int epoll_process(os_pollset_t *pollset, os_time_t timeout)
{
    struct epoll_context_s *context = NULL;
    int num_of_poll;
    int i;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    num_of_poll = epoll_wait(context->epfd, context->event_list,
            pollset->capacity,
            timeout == OS_INFINITE_TIME ? OS_INFINITE_TIME :
                os_time_to_msec(timeout));
    if (num_of_poll < 0) {
        os_logp0(ERROR, ERRNOID, os_socket_errno, "epoll failed");
        return OS_ERROR;
    } else if (num_of_poll == 0) {
        return OS_TIMEUP;
    }

    for (i = 0; i < num_of_poll; i++) {
        struct epoll_map_s *map = NULL;
        uint32_t received;
        short when = 0;
        os_socket_t fd;

        received = context->event_list[i].events;
        if (received & EPOLLERR) {
        /*
         * The libevent library has OS_POLLOUT turned on in EPOLLERR.
         *
         * However, SIGPIPE can occur if write() is called
         * when the peer connection is closed.
         *
         * Therefore, Open5GS turns off OS_POLLOUT
         * so that write() cannot be called in case of EPOLLERR.
         *
         * See also #2411 and #2312
         */
#if 0
            when = OS_POLLIN|OS_POLLOUT;
#else
            when = OS_POLLIN;
#endif
        } else if ((received & EPOLLHUP) && !(received & EPOLLRDHUP)) {
            when = OS_POLLIN|OS_POLLOUT;
        } else {
            if (received & EPOLLIN) {
                when |= OS_POLLIN;
            }
            if (received & EPOLLOUT) {
                when |= OS_POLLOUT;
            }
            if (received & EPOLLRDHUP) {
                when |= OS_POLLIN;
                when &= ~OS_POLLOUT;
            }
        }

        if (!when)
            continue;

        fd = context->event_list[i].data.fd;
        os_assert(fd != INVALID_SOCKET);

        map = os_hash_get(context->map_hash, &fd, sizeof(fd));
        if (!map) continue;

        if (map->read && map->write && map->read == map->write) {
            map->read->handler(when, map->read->fd, map->read->data);
        } else {
            if ((when & OS_POLLIN) && map->read)
                map->read->handler(when, map->read->fd, map->read->data);

            /*
             * map->read->handler() can call os_remove_epoll()
             * So, we need to check map instance
             */
            map = os_hash_get(context->map_hash, &fd, sizeof(fd));
            if (!map) continue;

            if ((when & OS_POLLOUT) && map->write)
                map->write->handler(when, map->write->fd, map->write->data);
        }
    }
    
    return OS_OK;
}
