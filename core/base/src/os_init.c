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

_ENTER_API_ void os_core_initialize(void)
{
	fprintf(stderr, "os_core_initialize enter\n");

#if defined(OS_USE_CDLOG)
    os_cdlog_init();
	os_cdlog_add_stderr();
	os_cdlog_add_file();
#elif defined(OS_USE_CMLOG)
	os_cmlog_init();
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
#elif defined(OS_USE_CMLOG)
	os_cmlog_final();
#endif
	fprintf(stderr, "os_core_terminate exit\n");
}

_EXIT_API_ void os_core_callback(void (*termFunc)(void))
{
	atexit(termFunc);
}

