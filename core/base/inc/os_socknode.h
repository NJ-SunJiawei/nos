/************************************************************************
 *File name: os_socknode.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SOCKNODE_H
#define OS_SOCKNODE_H

OS_BEGIN_EXTERN_C

typedef struct os_socknode_s {
    os_lnode_t node;

    os_sockaddr_t *addr;
    char *dev;

    os_sock_t *sock;
    void (*cleanup)(os_sock_t *sock);
    os_poll_t *poll;

    os_sockopt_t *option;
} os_socknode_t;

os_socknode_t *os_socknode_new(os_sockaddr_t *addr);
void os_socknode_free(os_socknode_t *node);

os_socknode_t *os_socknode_add(os_list_t *list,
        int family, os_sockaddr_t *addr, os_sockopt_t *option);
void os_socknode_remove(os_list_t *list, os_socknode_t *node);
void os_socknode_remove_all(os_list_t *list);

int os_socknode_probe(os_list_t *list, os_list_t *list6,
        const char *dev, uint16_t port, os_sockopt_t *option);
#if 0 /* deprecated */
int os_socknode_fill_scope_id_in_local(os_sockaddr_t *sa_list);
#endif

void os_socknode_set_cleanup(
        os_socknode_t *node, void (*cleanup)(os_sock_t *));

os_sock_t *os_socknode_sock_first(os_list_t *list);

OS_END_EXTERN_C

#endif /* OS_SOCKNODE_H */
