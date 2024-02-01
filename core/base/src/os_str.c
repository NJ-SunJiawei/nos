/**
 *
 * Orcania library
 *
 * Different functions for different purposes but that can be shared between
 * other projects
 *
 * orcania.c: main functions definitions
 *
 * Copyright 2015-2020 Nicolas Mora <mail@babelouest.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU GENERAL PUBLIC LICENSE for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/************************************************************************
 *File name: os_str.c
 *Description:
 *
 *Current Version:
 *Author: Modified by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "os_init.h"

int os_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    int r = -1;

    /* Microsoft has finally implemented snprintf in Visual Studio 2015.
     * In previous versions, I will simulate it as below. */
#if defined(_MSC_VER) && _MSC_VER < 1900
    os_assert(str);

    if (size != 0)
        r = _vsnprintf_s(str, size, _TRUNCATE, format, ap);

    if (r == -1)
        r = _vscprintf(format, ap);
#else
    r = vsnprintf(str, size, format, ap);
#endif
    str[size-1] = '\0';

    return r;
}

int os_snprintf(char *str, size_t size, const char *format, ...)
{
    int r;
    va_list ap;

    va_start(ap, format);
    r = os_vsnprintf(str, size, format, ap);
    va_end(ap);

    return r;
}

char *os_vslprintf(char *str, char *last, const char *format, va_list ap)
{
    int r = -1;

    os_assert(last);

    if (!str)
        return NULL;

    if (str < last)
        r = os_vsnprintf(str, last - str, format, ap);

    return (str + r);
}

char *os_slprintf(char *str, char *last, const char *format, ...)
{
    char *r;
    va_list ap;

    va_start(ap, format);
    r = os_vslprintf(str, last, format, ap);
    va_end(ap);

    return r;
}

char *os_cpystrn(char *dst, const char *src, size_t dst_size)
{
    char *d = dst, *end;

    if (dst_size == 0) {
        return (dst);
    }

    if (src) {
        end = dst + dst_size - 1;

        for (; d < end; ++d, ++src) {
            if (!(*d = *src)) {
                return (d);
            }
        }
    }

    *d = '\0';    /* always null terminate */

    return (d);
}

#if OS_USE_TALLOC == 1
/*****************************************
 * Use talloc library
 *****************************************/

char *os_talloc_strdup(const void *t, const char *p)
{
    char *ptr = NULL;

    os_thread_mutex_lock(os_kmem_get_mutex());

    ptr = talloc_strdup(t, p);
    os_expect(ptr);

    os_thread_mutex_unlock(os_kmem_get_mutex());

    return ptr;
}

char *os_talloc_strndup(const void *t, const char *p, size_t n)
{
    char *ptr = NULL;

    os_thread_mutex_lock(os_kmem_get_mutex());

    ptr = talloc_strndup(t, p, n);
    os_expect(ptr);

    os_thread_mutex_unlock(os_kmem_get_mutex());

    return ptr;
}

void *os_talloc_memdup(const void *t, const void *p, size_t size)
{
    void *ptr = NULL;

    os_thread_mutex_lock(os_kmem_get_mutex());

    ptr = talloc_memdup(t, p, size);
    os_expect(ptr);

    os_thread_mutex_unlock(os_kmem_get_mutex());

    return ptr;
}

char *os_talloc_asprintf(const void *t, const char *fmt, ...)
{
    va_list ap;
    char *ret;

    os_thread_mutex_lock(os_kmem_get_mutex());

    va_start(ap, fmt);
    ret = talloc_vasprintf(t, fmt, ap);
    os_expect(ret);
    va_end(ap);

    os_thread_mutex_unlock(os_kmem_get_mutex());

    return ret;
}

char *os_talloc_asprintf_append(char *s, const char *fmt, ...)
{
    va_list ap;

    os_thread_mutex_lock(os_kmem_get_mutex());

    va_start(ap, fmt);
    s = talloc_vasprintf_append(s, fmt, ap);
    os_expect(s);
    va_end(ap);

    os_thread_mutex_unlock(os_kmem_get_mutex());

    return s;
}

#else
/*****************************************
 * Use buf library
 *****************************************/

char *os_strdup_debug(const char *s, const char *file_line)
{
    char *res;
    size_t len;

    if (s == NULL)
        return NULL;

    len = strlen(s) + 1;
    res = os_memdup_debug(s, len, file_line);
    if (!res) {
        OS_ERR("os_memdup_debug[len:%d] failed", (int)len);
        return res;
    }
    return res;
}

char *os_strndup_debug(
        const char *s, size_t n, const char *file_line)
{
    char *res;
    const char *end;

    if (s == NULL)
        return NULL;

    end = memchr(s, '\0', n);
    if (end != NULL)
        n = end - s;
    res = os_malloc_debug(n + 1, file_line);
    if (!res) {
        OS_ERR("os_malloc_debug[n:%d] failed", (int)n);
        return res;
    }
    memcpy(res, s, n);
    res[n] = '\0';
    return res;
}

void *os_memdup_debug(
        const void *m, size_t n, const char *file_line)
{
    void *res;

    if (m == NULL)
        return NULL;

    res = os_malloc_debug(n, file_line);
    if (!res) {
        OS_ERR("os_malloc_debug[n:%d] failed", (int)n);
        return res;
    }
    memcpy(res, m, n);
    return res;
}

/*
 * char *os_msprintf(const char *message, ...)
 * char *mstrcatf(char *source, const char *message, ...)
 *
 * Orcania library
 * Copyright 2015-2018 Nicolas Mora <mail@babelouest.org>
 * License: LGPL-2.1
 *
 * https://github.com/babelouest/orcania.git
 */
char *os_msprintf_debug(const char *file_line, const char *message, ...)
{
    va_list argp, argp_cpy;
    size_t out_len = 0;
    char *out = NULL;
    if (message != NULL) {
        va_start(argp, message);
        va_copy(argp_cpy, argp); /* We make a copy because
                                    in some architectures,
                                    vsnprintf can modify argp */
        out_len = vsnprintf(NULL, 0, message, argp);
        out = os_malloc_debug(out_len + sizeof(char), file_line);
        if (out == NULL) {
            va_end(argp);
            va_end(argp_cpy);
            return NULL;
        }
        vsnprintf(out, (out_len + sizeof(char)), message, argp_cpy);
        va_end(argp);
        va_end(argp_cpy);
    }
    return out;
}

char *os_mstrcatf_debug(char *source, const char *file_line, const char *message, ...)
{
    va_list argp, argp_cpy;
    char *out = NULL, *message_formatted = NULL;
    size_t message_formatted_len = 0, out_len = 0;

    if (message != NULL) {
        if (source != NULL) {
            va_start(argp, message);
            va_copy(argp_cpy, argp); /* We make a copy because
                                        in some architectures,
                                        vsnprintf can modify argp */
            message_formatted_len = vsnprintf(NULL, 0, message, argp);
            message_formatted = os_malloc(message_formatted_len+sizeof(char));
            if (message_formatted != NULL) {
                vsnprintf(message_formatted,
                    (message_formatted_len+sizeof(char)), message, argp_cpy);
                out = os_msprintf_debug(
                        file_line, "%s%s", source, message_formatted);
                os_free(message_formatted);
                os_free(source);
            }
            va_end(argp);
            va_end(argp_cpy);
        } else {
            va_start(argp, message);
            va_copy(argp_cpy, argp); /* We make a copy because
                                        in some architectures,
                                        vsnprintf can modify argp */
            out_len = vsnprintf(NULL, 0, message, argp);
            out = os_malloc_debug(out_len+sizeof(char), file_line);
            if (out != NULL) {
                vsnprintf(out, (out_len+sizeof(char)), message, argp_cpy);
            }
            va_end(argp);
            va_end(argp_cpy);
        }
    }
    return out;
}

#endif
char *os_trimwhitespace(char *str)
{
    char *end;

    if (str == NULL) {
        return NULL;
    } else if (*str == 0) {
        return str;
    }

    while (isspace((unsigned char)*str)) str++;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end+1) = 0;

    return str;
}

char *os_left_trimcharacter(char *str, char to_remove)
{
    if (str == NULL) {
        return NULL;
    } else if (*str == 0) {
        return str;
    }

    while(*str == to_remove) str++;

    return str;
}

char *os_right_trimcharacter(char *str, char to_remove)
{
    char *end;

    if (str == NULL) {
        return NULL;
    } else if (*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while(end > str && (*end == to_remove)) {
        end--;
    }

    *(end+1) = 0;

    return str;
}

char *os_trimcharacter(char *str, char to_remove)
{
    return os_right_trimcharacter(
            os_left_trimcharacter(str, to_remove), to_remove);
}
