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
#if defined(OS_USE_CDLOG)
#define os_logs    cdlog_show
#define os_logh    cdlog_show
#define os_log0    cdlog_show
#define os_log1    cdlog_show
#define os_log2    cdlog_show
#define os_log3    cdlog_show
#define os_log4    cdlog_show
#define os_logp0   cdlog_show_sp
#define os_logp1   cdlog_show_sp
#define os_logp2   cdlog_show_sp
#define os_logp3   cdlog_show_sp
#define os_logp4   cdlog_show_sp
#else
#define os_logs    CTLOG_STR
#define os_logh    CTLOG_HEX
#define os_log0    CTLOG0
#define os_log1    CTLOG1
#define os_log2    CTLOG2
#define os_log3    CTLOG3
#define os_log4    CTLOG4
#define os_logp0   CTLOG_SP0
#define os_logp1   CTLOG_SP1
#define os_logp2   CTLOG_SP2
#define os_logp3   CTLOG_SP3
#define os_logp4   CTLOG_SP4
#endif
/****************************************/
/***************declaration**************/

//number of microseconds since 00:00:00 january 1, 1970 UTC
typedef int64_t os_time_t;
typedef struct os_pollset_s os_pollset_t;
typedef struct os_poll_s os_poll_t;

/****************************************/
/***************global*******************/


#ifdef __cplusplus
}
#endif

#endif
