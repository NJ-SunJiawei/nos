/************************************************************************
 *File name: os_list.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

void *os_list_first(const os_list_t *list)
{
    return list->next;
}

void *os_list_last(const os_list_t *list)
{
    return list->prev;
}

void *os_list_next(void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;
    return node->next;
}

void *os_list_prev(void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;
    return node->prev;
}

void os_list_prepend(os_list_t *list, void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;

    node->prev = NULL;
    node->next = list->next;
    if (list->next)
        list->next->prev = node;
    else
        list->prev = node;
    list->next = node;
}

void os_list_add(os_list_t *list, void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;

    node->prev = list->prev;
    node->next = NULL;
    if (list->prev)
        list->prev->next = node;
    else
        list->next = node;
    list->prev = node;
}

void os_list_remove(os_list_t *list, void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;
    os_list_t *prev = node->prev;
    os_list_t *next = node->next;

    if (prev)
        prev->next = next;
    else
        list->next = next;

    if (next)
        next->prev = prev;
    else
        list->prev = prev;
}

void os_list_insert_prev(os_list_t *list, void *lnext, void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;
    os_list_t *next = (os_list_t *)lnext;

    node->prev = next->prev;
    node->next = next;
    if (next->prev)
        next->prev->next = node;
    else
        list->next = node;
    next->prev = node;
}

void os_list_insert_next(os_list_t *list, void *lprev, void *lnode)
{
    os_list_t *node = (os_list_t *)lnode;
    os_list_t *prev = (os_list_t *)lprev;

    node->prev = prev;
    node->next = prev->next;
    if (prev->next)
        prev->next->prev = node;
    else
        list->prev = node;
    prev->next = node;
}

void __os_list_insert_sorted(os_list_t *list, void *lnode, os_list_compare_f compare)
{
    os_list_t *node = (os_list_t *)lnode;
    void *iter = NULL;

    os_list_for_each(list, iter) {
        if ((*compare)(node, (os_list_t *)iter) < 0) {
            os_list_insert_prev(list, iter, node);
            break;
        }
    }

    if (iter == NULL)
        os_list_add(list, node);
}

bool os_list_empty(const os_list_t *list)
{
    return list->next == NULL;
}

int os_list_count(const os_list_t *list)
{
    void *node;
    int i = 0;
    os_list_for_each(list, node)
        i++;
    return i;
}