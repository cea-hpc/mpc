/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpc_common_debug.h>
#include <sctk_net_tools.h>
#include <ib.h>
#include <mpc_launch_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <mpc_common_asm.h>
#include <netdb.h>
#include <mpc_common_spinlock.h>
#include <sctk_net_tools.h>
#include <ib.h>
#include <qp.h>
#include <cp.h>
#include <ibtoolkit.h>
#include <ibrdma.h>
#include <ibeager.h>

#include "sctk_rail.h"
#include <ibufs_rdma.h>
#include <mpc_common_helper.h>
#include <sctk_alloc.h>


/* Initialize a new route table */
void sctk_ib_init_remote ( mpc_lowcomm_peer_uid_t dest, sctk_rail_info_t *rail, struct _mpc_lowcomm_endpoint_s *route_table, int ondemand )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	_mpc_lowcomm_endpoint_info_ib_t *route_ib;

	/* Init endpoint */
	_mpc_lowcomm_endpoint_type_t origin = _MPC_LOWCOMM_ENDPOINT_STATIC;
	
	if( ondemand )
	{
		 origin = _MPC_LOWCOMM_ENDPOINT_DYNAMIC;
	}
		
	_mpc_lowcomm_endpoint_init( route_table,  dest, rail, origin );

	/* Fill IB dependent CTX */

	route_ib = &route_table->data.ib;
	route_ib->remote = sctk_ib_qp_new();

	mpc_common_nodebug ( "Initializing QP for dest %d", dest );
	
	/* Allocate QP */
	sctk_ib_qp_allocate_init ( rail_ib, dest, route_ib->remote, ondemand, route_table );
	
	return;
}

/* Create a new route table */
_mpc_lowcomm_endpoint_t * sctk_ib_create_remote()
{
	_mpc_lowcomm_endpoint_t *tmp;


	tmp = sctk_malloc ( sizeof ( _mpc_lowcomm_endpoint_t ) );

	assume( tmp != NULL );

	return tmp;
}

char *sctk_ib_print_procotol ( _mpc_lowcomm_ib_protocol_t p )
{
	switch ( p )
	{
		case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
			return "MPC_LOWCOMM_IB_EAGER_PROTOCOL";

		case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
			return "MPC_LOWCOMM_IB_RDMA_PROTOCOL";

		case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
			return "MPC_LOWCOMM_IB_BUFFERED_PROTOCOL";

		default:
			not_reachable();
	}

	return NULL;
}

void sctk_ib_print_msg ( mpc_lowcomm_ptp_message_t *msg )
{
	mpc_common_debug_error ( "IB protocol: %s", sctk_ib_print_procotol ( msg->tail.ib.protocol ) );

	switch ( msg->tail.ib.protocol )
	{
		case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
			break;

		case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
			_mpc_lowcomm_ib_rdma_print ( msg );
			break;

		case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
			break;

		default:
			not_reachable();
	}

}
