/************************************************************************
 *File name: os_sctp.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SCTP_H
#define OS_SCTP_H

#include "system_config.h"

#if HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif

#if HAVE_LOCAL_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#undef OS_LOG_DOMAIN
#define OS_LOG_DOMAIN __os_sctp_domain

#define OS_S1AP_SCTP_PORT              36412
//#define OS_SGSAP_SCTP_PORT             29118
#define OS_NGAP_SCTP_PORT              38412

#define OS_SCTP_S1AP_PPID              18
#define OS_SCTP_X2AP_PPID              27
//#define OS_SCTP_SGSAP_PPID             0
#define OS_SCTP_NGAP_PPID              60

#define os_sctp_ppid_in_buf(__pkBUF)         (__pkBUF)->param[0]
#define os_sctp_stream_no_in_buf(__pkBUF)    (__pkBUF)->param[1]

#undef MSG_NOTIFICATION
#define MSG_NOTIFICATION 0x8000  //0x2000 uksctp

#ifndef INET
#define INET            1
#endif
#ifndef INET6
#define INET6           1
#endif

#define os_sctp_destroy os_sock_destroy
#define os_sctp_accept os_sock_accept

typedef struct os_sctp_sock_s {
    int             type;           /* SOCK_STREAM or SOCK_SEQPACKET */

    os_sock_t      *sock;          /* Socket */
    os_sockaddr_t  *addr;          /* Address */

    struct {
        os_poll_t  *read;          /* Read Poll */
        os_poll_t  *write;         /* Write Poll */
    } poll;

    os_list_t      write_queue;    /* Write Queue for Sending S1AP message */
} os_sctp_sock_t;

typedef struct os_sctp_info_s {
    uint32_t ppid;
    uint16_t stream_no;
    uint16_t inbound_streams;
    uint16_t outbound_streams;
} os_sctp_info_t;

void os_sctp_init(uint16_t port);
void os_sctp_final(void);

os_sock_t *os_sctp_socket(int family, int type);
os_sock_t *os_sctp_server(int type, os_sockaddr_t *sa_list, os_sockopt_t *socket_option);
os_sock_t *os_sctp_client(int type, os_sockaddr_t *sa_list, os_sockopt_t *socket_option);
int os_sctp_bind(os_sock_t *sock, os_sockaddr_t *sa_list);
int os_sctp_connect(os_sock_t *sock, os_sockaddr_t *sa_list);
int os_sctp_listen(os_sock_t *sock);
int os_sctp_peer_addr_params(os_sock_t *sock, os_sockopt_t *option);
int os_sctp_rto_info(os_sock_t *sock, os_sockopt_t *option);
int os_sctp_initmsg(os_sock_t *sock, os_sockopt_t *option);
int os_sctp_nodelay(os_sock_t *sock, int on);
int os_sctp_so_linger(os_sock_t *sock, int l_linger);
int os_sctp_sendmsg(os_sock_t *sock, const void *msg, size_t len, os_sockaddr_t *to, uint32_t ppid, uint16_t stream_no);
int os_sctp_recvmsg(os_sock_t *sock, void *msg, size_t len, os_sockaddr_t *from, os_sctp_info_t *sinfo, int *msg_flags);
int os_sctp_recvdata(os_sock_t *sock, void *msg, size_t len, os_sockaddr_t *from, os_sctp_info_t *sinfo);
int os_sctp_senddata(os_sock_t *sock, os_buf_t *buf, os_sockaddr_t *addr);
void os_sctp_write_to_buffer(os_sctp_sock_t *sctp, os_buf_t *buf);
void os_sctp_flush_and_destroy(os_sctp_sock_t *sctp);

#ifdef __cplusplus
}
#endif

#endif
