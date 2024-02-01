/************************************************************************
 *File name: os_kqueue.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/queue.h>
#include <sys/event.h>

#include "os_init.h"
#include "private/os_poll_priv.h"

PRIVATE void kqueue_init(os_pollset_t *pollset);
PRIVATE void kqueue_cleanup(os_pollset_t *pollset);
PRIVATE int kqueue_add(os_poll_t *poll);
PRIVATE int kqueue_remove(os_poll_t *poll);
PRIVATE int kqueue_process(os_pollset_t *pollset, os_time_t timeout);

PRIVATE void kqueue_notify_init(os_pollset_t *pollset);
PRIVATE int kqueue_notify_pollset(os_pollset_t *pollset);

const os_pollset_actions_t os_kqueue_actions = {
    kqueue_init,
    kqueue_cleanup,

    kqueue_add,
    kqueue_remove,
    kqueue_process,

    kqueue_notify_pollset,
};

struct kqueue_context_s {
    int kqueue;

    struct kevent *change_list;
    struct kevent *event_list;
    int nchanges, nevents;
};

PRIVATE void kqueue_init(os_pollset_t *pollset)
{
    struct kqueue_context_s *context = NULL;
    os_assert(pollset);

    context = os_calloc(1, sizeof *context);
    os_assert(context);
    pollset->context = context;

    context->change_list = os_calloc(
        pollset->capacity, sizeof(struct kevent));
    os_assert(context->change_list);
    context->event_list = os_calloc(
        pollset->capacity, sizeof(struct kevent));
    os_assert(context->change_list);
    context->nchanges = 0;
    context->nevents = pollset->capacity;

    context->kqueue = kqueue();
    os_assert(context->kqueue != -1);

    kqueue_notify_init(pollset);
}

PRIVATE void kqueue_cleanup(os_pollset_t *pollset)
{
    struct kqueue_context_s *context = NULL;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    os_free(context->change_list);
    os_free(context->event_list);

    close(context->kqueue);

    os_free(context);
}

PRIVATE int kqueue_set(os_poll_t *poll, int filter, int flags)
{
    os_pollset_t *pollset = NULL;
    struct kqueue_context_s *context = NULL;
    struct kevent *kev;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    os_assert(context->nchanges < pollset->capacity);

    kev = &context->change_list[context->nchanges];
    memset(kev, 0, sizeof *kev);
    kev->ident = poll->fd;
    kev->filter = filter;
    kev->flags = flags;
    kev->udata = poll;

    poll->index = context->nchanges;
    context->nchanges++;

    return OS_OK;
}

PRIVATE int kqueue_add(os_poll_t *poll)
{
    int filter = 0;

    if (poll->when & OS_POLLIN) {
        filter = EVFILT_READ;
    }
    if (poll->when & OS_POLLOUT) {
        filter = EVFILT_WRITE;
    }

    return kqueue_set(poll, filter, EV_ADD|EV_ENABLE);
}

#if 0 /* os_pollset_remove() is not working, SHOULD remove the below code */
PRIVATE int kqueue_remove(os_poll_t *poll)
{
    os_pollset_t *pollset = NULL;
    struct kqueue_context_s *context = NULL;
    struct kevent *kev;
    os_poll_t *last = NULL;

    os_assert(poll);
    pollset = poll->pollset;
    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    os_assert(poll->index < context->nchanges);

    context->nchanges--;
    kev = &context->change_list[context->nchanges];

    os_assert(kev);
    context->change_list[poll->index] = *kev;

    last = kev->udata;
    os_assert(last);

    last->index = poll->index;

    return OS_OK;
}
#else /* New approach : os_pollset_remove() is properly working. */

PRIVATE int kqueue_remove(os_poll_t *poll)
{
    int filter = 0;

    if (poll->when & OS_POLLIN) {
        filter = EVFILT_READ;
    }
    if (poll->when & OS_POLLOUT) {
        filter = EVFILT_WRITE;
    }

    return kqueue_set(poll, filter, EV_DELETE);
}
#endif

PRIVATE int kqueue_process(os_pollset_t *pollset, os_time_t timeout)
{
    struct kqueue_context_s *context = NULL;
    struct timespec ts, *tp;
    int i, n;

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    if (timeout == OS_INFINITE_TIME) {
        tp = NULL;
    } else {
        ts.tv_sec = os_time_sec(timeout);
        ts.tv_nsec = os_time_usec(timeout) * 1000;
        tp = &ts;
    }

    n = kevent(context->kqueue,
            context->change_list, context->nchanges,
            context->event_list, context->nevents, tp);

    context->nchanges = 0;

    if (n < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno, "kqueue failed");
        return OS_ERROR;
    } else if (n == 0) {
        return OS_TIMEUP;
    }

    for (i = 0; i < n; i++) {
        os_poll_t *poll = NULL;
        short when = 0;

        if (context->event_list[i].flags & EV_ERROR) {
            switch (context->event_list[i].data) {

            /* Can occur on delete if we are not currently
             * watching any events on this fd.  That can
             * happen when the fd was closed and another
             * file was opened with that fd. */
            case ENOENT:
            /* Can occur for reasons not fully understood
             * on FreeBSD. */
            case EINVAL:
                continue;
#if defined(__FreeBSD__)
            /*
             * This currently occurs if an FD is closed
             * before the EV_DELETE makes it out via kevent().
             * The FreeBSD capabilities code sees the blank
             * capability set and rejects the request to
             * modify an event.
             *
             * To be strictly correct - when an FD is closed,
             * all the registered events are also removed.
             * Queuing EV_DELETE to a closed FD is wrong.
             * The event(s) should just be deleted from
             * the pending changelist.
             */
            case ENOTCAPABLE:
                continue;
#endif

            /* Can occur on a delete if the fd is closed. */
            case EBADF:
                /* XXXX On NetBSD, we can also get EBADF if we
                 * try to add the write side of a pipe, but
                 * the read side has already been closed.
                 * Other BSDs call this situation 'EPIPE'. It
                 * would be good if we had a way to report
                 * this situation. */
                continue;
            /* These two can occur on an add if the fd was one side
             * of a pipe, and the other side was closed. */
            case EPERM:
            case EPIPE:
                /* Report read events, if we're listening for
                 * them, so that the user can learn about any
                 * add errors.  (If the operation was a
                 * delete, then udata should be cleared.) */
                if (context->event_list[i].udata) {
                    /* The operation was an add:
                     * report the error as a read. */
                    when |= OS_POLLIN;
                    break;
                } else {
                    /* The operation was a del:
                     * report nothing. */
                    continue;
                }

            /* Other errors shouldn't occur. */
            default:
                OS_ERR("kevent() error : flags = 0x%x, errno = %d",
                        context->event_list[i].flags,
                        (int)context->event_list[i].data);
                return OS_ERROR;
            }
        } else if (context->event_list[i].filter == EVFILT_READ) {
            when |= OS_POLLIN;
        } else if (context->event_list[i].filter == EVFILT_WRITE) {
            when |= OS_POLLOUT;
        } else if (context->event_list[i].filter == EVFILT_USER) {
            /* Nothing */
        } else {
            OS_WARN("kevent() unknown filter = 0x%x\n",
                context->event_list[i].filter);
        }

        if (!when)
            continue;

        poll = (os_poll_t *)context->event_list[i].udata;
        os_assert(poll);

        if (poll->handler) {
            poll->handler(when, poll->fd, poll->data);
        }
    }
    
    return OS_OK;
}

#define NOTIFY_IDENT 42 /* Magic number */

PRIVATE void kqueue_notify_init(os_pollset_t *pollset)
{
    int rc;
    struct kqueue_context_s *context = NULL;
    struct kevent kev;
    struct timespec timeout = { 0, 0 };
    os_assert(pollset);

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    memset(&kev, 0, sizeof kev);
    kev.ident = NOTIFY_IDENT;
    kev.filter = EVFILT_USER;
    kev.flags = EV_ADD | EV_CLEAR;

    rc = kevent(context->kqueue, &kev, 1, NULL, 0, &timeout);
    os_assert(rc != -1);
}

PRIVATE int kqueue_notify_pollset(os_pollset_t *pollset)
{
    int rc;
    struct kqueue_context_s *context = NULL;
    struct kevent kev;
    struct timespec timeout = { 0, 0 };
    os_assert(pollset);

    os_assert(pollset);
    context = pollset->context;
    os_assert(context);

    memset(&kev, 0, sizeof kev);
    kev.ident = NOTIFY_IDENT;
    kev.filter = EVFILT_USER;
    kev.fflags = NOTE_TRIGGER;

    rc = kevent(context->kqueue, &kev, 1, NULL, 0, &timeout);
    if (rc == -1) {
        OS_WARN("kevent() failed");
        return OS_ERROR;
    }

    return OS_OK;
}
