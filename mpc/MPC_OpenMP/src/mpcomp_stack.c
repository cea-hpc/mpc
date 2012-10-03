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


mpcomp_stack_t * __mpcomp_create_stack(int max_elements)
{
     mpcomp_stack_t *s;

     s = (mpcomp_stack_t *) malloc(sizeof(mpcomp_stack_t));
     s->elements = (mpcomp_node_t **) malloc(max_elements * sizeof(mpcomp_node_t *));
     sctk_assert(s->elements != NULL);
     s->max_elements = max_elements;
     s->n_elements = 0; 

     return s;
}

int __mpcomp_is_stack_empty(mpcomp_stack_t *s)
{
     return (s->n_elements == 0);
}

void __mpcomp_push(mpcomp_stack_t *s, mpcomp_node_t *n)
{
     if (s->n_elements == s->max_elements)
	  exit(1);   /* Error */

     s->elements[s->n_elements] = n;
     s->n_elements++;
}

mpcomp_node_t * __mpcomp_pop(mpcomp_stack_t *s)
{
     mpcomp_node_t *n;

     if (s->n_elements == 0)
	  return NULL;
     
     n = s->elements[s->n_elements - 1];
     s->n_elements--;

     return n;
}

void __mpcomp_free_stack(mpcomp_stack_t *s)
{
     free(s->elements);
}
