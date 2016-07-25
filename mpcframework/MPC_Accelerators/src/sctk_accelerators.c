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
#include <sctk_device_topology.h>
#include <sctk_allocator.h>

static size_t nb_devices = 0;

int sctk_accl_init()
{

	sctk_device_t** list = sctk_device_get_from_handle_regexp("card*", &nb_devices);

	/* we don't want the list but the number of devices 
	 * Maybe there are a better way for doing it...
	 */
	sctk_free(list);

#ifdef MPC_USE_CUDA
	cuInit(0);
#endif
}

/**
 * Returns the number of GPUs on the current node.
 * 
 * Especially used to check CUDA can be used without errors.
 */
size_t sctk_accl_get_nb_devices()
{
	return nb_devices;
}
#endif
