/************************************************************************
 *File name: os_cmlog.c
 *Description: CMLOG_USE_CIRCULAR_BUFFER + CMLOG_USE_TTI_LOGGING
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

int g_logLevel = MAX_LOG_LEVEL; 
unsigned int g_modMask = 0; 

PRIVATE FILE* g_fp = NULL;       /* global file pointer */
PRIVATE int g_fd;                /* Global file descriptor for L2 & L3 */
PRIVATE char g_logDir[MAX_FILENAME_LEN] = "/var/log";
PRIVATE char g_fileName[MAX_FILENAME_LEN] = "os";
PRIVATE char g_fileList[CLOG_MAX_FILES][MAX_FILENAME_LENGTH];

PRIVATE unsigned char g_nMaxLogFiles = 1;                       /* MAX Log Files 1 */
PRIVATE unsigned int g_uiMaxFileSizeLimit = MAX_FILE_SIZE;      /* Max File Size limit for each log file */
PRIVATE unsigned int g_cirMaxBufferSize = CLOG_MAX_CIRBUF_SIZE; /* Default circular buffer size 100Kb*/
PRIVATE char tz_name[2][CLOG_TIME_ZONE_LEN + 1] = {"CST-8", ""};


PRIVATE int	g_nWrites = 0;                /* number of times log function is called */
PRIVATE int g_nCurrFileIdx = 0;           /* Current File Number index */

PRIVATE void cmlog_create_new_log_file(void);

PRIVATE unsigned int g_nLogPort = CLOG_REMOTE_LOGGING_PORT;     /* Remote Logging port */
PRIVATE unsigned char g_bRemoteLoggingDisabled = 0;             /* Remote logging flag */

#ifdef CMLOG_USE_CIRCULAR_BUFFER
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
PRIVATE void cmlog_read_cirbuf(void);
#endif

PRIVATE unsigned char g_writeCirBuf = 0;
PRIVATE volatile unsigned int g_rlogPositionIndex=0;
PRIVATE unsigned int numTtiTicks;         /* TTI Count */
PRIVATE int g_nCliSocket = 0;             /* Socke descriptor if remote client is connected */

PRIVATE unsigned int g_clogWriteCount = 0;
PRIVATE unsigned int g_maxClogCount   = 50;
PRIVATE unsigned int g_logsDropCnt    = 0;
PRIVATE cLogCntLmt   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_STOP;
PRIVATE void cmlog_close_connection(int sockfd);

#ifdef CMLOG_USE_CIRCULAR_BUFFER

    #ifdef CMLOG_USE_TTI_LOGGING
    #define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
	    { \
		    g_nWrites = 0; \
		    ctlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));\
	    } 
    #else
    #define CHECK_FILE_SIZE
    #endif /* CMLOG_USE_TTI_LOGGING */

#else /* CMLOG_USE_CIRCULAR_BUFFER */

	#ifdef CMLOG_USE_TTI_LOGGING
	#define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
		{ \
			if( g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit ) { \
				cmlog_create_new_log_file(); \
			}\
			g_nWrites = 0; \
			ctlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));\
		} 
	#else
	#define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
		{ \
			if( g_fp && ( (unsigned int)(ftell(g_fp)) > g_uiMaxFileSizeLimit) ) { \
				cmlog_create_new_log_file(); \
			}\
			g_nWrites = 0; \
		} 
	#endif /* CMLOG_USE_TTI_LOGGING */
#endif /*  CMLOG_USE_CIRCULAR_BUFFER */


#ifdef CMLOG_USE_CIRCULAR_BUFFER

#define CHECK_CIRFILE_SIZE if( g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit ) \
	{ \
		cmlog_create_new_log_file(); \
	}

PRIVATE void cmlog_deregister_thread(void* argv)
{

	THREAD_DATA* pThrData = (THREAD_DATA*)(argv);

	if( argv == NULL )
		return;

	/* lock the mutex, to make sure no one is accessing this buffer */
	os_thread_mutex_lock(&g_logmutex);

	g_pCirList[pThrData->listIndex]  = NULL;

	if( pThrData->logBuff != NULL )
		os_free(pThrData->logBuff);

	os_free(argv);

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
	
	if( retVal!=0 ) {
      fprintf(stderr, "Failed to associate the value with the key or invalid key");
      _exit(0);
   }
}


PRIVATE THREAD_DATA* cmlog_register_thread(const char* taskName)
{
	THREAD_DATA* pThrData = (THREAD_DATA*) cl_calloc(sizeof(THREAD_DATA));

	if( pThrData == NULL ) {
		fprintf(stderr, "Failed to allocate memory for thread %s\n", taskName);
		_exit(0);
	}

	os_thread_mutex_lock(&g_logmutex);

	/* Allocate circular buffer */
	pThrData->logBuff = (unsigned char*) os_malloc(g_cirMaxBufferSize);

	if( pThrData->logBuff == NULL ) {
		fprintf(stderr, "Failed to allocate memory [%ld] for thread %s\n",g_cirMaxBufferSize, taskName);
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
		if( retCode == 0 || retCode  == ETIMEDOUT ){
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

         /* If post processor connected send logs */
         if(g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff+pThrData->logReadPos, dataLen, 0 ) == -1) 
         {
            cmlog_close_connection(g_nCliSocket);
            g_nCliSocket = 0;
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

         /* If post processor connected send logs */
         if(g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff+pThrData->logReadPos, dataLen, 0 ) == -1) 
         {
            cmlog_close_connection(g_nCliSocket);
            g_nCliSocket = 0;
         }

         /* Write from 0 to len position */
         if(fwrite(pThrData->logBuff, 1, writerPos, g_fp) == -1)
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cmlog_create_new_log_file();
            continue;
         }

         /* If post processor connected send logs */
         if(g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff, writerPos, 0 ) == -1) 
         {
            cmlog_close_connection(g_nCliSocket);
            g_nCliSocket = 0;
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

#endif

PRIVATE EndianType cmlog_getCPU_endian(void)
{
    unsigned short x;
    unsigned char c;
 
    x = 0x0001;
    c = *(unsigned char *)(&x);

	return ( c == 0x01 ) ? little_endian : big_endian;
}

PRIVATE void cmlog_write_file_header(FILE* fp)
{
	FILE_HEADER fileHdr;

	memset(&fileHdr, 0, sizeof(FILE_HEADER));

	fileHdr.endianType = cmlog_getCPU_endian();
	fileHdr.dummy32 = 2818049;
	fileHdr.END_MARKER = 0xFFFF;
	strncpy(fileHdr.szTimeZone, tz_name[0], CLOG_TIME_ZONE_LEN);
   	fileHdr.time_sec = time(NULL);

	if(fwrite((const void*)&fileHdr, 1, sizeof(FILE_HEADER), fp) ==  -1 )
	{
		fprintf(stderr, "Failed to write file header\n");
		cmlog_create_new_log_file();
	}
}

PRIVATE void* cmlog_log_server_thread(void* arg)
{
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	int sockfd;
	int newsockfd;
	int clilen = 0;
	int domain = AF_INET;
	memset((void*)&serv_addr, 0, sizeof(serv_addr));

	printf("Initializing CTLOG for IPV4\n");
	clilen = sizeof(struct sockaddr_in);
	domain = AF_INET;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(g_nLogPort);

	if( (sockfd = socket(domain, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		fprintf(stderr, "CLOG: Failed to create socket\n");
		_exit(0);
	}

	if( bind(sockfd, (struct sockaddr*)&(serv_addr), sizeof(struct sockaddr_in)) < 0 ) {
		fprintf(stderr, "CLOG: Error in Binding\n");
		perror("RLOG");
		_exit(0);
	}

	listen(sockfd, 5);

	while(1)
	{
		newsockfd = accept(sockfd, (struct sockaddr*)&(cli_addr), (socklen_t *) &clilen);	
		if( newsockfd < 0 ) {
			fprintf(stderr, "CLOG: Error on accept\n");
			perror("CLOG");
			return 0;
		}

		/* If remote logging is disabled or there is already 1 client connected */
		if( g_bRemoteLoggingDisabled || g_nCliSocket ) {
			/* close the new connection and proceed */
			cmlog_close_connection(newsockfd);
			continue;
		} 

		g_nCliSocket = newsockfd;
	}

	return 0;
}

PRIVATE void cmlog_close_connection(int sockfd)
{
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
}

PRIVATE void cmlog_flush_data(int sig)
{
#ifdef CMLOG_USE_CIRCULAR_BUFFER
	g_threadRunFg = 0;
	cmlog_read_cirbuf();
#endif
	g_clogWriteCount = 0;

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

			cmlogS(OS_SIGSEGV, FATAL, strings[i]);
			//printf("BT[%d] : len [%d]: %s\n",i, strlen(sFunctions[i]),strings[i]);
			sprintf(buf+nStrLen, "	 in Function %s (from %s)\n", sFunctions[i], sFileNames[i]);
			nStrLen += strlen(sFunctions[i]) + strlen(sFileNames[i]) + 15;
		}
	
		cmlogS(OS_SIGSEGV, FATAL, buf);

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

   	sprintf(ts,"%04d/%02d/%02d %02d:%02d:%02d.%03d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec/1000));
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
   if ( dir == NULL )
   { 
      mkdir(g_logDir, O_RDWR);
   }
   else
   {
      closedir(dir);
   }

   /* remove old file from system */
   if( g_fileList[g_nCurrFileIdx][0] != '\0' )
      unlink(g_fileList[g_nCurrFileIdx]);

   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.bin",g_logDir, g_fileName, curTime );
   fp = fopen(g_fileList[g_nCurrFileIdx], "ab+");

   if( fp == NULL ) {
      fprintf(stderr, "Failed to open log file %s\n", g_fileList[g_nCurrFileIdx]);
      return;
   }

   fd = fileno(fp);

   g_fp = fp;
   g_fd = fd;

   if( fcntl(g_fd, F_SETFL, fcntl(g_fd, F_GETFL, 0) | O_NONBLOCK | O_ASYNC ) == -1 ) {
      fprintf(stderr, "RLOG: Cannot enable Buffer IO or make file non-blocking\n");
   }

   setvbuf ( fp , NULL, _IONBF, 1024 );//no buffer

   cmlog_write_file_header(fp);

   if( prev_fp != NULL )
      fclose(prev_fp);

   if( ++g_nCurrFileIdx == g_nMaxLogFiles )
      g_nCurrFileIdx = 0;

#ifdef CMLOG_USE_TTI_LOGGING
   cmlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));
#endif
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

void os_cmlog_set_remote_flag(int flag)
{
	g_bRemoteLoggingDisabled = !flag;
}

void os_cmlog_set_log_port(unsigned int port)
{
	g_nLogPort = port;
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
	fprintf(stderr, "File Size Limit:\t[%d]\n", g_uiMaxFileSizeLimit);
	fprintf(stderr, "Maximum Log Files:\t[%d]\n", g_nMaxLogFiles);
	fprintf(stderr, "Time Zone:\t\t[%s]\n", tz_name[0]);

	fprintf(stderr, "Console Logging:\t[Disabled]\n");
	fprintf(stderr, "Binary Logging:\t\t[Enabled]\n");
	fprintf(stderr, "Remote Logging:\t\t[%s]\n", g_bRemoteLoggingDisabled ? "Disabled" : "Enabled");
	fprintf(stderr, "Remote Logging Port:\t[%d]\n", g_nLogPort);
#ifdef CMLOG_USE_CIRCULAR_BUFFER
	fprintf(stderr, "Circular Buffer:\t[Enabled]\n");
	fprintf(stderr, "Circular BufferSize:\t[Actual:%ld][Derived:%ld]\n", g_cirMaxBufferSize/1024, g_cirMaxBufferSize);
#else
	fprintf(stderr, "Circular Buffer:\t[Disabled]\n");
#endif  /* CMLOG_USE_CIRCULAR_BUFFER */
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

	os_ctlog_printf_config();

	{
		os_thread_id_t tid;
		if(pthread_create(&tid, NULL, cmlog_log_server_thread, NULL) != 0 ) {
			fprintf(stderr, "Failed to initialize log server thread\n");
			_exit(0);
		}
	}

#ifdef CMLOG_USE_CIRCULAR_BUFFER
	{
		cmlog_init_specific();

		os_thread_id_t tid;
		os_thread_mutex_init(&g_logmutex, NULL);
		if( pthread_create(&tid, NULL, cmlog_cirbuf_read_thread, NULL) != 0 ) {
			fprintf(stderr, "Failed to initialize log server thread\n");
			_exit(0);
		}
		/* Initialize single circular buffer for all threads */
		g_pSingCirBuff = cmlog_register_thread("DUMMY");
	}
#endif

	cmlog_create_new_log_file();
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
   printf("Start Log Restriction\n");
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called by EnbApp after Cell Shutdown
//                 is triggered. This will enable to get all logs of shutdown.
//                 This function stops the log cnt limit. i.e Restricition on
//                 Number of logs getting logged into curcular will be removed
//////////////////////////////////////////////////////////////////////////
void os_cmlog_stop_count_limit(void)
{
   printf("Stop Log Restriction\n");
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


/* BINARY LOGGING */ 
#define CLOG_SAVE_TIME(_logTime) _logTime.ms_tti=numTtiTicks;
PRIVATE void cmlog_save_log_data(const void* buf, unsigned short len, unsigned int g_rlogWritePosIndex)
{
   ++g_clogWriteCount ;

   if((1 == g_writeCirBuf) || 
         ((g_clLogCntLimit == CL_LOG_COUNT_LIMIT_START) && 
          (g_clogWriteCount > g_maxClogCount)) || 
         (len > CLOG_FIXED_LENGTH_BUFFER_SIZE))
   {
      g_rlogPositionIndex --;
      g_logsDropCnt++;
      return;
   }

#ifdef CMLOG_USE_CIRCULAR_BUFFER
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
#else /* !CMLOG_USE_CIRCULAR_BUFFER */
   if(fwrite((const void*)buf, 1, CLOG_FIXED_LENGTH_BUFFER_SIZE, g_fp) == -1) 
   {
      fprintf(stderr, "Failed to write log data in file\n");
      perror("LOG");
      cmlog_create_new_log_file();
   }
#endif /* CMLOG_USE_CIRCULAR_BUFFER */

   CHECK_FILE_SIZE


   /*{
      static int maxlen = 0;
      if(len > maxlen) {
         maxlen = len;
         fprintf(stderr, "MAX BUFFER SIZE is binary mode is [%d]\n", maxlen);
      }
   }*/

}

void cmlogS( LOGID logId, os_clog_level_e logLevel, const char* str, ...)
{
   ARGDATA arg; unsigned short bufsize;


   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_STR;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 1;
   arg.logData.len = strlen(str);
   if(arg.logData.len > 0)
   {
      memcpy(arg.buf, (const void*)str, arg.logData.len);
      bufsize = sizeof(LOGDATA)+arg.logData.len;

      cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
   }
}

void cmlogH( LOGID logId, os_clog_level_e logLevel, const char* hex, int hexlen, ...)
{
   ARGDATA arg;
   int bufsize;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_HEX;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 1;
   arg.logData.len = hexlen;

   memcpy(arg.buf, (const void*)hex, hexlen);
   bufsize = sizeof(LOGDATA)+arg.logData.len;

   cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}

void cmlogSP(LOGID logId, os_clog_level_e logLevel, os_clog_level_e splType, unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...)
{
   SPL_ARGDATA arg;
   int bufsize;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_SPL;
   arg.logData.logLevel = logLevel;
   if( arg1 ) {
      arg.logData.numOfArgs = (arg2 == 0 ) ? 1 : (arg3==0 ? 2 : (arg4==0 ? 3 : 4));
   } else {
      arg.logData.numOfArgs = 0;
   }

   arg.logData.len  = sizeof(unsigned char) + sizeof(unsigned int) + (sizeof(unsigned int)*arg.logData.numOfArgs);

   arg.splEnum = splType;
   arg.splArg = splVal;
   arg.arg1 = arg1;
   arg.arg2 = arg2;
   arg.arg3 = arg3;
   arg.arg4 = arg4;

   bufsize = sizeof(LOGDATA)+arg.logData.len;

   cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}

void cmlog0( LOGID logId, os_clog_level_e logLevel, ...)
{
   //LOGDATA logData;
   ARG4DATA arg;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_STR;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 0;
   arg.logData.len = 0;

   cmlog_save_log_data((const void*)&arg, sizeof(LOGDATA), g_rlogPositionIndex++);	
}

void cmlog1( LOGID logId, os_clog_level_e logLevel, unsigned int arg1, ...)
{
   ARG4DATA arg;
   int bufsize;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_INT;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 1;
   arg.logData.len = sizeof(unsigned int);

   arg.arg1 = arg1;
   bufsize = sizeof(LOGDATA)+arg.logData.len;

   cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void cmlog2( LOGID logId, os_clog_level_e logLevel, unsigned int arg1, unsigned int arg2, ...)
{
   ARG4DATA arg;
   int bufsize;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_INT;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 2;
   arg.logData.len =  2 * sizeof(unsigned int);

   arg.arg1 = arg1;
   arg.arg2 = arg2;

   bufsize = sizeof(LOGDATA)+arg.logData.len;

   cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void cmlog3( LOGID logId, os_clog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, ...)
{
   ARG4DATA arg;
   int bufsize;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_INT;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 3;
   arg.logData.len = 3 * sizeof(unsigned int);

   arg.arg1 = arg1;
   arg.arg2 = arg2;
   arg.arg3 = arg3;

   bufsize = sizeof(LOGDATA)+arg.logData.len;

   cmlog_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void cmlog4( LOGID logId, os_clog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...)
{
   ARG4DATA arg;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_INT;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 4;
   arg.logData.len = 4 * sizeof(unsigned int);

   arg.arg1 = arg1;
   arg.arg2 = arg2;
   arg.arg3 = arg3;
   arg.arg4 = arg4;

   cmlog_save_log_data((const void*)&arg, sizeof(ARG4DATA), g_rlogPositionIndex++);	
}

