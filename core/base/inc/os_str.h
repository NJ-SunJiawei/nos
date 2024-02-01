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
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_STRINGS_H
#define OS_STRINGS_H

OS_BEGIN_EXTERN_C

#if defined(_WIN32)
#define os_strtok_r strtok_s
#define os_strcasecmp _stricmp
#define os_strncasecmp _strnicmp
#else
#define os_strtok_r strtok_r
#define os_strcasecmp strcasecmp
#define os_strncasecmp strncasecmp
#endif

#define OS_STRING_DUP(__dST, __sRC) \
    do { \
        OS_MEM_CLEAR(__dST); \
        __dST = os_strdup(__sRC); \
        os_assert(__dST); \
    } while(0)

int os_vsnprintf(char *str, size_t size, const char *format, va_list ap)
    OS_GNUC_PRINTF (3, 0);
int os_snprintf(char *str, size_t size, const char *format, ...)
    OS_GNUC_PRINTF(3, 4);
char *os_vslprintf(char *str, char *last, const char *format, va_list ap)
    OS_GNUC_PRINTF (3, 0);
char *os_slprintf(char *str, char *last, const char *format, ...)
    OS_GNUC_PRINTF(3, 4);

char *os_cpystrn(char *dst, const char *src, size_t dst_size);

char *os_talloc_strdup(const void *t, const char *p);
char *os_talloc_strndup(const void *t, const char *p, size_t n);
void *os_talloc_memdup(const void *t, const void *p, size_t size);
char *os_talloc_asprintf(const void *t, const char *fmt, ...)
    OS_GNUC_PRINTF(2, 3);
char *os_talloc_asprintf_append(char *s, const char *fmt, ...)
    OS_GNUC_PRINTF(2, 3);

char *os_strdup_debug(const char *s, const char *file_line);
char *os_strndup_debug(const char *s, size_t n, const char *file_line);
void *os_memdup_debug(const void *m, size_t n, const char *file_line);

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
    OS_GNUC_PRINTF(2, 3);
char *os_mstrcatf_debug(
    char *source, const char *file_line, const char *message, ...)
    OS_GNUC_PRINTF(3, 4);

#if OS_USE_TALLOC == 1

/*****************************************
 * Use talloc library
 *****************************************/

#define os_strdup(p)          os_talloc_strdup(__os_talloc_core, p)
#define os_strndup(p, n)      os_talloc_strndup(__os_talloc_core, p, n)
#define os_memdup(p, size)    os_talloc_memdup(__os_talloc_core, p, size)
#define os_msprintf(...)      os_talloc_asprintf(__os_talloc_core, __VA_ARGS__)
#define os_mstrcatf(s, ...)   os_talloc_asprintf_append(s, __VA_ARGS__)

#else

/*****************************************
 *Use buf library
 *****************************************/

#define os_strdup(s)         os_strdup_debug(s, OS_FILE_LINE)
#define os_strndup(s, n)     os_strndup_debug(s, n, OS_FILE_LINE)
#define os_memdup(m, n)      os_memdup_debug(m, n, OS_FILE_LINE)
#define os_msprintf(...)     os_msprintf_debug(OS_FILE_LINE, __VA_ARGS__)
#define os_mstrcatf(source, ...)  os_mstrcatf_debug(source, OS_FILE_LINE, __VA_ARGS__)

#endif

char *os_trimwhitespace(char *str);
char *os_left_trimcharacter(char *str, char to_remove);
char *os_right_trimcharacter(char *str, char to_remove);
char *os_trimcharacter(char *str, char to_remove);

OS_END_EXTERN_C

#endif
