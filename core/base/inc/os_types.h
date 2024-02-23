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
#define os_print   cdlog_print
#define os_log     cdlog_show
#define os_logsp   cdlog_show_sp
#define os_logh    cdlog_hexdump
#elif defined(OS_USE_CMLOG)
#define os_print   CMPRINT
#define os_log     CMLOGX
#define os_logsp   CMLOGSPX
#define os_logh    CMLOGH
#endif
/****************************************/
/***************declaration**************/

//number of microseconds since 00:00:00 january 1, 1970 UTC
typedef int64_t os_time_t;
typedef struct os_pollset_s os_pollset_t;
typedef struct os_poll_s os_poll_t;

/****************************************/
/***************global*******************/
extern int g_logLevel; 
extern unsigned int g_modMask; 

#ifdef __cplusplus
}
#endif

#endif
