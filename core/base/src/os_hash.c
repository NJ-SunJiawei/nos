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

#include "os_init.h"

typedef struct os_hash_entry_t os_hash_entry_t;
struct os_hash_entry_t {
    os_hash_entry_t    *next;
    unsigned int        hash;
    const void          *key;
    int                 klen;
    const void          *val;
};

struct os_hash_index_t {
    os_hash_t          *ht;
    os_hash_entry_t    *this, *next;
    unsigned int        index;
};

struct os_hash_t {
    os_hash_entry_t    **array;
    os_hash_index_t    iterator;  /* For os_hash_first(NULL, ...) */
    unsigned int        count, max, seed;
    os_hashfunc_t      hash_func;
    os_hash_entry_t    *free;  /* List of recycled entries */
};

#define INITIAL_MAX 15 /* tunable == 2^n - 1 */

PRIVATE os_hash_entry_t **alloc_array(os_hash_t *ht, unsigned int max)
{
    os_hash_entry_t **ptr = os_calloc(1, sizeof(*ht->array) * (max + 1));
    os_assert(ptr);
    return ptr;
}

os_hash_t *os_hash_make(void)
{
    os_hash_t *ht;
    os_time_t now = os_get_monotonic_time();

    ht = os_malloc(sizeof(os_hash_t));
    if (!ht) {
        os_log0(ERROR, "os_malloc() failed");
        return NULL;
    }

    ht->free = NULL;
    ht->count = 0;
    ht->max = INITIAL_MAX;
    ht->seed = (unsigned int)((now >> 32) ^ now ^ 
                              (uintptr_t)ht ^ (uintptr_t)&now) - 1;
    ht->array = alloc_array(ht, ht->max);
    ht->hash_func = NULL;

    return ht;
}

os_hash_t *os_hash_make_custom(os_hashfunc_t hash_func)
{
    os_hash_t *ht = os_hash_make();
    if (!ht) {
        os_log0(ERROR, "os_hash_make() failed");
        return NULL;
    }
    ht->hash_func = hash_func;
    return ht;
}

void os_hash_destroy(os_hash_t *ht)
{
    os_hash_entry_t *he = NULL, *next_he = NULL;

    os_assert(ht);
    os_assert(ht->array);

    os_hash_clear(ht);

    he = ht->free;
    while(he) {
        next_he = he->next;

        os_free(he);
        he = next_he;
    }

    os_free(ht->array);
    os_free(ht);
}

os_hash_index_t *os_hash_next(os_hash_index_t *hi)
{
    os_assert(hi);

    hi->this = hi->next;
    while (!hi->this) {
        if (hi->index > hi->ht->max)
            return NULL;

        hi->this = hi->ht->array[hi->index++];
    }
    hi->next = hi->this->next;
    return hi;
}

os_hash_index_t *os_hash_first(os_hash_t *ht)
{
    os_hash_index_t *hi;

    os_assert(ht);

    hi = &ht->iterator;

    hi->ht = ht;
    hi->index = 0;
    hi->this = NULL;
    hi->next = NULL;
    return os_hash_next(hi);
}

void os_hash_this(os_hash_index_t *hi,
        const void **key, int *klen, void **val)
{
    os_assert(hi);

    if (key)  *key  = hi->this->key;
    if (klen) *klen = hi->this->klen;
    if (val)  *val  = (void *)hi->this->val;
}

const void *os_hash_this_key(os_hash_index_t *hi)
{
    const void *key;

    os_hash_this(hi, &key, NULL, NULL);
    return key;
}

int os_hash_this_key_len(os_hash_index_t *hi)
{
    int klen;

    os_hash_this(hi, NULL, &klen, NULL);
    return klen;
}

void *os_hash_this_val(os_hash_index_t *hi)
{
    void *val;

    os_hash_this(hi, NULL, NULL, &val);
    return val;
}

PRIVATE void expand_array(os_hash_t *ht)
{
    os_hash_index_t *hi;
    os_hash_entry_t **new_array;
    unsigned int new_max;

    new_max = ht->max * 2 + 1;
    new_array = alloc_array(ht, new_max);
    for (hi = os_hash_first(ht); hi; hi = os_hash_next(hi)) {
        unsigned int i = hi->this->hash & new_max;
        hi->this->next = new_array[i];
        new_array[i] = hi->this;
    }
    os_free(ht->array);
    ht->array = new_array;
    ht->max = new_max;
}

PRIVATE unsigned int hashfunc_default(
        const char *char_key, int *klen, unsigned int hash)
{
    const unsigned char *key = (const unsigned char *)char_key;
    const unsigned char *p;
    int i;
    
    /*
     * This is the popular `times 33' hash algorithm which is used by
     * perl and also appears in Berkeley DB. This is one of the best
     * known hash functions for strings because it is both computed
     * very fast and distributes very well.
     *
     * The originator may be Dan Bernstein but the code in Berkeley DB
     * cites Chris Torek as the source. The best citation I have found
     * is "Chris Torek, Hash function for text in C, Usenet message
     * <27038@mimsy.umd.edu> in comp.lang.c , October, 1990." in Rich
     * Salz's USENIX 1992 paper about INN which can be found at
     * <http://citeseer.nj.nec.com/salz92internetnews.html>.
     *
     * The magic of number 33, i.e. why it works better than many other
     * constants, prime or not, has never been adequately explained by
     * anyone. So I try an explanation: if one experimentally tests all
     * multipliers between 1 and 256 (as I did while writing a low-level
     * data structure library some time ago) one detects that even
     * numbers are not useable at all. The remaining 128 odd numbers
     * (except for the number 1) work more or less all equally well.
     * They all distribute in an acceptable way and this way fill a hash
     * table with an average percent of approx. 86%.
     *
     * If one compares the chi^2 values of the variants (see
     * Bob Jenkins ``Hashing Frequently Asked Questions'' at
     * http://burtleburtle.net/bob/hash/hashfaq.html for a description
     * of chi^2), the number 33 not even has the best value. But the
     * number 33 and a few other equally good numbers like 17, 31, 63,
     * 127 and 129 have nevertheless a great advantage to the remaining
     * numbers in the large set of possible multipliers: their multiply
     * operation can be replaced by a faster operation based on just one
     * shift plus either a single addition or subtraction operation. And
     * because a hash function has to both distribute good _and_ has to
     * be very fast to compute, those few numbers should be preferred.
     *
     *                  -- Ralf S. Engelschall <rse@engelschall.com>
     */

    if (*klen == OS_HASH_KEY_STRING) {
        for (p = key; *p; p++) {
            hash = hash * 33 + *p;
        }
        *klen = p - key;
    }
    else {
        for (p = key, i = *klen; i; i--, p++) {
            hash = hash * 33 + *p;
        }
    }

    return hash;
}

unsigned int os_hashfunc_default(const char *char_key, int *klen)
{
    return hashfunc_default(char_key, klen, 0);
}

PRIVATE os_hash_entry_t **find_entry(os_hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    os_hash_entry_t **hep, *he;
    unsigned int hash;

    if (ht->hash_func)
        hash = ht->hash_func(key, &klen);
    else
        hash = hashfunc_default(key, &klen, ht->seed);

    /* scan linked list */
    for (hep = &ht->array[hash & ht->max], he = *hep;
         he; hep = &he->next, he = *hep) {
        if (he->hash == hash
            && he->klen == klen
            && memcmp(he->key, key, klen) == 0)
            break;
    }
    if (he || !val)
        return hep;

    /* add a new entry for non-NULL values */
    if ((he = ht->free) != NULL)
        ht->free = he->next;
    else {
        he = os_malloc(sizeof(*he));
        os_assert(he);
    }
    he->next = NULL;
    he->hash = hash;
    he->key  = key;
    he->klen = klen;
    he->val  = val;
    *hep = he;
    ht->count++;
    return hep;
}

void *os_hash_get_debug(os_hash_t *ht,
        const void *key, int klen, const char *file_line)
{
    os_hash_entry_t *he;

    os_assert(ht);
    os_assert(key);
    os_assert(klen);

    he = *find_entry(ht, key, klen, NULL, file_line);
    if (he)
        return (void *)he->val;
    else
        return NULL;
}

void os_hash_set_debug(os_hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    os_hash_entry_t **hep;

    os_assert(ht);
    os_assert(key);
    os_assert(klen);

    hep = find_entry(ht, key, klen, val, file_line);
    if (*hep) {
        if (!val) {
            /* delete entry */
            os_hash_entry_t *old = *hep;
            *hep = (*hep)->next;
            old->next = ht->free;
            ht->free = old;
            --ht->count;
        } else {
            /* replace entry */
            (*hep)->val = val;
            /* check that the collision rate isn't too high */
            if (ht->count > ht->max) {
                expand_array(ht);
            }
        }
    }
    /* else key not present and val==NULL */
}

void *os_hash_get_or_set_debug(os_hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    os_hash_entry_t **hep;

    os_assert(ht);
    os_assert(key);
    os_assert(klen);

    hep = find_entry(ht, key, klen, val, file_line);
    if (*hep) {
        val = (*hep)->val;
        /* check that the collision rate isn't too high */
        if (ht->count > ht->max) {
            expand_array(ht);
        }
        return (void *)val;
    }
    /* else key not present and val==NULL */
    return NULL;
}

unsigned int os_hash_count(os_hash_t *ht)
{
    os_assert(ht);
    return ht->count;
}

void os_hash_clear(os_hash_t *ht)
{
    os_hash_index_t *hi;

    os_assert(ht);

    for (hi = os_hash_first(ht); hi; hi = os_hash_next(hi))
        os_hash_set(ht, hi->this->key, hi->this->klen, NULL);
}

/* This is basically the following...
 * for every element in hash table {
 *    comp elemeny.key, element.value
 * }
 *
 * Like with table_do, the comp callback is called for each and every
 * element of the hash table.
 */
int os_hash_do(os_hash_do_callback_fn_t *comp,
                             void *rec, const os_hash_t *ht)
{
    os_hash_index_t  hix;
    os_hash_index_t *hi;
    int rv, dorv  = 1;

    hix.ht    = (os_hash_t *)ht;
    hix.index = 0;
    hix.this  = NULL;
    hix.next  = NULL;

    if ((hi = os_hash_next(&hix))) {
        /* Scan the entire table */
        do {
            rv = (*comp)(rec, hi->this->key, hi->this->klen, hi->this->val);
        } while (rv && (hi = os_hash_next(hi)));

        if (rv == 0) {
            dorv = 0;
        }
    }
    return dorv;
}
