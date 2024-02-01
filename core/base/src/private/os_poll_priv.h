/************************************************************************
 *File name: os_poll_priv.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#ifndef OS_POLL_PRIVATE_H
#define OS_POLL_PRIVATE_H

#if !defined(OS_BASE_COMPILATION)
#error "This private header cannot be included directly."
#endif

OS_BEGIN_EXTERN_C

typedef struct os_poll_s {
    os_lnode_t node;
    int index;

    short when;
    os_socket_t fd;
    os_poll_handler_f handler;
    void *data;

    os_pollset_t *pollset;
} os_poll_t;

typedef struct os_pollset_s {
    void *context;
    OS_POOL(pool, os_poll_t);

    struct {
        os_socket_t fd[2];
        os_poll_t *poll;
    } notify;

    unsigned int capacity;
} os_pollset_t;

OS_END_EXTERN_C

#endif
