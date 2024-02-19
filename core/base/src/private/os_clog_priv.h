/************************************************************************
 *File name: os_clog_priv.h
 *Description: common
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_CLOG_PRIV_H
#define OS_CLOG_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

PRIVATE const char* g_logStr[MAX_LOG_LEVEL] = { "NONE", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

#define CLOG_SEGFAULT_STR "Segmentation Fault Occurred\n%s"

#define MAX_FILE_SIZE  			        3145728 /* 3MB */
#define MAX_LOG_LEN				        256
#define MAX_FILENAME_LEN 		        300
#define MAX_FILENAME_LENGTH 	        700
#define LOG_TIME_LEN 			        64
#define MAX_LOG_BUF_SIZE 		        5000
#define CLOG_MAX_CIRBUF_SIZE			(1024*100)
#define CLOG_MAX_FILES		 			5
#define CLOG_MAX_TIME_STAMP 			128
#define CLOG_MAX_TAX_NAME				16
#define CLOG_CIRBUF_READ_INTERVAL	    1	/*seconds read interval */
#define CLOG_MAX_THREADS				16
#define CLOG_TIME_ZONE_LEN				8
#define CLOG_MAX_STACK_DEPTH 		    24
#define CLOG_MAX_BACKTRACE_BUFSZ	    2048
#define CLOG_READ_POS_THRESHOLD         (MAX_LOG_LEN*6) //300
#define CLOG_FIXED_LENGTH_BUFFER_SIZE   MAX_LOG_LEN     //50
#define CLOGTICKSCNTTOPRCLOGS           10

/*L2 Logging */
#define CLOG_LIMIT_L2_COUNT 50

#define TA_NOR              "\033[0m"       /* all off */
#define TA_FGC_BLACK        "\033[30m"      /* Black */
#define TA_FGC_RED          "\033[31m"      /* Red */
#define TA_FGC_BOLD_RED     "\033[1;31m"    /* Bold Red */
#define TA_FGC_GREEN        "\033[32m"      /* Green */
#define TA_FGC_BOLD_GREEN   "\033[1;32m"    /* Bold Green */
#define TA_FGC_YELLOW       "\033[33m"      /* Yellow */
#define TA_FGC_BOLD_YELLOW  "\033[1;33m"    /* Bold Yellow */
#define TA_FGC_BOLD_BLUE    "\033[1;34m"    /* Bold Blue */
#define TA_FGC_BOLD_MAGENTA "\033[1;35m"    /* Bold Magenta */
#define TA_FGC_BOLD_CYAN    "\033[1;36m"    /* Bold Cyan */
#define TA_FGC_WHITE        "\033[37m"      /* White  */
#define TA_FGC_BOLD_WHITE   "\033[1;37m"    /* Bold White  */
#define TA_FGC_DEFAULT      "\033[39m"      /* default */

typedef enum {
	LOG_ARG_INT,
	LOG_ARG_STR,
	LOG_ARG_HEX,
	LOG_ARG_SPL
} LOG_ARG_TYPE;
	
typedef enum cLogCntLmt
{
   CL_LOG_COUNT_LIMIT_START = 1,
   CL_LOG_COUNT_LIMIT_STOP
}cLogCntLmt;
	
typedef struct
{
	time_t tv_sec;
	unsigned int	ms_tti;
} OS_GNUC_PACKED LOGTIME;

typedef enum _endian {little_endian, big_endian} EndianType;


typedef struct {
	char	szTaskName[CLOG_MAX_TAX_NAME];
	unsigned char*	logBuff;	/* LOG Buffer */
	unsigned int	logBufLen;	/* Data Written till now */
	unsigned int	logReadPos; /* Reader thread position */
	unsigned char	listIndex;	/* Index to global list */
} THREAD_DATA;


#ifdef __cplusplus
}
#endif

#endif


