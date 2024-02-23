/************************************************************************
 *File name: os_socket.c
 *Description:
 *
 *Current Version:
 *Author: Copy by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "os_init.h"

void os_socket_init(void)
{
#if _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    os_assert(err == 0);
#endif
}

void os_socket_final(void)
{
}

os_sock_t *os_sock_create(void)
{
    os_sock_t *sock = NULL;

    sock = os_calloc(1, sizeof(*sock));
    if (!sock) {
        os_log(ERROR, "os_calloc() failed");
        return NULL;
    }

    sock->fd = INVALID_SOCKET;

    return sock;
}

void os_sock_destroy(os_sock_t *sock)
{
    os_assert(sock);

    if (sock->fd != INVALID_SOCKET) {
        os_closesocket(sock->fd);
    }
    sock->fd = INVALID_SOCKET;

    os_free(sock);
}

os_sock_t *os_sock_socket(int family, int type, int protocol)
{
    os_sock_t *sock = NULL;

    sock = os_sock_create();
    os_assert(sock);

    sock->family = family;
    sock->fd = socket(sock->family, type, protocol);
    if (sock->fd < 0) {
        os_sock_destroy(sock);
        os_logsp(ERROR, ERRNOID, os_socket_errno, "socket create(%d:%d:%d) failed", sock->family, type, protocol);
        return NULL;
    }

    os_log(DEBUG, "socket create(%d:%d:%d)", sock->family, type, protocol);

    return sock;
}

int os_sock_bind(os_sock_t *sock, os_sockaddr_t *addr)
{
    char buf[OS_ADDRSTRLEN];
    socklen_t addrlen;

    os_assert(sock);
    os_assert(addr);

    addrlen = os_sockaddr_len(addr);
    os_assert(addrlen);

    if (bind(sock->fd, &addr->sa, addrlen) != 0) {
        os_logsp(ERROR, ERRNOID, os_socket_errno,"socket bind(%d) [%s]:%d failed", addr->os_sa_family, OS_ADDR(addr, buf), OS_PORT(addr));
        return OS_ERROR;
    }

    memcpy(&sock->local_addr, addr, sizeof(sock->local_addr));

    os_log(DEBUG, "socket bind %s:%d", OS_ADDR(addr, buf), OS_PORT(addr));

    return OS_OK;
}

int os_sock_connect(os_sock_t *sock, os_sockaddr_t *addr)
{
    char buf[OS_ADDRSTRLEN];
    socklen_t addrlen;

    os_assert(sock);
    os_assert(addr);

    addrlen = os_sockaddr_len(addr);
    os_assert(addrlen);

    if (connect(sock->fd, &addr->sa, addrlen) != 0) {
        os_logsp(ERROR, ERRNOID, os_socket_errno ,"socket connect[%s]:%d failed", OS_ADDR(addr, buf), OS_PORT(addr));
        return OS_ERROR;
    }

    memcpy(&sock->remote_addr, addr, sizeof(sock->remote_addr));

    os_log(DEBUG, "socket connect %s:%d\n", OS_ADDR(addr, buf), OS_PORT(addr));

    return OS_OK;
}

int os_sock_listen(os_sock_t *sock)
{
    int rc;
    os_assert(sock);

    rc = listen(sock->fd, 5);
    if (rc < 0) {
        os_logsp(ERROR, ERRNOID, os_socket_errno, "listen failed");
        return OS_ERROR;
    }

    return OS_OK;
}

os_sock_t *os_sock_accept(os_sock_t *sock)
{
    os_sock_t *new_sock = NULL;

    int new_fd = -1;
    os_sockaddr_t addr;
    socklen_t addrlen;

    os_assert(sock);

    memset(&addr, 0, sizeof(addr));
    addrlen = sizeof(addr.ss);

    new_fd = accept(sock->fd, &addr.sa, &addrlen);
    if (new_fd < 0) {
        return NULL;
    }

    new_sock = os_sock_create();
    os_assert(new_sock);

    new_sock->family = sock->family;
    new_sock->fd = new_fd;

    memcpy(&new_sock->remote_addr, &addr, sizeof(new_sock->remote_addr));

    return new_sock;
}

ssize_t os_write(os_socket_t fd, const void *buf, size_t len)
{
    os_assert(fd != INVALID_SOCKET);

    return write(fd, buf, len);
}

ssize_t os_read(os_socket_t fd, void *buf, size_t len)
{
    os_assert(fd != INVALID_SOCKET);

    return read(fd, buf, len);
}

ssize_t os_send(os_socket_t fd, const void *buf, size_t len, int flags)
{
    os_assert(fd != INVALID_SOCKET);

    return send(fd, buf, len, flags);
}

ssize_t os_sendto(os_socket_t fd,
        const void *buf, size_t len, int flags, const os_sockaddr_t *to)
{
    socklen_t addrlen;

    os_assert(fd != INVALID_SOCKET);
    os_assert(to);

    addrlen = os_sockaddr_len(to);
    os_assert(addrlen);

    return sendto(fd, buf, len, flags, &to->sa, addrlen);
}

ssize_t os_recv(os_socket_t fd, void *buf, size_t len, int flags)
{
    os_assert(fd != INVALID_SOCKET);
    return recv(fd, buf, len, flags);
}

ssize_t os_recvfrom(os_socket_t fd,
        void *buf, size_t len, int flags, os_sockaddr_t *from)
{
    socklen_t addrlen = sizeof(struct sockaddr_storage);

    os_assert(fd != INVALID_SOCKET);
    os_assert(from);

    memset(from, 0, sizeof *from);
    return recvfrom(fd, buf, len, flags, &from->sa, &addrlen);
}

int os_closesocket(os_socket_t fd)
{
    int r;
#ifdef _WIN32
    r = closesocket(fd);
#else
    r = close(fd);
#endif
    if (r != 0) {
        os_logsp(ERROR, ERRNOID, os_socket_errno, "closesocket failed");
        return OS_ERROR;
    }

    return OS_OK;
}
