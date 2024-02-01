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
 *File name: os_list.h
 *Description:
 *
 *Current Version:
 *Author: Modified by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_HASH_H
#define OS_HASH_H

OS_BEGIN_EXTERN_C

#define OS_HASH_KEY_STRING     (-1)

typedef struct os_hash_t os_hash_t;
typedef struct os_hash_index_t os_hash_index_t;
typedef unsigned int (*os_hashfunc_t)(const char *key, int *klen);
unsigned int os_hashfunc_default(const char *key, int *klen);

os_hash_t *os_hash_make(void);
os_hash_t *os_hash_make_custom(os_hashfunc_t os_hash_func);
void os_hash_destroy(os_hash_t *ht);

#define os_hash_set(ht, key, klen, val) os_hash_set_debug(ht, key, klen, val, OS_FILE_LINE)
void os_hash_set_debug(os_hash_t *ht, const void *key, int klen, const void *val, const char *file_line);

#define os_hash_get(ht, key, klen) os_hash_get_debug(ht, key, klen, OS_FILE_LINE)
void *os_hash_get_debug(os_hash_t *ht, const void *key, int klen, const char *file_line);

#define os_hash_get_or_set(ht, key, klen, val) os_hash_get_or_set_debug(ht, key, klen, val, OS_FILE_LINE)
void *os_hash_get_or_set_debug(os_hash_t *ht,const void *key, int klen, const void *val, const char *file_line);

os_hash_index_t *os_hash_first(os_hash_t *ht);
os_hash_index_t *os_hash_next(os_hash_index_t *hi);
void os_hash_this(os_hash_index_t *hi, const void **key, int *klen, void **val);

const void* os_hash_this_key(os_hash_index_t *hi);
int os_hash_this_key_len(os_hash_index_t *hi);
void* os_hash_this_val(os_hash_index_t *hi);
unsigned int os_hash_count(os_hash_t *ht);
void os_hash_clear(os_hash_t *ht);

typedef int (os_hash_do_callback_fn_t)(void *rec, const void *key, int klen, const void *value);

int os_hash_do(os_hash_do_callback_fn_t *comp, void *rec, const os_hash_t *ht);

OS_END_EXTERN_C

#endif
