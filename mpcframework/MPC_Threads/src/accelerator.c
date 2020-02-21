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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_thread_accelerator.h>
#include <sctk_alloc.h>
#include <mpc_topology_device.h>
#include <mpc_common_flags.h>

static size_t nb_devices = 0;

/**
 * Initialize MPC_Accelerators.
 *
 * Should call sub-module init functions
 * @return 0 if everything succeded, 1 otherwise
 */
void sctk_accl_init() {

  if (!mpc_common_get_flags()->enable_accelerators) {
    nb_devices = 0;
    return;
  }

  sctk_warning("Accelerators support ENABLED");

  mpc_topology_device_t **list = mpc_topology_device_get_from_handle_regexp(
      "cuda-enabled-card*", (int *)&nb_devices);
  sctk_free(list);

#ifdef MPC_USE_CUDA
  sctk_accl_cuda_init();
#endif

#ifdef MPC_USE_OPENACC
#endif

#ifdef MPC_USE_OPENCL
#endif
}

/**
 * Returns the number of GPUs on the current node.
 *
 * Especially used to check CUDA can be used without errors.
 * @return the number of devices
 */
size_t sctk_accl_get_nb_devices() { return nb_devices; }

/*********************************
 * MPC ACCELERATOR INIT FUNCTION *
 *********************************/

void mpc_accelerator_register_function() __attribute__( ( constructor ) );

void mpc_accelerator_register_function()
{
	MPC_INIT_CALL_ONLY_ONCE


	mpc_common_init_callback_register( "Base Runtime Init Done", "Init accelerator Module", sctk_accl_init, 25 );
}