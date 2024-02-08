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

#ifndef CDLOG_DOMAIN
#define CDLOG_DOMAIN      1
#endif

#ifndef CTLOG_MODULE_ID
#define CTLOG_MODULE_ID     0x00000001
#endif

#ifndef CTLOG_MODULE_NAME
#define CTLOG_MODULE_NAME   "os"
#endif

typedef enum {
	NONE=0,
	FATAL=1,
	ERROR=2,
	WARN=3,
	INFO=4,
	DEBUG=5,
	TRACE=6,
	MAX_LOG_LEVEL
} log_level_e;

typedef struct {
    char name[32];
} log_sp_info_t;

#define FOREACH_ID(ARGV_DEF) \
	ARGV_DEF(DBG_CELLID)   \
	ARGV_DEF(DBG_PEERID)   \
	ARGV_DEF(DBG_ENBID)    \
	ARGV_DEF(DBG_MMEID)    \
	ARGV_DEF(DBG_CRNTI)    \
	ARGV_DEF(DBG_UEIDX)    \
	ARGV_DEF(DBG_UEID)     \
	ARGV_DEF(DBG_RBID)     \
	ARGV_DEF(DBG_LCID)     \
	ARGV_DEF(DBG_LCGID)    \
	ARGV_DEF(DBG_TRNSID)   \
	ARGV_DEF(DBG_INSTID)   \
	ARGV_DEF(PLACEHOLDER)  \
	ARGV_DEF(ERRNOID)      \
	ARGV_DEF(MAX_IDs)

#define ARGV_DEF(id)   {#id},

static const log_sp_info_t g_splStr[] = {
	FOREACH_ID(ARGV_DEF)
};

#define ARGV_ENUM(id) id,
typedef enum {
	FOREACH_ID(ARGV_ENUM)
} log_sp_arg_e;

typedef enum
{
	TIME_REFERENCE=0,
	TIME_DELIMITER,
	OS_SIGSEGV,
}LOGID_TYPE;

typedef	log_sp_arg_e os_ctlog_sp_arg_e;

typedef unsigned int LOGID;
extern int g_logLevel;
extern unsigned int g_modMask;

#define REGISTER_CLOG_MODULE(_module_name_, _module_id_) \
		#undef CTLOG_MODULE_ID \
		#define CTLOG_MODULE_ID   (_module_id_)\
		#undef CTLOG_MODULE_NAME \
		#define CTLOG_MODULE_NAME   (_module_name_)


#ifdef CLOG_ENABLE_TEXT_LOGGING
/****************************CFLOG**********************************************/
#define FMTSTR   "[%0.4d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d.%0.6d][%s]%s:%s:%d %s:"
#define FMTSTR_S "[%0.4d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d.%0.6d][%s]%s:%s:%d %s:%s:%ld:"

#define RLOGX(_level, _splenum, _splArg, _lstr, ...) \
do { \
    if (_splenum == PLACEHOLDER)\
    { \
        CTLOG_ARG_X(_level, _lstr, ##__VA_ARGS__);\
    } \
    else \
    { \
        CTLOG_ARG_SPX(_level, _splenum, _splArg, _lstr, ##__VA_ARGS__);\
    } \
} while(0);

#define RLOGX_CFG(_level, _splenum, _splArg, _lstr, ...) RLOGX(_level, _splenum, _splArg, _lstr, ##__VA_ARGS__);


#define CTLOGX(_level, _lstr, ...)                        CTLOG_ARG_X(_level, _lstr, ##__VA_ARGS__)
#define CTLOGSPX(_level, _lstr, ...)                      CTLOG_ARG_SPX(_level, _lstr, ##__VA_ARGS__)

#define CTLOG_STR(_level, _lstr, _strarg)                 CTLOG_ARG_N(S, _level, _lstr, _strarg)
#define CTLOG_HEX(_level, _lstr, _hexdata, _hexlen)       CTLOG_ARG_N(H, _level, _lstr, _hexdata, _hexlen)
#define CTLOG0(_level, _lstr)                             CTLOG_ARG_N(0, _level, _lstr)
#define CTLOG1(_level, _lstr, _arg1)                      CTLOG_ARG_N(1, _level, _lstr, _arg1)
#define CTLOG2(_level, _lstr, _arg1, _arg2)               CTLOG_ARG_N(2, _level, _lstr, _arg1, _arg2)
#define CTLOG3(_level, _lstr, _arg1, _arg2, _arg3)        CTLOG_ARG_N(3, _level, _lstr, _arg1, _arg2, _arg3)
#define CTLOG4(_level, _lstr, _arg1, _arg2, _arg3, _arg4) CTLOG_ARG_N(4, _level, _lstr, _arg1, _arg2, _arg3, _arg4)
#define CTLOG_SP0(_level, _splenum, _splArg, _lstr)                             CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, 0, 0, 0, 0)
#define CTLOG_SP1(_level, _splenum, _splArg, _lstr, _arg1)                      CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, 0, 0, 0)
#define CTLOG_SP2(_level, _splenum, _splArg, _lstr, _arg1, _arg2)               CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, 0, 0)
#define CTLOG_SP3(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3)        CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, 0)
#define CTLOG_SP4(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4) CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4)

#define CTLOG_ARG_N(_N, _level, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel || g_modMask & CTLOG_MODULE_ID)\
					{ \
						ctlog##_N(g_logStr[_level], CTLOG_MODULE_NAME, __FILE__, __OS_FUNC__, __LINE__, FMTSTR _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)

#define CTLOG_ARG_X(_level, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel || g_modMask & CTLOG_MODULE_ID)\
					{ \
						ctlogN(_level, CTLOG_MODULE_NAME, __FILE__, __OS_FUNC__, __LINE__, _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)


#define CTLOG_ARG_SP(_level, _splenum, _splArg, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel || g_modMask & CTLOG_MODULE_ID)\
					{ \
						ctlogSP(g_logStr[_level], CTLOG_MODULE_NAME, __FILE__, __OS_FUNC__,__LINE__, FMTSTR_S _fmtStr "\n", _splenum,_splArg, ##__VA_ARGS__); \
					} \
				} while (0)


#define CTLOG_ARG_SPX(_level, _splenum, _splArg, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel || g_modMask & CTLOG_MODULE_ID)\
					{ \
						ctlogSPN(_level, CTLOG_MODULE_NAME, __FILE__, __OS_FUNC__,__LINE__, _splenum, _splArg, _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)

void ctlog0(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, ...);
void ctlog1(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int arg1, ...);
void ctlog2(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int arg1, unsigned int arg2, ...);
void ctlog3(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int, unsigned int, unsigned int, ...);
void ctlog4(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, unsigned int, unsigned int, unsigned int, unsigned int, ...);
void ctlogSP(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, os_ctlog_sp_arg_e splType, unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void ctlogH(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, const char* hexdump, int hexlen, ...);
void ctlogS(const char* strLogLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, const char* str, ...);

void ctlogN(int logLevel, const char* modName, const char* file, const char* func, int lineno, const char* fmtStr, ...);
void ctlogSPN(int logLevel, const char* modName, const char* file, const char* func, int lineno, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...);

#else
/****************************CMLOG**********************************************/

#define CTLOG_STR(_level, _lstr, _strarg)                 CTLOG_ARG_N(S, _level, _lstr, _strarg)
#define CTLOG_HEX(_level, _lstr, _hexdata, _hexlen)       CTLOG_ARG_N(H, _level, _lstr, _hexdata, _hexlen)
#define CTLOG0(_level, _lstr)                             CTLOG_ARG_N(0, _level, _lstr)
#define CTLOG1(_level, _lstr, _arg1)                      CTLOG_ARG_N(1, _level, _lstr, _arg1)
#define CTLOG2(_level, _lstr, _arg1, _arg2)               CTLOG_ARG_N(2, _level, _lstr, _arg1, _arg2)
#define CTLOG3(_level, _lstr, _arg1, _arg2, _arg3)        CTLOG_ARG_N(3, _level, _lstr, _arg1, _arg2, _arg3)
#define CTLOG4(_level, _lstr, _arg1, _arg2, _arg3, _arg4) CTLOG_ARG_N(4, _level, _lstr, _arg1, _arg2, _arg3, _arg4)
#define CTLOG_SP0(_level, _splenum, _splArg, _lstr)                             CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, 0, 0, 0, 0)
#define CTLOG_SP1(_level, _splenum, _splArg, _lstr, _arg1)                      CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, 0, 0, 0)
#define CTLOG_SP2(_level, _splenum, _splArg, _lstr, _arg1, _arg2)               CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, 0, 0)
#define CTLOG_SP3(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3)        CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, 0)
#define CTLOG_SP4(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4) CTLOG_ARG_SP(_level, _splenum, _splArg, _lstr, _arg1, _arg2, _arg3, _arg4)

#define CTLOG_ARG_N(_N, _level, _fmtStr, ...) \
					do { \
						if( _level <= g_logLevel || g_modMask & CTLOG_MODULE_ID)\
						{ \
							ctlog##_N(_LOGID, _level, ##__VA_ARGS__, __FILE__,__LINE__, _fmtStr, CLOG_MODULE_NAME); \
						} \
					} while (0)

#define CTLOG_ARG_SP(_level, _splenum, _splArg, _fmtStr, ...) \
					do { \
						if(_level <= g_logLevel || g_modMask & CTLOG_MODULE_ID))\
						{ \
							ctlogSP(_LOGID,_level,_splenum, _splArg, ##__VA_ARGS__, __FILE__,__LINE__, _fmtStr, CLOG_MODULE_NAME); \
						} \
					} while (0)
s
typedef log_level_e os_ctlog_level_e;
void ctlog0(LOGID logId, os_ctlog_level_e logLevel, ...);
void ctlog1(LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, ...);
void ctlog2(LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, ...);
void ctlog3(LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, ...);
void ctlog4(LOGID logId, os_ctlog_level_e logLevel, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void ctlogSP(LOGID logId, os_ctlog_level_e logLevel, os_ctlog_sp_arg_e splType, unsigned int splVal, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, ...);
void ctlogH(LOGID logId, os_ctlog_level_e logLevel, const char* hex, int hexlen, ...);
void ctlogS(LOGID logId, os_ctlog_level_e logLevel, const char* str, ...);

#endif
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
    cdlog_printf(level, CDLOG_DOMAIN, err, __FILE__, __LINE__, __OS_FUNC__, 0, __VA_ARGS__) 

#define cdlog_print(level, ...) \
    cdlog_printf(level, CDLOG_DOMAIN, 0, NULL, 0, NULL, 1, __VA_ARGS__) 

#define cdlog_hexdump(level, _d, _l) \
    cdlog_hexdump_func(level, CDLOG_DOMAIN, _d, _l)

typedef log_level_e os_cdlog_level_e;
typedef struct cdlog_s os_cdlog_t;

void cdlog_remove(os_cdlog_t *cdlog);
void cdlog_vprintf(os_cdlog_level_e level, int id, os_err_t err, const char *file, int line, const char *func, int content_only, const char *format, va_list ap);
void cdlog_printf(os_cdlog_level_e level, int domain_id, os_err_t err, char *file, int line, const char *func, int content_only, const char *format, ...) OS_GNUC_PRINTF(8, 9);
void cdlog_hexdump_func(os_cdlog_level_e level, int domain_id, const unsigned char *data, size_t len);

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
typedef struct os_cdlog_domain_s os_cdlog_domain_t;
os_cdlog_domain_t *os_cdlog_add_domain(const char *name, os_cdlog_level_e level);
os_cdlog_domain_t *os_cdlog_find_domain(const char *name);
void os_cdlog_remove_domain(os_cdlog_domain_t *domain);
void os_cdlog_set_domain_level(int id, os_cdlog_level_e level);
os_cdlog_level_e os_cdlog_get_domain_level(int id);
const char *os_cdlog_get_domain_name(int id);
int os_cdlog_get_domain_id(const char *name);
void os_cdlog_install_domain(int *domain_id, const char *name, os_cdlog_level_e level);
int os_cdlog_config_domain(const char *domain_mask, const char *level);
void os_cdlog_set_mask_level(const char *mask, os_cdlog_level_e level);
void os_cdlog_set_file_size(unsigned int max_limit_size);
void os_cdlog_set_file_num(unsigned char max_file_num);
void os_cdlog_set_log_path(const char* log_dir);
void os_cdlog_set_file_name(const char* file_name);
void os_cdlog_init(void);
void os_cdlog_final(void);
os_cdlog_t *os_cdlog_add_stderr(void);
os_cdlog_t *os_cdlog_add_file(void);
void os_cdlog_cycle(int sig);
void os_cdlog_enable_coredump(bool enable_core);

////////////////////////////////////////////////////////////////////////////////

void os_ctlog_set_fileSize_limit(unsigned int maxFileSize);
void os_ctlog_set_fileNum(unsigned char maxFiles);
void os_ctlog_set_remote_flag(int flag);
void os_ctlog_set_log_port(unsigned int port);
void os_ctlog_set_log_path(const char* logDir);
void os_ctlog_set_circular_bufferSize(unsigned int bufSize);
void os_ctlog_printf_config(void);
void os_ctlog_enable_coredump(bool enable_core);
void os_ctlog_set_fileName(const char* fileName);
void os_ctlog_set_log_level(os_ctlog_level_e logLevel);
void os_ctlog_set_module_mask(unsigned int modMask);
void os_ctlog_init(void);
void os_ctlog_start_count_limit(void);
void os_ctlog_stop_count_limit(void);
void os_ctlog_update_ticks(void);//10ms update

#ifdef __cplusplus
}
#endif

#endif

