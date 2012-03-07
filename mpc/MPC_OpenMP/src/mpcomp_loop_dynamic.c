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
#include <mpcomp.h>
#include <mpcomp_abi.h>
#include <sctk_debug.h>
#include "mpcomp_internal.h"
#include "sctk.h"
#include "mpcmicrothread_internal.h"


/*
  Barrier for for dynamic construct
*/
void __mpcomp_barrier_for_dyn(void)
{
  mpcomp_thread_t *t;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Block only if I am not the only thread in the team */
  if (t->team->num_threads > 1) {
    __mpcomp_internal_barrier_for_dyn(t);
  }
  
}

/*
  Internal barrier called in the for dynamic barrier
*/
void __mpcomp_internal_barrier_for_dyn(mpcomp_thread_t *t)
{
   mpcomp_thread_team_t *team;
   mpcomp_mvp_t *mvp;
   mpcomp_node_t *c;
   int index;
   int b_done;
   int b;

   /* Grab the team info */
   team = t->team;
   sctk_assert(team != NULL);
 
   /* Grab the corresponding microVP */
   mvp = t->mvp;
   sctk_assert(mvp != NULL);
  
   /* Grab the index of the current loop */
   index = (t->private_current_for_dyn) % MPCOMP_MAX_ALIVE_FOR_DYN;
 
   /*Barrier to wait for the other microVPs*/
   c = mvp->father; 
   sctk_assert(c != NULL);

   /* Step 1: Climb in the tree */
   b_done = c->barrier_done;

#ifdef ATOMICS
   b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
   //sctk_atomics_write_barrier(); 

   while ((b+1 == c->barrier_num_threads) && (c->father != NULL)) {
      sctk_atomics_store_int (&c->barrier,0) ;

      c = c->father;

      b_done = c->barrier_done;      

      b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
      //sctk_atomics_write_barrier(); 
   }

   /* Step 2: Wait for the barrier to be done */
   if ((c->father != NULL) || (c->father == NULL && b+1 != c->barrier_num_threads)) {
     /* Wait for c->barrier == c->barrier_num_threads */ 
     while (b_done == c->barrier_done) {
        sctk_thread_yield();
     }

   }
   else {
     /* Reinitialize some information */
     sctk_spinlock_lock(&(team->lock_stop_for_dyn));
     team->stop[index] = MPCOMP_OK;

     if (index == 0) 
         team->stop[MPCOMP_MAX_ALIVE_FOR_DYN-1] = MPCOMP_STOP;
     else	      
         team->stop[index-1] = MPCOMP_STOP;

     sctk_spinlock_unlock(&(team->lock_stop_for_dyn));

     sctk_spinlock_lock(&(team->lock_exited_for_dyn[index]));
     team->nthread_exited_for_dyn[index] = 0;
     sctk_spinlock_unlock(&(team->lock_exited_for_dyn[index]));

     sctk_atomics_store_int (&c->barrier,0);
     c->barrier_done++;
     /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
   }

#else
   sctk_spinlock_lock(&(c->lock));
   b = c->barrier;
   b++;
   c->barrier = b;
   sctk_spinlock_unlock(&(c->lock));

   while ((b == c->barrier_num_threads) && (c->father != NULL)) {
      c->barrier = 0;

      c = c->father;

      b_done = c->barrier_done;      

      sctk_spinlock_lock(&(c->lock));
      b = c->barrier;
      b++;
      c->barrier = b;
      sctk_spinlock_unlock(&(c->lock));
   }

   /* Step 2: Wait for the barrier to be done */
   if ((c->father != NULL) || (c->father == NULL && b != c->barrier_num_threads)) {

     /* Wait for c->barrier == c->barrier_num_threads */ 
     while (b_done == c->barrier_done) {
        sctk_thread_yield();
     }
   }
   else {
     c->barrier = 0;
     c->barrier_done++;
     /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
   }
#endif


   /* Step 3: Go down in the tree to wake up the children */
   while (c->child_type != CHILDREN_LEAF) {
       c = c->children.node[mvp->tree_rank[c->depth]];
       c->barrier_done++;
   }
}


/* 
  Start a new for dynamic construct
*/
int __mpcomp_dynamic_loop_begin(int lb, int b, int incr, int chunk_size, int *from, int *to)
{
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;
  int rank;
  int index;
  int num_threads;
  int remain;
  int chunk_id;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Number of threads in the current team */
  num_threads = t->team->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
   * if this thread is the only one of its team -> it executes the whole loop */
  if (num_threads == 1){
    *from = lb;
    *to = b;
    return 1;
  }

  /* Get the team info */
  team = t->team;
  sctk_assert(team != NULL);

  /* Get the rank of the current thread */
  rank = t->rank;  

  /* Grab the index of the current loop */
  index = (t->private_current_for_dyn) % MPCOMP_MAX_ALIVE_FOR_DYN;

  /* If it is a barrier */
  if (team->stop[index] != MPCOMP_OK) {

    sctk_spinlock_lock(&(team->lock_stop_for_dyn));

    /* Is it the last loop to perform before synchronization? */
    if (team->stop[index] == MPCOMP_STOP) {

      team->stop[index] = MPCOMP_CONSUMED;
      sctk_spinlock_unlock(&(team->lock_stop_for_dyn));

      /* Call barrier */
      __mpcomp_barrier_for_dyn();

      return __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);       

    }
   
    /* This loop has been the last loop before synchronization */ 
    if (team->stop[index] == MPCOMP_CONSUMED) {

      sctk_spinlock_unlock(&(team->lock_stop_for_dyn));

      /* Call barrier */
      __mpcomp_barrier_for_dyn();

      return __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);       
    }

    sctk_spinlock_unlock(&(team->lock_stop_for_dyn));

  }

  /* Index of the next loop */
  t->private_current_for_dyn = index + 1;

  /* Check the number of remaining chunk for this index and this rank */
  //sctk_spinlock_lock(&(t->lock_for_dyn[rank][index]));
  sctk_spinlock_lock(&(team->lock_for_dyn[rank][index]));

  remain = team->chunk_info_for_dyn[rank][index].remain;

  /* remain == -1 -> nobody stole my work */
  if (remain == -1) {

    int total_nb_chunks;

    /* Compute the number of chunks for the thread "rank"  */
    total_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank(rank, num_threads, lb, b, incr, chunk_size); 
    team->chunk_info_for_dyn[rank][index].total = total_nb_chunks;
    
    /* First chunk is been scheduled */
    team->chunk_info_for_dyn[rank][index].remain = total_nb_chunks - 1;

    sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

    if (total_nb_chunks == 0) {
      sctk_spinlock_lock(&(team->lock_for_dyn[rank][index]));
      team->chunk_info_for_dyn[rank][index].remain = 0;
      sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

      sctk_nodebug("__mpcomp_dynamic_loop_begin[%d]: No chunk 0",rank);
      return 0;
    }
 
    /* Fill private information about the current loop */
    t->loop_lb = lb;
    t->loop_b = b;
    t->loop_incr = incr;
    t->loop_chunk_size = chunk_size;

    /* Current chunk */
    chunk_id = 0;

    __mpcomp_get_specific_chunk_per_rank(rank, num_threads, lb, b, incr, chunk_size, chunk_id, from, to);

    sctk_openmp_thread_tls = t; 

    return 1+chunk_id;
  }

  /* remain == 0 -> somebody stole everything */
  if (remain == 0) {

    sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

    /* TODO: this thread has no more chunks for this loop. Should we fill the private info about the loop anyway ? Maybe not. */
    /* Fill private information about the current loop */
    t->loop_lb = lb;
    t->loop_b = b;
    t->loop_incr = incr;
    t->loop_chunk_size = chunk_size;

    /* TODO: Try to steal someone ? */
    

    sctk_openmp_thread_tls = t; 

    return 0;
  }

  /* remain > 0 -> somebody stole at least one chunk and there is still at least one chunk to schedule */ 
  chunk_id =  team->chunk_info_for_dyn[rank][index].total - remain;

  /* TODO use atomics to decrement */
  team->chunk_info_for_dyn[rank][index].remain--;

  sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

  /* Fill the private information about the current loop */  
  t->loop_lb = lb;
  t->loop_b = b;
  t->loop_incr = incr;
  t->loop_chunk_size = chunk_size;

  __mpcomp_get_specific_chunk_per_rank(rank, num_threads, lb, b, incr, chunk_size, chunk_id, from, to);

  sctk_openmp_thread_tls = t;

  return 1+chunk_id;
  
}


/*
  Check if there is still chunks to execute for this for dynamic construct
*/
int __mpcomp_dynamic_loop_next(int *from, int *to)
{
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;
  int rank;
  int index;
  int num_threads;
  int remain;
  int chunk_id;
  int total_nb_chunks;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Number of threads in the current team */
  num_threads = t->team->num_threads;

  if (num_threads == 1) {
    return 0;
  }  

  /* Grab the rank of the current thread */
  rank = t->rank;

  /* Grab the team info */
  team = t->team;
  sctk_assert(team != NULL);

  /* Index of the current loop */
  index = t->private_current_for_dyn-1;

  sctk_spinlock_lock(&(team->lock_for_dyn[rank][index]));

  /* Grab the remaining chunks for rank for index */
  remain = team->chunk_info_for_dyn[rank][index].remain;

  /* Is there remaining chunks? */
  if (remain == 0) {
    sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

    /* TODO steal a chunk for other threads */
    /* There is still a bug in the current stealing.  
       When thread i steals a chunk from thread j it seems like 
       thread j does not see the stealing and executes the "stolen" chunk.
       That mean that a stolen chunk is executed twice */
#if 0
    int i;
    int chunk_id;
    int stolen_rank;
    int stolen_remain;
    int total_nb_chunks;
     
    for (i=1 ; i<num_threads ; i++) {
      stolen_rank = (i+rank) % num_threads;

      sctk_nodebug("__mpcomp_dynamic_loop_next: start stealing rank %d stolen rank %d",rank,stolen_rank);

      sctk_spinlock_lock(&(team->lock_for_dyn[stolen_rank][index]));
      
      stolen_remain = team->chunk_info_for_dyn[stolen_rank][index].remain;

      if (stolen_remain == -1) {
        /* The stolen thread has not achieved this loop yet. Steal its first chunk */
        sctk_nodebug("__mpcomp_dynamic_loop_next[%d]: stolen thread has not achieved the loop %d",stolen_rank,index);

        /* Compute the number of chunks for the thread "rank"  */
        total_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank(stolen_rank, num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size); 
        team->chunk_info_for_dyn[stolen_rank][index].total = total_nb_chunks;

        sctk_debug("__mpcomp_dynamic_loop_next before --: remain %d rank %d stolen rank %d",stolen_remain,rank,stolen_rank);

        /* First chunk is been scheduled */
        team->chunk_info_for_dyn[stolen_rank][index].remain = total_nb_chunks - 1;

        sctk_debug("__mpcomp_dynamic_loop_next after ++: remain %d rank %d stolen rank %d", team->chunk_info_for_dyn[stolen_rank][index].remain,rank,stolen_rank);

        sctk_spinlock_unlock(&(team->lock_for_dyn[stolen_rank][index]));

        if (total_nb_chunks == 0) {
          sctk_spinlock_lock(&(team->lock_for_dyn[stolen_rank][index]));
          team->chunk_info_for_dyn[stolen_rank][index].remain = 0;
          sctk_spinlock_unlock(&(team->lock_for_dyn[stolen_rank][index]));

          sctk_nodebug("__mpcomp_dynamic_loop_next[%d]: No chunk 0",stolen_rank);

          continue ;
        }

        /* Current chunk */
        chunk_id = 0;

        __mpcomp_get_specific_chunk_per_rank(stolen_rank, num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, chunk_id, from, to);
 
        return 1+chunk_id; 
      }

      if (stolen_remain == 0) {
        /* The stolen thread has no more chunks. All chunks have been stolen */
        sctk_nodebug("__mpcomp_dynamic_loop_next[%d]: stolen thread has no more chunks for loop %d",stolen_rank,index);
        sctk_debug("__mpcomp_dynamic_loop_next: remain %d rank %d stolen rank %d",stolen_remain,rank,stolen_rank);

        sctk_spinlock_unlock(&(team->lock_for_dyn[stolen_rank][index])); 

        continue ;
      }
  
      /* The stolen thread is currently working on this loop */
      sctk_nodebug("__mpcomp_dynamic_loop_next[%d]: stolen thread is working on loop %d",stolen_rank,index);
      sctk_debug("__mpcomp_dynamic_loop_next: remain %d rank %d stolen rank %d",stolen_remain,rank,stolen_rank);

      total_nb_chunks = team->chunk_info_for_dyn[stolen_rank][index].total;
      team->chunk_info_for_dyn[stolen_rank][index].remain = stolen_remain-1;

      sctk_spinlock_unlock(&(team->lock_for_dyn[stolen_rank][index]));
      
      chunk_id = total_nb_chunks - stolen_remain;

      __mpcomp_get_specific_chunk_per_rank(stolen_rank, num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, chunk_id, from, to);

      return 1+chunk_id;
    }
#endif

    /* Nothing to steal */
    return 0;    
  }

  /* Grab the total number of chunks for rank for loop "index" */
  total_nb_chunks = team->chunk_info_for_dyn[rank][index].total;
  
  /* Current chunk id */
  chunk_id = total_nb_chunks - remain;

  /* TODO use atomics to decrement */
  team->chunk_info_for_dyn[rank][index].remain--;

  sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));

  __mpcomp_get_specific_chunk_per_rank(rank, num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, chunk_id, from, to);
 
  sctk_openmp_thread_tls = t; 

  return 1+chunk_id;
}


/*
  End of the for dynamic
*/
void __mpcomp_dynamic_loop_end()
{
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team; 
  int rank;
  int index;

  /* Grab the info of the current thread */
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Grab the rank of the thread */
  rank = t->rank;

  /* Index of the current loop */
  index = t->private_current_for_dyn-1;

  /* Grab the team info */
  team = t->team;
  sctk_assert(team != NULL);

  __mpcomp_barrier_for_dyn();

  /* Reinitialize some information */
  team->chunk_info_for_dyn[rank][index].remain = -1;
}


/*
  End of the for dynamic nowait
*/
void __mpcomp_dynamic_loop_end_nowait()
{
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;
  int i;
  int rank;
  int index;
  int prev_index;
  int num_threads;
  int nb_exited_threads;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Grab the team info */
  team = t->team;
  sctk_assert(team != NULL);

  /* Number of threads of the current team */  
  num_threads = t->team->num_threads;

  if (num_threads == 1) {
    return;
  }

  /* Grab the rank of the current thread */
  rank = t->rank;

  /* Grab the index of the current loop */
  index = t->private_current_for_dyn-1;

  sctk_spinlock_lock(&(team->lock_exited_for_dyn[index]));

  /* TODO use atomics here */
  nb_exited_threads = team->nthread_exited_for_dyn[index];
  nb_exited_threads++;
  team->nthread_exited_for_dyn[index] = nb_exited_threads;

  sctk_spinlock_unlock(&(team->lock_exited_for_dyn[index]));

  if (nb_exited_threads == num_threads) {
    /* Change the index of the last dynamic for to process before synchronization */
    sctk_spinlock_lock(&(team->lock_stop_for_dyn));

    team->stop[index] = MPCOMP_STOP;
    /* Set to OK the previous STOP index */
    prev_index = index-1;
    if (index == 0) prev_index = MPCOMP_MAX_ALIVE_FOR_DYN-1;

    if (team->stop[prev_index] != MPCOMP_CONSUMED)
      team->stop[prev_index] = MPCOMP_OK;

    sctk_spinlock_unlock(&(team->lock_stop_for_dyn));

    /* Reinitialize some information */
    sctk_spinlock_lock(&(team->lock_exited_for_dyn[index]));
    team->nthread_exited_for_dyn[index] = 0;
    sctk_spinlock_unlock(&(team->lock_exited_for_dyn[index]));

    /* TODO free the threads that are waiting on a barrier. Set their t->barrier to t->barrier_num_threads */
    /* TODO to know who is waiting on a barrier add a table that contains the rank of each waiting thread */
    /* TODO this table should be reset (to -1) at the end of the barrier */
  }

  sctk_spinlock_lock(&(team->lock_for_dyn[rank][index]));

  team->chunk_info_for_dyn[rank][index].remain = -1;

  sctk_spinlock_unlock(&(team->lock_for_dyn[rank][index]));
 
  sctk_openmp_thread_tls = t;

}

/*
  Search in tree for other chunks to steal
*/
void __mpcomp_steal_chunk(mpcomp_mvp_t *dest_mvp, int start_index, int *dest_index)
{
  int node_index;
  int i;
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;
  mpcomp_mvp_t *mvp;
  mpcomp_node_t *node;
  mpcomp_node_t *n;
  int num_threads;
  int index;
  int target_rank;
  int rank;
  int remain;
  int mvp_rank;
  int mvp_subtree_rank;
  int node_rank;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Grab the team info */
  team = t->team;
  sctk_assert(team != NULL);

  /* Number of threads of the current team */  
  num_threads = t->team->num_threads;

  /* Grab the rank of current thread */
  rank = t->rank;
  
  /* Grab the current mvp */
  mvp = t->mvp;

  /* Grab the index of the current loop */
  index = (t->private_current_for_dyn) % MPCOMP_MAX_ALIVE_FOR_DYN;
  //remain = team->chunk_info_for_dyn[rank][index].remain;

  node = mvp->father;
 
  /* current thread is out of chunks */
  node->nb_children_chunks_idle += 1;
  
  if(node->nb_children_chunks_idle == node->nb_children) {
    node->chunks_avail = CHUNK_NOT_AVAIL;
  }

  mvp_subtree_rank = mvp->tree_rank[node->depth];  

  for(i=mvp_subtree_rank+1;i<(mvp_subtree_rank+node->nb_children)%(node->nb_children);i++) {
   mvp_rank = node->children.leaf[i]->rank;

   remain = team->chunk_info_for_dyn[mvp_rank][index].remain;
 
   if(remain != 0) {
    dest_mvp = node->children.leaf[i];
    break; 
   }
    
  }
  
  if(remain != 0) {
   
    /* Do steal a chunk */
    team->chunk_info_for_dyn[mvp_rank][index].remain--;
    team->chunk_info_for_dyn[rank][index].remain++; 
  }
  else 
  {
    while(node->father != NULL) {
      
      node_rank = node->rank;
 
      if(node->chunks_avail == CHUNK_NOT_AVAIL) {
        node->father->nb_children_chunks_idle += 1;
      }

      node = node->father;

      if(node->father->nb_children_chunks_idle == node->father->nb_children) {
        node->father->chunks_avail = CHUNK_NOT_AVAIL;
      }
      else {
     
       for(i=node_rank+1;i<(node_rank+node->nb_children)%(node->nb_children);i++) {
        
         n = node->children.node[i];

         /* Do a classic in depth tree search */
         mpcomp_in_depth_tree_search(n, &target_rank); 

         if(target_rank != -1) {
           break;
         }
         else {
           n->chunks_avail = CHUNK_NOT_AVAIL;
         } 
          
        }
      }
      
      if (target_rank != -1) {
       break;
      }
    }
  
    if (target_rank != -1) {
      //Do steal a chunk
      team->chunk_info_for_dyn[target_rank][index].remain--;
      team->chunk_info_for_dyn[rank][index].remain++; 
    }
  }

}




#if 0
void
__old_mpcomp_barrier_for_dyn (void) ;

int __old_mpcomp_dynamic_loop_begin (int lb, int b, int incr,
			     int chunk_size, int *from, int *to)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;			/* Rank of the thread calling this function */
  int index;			/* Index of this loop */
  int num_threads;		/* Number of threads of the current team */
  int remain ;
  int n;			/* Number of remaining iterations */

  /* Compute the total number iterations */
  n = (b - lb) / incr;

  /* If this loop contains no iterations then exit */
  if ( n <= 0 ) {
    return 0 ;
  }

  /* Grab the thread info */
  /* TODO Use TLS if available */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
   * if this thread is the only one of its team -> it executes the whole loop 
   */
  if (num_threads == 1)
    {
      *from = lb;
      *to = b;
      return 1;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;

  /* Compute the new index of this loop */
  index = (self->private_current_for_dyn+1)%(MPCOMP_MAX_ALIVE_FOR_DYN + 1);
  self->private_current_for_dyn = index ;

  sctk_nodebug
    ("__mpcomp_dynamic_loop_begin[%d]: "
     "Entering loop (%d -> %d step %d [cs=%d]) with "
     "index %d, microVP=%d (STOP=%d)",
     rank, 
     lb, b, incr, chunk_size, 
     index, self->vp, father->stop_index_for_dyn );

  /* Check if the current index represents a barrier or not */
  if ( father->stop_index_for_dyn == index ) {
    int stop_index ;
    int stop_value ;

    sctk_nodebug( "__mpcomp_dynamic_loop_begin[%d]: Maybe blocked at index %d",
	rank, index ) ;

    sctk_spinlock_lock (&(father->lock_stop_for_dyn));

    stop_index = father->stop_index_for_dyn ;
    stop_value = father->stop_value_for_dyn ;

    if ( stop_index == index ) {
      if ( stop_value == MPCOMP_NOWAIT_STOP_SYMBOL) {
	father->stop_value_for_dyn = MPCOMP_NOWAIT_STOP_CONSUMED ;

	sctk_spinlock_unlock (&(father->lock_stop_for_dyn));

	sctk_nodebug( "__mpcomp_dynamic_loop_begin[%d]: Blocked at index %d STOP->STOPPED",
	    rank, index ) ;

	/* Generate a barrier (it should reinitialize all counters) */
INFO("OpenMP: For/Dyn: Check which barrier is used")
	__mpcomp_barrier_for_dyn();
	/* Recall the function */
	return __mpcomp_dynamic_loop_begin (lb, b, incr, chunk_size, from, to);
      }

      if ( stop_value == MPCOMP_NOWAIT_STOP_CONSUMED ) {
	sctk_spinlock_unlock (&(father->lock_stop_for_dyn));

	sctk_nodebug( "__mpcomp_dynamic_loop_begin[%d]: Blocked at index %d STOPPED",
	    rank, index ) ;

	/* Generate a barrier (it should reinitialize all counters) */
INFO("OpenMP: For/Dyn: Check which barrier is used")
	__mpcomp_barrier_for_dyn();
	/* Recall the function */
	return __mpcomp_dynamic_loop_begin (lb, b, incr, chunk_size, from, to);
      }
    }

    sctk_spinlock_unlock (&(father->lock_stop_for_dyn));
  }

  /* Check the number of remaining chunk for this index and this rank */
  sctk_spinlock_lock (&(father->lock_for_dyn[rank][index]));

  remain = father->chunk_info_for_dyn[rank][index].remain ;

  /* If nobody stole my work already */
  if ( remain == -1 ) {
    int total_nb_chunks ;

    
    total_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank( rank,
	num_threads, lb, b, incr, chunk_size); 
    father->chunk_info_for_dyn[rank][index].remain = total_nb_chunks - 1 ;
    father->chunk_info_for_dyn[rank][index].total  = total_nb_chunks ;

    sctk_nodebug
      ("__mpcomp_dynamic_loop_begin[%d]: (FIRST)First one -> index %d, #chunks %d",
       rank, index, total_nb_chunks );

    sctk_spinlock_unlock (&(father->lock_for_dyn[rank][index]));

    /* Fill info about the current loop */
    self->loop_lb = lb;
    self->loop_b = b;
    self->loop_incr = incr;
    self->loop_chunk_size = chunk_size;

    if ( total_nb_chunks == 0 ) {
      sctk_nodebug
	("__mpcomp_dynamic_loop_begin[%d]: (FIRST) No chunk 0", rank ) ;
      return 0 ;
    }

    __mpcomp_get_specific_chunk_per_rank (rank,
	num_threads,
	lb, b, incr,
	chunk_size, 0, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_begin[%d]: (FIRST)chunk 0: %d -> %d", rank,
       *from, *to);

    return 1 ;
  }

  /* If somebody stole everything */
  if ( remain == 0 ) {
    sctk_spinlock_unlock (&(father->lock_for_dyn[rank][index]));
    /* Fill info about the current loop */
    self->loop_lb = lb;
    self->loop_b = b;
    self->loop_incr = incr;
    self->loop_chunk_size = chunk_size;
    /* TODO Try to steal someone? */

    sctk_nodebug
      ("__mpcomp_dynamic_loop_begin[%d]: (LAST)Already stolen -> index %d",
       rank, index );

    return 0 ;
  }

  int chunk_id = father->chunk_info_for_dyn[rank][index].total - remain ;
  father->chunk_info_for_dyn[rank][index].remain = remain - 1 ;

  sctk_spinlock_unlock (&(father->lock_for_dyn[rank][index]));

    /* Fill info about the current loop */
    self->loop_lb = lb;
    self->loop_b = b;
    self->loop_incr = incr;
    self->loop_chunk_size = chunk_size;


    __mpcomp_get_specific_chunk_per_rank (rank,
	num_threads,
	lb, b, incr,
	chunk_size, chunk_id, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_begin[%d]: (MIDDLE)chunk %d: %d -> %d", rank, chunk_id,
       *from, *to);

    return 1;
}

int
__old_mpcomp_dynamic_loop_next (int *from, int *to)
{
  mpcomp_thread_info_t *self;
  mpcomp_thread_info_t *father;
  long rank;
  int index;
  int num_threads;
  int i ;
  int remain ;

  /* Grab the thread info */
  /* TODO Use TLS if available */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive),
     or a team w/ only 1 thread, then the current thread is done 
     (the whole loop has been executed after the 'start' function) */
  if (num_threads == 1)
    {
      return 0;
    }

  /* Get the rank of the current thread */
  rank = self->rank;

  /* Get the father info */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the index of the current loop */
  index = self->private_current_for_dyn ;

  sctk_nodebug
    ("__mpcomp_dynamic_loop_next[%d]: Next in loop for index %d on microVP=%d",
     rank, index, self->vp);

  sctk_spinlock_lock (&(father->lock_for_dyn[rank][index]));

  remain = father->chunk_info_for_dyn[rank][index].remain ;

  /* Is there anything to do? */
  if ( remain == 0 ) {
    /* If not... */

    sctk_spinlock_unlock (&(father->lock_for_dyn[rank][index]));

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: Nothing left, try stealing...",
       rank );

    for ( i = 1 ; i < num_threads ; i++ ) {
      int target_rank = (i + rank)%num_threads ;
      int target_remain ;


      /* Trying to steal work from thread 'target_rank' */

      sctk_spinlock_lock (&(father->lock_for_dyn[target_rank][index]));

      target_remain = father->chunk_info_for_dyn[target_rank][index].remain ;

      if ( target_remain == -1 ) { 
	/* If this thread has not already reached this loop, fill
	   some info and then steal the first chunk */
	int nb_chunks ;
	
	nb_chunks = 
	  __mpcomp_get_static_nb_chunks_per_rank( target_rank,
	      num_threads, self->loop_lb,
	      self->loop_b, self->loop_incr,
	      self->loop_chunk_size);

	if ( nb_chunks == 0 ) {
	  father->chunk_info_for_dyn[target_rank][index].remain = 0 ;
	} else {
	  father->chunk_info_for_dyn[target_rank][index].remain = nb_chunks - 1 ;
	}
	father->chunk_info_for_dyn[target_rank][index].total = nb_chunks ;

	sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][index]));

	if ( nb_chunks == 0 ) {
	  continue ;
	}

	__mpcomp_get_specific_chunk_per_rank (target_rank,
	    num_threads,
	    self->loop_lb,
	    self->loop_b,
	    self->loop_incr,
	    self->loop_chunk_size,
	    0, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: "
       "(FIRST) steal next chunk (out of %d) from rank %d: %d -> %d",
       rank, nb_chunks, target_rank, *from, *to);

	return 1 ;

      }

      if ( target_remain == 0 ) { 
	/* If this thread has already finish the loop 'index', try the next
	 * thread */
	sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][index]));
	continue ;
      }

      /* Otherwise, the thread 'target_rank' is currently working on this loop
       * and is not done. Therefore, steal a chunk */


      int target_total ;
      
      target_total = father->chunk_info_for_dyn[target_rank][index].total ;
      father->chunk_info_for_dyn[target_rank][index].remain = target_remain - 1 ;

      sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][index]));

      int target_chunk_id ;

      target_chunk_id = target_total - target_remain ;

      __mpcomp_get_specific_chunk_per_rank (target_rank,
					    num_threads,
					    self->loop_lb,
					    self->loop_b,
					    self->loop_incr,
					    self->loop_chunk_size,
					    target_chunk_id, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: "
       "(MIDDLE) steal next chunk %d from rank %d: %d -> %d",
       rank, target_chunk_id, target_rank, *from, *to);

      return 1 ;
    }

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: "
       "Nothing found to steal" );

    /* Nothing found to steal */
    return 0 ;

  }


  int total = father->chunk_info_for_dyn[rank][index].total ;
  father->chunk_info_for_dyn[rank][index].remain = remain - 1 ;

  sctk_spinlock_unlock (&(father->lock_for_dyn[rank][index]));

  int chunk_id = total - remain ;

    __mpcomp_get_specific_chunk_per_rank (rank,
	num_threads,
	self->loop_lb, self->loop_b, self->loop_incr,
	self->loop_chunk_size, chunk_id, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: next chunk %d (out of %d) %d -> %d",
       rank, chunk_id, total, *from, *to);

    return 1;

}

void
__mpcomp_dynamic_loop_end ()
{

  /* Generate a barrier (this barrier will re-initialized values related to
     in-flight dynamic for loops */
  __mpcomp_barrier_for_dyn();
}

void
__old_mpcomp_dynamic_loop_end_nowait ()
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;
  int nb_exited_threads;
  int i ;

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
     this team has only 1 thread, no need to handle it */
  if (num_threads == 1)
    {
      return;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index = self->private_current_for_dyn;

    sctk_nodebug( "__mpcomp_dynamic_loop_end_nowait[%d]: Exiting from index %d"
	, rank, index ) ;


  /* Atomically update the number of exited threads */
  sctk_spinlock_lock (&(father->lock_exited_for_dyn[index]));

  nb_exited_threads = father->nb_threads_exited_for_dyn[index] ;
  nb_exited_threads++ ;
  father->nb_threads_exited_for_dyn[index] = nb_exited_threads ;

  sctk_spinlock_unlock (&(father->lock_exited_for_dyn[index]));

  /* If I was the last one (no data race because noone else will exit) */
  if ( nb_exited_threads == num_threads ) {
    //int previous_index = (index-1+MPCOMP_MAX_ALIVE_FOR_DYN+1)%(MPCOMP_MAX_ALIVE_FOR_DYN+1) ;

    sctk_nodebug( "__mpcomp_dynamic_loop_end_nowait[%d]: Reseting dynamic info for index %d"
	, rank, index ) ;

    /* TODO what if some threads are already waiting on the STOP symbol? */

    /* Reset the number of exited threads for this workshare */
    father->nb_threads_exited_for_dyn[index] = 0 ;

    /* Update the STOP symbol for the current workshare and the previous one */
    sctk_spinlock_lock (&(father->lock_stop_for_dyn));
    if ( father->stop_value_for_dyn == MPCOMP_NOWAIT_STOP_SYMBOL ) {
    // father->stop_value_for_dyn = MPCOMP_NOWAIT_STOP_SYMBOL ;

    father->stop_index_for_dyn = index ;
    }

    sctk_spinlock_unlock (&(father->lock_stop_for_dyn));

    /* Re-initialize the chunk info for the current workshare
       No data race because the 'index' workshare has a STOP symbol 
     */
    for ( i = 0 ; i < num_threads ; i++ ) {
      father->chunk_info_for_dyn[i][index].remain = -1 ;
    }
  }

}

/* TODO temp barrier to test faster overloading */
void
__old_mpcomp_barrier_for_dyn (void)
{
  mpcomp_thread_info_t *info;

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);

  /* Re-initialize local values related to for loop w/ dynamic schedule */
  __mpcomp_reset_dynamic_thread_info( info ) ;

  /* Block only if I'm not the only thread in the team */
  if (info->num_threads > 1)
    {

      sctk_microthread_vp_t *my_vp;
      long micro_vp_barrier;
      long micro_vp_barrier_done;
      int num_omp_threads_micro_vp;

      /* Grab the microVP */
      my_vp = &(info->task->__list[info->vp]);

      /* Get the total number of OpenMP threads on this microVP */
      num_omp_threads_micro_vp = my_vp->to_do_list;

      /* Is there only 1 OpenMP thread on this microVP? */
      if (num_omp_threads_micro_vp == 1)
	{
	  mpcomp_thread_info_t *father;
	  long barrier_done_init;
	  long barrier;

	  /* Grab the father region */
	  father = info->father;

	  /* Update info on the barrier (father region) */
	  sctk_spinlock_lock (&(father->lock2));
	  barrier_done_init = father->barrier_done;
	  barrier = father->barrier + 1;
	  father->barrier = barrier;

	  /* If I am the last thread of the team to enter this barrier */
	  if (barrier == info->num_threads)
	    {
	      /* Reset information about dynamic-scheduled loops */
	      __mpcomp_reset_dynamic_team_info( father, info->num_threads) ;

	      father->barrier = 0;
	      father->barrier_done++;

	      sctk_spinlock_unlock (&(father->lock2));
	    }
	  else
	    {
	      sctk_spinlock_unlock (&(father->lock2));

	      /* Wait for the barrier to be done */
	      while (father->barrier_done == barrier_done_init)
		{
		  sctk_thread_yield ();
		}
	    }

	}
      else
	{

	  sctk_nodebug
	    ("__mpcomp_barrier[mVP=%d]: number of threads on this vp: %d",
	     info->vp, num_omp_threads_micro_vp);

	  /* Get the barrier number for this microVP */
	  micro_vp_barrier = my_vp->barrier;
	  micro_vp_barrier_done = my_vp->barrier_done;

	  sctk_nodebug
	    ("__mpcomp_barrier[mVP=%d]: number of threads blocked on this vp: %ld",
	     info->vp, micro_vp_barrier);

	  /* Create a context for the next microthreads on the same VP (only the
	   * first time) */
	  if (micro_vp_barrier == 0)
	    {
	      mpcomp_fork_when_blocked (my_vp, info->step);
	    }

	  /* Increment the barrier counter for this microVP */
	  my_vp->barrier = micro_vp_barrier + 1;

	  if (my_vp->barrier == num_omp_threads_micro_vp)
	    {
	      /* The barrier on this microVP is done, only the barrier between
	         microVPs has to be done */
	      mpcomp_thread_info_t *father;
	      long barrier_done_init;
	      long barrier;

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: barrier done for this microVP",
		 info->vp);

	      /* Grab the father region */
	      father = info->father;

	      sctk_spinlock_lock (&(father->lock2));

	      barrier_done_init = father->barrier_done;
	      barrier = father->barrier + num_omp_threads_micro_vp;
	      father->barrier = barrier;

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: %d threads have entered (out of %d)",
		 info->vp, barrier, info->num_threads);

	      /* If I am the last microVP of the current team to finish this barrier */
	      if (barrier == info->num_threads)
		{

		  /* Reset information about dynamic-scheduled loops */
		  __mpcomp_reset_dynamic_team_info( father, info->num_threads) ;

		  father->barrier = 0;
		  father->barrier_done++;
		  sctk_spinlock_unlock (&(father->lock2));
		}
	      else
		{
		  sctk_spinlock_unlock (&(father->lock2));

		  /* No need to give access to other OpenMP threads because the
		   * intra-microVP barrier is done. Call the main scheduler */
		  while (father->barrier_done == barrier_done_init)
		    {
		      sctk_thread_yield ();
		    }
		}

	      /* Release each microVP internally */
	      my_vp->barrier = 0;
	      my_vp->barrier_done++;

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: waiting on this microVP",
		 info->vp);

	      /* Spin while the barrier is not done */
	      mpcomp_macro_scheduler (my_vp, info->step);
	      while (my_vp->barrier_done == micro_vp_barrier_done)
		{
		  mpcomp_macro_scheduler (my_vp, info->step);
		  if (my_vp->barrier_done == micro_vp_barrier_done)
		    {
		      sctk_thread_yield ();
		    }
		}
	    }

	}

    }

  sctk_nodebug ("__mpcomp_barrier: Leaving");
}
//#endif

void
__mpcomp_start_parallel_dynamic_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      int lb, int b, int incr, int chunk_size)
{
  mpcomp_thread_info_t *current_info;
  int num_threads;
  int n;			/* Number of iterations */

  SCTK_PROFIL_START (__mpcomp_start_parallel_region);

  /* Compute the total number iterations */
  n = (b - lb) / incr;

  /* If this loop contains no iterations then exit */
  if ( n <= 0 ) {
    return ;
  }

  /* Initialize the OpenMP environment (call several times, but really executed
   * once) */
  __mpcomp_init ();

  /* Retrieve the information (microthread structure and current region) */
  /* TODO Use TLS if available */
  current_info = sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (current_info != NULL);

  sctk_nodebug
    ("__mpcomp_start_parallel_dynamic_loop: "
     "Entering w/ loop %d -> %d step %d [cs=%d]",
     lb, b, incr, chunk_size);

  /* Grab the number of threads */
  num_threads = current_info->icvs.nthreads_var;
  if (arg_num_threads > 0 && arg_num_threads <= MPCOMP_MAX_THREADS)
    {
      num_threads = arg_num_threads;
    }

  /* Bypass if the parallel region contains only 1 thread */
  if (num_threads == 1)
    {
      int total_nb_chunks ;
      sctk_nodebug
	("__mpcomp_start_parallel_dynamic_loop: Only 1 thread -> call f");

      current_info->loop_lb = lb ;
      current_info->loop_b = b ;
      current_info->loop_incr = incr ;
      current_info->loop_chunk_size = chunk_size ;

      total_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank( 0,
	  num_threads, lb, b, incr, chunk_size); 
      current_info->chunk_info_for_dyn[0][0].remain = total_nb_chunks ;
      current_info->chunk_info_for_dyn[0][0].total  = total_nb_chunks ;

      func (shared);
      SCTK_PROFIL_END (__mpcomp_start_parallel_region);
      return;
    }

  sctk_nodebug
    ("__mpcomp_start_parallel_dynamic_loop: -> Final num threads = %d",
     num_threads);

  /* Creation of a new microthread structure if the current region is
   * sequential or if nesting is allowed */
  if (current_info->depth == 0 || mpcomp_get_nested ())
    {
      sctk_microthread_t *new_task;
      sctk_microthread_t *current_task;
      int i;
      /*
      int n = num_threads / current_info->icvs.nmicrovps_var;
      int index = num_threads % current_info->icvs.nmicrovps_var;
      int vp;
      */

      SCTK_PROFIL_START (__mpcomp_start_parallel_region__creation);

      sctk_nodebug
	("__mpcomp_start_parallel_dynamic_loop: starting new team at depth %d on %d microVP(s)",
	 current_info->depth, current_info->icvs.nmicrovps_var);

      current_task = current_info->task;

      /* Initialize a new microthread structure if needed */
      if (current_info->depth == 0)
	{
	  new_task = current_task;

	  sctk_nodebug
	    ("__mpcomp_start_parallel_dynamic_loop: reusing the thread_info depth 0");
	}
      else
	{

	  /* If the first child has already been created, get the
	   * corresponding thread-info structure */
	  if (current_info->children[0] != NULL)
	    {
	      sctk_nodebug
		("__mpcomp_start_parallel_dynamic_loop: reusing thread_info new depth");

	      new_task = current_info->children[0]->task;
	      sctk_assert (new_task != NULL);

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_dynamic_loop: allocating thread_info new depth");

	      new_task = sctk_malloc (sizeof (sctk_microthread_t));
	      sctk_assert (new_task != NULL);
	      sctk_microthread_init (current_info->icvs.nmicrovps_var, new_task);
	    }
	}


      /* Fill the microthread structure with these new threads */
      for (i = num_threads - 1; i >= 0; i--)
	{
	  mpcomp_thread_info_t *new_info;
	  int microVP;
	  int val;
	  int res;
	  int total_nb_chunks ;

	  /* Compute the VP this thread will be scheduled on and the behavior of
	   * 'add_task' */
	  if (i < current_info->icvs.nmicrovps_var)
	    {
	      microVP = i;
	      val = MPC_MICROTHREAD_LAST_TASK;
	    }
	  else
	    {
	      microVP = i % (current_info->icvs.nmicrovps_var);
	      val = MPC_MICROTHREAD_NO_TASK_INFO;
	    }

	  /* Get the structure of the i-th children */
	  new_info = current_info->children[i];

	  /* If this is the first time that such a child exists
	     -> allocate memory once and initialize once */
	  if (new_info == NULL)
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_dynamic_loop: "
		 "Child %d is NULL -> allocating thread_info",
		 i);

	      new_info =
		sctk_malloc_on_node (sizeof (mpcomp_thread_info_t), sctk_get_node_from_cpu(microVP));
	      sctk_assert (new_info != NULL);

	      current_info->children[i] = new_info;

	      __mpcomp_init_thread_info (new_info, func, shared, i,
					 num_threads, current_info->icvs,
					 current_info->depth + 1, microVP, 0,
					 current_info, 0, new_task);
	    }
	  else
	    {
	      __mpcomp_reset_thread_info (new_info, func, shared, num_threads,
					  current_info->icvs, 0, 0, microVP);
	    }


	  new_info->task = new_task;

	  /* Update private information about dynamic schedule */
	  new_info->loop_lb = lb ;
	  new_info->loop_b = b ;
	  new_info->loop_incr = incr ;
	  new_info->loop_chunk_size = chunk_size ;

	  /* Update shared information about dynamic schedule */
	  sctk_spinlock_lock (&(current_info->lock_for_dyn[i][0]));
	  total_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank( i,
	      num_threads, lb, b, incr, chunk_size); 
	  current_info->chunk_info_for_dyn[i][0].remain = total_nb_chunks ;
	  current_info->chunk_info_for_dyn[i][0].total  = total_nb_chunks ;
	  sctk_spinlock_unlock (&(current_info->lock_for_dyn[i][0]));

	  sctk_nodebug
	    ("__mpcomp_start_parallel_dynamic_loop: "
	     "Adding op %d on microVP %d", i, microVP);

	  res = sctk_microthread_add_task (__mpcomp_wrapper_op, new_info, microVP,
					   &(new_info->step), new_task, val);
	  sctk_assert (res == 0);


	}

      SCTK_PROFIL_END (__mpcomp_start_parallel_region__creation);

      /* Launch the execution of this microthread structure */
      sctk_microthread_parallel_exec (new_task,
				      MPC_MICROTHREAD_DONT_WAKE_VPS);

      sctk_nodebug
	("__mpcomp_start_parallel_dynamic_loop: Microthread execution done");

      /* Restore the key value (microthread structure & OpenMP info) */
      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);
      sctk_thread_setspecific (sctk_microthread_key, current_task);

      /* Free the memory allocated for the new microthread structure */
      if (current_info->depth != 0)
	{
	  sctk_free (new_task);
	}

      /* TODO free the memory allocated for the OpenMP-thread info 
         maybe not because this is stored in current_info->children[] */


    }
  else
    {
      mpcomp_thread_info_t *new_info;

      sctk_nodebug
	("__mpcomp_start_parallel_dynamic_loop: Serialize a new team at depth %d",
	 current_info->depth);

      /* TODO only flatterned nested supported for now */

      num_threads = 1;


      new_info = current_info->children[0];
      if (new_info == NULL)
	{

	  sctk_nodebug
	    ("__mpcomp_start_parallel_dynamic_loop: Allocating new thread_info");

	  new_info = sctk_malloc (sizeof (mpcomp_thread_info_t));
	  sctk_assert (new_info != NULL);
	  current_info->children[0] = new_info;
	  __mpcomp_init_thread_info (new_info, func, shared,
				     0, 1,
				     current_info->icvs,
				     current_info->depth + 1,
				     current_info->vp, 0,
				     current_info, 0, current_info->task);
	}
      else
	{
	  sctk_nodebug
	    ("__mpcomp_start_parallel_dynamic_loop: Reusing older thread_info");
	  __mpcomp_reset_thread_info (new_info, func, shared, 0,
				      current_info->icvs, 0, 0,
				      current_info->vp);
	}

      __mpcomp_wrapper_op (new_info);

      sctk_nodebug ("__mpcomp_start_parallel_dynamic_loop: after flat nested");

      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);

      sctk_free (new_info);
    }

TODO("can we only reset dynamic-team info?")
  /* Re-initializes team info for this thread */
  __mpcomp_reset_team_info (current_info, num_threads);


  /* Restore the TLS for the main thread */
  sctk_extls = current_info->children[0]->extls;

  SCTK_PROFIL_END (__mpcomp_start_parallel_region);
} /* __mpcomp_start_parallel_dynamic_loop */

int
__mpcomp_dynamic_loop_next_ignore_nowait (int *from, int *to)
{
  mpcomp_thread_info_t *self;
  mpcomp_thread_info_t *father;
  long rank;
  int num_threads;
  int i ;
  int remain ;

  /* Grab the thread info */
  /* TODO Use TLS if available */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive),
     or a team w/ only 1 thread, then the current thread is done 
     (the whole loop has been executed after the 'start' function) */
  if (num_threads == 1)
    {
      *from = self->loop_lb ;
      *to = self->loop_b ;

      remain = self->chunk_info_for_dyn[0][0].remain ;
      if ( remain == 0 ) {
	return 0 ;
      }

      int total = self->chunk_info_for_dyn[0][0].total ;
      self->chunk_info_for_dyn[0][0].remain = remain - 1 ;

      int chunk_id = total - remain ;

      __mpcomp_get_specific_chunk_per_rank (0,
	  num_threads,
	  self->loop_lb, self->loop_b, self->loop_incr,
	  self->loop_chunk_size, chunk_id, from, to);
      
      return 1;
    }

  /* Get the rank of the current thread */
  rank = self->rank;

  /* Get the father info */
  father = self->father;
  sctk_assert (father != NULL);

  sctk_nodebug
    ("__mpcomp_dynamic_loop_next_ignore_nowait[%d]: "
     "Next in loop on microVP=%d",
     rank, self->vp);

  sctk_spinlock_lock (&(father->lock_for_dyn[rank][0]));

  remain = father->chunk_info_for_dyn[rank][0].remain ;

  /* Is there anything to do? */
  if ( remain == 0 ) {
    /* If not... */

    sctk_spinlock_unlock (&(father->lock_for_dyn[rank][0]));

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next_ignore_nowait[%d]: "
       "Nothing left, try stealing...",
       rank );

    for ( i = 1 ; i < num_threads ; i++ ) {
      int target_rank = (i + rank)%num_threads ;
      int target_remain ;


      /* Trying to steal work from thread 'target_rank' */

      sctk_spinlock_lock (&(father->lock_for_dyn[target_rank][0]));

      target_remain = father->chunk_info_for_dyn[target_rank][0].remain ;

      if ( target_remain == -1 ) { 
	/* If this thread has not already reached this loop, fill
	   some info and then steal the first chunk */
	int nb_chunks ;
	
	nb_chunks = 
	  __mpcomp_get_static_nb_chunks_per_rank( target_rank,
	      num_threads, self->loop_lb,
	      self->loop_b, self->loop_incr,
	      self->loop_chunk_size);

	if ( nb_chunks == 0 ) {
	  father->chunk_info_for_dyn[target_rank][0].remain = 0 ;
	} else {
	  father->chunk_info_for_dyn[target_rank][0].remain = nb_chunks - 1 ;
	}
	father->chunk_info_for_dyn[target_rank][0].total = nb_chunks ;

	sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][0]));

	if ( nb_chunks == 0 ) {
	  continue ;
	}

	__mpcomp_get_specific_chunk_per_rank (target_rank,
	    num_threads,
	    self->loop_lb,
	    self->loop_b,
	    self->loop_incr,
	    self->loop_chunk_size,
	    0, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next_ignore_nowait[%d]: "
       "(FIRST) stolen next chunk (out of %d) from rank %d: %d -> %d",
       rank, nb_chunks, target_rank, *from, *to);

	return 1 ;

      }

      if ( target_remain == 0 ) { 
	/* If this thread has already finish the loop 'index', try the next
	 * thread */
	sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][0]));
	continue ;
      }

      /* Otherwise, the thread 'target_rank' is currently working on this loop
       * and is not done. Therefore, steal a chunk */


      int target_total ;
      
      target_total = father->chunk_info_for_dyn[target_rank][0].total ;
      father->chunk_info_for_dyn[target_rank][0].remain = target_remain - 1 ;

      sctk_spinlock_unlock (&(father->lock_for_dyn[target_rank][0]));

      int target_chunk_id ;

      target_chunk_id = target_total - target_remain ;

      __mpcomp_get_specific_chunk_per_rank (target_rank,
					    num_threads,
					    self->loop_lb,
					    self->loop_b,
					    self->loop_incr,
					    self->loop_chunk_size,
					    target_chunk_id, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next_ignore_nowait[%d]: "
       "(MIDDLE) stolen next chunk %d from rank %d: %d -> %d",
       rank, target_chunk_id, target_rank, *from, *to);

      return 1 ;
    }

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next_ignore_nowait[%d]: "
       "Nothing found to steal" );

    /* Nothing found to steal */
    return 0 ;

  }


  int total = father->chunk_info_for_dyn[rank][0].total ;
  father->chunk_info_for_dyn[rank][0].remain = remain - 1 ;

  sctk_spinlock_unlock (&(father->lock_for_dyn[rank][0]));

  int chunk_id = total - remain ;

    __mpcomp_get_specific_chunk_per_rank (rank,
	num_threads,
	self->loop_lb, self->loop_b, self->loop_incr,
	self->loop_chunk_size, chunk_id, from, to);

    sctk_nodebug
      ("__mpcomp_dynamic_loop_next[%d]: next chunk %d (out of %d) %d -> %d",
       rank, chunk_id, total, *from, *to);

    return 1;

}

/****
  *
  * ORDERED VERSION 
  *
  *
  *****/

int
__mpcomp_ordered_dynamic_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to)
{
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_dynamic_loop_begin( lb, b, incr, chunk_size, from, to ) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  info->current_ordered_iteration = *from ;

  return res ;
}

int
__mpcomp_ordered_dynamic_loop_next(int *from, int *to)
{
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_dynamic_loop_next( from, to ) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  info->current_ordered_iteration = *from ;

  return res ;
}

void
__mpcomp_ordered_dynamic_loop_end()
{
  __mpcomp_dynamic_loop_end() ;
}

void
__mpcomp_ordered_dynamic_loop_end_nowait()
{
  __mpcomp_dynamic_loop_end() ;
}
#endif
