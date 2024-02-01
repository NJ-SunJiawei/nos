/************************************************************************
 *File name: os_poll.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#include "os_init.h"
#include "private/os_poll_priv.h"

extern const os_pollset_actions_t os_kqueue_actions;
extern const os_pollset_actions_t os_epoll_actions;
extern const os_pollset_actions_t os_select_actions;

static void *self_handler_data = NULL;

os_pollset_actions_t os_pollset_actions;
bool os_pollset_actions_initialized = false;

void *os_pollset_self_handler_data(void)
{
    return &self_handler_data;
}

os_pollset_t *os_pollset_create(unsigned int capacity)
{
    os_pollset_t *pollset = os_calloc(1, sizeof *pollset);
    if (!pollset) {
        OS_ERR("os_calloc() failed");
        return NULL;
    }

    pollset->capacity = capacity;

    os_pool_init(&pollset->pool, capacity);

    if (os_pollset_actions_initialized == false) {
#if defined(HAVE_KQUEUE)
        os_pollset_actions = os_kqueue_actions;
#elif defined(HAVE_EPOLL)
        os_pollset_actions = os_epoll_actions;
#else
        os_pollset_actions = os_select_actions;
#endif
        os_pollset_actions_initialized = true;
    }

    os_pollset_actions.init(pollset);

    return pollset;
}

void os_pollset_destroy(os_pollset_t *pollset)
{
    os_assert(pollset);

    os_pollset_actions.cleanup(pollset);

    os_pool_final(&pollset->pool);
    os_free(pollset);
}

os_poll_t *os_pollset_add(os_pollset_t *pollset, short when,
        os_socket_t fd, os_poll_handler_f handler, void *data)
{
    os_poll_t *poll = NULL;
    int rc;

    os_assert(pollset);

    os_assert(fd != INVALID_SOCKET);
    os_assert(handler);

    os_pool_alloc(&pollset->pool, &poll);
    os_assert(poll);

    rc = os_nonblocking(fd);
    os_assert(rc == OS_OK);
    rc = os_closeonexec(fd);
    os_assert(rc == OS_OK);

    poll->when = when;
    poll->fd = fd;
    poll->handler = handler;

    if (data == &self_handler_data)
        poll->data = poll;
    else
        poll->data = data;

    poll->pollset = pollset;

    rc = os_pollset_actions.add(poll);
    if (rc != OS_OK) {
        OS_ERR("cannot add poll");
        os_pool_free(&pollset->pool, poll);
        return NULL;
    }

    return poll;
}

void os_pollset_remove(os_poll_t *poll)
{
    int rc;
    os_pollset_t *pollset = NULL;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);

    rc = os_pollset_actions.remove(poll);
    if (rc != OS_OK) {
        OS_ERR("cannot delete poll");
    }

    os_pool_free(&pollset->pool, poll);
}
