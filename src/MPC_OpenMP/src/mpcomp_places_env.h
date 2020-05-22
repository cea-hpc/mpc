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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPCOMP_PLACES_ENV_H__
#define __MPCOMP_PLACES_ENV_H__

#include "hwloc.h"
#include "utlist.h"
#include "mpc_common_debug.h"
#include "sctk_alloc.h"

typedef struct mpcomp_places_info_s
{
	unsigned int id;
	hwloc_bitmap_t interval;
	hwloc_bitmap_t logical_interval;
	struct mpcomp_places_info_s *prev, *next;
} mpcomp_places_info_t;

void mpcomp_display_places( mpcomp_places_info_t *list );
mpcomp_places_info_t *mpcomp_places_env_variable_parsing( const int );
int mpcomp_places_get_real_nb_mvps( mpcomp_places_info_t *list );
int mpcomp_places_get_topo_info( mpcomp_places_info_t *list, int **shape, int **cpus_order );

hwloc_bitmap_t mpcomp_places_get_default_include_bitmap( const int );

#endif /* __MPCOMP_PLACES_ENV_H__*/
