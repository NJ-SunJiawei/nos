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


#if 1
#ifndef CMLOG_MODULE_ID
#define CMLOG_MODULE_ID     0x00000001
#endif
#ifndef CMLOG_MODULE_NAME
#define CMLOG_MODULE_NAME   "os"
#endif
#else //1
#define DECLARE_CMLOG_MODULE(_module_name_, _file_id_, _module_id_) \
    	PRIVATE const char* CMLOG_MODULE_NAME __attribute__((unused)) = (_module_name_); \
    	PRIVATE int CMLOG_MODULE_ID __attribute__((unused)) = (_module_id_);
#endif

#define FMTSTR_T   "[%s] [%s] %s:%d %s:\n"
#define FMTSTR_M   "[%u] [%s] %s:%d %s:\n"

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

//////////////////////////////////////
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

PRIVATE const log_sp_info_t g_splStr[] = {
	FOREACH_ID(ARGV_DEF)
};

#define ARGV_ENUM(id) id,
typedef enum {
	FOREACH_ID(ARGV_ENUM)
} log_sp_arg_e;
////////////////////////////////////

typedef	log_sp_arg_e os_clog_sp_arg_e;
typedef log_level_e os_clog_level_e;

typedef unsigned int LOGID;

extern int g_logLevel; 
extern unsigned int g_modMask; 

/****************************CTLOG**********************************************/
#define RLOGX(_level, _splenum, _splArg, _lstr, ...) \
do { \
    if (_splenum == PLACEHOLDER)\
    { \
        CMLOG_ARG_X(_level, _lstr, ##__VA_ARGS__);\
    } \
    else \
    { \
        CMLOG_ARG_SPX(_level, _splenum, _splArg, _lstr, ##__VA_ARGS__);\
    } \
} while(0);

#define RLOGX_CFG(_level, _splenum, _splArg, _lstr, ...)  RLOGX(_level, _splenum, _splArg, _lstr, ##__VA_ARGS__)
/****************************CMLOG**********************************************/
#define CMPRINT(_level, _lstr, ...)                       CMLOG_ARG_X(1, CMLOG_MODULE_NAME, _level, _lstr, ##__VA_ARGS__)
#define CMLOGX(_level, _lstr, ...)                        CMLOG_ARG_X(0, CMLOG_MODULE_NAME, _level, _lstr, ##__VA_ARGS__)
#define CMLOGSPX(_level, _splenum, _splArg, _lstr, ...)   CMLOG_ARG_SPX(CMLOG_MODULE_NAME, _level, _splenum, _splArg, _lstr, ##__VA_ARGS__)
#define CMLOGH(_level, _lstr, _hexdata, _hexlen)          CMLOG_ARG_H(CMLOG_MODULE_NAME, _level, _lstr, _hexdata, _hexlen)

#ifdef CMLOG_ALLOW_CLOCK_TIME
#define CMLOG_ARG_H(_modName, _level, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel && g_modMask & CMLOG_MODULE_ID)\
					{ \
						cmlogH(_level, _modName, __FILE__, __OS_FUNC__, __LINE__, FMTSTR_T _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)
#else
#define CMLOG_ARG_H(_modName, _level, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel && g_modMask & CMLOG_MODULE_ID)\
					{ \
						cmlogH(_level, _modName, __FILE__, __OS_FUNC__, __LINE__, FMTSTR_M _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)
#endif

#define CMLOG_ARG_X(_contentFg, _modName, _level, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel && g_modMask & CMLOG_MODULE_ID)\
					{ \
						cmlogN(_contentFg, _level, _modName, __FILE__, __OS_FUNC__, __LINE__, _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)

#define CMLOG_ARG_SPX(_modName, _level, _splenum, _splArg, _fmtStr, ...) \
				do { \
					if( _level <= g_logLevel && g_modMask & CMLOG_MODULE_ID)\
					{ \
						cmlogSPN(_level, _modName, __FILE__, __OS_FUNC__,__LINE__, _splenum, _splArg, _fmtStr "\n", ##__VA_ARGS__); \
					} \
				} while (0)

void cmlogN(int content_only, int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, ...);
void cmlogSPN(int logLevel, const char* modName, char* file, const char* func, int line, log_sp_arg_e splType, unsigned int splVal, const char* fmtStr, ...);
void cmlogH(int logLevel, const char* modName, char* file, const char* func, int line, const char* fmtStr, const unsigned char* hexdump, int hexlen, ...);

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
            os_log(FATAL, "%s: Assertion `%s' failed.", OS_FUNC, #expr); \
            os_abort(); \
        } \
    } while(0)

#define os_assert_if_reached() \
    do { \
        os_log(FATAL, "%s: should not be reached.", OS_FUNC); \
        os_abort(); \
    } while(0)

#define os_expect(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
			os_log(ERROR, "%s: Expectation `%s' failed.", OS_FUNC, #expr); \
        } \
    } while (0)

#define os_expect_or_return(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
			os_log(ERROR, "%s: Expectation `%s' failed.", OS_FUNC, #expr); \
            return; \
        } \
    } while (0)

#define os_expect_or_return_val(expr, val) \
    do { \
        if (os_likely(expr)) ; \
        else { \
			os_log(ERROR, "%s: Expectation `%s' failed.", OS_FUNC, #expr); \
            return (val); \
        } \
    } while (0)

////////////////////////////cdlog//////////////////////////////////////////////////
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

//////////////////////////cmlog//////////////////////////////////////////////////////
void os_cmlog_set_filesize_limit(unsigned int maxFileSize);
void os_cmlog_set_filenum(unsigned char maxFiles);
void os_cmlog_set_log_path(const char* logDir);
void os_cmlog_set_cirbuf_dep(unsigned int bufNum);
void os_cmlog_printf_config(void);
void os_cmlog_set_filename(const char* fileName);
void os_cmlog_set_log_level(os_clog_level_e logLevel);
void os_cmlog_set_module_mask(unsigned int modMask);
void os_cmlog_init(void);
void os_cmlog_final(void);
void os_cmlog_update_ticks(void);
void os_cmlog_reset_rate_limit(void);
void os_cmlog_stop_count_limit(void);
void os_cmlog_start_count_limit(void);


#ifdef __cplusplus
}
#endif

#endif

