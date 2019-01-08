
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
                                            __UNUSED__ void* args) {
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

int mpcomp_task_get_victim_default(int globalRank, __UNUSED__ int index,
                                   __UNUSED__ mpcomp_tasklist_type_t type) {
  return globalRank;
}

/** Hierarchical stealing policy **/

int mpcomp_task_get_victim_hierarchical(int globalRank, __UNUSED__ int index,
                                        __UNUSED__ mpcomp_tasklist_type_t type) {
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
        MPCOMP_TASK_NODE_SET_TASK_LIST_RANDBUFFER( gen_node->ptr.node, randBuffer );
    }
}

int mpcomp_task_get_victim_random(const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type) 
{
    int victim,i;
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

    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];

    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    int first_rank = 0 ; 
    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );

    sctk_assert( gen_node->type != MPCOMP_CHILDREN_NULL );

    if(  gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
        sctk_assert( gen_node->ptr.mvp->threads );
        randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER(thread->mvp);
    }
    else
    {
        sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
        randBuffer = MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER(gen_node->ptr.node);
    }

    
    do {
        /* Get a random value */
        lrand48_r(randBuffer, &value);
        /* Select randomly a globalRank among neighbourhood */
        victim = (value % nbVictims + first_rank);
    } while (victim == globalRank); 

  return victim;
}

int mpcomp_task_get_victim_hierarchical_random(const int globalRank, const int index, mpcomp_tasklist_type_t type)
{   
  int victim,i,j;
  long int value;
  int *order;
  mpcomp_thread_t *thread;
  mpcomp_instance_t *instance;
  mpcomp_node_t *node;
  struct mpcomp_mvp_s *mvp ;
  struct drand48_data *randBuffer;
  mpcomp_generic_node_t* gen_node;
  mpcomp_task_list_t *list;

  /* Retrieve the information (microthread structure and current region) */
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  mvp = thread->mvp ;
  node = mvp->father;
  instance = thread->instance;

  if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread) ) 
    mpcomp_task_allocate_larceny_order(thread);

  order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
  sctk_assert( order );
  if(index==1) {
    int local_rank = thread->rank % node->barrier_num_threads;
    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);

    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    int first_rank = 0 ; 


    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &( instance->tree_array[globalRank] );

    sctk_assert( gen_node->type != MPCOMP_CHILDREN_NULL );

    if(  gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
      sctk_assert( gen_node->ptr.mvp->threads );
      randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER(thread->mvp);
    }
    else
    {
      sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
    }

    int tmporder[nbVictims-1];
    for (i = 0; i < nbVictims-1; i++) 
    {
      order[i] =  first_rank + i;
      order[i] = (order[i] >= globalRank) ? order[i] + 1 : order[i];
      tmporder[i]=order[i];
    }

    int parentrank=node->instance_global_rank - first_rank;
    int startrank=parentrank;
    int x=1,k,l,cpt=0,nb_elt;

    for(i=0;i<tasklistDepth;i++)
    {

      for(j=0;j<(node->barrier_num_threads-1)*x;j++)
      {
        lrand48_r(randBuffer, &value);
        k = (value % ((node->barrier_num_threads-1)*x - j) + parentrank + j) % (nbVictims-1);
        l=(parentrank+j) % (nbVictims-1);
        int tmp = tmporder[l];
        tmporder[l] = tmporder[k];
        tmporder[k]=tmp;
        order[cpt] = tmporder[l] ;
        cpt++;
      }

      x*=node->barrier_num_threads;
      startrank = startrank - node->rank * x;
      if(node->father) node = node->father;
      parentrank = ((((parentrank/x)*x) + x) % (x*node->barrier_num_threads) + startrank) % (nbVictims -1);
      parentrank = (parentrank >= globalRank -first_rank) ? parentrank - 1 : parentrank;

    }
  }
  return order[index-1];
}
    


static void
mpcomp_task_prepare_victim_random_order(const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type) 
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

    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    int first_rank = 0 ; 

    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

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

    for (i = 0; i < nbVictims; i++) 
    {
        order[i] = first_rank + i;
    }

    for (i = 0; i < nbVictims; i++) 
    {
        int tmp = order[i];
        lrand48_r(randBuffer, &value);
        int j = value % (nbVictims - i);
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

  return order[index - 1];
}

static void 
mpcomp_task_prepare_victim_roundrobin(const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type) 
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

    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    int first_rank = 0 ; 

    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

    int decal = (globalRank - first_rank + 1) % (nbVictims);

    if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread) )
        mpcomp_task_allocate_larceny_order(thread);

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    sctk_assert(order);

    for (i = 0; i < nbVictims; i++) 
    {
        order[i] = first_rank + (decal + i) % (nbVictims);
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
  return order[index - 1];
}

int mpcomp_task_get_victim_producer(int globalRank, __UNUSED__ int index, mpcomp_tasklist_type_t type) 
{
    int i, max_elt, victim, rank, nb_elt;
    mpcomp_thread_t *thread;
    mpcomp_task_list_t *list;
    mpcomp_instance_t* instance;

    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance; 
    
    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    int first_rank = 0 ; 

    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

    victim = (globalRank == first_rank) ? first_rank + 1 : first_rank;

    for (max_elt = 0, i = 0; i < nbVictims -1; i++) 
    {
        rank = i + first_rank;
        rank += (rank >= globalRank) ? 1 : 0;
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
mpcomp_task_prepare_victim_producer_order( __UNUSED__ const int globalRank, __UNUSED__ const int index, mpcomp_tasklist_type_t type) 
{
    int i;
    int *order;
    void *ext_arg = NULL;
    mpcomp_thread_t *thread;
    mpcomp_instance_t* instance;
    struct mpcomp_task_list_s *list;

    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert(thread->instance);
    instance = thread->instance;

    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(instance->team, type);
    int nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    int first_rank = 0 ; 

    while(nbVictims == 0)
    {
      tasklistDepth --;
      nbVictims = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    for(i=0;i<tasklistDepth;i++) 
      first_rank += thread->instance->tree_nb_nodes_per_depth[i];

    if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
        mpcomp_task_allocate_larceny_order(thread);

    order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread);
    int nbElts[nbVictims][2];

    for (i = 0; i < nbVictims ; i++) 
    {
        nbElts[i][1] = i + first_rank;
        list = mpcomp_task_get_list(nbElts[i][1], type);
        nbElts[i][0] = (list) ? sctk_atomics_load_int(&list->nb_elements) : -1;
    }

    qsort_r(nbElts, nbVictims, 2 * sizeof(int), mpcomp_task_cmp_decr_func, ext_arg);

    for (i = 0; i < nbVictims; i++)
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
    return order[index - 1];
}

#endif /* MPCOMP_TASK */
