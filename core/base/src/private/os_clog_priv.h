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

#define RLOG_SEGFAULT_STR "Segmentation Fault Occurred\n%s"

typedef unsigned int LOGID;

#define MAX_FILE_SIZE  			        3145728 /* 3MB */
#define MAX_LOG_LEN				        256
#define MAX_FILENAME_LEN 		        300
#define MAX_FILENAME_LENGTH 	        700
#define LOG_TIME_LEN 			        64
#define MAX_LOG_BUF_SIZE 		        5000
#define CLOG_MAX_CIRBUF_SIZE			(1024*100)
#define CLOG_REMOTE_LOGGING_PORT		9099
#define CLOG_MAX_FILES		 			5
#define CLOG_MAX_TIME_STAMP 			80
#define CLOG_MAX_TAX_NAME				16
#define CLOG_FIFO_FILE					"/tmp/l2logs"
#define CLOG_CIRBUF_READ_INTERVAL	1	/* 60 seconds read interval */
#define CLOG_MAX_THREADS				16
#define CLOG_TIME_ZONE_LEN				8
#define CLOG_MAX_STACK_DEPTH 		24
#define CLOG_MAX_BACKTRACE_BUFSZ	2048
#define CLOG_READ_POS_THRESHOLD 300
#define CLOG_FIXED_LENGTH_BUFFER_SIZE 50
#define CLOGTICKSCNTTOPRCL2LOGS 10

/* Console handling */
#define CLOG_CTRL_L    12
#define CLOG_CTRL_Y    25
#define cLOG_ENTER_KEY 10
#define CLOG_SET_LOGLEVEL  1
#define CLOG_SET_MODMASK   2

/*L2 Logging */
#define PROCESS_L2LOG_TTI 10

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
	
	
typedef enum
{
	L_TIME_REFERENCE=0,
	L_TIME_DELIMITER,
	L_SIGSEGV,
	L_TIME_TTI_UPDT
} LOGID_TYPE;
	
typedef struct
{
	time_t tv_sec;
	unsigned int	ms_tti;
} OS_GNUC_PACKED LOGTIME;

typedef struct
{
	LOGID       logId;		/* 4 bytes | 32 bits */
	LOGTIME     logTime;	/* 8 bytes */
	unsigned int argType	:2;
	unsigned int logLevel	:3;
	unsigned int numOfArgs	:3;
	unsigned short			len;
} OS_GNUC_PACKED LOGDATA;

typedef struct
{
	LOGDATA 	logData;
	char		buf[MAX_LOG_BUF_SIZE];
}  OS_GNUC_PACKED ARGDATA;

typedef struct
{
	LOGDATA logData;
	unsigned int arg1;
	unsigned int arg2;
	unsigned int arg3;
	unsigned int arg4;
   char unusedByte[19]; /* To make it as 50 byte */
}  OS_GNUC_PACKED ARG4DATA;

typedef struct
{
	LOGDATA logData;
	unsigned char		  splEnum;
	unsigned int splArg;
	unsigned int arg1;
	unsigned int arg2;
	unsigned int arg3;
	unsigned int arg4;
   char unusedByte[14]; /* To make it as 50 byte */
}  OS_GNUC_PACKED SPL_ARGDATA;

typedef enum _endian {little_endian, big_endian} EndianType;

typedef struct
{
	unsigned short     version;
	unsigned int       dummy32;
	unsigned char      endianType;
	char			   szTimeZone[CLOG_TIME_ZONE_LEN+1];
	unsigned short     END_MARKER;
	time_t	           time_sec;
} OS_GNUC_PACKED FILE_HEADER;


typedef struct {
	
	char	szTaskName[CLOG_MAX_TAX_NAME];
	unsigned char*	logBuff;		/* LOG Buffer */
	unsigned int	logBufLen;	/* Data Written till now */
	unsigned int	logReadPos; /* Reader thread position */
	unsigned char	listIndex;	/* Index to global list */

} THREAD_DATA;

void readL2LogBuff(void);
void processL2LogBuff(void);
signed short  sendL2LogBuftoL3(void);
void clInitL2Log(void);
/* Extern for soc specific file */
void clProcessLogBufFromL2(void *mBuf);
void clInitL2SocSpecific(void);
//extern void processL2LogBuff(void);
void clProcessTicks(void);
void clGetL2LogBufPtr (void *mBuf, unsigned int *logLen,unsigned char **logPtr);
void clInvalidateL2LogsInCache(unsigned char *ptr,unsigned int len);

extern unsigned char	 *g_l2rlogBuf;		  /* buffer pointer for shared memory allocation */
extern unsigned char	 *g_l2LogBufStartPtr; /* buffer pointer where logs has to be written */
extern unsigned char	 *g_l2LogBufBasePtr;  /* Base pointer for log buffer */
extern unsigned char	 *g_logBufRcvdFromL2; /* Buffer pointer received from L2 at L3*/
extern unsigned char	 *g_l2LogBaseBuff;	  /* Base log buffer received at L3 */
extern unsigned int 	g_l2LogBufLen;		/* Log Buffer length written at L2 */
extern unsigned int 	startL2Logging; 	/* flag to start processing of L2 logs */
extern unsigned int 	g_l2logBuffPos; 	/* Log Buffer block which is in use for L2 logging */
extern unsigned char	  g_writeCirBuf;	  /* Flag to indicate whether to write logs or not */

void hex2Asii(char* p, const char* h, int hexlen);


#ifdef __cplusplus
}
#endif

#endif


