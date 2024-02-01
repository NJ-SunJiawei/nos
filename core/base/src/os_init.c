/************************************************************************
 *File name: os_init.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

int __os_mem_domain;
int __os_sock_domain;
int __os_sctp_domain;

os_pollset_t* os_global_pollset;

PRIVATE os_context_t self = {
    .buf.pool = 8,
    .buf.config_pool = 8,

    .tlog.pool = 8,
    .tlog.domain_pool = 64,
    .tlog.level = OS_TLOG_DEFAULT,

	.pool.socket = 16,
};

_CONF_API_ os_context_t *os_global_context(void)
{
    return &self;
}

_ENTER_API_ void os_core_initialize(void)
{
    os_tlog_init();
#if OS_USE_TALLOC == 1
	os_kmem_init();
#else
    os_buf_init();
#endif
    os_socket_init();

    os_tlog_install_domain(&__os_mem_domain, "mem", self.tlog.level);
    os_tlog_install_domain(&__os_sock_domain, "sock", self.tlog.level);
    os_tlog_install_domain(&__os_sctp_domain, "sctp", self.tlog.level);

    os_global_pollset = os_pollset_create(self.pool.socket);
    os_assert(os_global_pollset);

}

_EXIT_API_ void os_core_terminate(void)
{
    if (os_global_pollset) os_pollset_destroy(os_global_pollset);

    os_socket_final();
#if OS_USE_TALLOC == 1
	os_kmem_final();
#else
    os_buf_final();
#endif
    os_tlog_final();
}
