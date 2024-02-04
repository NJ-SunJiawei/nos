/************************************************************************
 *File name: cdlog.c
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
} cdlog_type_e;

typedef struct cdlog_s {
    os_lnode_t node;

    cdlog_type_e type;

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

    void (*writer)(cdlog_t *cdlog, cdlog_level_e level, const char *string);

} cdlog_t;


PRIVATE OS_POOL(cdlog_pool, cdlog_t);
PRIVATE OS_LIST(cdlog_list);

PRIVATE cdlog_t *add_cdlog(cdlog_type_e type);
PRIVATE char *cdlog_timestamp(char *buf, char *last, int use_color);
PRIVATE char *cdlog_domain(char *buf, char *last, const char *name, int use_color);
PRIVATE char *cdlog_content(char *buf, char *last, const char *format, va_list ap);
PRIVATE char *cdlog_level(char *buf, char *last, cdlog_level_e level, int use_color);
PRIVATE char *cdlog_linefeed(char *buf, char *last);
PRIVATE void file_writer(cdlog_t *cdlog, cdlog_level_e level, const char *string);


/////////////////////////////////////////////////////////////
_API_ void os_cdlog_init(void)
{
    os_pool_init(&cdlog_pool, os_global_context()->log.pool);

    os_log_add_domain("os", os_global_context()->log.level);
    cdlog_add_stderr();
}

_API_ void os_cdlog_final(void)
{
    cdlog_t *cdlog, *saved_cdlog;

    os_list_for_each_safe(&cdlog_list, saved_cdlog, cdlog)
        cdlog_remove(cdlog);
    os_pool_final(&cdlog_pool);
}

//////////////////////////////////////////////////////////////////////////////////

cdlog_t *cdlog_add_stderr(void)
{
    cdlog_t *cdlog = NULL;
    
    cdlog = add_cdlog(OS_LOG_STDERR_TYPE);
    os_assert(cdlog);

    cdlog->file.out = stderr;
    cdlog->writer = file_writer;

#if !defined(_WIN32)
    cdlog->print.color = 1;
#endif

    return cdlog;
}

cdlog_t *cdlog_add_file(const char *name)
{
    FILE *out = NULL;
    cdlog_t *cdlog = NULL;

    out = fopen(name, "a");
    if (!out) 
        return NULL;
    
    cdlog = add_cdlog(OS_LOG_FILE_TYPE);
    os_assert(cdlog);

    cdlog->file.name = name;
    cdlog->file.out = out;

    cdlog->writer = file_writer;

    return cdlog;
}

void cdlog_remove(cdlog_t *cdlog)
{
    os_assert(cdlog);

    os_list_remove(&cdlog_list, cdlog);

    if (cdlog->type == OS_LOG_FILE_TYPE) {
        os_assert(cdlog->file.out);
        fclose(cdlog->file.out);
        cdlog->file.out = NULL;
    }

    os_pool_free(&cdlog_pool, cdlog);
}


void cdlog_vprintf(cdlog_level_e level, int id, os_err_t err, const char *file, int line, const char *func, int content_only, const char *format, va_list ap)
{
    cdlog_t *cdlog = NULL;
    os_log_domain_t *domain = NULL;

    char cdlogstr[OS_HUGE_LEN];
    char *p, *last;

    int wrote_stderr = 0;

    os_list_for_each(&cdlog_list, cdlog) {
        domain = os_pool_find(&domain_pool, id);
        if (!domain) {
            fprintf(stderr, "No tLogDomain[id:%d] in %s:%d", id, file, line);
            os_assert_if_reached();
        }
        if (level > domain->level)
            return;

        p = cdlogstr;
        last = cdlogstr + OS_HUGE_LEN;

        if (!content_only) {
            if (cdlog->print.timestamp)
                p = cdlog_timestamp(p, last, cdlog->print.color);
            if (cdlog->print.domain)
                p = cdlog_domain(p, last, domain->name, cdlog->print.color);
            if (cdlog->print.fileline)
                p = os_slprintf(p, last, "[%s:%d] ", file, line);
            if (cdlog->print.function)
                p = os_slprintf(p, last, "%s() ", func);
            if (cdlog->print.level)
                p = cdlog_level(p, last, level, cdlog->print.color);
        }

        p = cdlog_content(p, last, format, ap);

        if (err) {
            char errbuf[OS_HUGE_LEN];
            p = os_slprintf(p, last, " (%d:%s)",
                    (int)err, os_strerror(err, errbuf, OS_HUGE_LEN));
        }

        if (!content_only) {
            if (cdlog->print.linefeed) 
                p = cdlog_linefeed(p, last);
        }
		
        cdlog->writer(cdlog, level, cdlogstr);
        
        if (cdlog->type == OS_LOG_STDERR_TYPE)
            wrote_stderr = 1;
    }

    if (!wrote_stderr)
    {
        int use_color = 0;
#if !defined(_WIN32)
        use_color = 1;
#endif

        p = cdlogstr;
        last = cdlogstr + OS_HUGE_LEN;

        if (!content_only) {
            p = cdlog_timestamp(p, last, use_color);
            p = cdlog_level(p, last, level, use_color);
        }
        p = cdlog_content(p, last, format, ap);
        if (!content_only) {
            p = os_slprintf(p, last, " (%s:%d)", file, line);
            p = os_slprintf(p, last, " %s()", func);
            p = cdlog_linefeed(p, last);
        }

        fprintf(stderr, "%s", cdlogstr);
        fflush(stderr);
    }
}

void cdlog_printf(cdlog_level_e level, int id, os_err_t err, char *file, int line, const char *func, int content_only, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cdlog_vprintf(level, id, err, basename(file), line, func, content_only, format, args);
    va_end(args);
}

void cdlog_hexdump_func(cdlog_level_e level, int id, const unsigned char *data, size_t len)
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

    cdlog_print(level, "%s", dumpstr);
}

PRIVATE cdlog_t *add_cdlog(cdlog_type_e type)
{
    cdlog_t *cdlog = NULL;

    os_pool_alloc(&cdlog_pool, &cdlog);
    os_assert(cdlog);
    memset(cdlog, 0, sizeof *cdlog);

    cdlog->type = type;

    cdlog->print.timestamp = 1;
    cdlog->print.domain = 1;
    cdlog->print.level = 1;
    cdlog->print.fileline = 1;
    cdlog->print.linefeed = 1;

    os_list_add(&cdlog_list, cdlog);

    return cdlog;
}


PRIVATE char *cdlog_timestamp(char *buf, char *last, int use_color)
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

PRIVATE char *cdlog_domain(char *buf, char *last, const char *name, int use_color)
{
    buf = os_slprintf(buf, last, "[%s%s%s] ",
            use_color ? TA_FGC_YELLOW : "",
            name,
            use_color ? TA_NOR : "");

    return buf;
}

PRIVATE char *cdlog_level(char *buf, char *last, cdlog_level_e level, int use_color)
{
    const char *colors[] = {
        TA_NOR,
        TA_FGC_BOLD_RED, TA_FGC_BOLD_YELLOW, TA_FGC_BOLD_CYAN,
        TA_FGC_BOLD_GREEN, TA_FGC_BOLD_WHITE, TA_FGC_WHITE,
    };

    buf = os_slprintf(buf, last, "%s%s%s: ",
            use_color ? colors[level] : "",
            g_logStr[level],
            use_color ? TA_NOR : "");

    return buf;
}

PRIVATE char *cdlog_content(char *buf, char *last, const char *format, va_list ap)
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

PRIVATE char *cdlog_linefeed(char *buf, char *last)
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

PRIVATE void file_writer(cdlog_t *cdlog, cdlog_level_e level, const char *string)
{
    fprintf(cdlog->file.out, "%s", string);
    fflush(cdlog->file.out);
}

