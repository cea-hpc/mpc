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

#ifndef __MPC_LOWCOMM_OFI_TYPES_H_
#define __MPC_LOWCOMM_OFI_TYPES_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "ofi_headers.h"
#include "ofi_msg_types.h"
#include "ofi_rdma_types.h"

#ifndef NDEBUG
#define MPC_LOWCOMM_OFI_CHECK_RC(u) do { \
    int ___check_ret = u; \
    if(___check_ret < 0) { \
        switch(___check_ret) { \
            case -FI_ECANCELED: \
            case -FI_EAGAIN: \
	    case -FI_ENODATA: \
                break; \
            default: \
                mpc_common_debug_fatal("OFI: '%s' (code=%d) in func %s()", fi_strerror(-___check_ret), ___check_ret,  __FUNCTION__); \
        }; \
    } \
 } while(0)
#else
#define MPC_LOWCOMM_OFI_CHECK_RC(u) u
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/**
 * @brief Content of OFI-specific tail messages
 * 
 */
typedef struct mpc_lowcomm_ofi_tail_s
{
	int rdv_complete;
	int padding;
	struct mpc_lowcomm_ofi_rail_info_s* ref_rail;
	fi_addr_t remote;
	uint64_t mr_key;
	uint64_t addr;
} mpc_lowcomm_ofi_tail_t;

/**
 * @brief specialization of what a RDMA-specific context is.
 * 
 */
typedef union mpc_lowcomm_ofi_rma_ctx_spec_u
{
	struct mpc_lowcomm_ofi_rma_ctx_msg_s msg;  /**< for OFI-MSG driver */
	struct mpc_lowcomm_ofi_rdma_ctx_req_s rdma; /**< for OFI-RDMA driver */
} mpc_lowcomm_ofi_rma_ctx_spec_t;

/**
 * @brief RDMA-oriented generic information (any impl. can access these fields)
 * 
 */
typedef struct mpc_lowcomm_ofi_rma_ctx
{
	struct fid_mr *mr;                   /**< the registered memory region */
	void *start;                         /**< the base addressed used */
	size_t size;                         /**< memory region size */
	uint64_t key;                        /**< memory region key (OFI-determined) */
	union mpc_lowcomm_ofi_rma_ctx_spec_u spec; /**< specific information from the driver registering the region */
} mpc_lowcomm_ofi_rdma_ctx_t;

/**
 * @brief Specialization of what a endpoint context is
 * 
 */
typedef union mpc_lowcomm_ofi_ep_spec_u
{
	struct mpc_lowcomm_ofi_msg_endpoint_s msg;   /**< for OFI-MSG driver */
	struct mpc_lowcomm_ofi_rdma_endpoint_s rdma; /**< for OFI-RDMA driver */
} mpc_lowcomm_ofi_ep_spec_t;

/**
 * @brief TWO-SIDED oriented generic information (any impl. can access these fields)
 * 
 */
typedef struct mpc_lowcomm_ofi_ep_s
{
	struct fid_ep* ep;                 /**< the OFI endpoint */
	struct fid_cq* cq_r;               /**< the recv completion queue */
	struct fid_cq* cq_s;               /**< the send completion queue */
	void* sctk_ep;                     /**< a (dirty) pointer to the MPC endpoint (opaque becaure header inclusion issue) */
	union mpc_lowcomm_ofi_ep_spec_u spec; /**< specific information from the driver owning the endpoint */
	mpc_common_spinlock_t lock;              /* endpoint lock */
} mpc_lowcomm_ofi_ep_t;

/**
 * @brief Specialization fo what a OFI rail is
 * 
 */
typedef union mpc_lowcomm_ofi_rail_spec_u
{
	struct mpc_lowcomm_ofi_rail_spec_msg_s msg;   /**< for OFI-MSG driver */
	struct mpc_lowcomm_ofi_rail_spec_rdma_s rdma; /**< for OFI-RDMA driver */
} mpc_lowcomm_ofi_rail_spec_t;

/**
 * @brief Structure stored into an MPC endpoint.
 * This ctx object is opaque to let each implementation decides what's in there...
 * 
 */
typedef struct mpc_lowcomm_ofi_route_info_s
{
	void* ctx; /**< an opaque pointer, impl. determined */
} _mpc_lowcomm_endpoint_info_ofi_t;

/**
 * @brief Generic information describing an OFI driver
 */
typedef struct mpc_lowcomm_ofi_rail_info_s
{
	struct fi_info* provider;        /**< the provider */
	struct fid_domain* domain;       /**< the domain */
	struct fid_fabric* fabric;       /**< the fabric */
	union mpc_lowcomm_ofi_rail_spec_u spec; /**< specific information from the driver instanciated for this rail */
	char name_info[FI_NAME_MAX];     /**< ID string */
    	size_t name_info_sz;             /**< ID string size */
} mpc_lowcomm_ofi_rail_info_t;

#ifdef __cplusplus
}
#endif

#endif
