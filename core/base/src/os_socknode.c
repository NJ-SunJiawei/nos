/************************************************************************
 *File name: os_socknode.c
 *Description:
 *
 *Current Version:
 *Author: Copy by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#if HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "os_init.h"

os_socknode_t *os_socknode_new(os_sockaddr_t *addr)
{
    os_socknode_t *node = NULL;

    os_assert(addr);

    node = os_calloc(1, sizeof(os_socknode_t));
    if (!node) {
        os_log(ERROR, "os_calloc() failed");
        return NULL;
    }

    node->addr = addr;

    return node;
}

void os_socknode_free(os_socknode_t *node)
{
    os_assert(node);

    os_freeaddrinfo(node->addr);
    if (node->dev)
        os_free(node->dev);
    if (node->poll)
        os_pollset_remove(node->poll);
    if (node->sock) {
        if (node->cleanup)
            node->cleanup(node->sock);
        else
            os_sock_destroy(node->sock);
    }
    if (node->option)
        os_free(node->option);
    os_free(node);
}

os_socknode_t *os_socknode_add(os_list_t *list,
        int family, os_sockaddr_t *addr, os_sockopt_t *option)
{
    os_socknode_t *node = NULL;
    os_sockaddr_t *dup = NULL;

    os_assert(list);
    os_assert(addr);

    os_assert(OS_OK == os_copyaddrinfo(&dup, addr));
    if (family != AF_UNSPEC)
        os_filteraddrinfo(&dup, family);

    if (dup) {
        node = os_socknode_new(dup);
        os_assert(node);
        os_list_add(list, node);

        if (option)
            node->option = os_memdup(option, sizeof *option);
    }

    return node;
}

void os_socknode_remove(os_list_t *list, os_socknode_t *node)
{
    os_assert(node);

    os_list_remove(list, node);
    os_socknode_free(node);
}

void os_socknode_remove_all(os_list_t *list)
{
    os_socknode_t *node = NULL, *saved_node = NULL;

    os_list_for_each_safe(list, saved_node, node)
        os_socknode_remove(list, node);
}

int os_socknode_probe(os_list_t *list, os_list_t *list6,
        const char *dev, uint16_t port, os_sockopt_t *option)
{
#if defined(HAVE_GETIFADDRS)
    os_socknode_t *node = NULL;
    struct ifaddrs *iflist, *cur;
    int rc;

    rc = getifaddrs(&iflist);
    if (rc != 0) {
         os_logsp(ERROR, ERRNOID, os_socket_errno, "getifaddrs failed");
        return OS_ERROR;
    }

    for (cur = iflist; cur != NULL; cur = cur->ifa_next) {
        os_sockaddr_t *addr = NULL;

        if (cur->ifa_flags & IFF_LOOPBACK)
            continue;

        if (cur->ifa_flags & IFF_POINTOPOINT)
            continue;

        if (cur->ifa_addr == NULL) /* may happen with ppp interfaces */
            continue;

        if (dev && strcmp(dev, cur->ifa_name) != 0)
            continue;

        addr = (os_sockaddr_t *)cur->ifa_addr;
        if (cur->ifa_addr->sa_family == AF_INET) {
            if (!list) continue;

#ifndef IN_IS_ADDR_LOOPBACK
#define IN_IS_ADDR_LOOPBACK(a) \
  ((((long int) (a)->s_addr) & be32toh(0xff000000)) == be32toh(0x7f000000))
#endif /* IN_IS_ADDR_LOOPBACK */

/* An IP equivalent to IN6_IS_ADDR_UNSPECIFIED */
#ifndef IN_IS_ADDR_UNSPECIFIED
#define IN_IS_ADDR_UNSPECIFIED(a) \
  (((long int) (a)->s_addr) == 0x00000000)
#endif /* IN_IS_ADDR_UNSPECIFIED */
            if (IN_IS_ADDR_UNSPECIFIED(&addr->sin.sin_addr) ||
                IN_IS_ADDR_LOOPBACK(&addr->sin.sin_addr))
                continue;
        } else if (cur->ifa_addr->sa_family == AF_INET6) {
            if (!list6) continue;

            if (IN6_IS_ADDR_UNSPECIFIED(&addr->sin6.sin6_addr) ||
                IN6_IS_ADDR_LOOPBACK(&addr->sin6.sin6_addr) ||
                IN6_IS_ADDR_MULTICAST(&addr->sin6.sin6_addr) ||
                IN6_IS_ADDR_LINKLOCAL(&addr->sin6.sin6_addr) ||
                IN6_IS_ADDR_SITELOCAL(&addr->sin6.sin6_addr))
                continue;
        } else
            continue;

        addr = os_calloc(1, sizeof(os_sockaddr_t));
        memcpy(&addr->sa, cur->ifa_addr, os_sockaddr_len(cur->ifa_addr));
        addr->os_sin_port = htobe16(port);

        node = os_calloc(1, sizeof(os_socknode_t));
        node->addr = addr;
        if (dev)
            node->dev = os_strdup(dev);

        if (addr->os_sa_family == AF_INET) {
            os_assert(list);
            os_list_add(list, node);
        } else if (addr->os_sa_family == AF_INET6) {
            os_assert(list6);
            os_list_add(list6, node);
        } else
            os_assert_if_reached();

        if (option)
            node->option = os_memdup(option, sizeof *option);
    }

    freeifaddrs(iflist);
    return OS_OK;
#elif defined(_WIN32)
    return OS_OK;
#else
    os_assert_if_reached();
    return OS_ERROR;
#endif

}

#if 0 /* deprecated */
int os_socknode_fill_scope_id_in_local(os_sockaddr_t *sa_list)
{
#if defined(HAVE_GETIFADDRS)
    struct ifaddrs *iflist = NULL, *cur;
    int rc;
    os_sockaddr_t *addr, *ifaddr;

    for (addr = sa_list; addr != NULL; addr = addr->next) {
        if (addr->os_sa_family != AF_INET6)
            continue;

        if (!IN6_IS_ADDR_LINKLOCAL(&addr->sin6.sin6_addr))
            continue;

        if (addr->sin6.sin6_scope_id != 0)
            continue;

        if (iflist == NULL) {
            rc = getifaddrs(&iflist);
            if (rc != 0) {
                 os_logsp(ERROR, ERRNOID, os_socket_errno, "getifaddrs failed");
                return OS_ERROR;
            }
        }

        for (cur = iflist; cur != NULL; cur = cur->ifa_next) {
            ifaddr = (os_sockaddr_t *)cur->ifa_addr;

            if (cur->ifa_addr == NULL) /* may happen with ppp interfaces */
                continue;

            if (cur->ifa_addr->sa_family != AF_INET6)
                continue;

            if (!IN6_IS_ADDR_LINKLOCAL(&ifaddr->sin6.sin6_addr))
                continue;

            if (memcmp(&addr->sin6.sin6_addr,
                    &ifaddr->sin6.sin6_addr, sizeof(struct in6_addr)) == 0) {
                /* Fill Scope ID in localhost */
                addr->sin6.sin6_scope_id = ifaddr->sin6.sin6_scope_id;
            }
        }
    }

    if (iflist)
        freeifaddrs(iflist);

    return OS_OK;
#elif defined(_WIN32)
    return OS_OK;
#else
    os_assert_if_reached();
    return OS_ERROR;
#endif
}
#endif

void os_socknode_set_cleanup(
        os_socknode_t *node, void (*cleanup)(os_sock_t *))
{
    os_assert(node);
    os_assert(cleanup);

    node->cleanup = cleanup;
}

os_sock_t *os_socknode_sock_first(os_list_t *list)
{
    os_socknode_t *snode = NULL;

    os_assert(list);
    os_list_for_each(list, snode) {
        if (snode->sock)
            return snode->sock;
    }

    return NULL;
}
