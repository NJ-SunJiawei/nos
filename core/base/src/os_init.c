/************************************************************************
 *File name: os_init.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"
#include "os_init.h"

int g_logLevel = TRACE; 
unsigned int g_modMask = 0; 

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


PRIVATE void os_clog_enable_coredump(bool enable_core)
{
#ifdef HAVE_SETRLIMIT
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = enable_core ? RLIM_INFINITY : 0;
	setrlimit(RLIMIT_CORE, &core_limits);
#endif
}

_ENTER_API_ void os_initialize(void)
{
	os_clog_enable_coredump(true);

#if defined(OS_USE_CDLOG)
    os_cdlog_init();
	os_cdlog_add_stderr();
	os_cdlog_add_file();
#elif defined(OS_USE_CTLOG)
	os_ctlog_init();
#elif defined(OS_USE_CMLOG)
	os_cmlog_init();
#endif

#if OS_USE_TALLOC == 1
	os_kmem_init();
#else
    os_buf_init();
#endif
}

_EXIT_API_ void os_terminate(void)
{
#if OS_USE_TALLOC == 1
	os_kmem_final();
#else
    os_buf_final();
#endif

#if defined(OS_USE_CDLOG)
	os_cdlog_final();
#elif defined(OS_USE_CTLOG)
	os_ctlog_final();
#elif defined(OS_USE_CMLOG)
	os_cmlog_final();
#endif
}
