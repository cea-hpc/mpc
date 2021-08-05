/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#define _GNU_SOURCE
#include <stdlib.h>

#include "mpcomp_task.h"

#include <mpc_common_debug.h>
#include <mpc_common_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_rank.h>

#include "mpcomp_core.h"

#include "mpcomp_tree.h"

/************************
 *  * OMP INSTANCE RELATED 
 *   ***********************/
static inline void
__instance_incr_ready_tasks(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = (mpc_omp_instance_t *) thread->instance;
    assert(instance);

    OPA_incr_int(&(instance->task_infos.ntasks_ready));
}

static inline void
__instance_decr_ready_tasks(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = (mpc_omp_instance_t *) thread->instance;
    assert(instance);

    OPA_decr_int(&(instance->task_infos.ntasks_ready));
}

/****************
 * TREE + LISTS *
 ****************/

/**
 * Check that given task can be attached to the current thread
 */
static mpc_omp_thread_t *
__thread_task_coherency(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;

    assert(thread);
    assert(thread->mvp);
    assert(thread->instance);
    assert(thread->instance->team);

    if (task)
    {
        /* tied-ness checks */
        int check = !mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_STARTED) || mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNTIED) || mpc_omp_tls == task->thread;

        if (!check)
        {
            printf("%d %d %d\n",
                mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_STARTED),
                mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNTIED),
                mpc_omp_tls == task->thread);
        }
        assert(check);
    }

    return thread;
}

static inline int
__task_list_is_empty(mpc_omp_task_list_t * list)
{
    return (list->head == NULL);
}

static inline void
__task_list_push_to_head(
    mpc_omp_task_list_t * list,
    mpc_omp_task_t * task)
{
    assert(list);
    const int t = list->type;
    if (__task_list_is_empty(list))
    {
        task->prev[t] = NULL;
        task->next[t] = NULL;
        list->head = task;
        list->tail = task;
    }
    else
    {
        task->prev[t] = NULL;
        task->next[t] = list->head;
        list->head->prev[t] = task;
        list->head = task;
    }
    OPA_incr_int(&(list->nb_elements));
}

static inline void
__task_list_push_to_tail(
    mpc_omp_task_list_t * list,
    mpc_omp_task_t * task)
{
    assert(list);
    const int t = list->type;
    if (__task_list_is_empty(list))
    {
        task->prev[t] = NULL;
        task->next[t] = NULL;
        list->head = task;
        list->tail = task;
    }
    else
    {
        task->prev[t] = list->tail;
        task->next[t] = NULL;
        list->tail->next[t] = task;
        list->tail = task;
    }
    OPA_incr_int(&(list->nb_elements));
}

static inline void
__task_list_push(
        mpc_omp_task_list_t * list,
        mpc_omp_task_t * task)
{
    __task_list_push_to_tail(list, task);
}

static inline void
__task_pqueue_node_push(
    mpc_omp_task_pqueue_node_t * node,
    mpc_omp_task_t * task)
{
    assert(node);
    assert(node);
    __task_list_push(&(node->tasks), task);
}

static inline void
__task_list_remove(
    mpc_omp_task_list_t * list,
    mpc_omp_task_t * task)
{
    assert(list);

    OPA_decr_int(&(list->nb_elements));

    const int t = list->type;

    if (task == list->head)
    {
        list->head = task->next[t];
    }

    if (task == list->tail)
    {
        list->tail = task->prev[t];
    }

    if (task->next[t])
    {
        task->next[t]->prev[t] = task->prev[t];
    }

    if (task->prev[t])
    {
        task->prev[t]->next[t] = task->next[t];
    }
}

static inline mpc_omp_task_t *
__task_list_pop_from_head(mpc_omp_task_list_t * list)
{
    assert(list);
    if (__task_list_is_empty(list))
    {
        assert(0);
        return NULL;
    }

    int t = list->type;
    mpc_omp_task_t * task = list->head;
    list->head = task->next[t];
    if (list->head)
    {
        list->head->prev[t] = NULL;
    }
    if (task->next[t] == NULL)
    {
        list->tail = NULL;
    }
    OPA_decr_int(&(list->nb_elements));
    return task;
}

static inline mpc_omp_task_t *
__task_list_pop_from_tail(mpc_omp_task_list_t * list)
{
    assert(list);
    if (__task_list_is_empty(list))
    {
        assert(0);
        return NULL;
    }

    int t = list->type;
    mpc_omp_task_t * task = list->tail;
    list->tail = task->prev[t];
    if (list->tail)
    {
        list->tail->next[t] = NULL;
    }
    if (task->prev[t] == NULL)
    {
        list->head = NULL;
    }
    OPA_decr_int(&(list->nb_elements));
    return task;
}

static inline mpc_omp_task_t *
__task_list_pop(mpc_omp_task_list_t * list)
{
    assert(list);
    return __task_list_pop_from_head(list);
}

static inline void
__task_pqueue_coherency_check_colors(
    mpc_omp_task_pqueue_t * tree,
    mpc_omp_task_pqueue_node_t * node)
{
    if (node)
    {
        /* root node has no parent, every other nodes have */
        assert(node == tree->root || node->parent);

        /* node is either red or black */
        assert(node->color == 'B' || node->color == 'R');

        /* If a node is red, then both its children are black. */
        assert(!node->parent || !(node->color == 'R' && node->parent->color == 'R'));

        __task_pqueue_coherency_check_colors(tree, node->left);
        __task_pqueue_coherency_check_colors(tree, node->right);
    }
}

static int
__task_pqueue_coherency_check_paths(mpc_omp_task_pqueue_node_t * node)
{
    if (node == NULL) 
    {
        return 1;
    }

    int left_height = __task_pqueue_coherency_check_paths(node->left);
    if (left_height == 0)
    {
        return 0;
    }

    int right_height = __task_pqueue_coherency_check_paths(node->right);
    if (right_height == 0)
    {
        return right_height;
    }

    if (left_height != right_height)
    {
        return 0;
    }
    else
    {
        return left_height + (node->color == 'B') ? 1 : 0;
    }
}

static void
__task_pqueue_coherency_check(mpc_omp_task_pqueue_t * tree)
{
    /* 1. The root of the tree is always black */
    assert(!tree->root || tree->root->color == 'B');

    if (tree->root)
    {
        /* 2. If a node is red, then both its children are black */
        __task_pqueue_coherency_check_colors(tree, tree->root);

        /* 3. Every path from a node to any of its descendant NULL nodes
         * has the same number of black nodes */
        assert(__task_pqueue_coherency_check_paths(tree->root));
    }
}

static void
__task_pqueue_rotate_right(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * x)
{
    mpc_omp_task_pqueue_node_t * y = x->left;
    x->left = y->right;
    if (y->right)
    {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL)
    {
        tree->root = y;
    }
    else if (x == x->parent->right)
    {
        x->parent->right = y;
    }
    else
    {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

static void
__task_pqueue_rotate_left(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * x)
{
    mpc_omp_task_pqueue_node_t * y = x->right;

    x->right = y->left;
    if (y->left)
    {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL)
    {
        tree->root = y;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = y;
    }
    else
    {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

static inline void
__task_pqueue_change_child(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * old,
        mpc_omp_task_pqueue_node_t * new,
        mpc_omp_task_pqueue_node_t * parent)
{
    if (parent)
    {
        if (parent->left == old)
        {
            parent->left = new;
        }
        else
        {
            parent->right = new;
        }
    }
    else
    {
        tree->root = new;
    }
}

static mpc_omp_task_pqueue_node_t *
__task_pqueue_delete_node_locally(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * node)
{
    assert(node->right == NULL);

    mpc_omp_task_pqueue_node_t * rebalance = NULL;

    /* The node has no child */
    if (node->left == NULL)
    {
        __task_pqueue_change_child(tree, node, NULL, node->parent);
        rebalance = (node->color == 'B') ? node->parent : NULL;
    }
    /* The node has a left child */
    else
    {
        node->left->parent = node->parent;
        node->left->color = node->color;
        __task_pqueue_change_child(tree, node, node->left, node->parent);
        rebalance = NULL;
    }

    return rebalance;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
static inline void
__task_pqueue_rotate_set_parents(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * old,
        mpc_omp_task_pqueue_node_t * new,
        char color)
{
    mpc_omp_task_pqueue_node_t * parent = old->parent;

    new->parent = old->parent;
    new->color  = old->color;

    old->parent = new;
    old->color  = color;

    __task_pqueue_change_child(tree, old, new, parent);
}

/**
 * https://github.com/torvalds/linux/blob/fcadab740480e0e0e9fa9bd272acd409884d431a/include/linux/rbtree_augmented.h#L197
 */
static void
__task_pqueue_delete_node_color(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * parent)
{
    mpc_omp_task_pqueue_node_t * node = NULL;
    mpc_omp_task_pqueue_node_t * sibling, *tmp1, *tmp2;

    while (true)
    {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->right;
        if (node != sibling)    /* node == parent->left */
        {
            if (sibling->color == 'R')
            {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->left;
                parent->right = tmp1;
                sibling->left = parent;
                tmp1->parent = parent;
                tmp1->color = 'B';
                __task_pqueue_rotate_set_parents(tree, parent, sibling, 'R');
                sibling = tmp1;
            }
            tmp1 = sibling->right;
            if (!tmp1 || tmp1->color == 'B')
            {
                tmp2 = sibling->left;
                if (!tmp2 || tmp2->color == 'B')
                {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    sibling->parent = parent;
                    sibling->color  = 'R';
                    if (parent->color == 'R')
                    {
                        parent->color = 'B';
                    }
                    else
                    {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                        {
                            continue;
                        }
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *                       \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in __task_pqueue_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RB_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */
                tmp1 = tmp2->right;
                sibling->left = tmp1;
                tmp2->right = sibling;
                parent->right = tmp2;
                if (tmp1)
                {
                    tmp1->parent    = sibling;
                    tmp1->color     = 'B';
                }
                tmp1 = sibling;
                sibling = tmp2;
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
            tmp2 = sibling->left;
            parent->right = tmp2;
            sibling->left = parent;
            tmp1->parent = sibling;
            tmp1->color = 'B';
            if (tmp2)
            {
                tmp2->parent = parent;
            }
            __task_pqueue_rotate_set_parents(tree, parent, sibling, 'B');
            break;
        }
        else
        {
            sibling = parent->left;
            if (sibling->color == 'R')
            {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->right;
                parent->left = tmp1;
                sibling->right = parent;
                tmp1->parent = parent;
                tmp1->color = 'B';
                __task_pqueue_rotate_set_parents(tree, parent, sibling, 'R');
                sibling = tmp1;
            }
            tmp1 = sibling->left;
            if (!tmp1 || tmp1->color == 'B')
            {
                tmp2 = sibling->right;
                if (!tmp2 || tmp2->color == 'B')
                {
                    /* Case 2 - sibling color flip */
                    sibling->parent = parent;
                    sibling->color = 'R';
                    if (parent->color == 'R')
                    {
                        parent->color = 'B';
                    }
                    else
                    {
                        node = parent;
                        parent = node->parent;
                        if (parent)
                        {
                            continue;
                        }
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->left;
                sibling->right = tmp1;
                tmp2->left = sibling;
                parent->left = tmp2;
                if (tmp1)
                {
                    tmp1->parent = sibling;
                    tmp1->color = 'B';
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->right;
            parent->left = tmp2;
            sibling->right = parent;
            tmp1->parent = sibling;
            tmp1->color = 'B';
            if (tmp2)
            {
                tmp2->parent = parent;
            }
            __task_pqueue_rotate_set_parents(tree, parent, sibling, 'B');
            break;
        }
    }
}

/** WARNING :
 *  This function deletes `node` from `tree`, assuming that `node`
 *  is the right-most node in the tree.
 */
static void
__task_pqueue_delete_node(
    mpc_omp_task_pqueue_t * tree,
    mpc_omp_task_pqueue_node_t * node)
{
    assert(node->right == NULL);
    mpc_omp_task_pqueue_node_t * rebalance = __task_pqueue_delete_node_locally(tree, node);
    if (rebalance)
    {
        __task_pqueue_delete_node_color(tree, rebalance);
    } 
    mpc_omp_free(node);
}

/** Dump pqueue tree in .dot format */
static void
__task_pqueue_dump_node(mpc_omp_task_pqueue_node_t * node, FILE * f)
{
    if (!node)
    {
        return ;
    }
    char * color = (node->color == 'R') ? "#ff0000" : "#000000";
    fprintf(f, "    N%p[fontcolor=\"#ffffff\", label=\"%d\", style=filled, fillcolor=\"%s\"] ;\n", node, node->priority, color);
    __task_pqueue_dump_node(node->left, f);
    __task_pqueue_dump_node(node->right, f);
}

static void
__task_pqueue_dump_edge(mpc_omp_task_pqueue_node_t * node, FILE * f)
{
    if (!node)
    {
        return ;
    }
    if (node->left)
    {
        fprintf(f, "    N%p->N%p ; \n", node, node->left);
        __task_pqueue_dump_edge(node->left, f);
    }
    if (node->right)
    {
        fprintf(f, "    N%p->N%p ; \n", node, node->right);
        __task_pqueue_dump_edge(node->right, f);
    }
}

/**
 * Example usage:
 *  _mpc_task_pqueue_dump(tree, "after-delete");
 */
void
_mpc_task_pqueue_dump(mpc_omp_task_pqueue_t * tree, char * label)
{
    char filepath[256];
    sprintf(filepath, "%lf-%p-%s-tree.dot", mpc_omp_timestamp(), tree, label);
    FILE * f = fopen(filepath, "w");

    fprintf(f, "digraph g%d {\n", OPA_load_int(&(tree->nb_elements)));
    __task_pqueue_dump_node(tree->root, f);
    __task_pqueue_dump_edge(tree->root, f);
    fprintf(f, "}\n");

    fclose(f);
}

/**
 * Fixup the rb tree after 'node' was inserted inside 'tree'
 * https://en.wikipedia.org/wiki/Red-black_tree
 * https://www.youtube.com/watch?v=qA02XWRTBdw
 */
static void
__task_pqueue_fixup(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * node)
{
    while (node->parent->color == 'R')
    {
        if (node->parent == node->parent->parent->right)
        {
            /* uncle */
            mpc_omp_task_pqueue_node_t * u = node->parent->parent->left;
            if (u && u->color == 'R')
            {
                /* case 3.1 */
                u->color = 'B';
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                node = node->parent->parent;
            }
            else
            {
                if (node == node->parent->left)
                {
                    /* case 3.2.2 */
                    node = node->parent;
                    __task_pqueue_rotate_right(tree, node);
                }
                /* case 3.2.1 */
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                __task_pqueue_rotate_left(tree, node->parent->parent);
            }
        }
        else
        {
            /* uncle */
            mpc_omp_task_pqueue_node_t * u = node->parent->parent->right;

            if (u && u->color == 'R')
            {
                /* case 3.1 */
                u->color = 'B';
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                node = node->parent->parent;
            }
            else
            {
                if (node == node->parent->right)
                {
                    /* case 3.2.2 */
                    node = node->parent;
                    __task_pqueue_rotate_left(tree, node);
                }
                /* case 3.2.1 */
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                __task_pqueue_rotate_right(tree, node->parent->parent);
            }
        }
        if (node == tree->root)
        {
            break;
        }
    }
    tree->root->color = 'B';
}

static mpc_omp_task_pqueue_node_t *
__task_pqueue_node_new(mpc_omp_task_pqueue_node_t * parent, char color, size_t priority)
{
    /* TODO : use a recycler */
    mpc_omp_task_pqueue_node_t * node = (mpc_omp_task_pqueue_node_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_node_t));
    assert(node);

    memset(node, 0, sizeof(mpc_omp_task_pqueue_node_t));
    node->parent        = parent;
    node->priority      = priority;
    node->color         = color;
    node->tasks.type    = MPC_OMP_TASK_LIST_TYPE_SCHEDULER;
    return node;
}

/**
 * Return the pqueue tree node of the given priority
 */
static mpc_omp_task_pqueue_node_t *
__task_pqueue_insert(mpc_omp_task_pqueue_t * tree, int priority)
{
    /* Get current OpenMP thread */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_pqueue_node_t * node;

    if (tree->root == NULL)
    {
        node = __task_pqueue_node_new(NULL, 'B', priority);
        tree->root = node;
        return node;
    }

    mpc_omp_task_pqueue_node_t * n = tree->root;
    while (1)
    {
        if (priority == n->priority)
        {
            return n;
        }
        else if (priority < n->priority)
        {
            if (n->left)
            {
                n = n->left;
            }
            else
            {
                node = __task_pqueue_node_new(n, 'R', priority);
                n->left = node;
                break ;
            }
        }
        else
        {
            if (n->right)
            {
                n = n->right;
            }
            else
            {
                node = __task_pqueue_node_new(n, 'R', priority);
                n->right = node;
                break ;
            }
        }
    }

    __task_pqueue_fixup(tree, node);
    __task_pqueue_coherency_check(tree);
    return node;
}

/**
 * O(log2(n)) down there
 */
static mpc_omp_task_t *
__task_pqueue_pop_top_priority(mpc_omp_task_pqueue_t * tree)
{
    assert(tree->root);

    mpc_omp_task_pqueue_node_t * node = tree->root;
    while (node->right)
    {
        node = node->right;
    }
    mpc_omp_task_list_t * list = &(node->tasks);
    mpc_omp_task_t * task = __task_list_pop(list);
    if (task && OPA_load_int(&(list->nb_elements)) == 0)
    {
        __task_pqueue_delete_node(tree, node);
    }
    return task;
}

static mpc_omp_task_t *
__task_pqueue_pop(mpc_omp_task_pqueue_t * pqueue)
{
    if (OPA_load_int(&(pqueue->nb_elements)) == 0)
    {
        return NULL;
    }

    mpc_omp_task_t * task = NULL;

    if (OPA_load_int(&(pqueue->nb_elements)))
    {
        /* TODO : maybe do not lock all the time (tied lists) */
        mpc_common_spinlock_lock(&(pqueue->lock));
        {
            if (OPA_load_int(&(pqueue->nb_elements)))
            {
                task = __task_pqueue_pop_top_priority(pqueue);
                assert(task);
                assert(task->pqueue == pqueue);
                task->pqueue = NULL;
                OPA_decr_int(&(pqueue->nb_elements));
                __instance_decr_ready_tasks();
            }
        }
        mpc_common_spinlock_unlock(&(pqueue->lock));
    }

    return task;
}


/* Return the tasks tree contained in element of rank 'globalRank' */
static inline mpc_omp_task_pqueue_t *
__rank_get_task_pqueue(int globalRank, mpc_omp_task_pqueue_type_t type)
{
    mpc_omp_thread_t *thread;
    mpc_omp_instance_t *instance;
    mpc_omp_generic_node_t *gen_node;
    assert( type >= 0 && type < MPC_OMP_PQUEUE_TYPE_COUNT );
    /* Retrieve the current thread information */
    assert( mpc_omp_tls != NULL );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread->mvp );
    /* Retrieve the current thread instance */
    assert( thread->instance );
    instance = thread->instance;
    /* Retrieve target node or mvp generic node */
    assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );

    /* Retrieve target node or mvp pqueue */
    if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
    {
        return MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_HEAD( gen_node->ptr.mvp, type );
    }

    if ( gen_node->type == MPC_OMP_CHILDREN_NODE )
    {
        return MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_HEAD( gen_node->ptr.node, type );
    }

    /* Extra node to preserve regular tree_shape */
    //assert( gen_node->type == MPC_OMP_CHILDREN_NULL );
    return NULL;
}

static inline mpc_omp_task_pqueue_t *
__thread_get_task_pqueue(mpc_omp_thread_t * thread, mpc_omp_task_pqueue_type_t type)
{
    return __rank_get_task_pqueue(MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_NODE_RANK(thread->mvp, type), type);
}

static mpc_omp_task_t *
__thread_task_pop_type(mpc_omp_thread_t * thread, mpc_omp_task_pqueue_type_t type)
{
    return __task_pqueue_pop(__thread_get_task_pqueue(thread, type));
}

static void
__task_pqueue_push(mpc_omp_task_pqueue_t * pqueue, mpc_omp_task_t * task)
{
    assert(pqueue);
    assert(task);
    assert(task->pqueue == NULL);

    mpc_common_spinlock_lock(&(pqueue->lock));
    {
        mpc_omp_task_pqueue_node_t * node = __task_pqueue_insert(pqueue, task->priority);
        __task_pqueue_node_push(node, task);
        task->pqueue = pqueue;
    }
    mpc_common_spinlock_unlock(&(pqueue->lock));
    
    OPA_incr_int(&(pqueue->nb_elements));
    __instance_incr_ready_tasks();
}

/****************
 * TASK WEIGHT  *
 ****************/

# if 0
# define ABS(X)     ((X) < 0 ? -(X) : (X))
# define MIN(X, Y)  (((X) < (Y)) ? (X) : (Y))
# define MAX(X, Y)  (((X) < (Y)) ? (Y) : (X))

static float
__task_profile_distance(mpc_omp_task_t * task, mpc_omp_task_profile_t * profile)
{
    int npred = OPA_load_int(&(task->dep_node.npredecessors));
    int nsucc = OPA_load_int(&(task->dep_node.nsuccessors));

    int max_npred = MAX(npred, profile->npredecessors);
    int max_nsucc = MAX(nsucc, profile->nsuccessors);

    return 
        0.2f * ABS((int)task->size - (int)profile->size) / MAX(task->size, profile->size) + 
        0.2f * ((task->property & MPC_OMP_TASK_PROP_PROFILE_MASK) == (profile->property & MPC_OMP_TASK_PROP_PROFILE_MASK) ? 0.0f : 1.0f) +
        0.2f * (max_npred ? ABS(npred - profile->npredecessors) / max_npred : 0.0f) +
        0.2f * (max_nsucc ? ABS(nsucc - profile->nsuccessors)   / max_nsucc : 0.0f) +
        0.2f * (task->parent->uid == profile->parent_uid ? 0.0f : 1.0f)
    ;
}

/**
 * Return the minimum distance between the task and an existing profile.
 * N.B: distance is normalized (in [0.0, 1.0])
 * 0.0 means the 2 profiles matches absolutely
 * 1.0 means they dont
 */
static float
__task_profile_min_distance(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    float distance = 1.0;
    mpc_omp_task_profile_t * profile = (mpc_omp_task_profile_t *) OPA_load_ptr(&(thread->instance->task_infos.profiles.head));
    while (profile)
    {
        distance = MIN(distance, __task_profile_distance(task, profile));
        profile = profile->next;
    }
    return distance;
}

# undef ABS
# undef MAX
# undef MIN
# endif

/**
 * Return 1 if given task profile alraedy was registered
 */
static int
__task_profile_exists(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_profile_t * profile = (mpc_omp_task_profile_t *) OPA_load_ptr(&(thread->instance->task_infos.profiles.head));
    while (profile)
    {
        /** TODO : this matching function is optimized for apps with:
         *  - mono productor
         *  - no task recursion
         *  - complete graph discovery before running a match (so that we have the correct number of successors)
         */
        if (task->size == profile->size &&
            (task->property & MPC_OMP_TASK_PROP_PROFILE_MASK) == (profile->property & MPC_OMP_TASK_PROP_PROFILE_MASK) &&
            OPA_load_int(&(task->dep_node.npredecessors)) == profile->npredecessors &&
            OPA_load_int(&(task->dep_node.nsuccessors)) == profile->nsuccessors &&
            task->parent->uid == profile->parent_uid)
        {
            return 1;
        }
        profile = profile->next;
    }
    return 0;
}

void mpc_omp_task_is_send(int priority)
{
    mpc_omp_task_priority_policy_t policy = mpc_omp_conf_get()->task_priority_policy;
    if (policy != MPC_OMP_TASK_PRIORITY_POLICY_FA1)
    {
        return ;
    }

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    if (__task_profile_exists(task))
    {
        return ;
    }

    mpc_omp_task_profile_t * profile;
    mpc_common_spinlock_lock(&(thread->instance->task_infos.profiles.spinlock));
    {
        profile = (mpc_omp_task_profile_t *) mpc_omp_alloc(sizeof(mpc_omp_task_profile_t));
        assert(profile);
        profile->size = task->size;
        profile->property = task->property;
        profile->next = (mpc_omp_task_profile_t *) OPA_load_ptr(&(thread->instance->task_infos.profiles.head));
        profile->npredecessors  = OPA_load_int(&(task->dep_node.npredecessors));
        profile->nsuccessors    = OPA_load_int(&(task->dep_node.nsuccessors));
        profile->priority = priority;
        OPA_store_ptr(&(thread->instance->task_infos.profiles.head), profile);
        OPA_fetch_and_incr_int(&(thread->instance->task_infos.profiles.n));
    }
    mpc_common_spinlock_unlock(&(thread->instance->task_infos.profiles.spinlock));
}

/*********************
 * TASK DEPENDENCIES *
 *********************/

# define MPC_OMP_TASK_LOCK(task)     mpc_common_spinlock_lock(&(task->lock))
# define MPC_OMP_TASK_UNLOCK(task)   mpc_common_spinlock_unlock(&(task->lock))

static inline uint32_t
__task_dep_hash(uintptr_t addr)
{
    return addr;
}

static inline void
__task_ref(mpc_omp_task_t * task)
{
    assert(task);
    OPA_incr_int(&(task->ref_counter));
}

void __task_delete_hard(mpc_omp_task_t * task);

static void
__task_unref(mpc_omp_task_t * task)
{
    assert(task);
    assert(OPA_load_int(&(task->ref_counter)) > 0);
    if (OPA_fetch_and_decr_int(&(task->ref_counter)) == 1)
    {
        __task_delete_hard(task);
    }
}

static inline void
__task_list_elt_delete(mpc_omp_task_dep_list_elt_t * elt)
{
    __task_unref(elt->task);
# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    mpc_common_recycler_recycle(&(thread->task_infos.list_recycler), elt);
# else /* MPC_OMP_TASK_USE_RECYCLERS */
    mpc_omp_free(elt);
# endif /* MPC_OMP_TASK_USE_RECYCLERS */
}

static void
__task_list_delete(mpc_omp_task_dep_list_elt_t * list)
{
    while (list)
    {
        mpc_omp_task_dep_list_elt_t * tmp = list->next;
        __task_list_elt_delete(list);
        list = tmp;
    }
}

static inline void
__task_delete_soft(mpc_omp_task_t * task)
{
# if MPC_OMP_TASK_COMPILE_FIBER
    if (MPC_OMP_TASK_FIBER_ENABLED && task->fiber)
    {
        mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
        if (thread->task_infos.fiber)
        {
# if MPC_OMP_TASK_USE_RECYCLERS
            mpc_common_recycler_recycle (&(thread->task_infos.fiber_recycler), task->fiber);
# else
            mpc_omp_free(task->fiber);
# endif /* MPC_OMP_TASK_USE_RECYCLERS */
            task->fiber = NULL;
        }
        else
        {
            thread->task_infos.fiber = task->fiber;
        }
    }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
}


void
__task_delete_hard(mpc_omp_task_t * task)
{
    assert(OPA_load_int(&(task->ref_counter)) == 0);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

     /* Release successors */
     __task_list_delete(task->dep_node.successors);
     task->dep_node.successors = NULL;

# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_common_nrecycler_recycle(&(thread->task_infos.task_recycler), task, task->size);
# else
    mpc_omp_free(task);
# endif /* MPC_OMP_TASK_USE_RECYCLERS */

    /* decrement number of existing tasks */
    OPA_decr_int(&(thread->instance->task_infos.ntasks));
}

/** called whenever a task completed it execution */
static void
__task_finalize(mpc_omp_task_t * task)
{
    assert(mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_COMPLETED));

    _mpc_omp_task_unref_parent_task(task);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    __task_delete_soft(task);
    mpc_omp_taskgroup_del_task(task);
    _mpc_omp_task_finalize_deps(task);
    __task_unref(task);
}

static mpc_omp_task_dep_list_elt_t *
__task_list_elt_new(mpc_omp_task_dep_list_elt_t * list, mpc_omp_task_t * task)
{
    assert(task);

# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    mpc_omp_task_dep_list_elt_t * new_node = (mpc_omp_task_dep_list_elt_t *) mpc_common_recycler_alloc(&(thread->task_infos.list_recycler));
# else /* MPC_OMP_TASK_USE_RECYCLERS */
    mpc_omp_task_dep_list_elt_t * new_node = (mpc_omp_task_dep_list_elt_t *) mpc_omp_alloc(sizeof(mpc_omp_task_dep_list_elt_t));
# endif /* MPC_OMP_TASK_USE_RECYCLERS */

    assert(new_node);
    __task_ref(task);
    new_node->task = task;
    new_node->next = list;

    return new_node;
}    

static mpc_omp_task_dep_htable_entry_t *
__task_dep_htable_entry_add(mpc_omp_task_dep_htable_t * htable, uintptr_t addr)
{
    mpc_omp_task_dep_htable_entry_t * entry;

    mpc_common_spinlock_lock(&(htable->lock));
    {
        const uint32_t hash = htable->hfunc(addr) % htable->capacity;
        entry = htable->entries[hash];
        while (entry)
        {
            if (entry->key == addr)
            {
                break ;
            }
            entry = entry->next;
        }

        /* Allocation */
        /* TODO : use recycler maybe */
        if (entry == NULL)
        {
            entry = (mpc_omp_task_dep_htable_entry_t *)mpc_omp_alloc(sizeof(mpc_omp_task_dep_htable_entry_t));
            assert(entry);
            entry->key = addr;
            entry->last_out = NULL;
            entry->last_in = NULL;
            entry->next = htable->entries[hash];
            htable->entries[hash] = entry;
        }
        ++htable->size;
    }
    mpc_common_spinlock_unlock(&(htable->lock));

    return entry;
}

static void
__task_dep_htable_delete(mpc_omp_task_dep_htable_t * htable)
{
    unsigned int i;
    for (i = 0; htable && i < htable->capacity; i++)
    {
        mpc_omp_task_dep_htable_entry_t * entry = htable->entries[i];
        if (entry)
        {
            while (entry)
            {
                mpc_omp_task_dep_htable_entry_t * next = entry->next;
                if (entry->last_in)
                {
                    __task_list_delete(entry->last_in);
                }
                if (entry->last_out)
                {
                    __task_unref(entry->last_out);
                }
                mpc_omp_free(entry);
                entry = next;
            }
        }
    }
    mpc_omp_free(htable);
}

static mpc_omp_task_dep_htable_t *
__task_dep_htable_new(mpc_omp_task_dep_hash_func_t hfunc)
{
    assert(hfunc);

    mpc_omp_task_dep_htable_t * htable = (mpc_omp_task_dep_htable_t *) mpc_omp_alloc(sizeof(mpc_omp_task_dep_htable_t));
    htable->hfunc       = hfunc;
    htable->size        = 0;
    htable->capacity    = MPC_OMP_TASK_DEP_HTABLE_CAPACITY;
    mpc_common_spinlock_init(&(htable->lock), 0);
    memset(htable->entries, 0, sizeof(htable->entries));

    return htable;
}

static inline void
__task_update_dependency_depth(mpc_omp_task_t * task, mpc_omp_task_t * predecessor)
{
    if (predecessor->dep_node.min_depth + 1 < task->dep_node.min_depth)
    {
        task->dep_node.min_depth = predecessor->dep_node.min_depth + 1;
    }
    if (predecessor->dep_node.max_depth + 1 > task->dep_node.max_depth)
    {
        task->dep_node.max_depth = predecessor->dep_node.max_depth + 1;
    }
}

static inline void
__task_process_deps(mpc_omp_task_t * task,
                     mpc_omp_task_dep_htable_t * htable,
                     void ** depend)
{
    assert(task);
    assert(htable);
    assert(depend);

    size_t i, j;

    const size_t tot_deps_num = (uintptr_t)depend[0];
    const size_t out_deps_num = (uintptr_t)depend[1];

    if (!tot_deps_num)
    {
        return ; 
    }

    // Filter redundant value
    size_t task_already_process_num = 0;
    uintptr_t * task_already_process_list = (uintptr_t *) mpc_omp_alloc(sizeof(uintptr_t) * tot_deps_num);
    assert(task_already_process_list);

    for (i = 0; i < tot_deps_num; i++)
    {
        int redundant = 0;

        /* FIND HASH IN HTABLE */
        const uintptr_t addr = (uintptr_t)depend[2 + i];
        const int type = (i < out_deps_num) ? MPC_OMP_TASK_DEP_OUT : MPC_OMP_TASK_DEP_IN;
        assert(task_already_process_num < tot_deps_num);

        for (j = 0; j < task_already_process_num; j++)
        {
            if (task_already_process_list[j] == addr)
            {
                redundant = 1;
                break;
            }
        }

        /** OUT are in first position en OUT > IN deps */
        if (redundant)
        {
            continue;
        }

        task_already_process_list[task_already_process_num++] = addr;

        mpc_omp_task_dep_htable_entry_t * entry = __task_dep_htable_entry_add(htable, addr);
        assert(entry);

        /**
         * For the in dependence-type, if the storage location of at least one of the list items matches
         * the storage location of a list item appearing in a depend clause with an out or inout dependence-type
         * on a construct from which a sibling task was previously generated,
         * then the generated task will be a dependent task of that sibling task.
         */

        /**
         * For the out and inout dependence-types, if the storage location of at least one of the list items matches
         * the storage location of a list item appearing in a depend clause with an in, out or inout dependence-type
         * on a construct from which a sibling task was previously generated,
         * then the generated task will be a dependent task of that sibling task.
         */

        // NEW [out] dep must be after all [in] deps
        if (type == MPC_OMP_TASK_DEP_OUT && entry->last_in != NULL)
        {
            mpc_omp_task_dep_list_elt_t * list_elt;
            for (list_elt = entry->last_in; list_elt; list_elt = list_elt->next)
            {
                mpc_omp_task_t * out_task = list_elt->task;

                if (OPA_load_int(&(out_task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED)
                {
                    MPC_OMP_TASK_LOCK(out_task);
                    {
                        if (OPA_load_int(&(out_task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED)
                        {
                            out_task->dep_node.successors = __task_list_elt_new(out_task->dep_node.successors, task);
                            OPA_incr_int(&(out_task->dep_node.nsuccessors));

                            task->dep_node.predecessors = __task_list_elt_new(task->dep_node.predecessors, out_task);
                            OPA_incr_int(&(task->dep_node.npredecessors));
                            __task_update_dependency_depth(task, out_task);
                        }
                    }
                    MPC_OMP_TASK_UNLOCK(out_task);
                }
            }
            __task_list_delete(entry->last_in);
            entry->last_in = NULL;
        }
        else
        {
            /** Non executed OUT dependency**/
            mpc_omp_task_t * last_out = entry->last_out;

            if (last_out && (OPA_load_int(&(last_out->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED))
            {
                MPC_OMP_TASK_LOCK(last_out);
                {
                    if (OPA_load_int(&(last_out->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED)
                    {
                        last_out->dep_node.successors = __task_list_elt_new(last_out->dep_node.successors, task);
                        OPA_incr_int(&(last_out->dep_node.nsuccessors));

                        task->dep_node.predecessors = __task_list_elt_new(task->dep_node.predecessors, last_out);
                        OPA_incr_int(&(task->dep_node.npredecessors));
                        __task_update_dependency_depth(task, last_out);
                    }
                }
                MPC_OMP_TASK_UNLOCK(last_out);
            }
        }

        if (type == MPC_OMP_TASK_DEP_OUT)
        {
            if (entry->last_out)
            {
                __task_unref(entry->last_out);
            }
            __task_ref(task);
            entry->last_out = task;
        }
        else
        {
            assert(type == MPC_OMP_TASK_DEP_IN);
            entry->last_in = __task_list_elt_new(entry->last_in, task);
        }
    }

    mpc_omp_free(task_already_process_list);
}

void
_mpc_omp_task_finalize_deps(mpc_omp_task_t * task)
{
    assert(task);

    /* if no dependencies */
    if (!mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_DEPEND))
    {
        return ;
    }

    MPC_OMP_TASK_LOCK(task);
    {
        OPA_store_int(&(task->dep_node.status), MPC_OMP_TASK_DEP_TASK_FINALIZED);
    }
    MPC_OMP_TASK_UNLOCK(task);

    /* delete task dependencies hashtable */
    if (task->dep_node.htable)
    {
        __task_dep_htable_delete(task->dep_node.htable);
        task->dep_node.htable = NULL;
    }

    /* TODO : remove me from my parent htable */

    /* Resolve successor's data dependency */
    mpc_omp_task_dep_list_elt_t * succ = task->dep_node.successors;
    while (succ)
    {
        MPC_OMP_TASK_TRACE_DEPENDENCY(task, succ->task);

        /* save next successor */
        mpc_omp_task_dep_list_elt_t * next_elt = succ->next;

        /* if successor's dependencies are fullfilled, process it */
        if (OPA_fetch_and_decr_int(&(succ->task->dep_node.ref_predecessors)) == 1)
        {
            if (OPA_cas_int(&(succ->task->dep_node.status), MPC_OMP_TASK_DEP_TASK_NOT_READY, MPC_OMP_TASK_DEP_TASK_READY) == MPC_OMP_TASK_DEP_TASK_NOT_READY)
            {
                _mpc_omp_task_process(succ->task);
            }
        }

        /* processes next successor */
        succ = next_elt;
    }

    /* Since 'task' is completed, no longer need to access it predecessors */
    __task_list_delete(task->dep_node.predecessors);
    task->dep_node.predecessors = NULL;
}

static void
__task_priority_propagate_on_predecessors(mpc_omp_task_t * task)
{
    mpc_omp_task_priority_policy_t policy = mpc_omp_conf_get()->task_priority_policy;
    if (task->dep_node.predecessors && OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED)
    {
        MPC_OMP_TASK_LOCK(task);
        {
            if (OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_FINALIZED)
            {
                mpc_omp_task_dep_list_elt_t * predecessor = task->dep_node.predecessors;
                while (predecessor)
                {
                    /* if the task is not in queue, propagate the priority */
                    switch (policy)
                    {
                        case (MPC_OMP_TASK_PRIORITY_POLICY_MA2):
                        case (MPC_OMP_TASK_PRIORITY_POLICY_SA1):
                            {
                                if (predecessor->task->priority < task->priority)
                                {
                                    predecessor->task->priority = task->priority;
                                    __task_priority_propagate_on_predecessors(predecessor->task);
                                }
                                break ;
                            }

                        case (MPC_OMP_TASK_PRIORITY_POLICY_SA2):
                            {
                                if (predecessor->task->priority < task->priority - 1)
                                {
                                    predecessor->task->priority = task->priority - 1;
                                    __task_priority_propagate_on_predecessors(predecessor->task);
                                }
                                break ;
                            }

                        default:
                            {
                                assert(0);
                                break ;
                            }
                    }
                    predecessor = predecessor->next;
                }
            }
        }
        MPC_OMP_TASK_UNLOCK(task);
    }
}

static inline void
__task_priority_compute_profile(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    mpc_omp_instance_t * instance = thread->instance;
    int profile_version = OPA_load_int(&(instance->task_infos.profiles.n));
    if (task->dep_node.profile_version == profile_version)
    {
        return ;
    }
    task->dep_node.profile_version = profile_version;

    int priority = __task_profile_exists(task) ? INT_MAX : 0;
    if (task->priority < priority)
    {
        task->priority = priority;
    }
}

# if 0
static inline void
__task_priority_compute_profile_distance(mpc_omp_task_t * task)
{
    float distance = __task_profile_min_distance(task);
    assert(0.0f <= distance && distance <= 1.0f);
    int priority = (int)((float)INT_MAX * (1.0 - distance));
    if (task->priority < priority)
    {
        task->priority = priority;
    }
}
# endif

static void
__task_priority_compute(mpc_omp_task_t * task)
{
    const struct mpc_omp_conf_s * config = mpc_omp_conf_get();

    switch (config->task_priority_policy)
    {
        case (MPC_OMP_TASK_PRIORITY_POLICY_FIFO):
        case (MPC_OMP_TASK_PRIORITY_POLICY_FA1):
        {
            break ;
        }

        case (MPC_OMP_TASK_PRIORITY_POLICY_MA1):
        {
            task->priority = task->omp_priority_hint;
            break ;
        }

        case (MPC_OMP_TASK_PRIORITY_POLICY_MA2):
        case (MPC_OMP_TASK_PRIORITY_POLICY_SA1):
        {
            task->priority = task->omp_priority_hint;
            __task_priority_propagate_on_predecessors(task);
            break ;
        }

        case (MPC_OMP_TASK_PRIORITY_POLICY_SA2):
        {
            if (task->omp_priority_hint)
            {
                task->priority = INT_MAX;
                __task_priority_propagate_on_predecessors(task);
            }
            break ;
        }

        default:
        {
            assert(0);
            break ;
        }
    }
}

/* Initialization of a task structure */
void
_mpc_omp_task_init(
    mpc_omp_task_t *task,
    void (*func)(void *),
    void *data,
    unsigned int size,
    mpc_omp_task_property_t properties,
    mpc_omp_thread_t * thread
)
{
    assert(task != NULL);
    assert(thread != NULL);

    /* Reset all task infos to NULL */
    memset(task, 0, sizeof(mpc_omp_task_t));

    /* Set non null task infos field */
    task->func = func;
    task->data = data;
    task->size = size;
    task->icvs = thread->info.icvs;
    task->property = properties;

    task->parent = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    task->depth = (task->parent) ? task->parent->depth + 1 : 0;
    OPA_store_int(&(task->ref_counter), 0);

#if MPC_USE_SISTER_LIST
    task->children_lock = SCTK_SPINLOCK_INITIALIZER;
#endif
}

static inline int
___task_mvp_pqueue_init( struct mpc_omp_node_s* parent, struct mpc_omp_mvp_s* child, const mpc_omp_task_pqueue_type_t type  )
{
    int taskPqueueNodeRank, allocation;
    mpc_omp_task_mvp_infos_t * infos = &(child->task_infos);
    mpc_omp_task_pqueue_t * pqueue;

    if (parent->task_infos.pqueue[type])
    {
        allocation = 0;
        pqueue = parent->task_infos.pqueue[type];
        taskPqueueNodeRank = parent->task_infos.taskPqueueNodeRank[type];
    }
    else
    {
        allocation = 1;
        pqueue = (mpc_omp_task_pqueue_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_t));
        assert(pqueue);
        memset(pqueue, 0, sizeof(mpc_omp_task_pqueue_t));
        assert(child->threads);
        taskPqueueNodeRank = child->threads->tree_array_rank;
    }

    infos->pqueue[type] = pqueue;
    infos->lastStolen_pqueue[type] = NULL;
    infos->taskPqueueNodeRank[type] = taskPqueueNodeRank;
    infos->last_thief = -1;

    return allocation;
}

static inline int
___task_root_pqueue_init( struct mpc_omp_node_s *root, const mpc_omp_task_pqueue_type_t type )
{
    const int task_vdepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( root->instance->team, type );

    if ( task_vdepth != 0 )
    {
        return 0;
    }

    mpc_omp_task_node_infos_t * infos = &(root->task_infos);
    mpc_omp_task_pqueue_t * pqueue = (mpc_omp_task_pqueue_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_t));
    assert(pqueue);
    memset(pqueue, 0, sizeof(mpc_omp_task_pqueue_t));
    infos->pqueue[type] = pqueue;
    infos->lastStolen_pqueue[type] = NULL;
    infos->taskPqueueNodeRank[type] = 0;
    return 1;
}

/** Hierarchical stealing policy **/

static inline int
__task_get_victim_hierarchical( int globalRank, __UNUSED__ int index,
                                __UNUSED__ mpc_omp_task_pqueue_type_t type )
{
    return mpc_omp_tree_array_get_neighbor( globalRank, index );
}

/** Random stealing policy **/

static inline void
___task_random_victim_init( mpc_omp_generic_node_t *gen_node )
{
    int already_init;
    struct drand48_data *randBuffer;
    assert( gen_node != NULL );
    already_init = ( gen_node->type == MPC_OMP_CHILDREN_LEAF && MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_RANDBUFFER( gen_node->ptr.mvp ) );
    already_init |= ( gen_node->type == MPC_OMP_CHILDREN_NODE && MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_RANDBUFFER( gen_node->ptr.node ) );

    if ( already_init && gen_node->type == MPC_OMP_CHILDREN_NULL )
    {
        return;
    }

    randBuffer = ( struct drand48_data * ) mpc_omp_alloc( sizeof( struct drand48_data ) );
    assert( randBuffer );

    if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
    {
        assert( gen_node->ptr.mvp->threads );
        const int __globalRank = gen_node->ptr.mvp->threads->tree_array_rank;
        srand48_r( mpc_arch_get_timestamp_gettimeofday() * __globalRank, randBuffer );
        MPC_OMP_TASK_MVP_SET_TASK_PQUEUE_RANDBUFFER( gen_node->ptr.mvp, randBuffer );
    }
    else
    {
        assert( gen_node->type == MPC_OMP_CHILDREN_NODE );
        const int __globalRank = gen_node->ptr.node->tree_array_rank;
        srand48_r( mpc_arch_get_timestamp_gettimeofday() * __globalRank, randBuffer );
        MPC_OMP_TASK_NODE_SET_TASK_PQUEUE_RANDBUFFER( gen_node->ptr.node, randBuffer );
    }
}

static inline int
___task_get_victim_random( const int globalRank, __UNUSED__ const int index, mpc_omp_task_pqueue_type_t type )
{
    int victim, i;
    long int value;
    mpc_omp_thread_t *thread;
    mpc_omp_instance_t *instance;
    struct drand48_data *randBuffer;
    mpc_omp_generic_node_t *gen_node;
    /* Retrieve the information (microthread structure and current region) */
    assert( mpc_omp_tls != NULL );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread->instance );
    instance = thread->instance;
    assert( instance->tree_array );
    assert( instance->tree_nb_nodes_per_depth );
    assert( instance->tree_first_node_per_depth );
    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
    int nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];

    while ( nbVictims == 0 )
    {
        pqueueDepth--;
        nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    int first_rank = 0;

    for ( i = 0; i < pqueueDepth; i++ )
    {
        first_rank += thread->instance->tree_nb_nodes_per_depth[i];
    }

    assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );
    assert( gen_node->type != MPC_OMP_CHILDREN_NULL );

    if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
    {
        assert( gen_node->ptr.mvp->threads );
        randBuffer = MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_RANDBUFFER( thread->mvp );
    }
    else
    {
        assert( gen_node->type == MPC_OMP_CHILDREN_NODE );
        randBuffer = MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_RANDBUFFER( gen_node->ptr.node );
    }

    do
    {
        /* Get a random value */
        lrand48_r( randBuffer, &value );
        /* Select randomly a globalRank among neighbourhood */
        victim = ( value % nbVictims + first_rank );
    }
    while ( victim == globalRank );

    return victim;
}


static inline void
___task_allocate_larceny_order( mpc_omp_thread_t *thread )
{
    assert(thread);
    assert(thread->instance);
    assert(thread->instance->team);

    mpc_omp_team_t* team = thread->instance->team;
    const int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH(team, MPC_OMP_PQUEUE_TYPE_UNTIED);
    const int max_num_victims = thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    assert(max_num_victims >= 0);

    int *larceny_order = (int *) mpc_omp_alloc( max_num_victims * sizeof(int) );
    MPC_OMP_TASK_THREAD_SET_LARCENY_ORDER(thread, larceny_order);
}

static inline int
__task_get_victim_hierarchical_random( const int globalRank, const int index, mpc_omp_task_pqueue_type_t type )
{
    int i, j;
    long int value;
    int *order;
    mpc_omp_thread_t *thread;
    mpc_omp_instance_t *instance;
    mpc_omp_node_t *node;
    struct mpc_omp_mvp_s *mvp;
    struct drand48_data *randBuffer = NULL;
    mpc_omp_generic_node_t *gen_node;
    /* Retrieve the information (microthread structure and current region) */
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    mvp = thread->mvp;
    node = mvp->father;
    instance = thread->instance;

    if ( !MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
    {
        ___task_allocate_larceny_order( thread );
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );

    if ( index == 1 )
    {
        int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
        int nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
        int first_rank = 0;

        while ( nbVictims == 0 )
        {
            pqueueDepth--;
            nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
        }

        for ( i = 0; i < pqueueDepth; i++ )
        {
            first_rank += thread->instance->tree_nb_nodes_per_depth[i];
        }

        assert( globalRank >= 0 && globalRank < instance->tree_array_size );
        gen_node = &( instance->tree_array[globalRank] );
        assert( gen_node->type != MPC_OMP_CHILDREN_NULL );

        if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
        {
            assert( gen_node->ptr.mvp->threads );
            randBuffer = MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_RANDBUFFER( thread->mvp );
        }
        else
        {
            assert( gen_node->type == MPC_OMP_CHILDREN_NODE );
        }

        int tmporder[nbVictims - 1];

        for ( i = 0; i < nbVictims - 1; i++ )
        {
            order[i] = first_rank + i;
            order[i] = ( order[i] >= globalRank ) ? order[i] + 1 : order[i];
            tmporder[i] = order[i];
        }

        int parentrank = node->instance_global_rank - first_rank;
        int startrank = parentrank;
        int x = 1, k, l, cpt = 0;

        for ( i = 0; i < pqueueDepth; i++ )
        {
            for ( j = 0; j < ( node->barrier_num_threads - 1 ) * x; j++ )
            {
                lrand48_r( randBuffer, &value );
                k = ( value % ( ( node->barrier_num_threads - 1 ) * x - j ) + parentrank + j ) % ( nbVictims - 1 );
                l = ( parentrank + j ) % ( nbVictims - 1 );
                int tmp = tmporder[l];
                tmporder[l] = tmporder[k];
                tmporder[k] = tmp;
                order[cpt] = tmporder[l];
                cpt++;
            }

            x *= node->barrier_num_threads;
            startrank = startrank - node->rank * x;

            if ( node->father )
            {
                node = node->father;
            }

            parentrank = ( ( ( ( parentrank / x ) * x ) + x ) % ( x * node->barrier_num_threads ) + startrank ) % ( nbVictims - 1 );
            parentrank = ( parentrank >= globalRank - first_rank ) ? parentrank - 1 : parentrank;
        }
    }

    return order[index - 1];
}

static inline  void
___task_get_victim_random_order_prepare( const int globalRank, __UNUSED__ const int index, mpc_omp_task_pqueue_type_t type )
{
    int i;
    int *order;
    long int value;
    mpc_omp_thread_t *thread;
    mpc_omp_instance_t *instance;
    struct drand48_data *randBuffer;
    mpc_omp_generic_node_t *gen_node;
    /* Retrieve the information (microthread structure and current region) */
    assert( mpc_omp_tls != NULL );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;

    if ( !MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
    {
        ___task_allocate_larceny_order( thread );
    }

    assert( thread->instance );
    instance = thread->instance;
    assert( instance->tree_array );
    assert( instance->tree_nb_nodes_per_depth );
    assert( instance->tree_first_node_per_depth );
    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
    int nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    int first_rank = 0;

    while ( nbVictims == 0 )
    {
        pqueueDepth--;
        nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    for ( i = 0; i < pqueueDepth; i++ )
    {
        first_rank += thread->instance->tree_nb_nodes_per_depth[i];
    }

    assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );
    assert( gen_node->type != MPC_OMP_CHILDREN_NULL );

    if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
    {
        assert( gen_node->ptr.mvp->threads );
        randBuffer = MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_RANDBUFFER( thread->mvp );
        //TODO transfert data to current thread ...
        //randBuffer = gen_node->ptr.mvp->threads->task_infos.pqueue_randBuffer;
    }
    else
    {
        assert( gen_node->type == MPC_OMP_CHILDREN_NODE );
        randBuffer = gen_node->ptr.node->task_infos.pqueue_randBuffer;
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );

    for ( i = 0; i < nbVictims; i++ )
    {
        order[i] = first_rank + i;
    }

    for ( i = 0; i < nbVictims; i++ )
    {
        int tmp = order[i];
        lrand48_r( randBuffer, &value );
        int j = value % ( nbVictims - i );
        order[i] = order[i + j];
        order[i + j] = tmp;
    }
}

static inline int
__task_get_victim_random_order( int globalRank, int index,
                                mpc_omp_task_pqueue_type_t type )
{
    int *order;
    mpc_omp_thread_t *thread;
    /* Retrieve the information (microthread structure and current region) */
    assert( mpc_omp_tls );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;

    if ( index == 1 )
    {
        ___task_get_victim_random_order_prepare( globalRank, index, type );
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );
    return order[index - 1];
}

static inline  void
___task_get_victim_roundrobin_prepare( const int globalRank, __UNUSED__ const int index, mpc_omp_task_pqueue_type_t type )
{
    int i;
    int *order;
    mpc_omp_thread_t *thread;
    mpc_omp_instance_t *instance;
    /* Retrieve the information (microthread structure and current region) */
    assert( mpc_omp_tls != NULL );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread->instance );
    instance = thread->instance;
    assert( instance->tree_array );
    assert( instance->tree_nb_nodes_per_depth );
    assert( instance->tree_first_node_per_depth );
    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
    int nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    int first_rank = 0;

    while ( nbVictims == 0 )
    {
        pqueueDepth--;
        nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    for ( i = 0; i < pqueueDepth; i++ )
    {
        first_rank += thread->instance->tree_nb_nodes_per_depth[i];
    }

    int decal = ( globalRank - first_rank + 1 ) % ( nbVictims );

    if ( !MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
    {
        ___task_allocate_larceny_order( thread );
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );

    for ( i = 0; i < nbVictims; i++ )
    {
        order[i] = first_rank + ( decal + i ) % ( nbVictims );
    }
}

static inline int
__task_get_victim_roundrobin( const int globalRank, const int index, mpc_omp_task_pqueue_type_t type )
{
    int *order;
    mpc_omp_thread_t *thread;
    assert( mpc_omp_tls );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;

    if ( index == 1 )
    {
        ___task_get_victim_roundrobin_prepare( globalRank, index, type );
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );
    return order[index - 1];
}

static inline int
__task_get_victim_producer( int globalRank, __UNUSED__ int index, mpc_omp_task_pqueue_type_t type )
{
    int i, max_elt, victim, rank, nb_elt;
    mpc_omp_thread_t *thread;
    mpc_omp_task_pqueue_t *pqueue;
    mpc_omp_instance_t *instance;
    assert( mpc_omp_tls );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread->instance );
    instance = thread->instance;
    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
    int nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    int first_rank = 0;

    while ( nbVictims == 0 )
    {
        pqueueDepth--;
        nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    for ( i = 0; i < pqueueDepth; i++ )
    {
        first_rank += thread->instance->tree_nb_nodes_per_depth[i];
    }

    victim = ( globalRank == first_rank ) ? first_rank + 1 : first_rank;

    for ( max_elt = 0, i = 0; i < nbVictims - 1; i++ )
    {
        rank = i + first_rank;
        rank += ( rank >= globalRank ) ? 1 : 0;
        pqueue = ( mpc_omp_task_pqueue_t * ) __rank_get_task_pqueue( rank, type );
        nb_elt = ( pqueue ) ? OPA_load_int( &( pqueue->nb_elements ) ) : -1;

        if ( max_elt < nb_elt )
        {
            max_elt = nb_elt;
            victim = rank;
        }
    }

    return victim;
}


/* Compare nb element in first entry of tab
 */
static inline int
___task_sort_decr( const void *opq_a, const void *opq_b)
{
    const int *a = ( int * ) opq_a;
    const int *b = ( int * ) opq_b;
    return ( a[0] == b[0] ) ? b[1] - a[1] : b[0] - a[0];
}

static inline int
___task_get_victim_producer_order_prepare(__UNUSED__ const int globalRank, __UNUSED__ const int index, mpc_omp_task_pqueue_type_t type)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    mpc_omp_instance_t * instance = thread->instance;
    assert(instance);

    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( instance->team, type );
    int nbVictims = (!pqueueDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];

    while ( nbVictims == 0 )
    {
        pqueueDepth--;
        nbVictims = ( !pqueueDepth ) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    int first_rank = 0;
    int i;
    for (i = 0; i < pqueueDepth; i++)
    {
        first_rank += thread->instance->tree_nb_nodes_per_depth[i];
    }

    if ( !MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
    {
        ___task_allocate_larceny_order( thread );
    }

    int * order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    int nbElts[nbVictims][2];
    mpc_omp_task_pqueue_t * pqueue;

    for (i = 0; i < nbVictims; i++)
    {
        nbElts[i][1] = i + first_rank;
        pqueue = __rank_get_task_pqueue(nbElts[i][1], type);
        nbElts[i][0] = (pqueue) ? OPA_load_int(&pqueue->nb_elements) : -1;
    }

    qsort(nbElts, nbVictims, 2 * sizeof( int ), ___task_sort_decr);

    for ( i = 0; i < nbVictims; i++ )
    {
        order[i] = nbElts[i][1];
    }

    return 0;
}

static inline int
__task_get_victim_producer_order( const int globalRank, const int index, mpc_omp_task_pqueue_type_t type )
{
    int *order;
    mpc_omp_thread_t *thread;
    assert( mpc_omp_tls );
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;

    if ( index == 1 )
    {
        ___task_get_victim_producer_order_prepare( globalRank, index, type );
    }

    order = MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    assert( order );
    return order[index - 1];
}

static inline int
__task_get_victim_default( int globalRank, __UNUSED__ int index,
                           __UNUSED__ mpc_omp_task_pqueue_type_t type )
{
    return globalRank;
}


/* Return the ith victim for task stealing initiated from element at
 * 'globalRank' */
static inline int
__task_get_victim( int globalRank, int index,
                   mpc_omp_task_pqueue_type_t type )
{
    int victim;
    /* Retrieve the information (microthread structure and current region) */
    assert( mpc_omp_tls );
    mpc_omp_thread_t * thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread->instance );
    assert( thread->instance->team );
    mpc_omp_team_t * team = thread->instance->team;
    const int larcenyMode = MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );

    switch ( larcenyMode )
    {
        case MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL:
            victim = __task_get_victim_hierarchical( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_RANDOM:
            victim = ___task_get_victim_random( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER:
            victim = __task_get_victim_random_order( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_ROUNDROBIN:
            victim = __task_get_victim_roundrobin( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_PRODUCER:
            victim = __task_get_victim_producer( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
            victim = __task_get_victim_producer_order( globalRank, index, type );
            break;

        case MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM:
            victim = __task_get_victim_hierarchical_random( globalRank, index, type );
            break;

        default:
            victim = __task_get_victim_default( globalRank, index, type );
            break;
    }
    return victim;
}

static mpc_omp_task_t *
__task_larceny(mpc_omp_task_pqueue_type_t type)
{
    /* Retrieve the information (microthread structure and current region) */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    struct mpc_omp_mvp_s * mvp = mvp = thread->mvp;
    assert(mvp);

    struct mpc_omp_team_s * team = thread->instance->team;
    assert(team);

    const int larcenyMode = MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE(team);
    const int isMonoVictim = (larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM || larcenyMode == MPC_OMP_TASK_LARCENY_MODE_PRODUCER);

    /* Retrieve informations about task lists */
    int pqueueDepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH(team, type);
    int nbTaskPqueues = (!pqueueDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];

    while(nbTaskPqueues == 0)
    {
        pqueueDepth --;
        nbTaskPqueues = (!pqueueDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[pqueueDepth];
    }

    mpc_omp_task_t * task = NULL;
    mpc_omp_task_pqueue_t * pqueue;

    /* If there are task lists in which we could steal tasks */
    int rank = MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_NODE_RANK(mvp, type);
    int victim;
    if ( nbTaskPqueues > 1)
    {
        /* Try to steal inside the last stolen list */
        if(mpc_omp_conf_get()->task_steal_last_stolen)
        {
            if ((pqueue = MPC_OMP_TASK_MVP_GET_LAST_STOLEN_TASK_PQUEUE(mvp, type)))
            {
                if ((task = __task_pqueue_pop(pqueue)) != NULL)
                {
                    return task;
                }
            }
        }
    
        /* Try to steal to last thread that stole us a task */
        /* TODO : this is broken, last_thief is always `-1` */
        if(mpc_omp_conf_get()->task_steal_last_thief)
        {
            if (mvp->task_infos.last_thief != -1
                && (pqueue = __rank_get_task_pqueue(mvp->task_infos.last_thief, type)) != NULL)
            {
                victim = mvp->task_infos.last_thief;
                if ((task = __task_pqueue_pop(pqueue)) != NULL)
                {
                    return task;
                }
                mvp->task_infos.last_thief = -1;
            }
        }

        /* Get the rank of the ancestor containing the task pqueue */
        unsigned int nbVictims = (isMonoVictim) ? 1 : nbTaskPqueues ;
        /* Look for a task in all victims pqueues */
        unsigned int j;
        for (j = 1; j < nbVictims + 1; j++)
        {
            victim = __task_get_victim(rank, j, type);
            if(victim != rank)
            {
                pqueue = __rank_get_task_pqueue(victim, type);
                if(pqueue)
                {
                    task = __task_pqueue_pop(pqueue);
                }
                if(task)
                {
                    return task;
                }
            }
        }
    }
    return NULL;
}

static inline int
___task_node_pqueue_init( struct mpc_omp_node_s *parent, struct mpc_omp_node_s *child, const mpc_omp_task_pqueue_type_t type )
{
    int taskPqueueNodeRank, allocation;
    const int task_vdepth = MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( parent->instance->team, type );
    const int child_vdepth = child->depth - parent->instance->root->depth;

    if (child_vdepth < task_vdepth)
    {
        return 0;
    }

    mpc_omp_task_node_infos_t * infos = &( child->task_infos );
    mpc_omp_task_pqueue_t * pqueue;

    if (parent->depth >= task_vdepth)
    {
        allocation = 0;
        pqueue = parent->task_infos.pqueue[type];
        assert(pqueue);
        taskPqueueNodeRank = parent->task_infos.taskPqueueNodeRank[type];
    }
    else
    {
        assert( child->depth == task_vdepth );
        allocation = 1;
        pqueue = (mpc_omp_task_pqueue_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_t));
        assert(pqueue) ;
        memset(pqueue, 0, sizeof(mpc_omp_task_pqueue_t));
        taskPqueueNodeRank = child->tree_array_rank;
    }

    infos->pqueue[type] = pqueue;
    infos->lastStolen_pqueue[type] = NULL;
    infos->taskPqueueNodeRank[type] = taskPqueueNodeRank;
    return allocation;
}

void
_mpc_omp_task_node_info_init( struct mpc_omp_node_s* parent, struct mpc_omp_node_s* child )
{
    int ret, type;
    mpc_omp_instance_t* instance;

    assert( child );
    assert( parent );
    assert( parent->instance );

    instance = parent->instance;
    assert( instance->team );

    const int larcenyMode = MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

    for (type = 0, ret = 0; type < MPC_OMP_PQUEUE_TYPE_COUNT; type++)
    {
        ret += ___task_node_pqueue_init( parent, child, (mpc_omp_task_pqueue_type_t)type );
    }

    if (ret)
    {
        if( larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM ||
                larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER)
        {
            const int global_rank = child->tree_array_rank;
            ___task_random_victim_init( &( instance->tree_array[global_rank] ) );
        }
    }
}

void
_mpc_omp_task_mvp_info_init( struct mpc_omp_node_s* parent, struct mpc_omp_mvp_s* child )
{
    int ret, type;
    mpc_omp_instance_t* instance;

    assert( child );
    assert( parent );
    assert( parent->instance );

    instance = parent->instance;
    assert( instance->team );

    const int larcenyMode = MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

    for (type = 0, ret = 0; type < MPC_OMP_PQUEUE_TYPE_COUNT; type++)
    {
        ret += ___task_mvp_pqueue_init( parent, child, (mpc_omp_task_pqueue_type_t)type );
    }

    if (ret )
    {
        if( larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM ||
                larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER)
        {
            assert( child->threads );
            const int global_rank = child->threads->tree_array_rank;
            ___task_random_victim_init( &( instance->tree_array[global_rank] ) );
        }
    }
}

void
_mpc_omp_task_root_info_init( struct mpc_omp_node_s * root)
{
    int ret, type;
    mpc_omp_instance_t* instance;

    assert( root );
    assert( root->instance );

    instance = root->instance;
    assert( instance->team );

    const int larcenyMode = MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team );

    for (type = 0, ret = 0; type < MPC_OMP_PQUEUE_TYPE_COUNT; type++)
    {
        ret += ___task_root_pqueue_init( root, (mpc_omp_task_pqueue_type_t)type );
    }

    if (ret )
    {
        if( larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM ||
                larcenyMode == MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER)
        {
            ___task_random_victim_init( &( instance->tree_array[0] ) );
        }
    }
}

void
_mpc_omp_task_team_info_init(struct mpc_omp_team_s *team,
                            int depth)
{
    memset(&(team->task_infos), 0, sizeof(mpc_omp_task_team_infos_t));

    /* Ensure tasks lists depths are correct */
    MPC_OMP_TASK_TEAM_SET_PQUEUE_DEPTH(team, MPC_OMP_PQUEUE_TYPE_TIED, depth - 1);

    const int x = mpc_omp_conf_get()->pqueue_untied_depth;
    const int pqueue_untied_depth = (x < depth) ? x : depth - 1;
    MPC_OMP_TASK_TEAM_SET_PQUEUE_DEPTH(team, MPC_OMP_PQUEUE_TYPE_UNTIED, pqueue_untied_depth);

    const int y = mpc_omp_conf_get()->pqueue_new_depth;
    const int pqueue_new_depth = (y < depth) ? y : depth - 1;
    MPC_OMP_TASK_TEAM_SET_PQUEUE_DEPTH(team, MPC_OMP_PQUEUE_TYPE_NEW, pqueue_new_depth);

    /* task nesting */
    const int task_depth_threshold = mpc_omp_conf_get()->task_depth_threshold;
    MPC_OMP_TASK_TEAM_SET_TASK_NESTING_MAX(team, task_depth_threshold);

    /* larceny mode */
    const int task_larceny_mode = mpc_omp_conf_get()->task_larceny_mode;
    MPC_OMP_TASK_TEAM_SET_TASK_LARCENY_MODE(team, task_larceny_mode);
}

/*********
 * UTILS *
 *********/

int mpc_omp_task_parse_larceny_mode(char * mode)
{
	if(!strcmp(mode, "hierarchical"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL;
	}else if(!strcmp(mode, "random"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_RANDOM;
	}else if(!strcmp(mode, "random_order"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER;
	}else if(!strcmp(mode, "round_robin"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_ROUNDROBIN;
	}else if(!strcmp(mode, "producer"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_PRODUCER;
	}else if(!strcmp(mode, "producer_order"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_PRODUCER_ORDER;
	}else if(!strcmp(mode, "hierarchical_random"))
	{
		return MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM;
	}else
	{
		bad_parameter("Could not parse mpc.omp.task.larceny = '%s' it must be one of: hierarchical, random, random_order, round_robin, producer, producer_order, hierarchical_random", mode);
	}
	
	return -1;
}

/******************
 * TASK INTERFACE *
 ******************/

void
_mpc_omp_task_ref_parent_task(mpc_omp_task_t * task)
{
    assert(task);
    assert(task->parent);
    OPA_incr_int(&(task->parent->children_count));
}

void
_mpc_omp_task_unref_parent_task(mpc_omp_task_t *task)
{
    assert(task);
    assert(task->parent);
    OPA_decr_int(&(task->parent->children_count));
}

/**
 * Perform a taskwait construct.
 *
 * Can be the entry point for a compiler.
 *
 * > The taskwait region includes an implicit task scheduling point in the current task
 * > region. The current task region is suspended at the task scheduling point until all child
 * > tasks that it generated before the taskwait region complete execution.
 */
void
_mpc_omp_task_wait(void)
{
    mpc_omp_init();

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->info.num_threads > 0);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    /* Look for a children tasks list */
    while (OPA_load_int(&(task->children_count)))
    {
        /* Schedule any other task */
        _mpc_omp_task_schedule();
    }
}

/**
 * Get the next task to run on this thread, and pop it from the thread list
 * If no task are found, a task is stolen from another thread
 * If no tasks could be stolen, NULL is returned, meaning there is no task to run.
 * \param the thread
 * \return the task
 */
static inline mpc_omp_task_t *
__task_schedule_next(mpc_omp_thread_t * thread)
{
    mpc_omp_task_t * task;

    /* current thread tied tasks */
    task = __thread_task_pop_type(thread, MPC_OMP_PQUEUE_TYPE_TIED);
    if (task) return task;

    /* current thread untied tasks */
    task = __thread_task_pop_type(thread, MPC_OMP_PQUEUE_TYPE_UNTIED);
    if (task) return task;

    /* current thread new tasks */
    task = __thread_task_pop_type(thread, MPC_OMP_PQUEUE_TYPE_NEW);
    if (task) return task;

    /* other thread untied tasks */
    task = __task_larceny(MPC_OMP_PQUEUE_TYPE_UNTIED);
    if (task) return task;

    /* other thread new tasks */
    task = __task_larceny(MPC_OMP_PQUEUE_TYPE_NEW);
    if (task) return task;

    return NULL;
}

static inline void
___thread_bind_task(
    mpc_omp_thread_t * thread,
    mpc_omp_task_t * task,
    mpc_omp_local_icv_t * icv)
{

    /* Restore thread icv envionnement */
    thread->info.icvs = *icv;

    /* Attach task and thread */
    MPC_OMP_TASK_THREAD_SET_CURRENT_TASK(thread, task);
    task->thread = thread;
}

/**
 * Run the given task as a function.
 * Delete it on completion
 */
static void
__task_run_as_function(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = __thread_task_coherency(task);
    mpc_omp_task_t * curr = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);

    ___thread_bind_task(thread, task, &(task->icvs));

    mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_STARTED);
    MPC_OMP_TASK_TRACE_SCHEDULE(task);
    task->func(task->data);
    mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_COMPLETED);
    MPC_OMP_TASK_TRACE_SCHEDULE(task);

    ___thread_bind_task(thread, curr, &(curr->icvs));

    /* delete the task */
    __task_finalize(task);
}

# if MPC_OMP_TASK_COMPILE_FIBER

/* Entry point for a task when it is first run under it own context */
static void
__task_start_routine(__UNUSED__ void * unused)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);

    mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_STARTED);
    task->func(task->data);
    mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_COMPLETED);

    sctk_setcontext_no_tls(task->fiber->exit);
}

static mpc_omp_task_fiber_t *
__thread_generate_new_task_fiber(mpc_omp_thread_t * thread)
{
    mpc_omp_task_fiber_t * fiber;
    if (thread->task_infos.fiber)
    {
        fiber = thread->task_infos.fiber;
        thread->task_infos.fiber = NULL;
    }
    else
    {
# if MPC_OMP_TASK_USE_RECYCLERS
        fiber = (mpc_omp_task_fiber_t *) mpc_common_recycler_alloc(&(thread->task_infos.fiber_recycler));
# else
        fiber = (mpc_omp_task_fiber_t *) mpc_omp_alloc(sizeof(mpc_omp_task_fiber_t) + MPC_OMP_TASK_FIBER_STACK_SIZE);
# endif
        sctk_makecontext_no_tls(
            &(fiber->initial),
            fiber,
            (void (*)(void *)) __task_start_routine,
            (char *)(fiber + 1),
            MPC_OMP_TASK_FIBER_STACK_SIZE
        );
    }

    return fiber;
}

/**
 * Start or resume a task by the current thread, using the task context.
 * Current task context is then restored before this function returns
 * \param task Target task to execute by the current thread
 * \param exit The task to return after 'task' ends
 */
static void
__task_run_with_fiber(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = __thread_task_coherency(task);
    mpc_omp_task_t * curr = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);

    assert(curr != task);

    if (task->fiber == NULL)
    {
        assert(!mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_STARTED));
        task->fiber = __thread_generate_new_task_fiber(thread);
    }

    ___thread_bind_task(thread, task, &(task->icvs));
    task->fiber->exit = &(thread->task_infos.mctx);
    MPC_OMP_TASK_TRACE_SCHEDULE(task);
    sctk_mctx_t * mctx = mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_STARTED) ? &(task->fiber->current) : &(task->fiber->initial);
    sctk_swapcontext_no_tls(task->fiber->exit, mctx);
    MPC_OMP_TASK_TRACE_SCHEDULE(task);
    
    ___thread_bind_task(thread, curr, &(curr->icvs));
    
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_COMPLETED))
    {
        __task_finalize(task);
    }
    else
    {
        if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_BLOCKED))
        {
            /* add callbackhronous polling function if set */
            if (thread->task_infos.callback_to_push)
            {
                mpc_omp_callback(thread->task_infos.callback_to_push);
                thread->task_infos.callback_to_push = NULL;
            }
        }
        else
        {
            /** TODO :
             * The task yielded but was not blocked.
             * We may want to defer it into `tied` or `untied` priority queues
             * For now, this is an error
             */
            not_implemented();
        }
    }
}

# endif /* MPC_OMP_TASK_COMPILE_FIBER */

/**
 * Run the task
 * @param task : the task
 */
static inline void
__task_run(mpc_omp_task_t * task)
{
# if MPC_OMP_TASK_COMPILE_FIBER
    if (MPC_OMP_TASK_FIBER_ENABLED && mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_HAS_FIBER))
    {
        return __task_run_with_fiber(task);
    }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
    return __task_run_as_function(task);
}

static inline int
__task_profile_propagate_should_stop(mpc_omp_thread_t * thread)
{
    // for now, we only have 1 thread that propagates priority at a time
    // once we enable parallel priority propagation, next line might be interesting
    // to reduce contention.
    // return thread->rank == ntasks;
    int nready = OPA_load_int(&(thread->instance->task_infos.ntasks_ready));
    return nready > 0;
}

static void
__task_profile_propagate(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = thread->instance;
    assert(instance);

    // TAKE LOCK: only 1 thread may propagate at once
    if (mpc_common_spinlock_trylock(&(instance->task_infos.propagation.lock)) == 0)
    {
        int version = OPA_fetch_and_incr_int(&(instance->task_infos.propagation.version));

        // list to find leaves
        mpc_omp_task_list_t * down = &(instance->task_infos.propagation.down);

        // list to propagate priorities up
        mpc_omp_task_list_t * up = &(instance->task_infos.propagation.up);

        // list of blocked tasks
        mpc_omp_task_list_t * blocked_tasks = &(thread->instance->task_infos.blocked_tasks);

        assert(blocked_tasks->type == MPC_OMP_TASK_LIST_TYPE_SCHEDULER);
        assert(down->type == MPC_OMP_TASK_LIST_TYPE_UP_DOWN);
        assert(up->type == MPC_OMP_TASK_LIST_TYPE_UP_DOWN);

        assert(__task_list_is_empty(down));
        assert(__task_list_is_empty(up));

        // RETRIEVE BLOCKED TASKS
        mpc_common_spinlock_lock(&(blocked_tasks->lock));
        {
            mpc_omp_task_t * task = blocked_tasks->head;
            while (task)
            {
                assert(OPA_load_int(&(task->ref_counter)) > 0);
                task->propagation_version = version;
                __task_ref(task);
                __task_list_push_to_tail(down, task);
                task = task->next[MPC_OMP_TASK_LIST_TYPE_SCHEDULER];
            }
        }
        mpc_common_spinlock_unlock(&(blocked_tasks->lock));

        // FIND LEAVES
        while (!__task_list_is_empty(down))
        {
            mpc_omp_task_t * task = __task_list_pop_from_head(down);
            assert(OPA_load_int(&(task->ref_counter)) > 0);
            MPC_OMP_TASK_LOCK(task);
            {
                if (task->dep_node.successors)
                {
                    mpc_omp_task_dep_list_elt_t * elt = task->dep_node.successors;
                    while (elt)
                    {
                        mpc_omp_task_t * successor = elt->task;
                        if (successor->propagation_version < version)
                        {
                            successor->propagation_version = version;
                            __task_ref(successor);
                            __task_list_push_to_tail(down, successor);
                        }
                        elt = elt->next;
                    }
                }
                else
                {
                    __task_ref(task);
                    __task_list_push_to_tail(up, task);
                }
            }
            MPC_OMP_TASK_UNLOCK(task);
            __task_unref(task);

            // PROPAGATE UP FROM LEAVES
            while (!__task_list_is_empty(up))
            {
                mpc_omp_task_t * task = __task_list_pop_from_head(up);
                if (OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_READY)
                {
                    // TODO replace lock by a reference counter on 'predecessors' list
                    MPC_OMP_TASK_LOCK(task);
                    {
                        if (OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_DEP_TASK_READY)
                        {
                            __task_priority_compute_profile(task);
                            if (task->dep_node.predecessors)
                            {
                                assert(task->dep_node.predecessors);
                                mpc_omp_task_dep_list_elt_t * elt = task->dep_node.predecessors;
                                while (elt)
                                {
                                    mpc_omp_task_t * predecessor = elt->task;
                                    if (predecessor->priority < task->priority - 1)
                                    {
                                        predecessor->priority = task->priority - 1;
                                        __task_ref(predecessor);
                                        __task_list_push_to_head(up, predecessor);
                                    }
                                    elt = elt->next;
                                }
                            }
                        }
                    }
                    MPC_OMP_TASK_UNLOCK(task);
                }
                __task_unref(task);
            }
        }

        // RELEASE LOCK
        mpc_common_spinlock_unlock(&(instance->task_infos.propagation.lock));
    }
}

/**
 * Task scheduling.
 *
 * Try to find a task to be scheduled and execute it on the calling omp thread.
 * This is the main function (it calls an internal function).
 */
void
_mpc_omp_task_schedule(void)
{
    _mpc_omp_callback_run(MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE);

    /* get current thread */
    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    mpc_omp_task_t * task = __task_schedule_next(thread);

    if (task)
    {
# if MPC_OMP_TASK_COMPILE_TRACE
        task->schedule_id = OPA_fetch_and_incr_int(&(thread->instance->task_infos.next_schedule_id));
# endif
        __task_run(task);
    }
    else
    {
        /* Famine detected */
        if (mpc_omp_conf_get()->task_priority_policy == MPC_OMP_TASK_PRIORITY_POLICY_FA1)
        {
            //MPC_OMP_TASK_TRACE_FAMINE_OVERLAP(0);
            __task_profile_propagate();
            //MPC_OMP_TASK_TRACE_FAMINE_OVERLAP(1);
        }
    }
}

/**
 * Run or defer the task depending on its properties
 * This should be call after an omp task constructor
 * \see MPC_OMP_TASK_PROP_UNDEFERRED
 * \see MPC_OMP_TASK_PROP_INCLUDED
 */
void
_mpc_omp_task_process(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = __thread_task_coherency(task);

    /** if the task should run undeferredly */
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNDEFERRED)
        || task->depth > mpc_omp_conf_get()->task_depth_threshold)
    {
        __task_run(task);
    }
    else
    {
        /* defer the task */
        mpc_omp_task_pqueue_t * pqueue = __thread_get_task_pqueue(thread, MPC_OMP_PQUEUE_TYPE_NEW);
        __task_pqueue_push(pqueue, task);
        
        if (OPA_load_int(&(thread->instance->task_infos.ntasks)) >= mpc_omp_conf_get()->maximum_tasks)
        {
            _mpc_omp_task_schedule(); 
        }
    }
}

# if MPC_OMP_TASK_COMPILE_FIBER

static void
__taskyield_return(void)
{
    assert(MPC_OMP_TASK_FIBER_ENABLED);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    mpc_omp_task_t * curr = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    sctk_swapcontext_no_tls(&(curr->fiber->current), curr->fiber->exit);
}

static inline void
__thread_requeue_task(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);

    mpc_omp_task_pqueue_type_t type;
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNTIED))
    {
        type = MPC_OMP_PQUEUE_TYPE_UNTIED;
    }
    else
    {
        type = MPC_OMP_PQUEUE_TYPE_TIED;
    } 

    mpc_omp_task_pqueue_t * pqueue = __thread_get_task_pqueue(thread, type);
    assert(pqueue);

    __task_pqueue_push(pqueue, task);
}

void
mpc_omp_taskyield_unblock(mpc_omp_event_handle_t * event)
{
    assert(MPC_OMP_TASK_FIBER_ENABLED);
    assert(event->type & MPC_OMP_EVENT_TASK_BLOCK);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    mpc_omp_task_t * task = (mpc_omp_task_t *) event->attr;
    assert(task);
    assert(mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_BLOCKED));

    /* the event is fulfilled */
    event->type = event->type & ~(MPC_OMP_EVENT_TASK_BLOCK);
    event->attr = NULL;

    /* mark the task as unblocked */
    mpc_omp_task_unset_property(&(task->property), MPC_OMP_TASK_PROP_BLOCKED);
    mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_UNBLOCKED);
    OPA_store_int(&(task->dep_node.status), MPC_OMP_TASK_DEP_TASK_READY);

    /* remove the task from the blocked list */
    mpc_omp_task_list_t * list = &(thread->instance->task_infos.blocked_tasks);
    mpc_common_spinlock_lock(&(list->lock));
    {
        __task_list_remove(list, task);
    }
    mpc_common_spinlock_unlock(&(list->lock));

    /* if task context was suspended */
    if (MPC_OMP_TASK_FIBER_ENABLED &&
        mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_HAS_FIBER))
    {
        /* the task can now be re-queued */
        __thread_requeue_task(task);
    }
}

TODO("The callback function may unblock the task before it is actually suspended. This is a race condition issue that must be fixed.");
void
mpc_omp_taskyield_block(mpc_omp_event_handle_t * event, mpc_omp_callback_t * callback)
{
    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    mpc_omp_task_t * curr = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(curr);

    if (event)
    {
        event->type = event->type | MPC_OMP_EVENT_TASK_BLOCK;
        event->attr = (void *) curr;
    }

    mpc_omp_task_set_property(&(curr->property), MPC_OMP_TASK_PROP_BLOCKED);
    OPA_store_int(&(curr->dep_node.status), MPC_OMP_TASK_DEP_TASK_NOT_READY);

    /* add the task to the blocked list */
    mpc_omp_task_list_t * list = &(thread->instance->task_infos.blocked_tasks);
    mpc_common_spinlock_lock(&(list->lock));
    {
        __task_list_push(list, curr);
    }
    mpc_common_spinlock_unlock(&(list->lock));

    /* if task context can be suspended, return to parent context */
    if (MPC_OMP_TASK_FIBER_ENABLED &&
        mpc_omp_task_property_isset(curr->property, MPC_OMP_TASK_PROP_HAS_FIBER))
    {
        thread->task_infos.callback_to_push = callback;  
        __taskyield_return();
    }
    /* otherwise, busy-loop until unblock */
    else
    {
        /* register the callback function if any */
        if (callback) mpc_omp_callback(callback);
        while (OPA_load_int(&(curr->dep_node.status)) != MPC_OMP_TASK_DEP_TASK_READY)
        {
            _mpc_omp_task_schedule();
        }
    }

}

# endif /* MPC_OMP_TASK_COMPILE_FIBER */

static void
__taskyield_stack(void)
{
    _mpc_omp_task_schedule();
}

/**
 * Taskyield construct.
 *
 * The current task may be suspended, in favor of execution of another task.
 *
 * Called when encountering a taskyield construct
 * This is a scheduling point.
 */
void
_mpc_omp_task_yield(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    if (!thread)
    {
        return ;
    }

    switch (mpc_omp_conf_get()->task_yield_mode)
    {
        case (MPC_OMP_TASK_YIELD_MODE_STACK):
        {
            __taskyield_stack();
            break ;
        }

# if MPC_OMP_TASK_COMPILE_FIBER
        case (MPC_OMP_TASK_YIELD_MODE_CIRCULAR):
        {
            fprintf(stderr, "Circular task-yield is not supported, please use 'mpc_omp_taskyield_block()'\n");
            assert(0);
            break ;
        }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */

        default:
        {
            break ;
        }
    }

    /* thread may have changed */
    /* thread = (mpc_omp_thread_t *)mpc_omp_tls; */
    /* assert(current_task == MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread)); */
}

void
mpc_omp_task_extra(char * label, int extra_clauses)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

# if MPC_OMP_TASK_COMPILE_TRACE
    thread->task_infos.incoming.label = label;
# endif /* MPC_OMP_TASK_COMPILE_TRACE */
    thread->task_infos.incoming.extra_clauses = extra_clauses;
}

/*
 * Create a new openmp task
 *
 * This function can be called when encountering an 'omp task' construct
 *
 * \param task the task buffer to use (may be NULL, and so, a new task is allocated)
 * \param fn the task entry point
 * \param data the task data (fn parameters)
 * \param cpyfn function to copy the data
 * \param arg_size
 * \param arg_align
 * \param properties
 */
mpc_omp_task_t *
_mpc_omp_task_new(
    mpc_omp_task_t * task,
    void (*fn)(void *),
    void *data,
    void (*cpyfn)(void *, void *),
    long arg_size,
    long arg_align,
    mpc_omp_task_property_t properties,
    void ** depend,
    int priority_hint)
{
    /* Intialize the OpenMP environnement (if needed) */
    mpc_omp_init();

    /* Retrieve the information (microthread structure and current region) */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    /* increment number of existing tasks */
    OPA_incr_int(&(thread->instance->task_infos.ntasks));

    /* default pading */
    const long align_size = (arg_align == 0) ? 8 : arg_align ;

    // mpc_omp_task + arg_size
    const unsigned long task_size = _mpc_omp_task_align_single_malloc(sizeof(mpc_omp_task_t), align_size);
    const unsigned long data_size = _mpc_omp_task_align_single_malloc(arg_size, align_size);
    const unsigned long task_tot_size = task_size + data_size;

    if (task)   /* TODO */
    {
        TODO("Non-null task buffer is not implemented yet");
        assert(0);
    }

# if MPC_OMP_TASK_USE_RECYCLERS
    task = mpc_common_nrecycler_alloc(&(thread->task_infos.task_recycler), task_tot_size);
# else
    task = mpc_omp_alloc(task_tot_size);
# endif
    void * task_data = arg_size ? (void *) (((unsigned char *)task) + task_size) : NULL;

    assert(task);
    assert(arg_size == 0 || task_data);

    if (arg_size > 0)
    {
        if (cpyfn)
        {
            cpyfn(task_data, data);
        }
        else
        {
            memcpy(task_data, data, arg_size);
        }
    }

    /* Initialize the task */
    _mpc_omp_task_init(task, fn, task_data, task_tot_size, properties, thread);

    _mpc_omp_taskgroup_add_task(task);
    _mpc_omp_task_ref_parent_task(task);
    __task_ref(task);

    /* extra parameters given to the mpc thread for this task */
# if MPC_OMP_TASK_COMPILE_TRACE
    task->omp_priority_hint = priority_hint;
    if (thread->task_infos.incoming.label)
    {
        strncpy(task->label, thread->task_infos.incoming.label, MPC_OMP_TASK_LABEL_MAX_LENGTH);
        thread->task_infos.incoming.label = NULL;
    }
    task->uid = OPA_fetch_and_incr_int(&(thread->instance->task_infos.next_task_uid));
# endif /* MPC_OMP_TASK_COMPILE_TRACE */
    if (thread->task_infos.incoming.extra_clauses & MPC_OMP_CLAUSE_USE_FIBER)
    {
        mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_HAS_FIBER);
    }
    memset(&(thread->task_infos.incoming), 0, sizeof(thread->task_infos.incoming));

    /* if there is no dependencies, process the task now */
    if (!mpc_omp_task_property_isset(properties, MPC_OMP_TASK_PROP_DEPEND))
    {
        __task_priority_compute(task);
        MPC_OMP_TASK_TRACE_CREATE(task);
        return task;
    }

    mpc_omp_task_t * parent = (mpc_omp_task_t *)MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);

    /* Is it possible ?! See GOMP source code   */
    assert((uintptr_t)depend[0] > (uintptr_t)0);

    if (!(parent->dep_node.htable))
    {
        parent->dep_node.htable = __task_dep_htable_new(__task_dep_hash);
        assert(parent->dep_node.htable);
    }

    /* initialize dependency node */
    mpc_common_spinlock_init(&(task->lock), 0);
    task->dep_node.predecessors = NULL;
    OPA_store_int(&(task->dep_node.npredecessors), 0);
    task->dep_node.successors = NULL;
    OPA_store_int(&(task->dep_node.nsuccessors), 0);
    OPA_store_int(&(task->dep_node.ref_predecessors), 0);
    OPA_store_int(&(task->dep_node.status), MPC_OMP_TASK_DEP_TASK_NOT_READY);
    task->dep_node.max_depth = 0;
    task->dep_node.min_depth = INT_MAX;

    /* parse dependencies */
    __task_process_deps(task, parent->dep_node.htable, depend);

    /* update number of predecessors */
    int npredecessors = OPA_load_int(&(task->dep_node.npredecessors));
    if (npredecessors == 0) task->dep_node.min_depth = 0;
    OPA_store_int(&(task->dep_node.ref_predecessors), npredecessors);

    __task_priority_compute(task);

    MPC_OMP_TASK_TRACE_CREATE(task);

    return task;
}

/**
 * MPC OpenMP task constrctor ABI
 */
void
mpc_omp_task(
    mpc_omp_task_t * task,
    void (*fn)(void *), void *data,
    void (*cpyfn)(void *, void *),
    long arg_size, long arg_align,
    mpc_omp_task_property_t properties,
    void **depend,
    int priority_hint)
{
    mpc_omp_init();

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    /* create the task */
    task = _mpc_omp_task_new(task, fn, data, cpyfn, arg_size, arg_align, properties, depend, priority_hint); 
    assert(task);

    /* if the task has no dependency */
    if (!mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_DEPEND)) 
    {
        _mpc_omp_task_process(task);
    }
    else
    {
        if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNDEFERRED))
        {
            while (OPA_load_int(&(task->dep_node.ref_predecessors)))
            {
                _mpc_omp_task_schedule();
            }
        }

        if (OPA_load_int(&(task->dep_node.ref_predecessors)) == 0)
        {
            if (OPA_cas_int(
                        &(task->dep_node.status),
                        MPC_OMP_TASK_DEP_TASK_NOT_READY,
                        MPC_OMP_TASK_DEP_TASK_READY) == MPC_OMP_TASK_DEP_TASK_NOT_READY)
            {
                _mpc_omp_task_process(task);
            }
        }
    }
}

/*************
 * TASKGROUP *
 *************/

void
_mpc_omp_task_taskgroup_start( void )
{
    mpc_omp_init();

    mpc_omp_thread_t * thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread );

    mpc_omp_task_t * current_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
    assert( current_task );

    mpc_omp_task_taskgroup_t * new_taskgroup = ( mpc_omp_task_taskgroup_t * ) mpc_omp_alloc( sizeof( mpc_omp_task_taskgroup_t ) );
    assert( new_taskgroup );

    /* Init new task group and store it in current task */
    memset( new_taskgroup, 0, sizeof( mpc_omp_task_taskgroup_t ) );
    new_taskgroup->prev = current_task->taskgroup;
    current_task->taskgroup = new_taskgroup;
}

void
_mpc_omp_task_taskgroup_end( void )
{
    mpc_omp_task_t *current_task = NULL;        /* Current task execute     */
    mpc_omp_thread_t *thread = NULL; /* thread private data      */
    mpc_omp_task_taskgroup_t *taskgroup = NULL; /* new_taskgroup allocated  */
    thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert( thread );
    current_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
    assert( current_task );
    taskgroup = current_task->taskgroup;

    if ( !taskgroup )
    {
        return;
    }

    while ( OPA_load_int( &( taskgroup->children_num ) ) )
    {
        _mpc_omp_task_schedule();
        _mpc_omp_task_wait();
    }

    current_task->taskgroup = taskgroup->prev;
    mpc_omp_free( taskgroup );
}


/************
 * TASKLOOP *
 ************/

static unsigned long _mpc_omp_task_loop_compute_num_iters(long start, long end,
        long step) {
    long decal = (step > 0) ? -1 : 1;

    if ((end - start) * decal >= 0)
        return 0;

    return (end - start + step + decal) / step;
}

static unsigned long
__loop_taskloop_compute_loop_value(
        long iteration_num, unsigned long num_tasks,
        long step, long *taskstep,
        unsigned long *extra_chunk)
{
    long compute_taskstep;
    unsigned long compute_num_tasks, compute_extra_chunk;

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    compute_taskstep = step;
    compute_extra_chunk = iteration_num;
    compute_num_tasks = num_tasks;

    if (!(compute_num_tasks))
    {
        compute_num_tasks = (thread->info.num_threads) ? thread->info.num_threads : 1;
    }

    if (num_tasks >= (unsigned long)iteration_num)
    {
        compute_num_tasks = iteration_num;
    }
    else
    {
        const long quotient = iteration_num / compute_num_tasks;
        const long reste = iteration_num % compute_num_tasks;
        compute_taskstep = quotient * step;
        if (reste)
        {
            compute_taskstep += step;
            compute_extra_chunk = reste - 1;
        }
    }

    *taskstep = compute_taskstep;
    *extra_chunk = compute_extra_chunk;
    return compute_num_tasks;
}

static unsigned long
__loop_taskloop_compute_loop_value_grainsize(
        long iteration_num, unsigned long num_tasks,
        long step, long *taskstep,
        unsigned long *extra_chunk)
{
    long compute_taskstep;
    unsigned long grainsize, compute_num_tasks, compute_extra_chunk;

    grainsize = num_tasks;

    compute_taskstep = step;
    compute_extra_chunk = iteration_num;
    compute_num_tasks = iteration_num / grainsize;

    if (compute_num_tasks <= 1) {
        compute_num_tasks = 1;
    } else {
        if (compute_num_tasks > grainsize) {
            const long mul = num_tasks * grainsize;
            const long reste = iteration_num - mul;
            compute_taskstep = grainsize * step;
            if (reste) {
                compute_taskstep += step;
                compute_extra_chunk = iteration_num - mul - 1;
            }
        } else {
            const long quotient = iteration_num / compute_num_tasks;
            const long reste = iteration_num % compute_num_tasks;
            compute_taskstep = quotient * step;
            if (reste) {
                compute_taskstep += step;
                compute_extra_chunk = reste - 1;
            }
        }
    }

    *taskstep = compute_taskstep;
    *extra_chunk = compute_extra_chunk;
  return compute_num_tasks;
}

void mpc_omp_task_loop(void (*fn)(void *), void *data,
                     void (*cpyfn)(void *, void *), long arg_size,
                     long arg_align, unsigned flags, unsigned long num_tasks,
                     __UNUSED__ int priority, long start, long end, long step)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    long taskstep;
    unsigned long extra_chunk, i;

    mpc_omp_init();

    const long num_iters = _mpc_omp_task_loop_compute_num_iters(start, end, step);

#if OMPT_SUPPORT
   _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_begin, num_iters );
#endif /* OMPT_SUPPORT */

   if (num_iters)
   {
       if (!(flags & GOMP_TASK_FLAG_GRAINSIZE)) {
           num_tasks = __loop_taskloop_compute_loop_value(num_iters, num_tasks, step, &taskstep, &extra_chunk);
       } else {
           num_tasks = __loop_taskloop_compute_loop_value_grainsize(num_iters, num_tasks, step, &taskstep, &extra_chunk);
           taskstep = (num_tasks == 1) ? end - start : taskstep;
       }

       if (!(flags & GOMP_TASK_FLAG_NOGROUP)) {
           _mpc_omp_task_taskgroup_start();
       }

       for (i = 0; i < num_tasks; i++) {
           mpc_omp_task_property_t properties = 0;
           mpc_omp_task_t * new_task = _mpc_omp_task_new(NULL, fn, data, cpyfn, arg_size, arg_align, properties, NULL, 0);
           ((long *)new_task->data)[0] = start;
           ((long *)new_task->data)[1] = start + taskstep;
           start += taskstep;
           taskstep -= (i == extra_chunk) ? step : 0;
           TODO("handle the if clause, and flags here");
           // for now, run task undeferredly
           mpc_omp_task_set_property(&(new_task->property), MPC_OMP_TASK_PROP_UNDEFERRED);
           _mpc_omp_task_process(new_task);
       }

       if (!(flags & GOMP_TASK_FLAG_NOGROUP)) {
           _mpc_omp_task_taskgroup_end();
       }
   }

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_end, 0 );
#if MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* MPCOMPT_HAS_FRAME_SUPPORT */
#endif /* OMPT_SUPPORT */
}

void mpc_omp_task_loop_ull(__UNUSED__ void (*fn)(void *), __UNUSED__ void *data,
                         __UNUSED__ void (*cpyfn)(void *, void *), __UNUSED__ long arg_size,
                         __UNUSED__ long arg_align, __UNUSED__ unsigned flags,
                         __UNUSED__  unsigned long num_tasks, __UNUSED__ int priority,
                         __UNUSED__ unsigned long long start, __UNUSED__ unsigned long long end,
                         __UNUSED__ unsigned long long step)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    not_implemented();

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

static inline void
__task_init_initial(mpc_omp_thread_t * thread)
{
    mpc_omp_task_t * initial_task = (mpc_omp_task_t*)mpc_omp_alloc( sizeof(mpc_omp_task_t));
    assert( initial_task );

    mpc_omp_task_property_t properties = 0;
    mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_INITIAL);
    mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_IMPLICIT);
    _mpc_omp_task_init(initial_task, NULL, NULL, 0, properties, thread);
    snprintf(initial_task->label, MPC_OMP_TASK_LABEL_MAX_LENGTH, "initial-%p", initial_task);

#ifdef MPC_OMP_USE_MCS_LOCK
    thread->task_infos.opaque = sctk_mcslock_alloc_ticket();
#else /* MPC_OMP_USE_MCS_LOCK  */
    thread->task_infos.opaque = NULL;
#endif /* MPC_OMP_USE_MCS_LOCK  */

    MPC_OMP_TASK_THREAD_SET_CURRENT_TASK(thread, initial_task);
    OPA_store_int(&(initial_task->children_count), 0);
}

# if MPC_OMP_TASK_USE_RECYCLERS
static inline void
__task_init_recyclers(mpc_omp_thread_t * thread)
{
    const struct mpc_omp_conf_s * config = mpc_omp_conf_get();
# if MPC_OMP_TASK_COMPILE_FIBER
    if (MPC_OMP_TASK_FIBER_ENABLED)
    {
        mpc_common_recycler_init (
            &(thread->task_infos.fiber_recycler),
            MPC_OMP_TASK_ALLOCATOR,
            MPC_OMP_TASK_DEALLOCATOR,
            sizeof(mpc_omp_task_fiber_t) + MPC_OMP_TASK_FIBER_STACK_SIZE,
            config->fiber_recycler_capacity
        );
    }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */

    unsigned int capacities[32];
    unsigned int i;
    for (i = 0 ; i < 32 ; i++)
    {
        capacities[i] = config->task_recycler_capacity;
    }
    mpc_common_nrecycler_init (
        &(thread->task_infos.task_recycler),
        MPC_OMP_TASK_ALLOCATOR,
        MPC_OMP_TASK_DEALLOCATOR,
        capacities
    );

    /* task dep. node list recycler */
    mpc_common_recycler_init(
        &(thread->task_infos.list_recycler),
        MPC_OMP_TASK_ALLOCATOR,
        MPC_OMP_TASK_DEALLOCATOR,
        sizeof(mpc_omp_task_dep_list_elt_t),
        config->task_recycler_capacity
    );
}
# endif

void
_mpc_omp_task_tree_init(mpc_omp_thread_t * thread)
{
    if (!MPC_OMP_TASK_THREAD_IS_INITIALIZED(thread))
    {
# if MPC_OMP_TASK_USE_RECYCLERS
        __task_init_recyclers(thread);
# endif
        __task_init_initial(thread);
        MPC_OMP_TASK_THREAD_CMPL_INIT(thread);
    }
}

/**
 * Deinitialize the thread task tree
 */
void
_mpc_omp_task_tree_deinit(mpc_omp_thread_t * thread)
{
    assert(thread);

    mpc_omp_task_t * task = thread->task_infos.current_task;
    assert(task);
    assert(task->dep_node.successors == NULL);
    assert(task->dep_node.predecessors == NULL);
    assert(OPA_load_int(&(task->dep_node.npredecessors)) == 0);
    assert(OPA_load_int(&(task->dep_node.nsuccessors)) == 0);

    if (task->dep_node.htable)
    {
        __task_dep_htable_delete(task->dep_node.htable);
        task->dep_node.htable = NULL;
    }

    assert(OPA_load_int(&(thread->instance->task_infos.ntasks_ready)) == 0);
    // N.B: this may not be true, because every threads deinitialize concurrently
    // assert(OPA_load_int(&(thread->instance->task_infos.ntasks)) == 0);
}
