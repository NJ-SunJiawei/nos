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

PRIVATE os_context_t self = {
    .buf.pool = 8,
    .buf.config_pool = 8,

    .log.pool = 8,
    .log.domain_pool = 64,
    .log.level = INFO,

	.pool.socket = 16,
};

_CONF_API_ os_context_t *os_global_context(void)
{
    return &self;
}

_ENTER_API_ void os_core_initialize(void)
{
	os_clog_domain_init();
    os_cdlog_init();

#if OS_USE_TALLOC == 1
	os_kmem_init();
#else
    os_buf_init();
#endif
    os_socket_init();

	//log module install
    os_log_install_domain(&__os_mem_domain, "mem", self.log.level);
    os_log_install_domain(&__os_sock_domain, "sock", self.log.level);
    os_log_install_domain(&__os_sctp_domain, "sctp", self.log.level);
}

_EXIT_API_ void os_core_terminate(void)
{
    os_socket_final();
#if OS_USE_TALLOC == 1
	os_kmem_final();
#else
    os_buf_final();
#endif

    os_cdlog_final();
	os_clog_domain_final();
}
