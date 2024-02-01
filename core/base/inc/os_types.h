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

#ifdef __cplusplus
extern "C" {
#endif

/***************DEFINE*******************/
/***************common*******************/
#define OS_ARG_MAX                     256
#define OS_MAX_FILEPATH_LEN            256
#define OS_MAX_IFNAME_LEN              32

#define OS_MAX_SDU_LEN                 32768 /* Should Heap */
#define OS_HUGE_LEN                    8192  /* Can Stack */
#define OS_MAX_PKT_LEN                 2048

/***************log**********************/
#define OS_LOG_MESSAGE   os_tlog_message
#define OS_LOG_PRINT     os_tlog_print
#define OS_LOG_HEXDUMP   os_tlog_hexdump

#define OS_FATAL os_fatal
#define OS_ERR   os_error
#define OS_WARN  os_warn
#define OS_INFO  os_info
#define OS_DEBUG os_debug
#define OS_TRACE os_trace
/****************************************/
/***************declaration**************/

//number of microseconds since 00:00:00 january 1, 1970 UTC
typedef int64_t os_time_t;
typedef struct os_pollset_s os_pollset_t;
typedef struct os_poll_s os_poll_t;

/****************************************/
/***************global*******************/
extern int __os_mem_domain;
extern int __os_sock_domain;
extern int __os_sctp_domain;

_API_ extern os_pollset_t* os_global_pollset;

#ifdef __cplusplus
}
#endif

#endif
