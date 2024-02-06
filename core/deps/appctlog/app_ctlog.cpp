/************************************************************************
 *File name: app_ctlog.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#include "os_init.h"
#include "private/os_clog_priv.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

//#define TEST_MODE

namespace cmdlinearg {
	int g_port = 0;
	std::string g_sFileDb;  /* File DB */
	std::string g_sLogDb;   /* Log DB */
	std::string g_sOutFile; /* Output file */
	std::string g_sBinLogFile; /* Input binary log file */
	std::string g_ipAddr;
}

typedef struct
{
  int lineno;
  std::string modName;
  std::string file;
  std::string logStr;
}LOG_INFO;

static struct option long_options[] =
{
   {"port",     required_argument,   0, 'p'},
   {"logdb",    required_argument,   0, 'l'},
   {"ofile",    required_argument,   0, 'o'},
   {"blog",     required_argument,   0, 'b'},
   {"ipaddr",   required_argument,   0, 'i'},
   {0, 0, 0, 0}
};

std::map<unsigned short, std::string> g_mFileInfo;
std::map<LOGID, LOG_INFO> g_mLogInfo;
FILE* g_fp=stderr;
unsigned int g_ttiCount = 0;
time_t g_basetimeSec;

void printUsage()
{
  printf("Usage: app [-l <logdb>] [-b <binlog> | -p <port> ] [-o <outfile>] \n");
  printf("\t[--logdb  | -l <input log db > ]\n");
  printf("\t[--binlog | -b <input binary log file> ]\n");
  printf("\t[--ofile  | -o <output text log file> ]\n");
  printf("\t[--port   | -p <port number> ]\n");
  _exit(0);
}

void printLogTime(LOGTIME & ltime)
{
  struct tm* tm;
  time_t curTimeSec;
  unsigned int secElapsedFromBeg;
  unsigned int ttiNum; 
  unsigned int miliseconds;

  ttiNum = ltime.ms_tti;
  secElapsedFromBeg = ttiNum/1000;
  miliseconds = ttiNum%1000;
  curTimeSec   = g_basetimeSec + secElapsedFromBeg;

  /* Obtain the time of day, and convert it to a tm struct. --*/
  tm = localtime (&curTimeSec);

  if(NULL != tm) 
   {
      fprintf(g_fp,"[%0.4d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d.%0.3d]", tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min,tm->tm_sec,miliseconds);
   }
}


void logLevN(LOGDATA & log, LOG_INFO & logInfo, const char* fmtStr, ...)
{
	va_list argList;
	const char* file = logInfo.file.c_str();

	printLogTime(log.logTime);
	fprintf(g_fp, "[%s]%s:%d\n%s:", logInfo.modName.c_str(), file, logInfo.lineno, g_logStr[log.logLevel]);

	va_start(argList,fmtStr);
	vfprintf(g_fp, fmtStr, argList);
	va_end(argList);
}

void logLevS(LOGDATA & log, LOG_INFO & logInfo, const char* fmtStr, const char* logStr)
{
   const char* file = logInfo.file.c_str();
   std::string argStr(logStr, log.len);
   printLogTime(log.logTime);
   fprintf(g_fp, "[%s]%s:%d\n%s:", logInfo.modName.c_str(), file, logInfo.lineno, g_logStr[log.logLevel]);
   fprintf(g_fp, fmtStr, argStr.c_str());
}


void logIntArg(ARG4DATA* log)
{
#ifdef TEST_MODE
  printf("LOG ARG: INT, LEVEL: %s\n", g_logStr[log->logData.logLevel]);
#endif
  
  LOG_INFO & logInfo = g_mLogInfo[log->logData.logId];
  logLevN(log->logData,logInfo,logInfo.logStr.c_str(),log->arg1,log->arg2,log->arg3,log->arg4); 
}


void logString(ARGDATA & log)
{
#ifdef TEST_MODE
  printf("LOG ID %ld LOG ARG STRING, LEVEL: %s\n", log.logData.logId, g_logStr[log.logData.logLevel]);
#endif
  
  LOG_INFO & logInfo = g_mLogInfo[log.logData.logId];
  logLevS(log.logData, logInfo, logInfo.logStr.c_str(), log.buf); 
}

void logHexDump(ARGDATA & log)
{
#ifdef TEST_MODE
  	printf("LOG ARG HEX, LEVEL: %s\n", g_logStr[log.logData.logLevel]);
#endif
  
	LOG_INFO & logInfo = g_mLogInfo[log.logData.logId];

	char szHex[MAX_LOG_BUF_SIZE*3];
	hex_to_asii(szHex, log.buf, log.logData.len);

	logLevN(log.logData, logInfo, logInfo.logStr.c_str(), szHex); 
}

void logLevSpl(SPL_ARGDATA* log, LOG_INFO & logInfo, const char* fmtStr, ...)
{
	va_list argList;
	const char* file = logInfo.file.c_str();

	printLogTime(log->logData.logTime);
	fprintf(g_fp, "[%s]%s:%d\n%s:%s:%d:", logInfo.modName.c_str(), file, logInfo.lineno, g_logStr[log->logData.logLevel], g_splStr[log->splEnum], log->splArg);

	va_start(argList,fmtStr);
	vfprintf(g_fp, fmtStr, argList);
	va_end(argList);
}

void logArgSpl(SPL_ARGDATA* log)
{
#ifdef TEST_MODE
	printf("LOG ARG SPL, LEVEL: %s\n", g_logStr[log->logData.logLevel]);
#endif
	LOG_INFO & logInfo = g_mLogInfo[log->logData.logId];
	const char* file = logInfo.file.c_str();

	logLevSpl(log, logInfo, logInfo.logStr.c_str(),log->arg1, log->arg2, log->arg3, log->arg4);
}


bool invalidLogId(LOGID logId)
{
   /* if this is a time reference this is valid log */
   if( g_mLogInfo.find(logId) == g_mLogInfo.end() )
   {
      if(logId != TIME_REFERENCE)
      {
         fprintf(stderr, "Invalid log id %d\n",logId);
      }
      return true;
   }
   return false;
}

void printLog(ARGDATA & log)
{

  if( invalidLogId(log.logData.logId) )
    return;
  
  switch( log.logData.argType )
  {
    case LOG_ARG_INT:
      logIntArg((ARG4DATA*)&log);
      break;

    case  LOG_ARG_STR:
      logString(log);
      break;

    case LOG_ARG_HEX:
      logHexDump(log);
      break;

    case LOG_ARG_SPL:
      logArgSpl((SPL_ARGDATA*)&log);
      break;
  }
}


bool cmp_tti(ARGDATA & log1, ARGDATA & log2)
{
  return log2.logData.logTime.ms_tti > log1.logData.logTime.ms_tti ? true : false;
}

void readCmdLineArgs(int argc,char **argv)
{
  using namespace cmdlinearg;
   /* getopt_long stores the option index here */
   int c, option_index = 0;

   while((c = getopt_long(argc, argv, "p:f:l:o:b:i:", long_options,&option_index)) != -1 )
   {
      switch(c)
       {
           case 'p':
                std::cout<<"option -p with value = " << optarg << std::endl;
                cmdlinearg::g_port = atoi(optarg);
                break;
           case 'l':
                std::cout<<"option -l with value = "<< optarg << std::endl;
                cmdlinearg::g_sLogDb = optarg;
                break;
           case 'o': 
                std::cout<<"option -o with value = " << optarg << std::endl;
                cmdlinearg::g_sOutFile = optarg;
                break;
           case 'b': 
                std::cout<<"option -b with value = " << optarg << std::endl;
                cmdlinearg::g_sBinLogFile = optarg;
                break;
           case 'i': 
                std::cout<<"option -i with value = " << optarg << std::endl;
                cmdlinearg::g_ipAddr = optarg;
                break;
        defult:
          std::cout << "Invalid arguments" << std::endl;
          exit(0);
    }
  }
  if(g_port == 0 && g_sBinLogFile.empty()) printUsage();

}

void loadLogDb()
{
	std::ifstream logdb(cmdlinearg::g_sLogDb.c_str());

	if(logdb.is_open() == false) {
		fprintf(stderr, "Failed to open file %s\n", cmdlinearg::g_sLogDb.c_str());
		_exit(0);
	}

	LOGID logId; LOG_INFO logInfo;
	std::string line;
	std::string & lstr = logInfo.logStr;

	while( getline(logdb, line) )
	{
		std::istringstream iss(line);
		int pos = 0, newpos=0;

		logId = atol( line.substr(pos, newpos=line.find(';', pos)).c_str() );

		pos = newpos+1;
		logInfo.lineno = atol( line.substr(pos, (newpos=line.find(';', pos)) - pos).c_str() );

		pos = newpos+1;
		logInfo.modName = line.substr(pos, (newpos=line.find(';', pos)) - pos);

		pos = newpos+3;
		logInfo.file = line.substr(pos, (newpos=line.find('\"', pos)) - pos);

		pos=line.find('\"',newpos+1) + 1;
		logInfo.logStr = line.substr(pos, (newpos=line.find_last_of("\"", pos)) - pos);
		lstr.erase(std::remove(lstr.begin(),lstr.end(),'\"')++,lstr.end());


#ifdef TEST_MODE
		std::cout << "LOG Id:" << logId << " FILE:" << logInfo.file << " lineno:" << logInfo.lineno;
		std::cout << " MOD Name: " << logInfo.modName << std::endl;
		std::cout << "LOG STR:[" << logInfo.logStr << "]" <<  std::endl;
#endif

		logInfo.logStr+= "\n\n";

		g_mLogInfo[logId] = logInfo;
  }

  logdb.close();

  logInfo.file = "NULL";
  logInfo.lineno = 0;
  logInfo.modName = "CLOG";
  logInfo.logStr =  CLOG_SEGFAULT_STR; 
  g_mLogInfo[SIGSEGV] = logInfo;
  
}

void openLogFile(const char* file)
{
   g_fp = fopen(file, "w+");
   
  if( g_fp == NULL ) 
  {
      fprintf(stderr, "Failed to open log file %s\n", file);
      _exit(0);
   }

   fprintf(stderr, "Log Output will be written in %s\n", file);
}

void processLogs(int fd, bool (*fpReadLog)(int, ARGDATA &))
{
  ARGDATA log;
  bool bSortingRequired = false, bStartBuffering = false;
  std::list<ARGDATA> logList;

  while((*fpReadLog)(fd, log)) 
  {
    if( log.logData.logId == TIME_DELIMITER ) 
    {
      bStartBuffering = (bStartBuffering == false) ? true : false;
      if( bStartBuffering == true ) {
        bSortingRequired = true;
        continue;
      }
    }

    if( bStartBuffering ) {
      logList.push_back(log);
      continue;
    }
  
    if( bSortingRequired ) {
      logList.sort(cmp_tti);  
      std::for_each(logList.begin(), logList.end(), printLog);
      logList.clear();
      bSortingRequired = false;
      bStartBuffering = false;
      continue;
    }

    printLog(log);
  }

  /* connection was closed due to some error, but we still have some logs */
  if( bStartBuffering && !logList.empty() ) 
  {
    logList.sort(cmp_tti);  
    std::for_each(logList.begin(), logList.end(), printLog);
  }

  close(fd);
}

bool readFileLogs(int fd, ARGDATA & log)
{
	int len = read(fd, (void*)&log.logData, sizeof(LOGDATA));

	if( len <= 0 ) {
	return false;
	}

#ifdef CLOG_MULTI_CIRCULAR_BUFFER
	if( log.logData.len && read(fd, (void*)log.buf, log.logData.len) <= 0 ) {
	return false;
	}
#else
	unsigned short size = CLOG_FIXED_LENGTH_BUFFER_SIZE - sizeof(LOGDATA);
	//  if( log.logData.len && read(fd, (void*)log.buf, size) <= 0 ) {
	if( read(fd, (void*)log.buf, size) <= 0 ) {
	return false;
	}
#endif

  return true;
}


int openBinLogFile(void)
{
	int fd = open(cmdlinearg::g_sBinLogFile.c_str(), O_RDONLY);

	if( fd == -1) {
	  fprintf(stderr, "Failed to open log file %s\n", cmdlinearg::g_sBinLogFile.c_str());
	_exit(0);
	}

	FILE_HEADER fileHdr;

	int len = read(fd, (void*)&fileHdr, sizeof(FILE_HEADER));

	if( fileHdr.END_MARKER != 0xFFFF ) {
	fprintf(stderr, "Invalid file header\n");
	_exit(0);
	}

	fprintf(stderr, "FILE ENDIAN: %s\n", fileHdr.endianType == big_endian ? "BIG ENDIAN" : "LITTLE ENDIAN");
	fprintf(stderr, "TIME ZONE: %s\n", fileHdr.szTimeZone);

	setenv("TZ", fileHdr.szTimeZone, 1);
	tzset();

	if( fileHdr.endianType == big_endian ) {
	}
	g_basetimeSec = fileHdr.time_sec;
	return fd;
}

bool readRemoteLogs(int sockfd, ARGDATA & log)
{
	int len;

	while( (len = recv(sockfd, (void*)&log.logData, RLOG_FIXED_LENGTH_BUFFER_SIZE, MSG_WAITALL)) == 0 );
		if( len < 0 ) {
		return false;
	}

	return true;
}


int connectToLogServer()
{
  int sockfd;
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  struct addrinfo *result = NULL;
  int errcode;
  char addrstr[100];
  void *ptr = NULL;
  struct sockaddr_in serv_addr;
  struct sockaddr_in6 serv_addr6;
  void *sockServAddr = NULL;
  int ai_family = AF_UNSPEC;
  int size = 0;

  /* ccpu00147898 fixes */
  memset(&hints, 0, sizeof(hints));
  memset((void*)&serv_addr, 0, sizeof(serv_addr));
  memset((void*)&serv_addr6, 0, sizeof(serv_addr6));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  errcode = getaddrinfo(cmdlinearg::g_ipAddr.c_str(), NULL, &hints, &res);
  if(errcode != 0)
  {
    perror ("getaddrinfo");
    return -1;
  }

  result = res;
  while(res)
  {
    inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 100);

    switch(res->ai_family)
    {
      case AF_INET:
        ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
        serv_addr.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
        sockServAddr = &serv_addr;
        ai_family = res->ai_family; 
        serv_addr.sin_family = res->ai_family;
        serv_addr.sin_port = htons(cmdlinearg::g_port);
        size =  sizeof(serv_addr);
        break;
      case AF_INET6:
        ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
        sockServAddr = &serv_addr6; 
        /* Copy IPv6 address(16bytes) into the destination */ 
        memcpy((unsigned char*)serv_addr6.sin6_addr.s6_addr, (unsigned char *)
         &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr, 16);
        ai_family = res->ai_family;
        serv_addr6.sin6_family = res->ai_family;
        serv_addr6.sin6_port = htons(cmdlinearg::g_port);
        size =  sizeof(serv_addr6);
        break;
      default:
        sockServAddr = NULL;
        break;
    }

    if(ptr != NULL)
    {
       inet_ntop(res->ai_family, ptr, addrstr, 100);
       printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
             addrstr, res->ai_canonname);
    }
    res = res->ai_next;
  }
 
  if(sockServAddr == NULL || size == 0)
  {
     fprintf(stderr, "Not able to parse server address\n");
     _exit(0);
  }

  if( (sockfd = socket(ai_family, SOCK_STREAM,0)) < 0 ) {
    fprintf(stderr, "Failed to create socket\n");
    _exit(0);
  }
  
  if( connect(sockfd, (const sockaddr*)sockServAddr, size) < 0 ) {
    perror("ERROR Connecting");
    _exit(0);
  }

  if(result != NULL)
  {
    freeaddrinfo(result);
  }
  return sockfd;
}


int main(int argc, char* argv[])
{
	readCmdLineArgs(argc, argv);
	loadLogDb();

	if( !cmdlinearg::g_sBinLogFile.empty() ) 
	{
		if( !cmdlinearg::g_sOutFile.empty() )
		{
			openLogFile(cmdlinearg::g_sOutFile.c_str());
			processLogs(openBinLogFile(), readFileLogs);
			fclose(g_fp);
		}
	}

	if( cmdlinearg::g_port != 0 ) {
		if( !cmdlinearg::g_sOutFile.empty() ) {
			openLogFile(cmdlinearg::g_sOutFile.c_str());
		}
		processLogs(connectToLogServer(), readRemoteLogs);
		fclose(g_fp);
	}
	return 0;
}

