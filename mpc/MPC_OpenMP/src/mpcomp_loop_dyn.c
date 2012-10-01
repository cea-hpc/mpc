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
#include "mpcomp_internal.h"
#include <sctk_debug.h>

#if 0
/************** BEGIN HEADER ****************/

union mpcomp_node_leaf {
    mpcomp_node * n ;
    mpcomp_mvp * mvp ;
} ;

typedef union mpcomp_node_leaf mpcomp_node_leaf;

struct mpcomp_stack_node_leaf {
  mpcomp_node_leaf * elements ;
  children_t * child_type ;
  int max_elements ;
  int n_elements ; /* corresponds to the head of the stack */
} ;

typedef struct mpcomp_stack_node_leaf mpcomp_stack_node_leaf ;
#if 0
mpcomp_stack_node_leaf * __mpcomp_create_stack_node_leaf( int max_elements ) ;
int __mpcomp_is_stack_node_leaf_empty( mpcomp_stack_node_leaf * s ) ;
void __mpcomp_push_node( mpcomp_stack_node_leaf * s, mpcomp_node * n ) ;
void __mpcomp_push_leaf( mpcomp_stack_node_leaf * s, mpcomp_mvp * n ) ;
mpcomp_node_leaf * __mpcomp_pop_node_leaf( mpcomp_stack_node_leaf * s ) ;
void __mpcomp_free_stack_node_leaf( mpcomp_stack_node_leaf * s ) ;
#endif

#endif
/************** END HEADER ****************/

/************** BEGIN STACK ****************/

mpcomp_stack_node_leaf * __mpcomp_create_stack_node_leaf( int max_elements ) 
{
 int i;
  mpcomp_stack_node_leaf * s ;
  s = (mpcomp_stack_node_leaf *)sctk_malloc( sizeof( mpcomp_stack_node_leaf) ) ;

  sctk_nodebug("__mpcomp_create_stack_node_leaf: max_elements=%d", max_elements);

  //s->elements = (mpcomp_node_leaf **) malloc( max_elements * sizeof( mpcomp_node_leaf * ) ) ;
  s->elements = (mpcomp_elem_stack **)sctk_malloc(max_elements * sizeof(mpcomp_elem_stack *));
  //memset(s->elements, 0, max_elements*sizeof(mpcomp_elem_stack *)); 
 
  sctk_assert( s->elements != NULL ) ;
  //sctk_assert( s->elements[0] != NULL);
  s->max_elements = max_elements ;
  s->n_elements = 0 ;

  for(i=0;i<max_elements;i++) {
    sctk_nodebug("__mpcomp_create_stack_node_leaf: i=%d", i);
    s->elements[i] = (mpcomp_elem_stack *)sctk_malloc(sizeof(mpcomp_elem_stack));
    //sctk_assert(s->elements[i] != NULL);
    //s->elements[i]->node_leaf = (mpcomp_node_leaf *)sctk_malloc(sizeof(mpcomp_node_leaf));
  }
  
  sctk_nodebug("__mpcomp_create_stack_node_leaf: s->max_elements=%d", s->max_elements);
  return s ;

}

int __mpcomp_is_stack_node_leaf_empty( mpcomp_stack_node_leaf * s ) 
{
  sctk_nodebug("__mpcomp_is_stack_node_leaf_empty: s->n_elements=%d", s->n_elements);

  return ( s->n_elements == 0);
}

void __mpcomp_push_node( mpcomp_stack_node_leaf * s, mpcomp_node * n ) 
{
  if(s->n_elements == s->max_elements) {
    sctk_nodebug("__mpcomp_push_node: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);
    /* Error */
    exit(1);
  }

  sctk_assert( s->elements != NULL);
  sctk_assert( s->elements[s->n_elements] != NULL);
  //sctk_assert( s->elements[s->n_elements]->elem.node != NULL);

  //s->elements[0]->elem.node = (mpcomp_node *)sctk_malloc(sizeof(mpcomp_node));
  s->elements[s->n_elements]->elem.node = (mpcomp_node *)sctk_malloc(sizeof(mpcomp_node));
  sctk_assert(s->elements[s->n_elements]->elem.node != NULL);
  //s->elements[s->n_elements]->elem.node = (mpcomp_node *) sctk_malloc_on_node(sizeof(mpcomp_node),0);
  //sctk_assert(s->elements[s->n_elements]->node_leaf != NULL);    

  s->elements[s->n_elements]->elem.node = n;
  //s->elements[0]->elem.node = n;
  //s->elements[s->n_elements]->node_leaf->n = n;
  s->elements[s->n_elements]->node_or_leaf = 1;
  //s->elements[0]->node_or_leaf = 1;
  s->n_elements++;
}

void __mpcomp_push_leaf( mpcomp_stack_node_leaf * s, mpcomp_mvp * l ) 
{
  if(s->n_elements == s->max_elements) {
    sctk_nodebug("__mpcomp_push_leaf: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);    
    /* Error */
    exit(1);
  }
  
  sctk_assert( s->elements[s->n_elements] != NULL);

  s->elements[s->n_elements]->elem.leaf = (mpcomp_mvp *)sctk_malloc(sizeof(mpcomp_mvp));

  s->elements[s->n_elements]->elem.leaf = l;
  //s->elements[s->n_elements]->node_leaf->mvp = l;
  s->elements[s->n_elements]->node_or_leaf = 2;  
  s->n_elements++;
}

mpcomp_elem_stack * __mpcomp_pop_elem_stack( mpcomp_stack_node_leaf * s ) 
{
  //mpcomp_node_leaf * n ;
  mpcomp_elem_stack * e;

  if ( s->n_elements == 0 ) {
    return NULL ;
  }

  e = s->elements[ s->n_elements - 1 ];
  s->n_elements-- ;

  return e ;
}

void __mpcomp_free_stack_node_leaf( mpcomp_stack_node_leaf * s ) 
{
 free(s->elements);
}
 

/************** END STACK ****************/

#if 0
void __mpcomp_dynamic_initialize_stealing() {
/* Initialize the DFS:
  Push the root
  Call the DFS step
  Return
*/
}

int __mpcomp_dynamic_steal( int * from, int * to ) {
/*
Call the DFS step -> mvp m
done = false
while (!done)
  Try steal m (check remain and decrease it)
  if ok
    done = true
    Fill from and to
    return 1
  else
    call DFS step -> mvp m
    if ( m == NULL ) done = true
return 0
*/
}
//#endif

int __mpcomp_dynamic_steal_index()
{
 /* Tree shape */
 T[DEPTH]
 T[i] = degree[i]
 
 /* Leaf iterator */
 L[DEPTH]
 L[i] = 0

 /* Retenue : index of start mVP */
 R[DEPTH] 
 R[i] = index

 /* mVP */
 M[DEPTH]
 M = L+R

}

#endif


/* int __mpcomp_dynamic_steal2(int *from, int *to) */
int __mpcomp_dynamic_steal(int *from, int *to)
{

 mpcomp_thread *t;
 mpcomp_thread *target_t;
 mpcomp_mvp *mvp, *current_mvp;
 mpcomp_team_info *team;
 mpcomp_stack_node_leaf* s;
 mpcomp_node *n;
 mpcomp_node *root;
 mpcomp_node *node_root;
 //mpcomp_node_leaf *e;
 mpcomp_elem_stack *e;
 mpcomp_mvp *l;
 
 int i;
 int index;
 int depth;
 int tree_rank;
 int target_rank;
 int target_loop_index;
 int start_rank;
 int r;
 int real_r;
 int total;

 t = (mpcomp_thread *)sctk_openmp_thread_tls;
 sctk_assert(t != NULL);

 sctk_debug("__mpcomp_dynamic_steal2 rank=%d: begin", t->rank);

 /* Grab mvp from thread */
 mvp = t->mvp;

 /* Grab team infos from thread */
 team = t->team;

 /* Grab root from mvp */
 root = mvp->root;

//#if 0
 if(t->stolen_mvp != NULL) {

  sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: visit stolen mvp rank=%d", t->rank, t->stolen_mvp->rank); 

   current_mvp = t->stolen_mvp;
   
   target_t = &(current_mvp->threads[0]);
   target_loop_index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
   
   /* Grab remaining chunks */ 
   r = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain) );
  
   if(r > 0) {

    real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);
    
    while(real_r > 0 && real_r != r) {
      //sctk_nodebug("test1");
      r = real_r;
      real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);
    }

    if(real_r > 0) {
      //t->stolen_chunk_id = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r;
      sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: get a chunk from previously visited mvp rank=%d", t->rank, t->stolen_mvp->rank);
//#if 0      
      __mpcomp_get_specific_chunk_per_rank( target_t->rank, target_t->num_threads,
	  target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size, 
	  sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r, 
	  from, to ) ;  
//#endif
      return 1;
    }
   }
 }
 else {
   tree_rank = mvp->tree_rank[0];
   node_root = root->children.node[tree_rank];
   t->tree_stack = __mpcomp_create_stack_node_leaf(64);
   //s = __mpcomp_create_stack_node_leaf(1024);
   __mpcomp_push_node(t->tree_stack, node_root);
 }
//#endif

 //tree_rank = mvp->tree_rank[0];
 //node_root = root->children.node[tree_rank];

 //t->tree_stack = __mpcomp_create_stack_node_leaf(128);
 //__mpcomp_push_node(t->tree_stack, root);

 while(!__mpcomp_is_stack_node_leaf_empty(t->tree_stack)) {
 
   sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: pop an element: n_elements=%d", t->rank, t->tree_stack->n_elements);
 
   e = __mpcomp_pop_elem_stack(t->tree_stack);
   //depth = n->depth;
 
   //sctk_assert(mvp != NULL);
 
   //tree_rank = mvp->tree_rank[depth];

//#if 0
   if(t->stolen_mvp != NULL) {
     start_rank = t->stolen_mvp->rank;
     t->stolen_mvp = NULL;
   }
   else {
     start_rank = tree_rank;
   }
   //#endif

   if(e->node_or_leaf == 1) {
    
     //n = (mpcomp_node *) e;
     n = e->elem.node;

     sctk_assert(n != NULL);
     //depth = e->n->depth;
     depth = n->depth;
     sctk_assert( mvp != NULL);
     tree_rank = mvp->tree_rank[depth];

     sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: tree_rank=%d, depth=%d", t->rank, tree_rank, depth);

     //for(i=0;i<n->nb_children;i++) {
       //index = (tree_rank-i+n->nb_children) % (n->nb_children);
 
       if(n->child_type == CHILDREN_NODE) { 

         for(i=0;i<n->nb_children;i++) {
           index = (tree_rank-i+n->nb_children) % (n->nb_children);
           sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: push node, index=%d", t->rank, index);
           __mpcomp_push_node(t->tree_stack, n->children.node[index]); 
         }
       }
       else {
         
         for(i=0;i<n->nb_children;i++) {
          index = (start_rank-i+n->nb_children) % (n->nb_children);
          sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: push leaf, index=%d, leaf rank=%d", t->rank, index, n->children.leaf[index]->rank);         
          __mpcomp_push_leaf(t->tree_stack, n->children.leaf[index]);
         }
       }
 
     //}
   }
   else if(e->node_or_leaf == 2) {

     //l = (mpcomp_mvp *)e;
     l = e->elem.leaf;

     sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: reach a leaf rank=%d", t->rank, l->rank);

     //t->stolen_mvp = NULL;

     if(l->rank == mvp->rank) { 
      sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: leaf - continue", t->rank);
      continue;  
     }

     target_t = &(l->threads[0]);
     target_rank = target_t->rank;
     target_loop_index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
     r = sctk_atomics_load_int(&(target_t->for_dyn_chunk_info[target_loop_index].remain));

#if 0 
     if(r == -1) {
        sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: r=-1", t->rank);
       
        total = __mpcomp_get_static_nb_chunks_per_rank(target_t->rank, t->num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size);

        /* Store it w/out verification */
        sctk_atomics_store_int( &(target_t->for_dyn_chunk_info[target_loop_index].total), total);
   
        r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), -1, total-1);

        if(r == -1) {
            __mpcomp_get_specific_chunk_per_rank(t->rank, t->num_threads, t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, 0, from, to);
 
            return 1;
        }
     }
 #endif
      if(r > 0) {
        sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: r > 0", t->rank);

        //int real_r;
        real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);

        while( real_r > 0 && real_r != r) {
           sctk_nodebug("test2");
           r = real_r;
           real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);
        } 

        if( real_r > 0) {
           //t->stolen_chunk_id = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r;
           //total = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[index].total));
           //sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: chunk found: target mvp rank=%d, node depth=%d, rank=%d", t->rank, target_t->mvp->rank, n->depth, n->rank);
           t->stolen_mvp = target_t->mvp;
           //t->tree_stack = stack; /* Store stack state */

//#if 0
           __mpcomp_get_specific_chunk_per_rank( target_t->rank, target_t->num_threads,
	        target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size, 
                sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r, 
                from, to ) ; 
//#endif

           return 1;
         }        
          
      }
   }
   
//#endif
 }

 //t->stolen_mvp = NULL;
 //t->stolen_chunk_id = -1;

 sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: return 0", t->rank);

//#endif

 return 0;

}


//#if 0
int __mpcomp_dynamic_steal(int *from, int *to)
{
#if 0
  mpcomp_thread *t;
  mpcomp_thread *target_t;
  mpcomp_mvp *mvp, *current_mvp;
  mpcomp_team_info *team;
  mpcomp_stack *stack;
  mpcomp_node *n;
  mpcomp_node *root;
  mpcomp_node *root_node;

  int i;
  int index;
  int depth;
  int tree_rank;
  int target_rank;
  int target_loop_index;
  int start_rank;
  int r;
  int real_r;
  int total;
  int nb_chunks_empty_children;
  int chunks_avail;

  t = (mpcomp_thread *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_debug("__mpcomp_dynamic_steal rank=%d: begin", t->rank);

  /* Grab mvp from thread */
  mvp = t->mvp;

  /* Grab team infos from thread */
  team = t->team;

  /* Grab root from mvp */
  root = mvp->root;

  sctk_assert(root != NULL);

  tree_rank = mvp->tree_rank[0];

  sctk_nodebug("__mpcomp_dynamic_steal rank=%d: node tree_rank=%d", t->rank, tree_rank);

  root_node = root->children.node[tree_rank];

  sctk_assert(root_node != NULL);

  //sctk_nodebug("__mpcomp_dynamic_steal rank=%d: root depth=%d rank=%d", t->rank, root->depth, root->rank);
  //#if 0
  if(t->stolen_mvp != NULL) {

    sctk_nodebug("__mpcomp_dynamic_steal rank=%d: stolen_mvp rank=%d", t->rank, t->stolen_mvp->rank); 

    current_mvp = t->stolen_mvp;
    
    /* Check if there are still chunks to steal on last visited mvp */
    target_t = &(current_mvp->threads[0]);

    target_loop_index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

    /* Grab remaining chunks */
    r = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain) );

     if(r > 0) {
      real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);

      while ( real_r > 0 && real_r != r)  {
       r = real_r; 
       real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);
      }

      if( real_r > 0) {
        sctk_nodebug("__mpcomp_dynamic_steal rank=%d: steal chunk at previously visited mvp: stolen_mvp rank=%d, node depth=%d, rank=%d", t->rank, current_mvp->rank, current_mvp->father->depth, current_mvp->father->rank);
        //sctk_atomics_incr_int( &(team->stats_last_mvp_chunk));
        //t->stolen_chunk_id = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r;

        //#if 0
         __mpcomp_get_specific_chunk_per_rank( target_t->rank, target_t->num_threads,
	  target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size, 
	  sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r, 
	  from, to ) ; 
        //#endif

        return 1;
     }
    }

    n = current_mvp->father;
 
#if 0
    /* Update chunks availability infos */
    nb_chunks_empty_children = sctk_atomics_load_int( &(n->nb_chunks_empty_children));
    chunks_avail = sctk_atomics_load_int( &(n->chunks_avail));
    //nb_chunks_empty_children = sctk_atomics_fetch_and_incr_int( &(n->nb_chunks_empty_children));
    //if(chunks_avail == MPCOMP_CHUNKS_AVAIL) {
      sctk_nodebug("__mpcomp_dynamic_steal rank=%d: check chunks avail=1: nb_chunks_empty_children=%d, real nb chunks empty children=%d, node children=%d", t->rank, nb_chunks_empty_children, n->nb_chunks_empty_children, n->nb_children);

      //sctk_assert(nb_chunks_empty_children <= n->nb_children);

      if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children == n->nb_children)) {
        sctk_atomics_store_int(&(n->chunks_avail), MPCOMP_CHUNKS_NOT_AVAIL);	
      }
      else if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children < n->nb_children)) {
        sctk_atomics_incr_int( &(n->nb_chunks_empty_children));
      
      }
    //}
#endif

    //stack = t->tree_stack;
    //n = current_mvp->father;
    //__mpcomp_push(stack, n); 
    __mpcomp_push(&(t->tree_stack), &n);
//#endif
  }
  else {
   current_mvp = mvp;
   //stack = __mpcomp_create_stack(1024);
   //__mpcomp_push(stack, root);
   t->tree_stack = __mpcomp_create_stack(8);
   //__mpcomp_push(t->tree_stack, root);
   __mpcomp_push(t->tree_stack, root_node);
  }

  while( !__mpcomp_is_stack_empty(t->tree_stack)) {
   
   n = __mpcomp_pop(t->tree_stack);
   depth = n->depth;
   //current_mvp = mvp;
   tree_rank = current_mvp->tree_rank[depth];

   //#if 0
   if(t->stolen_mvp != NULL) {
    start_rank = t->stolen_mvp->rank;
    t->stolen_mvp = NULL;    
   }
   else {
    t->start_mvp_index = tree_rank;
    start_rank = tree_rank;
   }
   //#endif

   //start_rank = tree_rank;

   sctk_assert(n->nb_children != 0);
   
   //chunks_avail = sctk_atomics_load_int( &(n->chunks_avail));

   sctk_nodebug("__mpcomp_dynamic_steal rank=%d: chunk_avail=%d, real chunks_avail=%d", t->rank, chunks_avail, n->chunks_avail);

   //if(chunks_avail == MPCOMP_CHUNKS_AVAIL) {

   for(i=1;i<=n->nb_children;i++) {
     index = (start_rank-i+n->nb_children) % (n->nb_children);
     
     if(n->child_type == CHILDREN_NODE) {
      sctk_nodebug("__mpcomp_dynamic_steal rank=%d: push node (depth=%d, rank=%d)", t->rank, n->children.node[index]->depth, n->children.node[index]->rank);
      __mpcomp_push(t->tree_stack, n->children.node[index]);
     }
     else {
        sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: leaves reached, node depth=%d, rank=%d", t->rank, n->depth, n->rank);
        if(n->children.leaf[index]->rank == current_mvp->rank) {
          continue;
        }

        /* Check if we didn't check all mvps in the subtree */
        if(i == t->start_mvp_index) 
         break;

        target_t = &(n->children.leaf[index]->threads[0]);
        target_rank = target_t->rank;
        target_loop_index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
        r = sctk_atomics_load_int(&(target_t->for_dyn_chunk_info[target_loop_index].remain));
#if 0 
        if(r == -1) {
//#if 0        
          total = __mpcomp_get_static_nb_chunks_per_rank(target_t->rank, target_t->num_threads, target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size);

          /* Store it w/out verification */
          sctk_atomics_store_int( &(target_t->for_dyn_chunk_info[target_loop_index].total), total);
//#if 0  
          r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), -1, total-1);
//#if 0
          if(r == -1) {
            __mpcomp_get_specific_chunk_per_rank(target_t->rank, target_t->num_threads, target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size, 0, from, to);
 
            return 1;
          }
//#endif           
        }
#endif
 
        if(r > 0) {
          //int real_r;
          real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);

          while( real_r > 0 && real_r != r) {
             r = real_r;
             real_r = sctk_atomics_cas_int( &(target_t->for_dyn_chunk_info[target_loop_index].remain), r, r-1);
          } 

          if( real_r > 0) {
             //t->stolen_chunk_id = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r;
             total = sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[index].total));
             sctk_nodebug("__mpcomp_dynamic_steal2 rank=%d: chunk found: target mvp rank=%d, node depth=%d, rank=%d", t->rank, target_t->mvp->rank, n->depth, n->rank);
             t->stolen_mvp = target_t->mvp;
             //t->tree_stack = stack; /* Store stack state */

             //#if 0
             __mpcomp_get_specific_chunk_per_rank( target_t->rank, target_t->num_threads,
	        target_t->loop_lb, target_t->loop_b, target_t->loop_incr, target_t->loop_chunk_size, 
                sctk_atomics_load_int( &(target_t->for_dyn_chunk_info[target_loop_index].total)) - r, 
                from, to ) ; 
             //#endif

             return 1;
          }        
          
        }

        #if 0 
        /* Update chunks availability infos */
        nb_chunks_empty_children = sctk_atomics_load_int( &(n->nb_chunks_empty_children));
        //nb_chunks_empty_children = sctk_atomics_fetch_and_incr_int (&(n->nb_chunks_empty_children));
        chunks_avail = sctk_atomics_load_int( &(n->chunks_avail));

        sctk_nodebug("__mpcomp_dynamic_steal rank=%d: node depth=%d, rank=%d, chunks_avail=%d, nb_children=%d, nb_chunks_empty_children=%d", t->rank, n->depth, n->rank, n->chunks_avail, n->nb_children, nb_chunks_empty_children); 
        //sctk_assert(nb_chunks_empty_children <= n->nb_children);

        if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children == n->nb_children)) {
          sctk_nodebug("__mpcomp_dynamic_steal rank=%d: in node depth=%d, rank=%d, nb_children=%d, %d children are chunks empty: FLAG NOT AVAIL", t->rank, n->depth, n->rank,  n->nb_children, nb_chunks_empty_children);
          sctk_atomics_store_int( &(n->chunks_avail), MPCOMP_CHUNKS_NOT_AVAIL); 
          break;
        }
        else if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children < n->nb_children)) {
          sctk_nodebug("__mpcomp_dynamic_steal rank=%d: in node depth=%d, rank=%d, nb_chunks_empty_children=%d, increment nb chunks_empty_children", t->rank, n->depth, n->rank, nb_chunks_empty_children);
          sctk_atomics_incr_int( &(n->nb_chunks_empty_children)); 
        }
        else { 
          sctk_nodebug("__mpcomp_dynamic_steal rank=%d: chunks_avail = %d: break", t->rank, chunks_avail);
          break; 
        }

        //}
        #endif

      }
     }
#if 0
    }
    else {
      
      sctk_nodebug("__mpcomp_dynamic_steal rank=%d: node depth=%d rank=%d: no chunks avail", t->rank, n->depth, n->rank);
      #if 0
      /* No chunks available */
      if(n->father != NULL) {
        nb_chunks_empty_children = sctk_atomics_load_int( &(n->father->nb_chunks_empty_children));
        chunks_avail = sctk_atomics_load_int( &(n->father->chunks_avail));
        //nb_chunks_empty_children = sctk_atomics_fetch_and_incr_int( &(n->father->nb_chunks_empty_children));

        if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children == n->father->nb_children)) {
          sctk_atomics_store_int( &(n->father->chunks_avail), MPCOMP_CHUNKS_NOT_AVAIL);
        }
        else if((chunks_avail == MPCOMP_CHUNKS_AVAIL) && (nb_chunks_empty_children < n->father->nb_children)) {
          sctk_atomics_incr_int( &(n->father->nb_chunks_empty_children));
        }
            
      }
      #endif      
    }
#endif
  }

 t->stolen_mvp = NULL;

 return 0;
#endif
}
//#endif


int __mpcomp_dynamic_loop_begin (int lb, int b, int incr,
			     int chunk_size, int *from, int *to)
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int nb_threads_exited ; 
  int r ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  sctk_debug("[__mpcomp_dynamic_loop_begin] start");   

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  if (num_threads == 1) {
    sctk_nodebug("__mpcomp_dynamic_loop_begin rank=%d: num_threads=%d", t->rank, num_threads);
    *from = lb ;
    *to = b ;
    return 1;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);


  /* Reinitialize the previous loop */
  index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
  sctk_atomics_store_int( &(t->for_dyn_chunk_info[index].remain), -1 ) ;
  t->for_dyn_current++ ;

  sctk_nodebug( "__mpcomp_dynamic_loop_begin: Entering loop %d (%d -> %d [%d] cs:%d)",
      t->for_dyn_current, lb, b, incr, chunk_size ) ;

  /* Compute the index of the dynamic for construct */
  index = (index+1) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

  /* Stop if the maximum number of alive loops is reached */
  nb_threads_exited = sctk_atomics_load_int( 
      &(team_info->for_dyn_nb_threads_exited[index].i ) ) ;

  sctk_nodebug( "__mpcomp_dynamic_loop_begin: index %d -> #thread exited %d", index, nb_threads_exited ) ;

  sctk_assert( nb_threads_exited >= 0 || nb_threads_exited == MPCOMP_NOWAIT_STOP_SYMBOL ) ;

  if ( nb_threads_exited == MPCOMP_NOWAIT_STOP_SYMBOL ) {
    while( sctk_atomics_load_int( 
	  &(team_info->for_dyn_nb_threads_exited[index].i ) ) == 
	MPCOMP_NOWAIT_STOP_SYMBOL ) {
      sctk_nodebug("__mpcomp_dynamic_loop_begin rank=%d: STOP, thread yield", t->rank);
      sctk_thread_yield() ;
    }
  }

  /* Fill private info about the loop */
  t->loop_lb = lb ;
  t->loop_b = b ;
  t->loop_incr = incr ;
  t->loop_chunk_size = chunk_size ;

  /* Capture the number of chunks remaining for this thread */
  r = sctk_atomics_load_int( &(t->for_dyn_chunk_info[index].remain) ) ;

  sctk_nodebug("[__mpcomp_dynamic_loop_begin rank=%d] r=%d", t->rank, r); 

  /* If remain is -1, it means that nothing has been initialized for this loop */
  if ( r == -1 ) {
    int total ;
    
    /* Compute the total number of chunks for this thread */
    total = __mpcomp_get_static_nb_chunks_per_rank( 
	t->rank, num_threads, lb, b, incr, chunk_size ) ;

    sctk_nodebug( "__mpcomp_dynamic_loop_begin: total number of chunks %d", total ) ;

    /* Store it w/out verification */
    sctk_atomics_store_int( &(t->for_dyn_chunk_info[index].total), total ) ;

    /* Try to change the number of remaining chunks */
    r = sctk_atomics_cas_int( &(t->for_dyn_chunk_info[index].remain), -1, total-1 ) ;

    /* If the previous value was -1, it means that we successfully replaced the remaining value.
       Therefore, compute and return the first chunk.
       Otherwise, somebody already initialized the chunks, then try to get one */
    if ( r == -1 ) {
      __mpcomp_get_specific_chunk_per_rank( t->rank, num_threads, 
	  lb, b, incr, chunk_size, 0, from, to ) ;

      sctk_nodebug( "__mpcomp_dynamic_loop_begin: Get first chunk %d -> %d", *from, *to ) ;

      sctk_nodebug("[__mpcomp_dynamic_loop_begin rank=%d] r=-1, from=%d, to=%d", t->rank, *from, *to); 

      return 1 ;
    }
  }

  if ( r > 0 ) {
    int real_r ;

    real_r = sctk_atomics_cas_int( &(t->for_dyn_chunk_info[index].remain), r, r-1 ) ;
    while ( real_r > 0 && real_r != r ) {
      r = real_r ;
      real_r = sctk_atomics_cas_int( &(t->for_dyn_chunk_info[index].remain), r, r-1 ) ;
    }

    if ( real_r > 0 ) {
      __mpcomp_get_specific_chunk_per_rank( t->rank, num_threads,
	  lb, b, incr, chunk_size, 
	  sctk_atomics_load_int( &(t->for_dyn_chunk_info[index].total)) - r, 
	  from, to ) ;

      sctk_nodebug( "__mpcomp_dynamic_loop_begin: Get a chunk %d -> %d", *from, *to ) ;

      return 1 ;
    }
  }

  /* If remain is <= 0 it means that there is no chunk left for this loop */

  //* Initialize stealing and try to steal */
//#if 0
  if( __mpcomp_dynamic_steal(from, to)) {
   
    //sctk_nodebug("__mpcomp_dynamic_loop_begin: chunk found: id=%d", t->stolen_chunk_id);
 
    /* A chunk could be stolen */
    #if 0
    __mpcomp_get_specific_chunk_per_rank( t->rank, num_threads,
	  lb, b, incr, chunk_size, 
	  t->stolen_chunk_id, 
	  from, to ) ; 
    #endif

    sctk_debug( "__mpcomp_dynamic_loop_begin: Get a stolen chunk %d -> %d", *from, *to ) ;
    //printf( "__mpcomp_dynamic_loop_begin: Get a stolen chunk %d -> %d", *from, *to ) ;
     
    //sctk_atomics_incr_int(&(team_info->stats_stolen_chunks));
    return 1;
  }
//#endif
  sctk_nodebug("[__mpcomp_dynamic_loop_begin rank=%d] nothing to do: return 0", t->rank); 

  /* Nothing to do and to steal */
  return 0 ;

}

int
__mpcomp_dynamic_loop_next (int *from, int *to)
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int r ;

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  sctk_nodebug("[__mpcomp_dynamic_loop_next rank=%d] start", t->rank);   

  /* If this part is sequential, the whole loop has already been executed */
  if (num_threads == 1) {
    return 0;
  }

  /* Compute the index of the dynamic for construct */
  index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

  /* TODO check what the status is to see if we have to steal */

  r = sctk_atomics_load_int( &(t->for_dyn_chunk_info[index].remain) ) ;
  
//  sctk_nodebug("__mpcomp_dynamic_loop_next rank=%d: r=%d", t->rank, r);

  sctk_nodebug("__mpcomp_dynamic_loop_next rank=%d: index=%d, r=%d", t->rank, index, t->for_dyn_chunk_info[index].remain);

  if ( r > 0 ) {
    int real_r ;

    real_r = sctk_atomics_cas_int( &(t->for_dyn_chunk_info[index].remain), r, r-1 ) ;
    while ( real_r > 0 && real_r != r ) {
      sctk_nodebug("__mpcomp_dynamic_loop_nex rank=%d: r > 0, real_r=%d", t->rank, real_r);
      r = real_r ;
      real_r = sctk_atomics_cas_int( &(t->for_dyn_chunk_info[index].remain), r, r-1 ) ;
    }

    if ( real_r > 0 ) {
      int chunk_id ;

      __mpcomp_get_specific_chunk_per_rank( t->rank, num_threads,
	  t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, 
	  sctk_atomics_load_int( &(t->for_dyn_chunk_info[index].total) ) - r, 
	  from, to ) ;

      sctk_nodebug( "__mpcomp_dynamic_loop_next: Get a chunk %d -> %d", *from, *to ) ;
      //printf( "__mpcomp_dynamic_loop_next: Get a chunk %d -> %d\n", *from, *to ) ;

      return 1 ;
    }
  }

  /* TODO Initialize stealing and try to steal */

//#if 0
  if( __mpcomp_dynamic_steal(from, to)) {
    
    //sctk_nodebug("__mpcomp_dynamic_loop_next: chunk found: id=%d", t->stolen_chunk_id);

    #if 0 
    /* A chunk could be stolen */
    __mpcomp_get_specific_chunk_per_rank( t->rank, num_threads,
	  t->loop_lb, t->loop_b, t->loop_incr, t->loop_chunk_size, 
	  t->stolen_chunk_id, 
	  from, to ) ; 
    #endif
    //sctk_atomics_incr_int(&(team_info->stats_stolen_chunks));

    sctk_debug( "__mpcomp_dynamic_loop_next: Get a stolen chunk %d -> %d", *from, *to ) ;
    //printf( "__mpcomp_dynamic_loop_next: Get a stolen chunk %d -> %d\n", *from, *to ) ;

    return 1;
  }
//#endif

  /* Nothing to do and to steal */
  return 0 ;
}

void
__mpcomp_dynamic_loop_end ()
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int nb_threads_exited ; 

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  if (num_threads == 1) {
    return ;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  /* Reset local stack and flags */
  //if(t->tree_stack != NULL)
  //  __mpcomp_free_stack_node_leaf(t->tree_stack);

  //t->stolen_mvp = NULL;

  /* Compute the index of the dynamic for construct */
  index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

  sctk_nodebug("__mpcomp_dynamic_loop_end rank=%d: begin", t->rank);

  nb_threads_exited = sctk_atomics_fetch_and_incr_int( 
      &(team_info->for_dyn_nb_threads_exited[index].i ) ) ;

  sctk_nodebug( "__mpcomp_dynamic_loop_end: Exiting loop %d: %d -> %d",
      index, nb_threads_exited, nb_threads_exited + 1 ) ;

  sctk_assert( nb_threads_exited >= 0 && nb_threads_exited < num_threads ) ;

  if ( nb_threads_exited == num_threads - 1 ) {
    int previous_index ;


    sctk_atomics_store_int( 
	&(team_info->for_dyn_nb_threads_exited[index].i), 
	MPCOMP_NOWAIT_STOP_SYMBOL ) ;

    previous_index = (index - 1 + MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) % 
      ( MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) ;

  sctk_nodebug( "__mpcomp_dynamic_loop_end: Move STOP symbol %d -> %d", previous_index, index ) ;

    sctk_atomics_store_int( 
	&(team_info->for_dyn_nb_threads_exited[previous_index].i), 
	0 ) ;
  }

  return ;
}

void
__mpcomp_dynamic_loop_end_nowait ()
{
  __mpcomp_dynamic_loop_end() ;
}

/****
  *
  * COMBINED VERSION 
  *
  *
  *****/

void
__mpcomp_start_parallel_dynamic_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      int lb, int b, int incr, int chunk_size)
{
  not_implemented() ;
}

int
__mpcomp_dynamic_loop_next_ignore_nowait (int *from, int *to)
{
  not_implemented() ;
  return 0;
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
  not_implemented() ;
  return -1;
}

int
__mpcomp_ordered_dynamic_loop_next(int *from, int *to)
{
  not_implemented() ;
  return -1;
}

void
__mpcomp_ordered_dynamic_loop_end()
{
  not_implemented() ;
}

void
__mpcomp_ordered_dynamic_loop_end_nowait()
{
  not_implemented() ;
}
