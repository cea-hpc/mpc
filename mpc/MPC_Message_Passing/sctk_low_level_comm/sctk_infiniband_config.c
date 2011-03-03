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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk.h>
#include "sctk_infiniband_config.h"

/* default constants */
#define IBV_EAGER_THRESHOLD (16 * 1024)
#define IBV_QP_TX_DEPTH     100
#define IBV_QP_RC_DEPTH     100
#define IBV_MAX_SG_SQ       4
#define IBV_MAX_SG_RQ       4
#define IBV_MAX_INLINE      128
#define IBV_MAX_IBUFS       600
#define IBV_MAX_SRQ_IBUFS   100
#define IBV_SRQ_CREDIT_LIMIT 30

#define IBV_WC_IN_NUMBER    20
#define IBV_WC_OUT_NUMBER   20

#define IBV_MAX_MR          2000
#define IBV_ADM_PORT        1

#define IBV_RDMA_DEPTH       4
#define IBV_RDMA_DEST_DEPTH  1

/* global values */
int  ibv_eager_threshold  = IBV_EAGER_THRESHOLD;
int  ibv_qp_tx_depth      = IBV_QP_TX_DEPTH;
int  ibv_qp_rx_depth      = IBV_QP_RC_DEPTH;
int  ibv_max_sg_sq        = IBV_MAX_SG_SQ;
int  ibv_max_sg_rq        = IBV_MAX_SG_RQ;
int  ibv_max_inline       = IBV_MAX_INLINE;
int  ibv_max_ibufs        = IBV_MAX_IBUFS;
int  ibv_max_srq_ibufs    = IBV_MAX_SRQ_IBUFS;
int  ibv_srq_credit_limit = IBV_SRQ_CREDIT_LIMIT;
int  ibv_rdvz_protocol    = IBV_RDVZ_READ_PROTOCOL;

int  ibv_wc_in_number     = IBV_WC_IN_NUMBER;
int  ibv_wc_out_number    = IBV_WC_OUT_NUMBER;
int  ibv_max_mr           = IBV_MAX_MR;
int  ibv_adm_port         = IBV_ADM_PORT;
int  ibv_rdma_depth       = IBV_RDMA_DEPTH;
int  ibv_rdma_dest_depth  = IBV_RDMA_DEST_DEPTH;

sctk_net_ibv_config_init()
{
  char* value;

  if ( (value = getenv("MPC_IBV_EAGER_THRESHOLD")) != NULL )
    ibv_eager_threshold = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_TX_DEPTH")) != NULL )
    ibv_qp_tx_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_RX_DEPTH")) != NULL )
    ibv_qp_rx_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_SG_SQ")) != NULL )
    ibv_max_sg_sq = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_RG_SQ")) != NULL )
    ibv_max_sg_rq = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_INLINE")) != NULL )
    ibv_max_inline = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_IBUFS")) != NULL )
    ibv_max_ibufs = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_SRQ_IBUFS")) != NULL )
    ibv_max_srq_ibufs = atoi(value);

  if ( (value = getenv("MPC_IBV_SRQ_CREDIT_LIMIT")) != NULL )
    ibv_srq_credit_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_RDVZ_WRITE_PROTOCOL")) != NULL )
    ibv_rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
  if ( (value = getenv("MPC_IBV_RDVZ_READ_PROTOCOL")) != NULL )
    ibv_rdvz_protocol = IBV_RDVZ_READ_PROTOCOL;

  if ( (value = getenv("MPC_IBV_WC_IN_NUMBER")) != NULL )
    ibv_rdvz_protocol = IBV_WC_IN_NUMBER;

  if ( (value = getenv("MPC_IBV_WC_OUT_NUMBER")) != NULL )
    ibv_rdvz_protocol = IBV_WC_OUT_NUMBER;

  if ( (value = getenv("MPC_IBV_MAX_MR")) != NULL )
    ibv_rdvz_protocol = IBV_MAX_MR;

  if ( (value = getenv("MPC_IBV_ADM_PORT")) != NULL )
    ibv_rdvz_protocol = IBV_ADM_PORT;

  if ( (value = getenv("MPC_IBV_RDMA_DEPTH")) != NULL )
    ibv_rdvz_protocol = IBV_RDMA_DEPTH;

  if ( (value = getenv("MPC_IBV_RDMA_DEST_DEPTH")) != NULL )
    ibv_rdvz_protocol = IBV_RDMA_DEST_DEPTH;
}

