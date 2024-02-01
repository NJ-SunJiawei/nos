/************************************************************************
 *File name: os_tlog.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "os_init.h"
#include <libgen.h>
#define TA_NOR              "\033[0m"       /* all off */
#define TA_FGC_BLACK        "\033[30m"      /* Black */
#define TA_FGC_RED          "\033[31m"      /* Red */
#define TA_FGC_BOLD_RED     "\033[1;31m"    /* Bold Red */
#define TA_FGC_GREEN        "\033[32m"      /* Green */
#define TA_FGC_BOLD_GREEN   "\033[1;32m"    /* Bold Green */
#define TA_FGC_YELLOW       "\033[33m"      /* Yellow */
#define TA_FGC_BOLD_YELLOW  "\033[1;33m"    /* Bold Yellow */
#define TA_FGC_BOLD_BLUE    "\033[1;34m"    /* Bold Blue */
#define TA_FGC_BOLD_MAGENTA "\033[1;35m"    /* Bold Magenta */
#define TA_FGC_BOLD_CYAN    "\033[1;36m"    /* Bold Cyan */
#define TA_FGC_WHITE        "\033[37m"      /* White  */
#define TA_FGC_BOLD_WHITE   "\033[1;37m"    /* Bold White  */
#define TA_FGC_DEFAULT      "\033[39m"      /* default */

typedef enum {
    OS_LOG_STDERR_TYPE,
    OS_LOG_FILE_TYPE,
} os_tlog_type_e;

typedef struct os_tlog_s {
    os_lnode_t node;

    os_tlog_type_e type;

    union {
        struct {
            FILE *out;
            const char *name;
        } file;
    };

    struct {
    ED7(uint8_t color:1;,
        uint8_t timestamp:1;,
        uint8_t domain:1;,
        uint8_t level:1;,
        uint8_t fileline:1;,
        uint8_t function:1;,
        uint8_t linefeed:1;)
    } print;

    void (*writer)(os_tlog_t *tlog, os_tlog_level_e level, const char *string);

} os_tlog_t;

typedef struct os_tlog_domain_s {
    os_lnode_t node;

    int id;
    os_tlog_level_e level;
    const char *name;
} os_tlog_domain_t;

const char *level_strings[] = {
    NULL,
    "FATAL", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE",
};

PRIVATE OS_POOL(tlog_pool, os_tlog_t);
PRIVATE OS_LIST(tlog_list);

PRIVATE OS_POOL(domain_pool, os_tlog_domain_t);
PRIVATE OS_LIST(domain_list);

PRIVATE os_tlog_t *add_tlog(os_tlog_type_e type);
PRIVATE int file_cycle(os_tlog_t *tlog);

PRIVATE char *tlog_timestamp(char *buf, char *last,
        int use_color);
PRIVATE char *tlog_domain(char *buf, char *last,
        const char *name, int use_color);
PRIVATE char *tlog_content(char *buf, char *last,
        const char *format, va_list ap);
PRIVATE char *tlog_level(char *buf, char *last,
        os_tlog_level_e level, int use_color);
PRIVATE char *tlog_linefeed(char *buf, char *last);

PRIVATE void file_writer(
        os_tlog_t *tlog, os_tlog_level_e level, const char *string);

void os_tlog_init(void)
{
    os_pool_init(&tlog_pool, os_global_context()->tlog.pool);
    os_pool_init(&domain_pool, os_global_context()->tlog.domain_pool);

    os_tlog_add_domain("os", os_global_context()->tlog.level);
    os_tlog_add_stderr();
}

void os_tlog_final(void)
{
    os_tlog_t *tlog, *saved_tlog;
    os_tlog_domain_t *domain, *saved_domain;

    os_list_for_each_safe(&tlog_list, saved_tlog, tlog)
        os_tlog_remove(tlog);
    os_pool_final(&tlog_pool);

    os_list_for_each_safe(&domain_list, saved_domain, domain)
        os_tlog_remove_domain(domain);
    os_pool_final(&domain_pool);
}


os_tlog_t *os_tlog_add_stderr(void)
{
    os_tlog_t *tlog = NULL;
    
    tlog = add_tlog(OS_LOG_STDERR_TYPE);
    os_assert(tlog);

    tlog->file.out = stderr;
    tlog->writer = file_writer;

#if !defined(_WIN32)
    tlog->print.color = 1;
#endif

    return tlog;
}

os_tlog_t *os_tlog_add_file(const char *name)
{
    FILE *out = NULL;
    os_tlog_t *tlog = NULL;

    out = fopen(name, "a");
    if (!out) 
        return NULL;
    
    tlog = add_tlog(OS_LOG_FILE_TYPE);
    os_assert(tlog);

    tlog->file.name = name;
    tlog->file.out = out;

    tlog->writer = file_writer;

    return tlog;
}

void os_tlog_remove(os_tlog_t *tlog)
{
    os_assert(tlog);

    os_list_remove(&tlog_list, tlog);

    if (tlog->type == OS_LOG_FILE_TYPE) {
        os_assert(tlog->file.out);
        fclose(tlog->file.out);
        tlog->file.out = NULL;
    }

    os_pool_free(&tlog_pool, tlog);
}

os_tlog_domain_t *os_tlog_add_domain(const char *name, os_tlog_level_e level)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(name);

    os_pool_alloc(&domain_pool, &domain);
    os_assert(domain);

    domain->name = name;
    domain->id = os_pool_index(&domain_pool, domain);
    domain->level = level;

    os_list_add(&domain_list, domain);

    return domain;
}

os_tlog_domain_t *os_tlog_find_domain(const char *name)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(name);

    os_list_for_each(&domain_list, domain)
        if (!os_strcasecmp(domain->name, name))
            return domain;

    return NULL;
}

void os_tlog_remove_domain(os_tlog_domain_t *domain)
{
    os_assert(domain);

    os_list_remove(&domain_list, domain);
    os_pool_free(&domain_pool, domain);
}

void os_tlog_set_domain_level(int id, os_tlog_level_e level)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->tlog.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    domain->level = level;
}

os_tlog_level_e os_tlog_get_domain_level(int id)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->tlog.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->level;
}

const char *os_tlog_get_domain_name(int id)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->tlog.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->name;
}

int os_tlog_get_domain_id(const char *name)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(name);
    
    domain = os_tlog_find_domain(name);
    os_assert(domain);

    return domain->id;
}

void os_tlog_install_domain(int *domain_id,
        const char *name, os_tlog_level_e level)
{
    os_tlog_domain_t *domain = NULL;

    os_assert(domain_id);
    os_assert(name);

    domain = os_tlog_find_domain(name);
    if (domain) {
        OS_WARN("`%s` tlog-domain duplicated", name);
        if (level != domain->level) {
            OS_WARN("[%s]->[%s] tlog-level changed",
                    level_strings[domain->level], level_strings[level]);
            os_tlog_set_domain_level(domain->id, level);
        }
    } else {
        domain = os_tlog_add_domain(name, level);
        os_assert(domain);
    }

    *domain_id = domain->id;
}

void os_tlog_set_mask_level(const char *_mask, os_tlog_level_e level)
{
    os_tlog_domain_t *domain = NULL;

    if (_mask) {
        const char *delim = " \t\n,:";
        char *mask = NULL;
        char *saveptr;
        char *name;

        mask = os_strdup(_mask);
        os_assert(mask);

        for (name = os_strtok_r(mask, delim, &saveptr);
            name != NULL;
            name = os_strtok_r(NULL, delim, &saveptr)) {

            domain = os_tlog_find_domain(name);
            if (domain)
                domain->level = level;
        }

        os_free(mask);
    } else {
        os_list_for_each(&domain_list, domain)
            domain->level = level;
    }
}

PRIVATE os_tlog_level_e os_tlog_level_from_string(const char *string)
{
    os_tlog_level_e level = OS_ERROR;

    if (!strcasecmp(string, "none")) level = OS_TLOG_NONE;
    else if (!strcasecmp(string, "fatal")) level = OS_TLOG_FATAL;
    else if (!strcasecmp(string, "error")) level = OS_TLOG_ERROR;
    else if (!strcasecmp(string, "warn")) level = OS_TLOG_WARN;
    else if (!strcasecmp(string, "info")) level = OS_TLOG_INFO;
    else if (!strcasecmp(string, "debug")) level = OS_TLOG_DEBUG;
    else if (!strcasecmp(string, "trace")) level = OS_TLOG_TRACE;

    return level;
}

int os_tlog_config_domain(const char *domain, const char *level)
{
    if (domain || level) {
        int l = os_global_context()->tlog.level;

        if (level) {
            l = os_tlog_level_from_string(level);
            if (l == OS_ERROR) {
                OS_ERR("Invalid TLOG-LEVEL "
                        "[none:fatal|error|warn|info|debug|trace]: %s\n",
                        level);
                return OS_ERROR;
            }
        }

        os_tlog_set_mask_level(domain, l);
    }

    return OS_OK;
}

void os_tlog_vprintf(os_tlog_level_e level, int id,
    os_err_t err, const char *file, int line, const char *func,
    int content_only, const char *format, va_list ap)
{
    os_tlog_t *tlog = NULL;
    os_tlog_domain_t *domain = NULL;

    char tlogstr[OS_HUGE_LEN];
    char *p, *last;

    int wrote_stderr = 0;

    os_list_for_each(&tlog_list, tlog) {
        domain = os_pool_find(&domain_pool, id);
        if (!domain) {
            fprintf(stderr, "No tLogDomain[id:%d] in %s:%d", id, file, line);
            os_assert_if_reached();
        }
        if (domain->level < level)
            return;

        p = tlogstr;
        last = tlogstr + OS_HUGE_LEN;

        if (!content_only) {
            if (tlog->print.timestamp)
                p = tlog_timestamp(p, last, tlog->print.color);
            if (tlog->print.domain)
                p = tlog_domain(p, last, domain->name, tlog->print.color);
            if (tlog->print.fileline)
                p = os_slprintf(p, last, "[%s:%d] ", file, line);
            if (tlog->print.function)
                p = os_slprintf(p, last, "%s() ", func);
            if (tlog->print.level)
                p = tlog_level(p, last, level, tlog->print.color);
        }

        p = tlog_content(p, last, format, ap);

        if (err) {
            char errbuf[OS_HUGE_LEN];
            p = os_slprintf(p, last, " (%d:%s)",
                    (int)err, os_strerror(err, errbuf, OS_HUGE_LEN));
        }

        if (!content_only) {
            if (tlog->print.linefeed) 
                p = tlog_linefeed(p, last);
        }
		
        tlog->writer(tlog, level, tlogstr);
        
        if (tlog->type == OS_LOG_STDERR_TYPE)
            wrote_stderr = 1;
    }

    if (!wrote_stderr)
    {
        int use_color = 0;
#if !defined(_WIN32)
        use_color = 1;
#endif

        p = tlogstr;
        last = tlogstr + OS_HUGE_LEN;

        if (!content_only) {
            p = tlog_timestamp(p, last, use_color);
            p = tlog_level(p, last, level, use_color);
        }
        p = tlog_content(p, last, format, ap);
        if (!content_only) {
            p = os_slprintf(p, last, " (%s:%d)", file, line);
            p = os_slprintf(p, last, " %s()", func);
            p = tlog_linefeed(p, last);
        }

        fprintf(stderr, "%s", tlogstr);
        fflush(stderr);
    }
}

void os_tlog_printf(os_tlog_level_e level, int id,
    os_err_t err, char *file, int line, const char *func,
    int content_only, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    os_tlog_vprintf(level, id,
            err, basename(file), line, func, content_only, format, args);
    va_end(args);
}

void os_tlog_hexdump_func(os_tlog_level_e level, int id,
        const unsigned char *data, size_t len)
{
    size_t n, m;
    char dumpstr[OS_HUGE_LEN];
    char *p, *last;

    last = dumpstr + OS_HUGE_LEN;
    p = dumpstr;

    for (n = 0; n < len; n += 16) {
        p = os_slprintf(p, last, "%04x: ", (int)n);
        
        for (m = n; m < n + 16; m++) {
            if (m > n && (m % 4) == 0)
                p = os_slprintf(p, last, " ");
            if (m < len)
                p = os_slprintf(p, last, "%02x", data[m]);
            else
                p = os_slprintf(p, last, "  ");
        }

        p = os_slprintf(p, last, "   ");

        for (m = n; m < len && m < n + 16; m++)
            p = os_slprintf(p, last, "%c", isprint(data[m]) ? data[m] : '.');

        p = os_slprintf(p, last, "\n");
    }

    os_tlog_print(level, "%s", dumpstr);
}

PRIVATE os_tlog_t *add_tlog(os_tlog_type_e type)
{
    os_tlog_t *tlog = NULL;

    os_pool_alloc(&tlog_pool, &tlog);
    os_assert(tlog);
    memset(tlog, 0, sizeof *tlog);

    tlog->type = type;

    tlog->print.timestamp = 1;
    tlog->print.domain = 1;
    tlog->print.level = 1;
    tlog->print.fileline = 1;
    tlog->print.linefeed = 1;

    os_list_add(&tlog_list, tlog);

    return tlog;
}

PRIVATE int file_cycle(os_tlog_t *tlog)
{
    os_assert(tlog);
    os_assert(tlog->file.out);
    os_assert(tlog->file.name);

    fclose(tlog->file.out);
    tlog->file.out = fopen(tlog->file.name, "a");
    os_assert(tlog->file.out);

    return 0;
}

PRIVATE char *tlog_timestamp(char *buf, char *last,
        int use_color)
{
    struct timeval tv;
    struct tm tm;
    char nowstr[32];

    os_gettimeofday(&tv);
    os_localtime(tv.tv_sec, &tm);
    strftime(nowstr, sizeof nowstr, "%m/%d %H:%M:%S", &tm);

    buf = os_slprintf(buf, last, "%s%s.%03d%s: ",
            use_color ? TA_FGC_GREEN : "",
            nowstr, (int)(tv.tv_usec/1000),
            use_color ? TA_NOR : "");

    return buf;
}

PRIVATE char *tlog_domain(char *buf, char *last,
        const char *name, int use_color)
{
    buf = os_slprintf(buf, last, "[%s%s%s] ",
            use_color ? TA_FGC_YELLOW : "",
            name,
            use_color ? TA_NOR : "");

    return buf;
}

PRIVATE char *tlog_level(char *buf, char *last,
        os_tlog_level_e level, int use_color)
{
    const char *colors[] = {
        TA_NOR,
        TA_FGC_BOLD_RED, TA_FGC_BOLD_YELLOW, TA_FGC_BOLD_CYAN,
        TA_FGC_BOLD_GREEN, TA_FGC_BOLD_WHITE, TA_FGC_WHITE,
    };

    buf = os_slprintf(buf, last, "%s%s%s: ",
            use_color ? colors[level] : "",
            level_strings[level],
            use_color ? TA_NOR : "");

    return buf;
}

static char *tlog_content(char *buf, char *last,
        const char *format, va_list ap)
{
    va_list bp;

    va_copy(bp, ap);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    buf = os_vslprintf(buf, last, format, bp);
#pragma GCC diagnostic pop
    va_end(bp);

    return buf;
}

PRIVATE char *tlog_linefeed(char *buf, char *last)
{
#if defined(_WIN32)
    if (buf > last - 3)
        buf = last - 3;

    buf = os_slprintf(buf, last, "\r\n");
#else
    if (buf > last - 2)
        buf = last - 2;

    buf = os_slprintf(buf, last, "\n");
#endif

    return buf;
}

PRIVATE void file_writer(
        os_tlog_t *tlog, os_tlog_level_e level, const char *string)
{
    fprintf(tlog->file.out, "%s", string);
    fflush(tlog->file.out);
}

