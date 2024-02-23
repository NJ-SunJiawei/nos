/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/************************************************************************
 *File name: os_queue.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/

#include "os_init.h"


typedef struct os_queue_s {
    void              **data;
    unsigned int        nelts; /**< # elements */
    unsigned int        in;    /**< next empty location */
    unsigned int        out;   /**< next filled location */
    unsigned int        bounds;/**< max size of queue */
    unsigned int        full_waiters;
    unsigned int        empty_waiters;
    os_thread_mutex_t  one_big_mutex;
    os_thread_cond_t   not_empty;
    os_thread_cond_t   not_full;
    int                 terminated;
} os_queue_t;

/**
 * Detects when the os_queue_t is full. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define os_queue_full(queue) ((queue)->nelts == (queue)->bounds)

/**
 * Detects when the os_queue_t is empty. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define os_queue_empty(queue) ((queue)->nelts == 0)

/**
 * Callback routine that is called to destroy this
 * os_queue_t when its pool is destroyed.
 */
os_queue_t *os_queue_create(unsigned int capacity)
{
    os_queue_t *queue = (os_queue_t *)calloc(1, sizeof *queue);
    os_expect_or_return_val(queue, NULL);
    os_assert(queue);

    os_thread_mutex_init(&queue->one_big_mutex);
    os_thread_cond_init(&queue->not_empty);
    os_thread_cond_init(&queue->not_full);

    queue->data = calloc(1, capacity * sizeof(void*));
    os_expect_or_return_val(queue->data, NULL);
    queue->bounds = capacity;
    queue->nelts = 0;
    queue->in = 0;
    queue->out = 0;
    queue->terminated = 0;
    queue->full_waiters = 0;
    queue->empty_waiters = 0;

    return queue;
}

void os_queue_destroy(os_queue_t *queue)
{
    os_assert(queue);

    free(queue->data);

    os_thread_cond_destroy(&queue->not_empty);
    os_thread_cond_destroy(&queue->not_full);
    os_thread_mutex_destroy(&queue->one_big_mutex);

    free(queue);
}

static int queue_push(os_queue_t *queue, void *data, os_time_t timeout)
{
    int rv;

    if (queue->terminated) {
        return OS_DONE; /* no more elements ever again */
    }

    os_thread_mutex_lock(&queue->one_big_mutex);

    if (os_queue_full(queue)) {
        if (!timeout) {
            os_thread_mutex_unlock(&queue->one_big_mutex);
            return OS_RETRY;
        }
        if (!queue->terminated) {
            queue->full_waiters++;
            if (timeout > 0) {
                rv = os_thread_cond_timedwait(&queue->not_full,
                                               &queue->one_big_mutex,
                                               timeout);
            }
            else {
                rv = os_thread_cond_wait(&queue->not_full,
                                          &queue->one_big_mutex);
            }
            queue->full_waiters--;
            if (rv != OS_OK) {
                os_thread_mutex_unlock(&queue->one_big_mutex);
                return rv;
            }
        }
        /* If we wake up and it's still empty, then we were interrupted */
        if (os_queue_full(queue)) {
            os_log(WARN, "queue full (intr)");
            os_thread_mutex_unlock(&queue->one_big_mutex);
            if (queue->terminated) {
                return OS_DONE; /* no more elements ever again */
            }
            else {
                return OS_ERROR;
            }
        }
    }

    queue->data[queue->in] = data;
    queue->in++;
    if (queue->in >= queue->bounds)
        queue->in -= queue->bounds;
    queue->nelts++;

    if (queue->empty_waiters) {
        os_log(TRACE, "signal !empty");
        os_thread_cond_signal(&queue->not_empty);
    }

    os_thread_mutex_unlock(&queue->one_big_mutex);
    return OS_OK;
}

int os_queue_push(os_queue_t *queue, void *data)
{
    return queue_push(queue, data, OS_INFINITE_TIME);
}

/**
 * Push new data onto the queue. If the queue is full, return OS_RETRY. If
 * the push operation completes successfully, it signals other threads
 * waiting in os_queue_pop() that they may continue consuming sockets.
 */
int os_queue_trypush(os_queue_t *queue, void *data)
{
    return queue_push(queue, data, 0);
}

int os_queue_timedpush(os_queue_t *queue, void *data, os_time_t timeout)
{
    return queue_push(queue, data, timeout);
}

/**
 * not thread safe
 */
unsigned int os_queue_size(os_queue_t *queue) {
    return queue->nelts;
}

/**
 * Retrieves the next item from the queue. If there are no
 * items available, it will either return OS_RETRY (timeout = 0),
 * or block until one becomes available (infinitely with timeout < 0,
 * otherwise until the given timeout expires). Once retrieved, the
 * item is placed into the address specified by 'data'.
 */
static int queue_pop(os_queue_t *queue, void **data, os_time_t timeout)
{
    int rv;

    if (queue->terminated) {
        return OS_DONE; /* no more elements ever again */
    }

    os_thread_mutex_lock(&queue->one_big_mutex);

    /* Keep waiting until we wake up and find that the queue is not empty. */
    if (os_queue_empty(queue)) {
        if (!timeout) {
            os_thread_mutex_unlock(&queue->one_big_mutex);
            return OS_RETRY;
        }
        if (!queue->terminated) {
            queue->empty_waiters++;
            if (timeout > 0) {
                rv = os_thread_cond_timedwait(&queue->not_empty,
                                               &queue->one_big_mutex,
                                               timeout);
            }
            else {
                rv = os_thread_cond_wait(&queue->not_empty,
                                          &queue->one_big_mutex);
            }
            queue->empty_waiters--;
            if (rv != OS_OK) {
                os_thread_mutex_unlock(&queue->one_big_mutex);
                return rv;
            }
        }
        /* If we wake up and it's still empty, then we were interrupted */
        if (os_queue_empty(queue)) {
            //os_log(WARN, "queue empty (intr)");
            os_thread_mutex_unlock(&queue->one_big_mutex);
            if (queue->terminated) {
                return OS_DONE; /* no more elements ever again */
            } else {
                return OS_ERROR;
            }
        }
    } 

    *data = queue->data[queue->out];
    queue->nelts--;

    queue->out++;
    if (queue->out >= queue->bounds)
        queue->out -= queue->bounds;
    if (queue->full_waiters) {
        os_log(TRACE, "signal !full");
        os_thread_cond_signal(&queue->not_full);
    }

    os_thread_mutex_unlock(&queue->one_big_mutex);
    return OS_OK;
}

int os_queue_pop(os_queue_t *queue, void **data)
{
    return queue_pop(queue, data, OS_INFINITE_TIME);
}

int os_queue_trypop(os_queue_t *queue, void **data)
{
    return queue_pop(queue, data, 0);
}

int os_queue_timedpop(os_queue_t *queue, void **data, os_time_t timeout)
{
    return queue_pop(queue, data, timeout);
}

int os_queue_interrupt_all(os_queue_t *queue)
{
    os_log(DEBUG, "interrupt all");
    os_thread_mutex_lock(&queue->one_big_mutex);

    os_thread_cond_broadcast(&queue->not_empty);
    os_thread_cond_broadcast(&queue->not_full);

    os_thread_mutex_unlock(&queue->one_big_mutex);

    return OS_OK;
}

int os_queue_term(os_queue_t *queue)
{
    os_thread_mutex_lock(&queue->one_big_mutex);

    /* we must hold one_big_mutex when setting this... otherwise,
     * we could end up setting it and waking everybody up just after a 
     * would-be popper checks it but right before they block
     */
    queue->terminated = 1;
    os_thread_mutex_unlock(&queue->one_big_mutex);

    return os_queue_interrupt_all(queue);
}

void* os_queue_pop_peek(os_queue_t *queue)
{
    if (os_queue_empty(queue)) {
		return NULL;
	}else{
		return queue->data[queue->out];
	}
}

