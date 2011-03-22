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
  IBV_CHAN_SEND     = 1 << 2,
  IBV_CHAN_RECV     = 1 << 3,
  IBV_CHAN_RC_SR_FRAG  = 1 << 4,
} sctk_net_ibv_allocator_type_t;

/* source of the message. Simpluy the destruction
 * of the message */
#define IBV_RC_SR_ORIGIN        0
#define IBV_RC_RDMA_ORIGIN      1
#define IBV_RC_SR_FRAG_ORIGIN        2
#define IBV_POLL_RC_SR_ORIGIN   3
#define IBV_POLL_RC_RDMA_ORIGIN 4
#define IBV_POLL_RC_SR_FRAG_ORIGIN   5

/*-----------------------------------------------------------
 *  MMU
 *----------------------------------------------------------*/

/* max number of WC extracted for the input
 * and the output completion queue */
/* FIXME cant change 1 to something else */
#define SCTK_PENDING_IN_NUMBER 100
#define SCTK_PENDING_OUT_NUMBER 100

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
