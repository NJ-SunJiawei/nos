/************************************************************************
 *File name: os_init.h
 *Description:os interface header file
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#ifndef OS_INIT_H
#define OS_INIT_H

#ifdef __cplusplus
#define OS_BEGIN_EXTERN_C       extern "C" {
#define OS_END_EXTERN_C         }
#else
#define OS_BEGIN_EXTERN_C
#define OS_END_EXTERN_C
#endif

#include "system_config.h"

/*thirdpart .c*/
#include "cJSON.h"
#include "cJSON_Utils.h"
#include "sqlite3.h"

#define OS_BASE_INSIDE

#define OS_USE_TALLOC 0
#include "os_platform.h"

#include "os_types.h"
#include "os_spool.h"
#include "os_abort.h"
#include "os_list.h"
#include "os_hash.h"
#include "os_rbtree.h"
#include "os_errno.h"
#include "os_random.h"
#include "os_time.h"
#include "os_thread.h"
#include "os_str.h"
#include "os_buf.h"
#include "os_mem.h"
#include "os_tlog.h"
#include "os_sockaddr.h"
#include "os_socket.h"
#include "os_sockopt.h"
#include "os_sockpair.h"
#include "os_socknode.h"
#include "os_sctp.h"
#include "os_poll.h"
#include "os_notify.h"


#undef OS_BASE_INSIDE

OS_BEGIN_EXTERN_C

typedef struct {
    struct {
        int pool;
        int config_pool;
    } buf;

    struct {
        int pool;
        int domain_pool;
        os_tlog_level_e level;
    } tlog;

    struct {
        uint64_t timer;
        uint64_t socket;
    } pool;

} os_context_t;

os_context_t *os_global_context(void);
void os_initialize(void);
void os_terminate(void);

OS_END_EXTERN_C
	
#endif
