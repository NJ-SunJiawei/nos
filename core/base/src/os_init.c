/************************************************************************
 *File name: os_init.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

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
#if defined(OS_USE_CDLOG)
	os_ctlog_enable_coredump(true);
	os_ctlog_init();
#else
	os_cdlog_enable_coredump(true);
    os_cdlog_init();
#endif

#if OS_USE_TALLOC == 1
	os_kmem_init();
#else
    os_buf_init();
#endif

}

_EXIT_API_ void os_core_terminate(void)
{
#if OS_USE_TALLOC == 1
	os_kmem_final();
#else
    os_buf_final();
#endif

#if defined(OS_USE_CDLOG)
   os_cdlog_final();
#endif
}
