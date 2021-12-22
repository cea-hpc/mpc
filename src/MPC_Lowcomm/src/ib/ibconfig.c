/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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

#include "ibconfig.h"

/* IB debug macros */
#if defined MPC_LOWCOMM_IB_MODULE_NAME
#error "MPC_LOWCOMM_IB_MODULE already defined"
#endif
#define MPC_LOWCOMM_IB_MODULE_NAME "CONFIG"
#include "ibtoolkit.h"
#include "ib.h"
#include "ibeager.h"
#include "ibufs_rdma.h"
#include "buffered.h"
#include "mpc_conf.h"
#include "sctk_rail.h"
#include <mpc_common_rank.h>

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/


#define EXPERIMENTAL(str) #str" (experimental)"
void sctk_ib_config_print ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );

	return;

	if ( mpc_common_get_process_rank() == 0 )
	{
		fprintf ( stderr, "############# IB configuration\n"
		          "eager_limit      = %ld\n"
		          "buffered_limit = %ld\n"
		          "max_rdma_connections = %d\n"
		          "rdma_min_size = %ld\n"
		          "rdma_max_size = %ld\n"
		          "rdma_min_nb = %d\n"
		          "rdma_max_nb = %d\n"
		          "rdma_resizing_min_size, = %ld\n"
		          "rdma_resizing_max_size = %ld\n"
		          "rdma_resizing_min_nb = %d\n"
		          "rdma_resizing_max_nb = %d\n"
		          "rdma_resizing    = %d\n"
		          "qp_tx_depth      = %d\n"
		          "qp_rx_depth      = %d\n"
		          "max_sg_sq        = %d\n"
		          "max_sg_rq        = %d\n"
		          "max_inline       = %ld\n"
		          "init_ibufs        = %d\n"
		          "init_recv_ibufs        = %d\n"
		          "max_srq_ibufs_posted    = %d\n"
		          "size_ibufs_chunk = %d\n"
		          "size_recv_ibufs_chunk = %d\n"
		          "srq_credit_thread_limit = %d\n"
		          "adm_port         = %d\n"
		          "rdma_depth       = %d\n"
		          "rdma_depth  = %d\n"
		          "quiet_crash      = %d\n"
		          "async_thread     = %d\n"
		          "#############\n",
		          config->eager_limit,
		          config->buffered_limit,
		          config->max_rdma_connections,
		          config->rdma_min_size,
		          config->rdma_max_size,
		          config->rdma_min_nb,
		          config->rdma_max_nb,
		          config->rdma_resizing_min_size,
		          config->rdma_resizing_max_size,
		          config->rdma_resizing_min_nb,
		          config->rdma_resizing_max_nb,
		          config->rdma_resizing,
		          config->qp_tx_depth,
		          config->qp_rx_depth,
		          config->max_sg_sq,
		          config->max_sg_rq,
		          config->max_inline,
		          config->init_ibufs,
		          config->init_recv_ibufs,
		          config->max_srq_ibufs_posted,
		          config->size_ibufs_chunk,
		          config->size_recv_ibufs_chunk,
		          config->srq_credit_thread_limit,
		          config->adm_port,
		          config->rdma_depth,
		          config->rdma_depth,
		          config->quiet_crash,
		          config->async_thread);
	}
}

void sctk_ib_config_mutate ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );

	config->eager_limit       = ALIGN_ON ( config->eager_limit + IBUF_GET_EAGER_SIZE, 64 );
	config->buffered_limit  = ( config->buffered_limit + sizeof ( mpc_lowcomm_ptp_message_body_t ) );
}

void sctk_ib_config_init ( sctk_ib_rail_info_t *rail_ib, __UNUSED__ char *network_name )
{


	rail_ib->config = &rail_ib->rail->runtime_config_driver_config->driver.value.infiniband;
	sctk_ib_config_mutate ( rail_ib );
}
