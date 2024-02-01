/************************************************************************
 *File name: os_select.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "os_init.h"
#include "private/os_poll_priv.h"

static void select_init(os_pollset_t *pollset);
static void select_cleanup(os_pollset_t *pollset);
static int select_add(os_poll_t *poll);
static int select_remove(os_poll_t *poll);
static int select_process(os_pollset_t *pollset, os_time_t timeout);

const os_pollset_actions_t os_select_actions = {
    select_init,
    select_cleanup,

    select_add,
    select_remove,
    select_process,

    os_notify_pollset,
};

struct select_context_s {
    int max_fd;
    fd_set master_read_fd_set;
    fd_set master_write_fd_set;
    fd_set work_read_fd_set;
    fd_set work_write_fd_set;

    os_list_t list;
};

static void select_init(os_pollset_t *pollset)
{
    struct select_context_s *context = NULL;
    os_assert(pollset);

    context = os_calloc(1, sizeof *context);
    os_assert(context);
    pollset->context = context;

    os_list_init(&context->list);

    context->max_fd = -1;
    FD_ZERO(&context->master_read_fd_set);
    FD_ZERO(&context->master_write_fd_set);

    os_notify_init(pollset);
}

static void select_cleanup(os_pollset_t *pollset)
{
    struct select_context_s *context = NULL;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    os_notify_final(pollset);
    os_free(context);
}

static int select_add(os_poll_t *poll)
{
    os_pollset_t *pollset = NULL;
    struct select_context_s *context = NULL;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    if (poll->when & OS_POLLIN) {
        FD_SET(poll->fd, &context->master_read_fd_set);
    }

    if (poll->when & OS_POLLOUT) {
        FD_SET(poll->fd, &context->master_write_fd_set);
    }

    if (poll->fd > context->max_fd)
        context->max_fd = poll->fd;

    os_list_add(&context->list, poll);

    return OS_OK;
}

static int select_remove(os_poll_t *poll)
{
    os_pollset_t *pollset = NULL;
    struct select_context_s *context = NULL;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    if (poll->when & OS_POLLIN)
        FD_CLR(poll->fd, &context->master_read_fd_set);

    if (poll->when & OS_POLLOUT)
        FD_CLR(poll->fd, &context->master_write_fd_set);

    if (context->max_fd == poll->fd) {
        context->max_fd = -1;
    }

    os_list_remove(&context->list, poll);

    return OS_OK;
}

static int select_process(os_pollset_t *pollset, os_time_t timeout)
{
    struct select_context_s *context = NULL;
    os_poll_t *poll = NULL, *next_poll = NULL;
    int rc;
    struct timeval tv, *tp;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    if (context->max_fd == -1) {
        os_list_for_each(&context->list, poll) {
            if (context->max_fd < poll->fd) {
                context->max_fd = poll->fd;
            }
        }
        OS_DEBUG("change max_fd: %d", context->max_fd);
    }

    context->work_read_fd_set = context->master_read_fd_set;
    context->work_write_fd_set = context->master_write_fd_set;

    if (timeout == OS_INFINITE_TIME) {
        tp = NULL;
    } else {
        tv.tv_sec = os_time_sec(timeout);
#if defined(_WIN32) /* I don't know why windows need more time */
        tv.tv_usec = os_time_usec(timeout) + os_time_from_msec(1);
#else
        tv.tv_usec = os_time_usec(timeout);
#endif

        tp = &tv;
    }

    rc = select(context->max_fd + 1,
            &context->work_read_fd_set, &context->work_write_fd_set, NULL, tp);
    if (rc < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "select() failed");
        return OS_ERROR;
    } else if (rc == 0) {
        return OS_TIMEUP;
    }

    os_list_for_each_safe(&context->list, next_poll, poll) {
        short when = 0;
        if ((poll->when & OS_POLLIN) &&
            FD_ISSET(poll->fd, &context->work_read_fd_set)) {
            when |= OS_POLLIN;
        }

        if ((poll->when & OS_POLLOUT) &&
            FD_ISSET(poll->fd, &context->work_write_fd_set)) {
            when |= OS_POLLOUT;
        }

        if (when && poll->handler) {
            poll->handler(when, poll->fd, poll->data);
        }
    }
    
    return OS_OK;
}
