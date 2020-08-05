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
#include "mpcomp_stack.h"
#include "mpcomp_types.h"

mpcomp_stack_node_leaf_t * __mpcomp_create_stack_node_leaf(int max_elements) 
{
     int i;
     mpcomp_stack_node_leaf_t *s;

     s = (mpcomp_stack_node_leaf_t *) sctk_malloc(sizeof(mpcomp_stack_node_leaf_t));

     mpc_common_nodebug("__mpcomp_create_stack_node_leaf: max_elements=%d", max_elements);
     s->elements = (mpcomp_elem_stack_t **) sctk_malloc(max_elements * sizeof(mpcomp_elem_stack_t *));
 
     assert(s->elements != NULL);
     s->max_elements = max_elements;
     s->n_elements = 0;

     for (i=0; i<max_elements; i++) {
	  mpc_common_nodebug("__mpcomp_create_stack_node_leaf: i=%d", i);
	  s->elements[i] = (mpcomp_elem_stack_t *) sctk_malloc(sizeof(mpcomp_elem_stack_t));
	  assert(s->elements[i] != NULL);
     }
  
     mpc_common_nodebug("__mpcomp_create_stack_node_leaf: s->max_elements=%d", s->max_elements);
     return s;
}

int __mpcomp_is_stack_node_leaf_empty(mpcomp_stack_node_leaf_t *s) 
{
     mpc_common_nodebug("__mpcomp_is_stack_node_leaf_empty: s->n_elements=%d", s->n_elements);

     return (s->n_elements == 0);
}

void __mpcomp_push_node( mpcomp_stack_node_leaf_t *s, mpcomp_node_t *n) 
{
     if (s->n_elements == s->max_elements) {
          /* Error */
	  mpc_common_nodebug("__mpcomp_push_node: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);
	  exit(1);
     }

     assert(s->elements != NULL);
     assert(s->elements[s->n_elements] != NULL);

     s->elements[s->n_elements]->elem.node = (mpcomp_node_t *) sctk_malloc(sizeof(mpcomp_node_t));
     assert(s->elements[s->n_elements]->elem.node != NULL);
     //s->elements[s->n_elements]->elem.node = (mpcomp_node_t *) sctk_malloc_on_node(sizeof(mpcomp_node_t),0);

     s->elements[s->n_elements]->elem.node = n;
     s->elements[s->n_elements]->type = MPCOMP_ELEM_STACK_NODE;
     s->n_elements++;
}

void __mpcomp_push_leaf(mpcomp_stack_node_leaf_t *s, mpcomp_mvp_t *l) 
{
     if (s->n_elements == s->max_elements) {
	  /* Error */
	  mpc_common_nodebug("__mpcomp_push_leaf: n_elements=%d max_elements=%d => exit", s->n_elements, s->max_elements);    
	  exit(1);
     }
  
     assert(s->elements[s->n_elements] != NULL);

     s->elements[s->n_elements]->elem.leaf = (mpcomp_mvp_t *) sctk_malloc(sizeof(mpcomp_mvp_t));

     s->elements[s->n_elements]->elem.leaf = l;
     s->elements[s->n_elements]->type = MPCOMP_ELEM_STACK_LEAF;  
     s->n_elements++;
}

mpcomp_elem_stack_t * __mpcomp_pop_elem_stack(mpcomp_stack_node_leaf_t *s)
{
     mpcomp_elem_stack_t *e;

     if (s->n_elements == 0)
	  return NULL;

     e = s->elements[s->n_elements - 1];
     s->n_elements--;

     return e;
}

void __mpcomp_free_stack_node_leaf(mpcomp_stack_node_leaf_t *s) 
{
     free(s->elements);
}
#endif // NO MORE USED
