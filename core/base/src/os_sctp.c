/************************************************************************
 *File name: os_sctp.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

PRIVATE void sctp_write_callback(short when, os_socket_t fd, void *data);

int os_sctp_recvdata(os_sock_t *sock, void *msg, size_t len,
        os_sockaddr_t *from, os_sctp_info_t *sinfo)
{
    int size;
    int flags = 0;

    do {
        size = os_sctp_recvmsg(sock, msg, len, from, sinfo, &flags);
        if (size < 0) {
           	os_logsp(ERROR, ERRNOID, os_socket_errno, "os_sctp_recvdata(%d)", size);
            return size;
        }

        if (flags & MSG_NOTIFICATION) {
            /* Nothing */
			union sctp_notification *not = (union sctp_notification *)msg;
	
			switch(not->sn_header.sn_type) {
			case SCTP_ASSOC_CHANGE :
				os_log(DEBUG, "SCTP_ASSOC_CHANGE:"
						"[T:%d, F:0x%x, S:%d, I/O:%d/%d]", 
						not->sn_assoc_change.sac_type,
						not->sn_assoc_change.sac_flags,
						not->sn_assoc_change.sac_state,
						not->sn_assoc_change.sac_inbound_streams,
						not->sn_assoc_change.sac_outbound_streams);

				if (not->sn_assoc_change.sac_state == SCTP_COMM_UP) {
					os_log(DEBUG, "SCTP_COMM_UP");
				} else if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP ||
						not->sn_assoc_change.sac_state == SCTP_COMM_LOST) {
	
					if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP)
						os_log(DEBUG, "SCTP_SHUTDOWN_COMP");
					if (not->sn_assoc_change.sac_state == SCTP_COMM_LOST)
						os_log(DEBUG, "SCTP_COMM_LOST");
				}
				break;
			case SCTP_SHUTDOWN_EVENT :
				os_log(DEBUG, "SCTP_SHUTDOWN_EVENT:[T:%d, F:0x%x, L:%d]",
						not->sn_shutdown_event.sse_type,
						not->sn_shutdown_event.sse_flags,
						not->sn_shutdown_event.sse_length);
				break;
			case SCTP_SEND_FAILED :
				os_log(ERROR, "SCTP_SEND_FAILED:[T:%d, F:0x%x, S:%d]",
						not->sn_send_failed.ssf_type,
						not->sn_send_failed.ssf_flags,
						not->sn_send_failed.ssf_error);
				break;
			case SCTP_PEER_ADDR_CHANGE:
				os_log(WARN, "SCTP_PEER_ADDR_CHANGE:[T:%d, F:0x%x, S:%d]", 
						not->sn_paddr_change.spc_type,
						not->sn_paddr_change.spc_flags,
						not->sn_paddr_change.spc_error);
				break;
			case SCTP_REMOTE_ERROR:
				os_log(WARN, "SCTP_REMOTE_ERROR:[T:%d, F:0x%x, S:%d]", 
						not->sn_remote_error.sre_type,
						not->sn_remote_error.sre_flags,
						not->sn_remote_error.sre_error);
				break;
			default :
				os_log(ERROR, "Discarding event with unknown flags:0x%x type:0x%x",
						flags, not->sn_header.sn_type);
				break;
			}
        }
        else if (flags & MSG_EOR) {
			//recv success
            break;
        }
        else {
            os_assert_if_reached();
        }
    } while(1);

    return size;
}

int os_sctp_senddata(os_sock_t *sock,
        os_buf_t *buf, os_sockaddr_t *addr)
{
    int sent;

    os_assert(sock);
    os_assert(buf);

    sent = os_sctp_sendmsg(sock, buf->data, buf->len, addr,
            os_sctp_ppid_in_buf(buf), os_sctp_stream_no_in_buf(buf));
    if (sent < 0 || sent != buf->len) {
        os_logsp(ERROR, ERRNOID, os_socket_errno, "os_sctp_senddata(len:%d,ssn:%d)", buf->len, (int)os_sctp_stream_no_in_buf(buf));
        os_buf_free(buf);
        return OS_ERROR;
    }

    os_buf_free(buf);
    return OS_OK;
}

void os_sctp_write_to_buffer(os_pollset_t *pollset, os_sctp_sock_t *sctp, os_buf_t *buf)
{
    os_assert(sctp);
    os_assert(buf);

    os_list_add(&sctp->write_queue, buf);

    if (!sctp->poll.write) {
        os_assert(sctp->sock);
        sctp->poll.write = os_pollset_add(pollset,
            OS_POLLOUT, sctp->sock->fd, sctp_write_callback, sctp);
        os_assert(sctp->poll.write);
    }
}

PRIVATE void sctp_write_callback(short when, os_socket_t fd, void *data)
{
    os_sctp_sock_t *sctp = data;
    os_buf_t *buf = NULL;

    os_assert(sctp);
    if (os_list_empty(&sctp->write_queue) == true) {
        os_assert(sctp->poll.write);
        os_pollset_remove(sctp->poll.write);
        sctp->poll.write = NULL;
        return;
    }

    buf = os_list_first(&sctp->write_queue);
    os_assert(buf);
    os_list_remove(&sctp->write_queue, buf);

    os_assert(sctp->sock);
    os_sctp_senddata(sctp->sock, buf, NULL);
}

void os_sctp_flush_and_destroy(os_sctp_sock_t *sctp)
{
    os_buf_t *buf = NULL, *next_buf = NULL;

    os_assert(sctp);

    os_assert(sctp->addr);
    os_free(sctp->addr);

    if (sctp->type == SOCK_STREAM) {
        os_assert(sctp->poll.read);
        os_pollset_remove(sctp->poll.read);

        if (sctp->poll.write)
            os_pollset_remove(sctp->poll.write);

        os_sctp_destroy(sctp->sock);

        os_list_for_each_safe(&sctp->write_queue, next_buf, buf) {
            os_list_remove(&sctp->write_queue, buf);
            os_buf_free(buf);
        }
    }
}
