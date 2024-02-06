/************************************************************************
 *File name: os_sockpair.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

int os_socketpair(int family, int type, int protocol, os_socket_t fd[2])
{
#ifndef WIN32
    return socketpair(family, type, protocol, fd);
#else
    int rc;
    os_socket_t server = INVALID_SOCKET;
    os_socket_t acceptor = INVALID_SOCKET;
    os_socket_t client = INVALID_SOCKET;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t size;

    os_assert(family == AF_INET);
    os_assert(type == SOCK_STREAM);
    os_assert(protocol == 0);

    server = socket(family, type, protocol);
    os_assert(server != INVALID_SOCKET);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htobe32(INADDR_LOOPBACK);
    server_addr.sin_port = 0;

    rc = bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr));
    os_assert(rc == 0);
    rc = listen(server, 1);
    os_assert(rc == 0);

    client = socket(AF_INET, SOCK_STREAM, 0);
    os_assert(client != INVALID_SOCKET);

    memset(&client_addr, 0, sizeof(client_addr));
    size = sizeof(client_addr);
    rc = getsockname(server, (struct sockaddr *)&client_addr, &size);
    os_assert(rc == 0);
    os_assert(size == sizeof(client_addr));

    rc = connect(client, (struct sockaddr *)&client_addr, sizeof(client_addr));
    os_assert(rc == 0);

    size = sizeof(server_addr);
    acceptor = accept(server, (struct sockaddr *)&server_addr, &size);
    os_assert(acceptor != INVALID_SOCKET);
    os_assert(size == sizeof(server_addr));

    rc = getsockname(client, (struct sockaddr *)&client_addr, &size);
    os_assert(rc == 0);
    os_assert(size == sizeof(client_addr));
    os_assert(server_addr.sin_family == client_addr.sin_family);
    os_assert(server_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr);
    os_assert(server_addr.sin_port == client_addr.sin_port);

    os_closesocket(server);
    fd[0] = client;
    fd[1] = acceptor;

    return OS_OK;
#endif
}

