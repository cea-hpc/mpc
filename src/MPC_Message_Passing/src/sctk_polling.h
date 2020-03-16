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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_POLLING
#define SCTK_POLLING

#include "sctk_debug.h"
#include "mpc_common_spinlock.h"
#include <mpc_common_asm.h>
#include <mpc_config_struct.h>

struct sctk_polling_cell
{
	OPA_int_t polling_counter;
	char is_free;
};

void sctk_polling_cell_zero( struct sctk_polling_cell * cell );

struct sctk_polling_tree
{
	struct sctk_polling_cell * cells;
	int * local_task_id_to_cell_lut;
	
	mpc_common_spinlock_t push_lock;
	char root_cell_used;
	
	int cell_count;
	int max_load;
};

static inline void sctk_polling_tree_set_max_load( struct sctk_polling_tree * tree,  int load )
{
	assume( 0 < load );
	tree->max_load = load;
}

void sctk_polling_tree_init_empty( struct sctk_polling_tree * tree );
void sctk_polling_tree_init_once( struct sctk_polling_tree * tree );

void sctk_polling_tree_release( struct sctk_polling_tree * tree );
void sctk_polling_tree_join( struct sctk_polling_tree * tree );

void sctk_polling_tree_poll( struct sctk_polling_tree * tree , void (*func)(void *), void * arg );



#endif /* SCTK_POLLING */
