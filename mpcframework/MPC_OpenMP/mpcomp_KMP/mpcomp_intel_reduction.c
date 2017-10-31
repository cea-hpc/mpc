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
/* #   - DIONISI Thomas thomas.dionisi@exascale-computing.eu              # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_global.h"
#include "mpcomp_intel_reduction.h"

#include "mpcomp_sync.h"
#include "mpcomp_barrier.h"

int __kmp_determine_reduction_method(
    ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
    void *reduce_data, void (*reduce_func)(void *lhs_data, void *rhs_data),
    kmp_critical_name *lck, mpcomp_thread_t *t) {
  int retval = critical_reduce_block;
  int team_size;
  int teamsize_cutoff = 4;
  team_size = t->instance->team->info.num_threads;

  if (team_size == 1) {
    return empty_reduce_block;
  } else {
    int atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
    int tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;

    if (tree_available) {
      if (team_size <= teamsize_cutoff) {
        if (atomic_available) {
          retval = atomic_reduce_block;
        }
      } else {
        retval = tree_reduce_block;
      }
    } else if (atomic_available) {
      retval = atomic_reduce_block;
    }
  }

  /* force reduction method */
  if (__kmp_force_reduction_method != reduction_method_not_defined) {
    int forced_retval;
    int atomic_available, tree_available;
    switch ((forced_retval = __kmp_force_reduction_method)) {
    case critical_reduce_block:
      sctk_assert(lck);
      if (team_size <= 1)
        forced_retval = empty_reduce_block;
      break;
    case atomic_reduce_block:
      atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
      sctk_assert(atomic_available);
      break;
    case tree_reduce_block:
      tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
      sctk_assert(tree_available);
      forced_retval = tree_reduce_block;
      break;
    default:
      forced_retval = critical_reduce_block;
      sctk_debug("Unknown reduction method");
    }
    retval = forced_retval;
  }
  return retval;
}

int __mpcomp_internal_tree_reduce_nowait(void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpcomp_thread_t *t )
{
	mpcomp_node_t *c, *new_root;
	mpcomp_mvp_t *mvp = t->mvp;
	if ( t->info.num_threads == 1 )
		return 1;
	sctk_assert( mvp );
	sctk_assert( mvp->father );
	sctk_assert( mvp->father->instance );
	sctk_assert( mvp->father->instance->team );
	c = mvp->father;

	new_root = c->instance->root;

	int local_rank = t->rank % c->barrier_num_threads; /* rank of mvp on the node */
	int remaining_threads = c->barrier_num_threads; /* number of threads of the node still on reduction */ 
  int cache_size = 64; 
	while ( 1 )
	{
		while ( remaining_threads > 1 ) /* do reduction of the node */
		{
			if ( remaining_threads % 2 != 0 ) 
			{
remaining_threads = ( remaining_threads / 2 ) + 1;
				if ( local_rank == remaining_threads - 1 ) 
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
				}
				else if ( local_rank >= remaining_threads )
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
					if ( c->isArrived[local_rank * cache_size] == 0 )
						c->isArrived[local_rank * cache_size] = 1;
					while ( c->isArrived[local_rank * cache_size] != 0 ) 
					{
            /* need to wait for the pair thread doing the reduction operation before exiting function because compiler will delete reduce_data after */
						sctk_thread_yield(); 
						//sctk_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
            /* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] );
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}

			else
			{
				remaining_threads = remaining_threads / 2;
				if ( local_rank >= remaining_threads )
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( c->isArrived[local_rank * cache_size] != 0 )         
					{

            /* need to wait for the thread doing the reduction operation before exiting function because compiler will delete reduce_data after */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}
					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
            /* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}
					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] );
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
		}
		if ( c == new_root ) 
		{
			return 1; 
		}
		local_rank = c->rank;
		c = c->father;
		remaining_threads = c->barrier_num_threads;
    /* Climb on the tree */
	}
}

int __mpcomp_internal_tree_reduce(void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpcomp_thread_t *t )
{
	mpcomp_node_t *c, *new_root;
	mpcomp_mvp_t *mvp = t->mvp;
	if ( t->info.num_threads == 1 )
		return 1;
	sctk_assert( mvp );
	sctk_assert( mvp->father );
	sctk_assert( mvp->father->instance );
	sctk_assert( mvp->father->instance->team );
	c = mvp->father;

	new_root = c->instance->root;

	int local_rank = t->rank % c->barrier_num_threads; /* rank of mvp on the node */
	int remaining_threads = c->barrier_num_threads; /* number of threads of the node still on reduction */ 
  long b_done;
  int cache_size = 64; 
	while ( 1 )
	{
		while ( remaining_threads > 1 ) /* do reduction of the node */
		{
			if ( remaining_threads % 2 != 0 ) 
			{
        remaining_threads = ( remaining_threads / 2 ) + 1; 
				if ( local_rank == remaining_threads - 1 ) 
				{
        /* one thread put his contribution and go next step if remaining threads is odd */    
					c->reduce_data[local_rank * cache_size] = reduce_data;
				}
				else if ( local_rank >= remaining_threads )
				{
          b_done=c->barrier_done;
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( b_done == c->barrier_done ) 
            /* wait for master thread to end and share the result, this is done when it enters in __kmpc_end_reduce function */
					{
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}
          
	        while ( c->child_type != MPCOMP_CHILDREN_LEAF ) { /* Go down */
		      c = c->children.node[mvp->tree_rank[c->depth]];
		      c->barrier_done++; 
        	}
					return 0;
				}

				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
            /* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] );
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}

			else
			{
				remaining_threads = remaining_threads / 2;
				if ( local_rank >= remaining_threads )
				{
          b_done=c->barrier_done;
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( b_done == c->barrier_done )
            /* wait for master thread to end and share the result, this is done when it enters in __kmpc_end_reduce function */
					{
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}
          
	        while ( c->child_type != MPCOMP_CHILDREN_LEAF ) { /* Go down */ 
		      c = c->children.node[mvp->tree_rank[c->depth]];
		      c->barrier_done++; 
        	}
					
					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
            /* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}
					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] );
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
		}
		if ( c == new_root ) 
		{
			return 1; 
		}
		local_rank = c->rank;
		c = c->father;
		remaining_threads = c->barrier_num_threads;
    /* Climb on the tree */
	}
}

kmp_int32
__kmpc_reduce_nowait(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                     size_t reduce_size, void *reduce_data,
                     void (*reduce_func)(void *lhs_data, void *rhs_data),
                     kmp_critical_name *lck) {
  int retval = 0;
  int packed_reduction_method;
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  /* get reduction method */
  packed_reduction_method = __kmp_determine_reduction_method(
      loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t);
  t->reduction_method = packed_reduction_method;
  if (packed_reduction_method == critical_reduce_block) {
    __mpcomp_anonymous_critical_begin();
    retval = 1;
  } else if (packed_reduction_method == empty_reduce_block) {
    retval = 1;
  } else if (packed_reduction_method == atomic_reduce_block) {
    retval = 2;
  } else if (packed_reduction_method == tree_reduce_block) {
    retval = __mpcomp_internal_tree_reduce_nowait(reduce_data, reduce_func, t );
    /* retval = 1 for thread with reduction value, 0 for others */
  } else {
    sctk_error("__kmpc_reduce_nowait: Unexpected reduction method %d",
               packed_reduction_method);
    sctk_abort();
  }
  return retval;
}

void __kmpc_end_reduce_nowait(ident_t *loc, kmp_int32 global_tid,
                              kmp_critical_name *lck) {
  int packed_reduction_method;

  /* get reduction method */
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  packed_reduction_method = t->reduction_method;

  if (packed_reduction_method == critical_reduce_block) {
    __mpcomp_anonymous_critical_end();
    __mpcomp_barrier();
  } else if (packed_reduction_method == empty_reduce_block) {
  } else if (packed_reduction_method == atomic_reduce_block) {
  } else if (packed_reduction_method == tree_reduce_block) {
  } else {
    sctk_error("__kmpc_end_reduce_nowait: Unexpected reduction method %d",
               packed_reduction_method);
    sctk_abort();
  }
}

kmp_int32
__kmpc_reduce(ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                     size_t reduce_size, void *reduce_data,
                     void (*reduce_func)(void *lhs_data, void *rhs_data),
                     kmp_critical_name *lck) {
  int retval = 0;
  int packed_reduction_method;
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  /* get reduction method */
  packed_reduction_method = __kmp_determine_reduction_method(
      loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t);
  t->reduction_method = packed_reduction_method;
  if (packed_reduction_method == critical_reduce_block) {
    __mpcomp_anonymous_critical_begin();
    retval = 1;
  } else if (packed_reduction_method == empty_reduce_block) {
    retval = 1;
  } else if (packed_reduction_method == atomic_reduce_block) {
    retval = 2;
  } else if (packed_reduction_method == tree_reduce_block) {
    retval = __mpcomp_internal_tree_reduce(reduce_data, reduce_func, t );
    /* retval = 1 for thread with reduction value (thread 0), 0 for others. When retval = 0 the thread doesn't enter __kmpc_end_reduce function */
  } else {
    sctk_error("__kmpc_reduce_nowait: Unexpected reduction method %d",
               packed_reduction_method);
    sctk_abort();
  }
  return retval;
}

void __kmpc_end_reduce(ident_t *loc, kmp_int32 global_tid,
    kmp_critical_name *lck) {

  int packed_reduction_method;

  /* get reduction method */
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  packed_reduction_method = t->reduction_method;
  if (packed_reduction_method == critical_reduce_block) {
    __mpcomp_anonymous_critical_end();
    __mpcomp_barrier();
  } else if (packed_reduction_method == atomic_reduce_block) {
    __mpcomp_barrier();
  } else if (packed_reduction_method == tree_reduce_block) {
    /* For tree reduction algorithm when thread 0 enter __kmpc_end_reduce reduction value is already shared among all threads */
    mpcomp_mvp_t *mvp = t->mvp; 
    mpcomp_node_t *c = t->instance->root;
    c->barrier_done++;
    /* Go down to release all threads still waiting on __mpcomp_internal_tree_reduce for the reduction to be done, we don't need to do a full barrier  */
    while ( c->child_type != MPCOMP_CHILDREN_LEAF ) { 
      c = c->children.node[mvp->tree_rank[c->depth]];
      c->barrier_done++; 
    }
  }
}

