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
/* #                                                                      # */
/* ######################################################################## */


#include "lowcomm.h"


#include <mpc_common_rank.h>
#include <mpc_launch.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm.h>
#include <mpc_topology_device.h>
#include <sctk_alloc.h>
#include <stdlib.h>

#include "alloc_mem.h"
#include "mpc_common_debug.h"
#include "checksum.h"



/*Networks*/
#include "driver.h"

#ifdef MPC_USE_DMTCP
#include "sctk_ft_iface.h"
#endif

#include "lowcomm_config.h"


#define MPC_MODULE "Lowcomm/Lowcomm"

/************************************************************************/
/* Network INIT                                                         */
/************************************************************************/


void sctk_net_init_task_level(int taskid, int vp)
{
	int i = 0;

	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->initialize_task)
		{
			rail->initialize_task(rail, taskid, vp);
		}
	}
}

void sctk_net_finalize_task_level(int taskid, int vp)
{
	int i = 0;

	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->finalize_task)
		{
			rail->finalize_task(rail, taskid, vp);
		}
	}
}



/********************************************************************/
/* Hybrid MPI+X                                                     */
/********************************************************************/
static int is_mode_hybrid = 0;

int sctk_net_is_mode_hybrid()
{
	return is_mode_hybrid;
}

int sctk_net_set_mode_hybrid()
{
	is_mode_hybrid = 1;
	return 0;
}
