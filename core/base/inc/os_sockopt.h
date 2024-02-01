/************************************************************************
 *File name: os_sockopt.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SOCKOPT_H
#define OS_SOCKOPT_H

OS_BEGIN_EXTERN_C

typedef struct os_sockopt_s {
    struct {
        uint32_t spp_hbinterval;
        uint32_t spp_sackdelay;
        uint32_t srto_initial;
        uint32_t srto_min;
        uint32_t srto_max;
#define OS_DEFAULT_SCTP_MAX_NUM_OF_OSTREAMS 30
        uint16_t sinit_num_ostreams;
        uint16_t sinit_max_instreams;
        uint16_t sinit_max_attempts;
        uint16_t sinit_max_init_timeo;
    } sctp;

    bool sctp_nodelay;
    bool tcp_nodelay;

    struct {
        bool l_onoff;
        int l_linger;
    } so_linger;

    const char *so_bindtodevice;
} os_sockopt_t;

void os_sockopt_init(os_sockopt_t *option);

int os_nonblocking(os_socket_t fd);
int os_closeonexec(os_socket_t fd);
int os_listen_reusable(os_socket_t fd, int on);
int os_tcp_nodelay(os_socket_t fd, int on);
int os_so_linger(os_socket_t fd, int l_linger);
int os_bind_to_device(os_socket_t fd, const char *device);

OS_END_EXTERN_C

#endif
