
#define _GNU_SOURCE
#include <stdlib.h>

#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>

#include "mpcomp_internal.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

#if MPCOMP_TASK

/* Compare nb element in first entry of tab
 */
static inline int 
mpcomp_task_cmp_decr_func( const void * opq_a, const void *opq_b ) 
{
    const int * a = (int*) opq_a;
    const int * b = (int*) opq_b;
    return (a[0] == b[0]) ? b[1] - a[1] : b[0] - a[0];
}  

static inline int
mpcomp_task_get_depth_from_global_rank(mpcomp_mvp_t *mvp, mpcomp_instance_t* instance, const int globalRank)
{
	mpcomp_thread_t* thread;

	sctk_assert( mvp );
	sctk_assert( instance );
	sctk_assert( globalRank > 0 && globalRank < MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE( instance ) ); 	

    return  (mpcomp_is_leaf( instance, globalRank ) ) ? 
            MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(mvp, globalRank)->father->depth + 1:
            MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(mvp, globalRank)->depth;
}

int 
mpcomp_task_get_victim_default(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    return globalRank;
}

/** Hierachical stealing policy **/

void
mpcomp_task_init_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

static void
mpcomp_task_prepare_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

int 
mpcomp_task_get_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    return mpcomp_get_neighbour(globalRank, index);
}

/** Random stealing policy **/

void
mpcomp_task_init_victim_random(mpcomp_node_t* node, int globalRank)
{
	struct drand48_data* randBuffer = NULL;
	sctk_assert( node != NULL ); 

	if( MPCOMP_TASK_NODE_GET_TASK_LIST_RANK_BUFFER( node ) )
	{
   	return; 
	}
	
	randBuffer = (struct drand48_data*) mpcomp_malloc( 1, sizeof(struct drand48_data), node->id_numa);
	sctk_assert( randBuffer );	
	srand48_r( sctk_get_time_stamp() * globalRank, randBuffer );
	MPCOMP_TASK_NODE_SET_TASK_LIST_RANK_BUFFER( node, randBuffer );
}

static void
mpcomp_task_prepare_victim_random(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

int 
mpcomp_task_get_victim_random(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
   int victim;
   long int value;
   mpcomp_thread_t* thread;
   struct drand48_data* randBuffer;

   /* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls != NULL );
   thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	sctk_assert( thread->mvp );
	sctk_assert( thread->instance );

	/* Only victim id and value while change */
   const int __globalRank = globalRank;
   const int __depth = mpcomp_task_get_depth_from_global_rank( thread->mvp, thread->instance, globalRank );
   const int __first_rank = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( thread->instance, __depth );
   const int __level_size = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, __depth );
   randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANK_BUFFER( thread->mvp, __globalRank );
    
    do{
        /* Get a random value */
        lrand48_r( randBuffer, &value );
        /* Select randomly a globalRank among neighbourhood */
        victim = ( value % __level_size + __first_rank );
      } while( victim == __globalRank ); /* Retry random draw if the victim is the thief */ 
   
   return victim;
}

/** Random order stealing policy **/

static void
mpcomp_task_prepare_victim_random_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
   int i;
   int *order;
   mpcomp_thread_t* thread;
   struct drand48_data *randBuffer;

   sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

   if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
        mpcomp_task_allocate_larceny_order( thread );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	sctk_assert( order );

	sctk_assert( thread->mvp ); 
	sctk_assert( thread->instance ); 

   const int __globalRank = globalRank;
	const int __depth = mpcomp_task_get_depth_from_global_rank( thread->mvp, thread->instance, globalRank ); 
   const int __first_rank = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( thread->instance, __depth );
   const int __nbVictims = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, __depth ) - 1;
	randBuffer = MPCOMP_TASK_MVP_GET_TASK_LIST_RANK_BUFFER( thread->mvp, __globalRank ); 

   order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	sctk_assert( order );

   for(i = 0; i < __nbVictims; i++)
   {
   	order[i] = __first_rank + i;
   	order[i] = ( order[i] >= __globalRank ) ? order[i] + 1 : order[i]; 
   }
    
   for(i = 0; i < __nbVictims; i++) 
	{
		long int value;
      int tmp = order[i]; 
      lrand48_r( randBuffer, &value );
      int j = value % ( __nbVictims - i );
      order[i] = order[i+j];
      order[i+j] = tmp;
	}
}

int 
mpcomp_task_get_victim_random_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
	int *order;
   mpcomp_thread_t* thread;

   /* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
 
	if(index == 1)
	{
   	mpcomp_task_prepare_victim_random_order(globalRank, index, type);
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	sctk_assert( order );
	sctk_assert( order[index-1] >= 0 && order[index-1]  != globalRank );
	return order[index-1];
}

static void
mpcomp_task_prepare_victim_roundrobin(int globalRank, int index, mpcomp_tasklist_type_t type)
{
   int i;
   int *order;
	mpcomp_thread_t* thread;

   /* Retrieve the information (microthread structure and current region) */
	sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

   if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
        mpcomp_task_allocate_larceny_order( thread );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	sctk_assert( order );

	sctk_assert( thread->mvp ); 
	sctk_assert( thread->instance ); 

   const int __globalRank 	= globalRank;
   const int __depth 		= mpcomp_task_get_depth_from_global_rank( thread->mvp, thread->instance, globalRank );
   const int __first_rank 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( thread->instance, __depth );
   const int __nbVictims 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, __depth ) - 1;
	const int __decal 		= ( __globalRank - __first_rank + 1 ) % ( __nbVictims + 1 );

	for(i = 0; i < __nbVictims; i++)
   {
		order[i] = __first_rank + (__decal + i) % (__nbVictims + 1); 
     	order[i] = (order[i] >= __globalRank) ? order[i] + 1 : order[i]; 
	}
}

int 
mpcomp_task_get_victim_roundrobin(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
	int *order;
	mpcomp_thread_t* omp_thread_tls;

	/* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls );
   omp_thread_tls = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    
	if( index == 1 )
	{
   	mpcomp_task_prepare_victim_roundrobin( globalRank, index, type );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( omp_thread_tls );
	sctk_assert( order );
	sctk_assert( order[index-1] >= 0 && order[index-1]  != globalRank );
	return order[index-1];
}

int 
mpcomp_task_get_victim_producer(int globalRank, int index, mpcomp_tasklist_type_t type)
{
   int max_elt, victim, i;
   mpcomp_thread_t* thread;

   /* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls );
   thread = (mpcomp_thread_t *) sctk_openmp_thread_tls;

	sctk_assert( thread->mvp ); 
	sctk_assert( thread->instance ); 

   const int __globalRank 	= globalRank;
   const int __depth 		= mpcomp_task_get_depth_from_global_rank( thread->mvp, thread->instance, globalRank );
   const int __first_rank 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( thread->instance, __depth );
   const int __nbVictims 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, __depth ) - 1;
    
    /* On recherche la liste avec le plus de tache a voler */
    max_elt = 0;
    victim = (globalRank == __first_rank) ? __first_rank + 1 : __first_rank;
    
    for(i = 0; i < __nbVictims; i++)
    {
        int rank, nb_elt;
        mpcomp_task_list_t* list;

        rank = i + __first_rank;
        rank += (rank >= __globalRank) ? 1 : 0;
        list = ( mpcomp_task_list_t *) mpcomp_task_get_list( rank, type );        
        nb_elt = sctk_atomics_load_int( &( list->nb_elements ) ); 
        
        if(max_elt < nb_elt)
        {
            max_elt = nb_elt;
            victim = rank;
        }
    } 

    return victim;
}

static inline int 
mpcomp_task_prepare_victim_producer_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
   int i;
   int *order; 
	mpcomp_thread_t* thread;

   /* Retrieve the information (microthread structure and current region) */
	sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

   if( !MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) )
	{
        mpcomp_task_allocate_larceny_order( thread );
	}

	sctk_assert( thread->mvp ); 
	sctk_assert( thread->instance ); 

   const int __globalRank 	= globalRank;
   const int __depth 		= mpcomp_task_get_depth_from_global_rank( thread->mvp, thread->instance, globalRank );
   const int __first_rank 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( thread->instance, __depth );
   const int __nbVictims 	= MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, __depth ) - 1;
   
	int nbElements[__nbVictims][2];

	for(i = 0; i < __nbVictims; i++)
   {
   	struct mpcomp_task_list_s *list;
      nbElements[i][1] = i + __first_rank; 
      nbElements[i][1] += (nbElements[i][1] >= __globalRank) ? 1 : 0;
      list = (struct mpcomp_task_list_s *) mpcomp_task_get_list(nbElements[i][1], type);        
      nbElements[i][0] = sctk_atomics_load_int(&list->nb_elements);
	} 

    void* extra_arg;
    qsort_r( nbElements, __nbVictims, 2*sizeof(int), mpcomp_task_cmp_decr_func, extra_arg );  
    
   for(i = 0; i < __nbVictims; i++)
	{
   	order[i] = nbElements[i][1];
	}

	return 0;
}

int 
mpcomp_task_get_victim_producer_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
	int *order;
   mpcomp_thread_t* thread;

   /* Retrieve the information (microthread structure and current region) */
	sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    
   if(index == 1)
	{
   	mpcomp_task_prepare_victim_producer_order( globalRank, index, type );
	}

	order = MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread );
	sctk_assert( order );
	sctk_assert( order[index-1] >= 0 && order[index-1]  != globalRank );
	return order[index-1];
}

#endif /* MPCOMP_TASK */
