
#define _GNU_SOURCE
#include <stdlib.h>

#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

#if MPCOMP_TASK

/* Compare nb element in first entry of tab
 */
static inline int mpcomp_task_cmp_decr_func(const void *opq_a,
                                            const void *opq_b, 
                                            void* args) {
  const int *a = (int *)opq_a;
  const int *b = (int *)opq_b;
  return (a[0] == b[0]) ? b[1] - a[1] : b[0] - a[0];
}

static inline int 
mpcomp_task_get_depth_from_global_rank( const int globalRank) 
{
    mpcomp_thread_t *thread;
    mpcomp_instance_t *instance;
    mpcomp_generic_node_t* gen_node; 

    sctk_assert( sctk_openmp_thread_tls );
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

    sctk_assert( thread->instance );
    instance = thread->instance;

    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    sctk_assert( instance->tree_array );

    gen_node = &( instance->tree_array[globalRank] );
    sctk_assert( gen_node->type != MPCOMP_CHILDREN_NULL ); 

    if( gen_node->type == MPCOMP_CHILDREN_NODE )
        return gen_node->ptr.node->depth;

    sctk_assert( gen_node->type == MPCOMP_CHILDREN_LEAF );
    sctk_assert( gen_node->ptr.mvp->threads );
    return gen_node->ptr.mvp->threads->depth;
}

int mpcomp_task_get_victim_default(int globalRank, int index,
                                   mpcomp_tasklist_type_t type) {
  return globalRank;
}

/** Hierachical stealing policy **/

void mpcomp_task_init_victim_hierarchical(int globalRank, int index,
                                          mpcomp_tasklist_type_t type) {
  return;
}

static void
mpcomp_task_prepare_victim_hierarchical(int globalRank, int index,
                                        mpcomp_tasklist_type_t type) {
  return;
}

int mpcomp_task_get_victim_hierarchical(int globalRank, int index,
                                        mpcomp_tasklist_type_t type) {
  return mpcomp_get_neighbour(globalRank, index);
}

/** Random stealing policy **/

void mpcomp_task_init_victim_random( mpcomp_generic_node_t* gen_node )
{
    int already_init;
    struct drand48_data *randBuffer;

    sctk_assert(gen_node != NULL);
    already_init =  ( gen_node->type == MPCOMP_CHILDREN_LEAF && MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( gen_node->ptr.mvp ) );
    already_init |= ( gen_node->type == MPCOMP_CHILDREN_NODE && MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( gen_node->ptr.node ) );
    if( already_init && gen_node->type == MPCOMP_CHILDREN_NULL ) return;

    randBuffer = (struct drand48_data *) mpcomp_alloc( sizeof(struct drand48_data) );
    sctk_assert( randBuffer );

    if( gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
        sctk_assert( gen_node->ptr.mvp->threads );
        const int __globalRank = gen_node->ptr.mvp->threads->tree_array_rank;
        srand48_r( sctk_get_time_stamp() * __globalRank, randBuffer);
        MPCOMP_TASK_MVP_SET_TASK_LIST_RANDBUFFER( gen_node->ptr.mvp, randBuffer );
    }
    else
    {
        sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
        const int __globalRank = gen_node->ptr.node->tree_array_rank;
        srand48_r( sctk_get_time_stamp() * __globalRank, randBuffer);
        MPCOMP_TASK_NODE_SET_TASK_LIST_RANDBUFFER( gen_node->ptr.mvp, randBuffer );
    }
}

static void mpcomp_task_prepare_victim_random(int globalRank, int index,
                                              mpcomp_tasklist_type_t type) {
  return;
}

int mpcomp_task_get_victim_random(const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
    int victim;
    long int value;
    mpcomp_thread_t *thread;
    mpcomp_instance_t *instance;
    struct drand48_data *randBuffer;
    mpcomp_generic_node_t* gen_node;

    /* Retrieve the information (microthread structure and current region) */
    sctk_assert(sctk_openmp_thread_tls != NULL);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance;
    sctk_assert( instance->tree_array );
    sctk_assert( instance->tree_nb_nodes_per_depth );
    sctk_assert( instance->tree_first_node_per_depth );

    /* Only victim id and value while change */
    const int __globalRank = globalRank;
    const int __depth = mpcomp_task_get_depth_from_global_rank( globalRank );
    const int __level_size = instance->tree_nb_nodes_per_depth[__depth];
    const int __first_rank = instance->tree_first_node_per_depth[__depth];

    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );

    sctk_assert( gen_node->type != MPCOMP_CHILDREN_NULL );

    if(  gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
        sctk_assert( gen_node->ptr.mvp->threads );
        randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER(thread->mvp);
        //TODO transfert data to current thread ...
        //randBuffer = gen_node->ptr.mvp->threads->task_infos.tasklist_randBuffer;
    }
    else
    {
        sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
        randBuffer = gen_node->ptr.node->task_infos.tasklist_randBuffer;
    }

    do {
        /* Get a random value */
        lrand48_r(randBuffer, &value);
        /* Select randomly a globalRank among neighbourhood */
        victim = (value % __level_size + __first_rank);
    } while (victim == __globalRank); 

  return victim;
}

/** Random order stealing policy **/

static void
mpcomp_task_prepare_victim_random_order(const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
    int i;
    int *order;
    long int value;
    mpcomp_thread_t *thread;
    mpcomp_instance_t *instance;
    struct drand48_data *randBuffer;
    mpcomp_generic_node_t* gen_node;

    /* Retrieve the information (microthread structure and current region) */
    sctk_assert(sctk_openmp_thread_tls != NULL);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread) ) 
        mpcomp_task_allocate_larceny_order(thread);

    sctk_assert(thread->instance);
    instance = thread->instance;
    sctk_assert( instance->tree_array );
    sctk_assert( instance->tree_nb_nodes_per_depth );
    sctk_assert( instance->tree_first_node_per_depth );
    
    /* Only victim id and value while change */
    const int __globalRank = globalRank;
    const int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    const int __nbVictims = thread->instance->tree_nb_nodes_per_depth[tasklistDepth]-1;
    const int __first_rank = instance->tree_first_node_per_depth[tasklistDepth];

    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );

    sctk_assert( gen_node->type != MPCOMP_CHILDREN_NULL );

    if(  gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
        sctk_assert( gen_node->ptr.mvp->threads );
        randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER(thread->mvp);
        //TODO transfert data to current thread ...
        //randBuffer = gen_node->ptr.mvp->threads->task_infos.tasklist_randBuffer;
    }
    else
    {
        sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
        randBuffer = gen_node->ptr.node->task_infos.tasklist_randBuffer;
    }

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    sctk_assert( order );

    for (i = 0; i < __nbVictims; i++) 
    {
        order[i] = __first_rank + i;
        order[i] = (order[i] >= __globalRank) ? order[i] + 1 : order[i];
    }

    for (i = 0; i < __nbVictims; i++) 
    {
        int tmp = order[i];
        lrand48_r(randBuffer, &value);
        int j = value % (__nbVictims - i);
        order[i] = order[i + j];
        order[i + j] = tmp;
    }
}

int mpcomp_task_get_victim_random_order(int globalRank, int index,
                                        mpcomp_tasklist_type_t type) {
  int *order;
  mpcomp_thread_t *thread;

  /* Retrieve the information (microthread structure and current region) */
  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  if (index == 1) {
    mpcomp_task_prepare_victim_random_order(globalRank, index, type);
  }

  order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
  sctk_assert(order);
  sctk_assert(order[index - 1] >= 0 && order[index - 1] != globalRank);
  return order[index - 1];
}

static void 
mpcomp_task_prepare_victim_roundrobin(const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
    int i;
    int *order;
    mpcomp_thread_t *thread;
    mpcomp_instance_t *instance;

    /* Retrieve the information (microthread structure and current region) */
    sctk_assert(sctk_openmp_thread_tls != NULL);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance;
    sctk_assert( instance->tree_array );
    sctk_assert( instance->tree_nb_nodes_per_depth );
    sctk_assert( instance->tree_first_node_per_depth );

    /* Only victim id and value while change */
    const int __globalRank = globalRank;
    const int __depth = mpcomp_task_get_depth_from_global_rank( globalRank );
    const int __nbVictims = instance->tree_nb_nodes_per_depth[__depth] - 1;
    const int __first_rank = instance->tree_first_node_per_depth[__depth];
    const int __decal = (__globalRank - __first_rank + 1) % (__nbVictims + 1);

    if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread) )
        mpcomp_task_allocate_larceny_order(thread);

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    sctk_assert(order);

    for (i = 0; i < __nbVictims; i++) 
    {
        order[i] = __first_rank + (__decal + i) % (__nbVictims + 1);
        order[i] = (order[i] >= __globalRank) ? order[i] + 1 : order[i];
    }
}

int 
mpcomp_task_get_victim_roundrobin(const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
  int *order;
  mpcomp_thread_t *thread;

  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  if (index == 1)
    mpcomp_task_prepare_victim_roundrobin(globalRank, index, type);

  order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
  sctk_assert(order);
  sctk_assert(order[index - 1] >= 0 && order[index - 1] != globalRank);
  return order[index - 1];
}

int mpcomp_task_get_victim_producer(int globalRank, int index, mpcomp_tasklist_type_t type) 
{
    int i, max_elt, victim, rank, nb_elt;
    mpcomp_thread_t *thread;
    mpcomp_task_list_t *list;
    mpcomp_instance_t* instance;

    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance; 

    const int __globalRank = globalRank;
    const int __depth = mpcomp_task_get_depth_from_global_rank( globalRank );
    const int __first_rank = instance->tree_first_node_per_depth[__depth];
    const int __nbVictims = instance->tree_nb_nodes_per_depth[__depth] - 1;

    victim = (globalRank == __first_rank) ? __first_rank + 1 : __first_rank;

    for (max_elt = 0, i = 0; i < __nbVictims; i++) 
    {
        rank = i + __first_rank;
        rank += (rank >= __globalRank) ? 1 : 0;
        list = (mpcomp_task_list_t *) mpcomp_task_get_list(rank, type);
        nb_elt = (list) ? sctk_atomics_load_int(&(list->nb_elements)) : -1;

        if (max_elt < nb_elt) 
        {
            max_elt = nb_elt;
            victim = rank;
        }
    }

    return victim;
}

static inline int
mpcomp_task_prepare_victim_producer_order( const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
    int i;
    int *order;
    void *ext_arg;
    mpcomp_thread_t *thread;
    mpcomp_instance_t* instance;
    struct mpcomp_task_list_s *list;

    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance;

    const int __globalRank = globalRank;
    const int __depth = mpcomp_task_get_depth_from_global_rank( globalRank );
    const int __first_rank = instance->tree_first_node_per_depth[__depth];
    const int __nbVictims = instance->tree_nb_nodes_per_depth[__depth] - 1;

    if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
        mpcomp_task_allocate_larceny_order(thread);

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    int nbElts[__nbVictims][2];

    for (i = 0; i < __nbVictims; i++) 
    {
        nbElts[i][1] = i + __first_rank;
        nbElts[i][1] += (nbElts[i][1] >= __globalRank) ? 1 : 0;
        list = mpcomp_task_get_list(nbElts[i][1], type);
        nbElts[i][0] = (list) ? sctk_atomics_load_int(&list->nb_elements) : -1;
    }

    qsort_r(nbElts, __nbVictims, 2 * sizeof(int), mpcomp_task_cmp_decr_func, ext_arg);

    for (i = 0; i < __nbVictims; i++)
        order[i] = nbElts[i][1];

    return 0;
}

int mpcomp_task_get_victim_producer_order(const int globalRank, const int index, mpcomp_tasklist_type_t type) 
{
    int *order;
    mpcomp_thread_t *thread;

    sctk_assert( sctk_openmp_thread_tls );
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

    if (index == 1)
        mpcomp_task_prepare_victim_producer_order( globalRank, index, type );

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
    sctk_assert( order );
    sctk_assert( order[index - 1] >= 0 && order[index - 1] != globalRank );
    return order[index - 1];
}

#endif /* MPCOMP_TASK */
