/************************************************************************
 *File name: os_notify.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_POLL_H
#define OS_POLL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*os_poll_handler_f)(short when, os_socket_t fd, void *data);

os_pollset_t *os_pollset_create(unsigned int capacity);
void os_pollset_destroy(os_pollset_t *pollset);

#define OS_POLLIN      0x01
#define OS_POLLOUT     0x02

os_poll_t *os_pollset_add(os_pollset_t *pollset, short when, os_socket_t fd, os_poll_handler_f handler, void *data);
void os_pollset_remove(os_poll_t *poll);

void *os_pollset_self_handler_data(void);

typedef struct os_pollset_actions_s {
    void (*init)(os_pollset_t *pollset);
    void (*cleanup)(os_pollset_t *pollset);

    int (*add)(os_poll_t *poll);
    int (*remove)(os_poll_t *poll);

    int (*poll)(os_pollset_t *pollset, os_time_t timeout);
    int (*notify)(os_pollset_t *pollset);
} os_pollset_actions_t;

extern os_pollset_actions_t os_pollset_actions;

#define os_pollset_poll os_pollset_actions.poll
#define os_pollset_notify os_pollset_actions.notify

#ifdef __cplusplus
}
#endif

#endif
