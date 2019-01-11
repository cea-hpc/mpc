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
#ifdef MPC_USE_INFINIBAND

#include <sctk_debug.h>
#include <sctk_net_tools.h>
#include <sctk_ib.h>
#include <sctk_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sctk.h>
#include <sctk_asm.h>
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>
#include <sctk_route.h>
#include <sctk_ib.h>
#include <sctk_ib_qp.h>
#include <sctk_ib_cp.h>
#include <sctk_ib_toolkit.h>
#include <sctk_ib_rdma.h>
#include <sctk_ib_eager.h>
#include <sctk_ib_prof.h>
#include <sctk_route.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_io_helper.h>

/* Initialize a new route table */
void sctk_ib_init_remote ( int dest, sctk_rail_info_t *rail, struct sctk_endpoint_s *route_table, int ondemand )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	sctk_ib_route_info_t *route_ib;

	/* Init endpoint */
	sctk_route_origin_t origin = ROUTE_ORIGIN_STATIC;
	
	if( ondemand )
	{
		 origin = ROUTE_ORIGIN_DYNAMIC;
	}
		
	sctk_endpoint_init( route_table,  dest, rail, origin );

	/* Fill IB dependent CTX */

	route_ib = &route_table->data.ib;
	route_ib->remote = sctk_ib_qp_new();

	sctk_nodebug ( "Initializing QP for dest %d", dest );
	
	/* Allocate QP */
	sctk_ib_qp_allocate_init ( rail_ib, dest, route_ib->remote, ondemand, route_table );
	
	return;
}

/* Create a new route table */
sctk_endpoint_t * sctk_ib_create_remote()
{
	sctk_endpoint_t *tmp;


	tmp = sctk_malloc ( sizeof ( sctk_endpoint_t ) );

	assume( tmp != NULL );

	return tmp;
}

char *sctk_ib_print_procotol ( sctk_ib_protocol_t p )
{
	switch ( p )
	{
		case SCTK_IB_EAGER_PROTOCOL:
			return "SCTK_IB_EAGER_PROTOCOL";

		case SCTK_IB_RDMA_PROTOCOL:
			return "SCTK_IB_RDMA_PROTOCOL";

		case SCTK_IB_BUFFERED_PROTOCOL:
			return "SCTK_IB_BUFFERED_PROTOCOL";

		default:
			not_reachable();
	}

	return NULL;
}

void sctk_ib_print_msg ( sctk_thread_ptp_message_t *msg )
{
	sctk_error ( "IB protocol: %s", sctk_ib_print_procotol ( msg->tail.ib.protocol ) );

	switch ( msg->tail.ib.protocol )
	{
		case SCTK_IB_EAGER_PROTOCOL:
			break;

		case SCTK_IB_RDMA_PROTOCOL:
			sctk_ib_rdma_print ( msg );
			break;

		case SCTK_IB_BUFFERED_PROTOCOL:
			break;

		default:
			not_reachable();
	}

}

#endif
