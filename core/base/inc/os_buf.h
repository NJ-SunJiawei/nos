/************************************************************************
 *File name: os_buf.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_BUF_H
#define OS_BUF_H

OS_BEGIN_EXTERN_C

typedef struct os_cluster_s {
    unsigned char *buffer;
    unsigned int size;
    unsigned int reference_count;
} os_cluster_t;

#if OS_USE_TALLOC == 1
typedef void os_buf_pool_t;
#else
typedef struct os_buf_pool_s os_buf_pool_t;
void os_buf_show_avail(os_buf_pool_t *pool);
#endif
typedef struct os_buf_s {
    os_lnode_t lnode;

    uint64_t param[2];

    os_cluster_t *cluster;

    unsigned int len;

    unsigned char *head;
    unsigned char *tail;
    unsigned char *data;
    unsigned char *end;

    const char *file_line;
    
    os_buf_pool_t *pool;

    unsigned char _data[0];
} os_buf_t;

typedef struct os_buf_config_s {
    int cluster_128_pool;
    int cluster_256_pool;
    int cluster_512_pool;
    int cluster_1024_pool;
    int cluster_2048_pool;
    int cluster_8192_pool;
    int cluster_32768_pool;
    int cluster_lil_pool;
    int cluster_mid_pool;
    int cluster_big_pool;
} os_buf_config_t;

void os_buf_init(void);
void os_buf_final(void);
void os_buf_default_init(os_buf_config_t *config);
void os_buf_default_create(os_buf_config_t *config);
void os_buf_default_destroy(void);

os_buf_pool_t *os_buf_pool_create(os_buf_config_t *config);
void os_buf_pool_destroy(os_buf_pool_t *pool);

#define os_buf_alloc(pool, size) os_buf_alloc_debug(pool, size, OS_FILE_LINE)
os_buf_t *os_buf_alloc_debug(os_buf_pool_t *pool, unsigned int size, const char *file_line);

void os_buf_free(os_buf_t *buf);
void *os_buf_put_data(os_buf_t *buf, const void *data, unsigned int len);

#define os_buf_copy(buf) os_buf_copy_debug(buf, OS_FILE_LINE)
os_buf_t *os_buf_copy_debug(os_buf_t *buf, const char *file_line);

int os_buf_tailroom(const os_buf_t *buf);
int os_buf_headroom(const os_buf_t *buf);
void os_buf_reserve(os_buf_t *buf, int len);
void *os_buf_put(os_buf_t *buf, unsigned int len);
void os_buf_put_u8(os_buf_t *buf, uint8_t val);
void os_buf_put_u16(os_buf_t *buf, uint16_t val);
void os_buf_put_u32(os_buf_t *buf, uint32_t val);
void *os_buf_push(os_buf_t *buf, unsigned int len);
void *os_buf_pull_inline(os_buf_t *buf, unsigned int len);
void *os_buf_pull(os_buf_t *buf, unsigned int len);
int os_buf_trim(os_buf_t *buf, int len);

OS_END_EXTERN_C

#endif
