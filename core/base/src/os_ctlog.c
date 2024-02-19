/************************************************************************
 *File name: os_ctlog.c
 *Description: CTLOG_ALLOW_CONSOLE_LOGS
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02s
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
PRIVATE char g_fileName[MAX_FILENAME_LEN] = "ct";
PRIVATE char g_fileList[CLOG_MAX_FILES][MAX_FILENAME_LENGTH];

PRIVATE unsigned char g_nMaxLogFiles = 1;                       /* MAX Log Files 1 */
PRIVATE unsigned int g_uiMaxFileSizeLimit = MAX_FILE_SIZE;      /* Max File Size limit for each log file */
PRIVATE char tz_name[2][CLOG_TIME_ZONE_LEN + 1] = {"CST-8", ""};

PRIVATE int	g_nWrites = 0;                /* number of times log function is called */
PRIVATE int g_nCurrFileIdx = 0;           /* Current File Number index */

PRIVATE void ctlog_create_new_log_file(void);


#define CHECK_CTFILE_SIZE if( ++g_nWrites == 200 ) \
	{ \
		if( g_fp && ( (unsigned int)(ftell(g_fp)) > g_uiMaxFileSizeLimit) ) { \
			ctlog_create_new_log_file(); \
		}\
		g_nWrites = 0; \
	} 


PRIVATE void ctlog_flush_data(int sig)
{
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

PRIVATE void ctlog_catch_segViolation(int sig)
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
	
			//printf("BT[%d] : len [%d]: %s\n",i, strlen(sFunctions[i]),strings[i]);
			sprintf(buf+nStrLen, "	 in Function %s (from %s)\n", sFunctions[i], sFileNames[i]);
			nStrLen += strlen(sFunctions[i]) + strlen(sFileNames[i]) + 15;
		}
	
		os_log(FATAL, CLOG_SEGFAULT_STR, buf);
		//fflush(g_fp);

		free(strings);
	}
#endif
	ctlog_flush_data(SIGSEGV);
}

PRIVATE void ctlog_timestamp(char* ts)
{
    struct timeval tv;
    struct tm tm;

    os_gettimeofday(&tv);
    os_localtime(tv.tv_sec, &tm);

   	sprintf(ts,"%04d/%02d/%02d %02d:%02d:%02d.%03d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec/1000));
}

PRIVATE struct tm* ctlog_get_timestamp(int* usec)
{
   struct timeval tv;

   os_gettimeofday(&tv);
   *usec = tv.tv_usec;
   /* Obtain the time of day, and convert it to a tm struct. --*/
   return localtime (&tv.tv_sec);
}

PRIVATE void ctlog_create_new_log_file(void)
{
   FILE *fp, *prev_fp = g_fp;
   char curTime[CLOG_MAX_TIME_STAMP];
   int fd;
   char *temptr;
   DIR *dir = NULL;

   /* get current time, when file is created */
   ctlog_timestamp(curTime); 
   temptr = strchr(curTime, '.');
   if (temptr != NULL){
      *temptr = 0;
   }

   dir = opendir(g_logDir);

   if (dir == NULL){ 
      mkdir(g_logDir, O_RDWR);
   }
   else{
      closedir(dir);
   }

   /* remove old file from system */
   if(g_fileList[g_nCurrFileIdx][0] != '\0')
      unlink(g_fileList[g_nCurrFileIdx]);

   /* create file name, Example-> dbglog_2013_08_11_15_30_00 */
   sprintf(g_fileList[g_nCurrFileIdx], "%s/%s_%s.log",g_logDir, g_fileName, curTime);
   fp = fopen(g_fileList[g_nCurrFileIdx], "a+");

   if(fp == NULL) {
      fprintf(stderr, "Failed to open log file %s\n", g_fileList[g_nCurrFileIdx]);
      return;
   }

   fd = fileno(fp);

   g_fp = fp;
   g_fd = fd;

   if(fcntl(g_fd, F_SETFL, fcntl(g_fd, F_GETFL, 0) | O_NONBLOCK | O_ASYNC ) == -1) {
      fprintf(stderr, "RLOG: Cannot enable Buffer IO or make file non-blocking\n");
   }

   setvbuf ( fp , NULL, _IOLBF, 1024 );//line buffer

   if(prev_fp != NULL)
      fclose(prev_fp);

   if(++g_nCurrFileIdx == g_nMaxLogFiles)
      g_nCurrFileIdx = 0;
}

#ifdef CTLOG_ALLOW_CONSOLE_LOGS
PRIVATE void ctlog_add_stderr(void)
{
	os_ctlog_set_fileName("stderr");
	g_fp = stderr;
    return;
}
#endif

void os_ctlog_set_fileSize_limit(unsigned int maxFileSize)
{
	g_uiMaxFileSizeLimit = (maxFileSize == 0) ? MAX_FILE_SIZE : maxFileSize*1048576;
}

void os_ctlog_set_fileNum(unsigned char maxFiles)
{
	if(maxFiles > CLOG_MAX_FILES || maxFiles == 0) {
		g_nMaxLogFiles = CLOG_MAX_FILES;
		return;
	}
	g_nMaxLogFiles = maxFiles;
}

void os_ctlog_set_log_path(const char* logDir)
{
	strncpy(g_logDir, logDir, MAX_FILENAME_LEN);
}

void os_ctlog_printf_config(void)
{
	fprintf(stderr, "Log File:\t\t[%s]\n", g_fileName);
	fprintf(stderr, "Log level[%d]:\t\t[%s]\n",g_logLevel, g_logStr[g_logLevel]);
	fprintf(stderr, "File Size Limit:\t[%d]\n", g_uiMaxFileSizeLimit);
	fprintf(stderr, "Maximum Log Files:\t[%d]\n", g_nMaxLogFiles);
	fprintf(stderr, "Time Zone:\t\t[%s]\n", tz_name[0]);

	fprintf(stderr, "Binary Logging:\t\t[Disabled]\n");
	fprintf(stderr, "Remote Logging:\t\t[Disabled]\n");
	fprintf(stderr, "Console Logging:\t[%s]\n", (g_fp==stderr) ? "Enabled" : "Disabled" );
}

void os_ctlog_set_fileName(const char* fileName)
{
	strncpy(g_fileName, fileName, MAX_FILENAME_LEN);
}

void os_ctlog_set_log_level(os_clog_level_e logLevel)
{
	g_logLevel = logLevel;
}

void os_ctlog_set_module_mask(unsigned int modMask)
{
	g_modMask = (modMask == 0 ) ? 0 : (g_modMask ^ modMask);
}

void os_ctlog_init(void)
{
	signal(SIGSEGV, ctlog_catch_segViolation);
	signal(SIGBUS,  ctlog_catch_segViolation);
	signal(SIGINT,  ctlog_flush_data);

	os_ctlog_printf_config();

#ifdef CTLOG_ALLOW_CONSOLE_LOGS
	ctlog_add_stderr();
#else
	ctlog_create_new_log_file();
#endif
}

void os_ctlog_final(void)
{
	fflush(g_fp);
	fclose(g_fp);
}

PRIVATE void ctlog_hex_to_asii(char* p, const char* h, int hexlen)
{
   for(int i = 0; i < hexlen; i++, p+=3, h++){
	   sprintf(p, "%02x ", *h);
   }
}

#define TIME_PARAMS tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,tm->tm_hour, tm->tm_min,tm->tm_sec,usec/1000

void ctlogN(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, ...)
{
	va_list argList;
	char szTime[CLOG_MAX_TIME_STAMP];
	char szLog1[MAX_LOG_LEN], szLog2[MAX_LOG_LEN];

	ctlog_timestamp(szTime);
	snprintf(szLog1, MAX_LOG_LEN, "[%s][%s]%s:%s:%d %s:", szTime, modName, basename(file), func, line, g_logStr[logLevel]);

	va_start(argList,fmtStr);
	vsnprintf(szLog2, MAX_LOG_LEN, fmtStr, argList);
	va_end(argList);

	fprintf(g_fp, "%s%s",szLog1, szLog2);

#ifdef CTLOG_ALLOW_CONSOLE_LOGS
	fflush(g_fp);
#else
	CHECK_CTFILE_SIZE
#endif
}

void ctlogSPN(int logLevel, const char* modName, char* file, const char* func, int line, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...)
{
	va_list argList;
	char szTime[CLOG_MAX_TIME_STAMP];
	char szLog1[MAX_LOG_LEN], szLog2[MAX_LOG_LEN];

	ctlog_timestamp(szTime);
    snprintf(szLog1, MAX_LOG_LEN, "[%s][%s]%s:%s:%d %s:%s:%d:", szTime, modName, basename(file), func, line, g_logStr[logLevel], g_splStr[splType].name, splVal);

    va_start(argList,fmtStr);
    vsnprintf(szLog2, MAX_LOG_LEN, fmtStr, argList);
    va_end(argList);

    fprintf(g_fp, "%s%s",szLog1, szLog2);

#ifdef CTLOG_ALLOW_CONSOLE_LOGS
	fflush(g_fp);
#else
	CHECK_CTFILE_SIZE
#endif
}

void ctlogH(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, const char* hexdump, int hexlen, ...)
{
	int usec=0;
	char szHex[MAX_LOG_BUF_SIZE*3];

	struct tm* tm = ctlog_get_timestamp(&usec);
	ctlog_hex_to_asii(szHex, hexdump, hexlen);
	if (tm) fprintf(g_fp, fmtStr, TIME_PARAMS, modName, basename(file), func, line, g_logStr[logLevel], szHex);

#ifdef CTLOG_ALLOW_CONSOLE_LOGS
	fflush(g_fp);
#else
	CHECK_CTFILE_SIZE
#endif
}

