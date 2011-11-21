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

#ifdef MPC_USE_INFINIBAND

#include "sctk_ib_config.h"
#define SCTK_IB_MODULE_NAME "CONFIG"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"

/*-----------------------------------------------------------
 *  CONSTS
 *----------------------------------------------------------*/

/* Maximum message size for each channels (in fact this in
 * not really a limit...). Here is the algorithm to
 * select which channel use:
 * if x < IBV_EAGER_LIMIT -> eager msg
 * if x < IBV_FRAG_EAGER_LIMIT -> frag msg (into several eager buffers)
 * if x > IBV_FRAG_EAGER_LIMIT -> rendezvous msg */
//#define IBV_EAGER_LIMIT ( 12 * 1024 )
#define IBV_EAGER_LIMIT       ( 12 * 1024)
#define IBV_FRAG_EAGER_LIMIT  ( 128 * 1024)
/* Number of allowed pending Work Queue Elements
 * for each QP */
#define IBV_QP_TX_DEPTH     15000
/* We don't need recv WQE when using SRQ.
 * This variable must be set to 0 */
#define IBV_QP_RX_DEPTH     0
/* Many CQE. In memory, it represents about
 * 1.22Mb for 40000 entries */
#define IBV_CQ_DEPTH        40000
#define IBV_MAX_SG_SQ       8
#define IBV_MAX_SG_RQ       8
#define IBV_MAX_INLINE      128

/* Maximum number of buffers to allocate during the
 * initialization step */
#define IBV_INIT_IBUFS         300

/* Maximum number of buffers which can be posted to the SRQ.
 * This number cannot be higher than than the number fixed by the HW.
 * The verification is done during the config_check function */
#define IBV_MAX_SRQ_IBUFS_POSTED     1000
/* When the async thread wakes, it means that the SRQ is full. We
 * allows the async thread to extract IBV_MAX_SRQ_WR_HANDLE_BY_THREAD messages
 * before posting new buffers .*/
#define IBV_MAX_SRQ_WR_HANDLE_BY_THREAD 50
/* Maximum number of buffers which can be used by SRQ. This number
 * is not fixed by the HW */
#define IBV_MAX_SRQ_IBUFS            1000
/* Minimum number of free recv buffer before
 * posting of new buffers. This thread is  activated
 * once a recv buffer is freed. If IBV_SRQ_CREDIT_LIMIT ==
 * IBV_MAX_SRQ_IBUFS_POSTED, receive buffers are re-post every-time
 * they are consumned */
#define IBV_SRQ_CREDIT_LIMIT  18000
//#define IBV_SRQ_CREDIT_LIMIT  10
/* Minimum number of free recv buffer before
 * the activation of the asynchronous
 * thread (if this thread is activated too much times,
 * the performance can be decreased) */
#define IBV_SRQ_CREDIT_THREAD_LIMIT  100

/* threshold before using dynamic allocation. For example, when
 * 80% of the SRQ buffers are busy, we make a copy of the message
 * before sending it to MPC. In the other case, MPC reads the message
 * directly from the network buffer */
#define IBV_DYNAMIC_ALLOCATION_THRESHOLD 0.80L

/* Number of new buffers allocated when
 * no more buffers are available */
#define IBV_SIZE_IBUFS_CHUNKS 1000

#define IBV_WC_IN_NUMBER    20
#define IBV_WC_OUT_NUMBER   20

/* Numer of MMU entries allocated during
 * the MPC initialization */
#define IBV_INIT_MR          400
/* Number of new MMU allocated when
 * no more MMU entries are available */
#define IBV_SIZE_MR_CHUNKS  200

#define IBV_ADM_PORT        1

#define IBV_RDMA_DEPTH       4
#define IBV_RDMA_DEST_DEPTH  4

#define IBV_NO_MEMORY_LIMITATION  1
/* Verbosity level (some infos can appears on
 * the terminal during runtime: new ibufs allocated,
 * new MMu entries allocated, etc...) */
#define IBV_VERBOSE_LEVEL         2

#define IBV_ADAPTIVE_POLLING      0
/* Define if the polling function must be run in a
 * secure mode. In this mode, only one task at the same
 * moment can make a call to the polling function*/
#define IBV_SECURE_POLLING        1

#define IBV_STEAL                 0
/*  0 -> MPC in normal mode, without work-stealing */
/*  1 -> MPC in collaborative-polling mode, without work-stealing */
/*  2 -> MPC in collaborative-polling mode, with work-stealing */

char* steal_names[3] = {
  "Normal mode",
  "Collaborative-polling w/o WS",
  "Collaborative-polling w/ WS"};

#define IBV_QUIET_CRASH           1
#define IBV_MATCH                 1

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_config_check(sctk_ib_rail_info_t *rail_ib)
{
  sctk_ib_config_t *c = rail_ib->config;

  /* FIXME: fix here */
#if 0
  c->device_attr = sctk_net_ibv_qp_get_dev_attr();

  /* If more wr for srq available than the hw maximum number */
  if (c->ibv_max_srq_ibufs_posted > c->device_attr->max_srq_wr) {
    if (!sctk_process_rank)
      sctk_warning("ibv_max_srq_ibufs_posted too high. Truncated to %d", c->device_attr->max_srq_wr);
    c->ibv_max_srq_ibufs_posted = c->device_attr->max_srq_wr;
  }
#endif

  /* Good conf, we return */
  return;

error:
  sctk_error("Wrong IB configuration");
  sctk_abort();
  return;
}

#define EXPERIMENTAL(str) #str" (experimental)"
void sctk_ib_config_print(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  if (sctk_process_rank == 0) {
    fprintf(stderr,
        "############# IB CONFIGURATION #############\n"
        "ibv_eager_limit      = %d\n"
        "ibv_frag_eager_limit = %d\n"
        "ibv_qp_tx_depth      = %d\n"
        "ibv_qp_rx_depth      = %d\n"
        "ibv_max_sg_sq        = %d\n"
        "ibv_max_sg_rq        = %d\n"
        "ibv_max_inline       = %d\n"
        "ibv_init_ibufs        = %d\n"
        "ibv_max_srq_ibufs_posted    = %d\n"
        "ibv_max_srq_ibufs    = %d\n"
        "ibv_size_ibufs_chunk = %d\n"
        "ibv_srq_credit_limit = %d\n"
        "ibv_srq_credit_thread_limit = %d\n"
        "ibv_rdvz_protocol    = %d\n"
        "ibv_wc_in_number     = %d\n"
        "ibv_wc_out_number    = %d\n"
        "ibv_init_mr           = %d\n"
        "ibv_adm_port         = %d\n"
        "ibv_rdma_depth       = %d\n"
        "ibv_rdma_dest_depth  = %d\n"
        "ibv_adaptive_polling = %d\n"
        "ibv_quiet_crash      = %d\n"
        "ibv_match            = %d\n"
        EXPERIMENTAL(ibv_secure_polling)"   = %d\n"
        EXPERIMENTAL(ibv_steal)"            = %d\n"
        "Stealing desc        = %s\n"
        "############# IB CONFIGURATION #############\n",
        config->ibv_eager_limit,
        config->ibv_frag_eager_limit,
        config->ibv_qp_tx_depth,
        config->ibv_qp_rx_depth,
        config->ibv_max_sg_sq,
        config->ibv_max_sg_rq,
        config->ibv_max_inline,
        config->ibv_init_ibufs,
        config->ibv_max_srq_ibufs_posted,
        config->ibv_max_srq_ibufs,
        config->ibv_size_ibufs_chunk,
        config->ibv_srq_credit_limit,
        config->ibv_srq_credit_thread_limit,
        config->ibv_rdvz_protocol,
        config->ibv_wc_in_number,
        config->ibv_wc_out_number,
        config->ibv_init_mr,
        config->ibv_adm_port,
        config->ibv_rdma_depth,
        config->ibv_rdma_dest_depth,
        config->ibv_adaptive_polling,
        config->ibv_quiet_crash,
        config->ibv_match,
        config->ibv_secure_polling,
        config->ibv_steal, steal_names[config->ibv_steal]);
  }
}

void load_ib_default_config(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);

  config->ibv_size_mr_chunk = IBV_SIZE_MR_CHUNKS;
  config->ibv_init_ibufs = IBV_INIT_IBUFS;

  config->ibv_eager_limit = IBV_EAGER_LIMIT;
  config->ibv_frag_eager_limit = IBV_FRAG_EAGER_LIMIT;
  config->ibv_qp_tx_depth = IBV_QP_TX_DEPTH;
  config->ibv_qp_rx_depth = IBV_QP_RX_DEPTH;
  config->ibv_cq_depth = IBV_CQ_DEPTH;
  config->ibv_max_sg_sq = IBV_MAX_SG_SQ;
  config->ibv_max_sg_rq = IBV_MAX_SG_RQ;
  config->ibv_max_inline = IBV_MAX_INLINE;
  config->ibv_max_srq_ibufs_posted = IBV_MAX_SRQ_IBUFS_POSTED;
  config->ibv_max_srq_ibufs = IBV_MAX_SRQ_IBUFS;
  config->ibv_srq_credit_limit = IBV_SRQ_CREDIT_LIMIT;
  config->ibv_srq_credit_thread_limit = IBV_SRQ_CREDIT_THREAD_LIMIT;
  config->ibv_max_srq_wr_handle_by_thread = IBV_MAX_SRQ_WR_HANDLE_BY_THREAD;
  config->ibv_size_ibufs_chunk = IBV_SIZE_IBUFS_CHUNKS;
  config->ibv_rdvz_protocol = IBV_RDVZ_READ_PROTOCOL;

  config->ibv_verbose_level = IBV_VERBOSE_LEVEL;
  config->ibv_wc_in_number = IBV_WC_IN_NUMBER;
  config->ibv_wc_out_number = IBV_WC_OUT_NUMBER;
  config->ibv_init_mr = IBV_INIT_MR;
  config->ibv_adm_port = IBV_ADM_PORT;
  config->ibv_rdma_depth = IBV_RDMA_DEPTH;
  config->ibv_rdma_dest_depth = IBV_RDMA_DEST_DEPTH;
  config->ibv_adaptive_polling = IBV_ADAPTIVE_POLLING;
  config->ibv_secure_polling = IBV_SECURE_POLLING;
  config->ibv_steal = IBV_STEAL;
  config->ibv_quiet_crash = IBV_QUIET_CRASH;
  config->ibv_match = IBV_MATCH;
}

/* Set IB configure with env variables */
void set_ib_env(sctk_ib_rail_info_t *rail_ib)
{
  char* value;
  sctk_ib_config_t* c = rail_ib->config;

  if ( (value = getenv("MPC_IBV_EAGER_LIMIT")) != NULL )
    c->ibv_eager_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_FRAG_EAGER_LIMIT")) != NULL )
    c->ibv_frag_eager_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_TX_DEPTH")) != NULL )
    c->ibv_qp_tx_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_RX_DEPTH")) != NULL )
    c->ibv_qp_rx_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_SG_SQ")) != NULL )
    c->ibv_max_sg_sq = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_RG_SQ")) != NULL )
    c->ibv_max_sg_rq = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_INLINE")) != NULL )
    c->ibv_max_inline = atoi(value);

  if ( (value = getenv("MPC_IBV_INIT_IBUFS")) != NULL )
    c->ibv_init_ibufs = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_SRQ_IBUFS_POSTED")) != NULL )
    c->ibv_max_srq_ibufs_posted = atoi(value);

  if ( (value = getenv("MPC_IBV_MAX_SRQ_IBUFS")) != NULL )
    c->ibv_max_srq_ibufs = atoi(value);

  if ( (value = getenv("MPC_IBV_SRQ_CREDIT_LIMIT")) != NULL )
    c->ibv_srq_credit_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_SRQ_CREDIT_THREAD_LIMIT")) != NULL )
    c->ibv_srq_credit_thread_limit = atoi(value);

  if ( (value = getenv("MPC_IBV_RDVZ_WRITE_PROTOCOL")) != NULL )
    c->ibv_rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
  if ( (value = getenv("MPC_IBV_RDVZ_READ_PROTOCOL")) != NULL )
    c->ibv_rdvz_protocol = IBV_RDVZ_READ_PROTOCOL;

  if ( (value = getenv("MPC_IBV_WC_IN_NUMBER")) != NULL )
    c->ibv_wc_in_number = atoi(value);

  if ( (value = getenv("MPC_IBV_WC_OUT_NUMBER")) != NULL )
    c->ibv_wc_out_number = atoi(value);

  if ( (value = getenv("MPC_IBV_INIT_MR")) != NULL )
    c->ibv_init_mr = atoi(value);

  if ( (value = getenv("MPC_IBV_ADM_PORT")) != NULL )
    c->ibv_adm_port = atoi(value);

  if ( (value = getenv("MPC_IBV_RDMA_DEPTH")) != NULL )
    c->ibv_rdma_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_RDMA_DEST_DEPTH")) != NULL )
    c->ibv_rdma_dest_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_QP_TX_DEPTH")) != NULL )
    c->ibv_qp_tx_depth = atoi(value);

  if ( (value = getenv("MPC_IBV_SIZE_IBUFS_CHUNK")) != NULL )
    c->ibv_size_ibufs_chunk = atoi(value);

  if ( (value = getenv("MPC_IBV_ADAPTIVE_POLLING")) != NULL )
    c->ibv_adaptive_polling = atoi(value);

  if ( (value = getenv("MPC_IBV_SECURE_POLLING")) != NULL )
    c->ibv_secure_polling = atoi(value);

  if ( (value = getenv("MPC_IBV_STEAL")) != NULL )
    c->ibv_steal = atoi(value);

  if ( (value = getenv("MPC_IBV_QUIET_CRASH")) != NULL )
    c->ibv_quiet_crash = atoi(value);

  if ( (value = getenv("MPC_IBV_MATCH")) != NULL )
    c->ibv_match = atoi(value);
}

void sctk_ib_config_init(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);

  config = sctk_malloc(sizeof(sctk_ib_config_t));
  assume(config);
  memset(config, 0, sizeof(sctk_ib_config_t));
  rail_ib->config = config;

  load_ib_default_config(rail_ib);
  set_ib_env(rail_ib);

  /*
   * Check if the variables are well set
   * */
  sctk_ib_config_check(rail_ib);
}


#if 0

#endif

#endif
