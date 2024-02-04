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
#define os_logx    cdlog_show
#define os_logp0   cdlog_show_sp
#define os_logp1   cdlog_show_sp
#define os_logp2   cdlog_show_sp
#define os_logp3   cdlog_show_sp
#define os_logp4   cdlog_show_sp
#elif defined(OS_USE_CMLOG)
#define os_logs    CMLOG_STR
#define os_logh    CMLOG_HEX
#define os_log0    CMLOG0
#define os_log1    CMLOG1
#define os_log2    CMLOG2
#define os_log3    CMLOG3
#define os_log4    CMLOG4
#define os_logx    CMLOGX
#define os_logp0   CMLOG_SP0
#define os_logp1   CMLOG_SP1
#define os_logp2   CMLOG_SP2
#define os_logp3   CMLOG_SP3
#define os_logp4   CMLOG_SP4
#elif defined(OS_USE_CFLOG)
#define os_logs    CFLOG_STR
#define os_logh    CFLOG_HEX
#define os_log0    CFLOG0
#define os_log1    CFLOG1
#define os_log2    CFLOG2
#define os_log3    CFLOG3
#define os_log4    CFLOG4
#define os_logx    CFLOGX
#define os_logp0   CFLOG_SP0
#define os_logp1   CFLOG_SP1
#define os_logp2   CFLOG_SP2
#define os_logp3   CFLOG_SP3
#define os_logp4   CFLOG_SP4
#endif
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


#ifdef __cplusplus
}
#endif

#endif
