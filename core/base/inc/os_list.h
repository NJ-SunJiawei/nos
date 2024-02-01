/************************************************************************
 *File name: os_list.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_LIST_H
#define OS_LIST_H

OS_BEGIN_EXTERN_C

struct os_list_s {
    struct os_list_s *prev, *next;
};

typedef struct os_list_s os_list_t;
typedef struct os_list_s os_lnode_t;

#define OS_LIST(name) \
    os_list_t name = { NULL, NULL }

#define os_list_init(list) do { \
    (list)->prev = (NULL); \
    (list)->next = (NULL); \
} while (0)

#define os_list_copy(dst, src) do { \
    (dst)->prev = (src)->prev; \
    (dst)->next = (src)->next; \
} while (0)

#define os_list_entry(ptr, type, member) \
    ptr ? os_container_of(ptr, type, member) : NULL

#define os_list_for_each(list, node) \
    for (node = os_list_first(list); (node); \
        node = os_list_next(node))

#define os_list_reverse_for_each(list, node) \
    for (node = os_list_last(list); (node); \
        node = os_list_prev(node))

#define os_list_for_each_entry(list, node, member) \
    for (node = os_list_entry(os_list_first(list), typeof(*node), member); \
            (node) && (&node->member); \
                node = os_list_entry( \
                        os_list_next(&node->member), typeof(*node), member))

#define os_list_for_each_safe(list, n, node) \
    for (node = os_list_first(list); \
        (node) && (n = os_list_next(node), 1); \
        node = n)

#define os_list_for_each_entry_safe(list, n, node, member) \
    for (node = os_list_entry(os_list_first(list), typeof(*node), member); \
            (node) && (&node->member) && \
                (n = os_list_entry( \
                    os_list_next(&node->member), typeof(*node), member), 1); \
            node = n)

void *os_list_first(const os_list_t *list);
void *os_list_last(const os_list_t *list);
void *os_list_next(void *lnode);
void *os_list_prev(void *lnode);
void os_list_prepend(os_list_t *list, void *lnode);
void os_list_add(os_list_t *list, void *lnode);
void os_list_remove(os_list_t *list, void *lnode);
void os_list_insert_prev(os_list_t *list, void *lnext, void *lnode);
void os_list_insert_next(os_list_t *list, void *lprev, void *lnode);

typedef int (*os_list_compare_f)(os_lnode_t *n1, os_lnode_t *n2);
void __os_list_insert_sorted(os_list_t *list, void *lnode, os_list_compare_f compare);
#define os_list_insert_sorted(__list, __lnode, __compare) __os_list_insert_sorted(__list, __lnode, (os_list_compare_f)__compare);

bool os_list_empty(const os_list_t *list);
int os_list_count(const os_list_t *list);

OS_END_EXTERN_C

#endif
