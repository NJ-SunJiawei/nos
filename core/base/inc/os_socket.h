/************************************************************************
 *File name: os_socket.h
 *Description:
 *
 *Current Version:
 *Author: Copy by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SOCKET_H
#define OS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef SOCKET os_socket_t;
#else
typedef int os_socket_t;
#endif

#if !defined(_WIN32) && !defined(INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif

typedef struct os_sock_s {
    int family;
    os_socket_t fd;

    os_sockaddr_t local_addr;
    os_sockaddr_t remote_addr;
} os_sock_t;

void os_socket_init(void);
void os_socket_final(void);

os_sock_t *os_sock_create(void);
void os_sock_destroy(os_sock_t *sock);

os_sock_t *os_sock_socket(int family, int type, int protocol);
int os_sock_bind(os_sock_t *sock, os_sockaddr_t *addr);
int os_sock_connect(os_sock_t *sock, os_sockaddr_t *addr);

int os_sock_listen(os_sock_t *sock);
os_sock_t *os_sock_accept(os_sock_t *sock);

ssize_t os_write(os_socket_t fd, const void *buf, size_t len);
ssize_t os_read(os_socket_t fd, void *buf, size_t len);

ssize_t os_send(os_socket_t fd, const void *buf, size_t len, int flags);
ssize_t os_sendto(os_socket_t fd,
        const void *buf, size_t len, int flags, const os_sockaddr_t *to);
ssize_t os_recv(os_socket_t fd, void *buf, size_t len, int flags);
ssize_t os_recvfrom(os_socket_t fd,
        void *buf, size_t len, int flags, os_sockaddr_t *from);

int os_closesocket(os_socket_t fd);

#ifdef __cplusplus
}
#endif

#endif
