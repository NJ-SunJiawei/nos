/************************************************************************
 *File name: os_cmlog.c
 *Description:CMLOG_ALLOW_CONSOLE_LOGS
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#include "system_config.h"

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include "os_init.h"
#include <libgen.h>
#include "private/os_clog_priv.h"


PRIVATE FILE* g_fp = NULL;        /* global file pointer */
PRIVATE int  g_fd;                /* Global file descriptor */
PRIVATE int  g_readyFg = 0;
PRIVATE char g_logDir[MAX_FILENAME_LEN] = "/var/log";
PRIVATE char g_fileName[MAX_FILENAME_LEN] = "cmem";
PRIVATE char g_fileList[CLOG_MAX_FILES][MAX_FILENAME_LENGTH];
PRIVATE unsigned char g_nMaxLogFiles = 1;                       /* MAX Log Files 1 */
PRIVATE unsigned int g_uiMaxFileSizeLimit = MAX_FILE_SIZE;      /* Max File Size limit for each log file */
PRIVATE int g_nCurrFileIdx = 0;           /* Current File Number index */

PRIVATE unsigned int g_clogWriteCount = 0;
PRIVATE unsigned int g_maxClogCount   = 50;
PRIVATE unsigned int g_logsDropCnt    = 0;
PRIVATE unsigned int g_logsLostCnt    = 0;
PRIVATE cLogCntLmt   g_cLogCntLimit   = CLOG_COUNT_LIMIT_STOP;
PRIVATE unsigned int g_cirBufferDepth = CLOG_FIXED_LENGTH_BUFFER_NUM;

PRIVATE unsigned int numTtiTicks;     /* TTI Count */
//PRIVATE int thread_signalled;

PRIVATE os_ring_queue_t  *log_queue = NULL;
PRIVATE os_ring_buf_t    *log_buf = NULL;
typedef unsigned char cmlog_buffer_t[CLOG_FIXED_LENGTH_BUFFER_SIZE];

PRIVATE void cmlog_create_new_log_file(void);
PRIVATE void cmlog_read_cirbuf(cmlog_buffer_t pkt , unsigned int len);

PRIVATE int	g_nWrites = 0;  /* number of times log function is called */

#define CHECK_CIRFILE_SIZE if(++g_nWrites == CLOG_LIMIT_COUNT/2)    \
	{ \
		if(g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit) \
		{ \
			cmlog_create_new_log_file(); \
		} \
	}

PRIVATE void cmlog_deregister_res(void)
{
	os_ring_buf_destroy(log_buf);
	os_ring_queue_destroy(log_queue);
}

PRIVATE void cmlog_register_res(void)
{
	log_queue = os_ring_queue_create(g_cirBufferDepth);
	log_buf = os_ring_buf_create(g_cirBufferDepth, sizeof(cmlog_buffer_t));
	if(NULL == log_buf)
	{
		os_ring_queue_destroy(log_queue);
		return;	
	}
	g_readyFg = 1;
}

PRIVATE void cmlog_read_cirbuf(cmlog_buffer_t pkt , unsigned int len)
{
	if(NULL == pkt || 0 == len){
		return;
	}

	fprintf(g_fp, "%s", pkt);

	CHECK_CIRFILE_SIZE
}

PRIVATE void cmlog_read_final(void)
{
	unsigned char *pkt = NULL;
	unsigned int len = 0;

	//no block
	while ((os_ring_queue_try_get(log_queue, &pkt, &len) == OS_OK) && pkt && len) {
		cmlog_read_cirbuf(pkt, len);
	}

}

/*PRIVATE EndianType cmlog_getCPU_endian(void)
{
    unsigned short x;
    unsigned char c;
 
    x = 0x0001;
    c = *(unsigned char *)(&x);

	return ( c == 0x01 ) ? little_endian : big_endian;
}*/

PRIVATE void cmlog_flush_data(int sig)
{
	g_readyFg = 0;
	g_clogWriteCount = 0;
	//os_ring_queue_term(log_queue); //all no block,no need term

	cmlog_read_final();

	if(SIGSEGV == sig)
	{
	   signal(sig, SIG_DFL);
	   kill(getpid(), sig);
	}
	else
	{
	   fprintf(stderr, "cmlog SIG[%d] exit\n", sig);
	   exit(0);
	}

	return;
}

PRIVATE void cmlog_catch_segViolation(int sig)
{
#if HAVE_BACKTRACE
	int i, nStrLen, nDepth;

	void 	*stackTraceBuf[CLOG_MAX_STACK_DEPTH];
	const char* sFileNames[CLOG_MAX_STACK_DEPTH];
	const char* sFunctions[CLOG_MAX_STACK_DEPTH];

	char **strings; 
    char buf[CLOG_MAX_STACK_DEPTH*128] = {0};

	nDepth = backtrace(stackTraceBuf, CLOG_MAX_STACK_DEPTH);


	strings = (char**) backtrace_symbols(stackTraceBuf, nDepth);

	if(strings){
		for(i = 0, nStrLen=0; i < nDepth; i++)
		{
			sFunctions[i] = (strings[i]);
			sFileNames[i] = "unknown file";

			//CMLOGX(FATAL, "%s", strings[i]);
			//printf("BT[%d] : len [%d]: %s\n",i, strlen(sFunctions[i]),strings[i]);
			sprintf(buf+nStrLen, "	 in Function %s (from %s)\n", sFunctions[i], sFileNames[i]);
			nStrLen += strlen(sFunctions[i]) + strlen(sFileNames[i]) + 15;
		}
	
		CMLOGX(FATAL, CLOG_SEGFAULT_STR, buf);

		free(strings);
	}
#endif
	cmlog_flush_data(SIGSEGV);
}

PRIVATE void cmlog_timestamp(char* ts)
{
    struct timeval tv;
    struct tm tm;

    os_gettimeofday(&tv);
    os_localtime(tv.tv_sec, &tm);

   	sprintf(ts,"%04d%02d%02d_%02d:%02d:%02d.%03d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec/1000));
}

#ifdef CMLOG_ALLOW_CONSOLE_LOGS
PRIVATE void cmlog_add_stderr(void)
{
	os_cmlog_set_filename("stderr");
	g_fp = stderr;
    return;
}
#endif

PRIVATE void cmlog_create_new_log_file(void)
{
   FILE *fp, *prev_fp = g_fp;
   char curTime[CLOG_MAX_TIME_STAMP];
   int fd;
   //char *temptr;
   DIR *dir = NULL;

   /* get current time, when file is created */
   cmlog_timestamp(curTime);

   //cm no need
   /*temptr = strchr(curTime, '.');
   if (temptr != NULL)
   {
      *temptr = 0;
   }*/

   dir  = opendir(g_logDir);
   if (dir == NULL)
   { 
      mkdir(g_logDir, O_RDWR);//0775
   }
   else
   {
      closedir(dir);
   }

   /* remove old file from system */
   if( g_fileList[g_nCurrFileIdx][0] != '\0' ){
	   unlink(g_fileList[g_nCurrFileIdx]);
	   //fprintf(stderr, "remove log file success%s\n", g_fileList[g_nCurrFileIdx]);
   }

   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.log",g_logDir, g_fileName, curTime);
   fp = fopen(g_fileList[g_nCurrFileIdx], "w");

   if( fp == NULL ) {
      fprintf(stderr, "Failed to open log file %s\n", g_fileList[g_nCurrFileIdx]);
	  perror("Error opening file");
      return;
   }

   //fprintf(stderr, "create log file success%s\n", g_fileList[g_nCurrFileIdx]);

   fd = fileno(fp);

   g_fp = fp;
   g_fd = fd;

   if( fcntl(g_fd, F_SETFL, fcntl(g_fd, F_GETFL, 0) | O_NONBLOCK | O_ASYNC ) == -1 ) {
      fprintf(stderr, "RLOG: Cannot enable Buffer IO or make file non-blocking\n");
   }

   //setvbuf ( fp , NULL, _IONBF, 1024 );//no buffer
   setvbuf ( fp , NULL, _IOLBF, 1024 );//line buffer

   if( prev_fp != NULL )
      fclose(prev_fp);

   if( ++g_nCurrFileIdx == g_nMaxLogFiles )
      g_nCurrFileIdx = 0;

}

PRIVATE void cmlog_printf_static(void)
{
	fprintf(g_fp, "Log drop count:\t\t[%d]\n", g_logsDropCnt);
	fprintf(g_fp, "Log lost count:\t\t[%d]\n", g_logsLostCnt);
	os_ring_queue_show(log_queue);
	os_ring_buf_show(log_buf);
}

/*/var/lib/apport/coredump*/
PRIVATE void cmlog_enable_coredump(bool enable_core)
{
#ifdef HAVE_SETRLIMIT
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = enable_core ? RLIM_INFINITY : 0;
	setrlimit(RLIMIT_CORE, &core_limits);
#endif
}

PRIVATE void* cmlog_cirbuf_read_thread(void* arg)
{
	unsigned char *pkt = NULL;
	unsigned int len = 0;
    int rv = OS_ERROR;

	//fprintf(stderr, "Circular Buffer Reader thread started\n");

	while(g_readyFg)
	{
		//no block
		rv = os_ring_queue_try_get(log_queue, &pkt, &len);
		if(rv != OS_OK)
		{
	       if (rv == OS_DONE)
		   	   break;

		   if (rv == OS_RETRY){
			   continue;
		   }
		}
		cmlog_read_cirbuf(pkt, len);
		os_ring_buf_ret(pkt);
		pkt = NULL;
		len = 0;
	}

	return NULL;
}

void os_cmlog_set_filesize_limit(unsigned int maxFileSize)
{
	g_uiMaxFileSizeLimit = (maxFileSize == 0) ? MAX_FILE_SIZE : maxFileSize*1048576;
}

void os_cmlog_set_filenum(unsigned char maxFiles)
{
	if( maxFiles > CLOG_MAX_FILES || maxFiles == 0 ) {
		g_nMaxLogFiles = CLOG_MAX_FILES;
		return;
	}
	g_nMaxLogFiles = maxFiles;
}

void os_cmlog_set_log_path(const char* logDir)
{
	strncpy(g_logDir, logDir, MAX_FILENAME_LEN);
}

void os_cmlog_set_cirbuf_depth(unsigned int depth)
{
	g_cirBufferDepth = depth;
}

void os_cmlog_printf_config(void)
{
	fprintf(stderr, "Log File:\t\t[%s]\n", g_fileName);
	fprintf(stderr, "Log level[%d]:\t\t[%s]\n",g_logLevel, g_logStr[g_logLevel]);
	fprintf(stderr, "File Size Limit:\t[%d KB]\n", g_uiMaxFileSizeLimit/1024);
	fprintf(stderr, "Maximum Log Files:\t[%d]\n", g_nMaxLogFiles);

	fprintf(stderr, "Memory Logging:\t\t[Enabled]\n");
	fprintf(stderr, "Circular BufferSize:\t[Actual:%d KB]\n", g_cirBufferDepth * CLOG_FIXED_LENGTH_BUFFER_SIZE/1024);
}

void os_cmlog_set_filename(const char* fileName)
{
	strncpy(g_fileName, fileName, MAX_FILENAME_LEN);
}

void os_cmlog_set_log_level(os_clog_level_e logLevel)
{
	g_logLevel = logLevel;
}

void os_cmlog_set_module_mask(unsigned int modMask)
{
	g_modMask =  (modMask == 0 ) ? 0 : (g_modMask ^ modMask);
}

void os_cmlog_init(void)
{
	os_thread_id_t tid;

	signal(SIGSEGV, cmlog_catch_segViolation);
	signal(SIGBUS,  cmlog_catch_segViolation);
	signal(SIGINT,  cmlog_flush_data);
	signal(SIGHUP,  cmlog_flush_data);

	cmlog_enable_coredump(true);

	g_maxClogCount = CLOG_LIMIT_COUNT;

	cmlog_register_res();

	if(!g_readyFg){
		fprintf(stderr, "Failed to initialize log resource\n");
		exit(0);
	}

#ifdef CMLOG_ALLOW_CONSOLE_LOGS
	cmlog_add_stderr();
#else
	cmlog_create_new_log_file();
#endif

	if(pthread_create(&tid, NULL, cmlog_cirbuf_read_thread, NULL) != 0) {
		fprintf(g_fp, "Failed to initialize log server thread\n");
		exit(0);
	}

	os_cmlog_printf_config();

}


void os_cmlog_final(void)
{
	g_readyFg = 0;
	g_clogWriteCount = 0;

	cmlog_read_final();

	cmlog_printf_static();

	fflush(g_fp);
	if(g_fp != stderr){
		fclose(g_fp);
	}

	cmlog_deregister_res();
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called by EnbApp after Cell is UP.
//                 This function start the log cnt limit. i.e Number of logs
//                 getting logged into curcular will be restricted to 100 logs.
//////////////////////////////////////////////////////////////////////////
void os_cmlog_start_count_limit(void)
{
   g_clogWriteCount = 0;
   g_cLogCntLimit = CLOG_COUNT_LIMIT_START;
   fprintf(stderr, "Start Log Restriction\n");
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called by EnbApp after Cell Shutdown
//                 is triggered. This will enable to get all logs of shutdown.
//                 This function stops the log cnt limit. i.e Restricition on
//                 Number of logs getting logged into curcular will be removed
//////////////////////////////////////////////////////////////////////////
void os_cmlog_stop_count_limit(void)
{
   fprintf(stderr, "Stop Log Restriction\n");
   g_cLogCntLimit = CLOG_COUNT_LIMIT_STOP;
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called every 10 ms whenever 
//                 application layer update the tti count. To reset the 
//                 log rate control. And Enable logging for next 10 ms
//////////////////////////////////////////////////////////////////////////
void os_cmlog_reset_rate_limit(void)
{
    g_clogWriteCount = 0;
    g_logsDropCnt = 0;
}

/* This function will get tick from RLC/CL and will process logs
   according to tick threshold. */
void os_cmlog_update_ticks(void)
{
   static unsigned int rlogTickCount;
   numTtiTicks++;
   if(++rlogTickCount >= CLOGTICKSCNTTOPRCLOGS)
   {
      rlogTickCount = 0;
      os_cmlog_reset_rate_limit();
   }
   return;
}

PRIVATE void cmlog_save_log_data(const void* buf, unsigned int len)
{
   ++g_clogWriteCount ;

   if(((g_cLogCntLimit == CLOG_COUNT_LIMIT_START) && (g_clogWriteCount > g_maxClogCount)) || \
        (len > CLOG_FIXED_LENGTH_BUFFER_SIZE)){
      g_logsDropCnt++;
      return;
   }

	if (OS_OK != os_ring_queue_try_put(log_queue, (unsigned char *)buf, len)) {
		//g_logsLostCnt++;
		os_ring_buf_ret((unsigned char *)buf);
	}	 
}

PRIVATE void cmlog_hex_to_asii(char* p, const char* h, int hexlen)
{
   for(int i = 0; i < hexlen; i++, p+=3, h++){
	   sprintf(p, "%02x ", *h);
   }
}

void cmlogN(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, ...)
{
	va_list argList;
	void *szLog = NULL;

	szLog = (void *)os_ring_buf_get(log_buf);
	if(NULL == szLog) {
		g_logsLostCnt++;
		return;
	}

#ifdef CMLOG_ALLOW_CLOCK_TIME
	unsigned char szTime[CLOG_MAX_TIME_STAMP] = {0};
	cmlog_timestamp(szLog);
	snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, "[%s][%s]%s:%s:%d %s:", szTime, modName, basename(file), func, line, g_logStr[logLevel]);
#else
	snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, "[%u][%s]%s:%s:%d %s:", numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel]);
#endif

	va_start(argList,fmtStr);
	vsnprintf(szLog + strlen(szLog), CLOG_FIXED_LENGTH_BUFFER_SIZE - strlen(szLog), fmtStr, argList);
	va_end(argList);

	cmlog_save_log_data((const void*)szLog, strlen(szLog)); 
}

void cmlogSPN(int logLevel, const char* modName, char* file, const char* func, int line, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...)
{
	va_list argList;
	void *szLog = NULL;

	szLog = (void *)os_ring_buf_get(log_buf);
	if(NULL == szLog) {
		g_logsLostCnt++;
		return;
	}

#ifdef CMLOG_ALLOW_CLOCK_TIME
	unsigned char szTime[CLOG_MAX_TIME_STAMP] = {0};
	cmlog_timestamp(szLog);
    snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, "[%s][%s]%s:%s:%d %s:%s:%d:", szTime, modName, basename(file), func, line, g_logStr[logLevel], g_splStr[splType].name, splVal);
#else
    snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, "[%u][%s]%s:%s:%d %s:%s:%d:", numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel], g_splStr[splType].name, splVal);
#endif
    va_start(argList,fmtStr);
    vsnprintf(szLog + strlen(szLog), CLOG_FIXED_LENGTH_BUFFER_SIZE - strlen(szLog), fmtStr, argList);
    va_end(argList);

	cmlog_save_log_data((const void*)szLog, strlen(szLog)); 
}

void cmlogH(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, const char* hexdump, int hexlen, ...)
{
	char szHex[MAX_LOG_BUF_SIZE*3] = {0};
	void *szLog = NULL;

	szLog = (void *)os_ring_buf_get(log_buf);
	if(NULL == szLog) {
		g_logsLostCnt++;
		return;
	}

	cmlog_hex_to_asii(szHex, hexdump, hexlen);

#ifdef CMLOG_ALLOW_CLOCK_TIME
	unsigned char szTime[CLOG_MAX_TIME_STAMP] = {0};
	cmlog_timestamp(szLog);
	snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, fmtStr, szTime, modName, basename(file), func, line, g_logStr[logLevel], szHex);
#else
	snprintf(szLog, CLOG_FIXED_LENGTH_BUFFER_SIZE, fmtStr, numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel], szHex);
#endif

	cmlog_save_log_data((const void*)szLog, strlen(szLog)); 
}
