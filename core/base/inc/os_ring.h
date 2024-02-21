/************************************************************************
 *File name: os_ring.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/

#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_RING_H
#define OS_RING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct os_ring_queue_s os_ring_queue_t;

os_ring_queue_t *os_ring_queue_create(unsigned int size);
int os_ring_queue_try_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size);
int os_ring_queue_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size);
int os_ring_queue_time_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size, os_time_t timeout);
int ring_queue_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size, os_time_t timeout);

int os_ring_queue_try_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size);
int os_ring_queue_time_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size, os_time_t timeout);
int os_ring_queue_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size);
int ring_queue_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size, os_time_t timeout);
int os_ring_queue_destroy(os_ring_queue_t *rque);
int os_ring_queue_interrupt_all(os_ring_queue_t *rque);
int os_ring_queue_term(os_ring_queue_t *rque);

typedef struct os_ring_buf_s os_ring_buf_t;

os_ring_buf_t *os_ring_buf_create(unsigned int count, unsigned int size);
unsigned char *os_ring_buf_get(os_ring_buf_t *rbuf);
unsigned int os_ring_buf_ret(unsigned char *blk);
int os_ring_buf_destroy(os_ring_buf_t *rbuf);
void os_ring_buf_show(os_ring_buf_t *rbuf);
void os_ring_queue_show(os_ring_queue_t *rque);

#ifdef __cplusplus
	}
#endif

#endif /* OS_QUEUE_H */
