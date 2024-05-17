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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_PERFORMANCE_TREE
#define SCTK_PERFORMANCE_TREE


#include "sctk_profiler_array.h"


struct sctk_performance_tree
{
	/* Level relative metrics */
	double entry_relative_percentage_time[ SCTK_PROFILE_KEY_COUNT ];
	double entry_relative_percentage_hits[ SCTK_PROFILE_KEY_COUNT ];

	/* Global metrics */
	uint64_t total_time;
	uint64_t total_hits;

	double entry_total_percentage_time[ SCTK_PROFILE_KEY_COUNT ];
	double entry_total_percentage_hits[ SCTK_PROFILE_KEY_COUNT ];

	uint64_t entry_average_time[ SCTK_PROFILE_KEY_COUNT ];

};


void  sctk_performance_tree_init( struct sctk_performance_tree *tr, struct sctk_profiler_array *array);
void  sctk_performance_tree_release( struct sctk_performance_tree *tr );



#endif /* SCTK_PERFORMANCE_TREE */
