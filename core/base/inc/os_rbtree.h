/************************************************************************
 *File name: os_rbtree.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_RBTREE_H
#define OS_RBTREE_H

OS_BEGIN_EXTERN_C

typedef enum {
    OS_RBTREE_BLACK = 0,
    OS_RBTREE_RED = 1,
} os_rbtree_color_e;

typedef struct os_rbnode_s {
    struct os_rbnode_s *parent;
    struct os_rbnode_s *left;
    struct os_rbnode_s *right;

    os_rbtree_color_e color;
} os_rbnode_t;

typedef struct os_rbtree_s {
    os_rbnode_t *root;
} os_rbtree_t;

#define OS_RBTREE(name) os_rbtree_t name = { NULL }

#define os_rb_entry(ptr, type, member) os_container_of(ptr, type, member)

void os_rbtree_link_node(void *rb_node, os_rbnode_t *parent, os_rbnode_t **rb_link);
void os_rbtree_insert_color(os_rbtree_t *tree, void *rb_node);
void os_rbtree_delete(os_rbtree_t *tree, void *rb_node);

void *os_rbtree_min(const os_rbnode_t *rb_node);
void *os_rbtree_max(const void *rb_node);

void *os_rbtree_first(const os_rbtree_t *tree);
void *os_rbtree_next(const void *node);
void *os_rbtree_last(const os_rbtree_t *tree);
void *os_rbtree_prev(const void *node);

#define os_rbtree_for_each(tree, node) \
    for (node = os_rbtree_first(tree); \
        (node); node = os_rbtree_next(node))

#define os_rbtree_reverse_for_each(tree, node) \
    for (node = os_rbtree_last(tree); \
        (node); node = os_rbtree_prev(node))

bool os_rbtree_empty(const os_rbtree_t *tree);
int os_rbtree_count(const os_rbtree_t *tree);

OS_END_EXTERN_C

#endif
