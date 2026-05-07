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
/* #   - BEAULIEU Corentin corentin.beaulieu@cea.fr                       # */
/* #                                                                      # */
/* ######################################################################## */

/**@file
 * OpenSHMEM module shared internal structure and helpers
 */

#ifndef OSHMEM_INTERNAL_H_
#include <stddef.h>                // For size_t

#include <mpc_common_spinlock.h>   // For mpc_common_spinlock_t
#include <mpc_lowcomm_one_sided.h> // For mpc_lowcomm_window_t

/** @defgroup openshmem_internals INTERNALS
 * @ingroup MPC_OPENSHMEM
 * @{
 * @brief MPC OpenSHMEM Internal datastructures and functions
 */

/** Context of OpenSHMEM task */
typedef struct _mpc_oshmem_context_s
{
	mpc_lowcomm_window_t *   symmetric_heap_window; /**< Window to target the symmetric heap */
	mpc_lowcomm_window_t *   user_bss_window;       /**< Window to target static variables */
	mpc_lowcomm_window_t *   user_data_window;      /**< Window to target global variables */

	void *                   symmetric_heap_start;  /**< First address of the symmetric heap */
	void *                   user_bss_start;        /**< First address of the .bss ELF section */
	void *                   user_data_start;       /**< First address of the .data ELF section */

	size_t                   symmetric_heap_size;   /**< Total size of the symmetric heap */
	size_t                   user_bss_size;         /**< Total size of the .bss ELF section */
	size_t                   user_data_size;        /**< Total size of the .data ELF section */

	_Atomic volatile ssize_t refcount;              /**< Counter of the number of refs on the context */
	mpc_common_spinlock_t    context_lock;          /**< Lock to manipulate the context */
}_mpc_oshmem_context_t;

/** Converts an MPI request (returned by @ref mpc_lowcomm_request_alloc ...) into a lowcomm request */
#define _mpc_oshmem_convert_mpi_request_to_lowcomm(__req) (((mpc_lowcomm_request_t *)__req) - 1)

/** @} */  // openshmem_internals

#endif // !OSHMEM_INTERNAL_H_
