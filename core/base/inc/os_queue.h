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
 *File name: os_queue.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/

#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct os_queue_s os_queue_t;

os_queue_t *os_queue_create(unsigned int capacity);
void os_queue_destroy(os_queue_t *queue);

int os_queue_push(os_queue_t *queue, void *data);
int os_queue_pop(os_queue_t *queue, void **data);

int os_queue_trypush(os_queue_t *queue, void *data);
int os_queue_trypop(os_queue_t *queue, void **data);

int os_queue_timedpush(os_queue_t *queue, void *data, os_time_t timeout);
int os_queue_timedpop(os_queue_t *queue, void **data, os_time_t timeout);

unsigned int os_queue_size(os_queue_t *queue);

int os_queue_interrupt_all(os_queue_t *queue);
int os_queue_term(os_queue_t *queue);
void* os_queue_pop_peek(os_queue_t *queue);

#ifdef __cplusplus
    }
#endif

#endif /* OS_QUEUE_H */
