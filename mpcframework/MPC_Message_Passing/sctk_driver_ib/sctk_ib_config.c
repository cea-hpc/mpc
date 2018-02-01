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

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_config.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_NAME "CONFIG"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_runtime_config.h"

static const char *steal_names[2] =
{
	"Normal mode",
	"Collaborative-polling mode"
};

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_config_check ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );

	if ( ( sctk_process_rank == 0 )
	        && ( config->low_memory ) )
	{
		sctk_error ( "LOW mem module enabled: use it at your own risk!" );
	}

	/* Good conf, we return */
	return;
}

#define EXPERIMENTAL(str) #str" (experimental)"
void sctk_ib_config_print ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );

	return;

	if ( sctk_process_rank == 0 )
	{
		fprintf ( stderr, "############# IB configuration\n"
		          "eager_limit      = %d\n"
		          "buffered_limit = %d\n"
		          "max_rdma_connections = %d\n"
		          "rdma_min_size = %d\n"
		          "rdma_max_size = %d\n"
		          "rdma_min_nb = %d\n"
		          "rdma_max_nb = %d\n"
		          "rdma_resizing_min_size, = %d\n"
		          "rdma_resizing_max_size = %d\n"
		          "rdma_resizing_min_nb = %d\n"
		          "rdma_resizing_max_nb = %d\n"
		          "rdma_resizing    = %d\n"
		          "qp_tx_depth      = %d\n"
		          "qp_rx_depth      = %d\n"
		          "max_sg_sq        = %d\n"
		          "max_sg_rq        = %d\n"
		          "max_inline       = %d\n"
		          "init_ibufs        = %d\n"
		          "init_recv_ibufs        = %d\n"
		          "max_srq_ibufs_posted    = %d\n"
		          "max_srq_ibufs    = %d\n"
		          "size_ibufs_chunk = %d\n"
		          "size_recv_ibufs_chunk = %d\n"
		          "srq_credit_limit = %d\n"
		          "srq_credit_thread_limit = %d\n"
		          "rdvz_protocol    = %d\n"
		          "wc_in_number     = %d\n"
		          "wc_out_number    = %d\n"
		          "init_mr          = %d\n"
		          "adm_port         = %d\n"
		          "rdma_depth       = %d\n"
		          "rdma_depth  = %d\n"
		          "quiet_crash      = %d\n"
		          "async_thread     = %d\n"
		          EXPERIMENTAL ( steal ) "            = %d\n"
		          "Stealing desc        = %s\n"
		          EXPERIMENTAL ( low_memory ) "            = %d\n"
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
		          config->max_srq_ibufs,
		          config->size_ibufs_chunk,
		          config->size_recv_ibufs_chunk,
		          config->srq_credit_limit,
		          config->srq_credit_thread_limit,
		          config->rdvz_protocol,
		          config->wc_in_number,
		          config->wc_out_number,
		          config->init_mr,
		          config->adm_port,
		          config->rdma_depth,
		          config->rdma_depth,
		          config->quiet_crash,
		          config->async_thread,
		          config->steal, steal_names[config->steal],
		          config->low_memory );
	}
}

void sctk_ib_config_mutate ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );

	config->eager_limit       = ALIGN_ON ( config->eager_limit + IBUF_GET_EAGER_SIZE, 64 );
	config->buffered_limit  = ( config->buffered_limit + sizeof ( sctk_thread_ptp_message_body_t ) );
}

void sctk_ib_config_init ( sctk_ib_rail_info_t *rail_ib, char *network_name )
{
	LOAD_CONFIG ( rail_ib );

	rail_ib->config = &rail_ib->rail->runtime_config_driver_config->driver.value.infiniband;
	sctk_ib_config_mutate ( rail_ib );

	//Check if the variables are well set
	sctk_ib_config_check ( rail_ib );
}

#endif
