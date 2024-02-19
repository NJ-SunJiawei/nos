/************************************************************************
 *File name: os_mem.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

/*****************************************
 *Use talloc library
 *****************************************/
#if OS_USE_TALLOC == 1

void *__os_talloc_core;
PRIVATE os_thread_mutex_t mutex;
#define TALLOC_MEMSIZE 1

void os_kmem_init(void)
{
    os_thread_mutex_init(&mutex);

    talloc_enable_null_tracking();

    __os_talloc_core = talloc_named_const(NULL, TALLOC_MEMSIZE, "base");
}

void os_kmem_final(void)
{
    if (talloc_total_size(__os_talloc_core) != TALLOC_MEMSIZE)
        talloc_report_full(__os_talloc_core, stderr);

    talloc_free(__os_talloc_core);

    os_thread_mutex_destroy(&mutex);
}

void *os_kmem_get_mutex(void)
{
    return &mutex;
}

void *os_talloc_size(const void *ctx, size_t size, const char *name)
{
    void *ptr = NULL;

    os_thread_mutex_lock(&mutex);

    ptr = talloc_named_const(ctx, size, name);
    os_expect(ptr);

    os_thread_mutex_unlock(&mutex);

    return ptr;
}

void *os_talloc_zero_size(const void *ctx, size_t size, const char *name)
{
    void *ptr = NULL;

    os_thread_mutex_lock(&mutex);

    ptr = _talloc_zero(ctx, size, name);
    os_expect(ptr);

    os_thread_mutex_unlock(&mutex);

    return ptr;
}

void *os_talloc_realloc_size(
        const void *context, void *oldptr, size_t size, const char *name)
{
    void *ptr = NULL;

    os_thread_mutex_lock(&mutex);

    ptr = _talloc_realloc(context, oldptr, size, name);
    os_expect(ptr);

    os_thread_mutex_unlock(&mutex);

    return ptr;
}

int os_talloc_free(void *ptr, const char *location)
{
    int ret;

    os_thread_mutex_lock(&mutex);

    ret = _talloc_free(ptr, location);

    os_thread_mutex_unlock(&mutex);

    return ret;
}

#else
/*****************************************
 * Use buf library
 *****************************************/

void *os_malloc_debug(size_t size, const char *file_line)
{
    size_t headroom = 0;
    os_buf_t *buf = NULL;

    os_assert(size);

    headroom = sizeof(os_buf_t *);
    buf = os_buf_alloc_debug(NULL, headroom + size, file_line);
    if (!buf) {
        os_log(ERROR, "os_buf_alloc_debug[headroom:%d, size:%d] failed",
                (int)headroom, (int)size);
        return NULL;
    }

    os_buf_reserve(buf, headroom);
    memcpy(buf->head, &buf, headroom);
    os_buf_put(buf, size);

    return buf->data;
}

int os_free_debug(void *ptr)
{
    size_t headroom;
    os_buf_t *buf = NULL;

    if (!ptr)
        return OS_ERROR;

    headroom = sizeof(os_buf_t *);
    memcpy(&buf, (unsigned char*)ptr - headroom, headroom);
    os_assert(buf);

    os_buf_free(buf);

    return OS_OK;
}

void *os_calloc_debug(size_t nmemb, size_t size, const char *file_line)
{
    void *ptr = NULL;

    ptr = os_malloc_debug(nmemb * size, file_line);
    if (!ptr) {
        os_log(ERROR, "os_malloc_debug[nmemb:%d, size:%d] failed",
                (int)nmemb, (int)size);
        return NULL;
    }

    memset(ptr, 0, nmemb * size);
    return ptr;
}

void *os_realloc_debug(void *ptr, size_t size, const char *file_line)
{
    size_t headroom = 0;
    os_buf_t *buf = NULL;
    os_cluster_t *cluster = NULL;

    if (!ptr)
        return os_malloc(size);

    headroom = sizeof(os_buf_t *);

    memcpy(&buf, (unsigned char*)ptr - headroom, headroom);

    if (!buf) {
        os_log(ERROR, "Cannot get pkbuf from ptr[%p], headroom[%d]", ptr, (int)headroom);

        return NULL;
    }

    cluster = buf->cluster;
    if (!cluster) {
        os_log(ERROR, "No cluster");
        return NULL;
    }

    if (!size) {
        os_buf_free(buf);
        return NULL;
    }

    if (size > (cluster->size - headroom)) {
        void *new = NULL;

        new = os_malloc_debug(size, file_line);
        if (!new) {
            os_log(ERROR, "os_malloc_debug[%d] failed", (int)size);
            return NULL;
        }

        memcpy(new, ptr, buf->len);

        os_buf_free(buf);
        return new;
    } else {
        buf->tail = buf->data + size;
        buf->len = size;
        return ptr;
    }
}

#endif
