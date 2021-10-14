/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:05 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Romain Pereira <pereirar@ocre.cea.fr>                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPC_OMP_PLACES_ENV_H__
#define __MPC_OMP_PLACES_ENV_H__

#include "hwloc.h"
#include "utlist.h"
#include "mpc_common_debug.h"
#include "sctk_alloc.h"

typedef struct mpc_omp_places_info_s
{
	unsigned int id;
	hwloc_bitmap_t interval;
	hwloc_bitmap_t logical_interval;
	struct mpc_omp_places_info_s *prev, *next;
} mpc_omp_places_info_t;

void mpc_omp_display_places( mpc_omp_places_info_t *list );
mpc_omp_places_info_t *_mpc_omp_places_env_variable_parsing( const int );
int mpc_omp_places_get_real_nb_mvps( mpc_omp_places_info_t *list );
int _mpc_omp_places_get_topo_info( mpc_omp_places_info_t *list, int **shape, int **cpus_order );

hwloc_bitmap_t mpc_omp_places_get_default_include_bitmap( const int );

#endif /* __MPC_OMP_PLACES_ENV_H__*/
