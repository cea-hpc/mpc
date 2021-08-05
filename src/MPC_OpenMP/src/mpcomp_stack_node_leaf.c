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

#if 0 // NO MORE USED
#include "mpc_common_debug.h"
#include "mpc_omp_stack.h"
#include "mpcomp_types.h"

mpc_omp_stack_node_leaf_t * _mpc_omp_create_stack_node_leaf(int max_elements) 
{
     int i;
     mpc_omp_stack_node_leaf_t *s;

     s = (mpc_omp_stack_node_leaf_t *) sctk_malloc(sizeof(mpc_omp_stack_node_leaf_t));

     mpc_common_nodebug("_mpcomp_create_stack_node_leaf: max_elements=%d", max_elements);
     s->elements = (mpc_omp_elem_stack_t **) sctk_malloc(max_elements * sizeof(mpc_omp_elem_stack_t *));
 
     assert(s->elements != NULL);
     s->max_elements = max_elements;
     s->n_elements = 0;

     for (i=0; i<max_elements; i++) {
	  mpc_common_nodebug("_mpcomp_create_stack_node_leaf: i=%d", i);
	  s->elements[i] = (mpc_omp_elem_stack_t *) sctk_malloc(sizeof(mpc_omp_elem_stack_t));
	  assert(s->elements[i] != NULL);
     }
  
     mpc_common_nodebug("_mpcomp_create_stack_node_leaf: s->max_elements=%d", s->max_elements);
     return s;
}

int _mpc_omp_is_stack_node_leaf_empty(mpc_omp_stack_node_leaf_t *s) 
{
     mpc_common_nodebug("_mpcomp_is_stack_node_leaf_empty: s->n_elements=%d", s->n_elements);

     return (s->n_elements == 0);
}

void _mpc_omp_push_node( mpc_omp_stack_node_leaf_t *s, mpc_omp_node_t *n) 
{
     if (s->n_elements == s->max_elements) {
          /* Error */
	  mpc_common_nodebug("_mpcomp_push_node: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);
	  exit(1);
     }

     assert(s->elements != NULL);
     assert(s->elements[s->n_elements] != NULL);

     s->elements[s->n_elements]->elem.node = (mpc_omp_node_t *) sctk_malloc(sizeof(mpc_omp_node_t));
     assert(s->elements[s->n_elements]->elem.node != NULL);
     //s->elements[s->n_elements]->elem.node = (mpc_omp_node_t *) sctk_malloc_on_node(sizeof(mpc_omp_node_t),0);

     s->elements[s->n_elements]->elem.node = n;
     s->elements[s->n_elements]->type = MPC_OMP_ELEM_STACK_NODE;
     s->n_elements++;
}

void _mpc_omp_push_leaf(mpc_omp_stack_node_leaf_t *s, mpc_omp_mvp_t *l) 
{
     if (s->n_elements == s->max_elements) {
	  /* Error */
	  mpc_common_nodebug("_mpcomp_push_leaf: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);    
	  exit(1);
     }
  
     assert(s->elements[s->n_elements] != NULL);

     s->elements[s->n_elements]->elem.leaf = (mpc_omp_mvp_t *) sctk_malloc(sizeof(mpc_omp_mvp_t));

     s->elements[s->n_elements]->elem.leaf = l;
     s->elements[s->n_elements]->type = MPC_OMP_ELEM_STACK_LEAF;  
     s->n_elements++;
}

mpc_omp_elem_stack_t * _mpc_omp_pop_elem_stack(mpc_omp_stack_node_leaf_t *s)
{
     mpc_omp_elem_stack_t *e;

     if (s->n_elements == 0)
	  return NULL;

     e = s->elements[s->n_elements - 1];
     s->n_elements--;

     return e;
}

void mpc_omp_free_stack_node_leaf(mpc_omp_stack_node_leaf_t *s) 
{
     free(s->elements);
}
#endif // NO MORE USED
