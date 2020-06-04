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

#ifndef __SCTK__IB_POLLING_H_
#define __SCTK__IB_POLLING_H_

#include <infiniband/verbs.h>
#include <mpc_common_rank.h>

#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include <mpc_launch_pmi.h>
#include "utlist.h"

#include "mpc_runtime_config.h"

typedef struct sctk_ib_polling_s
{
	int recv_own;
	int recv_other;
	int recv_cq;
	int recv;
} sctk_ib_polling_t;

#define  POLL_CQ_BUSY -1
#define  POLL_CQ_SKIPPED -2

#define HOSTNAME 2048
#define POLL_INIT(x) do { (x)->recv_other = 0;        \
						  (x)->recv_own = 0;          \
						  (x)->recv = 0;              \
						  (x)->recv_cq = 0; } while(0)

#define POLL_RECV_OWN(x) do { (x)->recv_own ++;                   \
							  (x)->recv ++; } while(0)

#define POLL_RECV_OTHER(x) do { (x)->recv_other ++;                   \
								(x)->recv ++; } while(0)

#define POLL_RECV_CQ(x) do { (x)->recv_cq ++;                   \
							 (x)->recv ++; } while(0)

#define POLL_RECV_CQ(x) do { (x)->recv_cq ++;                   \
							 (x)->recv ++; } while(0)

#define POLL_GET_RECV(x) ((x)->recv)

#define POLL_GET_RECV_CQ(x) ((x)->recv_cq)
#define POLL_SET_RECV_CQ(x, y) ((x)->recv_cq = y)

__UNUSED__ static inline void sctk_ib_polling_check_wc ( struct sctk_ib_rail_info_s *rail_ib,  struct ibv_wc wc )
{
	struct sctk_runtime_config_struct_net_driver_infiniband *config = ( rail_ib )->config;
	struct _mpc_lowcomm_ib_ibuf_s *ibuf;
	char host[HOSTNAME];
	char ibuf_desc[4096];

	if ( wc.status != IBV_WC_SUCCESS )
	{
		ibuf = ( struct _mpc_lowcomm_ib_ibuf_s * ) wc.wr_id;
		assume ( ibuf );
		gethostname ( host, HOSTNAME );

		if ( config->quiet_crash )
		{
			mpc_common_debug_error ( "\nIB - PROCESS %d CRASHED (%s): %s",
			             mpc_common_get_process_rank(), host, ibv_wc_status_str ( wc.status ) );
		}
		else
		{
			_mpc_lowcomm_ib_ibuf_print ( ibuf, ibuf_desc );
			mpc_common_debug_error ( "\nIB - FATAL ERROR FROM PROCESS %d (%s)\n"
			             "################################\n"
			             "Work ID is   : %d\n"
			             "Status       : %s\n"
			             "ERROR Vendor : %d\n"
			             "Byte_len     : %d\n"
			             "Dest process : %d\n"
			             "######### IBUF DESC ############\n"
			             "%s\n"
			             "################################", mpc_common_get_process_rank(),
			             host,
			             wc.wr_id,
			             ibv_wc_status_str ( wc.status ),
			             wc.vendor_err,
			             wc.byte_len,
			             /* Remote can be NULL if the buffer comes from SRQ */
			             ( ibuf->remote ) ? ibuf->remote->rank : 0,
			             ibuf_desc );
		}

		mpc_common_debug_abort();
	}
}

#define WC_COUNT 100
static inline void sctk_ib_cq_poll ( sctk_rail_info_t *rail,
                                                struct ibv_cq *cq,
                                                __UNUSED__ const int poll_nb,
                                                struct sctk_ib_polling_s *poll,
                                                int ( *ptr_func ) ( sctk_rail_info_t *rail, struct ibv_wc *) )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	struct ibv_wc wc[WC_COUNT];
	int res = 0;
	int i;

	do
	{
		res = ibv_poll_cq ( cq, WC_COUNT, wc );

		if ( res )
			mpc_common_nodebug ( "Polled %d msgs from cq", res );

		for ( i = 0; i < res; ++i )
		{
			sctk_ib_polling_check_wc ( rail_ib, wc[i] );
			ptr_func ( rail, &wc[i] );
			POLL_RECV_CQ ( poll );
		}
	}
	while ( res == WC_COUNT );
}

#endif
