/************************************************************************
 *File name: os_rbtree.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "os_init.h"

PRIVATE os_inline void rb_change_child(os_rbtree_t *tree,
        os_rbnode_t *old, os_rbnode_t *new, os_rbnode_t *parent)
{
    if (parent) {
        if (old == parent->left)
            parent->left = new;
        else
            parent->right = new;
    } else {
        tree->root = new;
    }
}

PRIVATE os_inline void rb_replace_node(os_rbtree_t *tree,
        os_rbnode_t *old, os_rbnode_t *new, os_rbnode_t *parent)
{
    rb_change_child(tree, old, new, parent);

    if (new)
        new->parent = parent;
}

/*
 * Example - Left rotate at A
 *
 *       A            B 
 *      / \          / \
 *     B   C  <--   D   A
 *    / \ / \      / \ / \
 *   D  3 4  5    1  2 3  C
 *  / \                  / \
 * 1   2                1   2
 */
PRIVATE void rb_rotate_left(os_rbtree_t *tree, os_rbnode_t *node)
{
    os_rbnode_t *right = node->right;
    node->right = right->left;
    if (right->left)
        right->left->parent = node;

    rb_replace_node(tree, node, right, node->parent);

    right->left = node;
    node->parent = right;
}

/*
 * Example - right rotate at A
 *
 *       A            B 
 *      / \          / \
 *     B   C  -->   D   A
 *    / \ / \      / \ / \
 *   D  3 4  5    1  2 3  C
 *  / \                  / \
 * 1   2                1   2
 */
PRIVATE void rb_rotate_right(os_rbtree_t *tree, os_rbnode_t *node)
{
    os_rbnode_t *left = node->left;
    node->left = left->right;
    if (left->right)
        left->right->parent = node;

    rb_replace_node(tree, node, left, node->parent);

    left->right = node;
    node->parent = left;
}

void os_rbtree_insert_color(os_rbtree_t *tree, void *rb_node)
{
    os_rbnode_t *node = rb_node;
    os_rbnode_t *parent;
    os_assert(tree);
    os_assert(node);

    while ((parent = node->parent) && parent->color == OS_RBTREE_RED) {
        os_rbnode_t *gparent = parent->parent;
        os_assert(gparent);

        /* parent == grandparent's left child */
        if (parent == gparent->left) {
            os_rbnode_t *uncle = gparent->right;

            if (uncle && uncle->color == OS_RBTREE_RED) {
                /*
                 * node's uncle == red (color flips)
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 */
                uncle->color = OS_RBTREE_BLACK;
                parent->color = OS_RBTREE_BLACK;

                gparent->color = OS_RBTREE_RED;

                node = gparent;
            } else {
                /* node's uncle == black */
                if (node == parent->right) {
                    /*
                     * node == the parent's right child,
                     * (left rotate at parent)
                     *
                     *      G             G
                     *     / \           / \
                     *    p   U  -->    p   U
                     *     \           /
                     *      n         n
                     */
                    node = node->parent;
                    rb_rotate_left(tree, node);
                }

                /*
                 * Now we're the left child
                 * (right rotate at grand parent)
                 *
                 *      g           P
                 *     / \         / \
                 *    P   U  -->  n   g
                 *   /                 \
                 *  n                   U
                 */
                node->parent->color = OS_RBTREE_BLACK;
                gparent->color = OS_RBTREE_RED;
                rb_rotate_right(tree, gparent);
            }
        /* parent  == grandparent's right child */
        } else {
            os_rbnode_t *uncle = gparent->left;

            if (uncle && uncle->color == OS_RBTREE_RED) {
                /*
                 * node's uncle == red (color flips)
                 *
                 *       G            g
                 *      / \          / \
                 *     u   p  -->   U   P
                 *          \            \
                 *           n            n
                 */
                uncle->color = OS_RBTREE_BLACK;
                parent->color = OS_RBTREE_BLACK;

                gparent->color = OS_RBTREE_RED;

                node = gparent;
            } else {
                /* node's uncle == black */
                if (node == parent->left) {
                    /*
                     * node == the parent's left child,
                     * (right rotate at parent)
                     *
                     *      G             G
                     *     / \           / \
                     *    p   U  -->    p   U
                     *   /               \
                     *  n                 n
                     */
                    node = node->parent;
                    rb_rotate_right(tree, node);
                }

                /*
                 * Now we're the right child,
                 * (left rotate at grand parent)
                 *
                 *      g           P
                 *     / \         / \
                 *    P   U  -->  n   g
                 *     \               \
                 *      n               U
                 */
                node->parent->color = OS_RBTREE_BLACK;
                gparent->color = OS_RBTREE_RED;
                rb_rotate_left(tree, gparent);
            }
        }
    }

    os_assert(tree->root);
    tree->root->color = OS_RBTREE_BLACK;
}

PRIVATE void rb_delete_color(
    os_rbtree_t *tree, os_rbnode_t *node, os_rbnode_t *parent)
{
    os_rbnode_t *sibling;
    os_assert(tree);

#define rb_is_black(r) ((!r) || (r)->color == OS_RBTREE_BLACK)
    while (node != tree->root && rb_is_black(node)) {
        if (node == parent->left) {
            sibling = parent->right;
            if (sibling->color == OS_RBTREE_RED) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                sibling->color = OS_RBTREE_BLACK;
                parent->color = OS_RBTREE_RED;
                rb_rotate_left(tree, parent);
                sibling = parent->right;
            }
            if (rb_is_black(sibling->left) && rb_is_black(sibling->right)) {
                /*
                 * Case 2 - sibling color flip
                 * (p could be either color here)
                 *
                 *    (p)           (p)
                 *    / \           / \
                 *   N   S    -->  N   s
                 *      / \           / \
                 *     Sl  Sr        Sl  Sr
                 */
                sibling->color = OS_RBTREE_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (rb_is_black(sibling->right)) {
                    /*
                     * Case 3 - right rotate at sibling
                     * (p could be either color here)
                     *
                     *   (p)           (p)
                     *   / \           / \
                     *  N   S    -->  N   Sl
                     *     / \             \
                     *    sl  Sr            s
                     *                       \
                     *                        Sr
                     */
                    sibling->left->color = OS_RBTREE_BLACK;
                    sibling->color = OS_RBTREE_RED;
                    rb_rotate_right(tree, sibling);
                    sibling = parent->right;
                }
                /*
                 * Case 4 - left rotate at parent + color flips
                 * (p and sl could be either color here.
                 *  After rotation, p becomes black, s acquires
                 *  p's color, and sl keeps its color)
                 *
                 *      (p)             (s)
                 *      / \             / \
                 *     N   S     -->   P   Sr
                 *        / \         / \
                 *      (sl) sr      N  (sl)
                 */
                sibling->color = parent->color;
                parent->color = OS_RBTREE_BLACK;
                sibling->right->color = OS_RBTREE_BLACK;
                rb_rotate_left(tree, parent);
                node = tree->root;
            }
        } else {
            sibling = parent->left;
            if (sibling->color == OS_RBTREE_RED) {
                sibling->color = OS_RBTREE_BLACK;
                parent->color = OS_RBTREE_RED;
                /* Case 1 - right rotate at parent */
                rb_rotate_right(tree, parent);
                sibling = parent->left;
            }
            if (rb_is_black(sibling->left) && rb_is_black(sibling->right)) {
                /* Case 2 - sibling color flip */
                sibling->color = OS_RBTREE_RED;
                node = parent;
                parent = node->parent;
            } else {
                if (rb_is_black(sibling->left)) {
                    sibling->right->color = OS_RBTREE_BLACK;
                    sibling->color = OS_RBTREE_RED;
                    /* Case 3 - left rotate at sibling */
                    rb_rotate_left(tree, sibling);
                    sibling = parent->left;
                }
                /* Case 4 - right rotate at parent + color flips */
                sibling->color = parent->color;
                parent->color = OS_RBTREE_BLACK;
                sibling->left->color = OS_RBTREE_BLACK;
                rb_rotate_right(tree, parent);
                node = tree->root;
            }
        }
    }
    if (node)
        node->color = OS_RBTREE_BLACK;
}

void os_rbtree_delete(os_rbtree_t *tree, void *rb_node)
{
    os_rbnode_t *node = rb_node;
    os_rbnode_t *child, *parent;
    os_rbtree_color_e color;
    os_assert(tree);
    os_assert(node);

    if (!node->left) {
        child = node->right;
        parent = node->parent;
        color = node->color;

        rb_replace_node(tree, node, child, parent);
    } else if (!node->right) {
        child = node->left;
        parent = node->parent;
        color = node->color;

        rb_replace_node(tree, node, child, parent);
    } else {
        os_rbnode_t *new = os_rbtree_min(node->right);

        child = new->right;
        parent = new->parent;
        color = new->color;

        new->left = node->left;
        node->left->parent = new;

        if (parent == node) {
            parent = new;
        } else {
            if (child) {
                child->parent = parent;
            }
            parent->left = child;

            new->right = node->right;
            node->right->parent = new;
        }

        new->color = node->color;

        rb_replace_node(tree, node, new, node->parent);
    }

    if (color == OS_RBTREE_BLACK)
        rb_delete_color(tree, child, parent);
}

void *os_rbtree_first(const os_rbtree_t *tree)
{
    os_rbnode_t *node;
    os_assert(tree);

    node = tree->root;
    if (!node)
        return NULL;

    return os_rbtree_min(node);
}

void *os_rbtree_last(const os_rbtree_t *tree)
{
    os_rbnode_t *node;
    os_assert(tree);

    node = tree->root;
    if (!node)
        return NULL;

    return os_rbtree_max(node);
}

#define rb_empty_node(node) ((node)->parent == (node))

void *os_rbtree_next(const void *rb_node)
{
    const os_rbnode_t *node = rb_node;
    os_rbnode_t *parent;
    os_assert(node);

    if (rb_empty_node(node))
        return NULL;

    if (node->right)
        return os_rbtree_min(node->right);

    while ((parent = node->parent) && node == parent->right)
        node = parent;

    return parent;
}

void *os_rbtree_prev(const void *rb_node)
{
    const os_rbnode_t *node = rb_node;
    os_rbnode_t *parent;
    os_assert(node);

    if (rb_empty_node(node))
        return NULL;

    if (node->left)
        return os_rbtree_max(node->left);

    while ((parent = node->parent) && node == parent->left)
        node = parent;

    return parent;
}

void os_rbtree_link_node(void *rb_node, os_rbnode_t *parent, os_rbnode_t **rb_link)
{
    os_rbnode_t *node = rb_node;
    node->parent = parent;
    node->left = node->right = NULL;
    node->color = OS_RBTREE_RED;

    *rb_link = node;
}

void *os_rbtree_min(const os_rbnode_t *rb_node)
{
    const os_rbnode_t *node = rb_node;
    os_assert(node);

    while (node->left)
        node = node->left;

    return (void *)node;
}

void *os_rbtree_max(const void *rb_node)
{
    const os_rbnode_t *node = rb_node;
    os_assert(node);

    while (node->right)
        node = node->right;

    return (void *)node;
}


bool os_rbtree_empty(const os_rbtree_t *tree)
{
    return tree->root == NULL;
}

int os_rbtree_count(const os_rbtree_t *tree)
{
    os_rbnode_t *node;
    int i = 0;
    os_rbtree_for_each(tree, node)
        i++;
    return i;
}

