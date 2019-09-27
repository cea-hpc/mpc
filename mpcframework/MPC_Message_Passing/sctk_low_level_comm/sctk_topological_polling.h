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
#ifndef SCTK_TOPOLOGICAL_POLLING
#define SCTK_TOPOLOGICAL_POLLING

#include "sctk_debug.h"
#include "mpc_common_spinlock.h"
#include <sctk_atomics.h>
#include <sctk_runtime_config_struct.h>

/************************************************************************/
/* ENUM DEFINITION                                                      */
/************************************************************************/

/** This enum defines the range and coverage of polling
 * according to the allowed topology */
typedef enum
{
	SCTK_POLL_NOT_SET = 0,  /**< This is for error handling */
	SCTK_POLL_NONE,  /**< This is to deactivate polling */
	SCTK_POLL_PU,  /**< This is for polling at core level */
	SCTK_POLL_CORE,  /**< This is for polling at core level */
	SCTK_POLL_SOCKET, /**< Socket level */
	SCTK_POLL_NUMA, /**< Numa level */
	SCTK_POLL_MACHINE, /**< Machine level */
}sctk_topological_polling_set_t;

/************************************************************************/
/* CONSTANTS                                                            */
/************************************************************************/

/** This constant means that this cell triggers the polling call */
#define SCTK_POLL_TRIGGER (-1)
/** This constant means that the cells is ignored */
#define SCTK_POLL_IGNORE  (-2)


/************************************************************************/
/* TOPOLOGICAL POLLING CELL SCTRUCTURE                                  */
/************************************************************************/

/** This defines a polling cell used by a VP */
struct sctk_topological_polling_cell
{
	sctk_atomics_int polling_counter; /**< Counter when doing the polling */
	int cell_id; /**< This is the ID of the cell relatively to the closest trigger
	              * cell, see the polling function implementation for details
	              * note that this field can take the special values SCTK_POLL_TRIGGER and SCTK_POLL_IGNORE */
};

void sctk_topological_polling_cell_init( struct sctk_topological_polling_cell * cell );

/************************************************************************/
/* TOPOLOGICAL POLLING TREE SCTRUCTURE                                  */
/************************************************************************/

struct sctk_topological_polling_tree
{
	struct sctk_topological_polling_cell * cells; /**< Polling cells (one for each VP) */
	int cell_count; /**< Cell count, in fact the number of VPs */
	sctk_topological_polling_set_t polling_trigger; /**< Trigger is the value defining at which level the polling is done */
	sctk_topological_polling_set_t polling_range; /**< Range defines how far from the root PU we do POLL */
	int root_pu_id; /**< This is the ID of the core which is the closest from the polling target */
};

void sctk_topological_polling_tree_init( struct sctk_topological_polling_tree * tree,
                                         sctk_topological_polling_set_t trigger,
                                         sctk_topological_polling_set_t range,
                                         int root_pu );

sctk_topological_polling_set_t sctk_rail_convert_polling_set_from_config( enum rail_topological_polling_level poll );

void sctk_topological_polling_tree_poll( struct sctk_topological_polling_tree * tree,  void (*func)( void *), void * arg );

#endif /* SCTK_TOPOLOGICAL_POLLING */
