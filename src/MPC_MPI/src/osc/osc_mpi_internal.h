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

/**@file
 *
 * This file contains the internal structures and functions of the MPI OSC submodule.
 * It is meant to be used only by this very submodule.
 */

/** @defgroup MPI_OSC_INTERNAL Internal implementation of MPI One-Sided
 * @{
 * @ingroup MPI_OSC
 * @brief Internals of the MPI module to perform RMA operations
 * @}
 */

#ifndef MPC_MPI_OSC_INTERNAL_H_
#define MPC_MPI_OSC_INTERNAL_H_

#include <mpc_common_datastructure.h> // For mpc_common_hashtable
#include <mpc_lowcomm_one_sided.h>    // For mpc_lowcomm_window_t

#include "mpc_mpi.h"                  // For MPI_Info, MPI_MAX_OBJECT_NAME...

/**@ingroup MPI_OSC_INTERNAL
 * Possible epoch (access and exposure) */
typedef enum mpc_mpi_osc_epoch_e
{
	NONE_EPOCH,           /**< No epoch opened */
	FENCE_EPOCH,          /**< Fence epoch opened */
	POST_WAIT_EPOCH,      /**< Post wait epoch opened (between a post and a wait) */
	START_COMPLETE_EPOCH, /**< Start complete epoch opened (between a start and a complete) */
	PASSIVE_EPOCH,        /**< Passive epoch opened (between lock and unlock) */
	PASSIVE_ALL_EPOCH,    /**< Passive all epoch opened (between lock_all and unlock_all) */
} mpc_mpi_osc_epoch_t;

/**@ingroup MPI_OSC_INTERNAL
 * Structure keeping the epoch state for both access and exposure */
typedef struct mpc_mpi_osc_epoch_type_s
{
	mpc_mpi_osc_epoch_t access;   /**< Epoch state for window access */
	mpc_mpi_osc_epoch_t exposure; /**< Epoch state for window exposure */
} mpc_mpi_osc_epoch_type_t;

/**@ingroup MPI_OSC_INTERNAL
 * Internal structure of an MPI window */
typedef struct mpc_mpi_win_s
{
	mpc_lowcomm_window_t *      win_module;                    /**< Lowcomm window */
	char                        win_name[MPI_MAX_OBJECT_NAME]; /**< Name of the window */

	int                         flavor;                        /**< Creation flavor of the window */
	int                         model;                         /**< Model of the window */
	size_t                      size;                          /**< Size of the underlying buffer */
	mpc_mpi_osc_epoch_type_t    epoch;                         /**< Epoch states for access and exposure */
	OPA_int_t                   lock_count;                    /**< Number of lock on the window */

	int                         disp_unit;                     /**< Displacement unit if it is the same across tasks */
	int *                       disp_units;                    /**< Displacement units of the all the tasks */

	struct mpc_common_hashtable attrs;                         /**< Attributes of the window */
	MPI_Info                    info;                          /**< Info stored in the window */
} mpc_mpi_win_t;

/**@ingroup MPI_OSC_INTERNAL
 * Helper to get the displacement unit at the target window */
#define OSC_GET_DISP(_module, _target) \
		((_module)->disp_unit < 0 ? (_module)->disp_units[_target] : (_module)->disp_unit)

/**@ingroup MPI_OSC_INTERNAL
 * @brief Release the attributes hashtable of a window
 *
 * @param[in] atht Hashtable to release
 * @return         MPI errorcode
 */
int osc_win_attr_ht_release(struct mpc_common_hashtable *atht);

#endif // MPC_MPI_OSC_INTERNAL_H_
