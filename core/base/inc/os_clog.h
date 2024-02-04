/************************************************************************
 *File name: os_clog.h
 *Description: codetrace log interface
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_CLOG_H
#define OS_CLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_LOG_DOMAIN
#define OS_LOG_DOMAIN      1
#endif

extern FILE* g_fp;
extern int g_logLevel;
extern unsigned int g_modMask;
extern const char* g_logStr[MAX_LOG_LEVEL]; 
extern const char* g_splStr[DBG_MAX_IDs];

#define FMTSTR "[%d-%d-%d %d:%d:%d.%03d][%s]%s:%d\n%s:"
#define FMTSTR_S "[%d-%d-%d %d:%d:%d.%03d][%s]%s:%d\n%s:%s:%ld:"

typedef enum {
	NONE=0,
	FATAL=1,
	ERROR=2,
	WARN=3,
	INFO=4,
	DEBUG=5,
	TRACE=6,
	MAX_LOG_LEVEL,
} log_level_e;
typedef	log_level_e os_log_level_e;

typedef struct {
    char name[32];
} log_sp_info_t;

#define FOREACH_ID(ARGV_DEF) \
	ARGV_DEF(CELLID)  \
	ARGV_DEF(GNBID)   \
	ARGV_DEF(AMFID)   \
	ARGV_DEF(CRNTI)   \
	ARGV_DEF(UEID)    \
	ARGV_DEF(RBID)    \
	ARGV_DEF(LCID)    \
	ARGV_DEF(LCGID)   \\
	ARGV_DEF(ERRNOID) \
	ARGV_DEF(MAX_IDs)

#define ARGV_DEF(id)   {#id},

static const log_sp_info_t g_splStr[] = {
	FOREACH_ID(ARGV_DEF)
};

#define ARGV_ENUM(id) id,
typedef enum {
	FOREACH_ID(ARGV_ENUM)
} log_sp_arg_e;

/****************************CMLOG**********************************************/
#define CMLOGX(_level, _lstr, ...)                        CMLOG_ARG_N(0, _level, _lstr)

#define CMLOG_STR(_level, _lstr, _strarg)                 CMLOG_ARG_N(S, _level, _lstr, _strarg)
#define CMLOG_HEX(_level, _lstr, _hexdata, _hexlen)       CMLOG_ARG_N(H, _level, _lstr, _hexdata, _hexlen)
#define CMLOG0(_level, _lstr)                             CMLOG_ARG_N(0, _level, _lstr)
#define CMLOG1(_level, _lstr, _arg1)                      CMLOG_ARG_N(1, _level, _lstr, _arg1)
#define CMLOG2(_level, _lstr, _arg1, _arg2)               CMLOG_ARG_N(2, _level, _lstr, _arg1, _arg2)
#define CMLOG3(_level, _lstr, _arg1, _arg2, _arg3)        CMLOG_ARG_N(3, _level, _lstr, _arg1, _arg2, _arg3)
#define CMLOG4(_level, _lstr, _arg1, _arg2, _arg3, _arg4) CMLOG_ARG_N(4, _level, _lstr, _arg1, _arg2, _arg3, _arg4)
#define CMLOG_SP0(_level, _splenum, _splArg, _lstr)                             CMLOG_ARG_SP(_level, _splenum, _splArg, _lstr, 0, 0, 0, 0)
#define CMLOG_SP1(_level, _splenum, _splArg, _lstr, _arg1)                      CMLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, 0, 0, 0)
#define CMLOG_SP2(_level, _splenum, _splArg, _lstr, _arg1, _arg2)               CMLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, 0, 0)
#define CMLOG_SP3(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3)        CMLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, 0)
#define CMLOG_SP4(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4) CMLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4)

#define CMLOG_ARG_N(_N, _level, _fmtStr, ...) \
					do { \
						if( _level <= os_log_get_domain_level(OS_LOG_DOMAIN))\
						{ \
							cmlog##_N(_LOGID, _level, ##__VA_ARGS__, __FILE__,__LINE__, _fmtStr, RLOG_MODULE_NAME); \
						} \
					} while (0)

#define CMLOG_ARG_SP(_level, _splenum, _splArg, _fmtStr, ...) \
					do { \
						if( _level <= os_log_get_domain_level(OS_LOG_DOMAIN))\
						{ \
							cmlogSP(_LOGID,_level,_splenum, _splArg, ##__VA_ARGS__, __FILE__,__LINE__, _fmtStr, RLOG_MODULE_NAME); \
						} \
					} while (0)

typedef log_level_e os_cmlog_level_e;
void cmlog0(LOGID logId, os_cmlog_level_e logLevel, ...);
void cmlog1(LOGID logId, os_cmlog_level_e logLevel, unsigned int arg1, ...);
void cmlog2(LOGID logId, os_cmlog_level_e logLevel, unsigned int arg1, unsigned int arg2, ...);
void cmlog3(LOGID logId, os_cmlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, ...);
void cmlog4(LOGID logId, os_cmlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void cmlogSP(LOGID logId, os_cmlog_level_e logLevel, R_SPL_ARG splType, unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void cmlogH(LOGID logId, os_cmlog_level_e logLevel, const char* hex, int hexlen, ...);
void cmlogS(LOGID logId, os_cmlog_level_e logLevel, const char* str, ...);

/****************************CFLOG**********************************************/
#define CFLOGX(_level, _lstr, ...)                        CFLOG_ARG_X(_level, _lstr, ##__VA_ARGS__)

#define CFLOG_STR(_level, _lstr, _strarg)                 CFLOG_ARG_N(S, _level, _lstr, _strarg)
#define CFLOG_HEX(_level, _lstr, _hexdata, _hexlen)       CFLOG_ARG_N(H, _level, _lstr, _hexdata, _hexlen)
#define CFLOG0(_level, _lstr)                             CFLOG_ARG_N(0, _level, _lstr)
#define CFLOG1(_level, _lstr, _arg1)                      CFLOG_ARG_N(1, _level, _lstr, _arg1)
#define CFLOG2(_level, _lstr, _arg1, _arg2)               CFLOG_ARG_N(2, _level, _lstr, _arg1, _arg2)
#define CFLOG3(_level, _lstr, _arg1, _arg2, _arg3)        CFLOG_ARG_N(3, _level, _lstr, _arg1, _arg2, _arg3)
#define CFLOG4(_level, _lstr, _arg1, _arg2, _arg3, _arg4) CFLOG_ARG_N(4, _level, _lstr, _arg1, _arg2, _arg3, _arg4)
#define CFLOG_SP0(_level, _splenum, _splArg, _lstr)                             CFLOG_ARG_SP(_level, _splenum, _splArg, _lstr, 0, 0, 0, 0)
#define CFLOG_SP1(_level, _splenum, _splArg, _lstr, _arg1)                      CFLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, 0, 0, 0)
#define CFLOG_SP2(_level, _splenum, _splArg, _lstr, _arg1, _arg2)               CFLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, 0, 0)
#define CFLOG_SP3(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3)        CFLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, 0)
#define CFLOG_SP4(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4) CFLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4)

#define CFLOG_ARG_N(_N, _level, _fmtStr, ...) \
				do { \
					if( _level < = os_log_get_domain_level(OS_LOG_DOMAIN) )\
					{ \
						cflog##_N(g_logStr[_level],RLOG_MODULE_NAME, __FILE__,__LINE__, FMTSTR _fmtStr "\n\n", ##__VA_ARGS__); \
					} \
				} while (0)

#define CFLOG_ARG_X(_level, _fmtStr, ...) \
				do { \
					if( _level < = os_log_get_domain_level(OS_LOG_DOMAIN) )\
					{ \
						cflogN(_level,RLOG_MODULE_NAME, __FILE__,__LINE__, _fmtStr "\n\n", __VA_ARGS__); \
					} \
				} while (0)


#define CFLOG_ARG_SP(_level, _splenum, _splArg, _fmtStr, ...) \
				do { \
					if( _level < = os_log_get_domain_level(OS_LOG_DOMAIN))\
					{ \
						cflogSP(g_logStr[_level],RLOG_MODULE_NAME, __FILE__,__LINE__, FMTSTR_S _fmtStr "\n\n", _splenum,_splArg, ##__VA_ARGS__); \
					} \
				} while (0)

void cflog0(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, ...);
void cflog1(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, unsigned int arg1, ...);
void cflog2(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, unsigned int arg1, unsigned int arg2, ...);
void cflog3(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, unsigned int, unsigned int, unsigned int, ...);
void cflog4(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, unsigned int, unsigned int, unsigned int, unsigned int, ...);
void cflogN(int logLevel, const char* modName, const char* file, int lineno, const char* fmtStr, ...);
void cflogSP(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, R_SPL_ARG splType, unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void cflogH(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, const char* hexdump, int hexlen, ...);
void cflogS(const char* strLogLevel, const char* modName, const char* file, int lineno, const char* fmtStr, const char* str, ...);

/****************************CDLOG**********************************************/
#define cdlog_fatal(...)  cdlog_message(FATAL, 0, __VA_ARGS__)
#define cdlog_error(...)  cdlog_message(ERROR, 0, __VA_ARGS__)
#define cdlog_warn(...)   cdlog_message(WARN, 0, __VA_ARGS__)
#define cdlog_info(...)   cdlog_message(INFO, 0, __VA_ARGS__)
#define cdlog_debug(...)  cdlog_message(DEBUG, 0, __VA_ARGS__)
#define cdlog_trace(...)  cdlog_message(TRACE, 0, __VA_ARGS__)

#define cdlog_show(level, ...) cdlog_message(level, 0, __VA_ARGS__);
#define cdlog_show_sp(level, err_type, err, ...) cdlog_message(level, err, __VA_ARGS__);

#define cdlog_message(level, err, ...) \
    cdlog_printf(level, OS_LOG_DOMAIN, err, __FILE__, __LINE__, __OS_FUNC__, 0, __VA_ARGS__) 

#define cdlog_print(level, ...) \
    cdlog_printf(level, OS_LOG_DOMAIN, 0, NULL, 0, NULL, 1, __VA_ARGS__) 

#define cdlog_hexdump(level, _d, _l) \
    cdlog_hexdump_func(level, OS_LOG_DOMAIN, _d, _l)

typedef log_level_e cdlog_level_e;
typedef struct cdlog_s cdlog_t;

////////////////////////////////////////////////////////////////////////////////
#define os_assert(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            os_logs(ERROR, ">>>%s", OS_FUNC); \
            os_logs(ERROR, "Assertion `%s' failed<<<", #expr); \
            os_abort(); \
        } \
    } while(0)

#define os_assert_if_reached() \
    do { \
        os_logs(WARN, "%s: should not be reached.", OS_FUNC); \
        os_abort(); \
    } while(0)

#define os_expect(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            os_logs(ERROR, ">>>%s", OS_FUNC); \
            os_logs(ERROR, "Expectation `%s' failed<<<", #expr); \
        } \
    } while (0)

#define os_expect_or_return(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            os_logs(ERROR, ">>>%s", OS_FUNC); \
            os_logs(ERROR, "Expectation `%s' failed<<<", #expr); \
            return; \
        } \
    } while (0)

#define os_expect_or_return_val(expr, val) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            os_logs(ERROR, ">>>%s", OS_FUNC); \
            os_logs(ERROR, "Expectation `%s' failed<<<", #expr); \
            return (val); \
        } \
    } while (0)
////////////////////////////////////////////////////////////////////////////////
typedef struct os_log_domain_s os_log_domain_t;
void os_clog_domain_init(void);
void os_clog_domain_final(void);

os_log_domain_t *os_log_add_domain(const char *name, os_log_level_e level);
os_log_domain_t *os_log_find_domain(const char *name);
void os_log_remove_domain(os_log_domain_t *domain);
void os_log_set_domain_level(int id, os_log_level_e level);
os_log_level_e os_log_get_domain_level(int id);
const char *os_log_get_domain_name(int id);
int os_log_get_domain_id(const char *name);
void os_log_install_domain(int *domain_id, const char *name, os_log_level_e level);
int os_log_config_domain(const char *domain_mask, const char *level);
void os_log_set_mask_level(const char *mask, os_log_level_e level);
////////////////////////////////////////////////////////////////////////////////
void os_cdlog_init(void);
void os_cdlog_final(void);

cdlog_t *cdlog_add_stderr(void);
cdlog_t *cdlog_add_file(const char *name);
void cdlog_remove(cdlog_t *cdlog);

void cdlog_vprintf(cdlog_level_e level, int id, os_err_t err, const char *file, int line, const char *func, int content_only, const char *format, va_list ap);
void cdlog_printf(cdlog_level_e level, int domain_id, os_err_t err, char *file, int line, const char *func, int content_only, const char *format, ...)
	OS_GNUC_PRINTF(8, 9);
void cdlog_hexdump_func(cdlog_level_e level, int domain_id, const unsigned char *data, size_t len);

/////////////////////////////////////////////////////////////
void clSetLogPath(const char* logDir);
void clInitLog(unsigned char type); 
void clSetLogFile(const char* fileName);
void clSetLogPort(unsigned int port);
void clSetModuleMask(unsigned int modMask);
void clSetLogFileSizeLimit(unsigned int maxFileSize);
void clSetNumOfLogFiles(unsigned char nMaxFiles);
void clSetCircularBufferSize(unsigned int bufSize);
void clSetRemoteLoggingFlag(int flag);
int  clHandleConInput(char ch);
void clEnableDisableCore(int enable_core);
void clEnaBleBufferedIO(void);
void clUpdateRlogTti(void);
void clResetLogRateLmt(void);// This API reset the RLOG rate control count and enable logging every 10 ms 
void clStartLogCountLimit(void);//This API Start the limit the number of logs loggd into circular buffer every 10ms
void clStopLogCountLimit(void);// This API stops restriciton of limiting number of logs every 10 ms


#ifdef __cplusplus
}
#endif

#endif

