/************************************************************************
 *File name: os_pool.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_POOL_H
#define OS_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int os_index_t;

#define OS_POOL(pool, type) \
	struct { \
		const char *name; \
		int head, tail, size, avail; \
		type **free, *array, **index; \
	} pool

#define os_pool_init(pool, _size) do { \
    int i; \
    (pool)->name = #pool; \
    (pool)->free = malloc(sizeof(*(pool)->free) * _size); \
    os_assert((pool)->free); \
    (pool)->array = malloc(sizeof(*(pool)->array) * _size); \
    os_assert((pool)->array); \
    (pool)->index = malloc(sizeof(*(pool)->index) * _size); \
    os_assert((pool)->index); \
    (pool)->size = (pool)->avail = _size; \
    (pool)->head = (pool)->tail = 0; \
    for (i = 0; i < _size; i++) { \
        (pool)->free[i] = &((pool)->array[i]); \
        (pool)->index[i] = NULL; \
    } \
} while (0)

#define os_pool_final(pool) do { \
    if (((pool)->size != (pool)->avail)){ \
		os_log(ERROR, "%d in '%s[%d]' were not released", (pool)->size - (pool)->avail, (pool)->name, (pool)->size); \
    } \
    free((pool)->free); \
    free((pool)->array); \
    free((pool)->index); \
} while (0)

#define os_pool_index(pool, node) (((node) - (pool)->array)+1)
#define os_pool_find(pool, _index) \
    (_index > 0 && _index <= (pool)->size) ? (pool)->index[_index-1] : NULL
#define os_pool_cycle(pool, node) \
    os_pool_find((pool), os_pool_index((pool), (node)))

#define os_pool_alloc(pool, node) do { \
    *(node) = NULL; \
    if ((pool)->avail > 0) { \
        (pool)->avail--; \
        *(node) = (void*)(pool)->free[(pool)->head]; \
        (pool)->free[(pool)->head] = NULL; \
        (pool)->head = ((pool)->head + 1) % ((pool)->size); \
        (pool)->index[os_pool_index(pool, *(node))-1] = *(node); \
    } \
} while (0)

#define os_pool_free(pool, node) do { \
    if ((pool)->avail < (pool)->size) { \
        (pool)->avail++; \
        (pool)->free[(pool)->tail] = (void*)(node); \
        (pool)->tail = ((pool)->tail + 1) % ((pool)->size); \
        (pool)->index[os_pool_index(pool, node)-1] = NULL; \
    } \
} while (0)

#define os_pool_size(pool) ((pool)->size)
#define os_pool_avail(pool) ((pool)->avail)


//type :: uint8_t
#define os_pool_order_id_generate(pool) do { \
    int n; \
    for (n = 0; n < (pool)->size; n++) \
        (pool)->array[n] = n+1; \
} while (0)

//type :: uint32_t
#define os_pool_random32_id_generate(pool) do { \
    int n, m; \
    os_index_t temp; \
    for (n = 0; n < (pool)->size; n++) \
        (pool)->array[n] = n+1; \
    for (n = (pool)->size - 1; n > 0; n--) { \
       m = os_random32() % (n + 1); \
       temp = (pool)->array[n]; \
       (pool)->array[n] = (pool)->array[m]; \
       (pool)->array[m] = temp; \
    } \
} while (0)

/////////////////////////////////////////////////////////
#define os_index_init(pool, _size) do { \
    int i; \
    (pool)->name = #pool; \
    (pool)->free = malloc(sizeof(*(pool)->free) * _size); \
    os_assert((pool)->free); \
    (pool)->array = malloc(sizeof(*(pool)->array) * _size); \
    os_assert((pool)->array); \
    (pool)->index = malloc(sizeof(*(pool)->index) * _size); \
    os_assert((pool)->index); \
    (pool)->size = (pool)->avail = _size; \
    (pool)->head = (pool)->tail = 0; \
    os_pool_order_id_generate(pool);\
    for (i = 0; i < _size; i++) { \
        (pool)->free[i] = &((pool)->array[i]); \
        (pool)->index[i] = NULL; \
    } \
} while (0)

#define os_index_final(pool) do { \
    if (((pool)->size != (pool)->avail)) { \
		os_log(ERROR, "%d in '%s[%d]' were not released", (pool)->size - (pool)->avail, (pool)->name, (pool)->size); \
    } \
    free((pool)->free); \
    free((pool)->array); \
    free((pool)->index); \
} while (0)

#ifdef __cplusplus
	}
#endif

#endif
