/************************************************************************
 *File name: os_lksctp.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

static int subscribe_to_events(os_sock_t *sock);

void os_sctp_init(uint16_t port)
{
}

void os_sctp_final(void)
{
}

os_sock_t *os_sctp_socket(int family, int type)
{
    os_sock_t *new = NULL;
    int rv;

    new = os_sock_socket(family, type, IPPROTO_SCTP);
    if (!new) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "os_sock_socket(family:%d type:%d) failed", family, type);
        return NULL;
    }

    rv = subscribe_to_events(new);
    if (rv != OS_OK) {
        os_sock_destroy(new);
        return NULL;
    }

    return new;
}

os_sock_t *os_sctp_server(
        int type, os_sockaddr_t *sa_list, os_sockopt_t *socket_option)
{
    int rv;
    char buf[OS_ADDRSTRLEN];

    os_sock_t *new = NULL;
    os_sockaddr_t *addr;
    os_sockopt_t option;

    os_assert(sa_list);

    os_sockopt_init(&option);
    if (socket_option)
        memcpy(&option, socket_option, sizeof option);

    addr = sa_list;
    while (addr) {
        new = os_sctp_socket(addr->os_sa_family, type);
        if (new) {
            rv = os_sctp_peer_addr_params(new, &option);
            os_assert(rv == OS_OK);

            rv = os_sctp_rto_info(new, &option);
            os_assert(rv == OS_OK);

            rv = os_sctp_initmsg(new, &option);
            os_assert(rv == OS_OK);

            if (option.sctp_nodelay == true) {
                rv = os_sctp_nodelay(new, true);
                os_assert(rv == OS_OK);
            } else
                OS_WARN("SCTP NO_DELAY Disabled");

            if (option.so_linger.l_onoff == true) {
                rv = os_sctp_so_linger(new, option.so_linger.l_linger);
                os_assert(rv == OS_OK);
            }

            rv = os_listen_reusable(new->fd, true);
            os_assert(rv == OS_OK);

            if (os_sock_bind(new, addr) == OS_OK) {
                OS_DEBUG("sctp_server() [%s]:%d",
                        OS_ADDR(addr, buf), OS_PORT(addr));
                break;
            }

            os_sock_destroy(new);
        }

        addr = addr->next;
    }

    if (addr == NULL) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "sctp_server() [%s]:%d failed",
                OS_ADDR(sa_list, buf), OS_PORT(sa_list));
        return NULL;
    }

    os_assert(new);

    rv = os_sock_listen(new);
    os_assert(rv == OS_OK);

    return new;
}

os_sock_t *os_sctp_client(
        int type, os_sockaddr_t *sa_list, os_sockopt_t *socket_option)
{
    int rv;
    char buf[OS_ADDRSTRLEN];

    os_sock_t *new = NULL;
    os_sockaddr_t *addr;
    os_sockopt_t option;

    os_assert(sa_list);

    os_sockopt_init(&option);
    if (socket_option)
        memcpy(&option, socket_option, sizeof option);

    addr = sa_list;
    while (addr) {
        new = os_sctp_socket(addr->os_sa_family, type);
        if (new) {
            rv = os_sctp_peer_addr_params(new, &option);
            os_assert(rv == OS_OK);

            rv = os_sctp_rto_info(new, &option);
            os_assert(rv == OS_OK);

            rv = os_sctp_initmsg(new, &option);
            os_assert(rv == OS_OK);

            if (option.sctp_nodelay == true) {
                rv = os_sctp_nodelay(new, true);
                os_assert(rv == OS_OK);
            } else
                OS_WARN("SCTP NO_DELAY Disabled");

            if (option.so_linger.l_onoff == true) {
                rv = os_sctp_so_linger(new, option.so_linger.l_linger);
                os_assert(rv == OS_OK);
            }

            if (os_sock_connect(new, addr) == OS_OK) {
                OS_DEBUG("sctp_client() [%s]:%d",
                        OS_ADDR(addr, buf), OS_PORT(addr));
                break;
            }

            os_sock_destroy(new);
        }

        addr = addr->next;
    }

    if (addr == NULL) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "sctp_client() [%s]:%d failed",
                OS_ADDR(sa_list, buf), OS_PORT(sa_list));
        return NULL;
    }

    os_assert(new);

    return new;
}

int os_sctp_connect(os_sock_t *sock, os_sockaddr_t *sa_list)
{
    os_sockaddr_t *addr;
    char buf[OS_ADDRSTRLEN];

    os_assert(sock);

    addr = sa_list;
    while (addr) {
        if (os_sock_connect(sock, addr) == OS_OK) {
            OS_DEBUG("sctp_connect() [%s]:%d",
                    OS_ADDR(addr, buf), OS_PORT(addr));
            break;
        }

        addr = addr->next;
    }

    if (addr == NULL) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "sctp_connect() [%s]:%d failed",
                OS_ADDR(sa_list, buf), OS_PORT(sa_list));
        return OS_ERROR;
    }

    return OS_OK;
}

int os_sctp_sendmsg(os_sock_t *sock, const void *msg, size_t len,
        os_sockaddr_t *to, uint32_t ppid, uint16_t stream_no)
{
    socklen_t addrlen = 0;

    os_assert(sock);

    if (to)
        addrlen = os_sockaddr_len(to);
    
    return sctp_sendmsg(sock->fd, msg, len,
            to ? &to->sa : NULL, addrlen,
            htobe32(ppid),
            0,  /* flags */
            stream_no,
            0,  /* timetolive */
            0); /* context */
}

int os_sctp_recvmsg(os_sock_t *sock, void *msg, size_t len,
        os_sockaddr_t *from, os_sctp_info_t *sinfo, int *msg_flags)
{
    int size;
    socklen_t addrlen = sizeof(struct sockaddr_storage);
    os_sockaddr_t addr;

    int flags = 0;
    struct sctp_sndrcvinfo sndrcvinfo;

    os_assert(sock);

    memset(&sndrcvinfo, 0, sizeof sndrcvinfo);
    memset(&addr, 0, sizeof addr);
    size = sctp_recvmsg(sock->fd, msg, len, &addr.sa, &addrlen,
                &sndrcvinfo, &flags);
    if (size < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "sctp_recvmsg(%d) failed", size);
        return size;
    }

    if (from) {
        memcpy(from, &addr, sizeof(os_sockaddr_t));
    }

    if (msg_flags) {
        *msg_flags = flags;
    }

    if (sinfo) {
        sinfo->ppid = be32toh(sndrcvinfo.sinfo_ppid);
        sinfo->stream_no = sndrcvinfo.sinfo_stream;
    }

    return size;
}

/* is any of the bytes from offset .. u8_size in 'u8' non-zero? return offset
 * or -1 if all zero */
static int byte_nonzero(
        const uint8_t *u8, unsigned int offset, unsigned int u8_size)
{
    int j;

    for (j = offset; j < u8_size; j++) {
        if (u8[j] != 0)
            return j;
    }

    return OS_ERROR;
}

static int sctp_sockopt_event_subscribe_size = 0;

static int determine_sctp_sockopt_event_subscribe_size(void)
{
    uint8_t buf[256];
    socklen_t buf_len = sizeof(buf);
    int sd, rc;

    /* only do this once */
    if (sctp_sockopt_event_subscribe_size != 0)
        return 0;

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sd < 0)
        return sd;

    memset(buf, 0, sizeof(buf));
    rc = getsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, buf, &buf_len);
    os_closesocket(sd);
    if (rc < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "getsockopt(SCTP_PEER_ADDR_PARAMS) failed [%d]", rc);
        return rc;
    }

    sctp_sockopt_event_subscribe_size = buf_len;

    OS_DEBUG("sizes of 'struct sctp_event_subscribe': "
            "compile-time %zu, kernel: %u",
            sizeof(struct sctp_event_subscribe),
            sctp_sockopt_event_subscribe_size);
    return 0;
}

/*
 * The workaround is stolen from libosmo-netif.
 * - http://osmocom.org/projects/libosmo-netif/repository/revisions/master/entry/src/stream.c
 *
 * Attempt to work around Linux kernel ABI breakage
 *
 * The Linux kernel ABI for the SCTP_EVENTS socket option has been broken
 * repeatedly.
 *  - until commit 35ea82d611da59f8bea44a37996b3b11bb1d3fd7 ( kernel < 4.11),
 *    the size is 10 bytes
 *  - in 4.11 it is 11 bytes
 *  - in 4.12 .. 5.4 it is 13 bytes
 *  - in kernels >= 5.5 it is 14 bytes
 *
 * This wouldn't be a problem if the kernel didn't have a "stupid" assumption
 * that the structure size passed by userspace will match 1:1 the length
 * of the structure at kernel compile time. In an ideal world, it would just
 * use the known first bytes and assume the remainder is all zero.
 * But as it doesn't do that, let's try to work around this */
static int sctp_setsockopt_event_subscribe_workaround(
        int fd, const struct sctp_event_subscribe *event_subscribe)
{
    const unsigned int compiletime_size = sizeof(*event_subscribe);
    int rc;

    if (determine_sctp_sockopt_event_subscribe_size() < 0) {
        OS_ERR("Cannot determine SCTP_EVENTS socket option size");
        return OS_ERROR;
    }

    if (compiletime_size == sctp_sockopt_event_subscribe_size) {
        /* no kernel workaround needed */
        return setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS,
                event_subscribe, compiletime_size);
    } else if (compiletime_size < sctp_sockopt_event_subscribe_size) {
        /* we are using an older userspace with a more modern kernel
         * and hence need to pad the data */
        uint8_t buf[256];
        os_assert(sctp_sockopt_event_subscribe_size <= sizeof(buf));

        memcpy(buf, event_subscribe, compiletime_size);
        memset(buf + sizeof(*event_subscribe),
                0, sctp_sockopt_event_subscribe_size - compiletime_size);
        return setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS,
                buf, sctp_sockopt_event_subscribe_size);
    } else /* if (compiletime_size > sctp_sockopt_event_subscribe_size) */ {
        /* we are using a newer userspace with an older kernel and hence
         * need to truncate the data - but only if the caller didn't try
         * to enable any of the events of the truncated portion */
        rc = byte_nonzero((const uint8_t *)event_subscribe,
                sctp_sockopt_event_subscribe_size, compiletime_size);
        if (rc >= 0) {
            OS_ERR("Kernel only supports sctp_event_subscribe of %u bytes, "
                "but caller tried to enable more modern event at offset %u",
                sctp_sockopt_event_subscribe_size, rc);
            return OS_ERROR;
        }

        return setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, event_subscribe,
                sctp_sockopt_event_subscribe_size);
    }
}

static int subscribe_to_events(os_sock_t *sock)
{
    struct sctp_event_subscribe event_subscribe;

    os_assert(sock);

    memset(&event_subscribe, 0, sizeof(event_subscribe));
    event_subscribe.sctp_data_io_event = 1;
    event_subscribe.sctp_association_event = 1;
    event_subscribe.sctp_send_failure_event = 1;
    event_subscribe.sctp_shutdown_event = 1;

#ifdef DISABLE_SCTP_EVENT_WORKAROUND
    if (setsockopt(sock->fd, IPPROTO_SCTP, SCTP_EVENTS,
                    &event_subscribe, sizeof(event_subscribe)) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(SCTP_EVENTS) failed");
        return OS_ERROR;
    }
#else
    if (sctp_setsockopt_event_subscribe_workaround(
                sock->fd, &event_subscribe) < 0) {
        OS_ERR("sctp_setsockopt_events_linux_workaround() failed");
        return OS_ERROR;
    }
#endif

    return OS_OK;
}

static int sctp_sockopt_paddrparams_size = 0;

static int determine_sctp_sockopt_paddrparams_size(void)
{
    uint8_t buf[256];
    socklen_t buf_len = sizeof(buf);
    int sd, rc;

    /* only do this once */
    if (sctp_sockopt_paddrparams_size != 0)
        return 0;

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sd < 0)
        return sd;

    memset(buf, 0, sizeof(buf));
    rc = getsockopt(sd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, buf, &buf_len);
    os_closesocket(sd);
    if (rc < 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "getsockopt(SCTP_PEER_ADDR_PARAMS) failed [%d]", rc);
        return rc;
    }

    sctp_sockopt_paddrparams_size = buf_len;

    OS_DEBUG("sizes of 'struct sctp_paddrparams': "
            "compile-time %zu, kernel: %u",
            sizeof(struct sctp_paddrparams),
            sctp_sockopt_paddrparams_size);
    return 0;
}

static int sctp_setsockopt_paddrparams_workaround(
        int fd, const struct sctp_paddrparams *paddrparams)
{
    const unsigned int compiletime_size = sizeof(*paddrparams);
    int rc;

    if (determine_sctp_sockopt_paddrparams_size() < 0) {
        OS_ERR("Cannot determine SCTP_PEER_ADDR_PARAMS socket option size");
        return OS_ERROR;
    }

    if (compiletime_size == sctp_sockopt_paddrparams_size) {
        /* no kernel workaround needed */
        return setsockopt(fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
                paddrparams, compiletime_size);
    } else if (compiletime_size < sctp_sockopt_paddrparams_size) {
        /* we are using an older userspace with a more modern kernel
         * and hence need to pad the data */
        uint8_t buf[256];
        os_assert(sctp_sockopt_paddrparams_size <= sizeof(buf));

        memcpy(buf, paddrparams, compiletime_size);
        memset(buf + sizeof(*paddrparams),
                0, sctp_sockopt_paddrparams_size - compiletime_size);
        return setsockopt(fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
                buf, sctp_sockopt_paddrparams_size);
    } else /* if (compiletime_size > sctp_sockopt_paddrparams_size) */ {
        /* we are using a newer userspace with an older kernel and hence
         * need to truncate the data - but only if the caller didn't try
         * to enable any of the events of the truncated portion */
        rc = byte_nonzero((const uint8_t *)paddrparams,
                sctp_sockopt_paddrparams_size, compiletime_size);
        if (rc >= 0) {
            OS_ERR("Kernel only supports sctp_paddrparams of %u bytes, "
                "but caller tried to enable more modern event at offset %u",
                sctp_sockopt_paddrparams_size, rc);
            return OS_ERROR;
        }

        return setsockopt(fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, paddrparams,
                sctp_sockopt_paddrparams_size);
    }
}

int os_sctp_peer_addr_params(os_sock_t *sock, os_sockopt_t *option)
{
    struct sctp_paddrparams paddrparams;
    socklen_t socklen;

    os_assert(sock);
    os_assert(option);

    memset(&paddrparams, 0, sizeof(paddrparams));
    socklen = sizeof(paddrparams);
    if (getsockopt(sock->fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
                            &paddrparams, &socklen) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "getsockopt(SCTP_PEER_ADDR) failed");
        return OS_ERROR;
    }

#if !defined(__FreeBSD__)
    OS_DEBUG("OLD spp_flags = 0x%x hbinter = %d pathmax = %d, sackdelay = %d",
            paddrparams.spp_flags,
            paddrparams.spp_hbinterval,
            paddrparams.spp_pathmaxrxt,
            paddrparams.spp_sackdelay);
#else
    OS_DEBUG("OLD spp_flags = 0x%x hbinter = %d pathmax = %d",
            paddrparams.spp_flags,
            paddrparams.spp_hbinterval,
            paddrparams.spp_pathmaxrxt);
#endif

    paddrparams.spp_hbinterval = option->sctp.spp_hbinterval;
#if !defined(__FreeBSD__)
    paddrparams.spp_sackdelay = option->sctp.spp_sackdelay;
#endif

#ifdef DISABLE_SCTP_EVENT_WORKAROUND
    if (setsockopt(sock->fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
                            &paddrparams, sizeof(paddrparams)) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(SCTP_PEER_ADDR_PARAMS) failed");
        return OS_ERROR;
    }
#else
    if (sctp_setsockopt_paddrparams_workaround(sock->fd, &paddrparams) < 0) {
        OS_ERR("sctp_setsockopt_paddrparams_workaround() failed");
        return OS_ERROR;
    }
#endif

#if !defined(__FreeBSD__)
    OS_DEBUG("NEW spp_flags = 0x%x hbinter = %d pathmax = %d, sackdelay = %d",
            paddrparams.spp_flags,
            paddrparams.spp_hbinterval,
            paddrparams.spp_pathmaxrxt,
            paddrparams.spp_sackdelay);
#else
    OS_DEBUG("NEW spp_flags = 0x%x hbinter = %d pathmax = %d",
            paddrparams.spp_flags,
            paddrparams.spp_hbinterval,
            paddrparams.spp_pathmaxrxt);
#endif

    return OS_OK;
}

int os_sctp_rto_info(os_sock_t *sock, os_sockopt_t *option)
{
    struct sctp_rtoinfo rtoinfo;
    socklen_t socklen;

    os_assert(sock);
    os_assert(option);

    memset(&rtoinfo, 0, sizeof(rtoinfo));
    socklen = sizeof(rtoinfo);
    if (getsockopt(sock->fd, IPPROTO_SCTP, SCTP_RTOINFO,
                            &rtoinfo, &socklen) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "getsockopt for SCTP_RTOINFO failed");
        return OS_ERROR;
    }

    OS_DEBUG("OLD RTO (initial:%d max:%d min:%d)",
            rtoinfo.srto_initial,
            rtoinfo.srto_max,
            rtoinfo.srto_min);

    rtoinfo.srto_initial = option->sctp.srto_initial;
    rtoinfo.srto_min = option->sctp.srto_min;
    rtoinfo.srto_max = option->sctp.srto_max;

    if (setsockopt(sock->fd, IPPROTO_SCTP, SCTP_RTOINFO,
                            &rtoinfo, sizeof(rtoinfo)) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt for SCTP_RTOINFO failed");
        return OS_ERROR;
    }

    OS_DEBUG("New RTO (initial:%d max:%d min:%d)",
            rtoinfo.srto_initial,
            rtoinfo.srto_max,
            rtoinfo.srto_min);

    return OS_OK;
}

int os_sctp_initmsg(os_sock_t *sock, os_sockopt_t *option)
{
    struct sctp_initmsg initmsg;
    socklen_t socklen;

    os_assert(sock);
    os_assert(option);
    os_assert(option->sctp.sinit_num_ostreams > 1);

    memset(&initmsg, 0, sizeof(initmsg));
    socklen = sizeof(initmsg);
    if (getsockopt(sock->fd, IPPROTO_SCTP, SCTP_INITMSG,
                            &initmsg, &socklen) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "getsockopt for SCTP_INITMSG failed");
        return OS_ERROR;
    }

    OS_DEBUG("Old INITMSG (numout:%d maxin:%d maxattempt:%d maxinit_to:%d)",
                initmsg.sinit_num_ostreams,
                initmsg.sinit_max_instreams,
                initmsg.sinit_max_attempts,
                initmsg.sinit_max_init_timeo);

    initmsg.sinit_num_ostreams = option->sctp.sinit_num_ostreams;
    initmsg.sinit_max_instreams = option->sctp.sinit_max_instreams;
    initmsg.sinit_max_attempts = option->sctp.sinit_max_attempts;
    initmsg.sinit_max_init_timeo = option->sctp.sinit_max_init_timeo;

    if (setsockopt(sock->fd, IPPROTO_SCTP, SCTP_INITMSG,
                            &initmsg, sizeof(initmsg)) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt for SCTP_INITMSG failed");
        return OS_ERROR;
    }

    OS_DEBUG("New INITMSG (numout:%d maxin:%d maxattempt:%d maxinit_to:%d)",
                initmsg.sinit_num_ostreams,
                initmsg.sinit_max_instreams,
                initmsg.sinit_max_attempts,
                initmsg.sinit_max_init_timeo);

    return OS_OK;
}

int os_sctp_nodelay(os_sock_t *sock, int on)
{
    os_assert(sock);

    OS_DEBUG("Turn on SCTP_NODELAY");
    if (setsockopt(sock->fd, IPPROTO_SCTP, SCTP_NODELAY,
                &on, sizeof(on)) != 0) {
        OS_LOG_MESSAGE(OS_TLOG_ERROR, os_socket_errno,
                "setsockopt(IPPROTO_SCTP, SCTP_NODELAY) failed");
        return OS_ERROR;
    }

    return OS_OK;
}

int os_sctp_so_linger(os_sock_t *sock, int l_linger)
{
    os_assert(sock);
    return os_so_linger(sock->fd, l_linger);
}
