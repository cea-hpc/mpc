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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_LOWCOMM_OFI_MSG_TYPES_H_
#define __MPC_LOWCOMM_OFI_MSG_TYPES_H_

#include "ofi_headers.h"
#include <mpc_common_spinlock.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define MPC_LOWCOMM_OFI_PASSIVE_SIDE (0x0)
#define MPC_LOWCOMM_OFI_ACTIVE_SIDE (0x1)

/**
 * @brief Context for OFI-MSG with RDMA operations
 * 
 */
typedef struct mpc_lowcomm_ofi_rma_ctx_msg_s
{
} mpc_lowcomm_ofi_rma_ctx_msg_t;

/**
 * @brief Context for OFI-MSG associated with each created endpoint
 * 
 */
typedef struct mpc_lowcomm_ofi_msg_endpoint_s
{
	size_t sz_buf; /**< buffer where posted recv() will store the size of the next message */
} mpc_lowcomm_ofi_msg_endpoint_t;

/**
 * @brief OFI-MSG specific data
 * 
 */
typedef struct mpc_lowcomm_ofi_rail_spec_msg_s
{
	struct fid_pep* pep;                /**< the passive endpoint */
	struct fid_eq* cm_eq;               /**< the event-queue, shared by all endpoints */
	struct fid_poll* cq_send_pollset;   /**< the pollset attached to send() CQ */
	struct fid_poll* cq_recv_pollset;   /**< the pollset attached to recv() CQ */
	char padding[64-(4*sizeof(void*))]; /**< padding to have the lock on a different cacheline */
	mpc_common_rwlock_t pollset_lock;    /**< pollsets lock */
} mpc_lowcomm_ofi_rail_spec_msg_t;

#ifdef __cplusplus
}
#endif

#endif
