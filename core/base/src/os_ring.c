/************************************************************************
 *File name: os_ring.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/

#include "os_init.h"

/*****support ring queue***********/

typedef struct os_item_s {
    unsigned char *data;
	unsigned int  len;
} os_item_t;

typedef struct os_ring_queue_s {
   size_t    que_size;
   unsigned long long ic;
   unsigned long long oc;
   size_t    head;
   size_t    tail;
   os_item_t *pkts;
   unsigned int 	   full_waiters;
   unsigned int 	   empty_waiters;
   os_thread_mutex_t  cs;
   os_thread_cond_t   not_empty;
   os_thread_cond_t   not_full;
   int       terminated;
} os_ring_queue_t;

os_ring_queue_t *os_ring_queue_create(unsigned int size)
{

    os_ring_queue_t *rque = NULL;
    os_assert(size);

	rque = malloc(sizeof(os_ring_queue_t));
	os_assert(rque);

	rque->que_size = size;

	rque->pkts = (os_item_t *)malloc((size + 1) * sizeof(os_item_t));
	if(rque->pkts == NULL)
	{
		free(rque);
		return NULL;
	}

    rque->ic = 0;
    rque->oc = 0;	
    rque->head = 0;		
    rque->tail = 0;	
    rque->full_waiters = 0;		
    rque->empty_waiters = 0;	
	rque->terminated = 0;
    os_thread_cond_init(&rque->not_empty);
    os_thread_cond_init(&rque->not_full);
    os_thread_mutex_init(&rque->cs);
    return rque;
}

int os_ring_queue_try_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size)
{
	return ring_queue_put(rque, data, size, 0);
}

int os_ring_queue_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size)
{
	return ring_queue_put(rque, data, size, OS_INFINITE_TIME);
}

int os_ring_queue_time_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size, os_time_t timeout)
{
	return ring_queue_put(rque, data, size, timeout);
}

int ring_queue_put(os_ring_queue_t *rque, unsigned char *data, unsigned int size, os_time_t timeout)
{
    size_t tmp;
    int rv = OS_ERROR;

	if(rque == NULL) return OS_ERROR;
	if((data == NULL) || (size <= 0))  return OS_ERROR;

	os_thread_mutex_lock(&rque->cs);
	tmp = rque->tail + 1;
	if(tmp == rque->que_size + 1) tmp = 0;

    if (tmp == rque->head) {
        if (!timeout) {
            os_thread_mutex_unlock(&rque->cs);
            return OS_RETRY;
        }
        if (!rque->terminated) {
            rque->full_waiters++;
            if (timeout > 0) {
                rv = os_thread_cond_timedwait(&rque->not_full,
                                               &rque->cs,
                                               timeout);
            }
            else {
                rv = os_thread_cond_wait(&rque->not_full,
                                          &rque->cs);
            }
            rque->full_waiters--;
            if (rv != OS_OK) {
                os_thread_mutex_unlock(&rque->cs);
                return rv;
            }
        }
    }

	if(tmp == rque->head){
       	os_log(WARN, "rqueue FULL!");
	    os_thread_mutex_unlock(&rque->cs);
		if (rque->terminated) {
			return OS_DONE; /* no more elements ever again */
		}
		else {
			return OS_ERROR;
		}

	}else{
        (rque->pkts + rque->tail)->data = data;
		(rque->pkts + rque->tail)->len = size;
		rque->tail = tmp;
		++rque->ic;
		if (rque->empty_waiters) {
			os_log(TRACE, "signal empty!");
			os_thread_cond_signal(&rque->not_empty);
		}
	    os_thread_mutex_unlock(&rque->cs);		
		return OS_OK;
	}

}

int os_ring_queue_try_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size)
{
	return ring_queue_get(rque, ptr, size, 0);
}

int os_ring_queue_time_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size, os_time_t timeout)
{
	return ring_queue_get(rque, ptr, size, timeout);
}

int os_ring_queue_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size)
{
	return ring_queue_get(rque, ptr, size, OS_INFINITE_TIME);
}

int ring_queue_get(os_ring_queue_t *rque, unsigned char **ptr, unsigned int *size, os_time_t timeout)
{
    int rv = OS_ERROR;

	if((rque == NULL) || (ptr == NULL) || (size <= 0)){
		return OS_ERROR;
	}

    //terminate and stop all
    if (rque->terminated){
        return OS_DONE;
    }

	os_thread_mutex_lock(&rque->cs);
    if (rque->head == rque->tail) {
        if (!timeout) {
            os_thread_mutex_unlock(&rque->cs);
            return OS_RETRY;
        }
        if (!rque->terminated) {
            rque->empty_waiters++;
            if (timeout > 0) {
                rv = os_thread_cond_timedwait(&rque->not_empty,
                                               &rque->cs,
                                               timeout);
            }
            else {
                rv = os_thread_cond_wait(&rque->not_empty,
                                          &rque->cs);
            }
            rque->empty_waiters--;
            if (rv != OS_OK) {
                os_thread_mutex_unlock(&rque->cs);
                return rv;
            }
        }
    } 

    if (rque->head == rque->tail) {
        //os_log(WARN, "rqueue EMPTY!");
        os_thread_mutex_unlock(&rque->cs);
        *ptr = NULL;
	    size = 0;
        if (rque->terminated) {
            return OS_DONE;
        } else {
            return OS_ERROR;
        }     
    }
	else{
        *ptr = (rque->pkts+rque->head)->data;
		*size = (rque->pkts+rque->head)->len;
		++rque->head;
		if(rque->head == rque->que_size + 1) rque->head = 0;
		++rque->oc;
		if (rque->full_waiters) {
			os_log(TRACE, "signal full!");
			os_thread_cond_signal(&rque->not_full);
		}
	    os_thread_mutex_unlock(&rque->cs);		
		return OS_OK;		
	}   
}

int os_ring_queue_destroy(os_ring_queue_t *rque)
{
    if(rque == NULL) return OS_OK;

    if(rque->pkts != NULL)	free(rque->pkts);

    os_thread_cond_destroy(&rque->not_empty);
    os_thread_cond_destroy(&rque->not_full);
    os_thread_mutex_destroy(&rque->cs);
    free(rque);

	return OS_OK;
}

int os_ring_queue_interrupt_all(os_ring_queue_t *rque)
{
    os_log(DEBUG, "interrupt all");
    os_thread_mutex_lock(&rque->cs);

    os_thread_cond_broadcast(&rque->not_empty);
    os_thread_cond_broadcast(&rque->not_full);

    os_thread_mutex_unlock(&rque->cs);

    return OS_OK;
}


int os_ring_queue_term(os_ring_queue_t *rque)
{
    os_thread_mutex_lock(&rque->cs);
    rque->terminated = 1;
    os_thread_mutex_unlock(&rque->cs);

    return os_ring_queue_interrupt_all(rque);
}


void os_ring_queue_show(os_ring_queue_t *rque)
{
    if(rque == NULL) return;
	char tmp[256] = {0};
	sprintf(tmp, "%p,size=%ld,ic=%lld,oc=%lld,backlog rate[%lld%%]",rque,rque->que_size,rque->ic,rque->oc,(rque->ic-rque->oc)/rque->que_size);
	fprintf(stderr, "ring que : %s!\n", tmp);
}

/*****support ring buf***********/
typedef struct os_ring_buf_s {
   size_t    buf_size;
   size_t    buf_unit;
   unsigned char *buf;
   unsigned long long ic;
   unsigned long long oc;
   size_t    head;
   size_t    tail;
   unsigned int *rque;
   os_thread_mutex_t  cs;
} os_ring_buf_t;

os_ring_buf_t *os_ring_buf_create(unsigned int count, unsigned int size)
{
    os_ring_buf_t *rbuf = NULL;

    os_assert(count);
    os_assert(size);

	rbuf = malloc(sizeof(os_ring_buf_t));
	os_assert(rbuf);

	rbuf->buf_size = count;
	rbuf->buf_unit = size + sizeof(void*);

	rbuf->buf = (unsigned char *)malloc(rbuf->buf_size * rbuf->buf_unit);
	if(rbuf->buf == NULL)
	{
		free(rbuf);
		return NULL;
	}

	rbuf->rque = (unsigned int *)malloc((count + 1) * sizeof(unsigned int));
	if(rbuf->rque == NULL)
	{
		free(rbuf->buf);
		free(rbuf);
		return NULL;
	}

    rbuf->ic = 0;
    rbuf->oc = 0;	
    rbuf->head = 0;		
    rbuf->tail = count;	
    for(unsigned i = 0; i < count; i++){
       *(os_ring_buf_t **)(rbuf->buf+i*rbuf->buf_unit) = NULL;
	   rbuf->rque[i] = i;
	}
	rbuf->rque[count] = -1;

    os_thread_mutex_init(&rbuf->cs);
    return rbuf;
}

// char *addr;
// plink = (void **)addr;      
// addr本来是一个一维指针，指向所申请内存分区的起始地址。本句将addr强制转换为二维指针(注意经过强制转换后addr指针本身指向的地址是没有变化的)，并将addr地址值赋给plink。
// 则plink的内存中存储的是addr的值，即plink也指向addr所指向的分区起始地址，并且这个地址内存放的内容是指针(*plink)，但指针(*plink)还未指向具体的地址。
unsigned char *os_ring_buf_get(os_ring_buf_t *rbuf)
{
    size_t idx;
    unsigned char *blk;

	if(rbuf == NULL) return NULL;

	os_thread_mutex_lock(&rbuf->cs);
    if(rbuf->head == rbuf->tail)
    {
	    os_thread_mutex_unlock(&rbuf->cs);
        os_log(ERROR, "rbuf EMPTY!");
		return NULL;        
    }else{
        idx = rbuf->head++;
		if(rbuf->head == rbuf->buf_size + 1) rbuf->head = 0;
		++rbuf->oc;
		blk = rbuf->buf + *(rbuf->rque+idx) * rbuf->buf_unit;
		//memcpy(blk, &rbuf, sizeof(void*));
		*(os_ring_buf_t**)blk = rbuf;  //sizeof(*) store struct os_ring_buf_t
		blk += sizeof(void*);
	    os_thread_mutex_unlock(&rbuf->cs);	
		return blk;		
	}   
}


unsigned int os_ring_buf_ret(unsigned char *blk)
{
	os_ring_buf_t *rbuf = NULL;
    unsigned int idx;
	size_t tmp;


	if(blk == NULL) return OS_ERROR;

    //memcpy(&rbuf, (unsigned char *)blk - sizeof(void*), sizeof(void*));
	rbuf = *(os_ring_buf_t**)(blk - sizeof(void*));
	if(rbuf == NULL) return OS_ERROR;

	idx = (int)((blk - rbuf->buf)/(rbuf->buf_unit));
	if((idx < 0) || (idx > rbuf->buf_size))  return OS_ERROR;

	os_thread_mutex_lock(&rbuf->cs);
	tmp = rbuf->tail + 1;
	if(tmp == rbuf->buf_size + 1) tmp = 0;
	if(tmp == rbuf->head){
	    os_thread_mutex_unlock(&rbuf->cs);
        os_log(WARN, "rbuf FULL!");
		return OS_ERROR; 		
	}else{
	    *(rbuf->rque + rbuf->tail) = idx;
		rbuf->tail = tmp;
		++rbuf->ic;
		*(os_ring_buf_t**)(blk - sizeof(void*)) = NULL;
	    os_thread_mutex_unlock(&rbuf->cs);		
		return 0;
	}
 
}

int os_ring_buf_destroy(os_ring_buf_t *rbuf)
{
    if(rbuf == NULL) return OS_OK;

	if(rbuf->buf != NULL)  free(rbuf->buf);
	if(rbuf->rque != NULL)	free(rbuf->rque);

	os_thread_mutex_destroy(&rbuf->cs);
	free(rbuf);

	return OS_OK;
}

void os_ring_buf_show(os_ring_buf_t *rbuf)
{
    if(rbuf == NULL) return;
	char tmp[256] = {0};
	sprintf(tmp, "%p,size=%ld,unit=%ld,ic=%lld,oc=%lld,used rate[%lld%%]",rbuf,rbuf->buf_unit,rbuf->buf_size,rbuf->ic,rbuf->oc,(rbuf->oc-rbuf->ic)/rbuf->buf_size);
	fprintf(stderr, "ring buf : %s!\n", tmp);
}


/*****support new hash table***********/
typedef struct os_hash_node_s {
	struct os_hash_node_s *prev;
	struct os_hash_node_s *next;
	void      *data;
} os_hash_node_t;

typedef struct os_hash_table_s {
    unsigned int size;
	unsigned int num;
	os_ring_buf_t  databuf;
	os_ring_buf_t  nidebuf;
	os_hash_node_t **tab;
	os_thread_mutex_t  cs;
} os_hash_table_t;


