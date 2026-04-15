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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_MPI_OSC_INTERNAL_H_
#define MPC_MPI_OSC_INTERNAL_H_

#include <mpc_common_datastructure.h> // For mpc_common_hashtable
#include "mpc_mpi.h"

typedef struct mpc_lowcomm_window_s mpc_lowcomm_window_t;

typedef enum mpc_mpi_osc_epoch_e
{
	NONE_EPOCH,
	FENCE_EPOCH,
	POST_WAIT_EPOCH,
	START_COMPLETE_EPOCH,
	PASSIVE_EPOCH,
	PASSIVE_ALL_EPOCH,
} mpc_mpi_osc_epoch_t;

typedef struct mpc_mpi_osc_epoch_type_s
{
	mpc_mpi_osc_epoch_t access;
	mpc_mpi_osc_epoch_t exposure;
} mpc_mpi_osc_epoch_type_t;

typedef struct mpc_mpi_win_s
{
	mpc_lowcomm_window_t *      win_module;
	char                        win_name[MPI_MAX_OBJECT_NAME];

	int                         flavor;
	int                         model;
	size_t                      size;
	mpc_mpi_osc_epoch_type_t    epoch;
	OPA_int_t                   lock_count;

	int                         disp_unit;
	int *                       disp_units;

	MPI_Info                    info;
	struct mpc_common_hashtable attrs;
} mpc_mpi_win_t;

#define OSC_GET_DISP(_module, _target) \
		((_module)->disp_unit < 0 ? (_module)->disp_units[_target] : (_module)->disp_unit)

int mpc_mpi_osc_win_attr_ht_release(struct mpc_common_hashtable *atht);

#endif // MPC_MPI_OSC_INTERNAL_H_
