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
#include "private/os_cdlog_priv.h"

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

    void (*writer)(os_cdlog_t *cdlog, os_cdlog_level_e level, const char *string);

} os_cdlog_t;


PRIVATE OS_POOL(cdlog_pool, os_cdlog_t);
PRIVATE OS_LIST(cdlog_list);

PRIVATE os_cdlog_t *add_cdlog(cdlog_type_e type);
PRIVATE char *cdlog_timestamp(char *buf, char *last, int use_color);
PRIVATE char *cdlog_domain(char *buf, char *last, const char *name, int use_color);
PRIVATE char *cdlog_content(char *buf, char *last, const char *format, va_list ap);
PRIVATE char *cdlog_level(char *buf, char *last, os_cdlog_level_e level, int use_color);
PRIVATE char *cdlog_linefeed(char *buf, char *last);
PRIVATE void file_writer(os_cdlog_t *cdlog, os_cdlog_level_e level, const char *string);


typedef struct os_cdlog_domain_s {
    os_lnode_t node;

    int id;
    os_cdlog_level_e level;
    const char *name;
} os_cdlog_domain_t;

PRIVATE OS_POOL(domain_pool, os_cdlog_domain_t);
PRIVATE OS_LIST(domain_list);

os_cdlog_domain_t *os_cdlog_add_domain(const char *name, os_cdlog_level_e level)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(name);

    os_pool_alloc(&domain_pool, &domain);
    os_assert(domain);

    domain->name = name;
    domain->id = os_pool_index(&domain_pool, domain);
    domain->level = level;

    os_list_add(&domain_list, domain);

    return domain;
}

os_cdlog_domain_t *os_cdlog_find_domain(const char *name)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(name);

    os_list_for_each(&domain_list, domain)
        if (!os_strcasecmp(domain->name, name))
            return domain;

    return NULL;
}

void os_cdlog_remove_domain(os_cdlog_domain_t *domain)
{
    os_assert(domain);

    os_list_remove(&domain_list, domain);
    os_pool_free(&domain_pool, domain);
}

void os_cdlog_set_domain_level(int id, os_cdlog_level_e level)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    domain->level = level;
}

os_cdlog_level_e os_cdlog_get_domain_level(int id)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->level;
}

const char *os_cdlog_get_domain_name(int id)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->name;
}

int os_cdlog_get_domain_id(const char *name)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(name);
    
    domain = os_cdlog_find_domain(name);
    os_assert(domain);

    return domain->id;
}

_API_ void os_cdlog_install_domain(int *domain_id, const char *name, os_cdlog_level_e level)
{
    os_cdlog_domain_t *domain = NULL;

    os_assert(domain_id);
    os_assert(name);

    domain = os_cdlog_find_domain(name);
    if (domain) {
        os_cdlogs(WARN, "`%s` log-domain duplicated", name);
        if (level != domain->level) {
            os_cdlogs(WARN, ">>>[%s]", g_logStr[domain->level]);
	        os_cdlogs(WARN, "[%s]log-level changed<<<", g_logStr[level]);
            os_cdlog_set_domain_level(domain->id, level);
        }
    } else {
        domain = os_cdlog_add_domain(name, level);
        os_assert(domain);
    }

    *domain_id = domain->id;
}

void os_cdlog_set_mask_level(const char *_mask, os_cdlog_level_e level)
{
    os_cdlog_domain_t *domain = NULL;

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

            domain = os_cdlog_find_domain(name);
            if (domain)
                domain->level = level;
        }

        os_free(mask);
    } else {//all
        os_list_for_each(&domain_list, domain)
            domain->level = level;
    }
}

PRIVATE os_cdlog_level_e os_cdlog_level_from_string(const char *string)
{
    os_cdlog_level_e level = OS_ERROR;

    if (!strcasecmp(string, "none")) level = NONE;
    else if (!strcasecmp(string, "fatal")) level = FATAL;
    else if (!strcasecmp(string, "error")) level = ERROR;
    else if (!strcasecmp(string, "warn")) level = WARN;
    else if (!strcasecmp(string, "info")) level = INFO;
    else if (!strcasecmp(string, "debug")) level = DEBUG;
    else if (!strcasecmp(string, "trace")) level = TRACE;

    return level;
}

_API_ int os_cdlog_config_domain(const char *domain_mask, const char *level)
{
    if (domain_mask || level) {
        int l = os_global_context()->log.level;

        if (level) {
            l = os_cdlog_level_from_string(level);
            if (l == OS_ERROR) {
                os_cdlog1(ERROR, "Invalid LOG-LEVEL "
                        "[none:fatal|error|warn|info|debug|trace]: %s\n",
                        level);
                return OS_ERROR;
            }
        }

        os_cdlog_set_mask_level(domain_masks, l);
    }

    return OS_OK;
}


/////////////////////////////////////////////////////////////
_API_ void os_cdlog_init(void)
{
    os_pool_init(&cdlog_pool, os_global_context()->log.pool);

    os_pool_init(&domain_pool, os_global_context()->log.domain_pool);

    os_cdlog_add_domain("os", os_global_context()->log.level);
    os_cdlog_add_stderr();
}

_API_ void os_cdlog_final(void)
{
    os_cdlog_t *cdlog, *saved_cdlog;

    os_list_for_each_safe(&cdlog_list, saved_cdlog, cdlog)
        cdlog_remove(cdlog);
    os_pool_final(&cdlog_pool);

    os_cdlog_domain_t *domain, *saved_domain;

    os_list_for_each_safe(&domain_list, saved_domain, domain)
        os_cdlog_remove_domain(domain);
    os_pool_final(&domain_pool);

}

//////////////////////////////////////////////////////////////////////////////////

os_cdlog_t *os_cdlog_add_stderr(void)
{
    os_cdlog_t *cdlog = NULL;
    
    cdlog = add_cdlog(OS_LOG_STDERR_TYPE);
    os_assert(cdlog);

    cdlog->file.out = stderr;
    cdlog->writer = file_writer;

#if !defined(_WIN32)
    cdlog->print.color = 1;
#endif

    return cdlog;
}

os_cdlog_t *os_cdlog_add_file(const char *name)
{
    FILE *out = NULL;
    os_cdlog_t *cdlog = NULL;

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

void cdlog_remove(os_cdlog_t *cdlog)
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


void cdlog_vprintf(os_cdlog_level_e level, int id, os_err_t err, const char *file, int line, const char *func, int content_only, const char *format, va_list ap)
{
    os_cdlog_t *cdlog = NULL;
    os_cdlog_domain_t *domain = NULL;

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

void cdlog_printf(os_cdlog_level_e level, int id, os_err_t err, char *file, int line, const char *func, int content_only, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cdlog_vprintf(level, id, err, basename(file), line, func, content_only, format, args);
    va_end(args);
}

void cdlog_hexdump_func(os_cdlog_level_e level, int id, const unsigned char *data, size_t len)
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

PRIVATE os_cdlog_t *add_cdlog(cdlog_type_e type)
{
    os_cdlog_t *cdlog = NULL;

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

PRIVATE char *cdlog_level(char *buf, char *last, os_cdlog_level_e level, int use_color)
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

PRIVATE void file_writer(os_cdlog_t *cdlog, os_cdlog_level_e level, const char *string)
{
    fprintf(cdlog->file.out, "%s", string);
    fflush(cdlog->file.out);
}

