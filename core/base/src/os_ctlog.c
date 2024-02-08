/************************************************************************
 *File name: os_clog_f.c
 *Description: CLOG_USE_CIRCULAR_BUFFER + CLOG_USE_TTI_LOGGING  / CLOG_ENABLE_TEXT_LOGGING
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02s
************************************************************************/
#include "os_init.h"
#include "private/os_clog_priv.h"

/* Thread-specific data key visible to all threads */
os_thread_key_t	g_threadkey;
os_thread_mutex_t g_logmutex; /* Mutex to protect circular buffers */
os_thread_mutex_t g_condmutex;
os_thread_cond_t g_cond;

FILE* g_fp = NULL;       /* global file pointer */
int g_fd;                /* Global file descriptor for L2 & L3 */
char g_logDir[MAX_FILENAME_LEN];
char g_fileName[MAX_FILENAME_LEN];
char g_fileList[CLOG_MAX_FILES][MAX_FILENAME_LENGTH];
int g_logLevel = MAX_LOG_LEVEL; 
unsigned int g_modMask = 0; 

unsigned char g_nMaxLogFiles = 1;                       /* MAX Log Files 1 */
unsigned int g_uiMaxFileSizeLimit = MAX_FILE_SIZE;      /* Max File Size limit for each log file */
unsigned int g_cirMaxBufferSize = CLOG_MAX_CIRBUF_SIZE; /* Default circular buffer size 100Kb*/
unsigned int g_nLogPort = CLOG_REMOTE_LOGGING_PORT;     /* Remote Logging port */
unsigned char g_bRemoteLoggingDisabled = 0;             /* Remote logging flag */
char tzname[2][CLOG_TIME_ZONE_LEN + 1] = {"CST-8", NULL};


int	g_nWrites = 0;                /* number of times log function is called */
int g_nCliSocket = 0;             /* Socke descriptor if remote client is connected */
int g_nCurrFileIdx = 0;           /* Current File Number index */

char g_l2Buf[CLOG_MAX_CIRBUF_SIZE];                        /* L2 Buffer */

unsigned int numTtiTicks;          /* TTI Count */
int g_kIdx, g_action, g_storeKeys; /* Console input handling parameters */
char g_keyBuf[32];

PRIVATE void cl_close_connection(int sockfd);
PRIVATE void cl_create_new_log_file(void);


#ifdef CLOG_USE_CIRCULAR_BUFFER
THREAD_DATA* g_pCirList[CLOG_MAX_THREADS]; /* List of thread data pointers */
int g_nThreadsRegistered;          /* Number of threads registered */
unsigned char g_writeCirBuf = 0;
int thread_signalled;
THREAD_DATA *g_pSingCirBuff = NULL;
unsigned short g_prevLogOffset=0;
int g_threadRunFg = 0;

PRIVATE void cl_read_cirbuf(void);
#endif

unsigned int g_clogWriteCount = 0;
unsigned int g_maxClogCount   = 50;
unsigned int g_logsDropCnt    = 0;
cLogCntLmt   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_STOP;

#ifndef CLOG_ENABLE_TEXT_LOGGING
PRIVATE volatile unsigned int g_rlogPositionIndex=0;
#endif


PRIVATE void* cl_alloc(size_t mem_size)
{
	return os_malloc(mem_size);
}

PRIVATE void cl_free(void* p)
{
	os_free(p);
}

PRIVATE void* cl_calloc(size_t mem_size)
{
	return os_calloc(mem_size, 1);
}


#ifdef CLOG_USE_CIRCULAR_BUFFER

    #ifdef CLOG_USE_TTI_LOGGING
    #define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
	    { \
		    g_nWrites = 0; \
		    ctlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));\
	    } 
    #else
    #define CHECK_FILE_SIZE
    #endif /* CLOG_USE_TTI_LOGGING */

#else /* CLOG_USE_CIRCULAR_BUFFER */

	#ifdef CLOG_USE_TTI_LOGGING
	#define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
		{ \
			if( g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit ) { \
				cl_create_new_log_file(); \
			}\
			g_nWrites = 0; \
			ctlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));\
		} 
	#else
	#define CHECK_FILE_SIZE if( ++g_nWrites == 200 ) \
		{ \
			if( g_fp && ( (unsigned int)(ftell(g_fp)) > g_uiMaxFileSizeLimit) ) { \
				cl_create_new_log_file(); \
			}\
			g_nWrites = 0; \
		} 
	#endif /* CLOG_USE_TTI_LOGGING */
#endif /*  CLOG_USE_CIRCULAR_BUFFER */


#ifdef CLOG_USE_CIRCULAR_BUFFER

#define CHECK_CIRFILE_SIZE if( g_fp && ftell(g_fp) > g_uiMaxFileSizeLimit ) \
	{ \
		cl_create_new_log_file(); \
	}

PRIVATE void cl_deregister_thread(void* argv)
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

PRIVATE void cl_init_specific(void)
{
	pthread_key_create(&g_threadkey, &cl_deregister_thread);
}

void cl_set_specific(const void *pThrData)
{
	int retVal = pthread_setspecific(g_threadkey, pThrData);
	
	if( retVal!=0 ) {
      fprintf(stderr, "Failed to associate the value with the key or invalid key");
      _exit(0);
   }
}


PRIVATE THREAD_DATA* cl_register_thread(const char* taskName)
{
	THREAD_DATA* pThrData = (THREAD_DATA*) cl_calloc(sizeof(THREAD_DATA));

	if( pThrData == NULL ) {
		fprintf(stderr, "Failed to allocate memory for thread %s\n", taskName);
		_exit(0);
	}

	os_thread_mutex_lock(&g_logmutex);

	/* Allocate circular buffer */
	pThrData->logBuff = (unsigned char*) cl_alloc(g_cirMaxBufferSize);

	if( pThrData->logBuff == NULL ) {
		fprintf(stderr, "Failed to allocate memory [%ld] for thread %s\n",g_cirMaxBufferSize, taskName);
		_exit(0);
	}

	/* store task name */
	strcpy(pThrData->szTaskName, taskName);

	cl_set_specific(pThrData);

	pThrData->listIndex = g_nThreadsRegistered++;

	/* Store this pointerin global list, to access it later */
	g_pCirList[pThrData->listIndex]  = pThrData;

	os_thread_mutex_unlock(&g_logmutex);

	//fprintf(stderr, "rlRegisterThread: allocated CIRCULAR BUFFER of size [%ld]\n", g_cirMaxBufferSize);
	//fprintf(stderr, "rlRegisterThread: Total registered threads [%d]\n", g_nThreadsRegistered);

	return pThrData;
}

PRIVATE void* cl_cirbuf_read_thread(void* arg)
{
	struct timespec timeout;
	int retCode;

	//fprintf(stderr, "Circular Buffer Reader thread started\n");

	while(!g_threadRunFg)
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
			cl_read_cirbuf();
			continue;
		}

		cl_read_cirbuf();

		fprintf(stderr, "System is exiting ??");
		perror("cirbuf_read_thread");

		break;
	}

	return NULL;
}

PRIVATE void cl_read_cirbuf(void)
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

      if( pThrData == NULL )
         continue;

      writerPos = pThrData->logBufLen;

      //fprintf(stderr, "Thread [%ld] WritePos:[%ld] ReadPos:[%ld]\n", i+1, writerPos, pThrData->logReadPos);

      if( pThrData->logReadPos < writerPos  )
      {
         /* Calculate the delta data to be read from buffer */
         int dataLen = writerPos - pThrData->logReadPos;

         /* Write the data into file */
         if(fwrite(pThrData->logBuff+pThrData->logReadPos,1, dataLen, g_fp) == -1 ) 
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cl_create_new_log_file();
            continue;
         }

         /* If post processor connected send logs */
         if( g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff+pThrData->logReadPos, dataLen, 0 ) == -1 ) 
         {
            cl_close_connection(g_nCliSocket);
            g_nCliSocket = 0;
         }

         /* reset log read position to last known position */
         pThrData->logReadPos = writerPos;
      }
      else if ( pThrData->logReadPos > writerPos ) 
      {
         /* Calculate the remaining data left in the buffer */
         int dataLen = g_cirMaxBufferSize -  pThrData->logReadPos;			

         /* Write from last know position till end */
         if( fwrite(pThrData->logBuff+pThrData->logReadPos, 1, dataLen, g_fp) == -1 )
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cl_create_new_log_file();
            continue;
         }

         /* If post processor connected send logs */
         if( g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff+pThrData->logReadPos, dataLen, 0 ) == -1 ) 
         {
            cl_close_connection(g_nCliSocket);
            g_nCliSocket = 0;
         }

         /* Write from 0 to len position */
         if( fwrite(pThrData->logBuff, 1, writerPos, g_fp) == -1 )
         {
            fprintf(stderr, "Failed to write data len %d\n", dataLen);
            cl_create_new_log_file();
            continue;
         }

         /* If post processor connected send logs */
         if( g_nCliSocket &&  os_send(g_nCliSocket, pThrData->logBuff, writerPos, 0 ) == -1 ) 
         {
            cl_close_connection(g_nCliSocket);
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

#ifndef CLOG_ENABLE_TEXT_LOGGING
PRIVATE EndianType cl_getCPU_endian(void)
{
    unsigned short x;
    unsigned char c;
 
    x = 0x0001;
    c = *(unsigned char *)(&x);

	return ( c == 0x01 ) ? little_endian : big_endian;
}

PRIVATE void cl_write_file_header(FILE* fp)
{
	FILE_HEADER fileHdr;

	memset(&fileHdr, 0, sizeof(FILE_HEADER));

	fileHdr.endianType = cl_getCPU_endian();
	fileHdr.dummy32 = 2818049;
	fileHdr.END_MARKER = 0xFFFF;
	strncpy(fileHdr.szTimeZone, tzname[0], CLOG_TIME_ZONE_LEN);
   	fileHdr.time_sec = time(NULL);

	if(fwrite((const void*)&fileHdr, 1, sizeof(FILE_HEADER), fp) ==  -1 )
	{
		fprintf(stderr, "Failed to write file header\n");
		cl_create_new_log_file();
	}
}

PRIVATE void* cl_log_server_thread(void* arg)
{
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  int sockfd;
  int newsockfd;
  int clilen = 0;
  int domain = AF_INET;
  memset((void*)&serv_addr, 0, sizeof(serv_addr));


  if(gIpType == CM_IPV4ADDR_TYPE)
  {
     printf("Initializing CLOG for IPV4- %ld\n",gIpType);
     clilen = serv_addr.len = sizeof(struct sockaddr_in);
     domain = AF_INET;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(g_nLogPort);
  }
  else
  {
     printf("NOT support CLOG for IPV6 - %ld\n",gIpType);
	 _exit(0);
	 
  }
	if( (sockfd = socket(domain, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		fprintf(stderr, "CLOG: Failed to create socket\n");
		_exit(0);
	}

	if( bind(sockfd, (struct sockaddr*)&(serv_addr),serv_addr.len) < 0 ) {
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
			closeConnection(newsockfd);
			continue;
		} 

		g_nCliSocket = newsockfd;
	}

	return 0;
}

PRIVATE void cl_close_connection(int sockfd)
{
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
}

#endif

PRIVATE void cl_flush_data(int sig)
{
#ifdef CLOG_USE_CIRCULAR_BUFFER
	g_threadRunFg = 1;
	cl_read_cirbuf();
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

PRIVATE void cl_catch_segViolation(int sig)
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
	
#ifndef CLOG_ENABLE_TEXT_LOGGING
			ctlogS(OS_SIGSEGV, FATAL, strings[i]);
#endif
			printf("BT[%d] : len [%d]: %s\n",i, strlen(sFunctions[i]),strings[i]);
			sprintf(buf+nStrLen, "	 in Function %s (from %s)\n", sFunctions[i], sFileNames[i]);
			nStrLen += strlen(sFunctions[i]) + strlen(sFileNames[i]) + 15;
		}
	
#ifdef CLOG_ENABLE_TEXT_LOGGING
		ctlogS(g_logStr[FATAL], "RLOG", "NULL", 0, FMTSTR CLOG_SEGFAULT_STR, buf);
		fflush(g_fp);
#else
		ctlogS(OS_SIGSEGV, FATAL, buf);
#endif

		free(strings);
	}
#endif
	cl_flush_data(SIGSEGV);
}

PRIVATE void cl_timestamp(char* ts)
{
    struct timeval tv;
    struct tm tm;

    os_gettimeofday(&tv);
    os_localtime(tv.tv_sec, &tm);

   	sprintf(ts,"%0.4d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d.%0.6d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,tm->tm_sec, tv.tv_usec);
}

PRIVATE struct tm* cl_get_timestamp(int* usec)
{
   struct timeval tv;

   os_gettimeofday(&tv);
   *usec = tv.tv_usec;
   /* Obtain the time of day, and convert it to a tm struct. --*/
   return localtime (&tv.tv_sec);
}

PRIVATE void cl_create_new_log_file(void)
{
   FILE *fp, *prev_fp = g_fp;
   char curTime[CLOG_MAX_TIME_STAMP];
   int fd;
   char *temptr;
   DIR *dir = NULL;

   /* get current time, when file is created */
   cl_timestamp(curTime); 
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

#ifdef CLOG_ENABLE_TEXT_LOGGING
   /* create file name, Example-> dbglog_2013_08_11_15_30_00 */
   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.txt",g_logDir, g_fileName, curTime );
   fp = fopen(g_fileList[g_nCurrFileIdx], "w+");
#else
   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.bin",g_logDir, g_fileName, curTime );
   fp = fopen(g_fileList[g_nCurrFileIdx], "ab+");
#endif

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

#ifdef CLOG_ENABLE_TEXT_LOGGING
   setvbuf ( fp , NULL, _IOLBF, 1024 );//line buffer
#else
   setvbuf ( fp , NULL, _IONBF, 1024 );//no buffer
#endif

#ifndef CLOG_ENABLE_TEXT_LOGGING
   cl_write_file_header(fp);
#endif

   if( prev_fp != NULL )
      fclose(prev_fp);

   if( ++g_nCurrFileIdx == g_nMaxLogFiles )
      g_nCurrFileIdx = 0;

#ifndef CLOG_ENABLE_TEXT_LOGGING
#ifdef CLOG_USE_TTI_LOGGING
   ctlog1(TIME_REFERENCE, NONE, (unsigned int)time(NULL));
#endif
#endif
}

PRIVATE void ctlog_add_stderr(void)
{
	os_ctlog_set_fileName("stdout");
	g_fp = stderr;
    return;
}

void os_ctlog_set_fileSize_limit(unsigned int maxFileSize)
{
	g_uiMaxFileSizeLimit = (maxFileSize == 0) ? MAX_FILE_SIZE : maxFileSize*1048576;
}

void os_ctlog_set_fileNum(unsigned char maxFiles)
{
	if( maxFiles > CLOG_MAX_FILES || maxFiles == 0 ) {
		g_nMaxLogFiles = CLOG_MAX_FILES;
		return;
	}
	g_nMaxLogFiles = maxFiles;
}

void os_ctlog_set_remote_flag(int flag)
{
	g_bRemoteLoggingDisabled = !flag;
}

void os_ctlog_set_log_port(unsigned int port)
{
	g_nLogPort = port;
}

void os_ctlog_set_log_path(const char* logDir)
{
	strncpy(g_logDir, logDir, MAX_FILENAME_LEN);
}

void os_ctlog_set_circular_bufferSize(unsigned int bufSize)
{
	g_cirMaxBufferSize = bufSize*1024;
	g_cirMaxBufferSize = (g_cirMaxBufferSize/50) * 50;
}

void os_ctlog_printf_config(void)
{
	fprintf(stderr, "Log File:\t\t[%s]\n", g_fileName);

#ifdef CLOG_ENABLE_TEXT_LOGGING

    os_log_domain_t *domain, *saved_domain;
    os_list_for_each_safe(&domain_list, saved_domain, domain){
		fprintf(stderr, "[%s]Log level[%d]:\t\t[%s]\n",domain->name, domain->level, g_logStr[domain->level]);
	}
#else
	fprintf(stderr, "Log level[%d]:\t\t[%s]\n",g_logLevel, g_logStr[g_logLevel]);
#endif
	fprintf(stderr, "File Size Limit:\t[%ld]\n", g_uiMaxFileSizeLimit);
	fprintf(stderr, "Maximum Log Files:\t[%d]\n", g_nMaxLogFiles);
	fprintf(stderr, "Time Zone:\t\t[%s]\n", tzname[0]);


#ifdef CLOG_ENABLE_TEXT_LOGGING
	fprintf(stderr, "Binary Logging:\t\t[Disabled]\n");
	fprintf(stderr, "Remote Logging:\t\t[Disabled]\n");
	fprintf(stderr, "Console Logging:\t[%s]\n", (g_fp==stderr) ? "Enabled" : "Disabled" );
#else
	fprintf(stderr, "Console Logging:\t[Disabled]\n");
	fprintf(stderr, "Binary Logging:\t\t[Enabled]\n");
	fprintf(stderr, "Remote Logging:\t\t[%s]\n", g_bRemoteLoggingDisabled ? "Disabled" : "Enabled");
	fprintf(stderr, "Remote Logging Port:\t[%ld]\n", g_nLogPort);
#ifdef CLOG_USE_CIRCULAR_BUFFER
	fprintf(stderr, "Circular Buffer:\t[Enabled]\n");
	fprintf(stderr, "Circular BufferSize:\t[Actual:%ld][Derived:%ld]\n", 
			g_cirMaxBufferSize/1024, g_cirMaxBufferSize);
#else
	fprintf(stderr, "Circular Buffer:\t[Disabled]\n");
#endif  /* CLOG_USE_CIRCULAR_BUFFER */
#endif /* CLOG_ENABLE_TEXT_LOGGING */

}

void os_ctlog_enable_coredump(bool enable_core)
{
#ifdef HAVE_SETRLIMIT
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = enable_core ? RLIM_INFINITY : 0;
	setrlimit(RLIMIT_CORE, &core_limits);
#endif
}

void os_ctlog_set_fileName(const char* fileName)
{
	strncpy(g_fileName, fileName, MAX_FILENAME_LEN);
}

void os_ctlog_set_log_level(os_ctlog_level_e logLevel)
{
	g_logLevel = logLevel;
}

void os_ctlog_set_module_mask(unsigned int modMask)
{
	g_modMask =  (modMask == 0 ) ? 0 : (g_modMask ^ modMask);
}

void os_ctlog_init(void)
{
	signal(SIGSEGV, cl_catch_segViolation);
	signal(SIGBUS,  cl_catch_segViolation);
	signal(SIGINT,  cl_flush_data);
	/* set rate limit count for L3 Logs */
	g_maxClogCount = CLOG_LIMIT_L3_COUNT;//CLOG_LIMIT_L2_COUNT

	os_ctlog_printf_config();

#ifndef CLOG_ENABLE_TEXT_LOGGING
	{
		os_thread_id_t tid;
		if(pthread_create(&tid, NULL, &cl_log_server_thread, NULL) != 0 ) {
			fprintf(stderr, "Failed to initialize log server thread\n");
			_exit(0);
		}
	}

#ifdef CLOG_USE_CIRCULAR_BUFFER
	{
		cl_init_specific();

		os_thread_id_t tid;
		os_thread_mutex_init(&g_logmutex, NULL);
		if( pthread_create(&tid, NULL, &cl_cirbuf_read_thread, NULL) != 0 ) {
			fprintf(stderr, "Failed to initialize log server thread\n");
			_exit(0);
		}
		/* Initialize single circular buffer for all threads */
		g_pSingCirBuff = cl_register_thread("DUMMY");
	}

#endif
#endif

	cl_create_new_log_file();
#endif
}

#ifdef CLOG_ENABLE_TEXT_LOGGING 

#define TIME_PARAMS tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,tm->tm_hour, tm->tm_min,tm->tm_sec,usec

void ctlogS(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, const char* str, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
   	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, str);

	CHECK_FILE_SIZE
}

void ctlogH(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, const char* hexdump, int hexlen, ...)
{
	int usec=0;
	char szHex[MAX_LOG_BUF_SIZE*3];

	struct tm* tm = cl_get_timestamp(&usec);
	hex_to_asii(szHex, hexdump, hexlen);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, szHex);

	CHECK_FILE_SIZE
}

void ctlogSP(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, log_sp_arg_e splType,
				unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, g_splStr[splType], splVal, 
			arg1, arg2, arg3, arg4);

	CHECK_FILE_SIZE
}


void ctlog0(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel);

	CHECK_FILE_SIZE
}

void ctlog1(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int arg1, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, arg1);

	CHECK_FILE_SIZE
}

void ctlog2(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int arg1, unsigned int arg2, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, arg1, arg2);

	CHECK_FILE_SIZE
}

void ctlog3(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, 
				unsigned int arg1, unsigned int arg2, unsigned int arg3, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, arg1, arg2, arg3);

	CHECK_FILE_SIZE
}

void ctlog4(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, 
				unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...)
{
	int usec=0;

	struct tm* tm = cl_get_timestamp(&usec);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, lineno, strLogLevel, arg1, arg2, arg3, arg4);

	CHECK_FILE_SIZE
}

void ctlogN(int logLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, ...)
{
	va_list argList;
	char szTime[CLOG_MAX_TIME_STAMP];
	char szLog1[MAX_LOG_LEN], szLog2[MAX_LOG_LEN];

	cl_timestamp(szTime);
	snprintf(szLog1, MAX_LOG_LEN, "[%s][%s]%s:%s:%d %s:", szTime, modName, basename(file), func, lineno, g_logStr[logLevel]);

	va_start(argList,fmtStr);
	vsnprintf(szLog2, MAX_LOG_LEN, fmtStr, argList);
	va_end(argList);

	fprintf(g_fp, "%s%s",szLog1, szLog2);

	CHECK_FILE_SIZE

}

void ctlogSPN(int logLevel, const char* modName, const char* file, const char* func, int lineno, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...)
{
	va_list argList;
	char szTime[CLOG_MAX_TIME_STAMP];
	char szLog1[MAX_LOG_LEN], szLog2[MAX_LOG_LEN];

	cl_timestamp(szTime);
    snprintf(szLog1, MAX_LOG_LEN, "[%s][%s]%s:%s:%d %s:%s:%ld:", szTime, modName, basename(file), func, lineno, g_logStr[logLevel], g_splStr[splType], splVal);

    va_start(argList,fmtStr);
    vsnprintf(szLog2, MAX_LOG_LEN, fmtStr, argList);
    va_end(argList);

    fprintf(g_fp, "%s%s",szLog1, szLog2);

	CHECK_FILE_SIZE
}

#endif

/* BINARY LOGGING */ 
#ifndef  CLOG_ENABLE_TEXT_LOGGING
#define CLOG_SAVE_TIME(_logTime) _logTime.ms_tti=numTtiTicks;
PRIVATE void cl_save_log_data(const void* buf, unsigned short len, unsigned int g_rlogWritePosIndex)
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

#ifdef CLOG_USE_CIRCULAR_BUFFER
   unsigned int logWritePointerPosition;
   THREAD_DATA* p = (THREAD_DATA*) g_pSingCirBuff;

   /* if buffer is about to full, write till end and continue writing from begining */
   if( ((g_rlogWritePosIndex+1) * CLOG_FIXED_LENGTH_BUFFER_SIZE) > g_cirMaxBufferSize ) 
   {
      /* setting this flag to 1 to avoid other threads
         to write in same circular buffer */
      g_writeCirBuf = 1;
      /* Start globalPositionIndex again */
      g_rlogPositionIndex = 0;

      /* if reader has not read initial data, minmum buffer size should be 100Kb */
      if( p->logReadPos < CLOG_READ_POS_THRESHOLD && !thread_signalled ) {
         pthread_cond_signal(&g_cond); /* this will wakeup thread */
      }

      /* we are unlikely to hit this condition, but to prevent corruption of binary logs */
      /* we cannot write the data, if we write, data will be corrected forever */
      if( CLOG_FIXED_LENGTH_BUFFER_SIZE > p->logReadPos ) {
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
      if( p->logReadPos > p->logBufLen && (p->logReadPos - p->logBufLen) < CLOG_READ_POS_THRESHOLD ) 
         pthread_cond_signal(&g_cond); /* this will wakeup thread */

      logWritePointerPosition = (g_rlogWritePosIndex * CLOG_FIXED_LENGTH_BUFFER_SIZE) + g_prevLogOffset;

      memcpy(p->logBuff+logWritePointerPosition, buf, len);
      p->logBufLen += CLOG_FIXED_LENGTH_BUFFER_SIZE;
   }
#else /* !CLOG_USE_CIRCULAR_BUFFER */
   if(fwrite((const void*)buf, 1, CLOG_FIXED_LENGTH_BUFFER_SIZE, g_fp) == -1 ) 
   {
      fprintf(stderr, "Failed to write log data in file\n");
      perror("LOG");
      cl_create_new_log_file();
   }
#endif /* CLOG_USE_CIRCULAR_BUFFER */

   CHECK_FILE_SIZE


   /*{
      static int maxlen = 0;
      if(len > maxlen) {
         maxlen = len;
         fprintf(stderr, "MAX BUFFER SIZE is binary mode is [%d]\n", maxlen);
      }
   }*/

}

void ctlogS( LOGID logId, os_ctlog_level_e logLevel, const char* str, ...)
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

      cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
   }
}

void ctlogH( LOGID logId, os_ctlog_level_e logLevel, const char* hex, int hexlen, ...)
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

   cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void ctlogSP(LOGID logId, os_ctlog_level_e logLevel, os_ctlog_level_e splType, unsigned int splVal, unsigned int arg1, unsigned int arg2,
      unsigned int arg3, unsigned int arg4, ...)
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

   cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}

void ctlog0( LOGID logId, os_ctlog_level_e logLevel, ...)
{
   //LOGDATA logData;
   ARG4DATA arg;

   CLOG_SAVE_TIME(arg.logData.logTime);

   arg.logData.logId = logId;
   arg.logData.argType = LOG_ARG_STR;
   arg.logData.logLevel = logLevel;
   arg.logData.numOfArgs = 0;
   arg.logData.len = 0;

   cl_save_log_data((const void*)&arg, sizeof(LOGDATA), g_rlogPositionIndex++);	
}

void ctlog1( LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, ...)
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

   cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void ctlog2( LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, ...)
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

   cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void ctlog3( LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, ...)
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

   cl_save_log_data((const void*)&arg, bufsize, g_rlogPositionIndex++);	
}
void ctlog4( LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...)
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

   cl_save_log_data((const void*)&arg, sizeof(ARG4DATA), g_rlogPositionIndex++);	
}
#endif

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called by EnbApp after Cell is UP.
//                 This function start the log cnt limit. i.e Number of logs
//                 getting logged into curcular will be restricted to 100 logs.
//////////////////////////////////////////////////////////////////////////
void os_ctlog_start_count_limit(void)
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
void os_ctlog_stop_count_limit(void)
{
   printf("Stop Log Restriction\n");
   g_clLogCntLimit = CL_LOG_COUNT_LIMIT_STOP;
}

//////////////////////////////////////////////////////////////////////////
//  @Discription : This function will be called every 10 ms whenever 
//                 application layer update the tti count. To reset the 
//                 log rate control. And Enable logging for next 10 ms
//////////////////////////////////////////////////////////////////////////
void os_ctlog_reset_rate_limit(void)
{
    g_clogWriteCount = 0;
    g_logsDropCnt = 0;
}

/* This function will get tick from RLC/CL and will process logs
   according to tick threshold. */
void os_ctlog_update_ticks(void)
{
   static unsigned int rlogTickCount;
   numTtiTicks++;
   if(++rlogTickCount >= CLOGTICKSCNTTOPRCLOGS)
   {
      rlogTickCount = 0;
      os_ctlog_reset_rate_limit(); /* Resetting rlog write count to 0 */ 
   }
   return;
}

void hex_to_asii(char* p, const char* h, int hexlen)
{
	for(int i = 0; i < hexlen; i++, p+=3, h++){
		sprintf(p, "%02x ", *h);
	}
}


