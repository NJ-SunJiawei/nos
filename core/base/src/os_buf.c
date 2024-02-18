/************************************************************************
 *File name: os_buf.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/

#include "os_init.h"


#if OS_USE_TALLOC == 0
#define OS_CLUSTER_128_SIZE    128
#define OS_CLUSTER_256_SIZE    256
#define OS_CLUSTER_512_SIZE    512
#define OS_CLUSTER_1024_SIZE   1024
#define OS_CLUSTER_2048_SIZE   2048
#define OS_CLUSTER_8192_SIZE   8192
#define OS_CLUSTER_32768_SIZE  32768
#define OS_CLUSTER_LIL_SIZE    1024*128
#define OS_CLUSTER_MID_SIZE    1024*512
#define OS_CLUSTER_BIG_SIZE    (1024*1024)

typedef uint8_t os_cluster_128_t[OS_CLUSTER_128_SIZE];
typedef uint8_t os_cluster_256_t[OS_CLUSTER_256_SIZE];
typedef uint8_t os_cluster_512_t[OS_CLUSTER_512_SIZE];
typedef uint8_t os_cluster_1024_t[OS_CLUSTER_1024_SIZE];
typedef uint8_t os_cluster_2048_t[OS_CLUSTER_2048_SIZE];
typedef uint8_t os_cluster_8192_t[OS_CLUSTER_8192_SIZE];
typedef uint8_t os_cluster_32768_t[OS_CLUSTER_32768_SIZE];
typedef uint8_t os_cluster_lil_t[OS_CLUSTER_LIL_SIZE];
typedef uint8_t os_cluster_mid_t[OS_CLUSTER_MID_SIZE];
typedef uint8_t os_cluster_big_t[OS_CLUSTER_BIG_SIZE];

OS_STATIC_ASSERT(sizeof(os_cluster_128_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_256_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_512_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_1024_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_2048_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_8192_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_32768_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_lil_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_mid_t) % sizeof(void *) == 0);
OS_STATIC_ASSERT(sizeof(os_cluster_big_t) % sizeof(void *) == 0);

PRIVATE OS_POOL(buf_pool, os_buf_pool_t);
PRIVATE os_buf_pool_t *default_pool = NULL;

typedef struct os_buf_pool_s {
    OS_POOL(buf, os_buf_t);
    OS_POOL(cluster, os_cluster_t);

    OS_POOL(cluster_128, os_cluster_128_t);
    OS_POOL(cluster_256, os_cluster_256_t);
    OS_POOL(cluster_512, os_cluster_512_t);
    OS_POOL(cluster_1024, os_cluster_1024_t);
    OS_POOL(cluster_2048, os_cluster_2048_t);
    OS_POOL(cluster_8192, os_cluster_8192_t);
    OS_POOL(cluster_32768, os_cluster_32768_t);
    OS_POOL(cluster_lil, os_cluster_lil_t);
    OS_POOL(cluster_mid, os_cluster_mid_t);
    OS_POOL(cluster_big, os_cluster_big_t);

    os_thread_mutex_t mutex;
} os_buf_pool_t;

PRIVATE os_cluster_t *cluster_alloc(os_buf_pool_t *pool, unsigned int size);
PRIVATE void cluster_free(os_buf_pool_t *pool, os_cluster_t *cluster);
#endif

void *os_buf_put_data(
        os_buf_t *buf, const void *data, unsigned int len)
{
    void *tmp = os_buf_put(buf, len);

    memcpy(tmp, data, len);
    return tmp;
}

void os_buf_init(void)
{
#if OS_USE_TALLOC == 0
    os_pool_init(&buf_pool, os_global_context()->buf.pool);
#endif
}

void os_buf_final(void)
{
#if OS_USE_TALLOC == 0
    os_pool_final(&buf_pool);
#endif
}

void os_buf_default_init(os_buf_config_t *config)
{
#if OS_USE_TALLOC == 0
    os_assert(config);
    memset(config, 0, sizeof *config);

    config->cluster_128_pool = 65536;
    config->cluster_256_pool = 16384;
    config->cluster_512_pool = 4096;
    config->cluster_1024_pool = 2048;
    config->cluster_2048_pool = 1024;
    config->cluster_8192_pool = 256;
    config->cluster_32768_pool = 64;
    config->cluster_lil_pool = 32;
    config->cluster_mid_pool = 16;
    config->cluster_big_pool = 8;
#endif
}

void os_buf_default_create(os_buf_config_t *config)
{
#if OS_USE_TALLOC == 0
    default_pool = os_buf_pool_create(config);
#endif
}

void os_buf_default_destroy(void)
{
#if OS_USE_TALLOC == 0
    os_buf_pool_destroy(default_pool);
#endif
}

os_buf_pool_t *os_buf_pool_create(os_buf_config_t *config)
{
    os_buf_pool_t *pool = NULL;
#if OS_USE_TALLOC == 0
    int tmp = 0;

    os_assert(config);

    os_pool_alloc(&buf_pool, &pool);
    os_assert(pool);
    memset(pool, 0, sizeof *pool);

    os_thread_mutex_init(&pool->mutex);

    tmp = config->cluster_128_pool + config->cluster_256_pool +\
        config->cluster_512_pool + config->cluster_1024_pool +\
        config->cluster_2048_pool + config->cluster_8192_pool +\
        config->cluster_32768_pool +\
		config->cluster_lil_pool + config->cluster_mid_pool +config->cluster_big_pool;

    os_pool_init(&pool->buf, tmp);
    os_pool_init(&pool->cluster, tmp);

    os_pool_init(&pool->cluster_128, config->cluster_128_pool);
    os_pool_init(&pool->cluster_256, config->cluster_256_pool);
    os_pool_init(&pool->cluster_512, config->cluster_512_pool);
    os_pool_init(&pool->cluster_1024, config->cluster_1024_pool);
    os_pool_init(&pool->cluster_2048, config->cluster_2048_pool);
    os_pool_init(&pool->cluster_8192, config->cluster_8192_pool);
    os_pool_init(&pool->cluster_32768, config->cluster_32768_pool);
    os_pool_init(&pool->cluster_lil, config->cluster_lil_pool);
    os_pool_init(&pool->cluster_mid, config->cluster_mid_pool);
    os_pool_init(&pool->cluster_big, config->cluster_big_pool);
#endif

    return pool;
}

#define os_buf_pool_final(pool) do { \
    if (((pool)->size != (pool)->avail)) { \
        int i; \
        os_logs(ERROR, ">>>%s", (pool)->name); \
		os_log2(ERROR, "%d in '[%d]' were not released<<<", (pool)->size - (pool)->avail, (pool)->size); \
        for (i = 0; i < (pool)->size; i++) { \
            os_buf_t *buf = (pool)->index[i]; \
            if (buf) { \
				os_logs(ERROR, ">>>(%s)", buf->file_line); \
				os_log1(ERROR, "SIZE[%d] is not freed<<<", buf->len); \
            } \
        } \
    } \
    free((pool)->free); \
    free((pool)->array); \
    free((pool)->index); \
} while (0)

void os_buf_pool_destroy(os_buf_pool_t *pool)
{
#if OS_USE_TALLOC == 0
    os_assert(pool);

    os_buf_pool_final(&pool->buf);
    os_pool_final(&pool->cluster);

    os_pool_final(&pool->cluster_128);
    os_pool_final(&pool->cluster_256);
    os_pool_final(&pool->cluster_512);
    os_pool_final(&pool->cluster_1024);
    os_pool_final(&pool->cluster_2048);
    os_pool_final(&pool->cluster_8192);
    os_pool_final(&pool->cluster_32768);
    os_pool_final(&pool->cluster_lil);
    os_pool_final(&pool->cluster_mid);
    os_pool_final(&pool->cluster_big);

    os_thread_mutex_destroy(&pool->mutex);

    os_pool_free(&buf_pool, pool);
#endif
}

os_buf_t *os_buf_alloc_debug(
        os_buf_pool_t *pool, unsigned int size, const char *file_line)
{
#if OS_USE_TALLOC == 1
    os_buf_t *buf = NULL;

    buf = os_talloc_zero_size(__os_talloc_core, sizeof(*buf) + size, file_line);//pool
    if (!buf) {
        os_log1(ERROR, "os_buf_alloc() failed [size=%d]", size);
        return NULL;
    }

    buf->head = buf->_data;
    buf->end = buf->_data + size;

    buf->len = 0;

    buf->data = buf->_data;
    buf->tail = buf->_data;

    buf->file_line = file_line; /* For debug */

    return buf;
#else
    os_buf_t *buf = NULL;
    os_cluster_t *cluster = NULL;

    if (pool == NULL)
        pool = default_pool;
    os_assert(pool);

    os_thread_mutex_lock(&pool->mutex);

    cluster = cluster_alloc(pool, size);
    if (!cluster) {
        os_log1(ERROR, "os_buf_alloc() failed [size=%d]", size);
        os_thread_mutex_unlock(&pool->mutex);
        return NULL;
    }

    os_pool_alloc(&pool->buf, &buf);
    if (!buf) {
        os_log1(ERROR, "os_buf_alloc() failed [size=%d]", size);
        os_thread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    memset(buf, 0, sizeof(*buf));

    OS_OBJECT_REF(cluster);

    buf->cluster = cluster;

    buf->len = 0;

    buf->data = cluster->buffer;
    buf->head = cluster->buffer;
    buf->tail = cluster->buffer;
    buf->end = cluster->buffer + size;

    buf->file_line = file_line; /* For debug */

    buf->pool = pool;

    os_thread_mutex_unlock(&pool->mutex);

    return buf;
#endif
}

void os_buf_free(os_buf_t *buf)
{
#if OS_USE_TALLOC == 1
    os_talloc_free(buf, OS_FILE_LINE);
#else
    os_buf_pool_t *pool = NULL;
    os_cluster_t *cluster = NULL;
    os_assert(buf);

    pool = buf->pool;
    os_assert(pool);

    os_thread_mutex_lock(&pool->mutex);

    cluster = buf->cluster;
    os_assert(cluster);

    if (OS_OBJECT_IS_REF(cluster)){
		OS_OBJECT_UNREF(cluster);
    }
    else{
    	cluster_free(pool, buf->cluster);
	}

    os_pool_free(&pool->buf, buf);

    os_thread_mutex_unlock(&pool->mutex);
#endif
}

os_buf_t *os_buf_copy_debug(os_buf_t *buf, const char *file_line)
{
#if OS_USE_TALLOC == 1
    os_buf_t *newbuf;
#else
    os_buf_pool_t *pool = NULL;
    os_buf_t *newbuf = NULL;
#endif
    int size = 0;

    os_assert(buf);
    size = buf->end - buf->head;
    if (size <= 0) {
		os_logs(ERROR, ">>>%s", file_line); \
        os_log1(ERROR, "Invalid argument[size=%d]<<<", size);
        return NULL;
    }

#if OS_USE_TALLOC == 1
    newbuf = os_buf_alloc_debug(NULL, size, file_line);
    if (!newbuf) {
        os_log1(ERROR, "os_buf_alloc() failed [size=%d]", size);
        return NULL;
    }

    /* copy data */
    memcpy(newbuf->_data, buf->_data, size);

    /* copy header */
    newbuf->len = buf->len;

    newbuf->tail += buf->tail - buf->_data;
    newbuf->data += buf->data - buf->_data;

    return newbuf;
#else
    pool = buf->pool;
    os_assert(pool);

    os_thread_mutex_lock(&pool->mutex);

    os_pool_alloc(&pool->buf, &newbuf);
    if (!newbuf) {
        os_log1(ERROR, "os_buf_copy() failed [size=%d]", size);
        os_thread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    os_assert(newbuf);
    memcpy(newbuf, buf, sizeof *buf);

    OS_OBJECT_REF(newbuf->cluster);

    os_thread_mutex_unlock(&pool->mutex);
#endif

    return newbuf;
}

#if OS_USE_TALLOC == 0
PRIVATE os_cluster_t *cluster_alloc(
        os_buf_pool_t *pool, unsigned int size)
{
    os_cluster_t *cluster = NULL;
    void *buffer = NULL;
    os_assert(pool);

    os_pool_alloc(&pool->cluster, &cluster);
    os_assert(cluster);
    memset(cluster, 0, sizeof(*cluster));

    if (size <= OS_CLUSTER_128_SIZE) {
        os_pool_alloc(&pool->cluster_128, (os_cluster_128_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_128_SIZE;
    } else if (size <= OS_CLUSTER_256_SIZE) {
        os_pool_alloc(&pool->cluster_256, (os_cluster_256_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_256_SIZE;
    } else if (size <= OS_CLUSTER_512_SIZE) {
        os_pool_alloc(&pool->cluster_512, (os_cluster_512_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_512_SIZE;
    } else if (size <= OS_CLUSTER_1024_SIZE) {
        os_pool_alloc(&pool->cluster_1024, (os_cluster_1024_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_1024_SIZE;
    } else if (size <= OS_CLUSTER_2048_SIZE) {
        os_pool_alloc(&pool->cluster_2048, (os_cluster_2048_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_2048_SIZE;
    } else if (size <= OS_CLUSTER_8192_SIZE) {
        os_pool_alloc(&pool->cluster_8192, (os_cluster_8192_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_8192_SIZE;
    } else if (size <= OS_CLUSTER_32768_SIZE) {
        os_pool_alloc(&pool->cluster_32768, (os_cluster_32768_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_32768_SIZE;
    } else if (size <= OS_CLUSTER_LIL_SIZE) {
        os_pool_alloc(&pool->cluster_lil, (os_cluster_lil_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_LIL_SIZE;
    }else if (size <= OS_CLUSTER_MID_SIZE) {
        os_pool_alloc(&pool->cluster_mid, (os_cluster_mid_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_MID_SIZE;
    }  else if (size <= OS_CLUSTER_BIG_SIZE) {
        os_pool_alloc(&pool->cluster_big, (os_cluster_big_t**)&buffer);
        if (!buffer) {
            os_log0(ERROR, "os_pool_alloc() failed");
            return NULL;
        }
        cluster->size = OS_CLUSTER_BIG_SIZE;
    } else {
        os_log1(FATAL, "invalid size = %d", size);
        os_assert_if_reached();
    }
    cluster->buffer = buffer;

    return cluster;
}

PRIVATE void cluster_free(os_buf_pool_t *pool, os_cluster_t *cluster)
{
    os_assert(pool);
    os_assert(cluster);
    os_assert(cluster->buffer);

    switch (cluster->size) {
    case OS_CLUSTER_128_SIZE:
        os_pool_free(&pool->cluster_128, (os_cluster_128_t*)cluster->buffer);
        break;
    case OS_CLUSTER_256_SIZE:
        os_pool_free(&pool->cluster_256, (os_cluster_256_t*)cluster->buffer);
        break;
    case OS_CLUSTER_512_SIZE:
        os_pool_free(&pool->cluster_512, (os_cluster_512_t*)cluster->buffer);
        break;
    case OS_CLUSTER_1024_SIZE:
        os_pool_free(
                &pool->cluster_1024, (os_cluster_1024_t*)cluster->buffer);
        break;
    case OS_CLUSTER_2048_SIZE:
        os_pool_free(
                &pool->cluster_2048, (os_cluster_2048_t*)cluster->buffer);
        break;
    case OS_CLUSTER_8192_SIZE:
        os_pool_free(
                &pool->cluster_8192, (os_cluster_8192_t*)cluster->buffer);
        break;
    case OS_CLUSTER_32768_SIZE:
        os_pool_free(
                &pool->cluster_32768, (os_cluster_32768_t*)cluster->buffer);
        break;
	case OS_CLUSTER_LIL_SIZE:
		os_pool_free(&pool->cluster_lil, (os_cluster_lil_t*)cluster->buffer);
		break;
	case OS_CLUSTER_MID_SIZE:
		os_pool_free(&pool->cluster_mid, (os_cluster_mid_t*)cluster->buffer);
		break;
    case OS_CLUSTER_BIG_SIZE:
        os_pool_free(&pool->cluster_big, (os_cluster_big_t*)cluster->buffer);
        break;
    default:
        os_assert_if_reached();
    }

    os_pool_free(&pool->cluster, cluster);
}

#endif


int os_buf_tailroom(const os_buf_t *buf)
{
    return buf->end - buf->tail;
}

int os_buf_headroom(const os_buf_t *buf)
{
    return buf->data - buf->head;
}

void os_buf_reserve(os_buf_t *buf, int len)
{
    buf->data += len;
    buf->tail += len;
}

void *os_buf_put(os_buf_t *buf, unsigned int len)
{
    void *tmp = buf->tail;

    if (os_unlikely(os_buf_tailroom(buf) < (int)len))
        os_assert_if_reached();

    buf->tail += len;
    buf->len += len;

    return tmp;
}

void os_buf_put_u8(os_buf_t *buf, uint8_t val)
{
    *(uint8_t *)os_buf_put(buf, 1) = val;
}

void os_buf_put_u16(os_buf_t *buf, uint16_t val)
{
    void *p = os_buf_put(buf, 2);
    uint16_t tmp = htobe16(val);
    memcpy(p, &tmp, 2);
}

void os_buf_put_u32(os_buf_t *buf, uint32_t val)
{
    void *p = os_buf_put(buf, 4);
    uint32_t tmp = htobe32(val);
    memcpy(p, &tmp, 4);
}

void *os_buf_push(os_buf_t *buf, unsigned int len)
{
    if (os_unlikely(os_buf_headroom(buf) < (int)len))
        os_assert_if_reached();

    buf->data -= len;
    buf->len += len;

    return buf->data;
}

void *os_buf_pull_inline(
        os_buf_t *buf, unsigned int len)
{
    buf->len -= len;
    return buf->data += len;
}

void *os_buf_pull(os_buf_t *buf, unsigned int len)
{
    return os_unlikely(len > buf->len) ?
        NULL : os_buf_pull_inline(buf, len);
}

int os_buf_trim(os_buf_t *buf, int len)
{
    if (os_unlikely(len < 0))
        os_assert_if_reached();
    if (os_unlikely(len > buf->len)) {
        os_log2(ERROR, "len(%d) > buf->len(%d)", len, buf->len);
        return OS_ERROR;
    }

    buf->tail = buf->data + len;
    buf->len = len;

    return OS_OK;
}

void os_buf_show_avail(os_buf_pool_t *pool)
{
#if OS_USE_TALLOC == 0
	if(NULL == pool) pool = default_pool;
	os_assert(pool);

    fprintf(stderr, "OS_BUF_POOL            size[%d], avail[%d]!\n", os_pool_size(&buf_pool), os_pool_avail(&buf_pool));
	fprintf(stderr, "OS_BUF                 size[%d], avail[%d]!\n", os_pool_size(&pool->buf), os_pool_avail(&pool->buf));
    fprintf(stderr, "OS_CLUSTER             size[%d], avail[%d]!\n", os_pool_size(&pool->cluster), os_pool_avail(&pool->cluster));
    fprintf(stderr, "OS_CLUSTER_128_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_128), os_pool_avail(&pool->cluster_128));
    fprintf(stderr, "OS_CLUSTER_256_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_256), os_pool_avail(&pool->cluster_256));
    fprintf(stderr, "OS_CLUSTER_512_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_512), os_pool_avail(&pool->cluster_512));
    fprintf(stderr, "OS_CLUSTER_1024_SIZE   size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_1024), os_pool_avail(&pool->cluster_1024));
    fprintf(stderr, "OS_CLUSTER_2048_SIZE   size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_2048), os_pool_avail(&pool->cluster_2048));
    fprintf(stderr, "OS_CLUSTER_8192_SIZE   size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_8192), os_pool_avail(&pool->cluster_8192));
	fprintf(stderr, "OS_CLUSTER_32768_SIZE	 size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_32768), os_pool_avail(&pool->cluster_32768));
	fprintf(stderr, "OS_CLUSTER_LIL_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_lil), os_pool_avail(&pool->cluster_lil));
    fprintf(stderr, "OS_CLUSTER_MID_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_mid), os_pool_avail(&pool->cluster_mid));
    fprintf(stderr, "OS_CLUSTER_BIG_SIZE    size[%d], avail[%d]!\n", os_pool_size(&pool->cluster_big), os_pool_avail(&pool->cluster_big));
#else
	fprintf(stderr,
			"%*s%-30s contains %6lu bytes in %3lu blocks (ref %d) %p\n",
			0, "", "core",
			(unsigned long)talloc_total_size(__os_talloc_core),
			(unsigned long)talloc_total_blocks(__os_talloc_core),
			(int)talloc_reference_count(__os_talloc_core),
			__os_talloc_core);
	talloc_report_full(__ogs_talloc_core, stderr);
#endif
}

