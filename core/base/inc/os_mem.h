/************************************************************************
 *File name: os_mem.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_MEM_H
#define OS_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define OS_MEM_CLEAR(__dATA) \
    do { \
        if ((__dATA)) { \
            os_free((__dATA)); \
            (__dATA) = NULL; \
        } \
    } while(0)

#if OS_USE_TALLOC == 1

#include <talloc.h>

extern void *__os_talloc_core;
void os_kmem_init(void);
void os_kmem_final(void);
void *os_kmem_get_mutex(void);
void *os_talloc_size(const void *ctx, size_t size, const char *name);
void *os_talloc_zero_size(const void *ctx, size_t size, const char *name);
void *os_talloc_realloc_size(
        const void *context, void *oldptr, size_t size, const char *name);
int os_talloc_free(void *ptr, const char *location);

/*****************************************
 * Memory Pool - Use talloc library
 *****************************************/

#define os_malloc(size)          os_talloc_size(__os_talloc_core, size, __location__)
#define os_calloc(nmemb, size)   os_talloc_zero_size(__os_talloc_core, (nmemb) * (size), __location__)
#define os_realloc(oldptr, size) os_talloc_realloc_size(__os_talloc_core, oldptr, size, __location__)
#define os_free(ptr)             os_talloc_free(ptr, __location__)

#else

void *os_malloc_debug(size_t size, const char *file_line);
void *os_calloc_debug(size_t nmemb, size_t size, const char *file_line);
void *os_realloc_debug( void *ptr, size_t size, const char *file_line);
int os_free_debug(void *ptr);

/*****************************************
 * Memory Pool - Use buf library
 *****************************************/

#define os_malloc(size) os_malloc_debug(size, OS_FILE_LINE)
#define os_calloc(nmemb, size) os_calloc_debug(nmemb, size, OS_FILE_LINE)
#define os_realloc(ptr, size) os_realloc_debug(ptr, size, OS_FILE_LINE)
#define os_free(ptr) os_free_debug(ptr)

#endif

#ifdef __cplusplus
}
#endif

#endif
