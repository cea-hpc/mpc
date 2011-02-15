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

#ifndef __SCTK__INFINIBAND_CONST_H_
#define __SCTK__INFINIBAND_CONST_H_

typedef enum
{
  IBV_CHAN_RC_SR    = 1 << 0,
  IBV_CHAN_RC_RDMA  = 1 << 1,
  IBV_CHAN_SEND  = 1 << 2,
  IBV_CHAN_RECV  = 1 << 3,
} sctk_net_ibv_allocator_type_t;

/* source of the message. Simpluy the destruction
 * of the message */
#define IBV_RC_SR_ORIGIN        0
#define IBV_RC_RDMA_ORIGIN      1
#define IBV_POLL_RC_SR_ORIGIN   2
#define IBV_POLL_RC_RDMA_ORIGIN 3

/*-----------------------------------------------------------
 *  MMU
 *----------------------------------------------------------*/
/* nb entries in buffers */
#define IBV_PENDING_SEND_PTP  60
#define IBV_PENDING_SEND_COLL 30
#define IBV_PENDING_RECV      60

/* define when the garbage collector has to be
 * run: 80 -> 80% of the number of buffers */
#define CEILING_SEND_BUFFERS  90

/* Threshould for eager messages */
#define SCTK_EAGER_THRESHOLD ( (128 * 1024) + sizeof(sctk_thread_ptp_message_t) )

/* max number of WC extracted for the input
 * and the output completion queue */
/* FIXME cant change 1 to something else */
#define SCTK_PENDING_IN_NUMBER 1
#define SCTK_PENDING_OUT_NUMBER 10

/*-----------------------------------------------------------
 *  MMU
 *----------------------------------------------------------*/
/* number of max number of memory entries pinned */
#define SCTK_MAX_MR_ALLOWED 20000

/*-----------------------------------------------------------
 *  QPs
 *----------------------------------------------------------*/
#define IB_TX_DEPTH 1000
#define IB_RX_DEPTH 1000
#define IB_MAX_SG_SQ 4
#define IB_MAX_SG_RQ 4
#define IB_MAX_INLINE 128
/* maximum number of resources for incoming RDMA requests */
#define IB_RMDA_DEPTH 4
#define IB_RMDA_DEST_DEPTH 1
/* physical port number to use */
#define IBV_ADM_PORT 1

/*-----------------------------------------------------------
 *  DEBUG
 *----------------------------------------------------------*/
#define DBG_S(x) if (x) \
  sctk_nodebug("BEGIN %s", __func__); \
else \
sctk_nodebug("");

#define DBG_E(x) if (x) \
  sctk_nodebug("END %s", __func__); \
else \
sctk_nodebug("");

#endif
