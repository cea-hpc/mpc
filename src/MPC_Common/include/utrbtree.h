/*
Copyright (c) 2003-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/* Author: Romain PEREIRA */

/**
 * TODO
 *  - use 'restrict' keyword whenever possible
 *  - use 'const' keyword whenever possible
 *  - expand API, it may be interesting to support
 *
 *      - RBTREE_MIN_[LIFO|FIFO](...)
 *          symetric of 'RBTREE_MAX[|_LIFO|_FIFO]'
 *
 *      - RBTREE_MAX[|_LIFO|_FIFO](...)
 *          give the object with max value, without popping it
 *
 *      - RBTREE_MIN_POP[|_LIFO|_FIFO](...)
 *          symetric of 'RBTREE_MAX_POP[|_LIFO|_FIFO]'
 *
 *      - RBTREE_POP(T, N, RBH)
 *          pop the node pointed by 'N->RBH' from the tree
 */


/****************************************************************************/
/*                          DEFINE DECLTYPES                                */
/****************************************************************************/

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#if !defined(DECLTYPE) && !defined(NO_DECLTYPE)
# if defined(_MSC_VER)   /* MS compiler */
#  if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#   define DECLTYPE(x) (decltype(x))
#  else                   /* VS2008 or older (or VS2010 in C mode) */
#   define NO_DECLTYPE
#  endif
# elif defined(__MCST__)  /* Elbrus C Compiler */
#  define DECLTYPE(x) (__typeof(x))
# elif defined(__BORLANDC__) || defined(__ICCARM__) || defined(__LCC__) || defined(__WATCOMC__)
#  define NO_DECLTYPE
# else                   /* GNU, Sun and other compilers */
#  define DECLTYPE(x) (__typeof(x))
# endif
#endif

#ifdef NO_DECLTYPE
# define DECLTYPE(x)
# define DECLTYPE_ASSIGN(dst,src)                                               \
    do {                                                                        \
        char **_da_dst = (char**)(&(dst));                                      \
        *_da_dst = (char*)(src);                                                \
    } while (0)
#else
# define DECLTYPE_ASSIGN(dst,src)                                               \
    do {                                                                        \
        (dst) = DECLTYPE(dst)(src);                                             \
    } while (0)
#endif
/* END OF DECLTYPE */

/****************************************************************************/
/*      RB TREE DECLARATIONS                                                */
/****************************************************************************/
#ifndef UTRBTREE_H
# define UTRBTREE_H

# define UTRBTREE_VERSION 2.3.0

/**
 * Example usage
 *
 *  typedef struct
 *  {
 *      char * label;
 *      UT_rbtree_node rbh;
 *      int x;
 *  } task_t;
 *
 *  UT_rbtree tree;
 *  task_t t1, t2, t3;
 *
 *  RBTREE_INSERT(&tree, &t1, rbh, 1);
 *  RBTREE_INSERT(&tree, &t2, rbh, 2);
 *  RBTREE_INSERT(&tree, &t3, rbh, 3);
 *
 *  task_t * pt1, * pt2, * pt3;
 *  RBTREE_POP_MAX(&tree, pt3);
 *  RBTREE_POP_MAX(&tree, pt2);
 *  RBTREE_POP_MAX(&tree, pt1);
 *
 *  assert(&t1 == pt1);
 *  assert(&t2 == pt2);
 *  assert(&t3 == pt3);
 *  assert(tree.root == NULL);
 */

# include <assert.h>
# include <stdio.h>     /* FILE */
# include <stddef.h>    /* ptrdiff_t */
# include <string.h>    /* memcmp, memset, strlen */

/**
 * List of hints to optimize the implementation
 *
 * RBTREE_EXT_UNIQUE_PRIORITY
 *      if defined, assumed every nodes has different priorities
 */

# ifndef RBTREE_EXT_UNIQUE_PRIORITY
#  define RBTREE_EXT_UNIQUE_PRIORITY 0
# endif /* RBTREE_EXT_UNIQUE_PRIORITY */

# ifndef RBTREE_EXT_MAX
# define RBTREE_EXT_MAX 0
# endif /* RBTREE_EXT_MAX */

# ifndef UTRBTREE_PRIORITY_TYPE_T
#  define UTRBTREE_PRIORITY_TYPE_T          int
#  define UTRBTREE_PRIORITY_TYPE_MODIFIER_T "%d"
# endif /* UTRBTREE_PRIORITY_TYPE_T */

/* Insert node 'OBJ->RBH' of priority 'P' within the tree 'T' */
# define RBTREE_INSERT(T, OBJ, RBH, P)                          \
    do {                                                        \
        rbtree_insert((T), &((OBJ)->RBH), (P));                 \
    } while (0)

/* Remove the node 'OBJ->RBH' from the tree 'T' */
# define RBTREE_REMOVE(T, OBJ, RBH) assert("not implemented" && 0)  // TODO

/* Return the node with maximum priority within the tree */
# define _RBTREE_POP_MAX(T, N, ORD)                                         \
    do {                                                                    \
        DECLTYPE_ASSIGN(N, (((char *)(rbtree_pop_max(T, ORD))) - T->rbho)); \
    } while (0)
# define RBTREE_POP_MAX_FIFO(T, N)  _RBTREE_POP_MAX(T, N, RBTREE_FIFO)
# define RBTREE_POP_MAX_LIFO(T, N)  _RBTREE_POP_MAX(T, N, RBTREE_LIFO)
# define RBTREE_POP_MAX(T, N)       RBTREE_POP_MAX_LIFO(T, N)

/* Dump the tree to the given file */
# define RB_DUMP(T, LBL, F)                                     \
    do {                                                        \
         fprintf(F, "digraph %s {\n", LBL);                     \
         rbtree_dump_node(T->root, F);                          \
         rbtree_dump_edge(T->root, F);                          \
         fprintf(F, "}\n");                                     \
    } while (0)

/* IMPLEMENTATION */

struct UT_rbtree_node;

typedef struct
{
    struct UT_rbtree_node * root;   /* tree root */
    ptrdiff_t rbho;                 /* offset between the handler and the object */
# if RBTREE_EXT_MAX
    struct {
        struct UT_rbtree_node * max;
    } cache;
# endif /* RBTREE_EXT_MAX */
} UT_rbtree;

# if !RBTREE_EXT_MAX
#  define RBTREE_INITIALIZER(TYPE, RBH)  { .root = NULL, .rbho = offsetof(TYPE, RBH) }
#  define RBTREE_INITIALIZE(TREE, TYPE, RBH)    do {                                    \
                                                    TREE.root = NULL;                   \
                                                    TREE.rbho = offsetof(TYPE, RBH);    \
                                                } while (0)
# else /* RBTREE_EXT_MAX */
#  define RBTREE_INITIALIZER(TYPE, RBH)  { .root = NULL, .rbho = offsetof(TYPE, RBH), .cache = {.max = NULL} }
#  define RBTREE_INITIALIZE(TREE, TYPE, RBH)    do {                                    \
                                                    TREE.root = NULL;                   \
                                                    TREE.rbho = offsetof(TYPE, RBH);    \
                                                    TREE.cache.maxx = NULL;             \
                                                } while (0)
# endif /* RBTREE_EXT_MAX */
# define RBTREE_CLEAR(T)                do { T->root = NULL; } while (0)

/**
 *  Note :
 *      The attributes 'parent', 'left', 'right', 'priority' and 'color' could
 *      be stored in a dedicated 'UT_rbtree_node_list' structure
 *      dynamically allocated. Right now, it is stored in the node inducing
 *      an important memory footprint overhead, but no dynamic memory allocation.
 *
 *      This structure induce computation overhead when popping in FIFO mode,
 *      to recopy these attributes between nodes
 *
 *      In LIFO mode, there is no computation overhead : these attributes
 *      are stored on the 1st inserted node.
 */

typedef struct UT_rbtree_node
{
   struct UT_rbtree_node * parent;      /* parent node  */
   struct UT_rbtree_node * left;        /* left node    */
   struct UT_rbtree_node * right;       /* right node   */
# if !RBTREE_EXT_UNIQUE_PRIORITY
   struct UT_rbtree_node * prev;        /* prev node with the same priority */
   struct UT_rbtree_node * next;        /* next node with the same priority */
# endif /* !RBTREE_EXT_UNIQUE_PRIORITY */
   UTRBTREE_PRIORITY_TYPE_T priority;   /* the priority */
   char color;                          /* 'R' or 'B'   */
} UT_rbtree_node;

typedef enum
{
    RBTREE_FIFO,
    RBTREE_LIFO,
} UT_rbtree_order;

static inline void
__rbtree_coherency_colors(
    UT_rbtree * tree,
    UT_rbtree_node * node)
{
    if (node)
    {
        /* root node has no parent, every other nodes have */
        assert(node == tree->root || node->parent);

        /* node is either red or black */
        assert(node->color == 'B' || node->color == 'R');

        /* If a node is red, then both its children are black. */
        assert(!node->parent || !(node->color == 'R' && node->parent->color == 'R'));

        __rbtree_coherency_colors(tree, node->left);
        __rbtree_coherency_colors(tree, node->right);
    }
}

static inline int
rbtree_coherency_paths(UT_rbtree_node * node)
{
    if (node == NULL) return 1;

    int left_height = rbtree_coherency_paths(node->left);
    if (left_height == 0)   return 0;

    int right_height = rbtree_coherency_paths(node->right);
    if (right_height == 0)  return 0;

    if (left_height != right_height)    return 0;
    else                                return left_height + (node->color == 'B') ? 1 : 0;
}

static inline void
rbtree_coherency_check(UT_rbtree * tree)
{
    /* 1. The root of the tree is always black */
    assert(!tree->root || tree->root->color == 'B');

    if (tree->root)
    {
        /* 2. If a node is red, then both its children are black */
        __rbtree_coherency_colors(tree, tree->root);

        /* 3. Every path from a node to any of its descendant NULL nodes
         * has the same number of black nodes */
        assert(rbtree_coherency_paths(tree->root));
    }
}

static inline UT_rbtree_node *
__rbtree_node_init(
    UT_rbtree_node * node,
    UT_rbtree_node * parent,
    char color,
    UTRBTREE_PRIORITY_TYPE_T priority)
{
    node->parent    = parent;
    node->left      = NULL;
    node->right     = NULL;
# if !RBTREE_EXT_UNIQUE_PRIORITY
    node->prev      = node;
    node->next      = node;
# endif /* !RBTREE_EXT_UNIQUE_PRIORITY */
    node->priority  = priority;
    node->color     = color;
    return node;
}

static inline void
rbtree_rotate_right(
        UT_rbtree * tree,
        UT_rbtree_node * x)
{
    UT_rbtree_node * y = x->left;

    x->left = y->right;
    if (y->right) y->right->parent = x;
    y->parent = x->parent;

    if (x->parent == NULL)          tree->root = y;
    else if (x == x->parent->right) x->parent->right = y;
    else                            x->parent->left = y;

    y->right = x;
    x->parent = y;
}

static inline void
rbtree_rotate_left(
        UT_rbtree * tree,
        UT_rbtree_node * x)
{
    UT_rbtree_node * y = x->right;

    x->right = y->left;
    if (y->left) y->left->parent = x;
    y->parent = x->parent;

    if (x->parent == NULL)          tree->root = y;
    else if (x == x->parent->left)  x->parent->left = y;
    else                            x->parent->right = y;

    y->left = x;
    x->parent = y;
}

static inline void
__rbtree_transplant(
        UT_rbtree * tree,
        UT_rbtree_node * u,
        UT_rbtree_node * v)
{
    if (u->parent == NULL)  tree->root = v;
    else
    {
        if (u == u->parent->left)   u->parent->left  = v;
        else                        u->parent->right = v;
    }
    if (v) v->parent = u->parent;
}


/** Dump pqueue tree in .dot format */

/**
 * Example usage:
 *  {
 *      char filepath[256];
 *      sprintf(filepath, "%lf-%p-%s-tree.dot", omp_get_wtime(), tree, label);
 *      FILE * f = fopen(filepath, "w");
 *
 *      fprintf(f, "digraph g%d {\n", OPA_load_int(&(tree->nb_elements)));
 *      rbtree_dump_node(tree->root, f);
 *      rbtree_dump_edge(tree->root, f);
 *      fprintf(f, "}\n");
 *
 *      fclose(f);
 *  }
 */

static inline void
rbtree_dump_node(UT_rbtree_node * node, FILE * f)
{
    if (!node) return ;

    char * color = (node->color == 'R') ? "#ff0000" : "#000000";
# if !RBTREE_EXT_UNIQUE_PRIORITY
    int times = 1;
    UT_rbtree_node * head = node;
    while (head->next != node)
    {
        head = head->next;
        ++times;
    }
    fprintf(f, "    N%p[fontcolor=\"#ffffff\", label=\""UTRBTREE_PRIORITY_TYPE_MODIFIER_T" (x%d)\", style=filled, fillcolor=\"%s\"] ;\n",
        node, node->priority, times, color);
# else /* !RBTREE_EXT_UNIQUE_PRIORITY */
    fprintf(f, "    N%p[fontcolor=\"#ffffff\", label=\""UTRBTREE_PRIORITY_TYPE_MODIFIER_T"\", style=filled, fillcolor=\"%s\"] ;\n",
        node, node->priority, color);
# endif /* !RBTREE_EXT_UNIQUE_PRIORITY */


    rbtree_dump_node(node->left, f);
    rbtree_dump_node(node->right, f);
}

static inline void
rbtree_dump_edge(UT_rbtree_node * node, FILE * f)
{
    if (!node) return ;

    if (node->left)
    {
        fprintf(f, "    N%p->N%p ; \n", node, node->left);
        rbtree_dump_edge(node->left, f);
    }

    if (node->right)
    {
        fprintf(f, "    N%p->N%p ; \n", node, node->right);
        rbtree_dump_edge(node->right, f);
    }
}

/**
 * Fixup the rb tree after 'node' was inserted inside 'tree'
 * https://en.wikipedia.org/wiki/Red-black_tree
 * https://www.youtube.com/watch?v=qA02XWRTBdw
 */
static inline void
rbtree_insert_fixup(
    UT_rbtree * tree,
    UT_rbtree_node * node)
{
    while (node->parent->color == 'R')
    {
        if (node->parent == node->parent->parent->right)
        {
            /* uncle */
            UT_rbtree_node * u = node->parent->parent->left;
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
                    rbtree_rotate_right(tree, node);
                }
                /* case 3.2.1 */
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                rbtree_rotate_left(tree, node->parent->parent);
            }
        }
        else
        {
            /* uncle */
            UT_rbtree_node * u = node->parent->parent->right;

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
                    rbtree_rotate_left(tree, node);
                }
                /* case 3.2.1 */
                node->parent->color = 'B';
                node->parent->parent->color = 'R';
                rbtree_rotate_right(tree, node->parent->parent);
            }
        }
        if (node == tree->root)
        {
            break;
        }
    }
    tree->root->color = 'B';
}

# if !RBTREE_EXT_UNIQUE_PRIORITY
static inline void
__rbtree_insert_tail(UT_rbtree_node ** head, UT_rbtree_node * node)
{
    assert(*head);

    UT_rbtree_node * tail = (*head)->prev;

    node->next      = *head;
    (*head)->prev   = node;

    node->prev      = tail;
    tail->next      = node;
}

static inline UT_rbtree_node *
__rbtree_remove_tail(UT_rbtree_node * head)
{
    if (head == head->next) return head;
    UT_rbtree_node * tail = head->prev;
    head->prev       = tail->prev;
    tail->prev->next = head;
    return tail;
}

# endif /* !RBTREE_EXT_UNIQUE_PRIORITY */

static inline void
rbtree_insert(
    UT_rbtree * tree,
    UT_rbtree_node * node,
    UTRBTREE_PRIORITY_TYPE_T priority)
{
    if (tree->root == NULL)
    {
        __rbtree_node_init(node, NULL, 'B', priority);
        tree->root = node;
# if RBTREE_EXT_MAX
        assert(tree->cache.max == NULL);
        tree->cache.max = node;
# endif /* RBTREE_EXT_MAX */
        return ;
    }

# if RBTREE_EXT_MAX
    if (priority > tree->cache.max->priority)
    {
        while (tree->cache.max->right) tree->cache.max = tree->cache.max->right;
        if (priority > tree->cache.max->priority) tree->cache.max = node;
    }
# endif /* RBTREE_EXT_MAX */

    UT_rbtree_node ** head = &(tree->root);
    while (1)
    {
        if (priority == (*head)->priority)
        {
            /* insert the node in a circular double-linked list */
# if !RBTREE_EXT_UNIQUE_PRIORITY
            __rbtree_insert_tail(head, node);
# else
            assert("Defined 'RBTREE_EXT_UNIQUE_PRIORITY' but two nodes have the same priority !" && 0);
# endif /* !RBTREE_EXT_UNIQUE_PRIORITY */
            return ;
        }
        else if (priority < (*head)->priority)
        {
            if ((*head)->left)
            {
                head = &((*head)->left);
            }
            else
            {
                __rbtree_node_init(node, (*head), 'R', priority);
                (*head)->left = node;
                break ;
            }
        }
        else
        {
            if ((*head)->right)
            {
                head = &((*head)->right);
            }
            else
            {
                __rbtree_node_init(node, (*head), 'R', priority);
                (*head)->right = node;
                break ;
            }
        }
    }

    rbtree_insert_fixup(tree, node);
    rbtree_coherency_check(tree);
}

static inline void
__rbtree_delete_fixup(
    UT_rbtree * tree,
    UT_rbtree_node * x)
{
    while (x && x != tree->root && x->color == 'B')
    {
        assert(x->parent);
        if (x == x->parent->left)
        {
            /* case 1 */
            UT_rbtree_node * w = x->parent->right;
            if (w->color == 'R')
            {
                w->color = 'B';
                x->parent->color = 'R';
                rbtree_rotate_left(tree, x->parent);
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
                    rbtree_rotate_right(tree, w);
                    w = x->parent->right;
                }

                /* case 4 */
                w->color = x->parent->color;
                x->parent->color = 'B';
                w->right->color = 'B';
                rbtree_rotate_left(tree, x->parent);
                x = tree->root;
            }
        }
        /* same as above with 'right' and 'left' exchanged */
        else
        {
            /* case 1 */
            UT_rbtree_node * w = x->parent->left;
            if (w->color == 'R')
            {
                w->color = 'B';
                x->parent->color = 'R';
                rbtree_rotate_right(tree, x->parent);
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
                    rbtree_rotate_left(tree, w);
                    w = x->parent->left;
                }

                /* case 4 */
                w->color = x->parent->color;
                x->parent->color = 'B';
                w->left->color = 'B';
                rbtree_rotate_right(tree, x->parent);
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
static inline void *
rbtree_pop_max(UT_rbtree * tree, UT_rbtree_order ord)
{
    assert(tree->root);

    /* get node with most priority */
# if RBTREE_EXT_MAX
    UT_rbtree_node * z = tree->cache.max;
# else /* RBTREE_EXT_MAX */
    UT_rbtree_node * z = tree->root;
# endif /* RBTREE_EXT_MAX */
    assert(z);

    while (z->right) z = z->right;
    assert(z->parent == NULL || (z->parent->right == z));

    /* pop a task from the list (LIFO) */
# if RBTREE_EXT_UNIQUE_PRIORITY
    UT_rbtree_node * r = z;
    const int unique = 1;
# else /* RBTREE_EXT_UNIQUE_PRIORITY */
    UT_rbtree_node * r;
    const int unique = (z->next == z);
    if (unique)
    {
        r = z;
    }
    else
    {
        assert(z->next != z);
        assert(z->prev != z);
        switch (ord)
        {
            case (RBTREE_FIFO):
            {
                /* make 'z->next' new head of the list */

                if (z->parent)  z->parent->right = z->next;
                else            tree->root = z->next;

                if (z->left)    z->left->parent = z->next;

                z->next->parent     = z->parent;
                z->next->left       = z->left;
                z->next->right      = z->right;
                z->next->priority   = z->priority;
                z->next->color      = z->color;

                z->next->prev       = z->prev;
                z->prev->next       = z->next;

                r = z;

# if RBTREE_EXT_MAX
                tree->cache.max = z->next;
# endif /* RBTREE_EXT_MAX */

                break ;
            }

            case (RBTREE_LIFO):
            {
                r = z->prev;
                r->prev->next = z;
                z->prev = r->prev;
                break ;
            }

            default:
            {
                assert("Wrong 'UT_rbtree_order' in 'rbtree_pop_max'" && 0);
                break ;
            }
        }
    }
# endif /* RBTREE_EXT_UNIQUE_PRIORITY */

    /* if we just poped the last object of this priority, remove the rbtree node 'z' */
    if (unique)
    {
        UT_rbtree_node * x;

        if (z->left == NULL)
        {
            x = z->right;
            __rbtree_transplant(tree, z, z->right);
        }
        else
        {
            x = z->left;
            __rbtree_transplant(tree, z, z->left);
        }

        if (z->color == 'B') __rbtree_delete_fixup(tree, x);

#  if RBTREE_EXT_MAX
        tree->cache.max = z->left ? z->left : z->parent;
        if (z->left)
        {
            tree->cache.max = z->left;
        }
        else
        {
            tree->cache.max = z->parent;
        }
#  endif /* RBTREE_EXT_MAX */

    }
    return r;
}

#endif /* UTRBTREE_H */
