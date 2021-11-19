/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:59 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Jerome Clet-Ortega <cletortegaj@ocre.cea.fr>                       # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
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
#include "omp_gomp_constants.h"

/************************
 * OMP INSTANCE RELATED
 ***********************/

 /**
  * Return true if the runtime should now execute task sequentially
  */
static inline int
__task_reached_thresholds(mpc_omp_task_t * task)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    return (task->depth > mpc_omp_conf_get()->task_depth_threshold
           || OPA_load_int(&(thread->instance->task_infos.ntasks_allocated)) >= mpc_omp_conf_get()->maximum_tasks
           || OPA_load_int(&(thread->instance->task_infos.ntasks_ready)) >= mpc_omp_conf_get()->maximum_ready_tasks);
}

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
        int check = !task->statuses.started || mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNTIED) || mpc_omp_tls == task->thread;

        if (!check)
        {
            fprintf(stderr, "%d %d %d\n",
                task->statuses.started,
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
    return __task_list_pop_from_tail(list);
}

/**************************
 * PRIORITY QUEUE (rb-tree)
 *************************/

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

static mpc_omp_task_pqueue_t *
__task_pqueue_new(void)
{
    mpc_omp_task_pqueue_t * tree = (mpc_omp_task_pqueue_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_t));
    assert(tree);
    memset(tree, 0, sizeof(mpc_omp_task_pqueue_t));
    return tree;
}

static mpc_omp_task_pqueue_node_t *
__task_pqueue_node_new(
    mpc_omp_task_pqueue_node_t * parent,
    char color,
    size_t priority)
{
    mpc_omp_task_pqueue_node_t * node = (mpc_omp_task_pqueue_node_t *) mpc_omp_alloc(sizeof(mpc_omp_task_pqueue_node_t));
    assert(node);

    node->parent        = parent;
    node->priority      = priority;
    node->color         = color;
    node->left          = NULL;
    node->right         = NULL;

    memset(&(node->tasks), 0, sizeof(mpc_omp_task_list_t));
    node->tasks.type    = MPC_OMP_TASK_LIST_TYPE_SCHEDULER;

    return node;
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
__task_pqueue_transplant(
        mpc_omp_task_pqueue_t * tree,
        mpc_omp_task_pqueue_node_t * u,
        mpc_omp_task_pqueue_node_t * v)
{
    if (u->parent == NULL)
    {
        tree->root = v;
    }
    else
    {
        if (u == u->parent->left)
        {
            u->parent->left = v;
        }
        else
        {
            u->parent->right = v;
        }
    }
    if (v)
    {
        v->parent = u->parent;
    }
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
    sprintf(filepath, "%lf-%p-%s-tree.dot", omp_get_wtime(), tree, label);
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
__task_pqueue_insert_fixup(
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

    __task_pqueue_insert_fixup(tree, node);
    __task_pqueue_coherency_check(tree);
    return node;
}

static void
__task_pqueue_delete_fixup(
    mpc_omp_task_pqueue_t * tree,
    mpc_omp_task_pqueue_node_t * x)
{
    while (x && x != tree->root && x->color == 'B')
    {
        assert(x->parent);
        if (x == x->parent->left)
        {
            /* case 1 */
            mpc_omp_task_pqueue_node_t * w = x->parent->right;
            if (w->color == 'R')
            {
                w->color = 'B';
                x->parent->color = 'R';
                __task_pqueue_rotate_left(tree, x->parent);
                w = x->parent->right;
            }

            /* case 2 */
            if ((!w->left || w->left->color == 'B')
                && (!w->right || w->right->color == 'B'))
            {
                w->color = 'R';
                x = x->parent;
            }
            else
            {
                /* case 3 */
                if (!w->right || w->right->color == 'B')
                {
                    w->left->color = 'B';
                    w->color = 'R';
                    __task_pqueue_rotate_right(tree, w);
                    w = x->parent->right;
                }

                /* case 4 */
                w->color = x->parent->color;
                x->parent->color = 'B';
                w->right->color = 'B';
                __task_pqueue_rotate_left(tree, x->parent);
                x = tree->root;
            }
        }
        /* same as above with 'right' and 'left' exchanged */
        else
        {
            /* case 1 */
            mpc_omp_task_pqueue_node_t * w = x->parent->left;
            if (w->color == 'R')
            {
                w->color = 'B';
                x->parent->color = 'R';
                __task_pqueue_rotate_right(tree, x->parent);
                w = x->parent->left;
            }

            /* case 2 */
            if ((!w->left || w->left->color == 'B')
                && (!w->right || w->right->color == 'B'))
            {
                w->color = 'R';
                x = x->parent;
            }
            else
            {
                /* case 3 */
                if (!w->left || w->left->color == 'B')
                {
                    w->right->color = 'B';
                    w->color = 'R';
                    __task_pqueue_rotate_left(tree, w);
                    w = x->parent->left;
                }

                /* case 4 */
                w->color = x->parent->color;
                x->parent->color = 'B';
                w->left->color = 'B';
                __task_pqueue_rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }
    if (x)
    {
        x->color = 'B';
    }
}

/** O(log2(n)) down there */
static mpc_omp_task_t *
__task_pqueue_pop_top_priority(mpc_omp_task_pqueue_t * tree)
{
    assert(tree->root);

    /* get rightmost task list */
    mpc_omp_task_pqueue_node_t * z = tree->root;
    while (z->right)
    {
        z = z->right;
    }

    /* pop a task from the list (FIFO) */
    mpc_omp_task_list_t * list = &(z->tasks);
    mpc_omp_task_t * task = __task_list_pop(list);
    assert(task);

    /* if the list is now empty, pop it from the tree */
    if (OPA_load_int(&(list->nb_elements)) == 0)
    {
        mpc_omp_task_pqueue_node_t * x;

        if (z->left == NULL)
        {
            x = z->right;
            __task_pqueue_transplant(tree, z, z->right);
        }
        else
        {
            x = z->left;
            __task_pqueue_transplant(tree, z, z->left);
        }

        if (z->color == 'B')
        {
            __task_pqueue_delete_fixup(tree, x);
        }

        mpc_omp_free(z);
    }
    return task;
}

static mpc_omp_task_t *
__task_pqueue_pop(mpc_omp_task_pqueue_t * pqueue)
{
    mpc_omp_task_t * task = NULL;
    if (OPA_load_int(&(pqueue->nb_elements)))
    {
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
        task->pqueue = pqueue;
        switch (mpc_omp_conf_get()->task_list_policy)
        {
            case (MPC_OMP_TASK_LIST_POLICY_FIFO):
            {
                __task_list_push_to_head(&(node->tasks), task);
                break ;
            }
            case (MPC_OMP_TASK_LIST_POLICY_LIFO):
            {
                __task_list_push_to_tail(&(node->tasks), task);
                break ;
            }
            default:
            {
                fprintf(stderr, "Invalid task list policy\n");
                not_implemented();
                break ;
            }
        }
    }
    mpc_common_spinlock_unlock(&(pqueue->lock));

    OPA_incr_int(&(pqueue->nb_elements));
    __instance_incr_ready_tasks();

# if MPC_OMP_BARRIER_COMPILE_COND_WAIT
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    pthread_cond_t * cond = &(thread->instance->task_infos.work_cond);
    pthread_cond_signal(cond);
# endif /* MPC_OMP_BARRIER_COMPILE_COND_WAIT */
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

/* increase task reference counter by 1 */
static inline void
__task_ref(mpc_omp_task_t * task)
{
    assert(task);
    assert(OPA_load_int(&(task->ref_counter)) > 0);
    OPA_incr_int(&(task->ref_counter));
}

void __task_delete(mpc_omp_task_t * task);

/* Decrease task reference counter by 1
 * if it reaches 0, the task is deleted */
static void
__task_unref(mpc_omp_task_t * task)
{
    assert(task);
    assert(OPA_load_int(&(task->ref_counter)) > 0);
    if (OPA_fetch_and_decr_int(&(task->ref_counter)) == 1)
    {
        __task_delete(task);
    }
}

static void
__task_ref_parent_task(mpc_omp_task_t * parent)
{
    assert(parent);
    OPA_incr_int(&(parent->children_count));
    __task_ref(parent);
}

static void
__task_unref_parent_task(mpc_omp_task_t * parent)
{
    assert(parent);
    OPA_decr_int(&(parent->children_count));
    __task_unref(parent);
}

static void
__task_ref_parallel_region(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    assert(thread->instance);
    assert(thread->instance->team);

    mpc_omp_parallel_region_t * region = &(thread->instance->team->info);
    OPA_incr_int(&(region->task_ref));
}

static void
__task_unref_parallel_region(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);
    assert(thread->instance);
    assert(thread->instance->team);

    mpc_omp_parallel_region_t * region = &(thread->instance->team->info);
    OPA_decr_int(&(region->task_ref));
}

static inline void
__task_list_elt_delete(mpc_omp_task_list_elt_t * elt)
{
    __task_unref(elt->task);
    mpc_omp_free(elt);
}

static void
__task_list_delete(mpc_omp_task_list_elt_t * list)
{
    while (list)
    {
        mpc_omp_task_list_elt_t * next = list->next;
        __task_list_elt_delete(list);
        list = next;
    }
}

# if MPC_OMP_TASK_COMPILE_FIBER
static inline void
__task_delete_fiber(mpc_omp_task_t * task)
{
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
}
# endif /* MPC_OMP_TASK_COMPILE_FIBER */

/****************
 * HTABLE RELATED
 ****************/

static mpc_omp_task_list_elt_t *
__task_list_elt_new(mpc_omp_task_list_elt_t * list, mpc_omp_task_t * task)
{
    assert(task);
    __task_ref(task);

    mpc_omp_task_list_elt_t * elt = (mpc_omp_task_list_elt_t *) mpc_omp_alloc(sizeof(mpc_omp_task_list_t));
    assert(elt);

    elt->task = task;
    elt->next = list;
    return elt;
}

static inline int
__task_in_list(mpc_omp_task_list_elt_t * list, mpc_omp_task_t * task)
{
#if 0
    while (list)
    {
        if (list->task == task) return 1;
        list = list->next;
    }
    return 0;
#endif
    return (list && list->task == task);
}

static inline void
__task_precedence_constraints(mpc_omp_task_t * predecessor, mpc_omp_task_t * successor)
{
    /* the 2 tasks must be siblings */
    assert(predecessor->parent == successor->parent);
    assert(predecessor->depth == successor->depth);

    if (OPA_load_int(&(predecessor->dep_node.status)) < MPC_OMP_TASK_STATUS_FINALIZED)
    {
        MPC_OMP_TASK_LOCK(predecessor);
        {
            if (OPA_load_int(&(predecessor->dep_node.status)) < MPC_OMP_TASK_STATUS_FINALIZED)
            {
                if (!__task_in_list(predecessor->dep_node.successors, successor))
                {
                    predecessor->dep_node.successors = __task_list_elt_new(predecessor->dep_node.successors, successor);
                    OPA_incr_int(&(predecessor->dep_node.nsuccessors));

                    successor->dep_node.predecessors = __task_list_elt_new(successor->dep_node.predecessors, predecessor);
                    OPA_incr_int(&(successor->dep_node.npredecessors));
                    OPA_incr_int(&(successor->dep_node.ref_predecessors));
                }

                if (predecessor->dep_node.top_level + 1 > successor->dep_node.top_level)
                {
                    successor->dep_node.top_level = predecessor->dep_node.top_level + 1;
                }
            }
        }
        MPC_OMP_TASK_UNLOCK(predecessor);
    }
}

static mpc_omp_task_dep_list_elt_t *
__task_dep_list_append(
        mpc_omp_task_t * task,
        mpc_omp_task_dep_htable_entry_t * entry,
        mpc_omp_task_dep_list_elt_t * list)
{
    assert(task);
    assert(entry);

    // mpc_omp_task_dep_list_elt_t * dep = (mpc_omp_task_dep_list_elt_t *) mpc_omp_alloc(sizeof(mpc_omp_task_dep_list_elt_t));
    mpc_omp_task_dep_list_elt_t * dep = task->dep_node.dep_list + task->dep_node.dep_list_size;
    assert(dep);

    dep->task   = task;
    dep->entry  = entry;

    dep->prev   = NULL;
    dep->next   = list;
    if (list) list->prev = dep;

    ++task->dep_node.dep_list_size;

    return dep;
}

/** Note : only 1 thread may run this function for a given `task->parent` */
    static void
__task_process_mpc_dep(
        mpc_omp_task_t * task,
        void * addr,
        mpc_omp_task_dep_type_t type)
{
    /* Retrieve entry for the given address, or generate a new one.
     * Also performon redundancy check */
//    mpc_common_spinlock_lock(&(task->parent->dep_node.hmap_lock));
    {
        mpc_omp_task_dep_htable_entry_t * entry;
        HASH_FIND_PTR(task->parent->dep_node.hmap, &addr, entry);
        if (entry == NULL)
        {
            entry = (mpc_omp_task_dep_htable_entry_t *) mpc_omp_alloc(sizeof(mpc_omp_task_dep_htable_entry_t));
            assert(entry);
            entry->addr     = addr;
            entry->out      = NULL;
            entry->ins      = NULL;
            entry->inoutset = NULL;
            entry->task_uid = 0;
            mpc_common_spinlock_init(&(entry->lock), 1);
            HASH_ADD_PTR(task->parent->dep_node.hmap, addr, entry);
        }
        else
        {
            mpc_common_spinlock_lock(&(entry->lock));
        }

        /* redundancy check */
        if (entry->task_uid != task->uid)
        {
            entry->task_uid = task->uid;
            /* case 1.1 - the generated task is dependant of previous 'in' */
            if (    type == MPC_OMP_TASK_DEP_OUT ||
                    type == MPC_OMP_TASK_DEP_INOUT ||
                    type == MPC_OMP_TASK_DEP_INOUTSET ||
                    type == MPC_OMP_TASK_DEP_MUTEXINOUTSET)
            {
                mpc_omp_task_dep_list_elt_t * in = entry->ins;
                while (in)
                {
                    __task_precedence_constraints(in->task, task);
                    in = in->next;
                }
            }

            /* case 1.2 - the generated task is dependant of previous 'out' and 'inout' */
            if (    type == MPC_OMP_TASK_DEP_IN ||
                    type == MPC_OMP_TASK_DEP_OUT ||
                    type == MPC_OMP_TASK_DEP_INOUT ||
                    type == MPC_OMP_TASK_DEP_INOUTSET ||
                    type == MPC_OMP_TASK_DEP_MUTEXINOUTSET)
            {
                if (entry->out)
                {
                    assert(entry->out->next == NULL);
                    assert(entry->out->prev == NULL);
                    __task_precedence_constraints(entry->out->task, task);
                }
            }

            /* case 1.3 - the generated task is dependant of previous 'inoutset' */
            if (    type == MPC_OMP_TASK_DEP_IN ||
                    type == MPC_OMP_TASK_DEP_OUT ||
                    type == MPC_OMP_TASK_DEP_INOUT ||
                    type == MPC_OMP_TASK_DEP_MUTEXINOUTSET)
            {
                mpc_omp_task_dep_list_elt_t * inoutset = entry->inoutset;
                while (inoutset)
                {
                    __task_precedence_constraints(inoutset->task, task);
                    inoutset = inoutset->next;
                }
            }

            /* case 1.4 - the generated task is dependant of previous 'mutexinoutset' */
# if 0
            if (    type == MPC_OMP_TASK_DEP_IN ||
                    type == MPC_OMP_TASK_DEP_OUT ||
                    type == MPC_OMP_TASK_DEP_INOUT ||
                    type == MPC_OMP_TASK_DEP_INOUTSET)
            {
                not_implemented();
            }
# endif

            /************************************************************/
            /* finally, clean-up and save dependencies for future tasks */
            /************************************************************/

            /* case 2.1 - if generating dependency is an 'in'
             *  - add it to the list of 'ins'
             */
            if (type == MPC_OMP_TASK_DEP_IN)
            {
                entry->ins = __task_dep_list_append(task, entry, entry->ins);
            }

            /* case 2.2 - if generating dependency is 'out' or 'inout'
             *  - delete siblings 'out' - it is now dependant of generating task
             *  - delete siblings 'ins' - they are all dependant of generating task
             *  - delete siblings 'inoutset' - they are all dependant of generating task
             */
            else if (type == MPC_OMP_TASK_DEP_OUT || type == MPC_OMP_TASK_DEP_INOUT)
            {
                entry->ins = NULL;
                entry->inoutset = NULL;
                entry->out = __task_dep_list_append(task, entry, NULL);
            }

            /* case 2.3 - if generating dependency is 'inoutset' */
            else if (type == MPC_OMP_TASK_DEP_INOUTSET)
            {
                entry->inoutset = __task_dep_list_append(task, entry, entry->inoutset);
            }

            else if (type == MPC_OMP_TASK_DEP_MUTEXINOUTSET)
            {
                not_implemented();
            }
        }
        mpc_common_spinlock_unlock(&(entry->lock));
    }
//    mpc_common_spinlock_unlock(&(task->parent->dep_node.hmap_lock));
}

/**
 * @param task - the task
 * @param depend - gomp formatted 'depend' array
 */
static void
__task_process_deps(mpc_omp_task_t * task, void ** depend)
{
    assert(task);

    /* retrieve threads */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;

    /* mpc specific dependencies */
    mpc_omp_task_dependency_t * dependencies = thread->task_infos.incoming.dependencies;
    uintptr_t ndependencies_type = thread->task_infos.incoming.ndependencies_type;
    thread->task_infos.incoming.dependencies = NULL;
    thread->task_infos.incoming.ndependencies_type = 0;

    /* if no dependencies, return */
    if (dependencies == NULL)
    {
        if (depend == NULL)     return ;
        if (depend[0] == NULL)  return;
    }

    /* compiler dependencies */
    uintptr_t omp_ndeps_tot = depend ? (uintptr_t) depend[0] : 0;
    uintptr_t omp_ndeps_out = depend ? (uintptr_t) depend[1] : 0;

    /* runtime specific dependencies */
    uintptr_t mpc_ndeps = 0;
    uintptr_t i, j;
    for (i = 0 ; i < ndependencies_type ; ++i) mpc_ndeps += dependencies[i].addrs_size;

    /* total number of dependencies */
    uintptr_t ndeps = omp_ndeps_tot + mpc_ndeps;
    assert(ndeps > 0);

    /* We may allocate useless dep_list_elt_t when there is redundancy.
     * 1) count unique element
     * 2) allocate
     * For now, we may allocate too much memory in redundancy cases
     */
    task->dep_node.dep_list = (mpc_omp_task_dep_list_elt_t *) malloc(sizeof(mpc_omp_task_dep_list_elt_t) * ndeps);
    assert(task->dep_node.dep_list);

     /* Ensure that dependencies are processed in the following order
     * So that we have correct behaviour on redundancy
     * 1) 'OUT' or 'INOUT'
     * 2) 'MUTEX_INOUTSET'
     * 3) 'INOUTSET'
     * 4) 'IN'
     */

    /* openmp 'out' dependencies */
    for (i = 0 ; i < omp_ndeps_out ; ++i)   __task_process_mpc_dep(task, depend[2 + i], MPC_OMP_TASK_DEP_OUT);

    /* mpc dependencies */
    for (i = 0 ; i < ndependencies_type ; ++i)
    {
        mpc_omp_task_dependency_t * dependency = dependencies + i;
        for (j = 0 ; j < dependency->addrs_size ; ++j)
        {
            __task_process_mpc_dep(task, dependency->addrs[j], dependency->type);
        }
    }

    /* openmp 'in' dependencies */
    for (i = omp_ndeps_out ; i < omp_ndeps_tot ; ++i)   __task_process_mpc_dep(task, depend[2 + i], MPC_OMP_TASK_DEP_IN);
}

static void
__task_delete_dependencies_hmap(mpc_omp_task_t * task)
{
    if (task->dep_node.hmap)
    {
        /* clean data dependency hmap */
        mpc_omp_task_dep_htable_entry_t * entry, * tmp;
        HASH_ITER(hh, task->dep_node.hmap, entry, tmp)
        {
            assert(!entry->out);
            assert(!entry->ins);
            assert(!entry->inoutset);
            HASH_DEL(task->dep_node.hmap, entry);
            mpc_omp_free(entry);
        }
        assert(HASH_COUNT(task->dep_node.hmap) == 0);
        HASH_CLEAR(hh, task->dep_node.hmap);
    }
}

static void
__task_finalize_deps_list(mpc_omp_task_t * task)
{
    assert(task->parent);

//    mpc_common_spinlock_lock(&(task->parent->dep_node.hmap_lock));
    {
        unsigned int i;
        for (i = 0 ; i < task->dep_node.dep_list_size ; ++i)
        {
            mpc_omp_task_dep_list_elt_t * elt = task->dep_node.dep_list + i;
            mpc_omp_task_dep_htable_entry_t * entry = elt->entry;
            assert(entry);

            mpc_common_spinlock_lock(&(entry->lock));
            {
                mpc_omp_task_dep_list_elt_t * next = elt->task_next;

                /* pop this task dependency element for the parent htable lists */
                if (entry->out == elt)
                {
                    assert(elt->next == NULL);
                    assert(elt->prev == NULL);
                    entry->out = NULL;
                }
                else if (entry->ins == elt)
                {
                    assert(elt->prev == NULL);
                    entry->ins = entry->ins->next;
                    if (entry->ins) entry->ins->prev = NULL;
                }
                else if (entry->inoutset == elt)
                {
                    assert(elt->prev == NULL);
                    entry->inoutset = entry->inoutset->next;
                    if (entry->inoutset) entry->inoutset->prev = NULL;
                }
                else
                {
                    if (elt->prev) elt->prev->next = elt->next;
                    if (elt->next) elt->next->prev = elt->prev;
                }

                // TODO : maybe delete the hmap entry if it is now empty ?

                elt = next;
            }
            mpc_common_spinlock_unlock(&(entry->lock));
        }
        free(task->dep_node.dep_list);
    }
//    mpc_common_spinlock_unlock(&(task->parent->dep_node.hmap_lock));
}

/** Given task completed -> fulfill its successors dependencies */
static void
__task_finalize_deps(mpc_omp_task_t * task)
{
    assert(task);

    /* if the task has no dependencies, there is no successors to release, return now */
    if (!mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_DEPEND)) return ;

    MPC_OMP_TASK_LOCK(task);
    {
        OPA_store_int(&(task->dep_node.status), MPC_OMP_TASK_STATUS_FINALIZED);
    }
    MPC_OMP_TASK_UNLOCK(task);

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    /* delete task dependencies */
    __task_finalize_deps_list(task);

    /* Resolve successor's data dependency */
    mpc_omp_task_list_elt_t * succ = task->dep_node.successors;
    while (succ)
    {
        MPC_OMP_TASK_TRACE_DEPENDENCY(task, succ->task);

        /* if successor's dependencies are fullfilled, process it */
        if (OPA_fetch_and_decr_int(&(succ->task->dep_node.ref_predecessors)) == 1)
        {
            if (OPA_load_int(&(succ->task->dep_node.status)) == MPC_OMP_TASK_STATUS_NOT_READY)
            {
                _mpc_omp_task_process(succ->task);
            }
        }

        /* process next successor */
        succ = succ->next;
    }

    /* Since 'task' is completed, no longer need to access it predecessors */
    __task_list_delete(task->dep_node.predecessors);
    task->dep_node.predecessors = NULL;
}


/** called whenever a task completed it execution */
static void
__task_finalize(mpc_omp_task_t * task)
{
    assert(task->statuses.completed || task->statuses.cancelled);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

# if MPC_OMP_TASK_COMPILE_FIBER
    __task_delete_fiber(task);
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
    mpc_omp_taskgroup_del_task(task);
    __task_unref_parallel_region();
    __task_finalize_deps(task);

    mpc_omp_task_t * parent = task->parent;
    __task_unref(task);                 /* _mpc_omp_task_init */
    __task_unref_parent_task(parent);   /* _mpc_omp_task_init */
}

/* task deletion function, when it is no longer referenced anywhere */
void
__task_delete(mpc_omp_task_t * task)
{
    assert(OPA_load_int(&(task->children_count)) == 0);
    assert(OPA_load_int(&(task->ref_counter)) == 0);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    /* release dependency hmap */
    __task_delete_dependencies_hmap(task);

     /* Release successors */
     __task_list_delete(task->dep_node.successors);
     task->dep_node.successors = NULL;

    MPC_OMP_TASK_TRACE_DELETE(task);

# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_common_nrecycler_recycle(&(thread->task_infos.task_recycler), task, task->size);
# else
    mpc_omp_free(task);
# endif /* MPC_OMP_TASK_USE_RECYCLERS */

    /* decrement number of existing tasks */
    OPA_decr_int(&(thread->instance->task_infos.ntasks_allocated));
}

static void
__task_priority_propagate_on_predecessors(mpc_omp_task_t * task)
{
    if (task->dep_node.predecessors && OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_STATUS_FINALIZED)
    {
        MPC_OMP_TASK_LOCK(task);
        {
            if (OPA_load_int(&(task->dep_node.status)) < MPC_OMP_TASK_STATUS_FINALIZED)
            {
                /* if the task is not in queue, propagate the priority */
                switch (mpc_omp_conf_get()->task_priority_policy)
                {
                    case (MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_NOOP):
                    {
                        break ;
                    }

                    case (MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_DECR):
                    {
                        mpc_omp_task_list_elt_t * predecessor = task->dep_node.predecessors;
                        while (predecessor)
                        {
                            if (predecessor->task->priority < task->priority - 1)
                            {
                                predecessor->task->priority = task->priority - 1;
                                __task_priority_propagate_on_predecessors(predecessor->task);
                            }
                            predecessor = predecessor->next;
                        }
                        break ;
                    }

                    case (MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_EQUAL):
                    {
                        mpc_omp_task_list_elt_t * predecessor = task->dep_node.predecessors;
                        while (predecessor)
                        {

                            if (predecessor->task->priority < task->priority)
                            {
                                predecessor->task->priority = task->priority;
                                __task_priority_propagate_on_predecessors(predecessor->task);
                            }
                            predecessor = predecessor->next;
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
        }
        MPC_OMP_TASK_UNLOCK(task);
    }
}

/* compute task priority depending on policy set */
static void
__task_priority_compute(mpc_omp_task_t * task)
{
    const struct mpc_omp_conf_s * config = mpc_omp_conf_get();

    switch (config->task_priority_policy)
    {
        case (MPC_OMP_TASK_PRIORITY_POLICY_ZERO):
        {
            task->priority = 0;
            break ;
        }

        case (MPC_OMP_TASK_PRIORITY_POLICY_COPY):
        {
            task->priority = task->omp_priority_hint;
            break ;
        }

        case (MPC_OMP_TASK_PRIORITY_POLICY_CONVERT):
        {
            if (task->omp_priority_hint)
            {
                task->priority = INT_MAX;
            }
            break ;
        }

        default:
        {
            not_implemented();
            break ;
        }
    }

    if (config->task_priority_propagation_policy != MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_NOOP)
    {
        __task_priority_propagate_on_predecessors(task);
    }
}

#ifdef WIP

static inline int
_mpc_omp_task_compute_priorities_exit_condition(mpc_omp_instance_t * instance, mpc_omp_thread_t * thread)
{
    return (instance->task_infos.ntasks_ready > 0);
}

/* TODO : WIP
 * Propagate priorities asynchronously.
 * This routine should be called during idle periods.
 */
void _mpc_omp_task_compute_priorities(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_instance_t * instance = thread->instance;
    assert(instance);

    mpc_omp_task_list * leaves = &(thread->task_infos.leaves);

    /* PHASE 1      - Update leaves for this thread */
    /*  PHASE 1.1   - Clean up remains of previous call */
    for (mpc_omp_task_t * leaf : leaves)
    {
        /* if this previous leaf became ready, or now has successors, remove it from the list */
        if (leaf->ready || leaf->successor)
        {
            leaves.pop(leaf);
        }
    }

    /*  PHASE 1.2   - Find new leaves */
    for (mpc_omp_task_t * root : roots)
    {
        down.push(successor);
    }
    while (!down.is_empty())
    {
        mpc_omp_task_t * task = down.pop();
        if (task->successor)
        {
            for (mpc_omp_task_t * successor : task->successor)
            {
                down.push(successor);
            }
        }
        else
        {
            leaves.push(task);
        }
    }

    // TODO : phase 1.2, do it in breadth first search in the TDG,
    // so that leaves are sorted by their depth

    /* PHASE 2 - From leaves, go up to roots and set priorities */
    for (mpc_omp_task_t * leaf : leaves)
    {
        mpc_omp_task_t * task = leaf;
        while (task)
        {
            for (mpc_omp_task_t * predecessor : task->predecessor)
            {
                stack.push(predecessor);
            }
            set_priority(task);
            task = stack.pop();
        }
    }
}
#endif /* WIP */

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
        pqueue = __task_pqueue_new();
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
    mpc_omp_task_pqueue_t * pqueue = __task_pqueue_new();
    infos->pqueue[type] = pqueue;
    infos->lastStolen_pqueue[type] = NULL;
    infos->taskPqueueNodeRank[type] = 0;
    return 1;
}

/***************
 * TASK STEALING
 ***************/

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

TODO("steal more than 1 task at a time");
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

    /* Retrieve information about task lists */
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
            if ((pqueue = MPC_OMP_TASK_MVP_GET_LAST_STOLEN_TASK_PQUEUE(mvp, type)) != NULL)
            {
                if ((task = __task_pqueue_pop(pqueue)) != NULL) return task;
            }
        }

        /* Try to steal to last thread that stole us a task */
        TODO("last_thief is always `-1`");
        if(mpc_omp_conf_get()->task_steal_last_thief)
        {
            if (mvp->task_infos.last_thief != -1
                    && (pqueue = __rank_get_task_pqueue(mvp->task_infos.last_thief, type)) != NULL)
            {
                victim = mvp->task_infos.last_thief;
                if ((task = __task_pqueue_pop(pqueue)) != NULL) return task;
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
            if (victim != rank)
            {
                pqueue = __rank_get_task_pqueue(victim, type);
                if ((task = __task_pqueue_pop(pqueue)) != NULL) return task;
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
_mpc_omp_task_team_info_init(struct mpc_omp_team_s * team, int depth)
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

static inline void
__task_run_coherency_check_pre(mpc_omp_task_t * task)
{
    __thread_task_coherency(task);

    assert(OPA_load_int(&(task->dep_node.ref_predecessors)) == 0);
    assert(OPA_load_int(&(task->ref_counter)) > 0);

    assert(task->statuses.started           == false);
    assert(task->statuses.completed         == false);
    assert(task->statuses.blocking          == false);
    assert(task->statuses.blocked           == false);
    assert(task->statuses.in_blocked_list   == false);
}

static inline void
__task_run_coherency_check_post(mpc_omp_task_t * task)
{
    __thread_task_coherency(task);

    assert(OPA_load_int(&(task->dep_node.ref_predecessors)) == 0);
    assert(OPA_load_int(&(task->ref_counter)) > 0);

    assert(task->statuses.started           == true);
    assert(task->statuses.completed         == true);
    assert(task->statuses.blocking          == false);
    assert(task->statuses.blocked           == false);
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

    __task_run_coherency_check_pre(task);
    task->statuses.started = true;
    MPC_OMP_TASK_TRACE_SCHEDULE(task);
    task->func(task->data);
    task->statuses.completed = true;
    __task_run_coherency_check_post(task);
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

    __task_run_coherency_check_pre(task);
    task->statuses.started = true;
    task->func(task->data);
    assert(task->statuses.completed == false);
    task->statuses.completed = true;
    __task_run_coherency_check_post(task);

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
        assert(!task->statuses.started);
        task->fiber = __thread_generate_new_task_fiber(thread);
    }

    ___thread_bind_task(thread, task, &(task->icvs));
    task->fiber->exit = &(thread->task_infos.mctx);

    sctk_mctx_t * mctx = task->statuses.started ? &(task->fiber->current) : &(task->fiber->initial);

    MPC_OMP_TASK_TRACE_SCHEDULE(task);
    sctk_swapcontext_no_tls(task->fiber->exit, mctx);
    MPC_OMP_TASK_TRACE_SCHEDULE(task);

    ___thread_bind_task(thread, curr, &(curr->icvs));

    if (task->statuses.completed)
    {
        __task_finalize(task);
    }
    else
    {
        /* The task yielded and blocked, unlock it associated event spinlock
         * see `mpc_omp_task_block()` */
        if (task->statuses.blocking)
        {
            assert(thread->task_infos.spinlock_to_unlock);
            task->statuses.blocked = true;
            task->statuses.blocking = false;
            mpc_common_spinlock_unlock(thread->task_infos.spinlock_to_unlock);
            thread->task_infos.spinlock_to_unlock = NULL;
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

    /* continue what the thread was doing */
}

# endif /* MPC_OMP_TASK_COMPILE_FIBER */

/**
 * Run the task
 * @param task : the task
 */
void
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

/**
 * Task scheduling.
 *
 * Try to find a task to be scheduled and execute it on the calling omp thread.
 * This is the main function (it calls an internal function).
 */
int
_mpc_omp_task_schedule(void)
{
    _mpc_omp_callback_run(MPC_OMP_CALLBACK_TASK_SCHEDULE_BEFORE);

    /* get current thread */
    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    mpc_omp_task_t * task = __task_schedule_next(thread);

    if (task)
    {
        if (task->taskgroup && OPA_load_int(&(task->taskgroup->cancelled)))
        {
            task->statuses.cancelled = true;
            __task_finalize(task);
            return 0;
        }

# if MPC_OMP_TASK_COMPILE_TRACE
        task->schedule_id = OPA_fetch_and_incr_int(&(thread->instance->task_infos.next_schedule_id));
# endif
        __task_run(task);
        return 1;
    }
    else
    {
        /* Famine detected */
        return 0;
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
# endif /* MPC_OMP_TASK_COMPILE_FIBER */

static inline void
__thread_requeue_task(mpc_omp_task_t * task)
{
    mpc_omp_task_pqueue_type_t type;
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNTIED))
    {
        type = MPC_OMP_PQUEUE_TYPE_UNTIED;
    }
    else
    {
        type = MPC_OMP_PQUEUE_TYPE_TIED;
    }

    mpc_omp_task_pqueue_t * pqueue = __thread_get_task_pqueue(task->thread, type);
    assert(pqueue);
    __task_pqueue_push(pqueue, task);
}

/* Warning : a task may unblock before blocking
 * Note that while only a single thread may block a task at a time,
 * multiple threads may unblock the same task concurrently */
void
mpc_omp_task_unblock(mpc_omp_event_handle_t * event)
{
    assert(event);
    assert(event->type & MPC_OMP_EVENT_TASK_BLOCK);

    mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
    assert(thread);

    mpc_omp_task_t * task = (mpc_omp_task_t *) event->attr;
    assert(task);

    /* if the task unblocked before blocking, there is nothing to do */
    if (OPA_cas_int(
                &(event->status),
                MPC_OMP_EVENT_HANDLE_STATUS_INIT,
                MPC_OMP_EVENT_HANDLE_STATUS_UNBLOCKED) == MPC_OMP_EVENT_HANDLE_STATUS_INIT)
    {
        /* nothing to do, task unblocked before blocking */
    }
    else
    {
        assert(
            OPA_load_int(&(event->status)) == MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED ||
            OPA_load_int(&(event->status)) == MPC_OMP_EVENT_HANDLE_STATUS_UNBLOCKED
        );

        if (OPA_cas_int(
                    &(event->status),
                    MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED,
                    MPC_OMP_EVENT_HANDLE_STATUS_UNBLOCKED) == MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED)
        {
            /* remove the task from the blocked list */
            mpc_omp_task_list_t * list = &(thread->instance->task_infos.blocked_tasks);
            mpc_common_spinlock_lock(&(list->lock));
            {
                if (task->statuses.in_blocked_list)
                {
                    __task_list_remove(list, task);
                    task->statuses.in_blocked_list = false;
                }
            }
            mpc_common_spinlock_unlock(&(list->lock));

            /* if the task has a fiber, check for requeue */
# if MPC_OMP_TASK_COMPILE_FIBER
            if (MPC_OMP_TASK_FIBER_ENABLED && mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_HAS_FIBER))
            {
                mpc_common_spinlock_lock(&(event->lock));
                {
                    task->statuses.unblocked = true;
                    if (task->statuses.blocked)
                    {
                        task->statuses.blocked = false;
                        __thread_requeue_task(task);
                    }
                }
                mpc_common_spinlock_unlock(&(event->lock));
            }
            /* otherwise, it will be resumed by its suspending thread once ready */
            else
            {
                /* nothing to do */
            }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
        }
        else
        {
            /* nothing to do, the task is already being unblocked by another thread */
        }
    }
}

/* Warning: a task may unblock before it was actually suspended
 * Note that only the executing thread may block the task,
 * so only 1 thread may block the same task at a time */
void
mpc_omp_task_block(mpc_omp_event_handle_t * event)
{
    assert(event);

    /* if the task did not unblock */
    if (OPA_cas_int(
        &(event->status),
        MPC_OMP_EVENT_HANDLE_STATUS_INIT,
        MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED) == MPC_OMP_EVENT_HANDLE_STATUS_INIT)
    {

        mpc_omp_thread_t * thread = __thread_task_coherency(NULL);
        assert(thread);

        mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
        assert(task);

        /* add the task to the blocked list */
        mpc_omp_task_list_t * list = &(thread->instance->task_infos.blocked_tasks);
        mpc_common_spinlock_lock(&(list->lock));
        {
            if (OPA_load_int(&(event->status)) == MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED)
            {
                task->statuses.in_blocked_list = true;
                __task_list_push_to_head(list, task);
            }
        }
        mpc_common_spinlock_unlock(&(list->lock));

        /* if the task has it own fiber, switch to parent' fiber */
# if MPC_OMP_TASK_COMPILE_FIBER
        if (MPC_OMP_TASK_FIBER_ENABLED && mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_HAS_FIBER))
        {
            mpc_common_spinlock_lock(&(event->lock));
            {
                if (OPA_load_int(&(event->status)) == MPC_OMP_EVENT_HANDLE_STATUS_BLOCKED)
                {
                    thread->task_infos.spinlock_to_unlock = &(event->lock);
                    task->statuses.blocking = true;
                    __taskyield_return();
                }
                else
                {
                    mpc_common_spinlock_unlock(&(event->lock));
                }
            }
        }
        /* otherwise, busy-loop until unblock */
        else
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
        {
            while (task->statuses.in_blocked_list)
            {
                _mpc_omp_task_schedule();
            }
        }
    }
}

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
        case (MPC_OMP_TASK_YIELD_MODE_NOOP):
        {
            /* do nothing */
            break ;
        }
        case (MPC_OMP_TASK_YIELD_MODE_STACK):
        {
            __taskyield_stack();
            break ;
        }

        case (MPC_OMP_TASK_YIELD_MODE_CIRCULAR):
        {
            fprintf(stderr, "Circular task-yield is not supported, please use 'mpc_omp_task_block()'\n");
            not_implemented();
            break ;
        }

        default:
        {
            break ;
        }
    }
}

/*
 * Allocate an openmp task
 * \param task total size (>= sizeof(mpc_omp_task_t))
 */
mpc_omp_task_t *
_mpc_omp_task_allocate(size_t size)
{
    /* Intialize the OpenMP environnement (if needed) */
    mpc_omp_init();

    /* Retrieve the information (microthread structure and current region) */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    /* increment number of existing tasks */
    OPA_incr_int(&(thread->instance->task_infos.ntasks_allocated));

# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_omp_task_t * task = mpc_common_nrecycler_alloc(&(thread->task_infos.task_recycler), size);
# else
    mpc_omp_task_t * task = mpc_omp_alloc(size);
# endif

    return task;
}

static void
_mpc_omp_task_init_attributes(
    mpc_omp_task_t * task,
    void (*func)(void *),
    void * data,
    size_t size,
    mpc_omp_task_property_t properties)
{
    /* Intialize the OpenMP environnement (if needed) */
    mpc_omp_init();

    /* Retrieve the information (microthread structure and current region) */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    /* Reset all task infos to NULL */
    assert(task);
    memset(task, 0, sizeof(mpc_omp_task_t));

    /* Set non null task infos field */
    task->func = func;
    task->data = data;
    task->size = size;
    task->property = properties;
    task->icvs = thread->info.icvs;

    task->parent = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    task->depth = (task->parent) ? task->parent->depth + 1 : 0;
    OPA_store_int(&(task->ref_counter), 1); /* _mpc_omp_task_deinit */
    OPA_store_int(&(task->children_count), 0);
    task->uid = OPA_fetch_and_incr_int(&(thread->instance->task_infos.next_task_uid));

#if MPC_USE_SISTER_LIST
    task->children_lock = SCTK_SPINLOCK_INITIALIZER;
#endif
}

/*
 * Initialize an openmp task
 *
 * \param task the task buffer to use (may be NULL, and so, a new task is allocated)
 * \param fn the task entry point
 * \param data the task data (fn parameters)
 * \param size total size of the task (sizeof(mpc_omp_task) + data_size)
 * \param properties
 */
mpc_omp_task_t *
_mpc_omp_task_init(
    mpc_omp_task_t * task,
    void (*func)(void *),
    void * data,
    size_t size,
    mpc_omp_task_property_t properties)
{
    /* set attributes */
    _mpc_omp_task_init_attributes(task, func, data, size, properties);

    /* Retrieve the information (microthread structure and current region) */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    /* reference the task */
    _mpc_omp_taskgroup_add_task(task);
    __task_ref_parallel_region();
    __task_ref_parent_task(task->parent); /* __task_finalize */
    __task_ref(task); /* __task_finalize */

    /* extra parameters given to the mpc thread for this task */
# if MPC_OMP_TASK_COMPILE_TRACE
    if (thread->task_infos.incoming.label)
    {
        strncpy(task->label, thread->task_infos.incoming.label, MPC_OMP_TASK_LABEL_MAX_LENGTH);
        thread->task_infos.incoming.label = NULL;
    }
# endif /* MPC_OMP_TASK_COMPILE_TRACE */
    if (thread->task_infos.incoming.extra_clauses & MPC_OMP_CLAUSE_USE_FIBER)
    {
        mpc_omp_task_set_property(&(task->property), MPC_OMP_TASK_PROP_HAS_FIBER);
        thread->task_infos.incoming.extra_clauses = 0;
    }

    return task;
}

/**
 * Between `_mpc_omp_task_init` and `_mpc_omp_task_deinit` - the task can be accessed safely
 * Calling `_mpc_omp_task_deinit` ensure to the runtime
 * that the task will no longer be accessed, and thatit can release it anytime
 */
void
_mpc_omp_task_deinit(mpc_omp_task_t * task)
{
    __task_unref(task);
}

TODO("refactor this call to avoid LLVM > GOMP depend array conversion");
/**
 * set task dependencies
 *  - task is the task
 *  - depend is the dependency array (GOMP format)
 *  - priority_hint is the user task priority hint
 */
void
_mpc_omp_task_deps(mpc_omp_task_t * task, void ** depend, int priority_hint)
{
    /* retrieve current thread */
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    /* set priority hint */
    task->omp_priority_hint = priority_hint;

    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_DEPEND))
    {
        /* parse dependencies */
        task->dep_node.hmap = NULL;
//        mpc_common_spinlock_init(&(task->dep_node.hmap_lock), 0);
        mpc_common_spinlock_init(&(task->lock), 0);
        task->dep_node.successors = NULL;
        OPA_store_int(&(task->dep_node.nsuccessors), 0);
        task->dep_node.predecessors = NULL;
        OPA_store_int(&(task->dep_node.npredecessors), 0);
        OPA_store_int(&(task->dep_node.ref_predecessors), 0);
        task->dep_node.top_level = 0;
        task->dep_node.dep_list = NULL;
        task->dep_node.dep_list_size = 0;

        /* protect task from being scheduled before all of its dependencies are processed */
        OPA_incr_int(&(task->dep_node.ref_predecessors));

        /* link task with predecessors, and register dependencies to the hmap */
        __task_process_deps(task, depend);

        /* remove protection */
        OPA_decr_int(&(task->dep_node.ref_predecessors));

        /* now this task can be queued */
        assert(!task->statuses.started);
    }

    /* compute priority */
    __task_priority_compute(task);

    /* trace task creation */
    MPC_OMP_TASK_TRACE_CREATE(task);
}

/**
 *  Process the task : run it or differ it
 *  return 0 if the task was differed, 1 if it started on current thread
 */
int
_mpc_omp_task_process(mpc_omp_task_t * task)
{
    /* Retrieve the information (microthread structure and current region) */
    int r = 0;
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    assert(thread->instance);

    int became_ready;

    /* if the task has dependencies, beware for concurrency in '__task_finalize_deps' */
    if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_DEPEND))
    {
        if (OPA_load_int(&(task->dep_node.ref_predecessors)) == 0)
        {
            /* mark it as ready */
            if (OPA_cas_int(&(task->dep_node.status), MPC_OMP_TASK_STATUS_NOT_READY, MPC_OMP_TASK_STATUS_READY) == MPC_OMP_TASK_STATUS_NOT_READY)
            {
                became_ready = 1;
            }
            else
            {
                became_ready = 0;
            }
        }
        else
        {
            became_ready = 0;
        }
    }
    /* otherwise, no dependencies means no concurrency issues */
    else
    {
        became_ready = 1;
    }

    /* if the task became ready */
    if (became_ready)
    {
        /* if the task is undeferred, or we reached thresholds */
        if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNDEFERRED) || __task_reached_thresholds(task) || mpc_omp_conf_get()->task_sequential)
        {
            /* run it */
            __task_run(task);
            r = 1;
        }
        else
        {
            /* else, differ it */
            mpc_omp_task_pqueue_t * pqueue = __thread_get_task_pqueue(thread, MPC_OMP_PQUEUE_TYPE_NEW);
            __task_pqueue_push(pqueue, task);
        }
    }
    /* else the task is not ready */
    else
    {
        while (__task_reached_thresholds(task) && !task->statuses.completed)
        {
            _mpc_omp_task_schedule();
        }

        /* A task for which execution is not deferred with respect to its
         * generating task region. That is, its generating task region is
         * suspended until execution of the undeferred task is completed. */
        if (mpc_omp_task_property_isset(task->property, MPC_OMP_TASK_PROP_UNDEFERRED))
        {
            while (!task->statuses.completed)
            {
                _mpc_omp_task_schedule();
            }
        }
    }

    return r;
}

/*************
 * TASKGROUP *
 *************/

void
_mpc_omp_task_taskgroup_start( void )
{
    mpc_omp_init();

    mpc_omp_thread_t * thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread );
    assert(task);

    /* Init new task group and store it in current task */
    mpc_omp_task_taskgroup_t * new_taskgroup = (mpc_omp_task_taskgroup_t *) mpc_omp_alloc(sizeof(mpc_omp_task_taskgroup_t));
    assert(new_taskgroup);

    OPA_store_int(&(new_taskgroup->children_num), 0);
    OPA_store_int(&(new_taskgroup->cancelled), 0);
    new_taskgroup->prev = task->taskgroup;

    task->taskgroup = new_taskgroup;
}

void
_mpc_omp_task_taskgroup_end(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_taskgroup_t * taskgroup = task->taskgroup;
    if (!taskgroup) return ;

    while (OPA_load_int(&(taskgroup->children_num)))
    {
        _mpc_omp_task_schedule();
    }

    task->taskgroup = taskgroup->prev;
    mpc_omp_free(taskgroup);
}

void
_mpc_omp_task_taskgroup_cancel(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_taskgroup_t * taskgroup = task->taskgroup;
    if (!taskgroup) return ;

    OPA_cas_int(&(taskgroup->cancelled), 0, 1);
}

/************
 * TASKLOOP *
 ************/
unsigned long
_mpc_omp_task_loop_compute_num_iters(long start, long end, long step)
{
    long decal = (step > 0) ? -1 : 1;

    if ((end - start) * decal >= 0)
        return 0;

    return (end - start + step + decal) / step;
}

unsigned long
_mpc_task_loop_compute_loop_value(
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

unsigned long
_mpc_task_loop_compute_loop_value_grainsize(
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

static inline void
__thread_task_init_initial(mpc_omp_thread_t * thread)
{
    const size_t size = sizeof(mpc_omp_task_t);
    mpc_omp_task_t * initial_task = _mpc_omp_task_allocate(size);
    assert(initial_task);

    mpc_omp_task_property_t properties = 0;
    mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_INITIAL);
    mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_IMPLICIT);
    _mpc_omp_task_init_attributes(initial_task, NULL, NULL, size, properties);
# if MPC_OMP_TASK_COMPILE_TRACE
    snprintf(initial_task->label, MPC_OMP_TASK_LABEL_MAX_LENGTH, "initial-%p", initial_task);
# endif /* MPC_OMP_TASK_COMPILE_TRACE */

#ifdef MPC_OMP_USE_MCS_LOCK
    thread->task_infos.opaque = sctk_mcslock_alloc_ticket();
#else /* MPC_OMP_USE_MCS_LOCK  */
    thread->task_infos.opaque = NULL;
#endif /* MPC_OMP_USE_MCS_LOCK  */
    MPC_OMP_TASK_TRACE_CREATE(initial_task);
    MPC_OMP_TASK_THREAD_SET_CURRENT_TASK(thread, initial_task);
}

# if MPC_OMP_TASK_USE_RECYCLERS
static inline void
__thread_task_init_recyclers(mpc_omp_thread_t * thread)
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
            config->task_fiber_recycler_capacity
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
}
# endif

void
_mpc_omp_task_tree_init(mpc_omp_thread_t * thread)
{
    if (mpc_omp_conf_get()->task_trace_auto) mpc_omp_task_trace_begin();
    memset(&(thread->task_infos.incoming), 0, sizeof(thread->task_infos.incoming));
# if MPC_OMP_TASK_USE_RECYCLERS
    __thread_task_init_recyclers(thread);
# endif
    __thread_task_init_initial(thread);
}

static void
__thread_task_deinit_initial(mpc_omp_thread_t * thread)
{
    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    MPC_OMP_TASK_THREAD_SET_CURRENT_TASK(thread, NULL);
    __task_delete_dependencies_hmap(task);
    _mpc_omp_task_deinit(task);
}

# if MPC_OMP_TASK_USE_RECYCLERS
static void
__thread_task_deinit_recyclers(mpc_omp_thread_t * thread)
{
# if MPC_OMP_TASK_COMPILE_FIBER
    if (MPC_OMP_TASK_FIBER_ENABLED)
    {
        mpc_common_recycler_deinit(&(thread->task_infos.fiber_recycler));
    }
# endif /* MPC_OMP_TASK_COMPILE_FIBER */
    mpc_common_nrecycler_deinit(&(thread->task_infos.task_recycler));
}
# endif

/**
 * Deinitialize the thread task tree
 */
void
_mpc_omp_task_tree_deinit(mpc_omp_thread_t * thread)
{
    assert(thread);
    __thread_task_deinit_initial(thread);
# if MPC_OMP_TASK_USE_RECYCLERS
    __thread_task_deinit_recyclers(thread);
# endif /* MPC_OMP_TASK_USE_RECYCLER */
    if (mpc_omp_conf_get()->task_trace_auto) mpc_omp_task_trace_end();
    // this may not be true since every threads are deinitialized concurrently
    // assert(OPA_load_int(&(thread->instance->task_infos.ntasks)) == 0);
    // assert(OPA_load_int(&(thread->instance->task_infos.ntasks_ready)) == 0);
}
