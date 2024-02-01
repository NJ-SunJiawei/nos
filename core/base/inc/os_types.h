/************************************************************************
 *File name: os_types.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_TYPES_H
#define OS_TYPES_H

OS_BEGIN_EXTERN_C

extern int __os_mem_domain;
extern int __os_sock_domain;
extern int __os_sctp_domain;

#define OS_LOG_MESSAGE   os_tlog_message
#define OS_LOG_PRINT     os_tlog_print
#define OS_LOG_HEXDUMP   os_tlog_hexdump

#define OS_FATAL os_fatal
#define OS_ERR   os_error
#define OS_WARN  os_warn
#define OS_INFO  os_info
#define OS_DEBUG os_debug
#define OS_TRACE os_trace


 /** number of microseconds since 00:00:00 january 1, 1970 UTC */
typedef int64_t os_time_t;

typedef struct os_pollset_s os_pollset_t;
typedef struct os_poll_s os_poll_t;

extern os_pollset_t* os_global_pollset;

OS_END_EXTERN_C

#endif
