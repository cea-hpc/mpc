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
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_comp_rc_rdma.h"

/* default constants */
#define IBV_EAGER_THRESHOLD (8 * 1024)
/*
 * should be high values if the event
 * IBV_EVENT_QP_LAST_WQE_REACHED is triggered
 */
#define IBV_QP_TX_DEPTH     1000
#define IBV_QP_RC_DEPTH     1000
/* Many CQE. In memory, it represents about
 * 1.22Mb for 40000 entries */
#define IBV_CQ_DEPTH        40000
#define IBV_MAX_SG_SQ       4
#define IBV_MAX_SG_RQ       4
#define IBV_MAX_INLINE        128
#define IBV_MAX_IBUFS         3000
//#define IBV_MAX_IBUFS         50
// #define IBV_MAX_SRQ_IBUFS     2000
#define IBV_MAX_SRQ_IBUFS     2000 /*  > 300 */
#define IBV_SRQ_CREDIT_LIMIT  1000 /* >=300 */
#define IBV_SRQ_CREDIT_THREAD_LIMIT  500

//#define IBV_MAX_SRQ_IBUFS     50
// #define IBV_SRQ_CREDIT_LIMIT  800
//#define IBV_SRQ_CREDIT_LIMIT  10
#define IBV_SIZE_IBUFS_CHUNKS 200

#define IBV_WC_IN_NUMBER    100
#define IBV_WC_OUT_NUMBER   100

#define IBV_MAX_MR          3000
#define IBV_ADM_PORT        1

#define IBV_RDMA_DEPTH       4
#define IBV_RDMA_DEST_DEPTH  4

#define IBV_NO_MEMORY_LIMITATION  1
#define IBV_VERBOSE_LEVEL         1

/* global values */
int  ibv_eager_threshold  = IBV_EAGER_THRESHOLD;
int  ibv_qp_tx_depth      = IBV_QP_TX_DEPTH;
int  ibv_qp_rx_depth      = IBV_QP_RC_DEPTH;
int  ibv_cq_depth         = IBV_CQ_DEPTH;
int  ibv_max_sg_sq        = IBV_MAX_SG_SQ;
int  ibv_max_sg_rq        = IBV_MAX_SG_RQ;
int  ibv_max_inline       = IBV_MAX_INLINE;
int  ibv_max_ibufs        = IBV_MAX_IBUFS;
int  ibv_size_ibufs_chunk = IBV_SIZE_IBUFS_CHUNKS;
int  ibv_max_srq_ibufs    = IBV_MAX_SRQ_IBUFS;
int  ibv_srq_credit_limit = IBV_SRQ_CREDIT_LIMIT;
int  ibv_srq_credit_thread_limit = IBV_SRQ_CREDIT_THREAD_LIMIT;
int  ibv_rdvz_protocol    = IBV_RDVZ_READ_PROTOCOL;
int  ibv_no_memory_limitation    = IBV_NO_MEMORY_LIMITATION;
int  ibv_verbose_level    = IBV_VERBOSE_LEVEL;

int  ibv_wc_in_number     = IBV_WC_IN_NUMBER;
int  ibv_wc_out_number    = IBV_WC_OUT_NUMBER;
int  ibv_max_mr           = IBV_MAX_MR;
int  ibv_adm_port         = IBV_ADM_PORT;
int  ibv_rdma_depth       = IBV_RDMA_DEPTH;
int  ibv_rdma_dest_depth  = IBV_RDMA_DEST_DEPTH;


void sctk_net_ibv_config_check()
{

  if (ibv_eager_threshold < (RC_SR_HEADER_SIZE + sizeof(sctk_net_ibv_rc_rdma_request_t)))
  {
      goto error;
  }
  if (ibv_eager_threshold < (RC_SR_HEADER_SIZE + sizeof(sctk_net_ibv_rc_rdma_coll_request_t)))
  {
      goto error;
  }
  if (ibv_eager_threshold < (RC_SR_HEADER_SIZE + sizeof(sctk_net_ibv_rc_rdma_request_ack_t)))
  {
      goto error;
  }
  if (ibv_eager_threshold < (RC_SR_HEADER_SIZE + sizeof(sctk_net_ibv_rc_rdma_done_t)))
  {
      goto error;
  }
  return;

error:
  sctk_error("Wrong IB configuration");
  sctk_abort();
  return;
}

void sctk_net_ibv_config_print()
{
  if (sctk_process_rank == 0)
  {
    fprintf(stderr,
        "############# IB CONFIGURATION #############\n"
        "ibv_eager_threshold  = %d\n"
        "ibv_qp_tx_depth      = %d\n"
        "ibv_qp_rx_depth      = %d\n"
        "ibv_max_sg_sq        = %d\n"
        "ibv_max_sg_rq        = %d\n"
        "ibv_max_inline       = %d\n"
        "ibv_max_ibufs        = %d\n"
        "ibv_max_srq_ibufs    = %d\n"
        "ibv_srq_credit_limit = %d\n"
        "ibv_srq_credit_thread_limit = %d\n"
        "ibv_rdvz_protocol    = %d\n"
        "ibv_wc_in_number     = %d\n"
        "ibv_wc_out_number    = %d\n"
        "ibv_max_mr           = %d\n"
        "ibv_adm_port         = %d\n"
        "ibv_rdma_depth       = %d\n"
        "ibv_rdma_dest_depth  = %d\n"
        "############# IB CONFIGURATION #############\n",
        ibv_eager_threshold,
        ibv_qp_tx_depth,
        ibv_qp_rx_depth,
        ibv_max_sg_sq,
        ibv_max_sg_rq,
        ibv_max_inline,
        ibv_max_ibufs,
        ibv_max_srq_ibufs,
        ibv_srq_credit_limit,
        ibv_srq_credit_thread_limit,
        ibv_rdvz_protocol,
        ibv_wc_in_number,
        ibv_wc_out_number,
        ibv_max_mr,
        ibv_adm_port,
        ibv_rdma_depth,
        ibv_rdma_dest_depth);
  }
}

void sctk_net_ibv_config_init()
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

  if ( (value = getenv("MPC_IBV_SRQ_CREDIT_THREAD_LIMIT")) != NULL )
    ibv_srq_credit_thread_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_RDVZ_WRITE_PROTOCOL")) != NULL )
    ibv_rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
  if ( (value = getenv("MPC_IBV_RDVZ_READ_PROTOCOL")) != NULL )
    ibv_rdvz_protocol = IBV_RDVZ_READ_PROTOCOL;

  if ( (value = getenv("MPC_IBV_WC_IN_NUMBER")) != NULL )
    ibv_wc_in_number = atoi(value);

  if ( (value = getenv("MPC_IBV_WC_OUT_NUMBER")) != NULL )
    ibv_wc_out_number = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_MR")) != NULL )
    ibv_max_mr = atoi(value);

  if ( (value = getenv("MPC_IBV_ADM_PORT")) != NULL )
    ibv_adm_port = atoi(value);

  if ( (value = getenv("MPC_IBV_RDMA_DEPTH")) != NULL )
    ibv_rdma_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_RDMA_DEST_DEPTH")) != NULL )
    ibv_rdma_dest_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_TX_DEPTH")) != NULL )
    ibv_qp_tx_depth = atoi(value);

  /*
   * Check if the variables are well set and print them
   * */
  sctk_net_ibv_config_check();
  sctk_net_ibv_config_print();
}

