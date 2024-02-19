/************************************************************************
 *File name: os_cmlog.c
 *Description:
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


PRIVATE FILE* g_fp = NULL;       /* global file pointer */
PRIVATE int g_fd;                /* Global file descriptor for L2 & L3 */
PRIVATE char g_logDir[MAX_FILENAME_LEN] = "/var/log";
PRIVATE char g_fileName[MAX_FILENAME_LEN] = "cm";
PRIVATE char g_fileList[CLOG_MAX_FILES][MAX_FILENAME_LENGTH];

PRIVATE unsigned char g_nMaxLogFiles = 1;                       /* MAX Log Files 1 */
PRIVATE unsigned int g_uiMaxFileSizeLimit = MAX_FILE_SIZE;      /* Max File Size limit for each log file */
PRIVATE unsigned int g_cirMaxBufferSize = CLOG_MAX_CIRBUF_SIZE; /* Default circular buffer size 100Kb*/
PRIVATE char tz_name[2][CLOG_TIME_ZONE_LEN + 1] = {"CST-8", ""};

PRIVATE int g_nCurrFileIdx = 0;           /* Current File Number index */

/* Thread-specific data key visible to all threads */
PRIVATE os_thread_key_t	g_threadkey;
PRIVATE os_thread_mutex_t g_logmutex; /* Mutex to protect circular buffers */
PRIVATE os_thread_mutex_t g_condmutex;
PRIVATE os_thread_cond_t g_cond;
PRIVATE THREAD_DATA* g_pCirList[CLOG_MAX_THREADS]; /* List of thread data pointers */
PRIVATE int g_nThreadsRegistered;          /* Number of threads registered */
PRIVATE int thread_signalled;
PRIVATE THREAD_DATA *g_pSingCirBuff = NULL;
PRIVATE unsigned short g_prevLogOffset=0;
PRIVATE int g_threadRunFg = 1;


PRIVATE unsigned char g_writeCirBuf = 0;
PRIVATE volatile unsigned int g_rlogPositionIndex=0;
PRIVATE unsigned int numTtiTicks;         /* TTI Count */

PRIVATE unsigned int g_clogWriteCount = 0;
PRIVATE unsigned int g_maxClogCount   = 50;
PRIVATE unsigned int g_logsDropCnt    = 0;
PRIVATE cLogCntLmt   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_STOP;

PRIVATE void cmlog_create_new_log_file(void);
PRIVATE void cmlog_read_cirbuf(void);


#define CHECK_CIRFILE_SIZE if( g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit ) \
	{ \
		cmlog_create_new_log_file(); \
	}

PRIVATE void cmlog_deregister_thread(void* argv)
{
	THREAD_DATA* pThrData = (THREAD_DATA*)(argv);

	if(argv == NULL)
		return;

	/* lock the mutex, to make sure no one is accessing this buffer */
	os_thread_mutex_lock(&g_logmutex);

	g_pCirList[pThrData->listIndex]  = NULL;

	if(pThrData->logBuff != NULL)
		free(pThrData->logBuff);

	free(argv);

	/* unlock the mutex */
	os_thread_mutex_unlock(&g_logmutex);

	os_thread_mutex_destroy(&g_logmutex);
}

PRIVATE void cmlog_init_specific(void)
{
	pthread_key_create(&g_threadkey, &cmlog_deregister_thread);
}

void cmlog_set_specific(const void *pThrData)
{
	int retVal = pthread_setspecific(g_threadkey, pThrData);
	
	if(retVal!=0) {
      fprintf(stderr, "Failed to associate the value with the key or invalid key");
      _exit(0);
   }
}


PRIVATE THREAD_DATA* cmlog_register_thread(const char* taskName)
{
	THREAD_DATA* pThrData = (THREAD_DATA*) malloc(sizeof(THREAD_DATA));

	if(pThrData == NULL) {
		fprintf(stderr, "Failed to allocate memory for thread %s\n", taskName);
		_exit(0);
	}

	os_thread_mutex_lock(&g_logmutex);

	/* Allocate circular buffer */
	pThrData->logBuff = (unsigned char*) malloc(g_cirMaxBufferSize);

	if(pThrData->logBuff == NULL) {
		fprintf(stderr, "Failed to allocate memory [%d] for thread %s\n",g_cirMaxBufferSize, taskName);
		_exit(0);
	}

	/* store task name */
	strcpy(pThrData->szTaskName, taskName);

	cmlog_set_specific(pThrData);

	pThrData->listIndex = g_nThreadsRegistered++;

	/* Store this pointerin global list, to access it later */
	g_pCirList[pThrData->listIndex]  = pThrData;

	os_thread_mutex_unlock(&g_logmutex);

	//fprintf(stderr, "rlRegisterThread: allocated CIRCULAR BUFFER of size [%ld]\n", g_cirMaxBufferSize);
	//fprintf(stderr, "rlRegisterThread: Total registered threads [%d]\n", g_nThreadsRegistered);

	return pThrData;
}

PRIVATE void* cmlog_cirbuf_read_thread(void* arg)
{
	struct timespec timeout;
	int retCode;

	//fprintf(stderr, "Circular Buffer Reader thread started\n");

	while(g_threadRunFg)
	{
		/*this thread is not active and waiting to timeout */
		thread_signalled = 0;

		/* set the thread timeout */
		timeout.tv_sec = time(NULL) + CLOG_CIRBUF_READ_INTERVAL;
		timeout.tv_nsec = 0;

		/* wait for CLOG_CIRBUF_READ_INTERVAL seconds time interval to read buffer */
		retCode = pthread_cond_timedwait(&g_cond, &g_condmutex, &timeout);

		/* this means, this thread is already active, no need to give any other signal to wake up */
		thread_signalled = 1;

		//if(retCode == 0) fprintf(stderr, "cirBufReaderThread: I am signalled to read data\n");

		/* If someone has given signal or there is timeout */
		if(retCode == 0 || retCode  == ETIMEDOUT){
			cmlog_read_cirbuf();
			continue;
		}

		cmlog_read_cirbuf();

		fprintf(stderr, "System is exiting ??");
		perror("cirbuf_read_thread");

		break;
	}

	return NULL;
}

PRIVATE void cmlog_read_cirbuf(void)
{
   unsigned int i, writerPos;

   g_writeCirBuf = 1;
   /* Before reading circular buffers, store delimiter */
   //storeTimeDelimeter(g_fp);

   /* lock the mutex */
   os_thread_mutex_lock(&g_logmutex);

   for(i=0; i < g_nThreadsRegistered; i++) 
   {
      THREAD_DATA* pThrData = g_pCirList[i];

      if(pThrData == NULL)
         continue;

      writerPos = pThrData->logBufLen;

      //fprintf(stderr, "Thread [%ld] WritePos:[%ld] ReadPos:[%ld]\n", i+1, writerPos, pThrData->logReadPos);

      if(pThrData->logReadPos < writerPos)
      {
         /* Calculate the delta data to be read from buffer */
         int dataLen = writerPos - pThrData->logReadPos;

         /* Write the data into file */
         if(fwrite(pThrData->logBuff+pThrData->logReadPos,1, dataLen, g_fp) == -1) 
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cmlog_create_new_log_file();
            continue;
         }

         /* reset log read position to last known position */
         pThrData->logReadPos = writerPos;
      }
      else if (pThrData->logReadPos > writerPos) 
      {
         /* Calculate the remaining data left in the buffer */
         int dataLen = g_cirMaxBufferSize -  pThrData->logReadPos;			

         /* Write from last know position till end */
         if(fwrite(pThrData->logBuff+pThrData->logReadPos, 1, dataLen, g_fp) == -1)
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cmlog_create_new_log_file();
            continue;
         }


         /* Write from 0 to len position */
         if(fwrite(pThrData->logBuff, 1, writerPos, g_fp) == -1)
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cmlog_create_new_log_file();
            continue;
         }

         /* reset log read position to last known position */
         pThrData->logReadPos = writerPos;
      }
   }

   /* unlock the mutex */
   os_thread_mutex_unlock(&g_logmutex);

   CHECK_CIRFILE_SIZE

   g_writeCirBuf = 0;
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
	g_threadRunFg = 0;

	cmlog_read_cirbuf();

	g_clogWriteCount = 0;

	fflush(g_fp);
	fclose(g_fp);

	if(SIGSEGV == sig)
	{
	   signal(sig, SIG_DFL);
	   kill(getpid(), sig);
	}
	else
	{
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

PRIVATE void cmlog_create_new_log_file(void)
{
   FILE *fp, *prev_fp = g_fp;
   char curTime[CLOG_MAX_TIME_STAMP];
   int fd;
   char *temptr;
   DIR *dir = NULL;

   /* get current time, when file is created */
   cmlog_timestamp(curTime); 
   temptr = strchr(curTime, '.');
   if (temptr != NULL)
   {
      *temptr = 0;
   }

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
   if( g_fileList[g_nCurrFileIdx][0] != '\0' )
      unlink(g_fileList[g_nCurrFileIdx]);

   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.log",g_logDir, g_fileName, curTime);
   fp = fopen(g_fileList[g_nCurrFileIdx], "a");

   if( fp == NULL ) {
      fprintf(stderr, "Failed to open log file %s\n", g_fileList[g_nCurrFileIdx]);
	  perror("Error opening file");
      return;
   }

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

void os_cmlog_set_fileSize_limit(unsigned int maxFileSize)
{
	g_uiMaxFileSizeLimit = (maxFileSize == 0) ? MAX_FILE_SIZE : maxFileSize*1048576;
}

void os_cmlog_set_fileNum(unsigned char maxFiles)
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

void os_cmlog_set_circular_bufferSize(unsigned int bufSize)
{
	g_cirMaxBufferSize = bufSize*1024;
	g_cirMaxBufferSize = (g_cirMaxBufferSize/50) * 50;
}

void os_cmlog_printf_config(void)
{
	fprintf(stderr, "Log File:\t\t[%s]\n", g_fileName);
	fprintf(stderr, "Log level[%d]:\t\t[%s]\n",g_logLevel, g_logStr[g_logLevel]);
	fprintf(stderr, "File Size Limit:\t[%d KB]\n", g_uiMaxFileSizeLimit/1024);
	fprintf(stderr, "Maximum Log Files:\t[%d]\n", g_nMaxLogFiles);
	fprintf(stderr, "Time Zone:\t\t[%s]\n", tz_name[0]);

	fprintf(stderr, "Memory Logging:\t\t[Enabled]\n");
	fprintf(stderr, "Circular BufferSize:\t[Actual:%d KB][Derived:%d Byte]\n", g_cirMaxBufferSize/1024, g_cirMaxBufferSize);
}

void os_cmlog_set_fileName(const char* fileName)
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
	signal(SIGSEGV, cmlog_catch_segViolation);
	signal(SIGBUS,  cmlog_catch_segViolation);
	signal(SIGINT,  cmlog_flush_data);

	g_maxClogCount = CLOG_LIMIT_L2_COUNT;

	os_cmlog_printf_config();

	cmlog_init_specific();
	os_thread_id_t tid;
	os_thread_mutex_init(&g_logmutex);
	if( pthread_create(&tid, NULL, cmlog_cirbuf_read_thread, NULL) != 0 ) {
		fprintf(stderr, "Failed to initialize log server thread\n");
		_exit(0);
	}
	/* Initialize single circular buffer for all threads */
	g_pSingCirBuff = cmlog_register_thread("DUMMY");

	cmlog_create_new_log_file();
}


void os_cmlog_final(void)
{
	g_threadRunFg = 0;
	cmlog_read_cirbuf();

	fflush(g_fp);
	fclose(g_fp);
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called by EnbApp after Cell is UP.
//                 This function start the log cnt limit. i.e Number of logs
//                 getting logged into curcular will be restricted to 100 logs.
//////////////////////////////////////////////////////////////////////////
void os_cmlog_start_count_limit(void)
{
   g_clogWriteCount = 0;
   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_START;
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
   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_STOP;
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
      os_cmlog_reset_rate_limit(); /* Resetting rlog write count to 0 */ 
   }
   return;
}


PRIVATE void cmlog_save_log_data(const void* buf, unsigned short len, unsigned int g_rlogWritePosIndex)
{
   ++g_clogWriteCount ;

   if((1 == g_writeCirBuf) || \
         ((g_clLogCntLimit == CL_LOG_COUNT_LIMIT_START) && \
          (g_clogWriteCount > g_maxClogCount)) || \
         (len > CLOG_FIXED_LENGTH_BUFFER_SIZE))
   {
      g_rlogPositionIndex --;
      g_logsDropCnt++;
      return;
   }

   unsigned int logWritePointerPosition;
   THREAD_DATA* p = (THREAD_DATA*) g_pSingCirBuff;

   /* if buffer is about to full, write till end and continue writing from begining */
   if(((g_rlogWritePosIndex+1) * CLOG_FIXED_LENGTH_BUFFER_SIZE) > g_cirMaxBufferSize) 
   {
      /* setting this flag to 1 to avoid other threads
         to write in same circular buffer */
      g_writeCirBuf = 1;
      /* Start globalPositionIndex again */
      g_rlogPositionIndex = 0;

      /* if reader has not read initial data, minmum buffer size should be 100Kb */
      if(p->logReadPos < CLOG_READ_POS_THRESHOLD && !thread_signalled) {
         pthread_cond_signal(&g_cond); /* this will wakeup thread */
      }

      /* we are unlikely to hit this condition, but to prevent corruption of binary logs */
      /* we cannot write the data, if we write, data will be corrected forever */
      if(p->logReadPos < CLOG_FIXED_LENGTH_BUFFER_SIZE) {
         fprintf(stderr, "cannot write data.retune buffer parameters\n");
         return;
      }

      /* Copy data from the start of buffer */
      memcpy(p->logBuff, buf, len);
      /* Store buffer length position */
      p->logBufLen = CLOG_FIXED_LENGTH_BUFFER_SIZE;
      g_prevLogOffset = 0;
      /* setting this flag to 0 so that other threads
         will start writing in circular buffer */
      g_writeCirBuf = 0;
   }
   else 
   {
      /* if reader is far behind and writer is reaching reader position, diff < 5Kb */
      /* its time to wakeup thread if reader has not read much of data */
      if(p->logReadPos > p->logBufLen && (p->logReadPos - p->logBufLen) < CLOG_READ_POS_THRESHOLD) 
         pthread_cond_signal(&g_cond); /* this will wakeup thread */

      logWritePointerPosition = (g_rlogWritePosIndex * CLOG_FIXED_LENGTH_BUFFER_SIZE) + g_prevLogOffset;

      memcpy(p->logBuff+logWritePointerPosition, buf, len);
      p->logBufLen += CLOG_FIXED_LENGTH_BUFFER_SIZE;
   }

   /*{
      static int maxlen = 0;
      if(len > maxlen) {
         maxlen = len;
         fprintf(stderr, "MAX BUFFER SIZE is binary mode is [%d]\n", maxlen);
      }
   }*/

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
	char szLog[MAX_LOG_LEN] = {0};

	snprintf(szLog, MAX_LOG_LEN, "[%u][%s]%s:%s:%d %s:", numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel]);

	va_start(argList,fmtStr);
	vsnprintf(szLog + strlen(szLog), MAX_LOG_LEN - strlen(szLog), fmtStr, argList);
	va_end(argList);

	cmlog_save_log_data((const void*)szLog, strlen(szLog), g_rlogPositionIndex++); 
}

void cmlogSPN(int logLevel, const char* modName, char* file, const char* func, int line, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...)
{
	va_list argList;
	char szLog[MAX_LOG_LEN] = {0};

    snprintf(szLog, MAX_LOG_LEN, "[%u][%s]%s:%s:%d %s:%s:%d:", numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel], g_splStr[splType].name, splVal);

    va_start(argList,fmtStr);
    vsnprintf(szLog + strlen(szLog), MAX_LOG_LEN - strlen(szLog), fmtStr, argList);
    va_end(argList);

	cmlog_save_log_data((const void*)szLog, strlen(szLog), g_rlogPositionIndex++); 
}

void cmlogH(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, const char* hexdump, int hexlen, ...)
{
	char szHex[MAX_LOG_BUF_SIZE*3] = {0};
	char szLog[MAX_LOG_LEN] = {0};

	cmlog_hex_to_asii(szHex, hexdump, hexlen);
	snprintf(szLog, MAX_LOG_LEN, fmtStr, numTtiTicks, modName, basename(file), func, line, g_logStr[logLevel], szHex);

	cmlog_save_log_data((const void*)szLog, strlen(szLog), g_rlogPositionIndex++); 
}


