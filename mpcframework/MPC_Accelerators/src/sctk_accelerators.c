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

#ifdef MPC_Accelerators
#include <sctk_accelerators.h>
#include <sctk_alloc.h>
#include <sctk_device_topology.h>

static size_t nb_devices = 0;
extern bool sctk_accl_support;

/**
 * Initialize MPC_Accelerators.
 *
 * Should call sub-module init functions
 * @return 0 if everything succeded, 1 otherwise
 */
int sctk_accl_init() {
	
  if( ! sctk_accl_support)
  {
	  nb_devices = 0;
	  return 1;
  }

  sctk_warning("Accelerators support ENABLED");
  
  sctk_device_t **list = sctk_device_get_from_handle_regexp("cuda-enabled-card*", (int*)&nb_devices);
  sctk_free(list);

#ifdef MPC_USE_CUDA
	sctk_accl_cuda_init();
#endif

#ifdef MPC_USE_OPENACC
#endif

#ifdef MPC_USE_OPENCL
#endif
  return 0;
}

/**
 * Returns the number of GPUs on the current node.
 *
 * Especially used to check CUDA can be used without errors.
 * @return the number of devices
 */
size_t sctk_accl_get_nb_devices() 
{ 
	return nb_devices; 
}
#endif
