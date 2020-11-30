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

#include "ofi.h"
#include "ofi_msg.h"
#include "ofi_rdma.h"

/**
 * @brief Redirect to the driver to initialize from configuration set by user
 * 
 * @param rail the rail owning the driver to inialize
 */
void sctk_network_init_ofi( sctk_rail_info_t *rail )
{
	switch(rail->runtime_config_driver_config->driver.value.ofi.link)
	{
		case MPC_LOWCOMM_OFI_CONNECTED:
			sctk_network_init_ofi_msg(rail);
			break;
		case MPC_LOWCOMM_OFI_CONNECTIONLESS:
			sctk_network_init_ofi_rdma(rail);
			break;
		default:
			mpc_common_debug_fatal("'%s' is not a supported OFI driver");
			not_reachable();
	}
}
