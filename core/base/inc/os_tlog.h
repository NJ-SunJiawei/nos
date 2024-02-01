/************************************************************************
 *File name: os_tlog.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_TLOG_H
#define OS_TLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_LOG_DOMAIN
#define OS_LOG_DOMAIN      1
#endif

#define os_fatal(...)  os_tlog_message(OS_TLOG_FATAL, 0, __VA_ARGS__)
#define os_error(...)  os_tlog_message(OS_TLOG_ERROR, 0, __VA_ARGS__)
#define os_warn(...)   os_tlog_message(OS_TLOG_WARN, 0, __VA_ARGS__)
#define os_info(...)   os_tlog_message(OS_TLOG_INFO, 0, __VA_ARGS__)
#define os_debug(...)  os_tlog_message(OS_TLOG_DEBUG, 0, __VA_ARGS__)
#define os_trace(...)  os_tlog_message(OS_TLOG_TRACE, 0, __VA_ARGS__)
////////////////////////////////////////////////////////////////////////////////
#define os_tlog_message(level, err, ...) \
    os_tlog_printf(level, OS_LOG_DOMAIN, \
    err, __FILE__, __LINE__, __OS_FUNC__,  \
    0, __VA_ARGS__) 

#define os_tlog_print(level, ...) \
    os_tlog_printf(level, OS_LOG_DOMAIN, \
    0, NULL, 0, NULL,  \
    1, __VA_ARGS__) 

#define os_tlog_hexdump(level, _d, _l) \
    os_tlog_hexdump_func(level, OS_LOG_DOMAIN, _d, _l)

typedef enum {
    OS_TLOG_NONE = 0,
    OS_TLOG_FATAL,
    OS_TLOG_ERROR,
    OS_TLOG_WARN,
    OS_TLOG_INFO,
    OS_TLOG_DEBUG,
    OS_TLOG_TRACE,
    OS_TLOG_DEFAULT = OS_TLOG_INFO,
    OS_TLOG_FULL = OS_TLOG_TRACE,
} os_tlog_level_e;

typedef struct os_tlog_s os_tlog_t;
typedef struct os_tlog_domain_s os_tlog_domain_t;

void os_tlog_init(void);
void os_tlog_final(void);

os_tlog_t *os_tlog_add_stderr(void);
os_tlog_t *os_tlog_add_file(const char *name);
void os_tlog_remove(os_tlog_t *tlog);

os_tlog_domain_t *os_tlog_add_domain(const char *name, os_tlog_level_e level);
os_tlog_domain_t *os_tlog_find_domain(const char *name);
void os_tlog_remove_domain(os_tlog_domain_t *domain);

void os_tlog_set_domain_level(int id, os_tlog_level_e level);
os_tlog_level_e os_tlog_get_domain_level(int id);

const char *os_tlog_get_domain_name(int id);
int os_tlog_get_domain_id(const char *name);

void os_tlog_install_domain(int *domain_id, const char *name, os_tlog_level_e level);
int os_tlog_config_domain(const char *domain, const char *level);

void os_tlog_set_mask_level(const char *mask, os_tlog_level_e level);

void os_tlog_vprintf(os_tlog_level_e level, int id,
    os_err_t err, const char *file, int line, const char *func,
    int content_only, const char *format, va_list ap);
void os_tlog_printf(os_tlog_level_e level, int domain_id,
    os_err_t err, char *file, int line, const char *func,
    int content_only, const char *format, ...)
    OS_GNUC_PRINTF(8, 9);

void os_tlog_hexdump_func(os_tlog_level_e level, int domain_id, const unsigned char *data, size_t len);

#define os_assert(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            OS_FATAL("%s: Assertion `%s' failed.", OS_FUNC, #expr); \
            os_abort(); \
        } \
    } while(0)

#define os_assert_if_reached() \
    do { \
        OS_WARN("%s: should not be reached.", OS_FUNC); \
        os_abort(); \
    } while(0)

#define os_expect(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            OS_ERR("%s: Expectation `%s' failed.", OS_FUNC, #expr); \
        } \
    } while (0)

#define os_expect_or_return(expr) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            OS_ERR("%s: Expectation `%s' failed.", OS_FUNC, #expr); \
            return; \
        } \
    } while (0)

#define os_expect_or_return_val(expr, val) \
    do { \
        if (os_likely(expr)) ; \
        else { \
            OS_ERR("%s: Expectation `%s' failed.", OS_FUNC, #expr); \
            return (val); \
        } \
    } while (0)

#ifdef __cplusplus
		}
#endif

#endif
