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

/*-----------------------------------------------------------
 *  CONSTS
 *----------------------------------------------------------*/

/* Maximum message size for each channels (in fact this in
 * not really a limit...). Here is the algorithm to
 * select which channel use:
 * if x < IBV_EAGER_LIMIT -> eager msg
 * if x < IBV_FRAG_EAGER_LIMIT -> frag msg (into several eager buffers)
 * if x > IBV_FRAG_EAGER_LIMIT -> rendezvous msg */
/* !!! WARNING !!! : diminishing IBV_EAGER_LIMIT me cause bad performance
 * on buffered eager messages */
#define IBV_EAGER_LIMIT       ( 1 * 1024)
#define IBV_EAGER_RDMA_LIMIT  ( 0 )
#define IBV_FRAG_EAGER_LIMIT  ( 256 * 1024)

/* Number of allowed pending Work Queue Elements
 * for each QP */
#define IBV_QP_TX_DEPTH     15000
/* We don't need recv WQE when using SRQ.
 * This variable must be set to 0 */
#define IBV_QP_RX_DEPTH     0
/* Many CQE. In memory, it represents about
 * 1.22Mb for 40000 entries */
#define IBV_CQ_DEPTH        10000
#define IBV_MAX_SG_SQ       8
#define IBV_MAX_SG_RQ       8
#define IBV_MAX_INLINE      128

/* Number of RDMA buffers allocated for each neighbor.
 * i.e: if IBV_MAX_RDMA_IBUFS = 256:
 * The total memory used is: 2 (1 for send and 1 for receive) * 256 buffers * IBV_EAGER_RDMA_LIMIT */
#define IBV_MAX_RDMA_IBUFS  0
#define IBV_MAX_RDMA_CONNECTIONS 0

/* Maximum number of buffers to allocate during the
 * initialization step */
#define IBV_INIT_IBUFS          2000

/* Maximum number of buffers which can be posted to the SRQ.
 * This number cannot be higher than than the number fixed by the HW.
 * The verification is done during the config_check function */
#define IBV_MAX_SRQ_IBUFS_POSTED     2000
/* When the async thread wakes, it means that the SRQ is full. We
 * allows the async thread to extract IBV_MAX_SRQ_WR_HANDLE_BY_THREAD messages
 * before posting new buffers .*/
#define IBV_MAX_SRQ_WR_HANDLE_BY_THREAD 0
/* Maximum number of buffers which can be used by SRQ. This number
 * is not fixed by the HW */
#define IBV_MAX_SRQ_IBUFS            2000
//#define IBV_MAX_SRQ_IBUFS            50
/* Minimum number of free recv buffer before
 * posting of new buffers. This thread is  activated
 * once a recv buffer is freed. If IBV_SRQ_CREDIT_LIMIT ==
 * IBV_MAX_SRQ_IBUFS_POSTED, receive buffers are re-post every-time
 * they are consumned */
#define IBV_SRQ_CREDIT_LIMIT  2000
/* Minimum number of free recv buffer before
 * the activation of the asynchronous
 * thread (if this thread is activated too much times,
 * the performance can be decreased) */
#define IBV_SRQ_CREDIT_THREAD_LIMIT  10

/* threshold before using dynamic allocation. For example, when
 * 80% of the SRQ buffers are busy, we make a copy of the message
 * before sending it to MPC. In the other case, MPC reads the message
 * directly from the network buffer.
 * FIXME: not used now */
#define IBV_DYNAMIC_ALLOCATION_THRESHOLD 0.80L

/* Number of new buffers allocated when
 * no more buffers are available */
#define IBV_SIZE_IBUFS_CHUNKS 100

#define IBV_WC_IN_NUMBER    4
#define IBV_WC_OUT_NUMBER   4

/* Number of MMU entries allocated during
 * the MPC initialization */
#define IBV_INIT_MR          10
/* Number of new MMU allocated when
 * no more MMU entries are available.
 * You must use this option at your own risks! */
#define IBV_SIZE_MR_CHUNKS  10
#define IBV_MMU_CACHE_ENABLED 0
#define IBV_MMU_CACHE_ENTRIES 0

#define IBV_ADM_PORT        1

#define IBV_RDMA_DEPTH       4
#define IBV_RDMA_DEST_DEPTH  4

#define IBV_LOW_MEMORY 0
/* Verbosity level (some infos can appears on
 * the terminal during runtime: new ibufs allocated,
 * new MMU entries allocated, etc...) */
#define IBV_VERBOSE_LEVEL         2

#define IBV_ADAPTIVE_POLLING      0

#define IBV_STEAL                 0
/*  0 -> MPC in normal mode, without work-stealing */
/*  1 -> MPC in collaborative-polling mode, without work-stealing */
/*  2 -> MPC in collaborative-polling mode, with work-stealing */

#define IBV_QUIET_CRASH           0

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
static void load_ib_fallback_default_config(sctk_ib_rail_info_t *rail_ib)
{
  LOAD_CONFIG(rail_ib);

  config->ibv_size_mr_chunk = IBV_SIZE_MR_CHUNKS;
  config->ibv_init_ibufs = IBV_INIT_IBUFS;

  config->ibv_eager_limit       = ALIGN_ON_64 (IBV_EAGER_LIMIT + IBUF_GET_EAGER_SIZE);
  config->ibv_eager_rdma_limit  = ALIGN_ON_64 (IBV_EAGER_RDMA_LIMIT + IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE);
  config->ibv_frag_eager_limit  = (IBV_FRAG_EAGER_LIMIT + sizeof(sctk_thread_ptp_message_body_t));

  config->ibv_max_rdma_ibufs  = IBV_MAX_RDMA_IBUFS;
  config->ibv_max_rdma_connections  = IBV_MAX_RDMA_CONNECTIONS;
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
  config->ibv_mmu_cache_enabled = IBV_MMU_CACHE_ENABLED;
  config->ibv_mmu_cache_entries = IBV_MMU_CACHE_ENTRIES;
  config->ibv_adm_port = IBV_ADM_PORT;
  config->ibv_rdma_depth = IBV_RDMA_DEPTH;
  config->ibv_rdma_dest_depth = IBV_RDMA_DEST_DEPTH;
  config->ibv_adaptive_polling = IBV_ADAPTIVE_POLLING;
  config->ibv_steal = IBV_STEAL;
  config->ibv_low_memory = IBV_LOW_MEMORY;
  config->ibv_quiet_crash = IBV_QUIET_CRASH;
}

void sctk_ib_fallback_config_init(sctk_ib_rail_info_t *rail_ib, char* network_name)
{
  LOAD_CONFIG(rail_ib);

  config = sctk_malloc(sizeof(sctk_ib_config_t));
  assume(config);
  memset(config, 0, sizeof(sctk_ib_config_t));
  rail_ib->config = config;
  rail_ib->config->network_name = strdup(network_name);

  load_ib_fallback_default_config(rail_ib);
}

#endif
