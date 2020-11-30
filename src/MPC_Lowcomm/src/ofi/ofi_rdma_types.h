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

#ifndef __MPC_LOWCOMM_OFI_RDMA_TYPES_H_
#define __MPC_LOWCOMM_OFI_RDMA_TYPES_H_

#include "ofi_headers.h"
#include "ofi_types.h"

#define MPC_LOWCOMM_OFI_RDMA_KEY (0x01)
#define MPC_LOWCOMM_OFI_RDV_KEY (0x10)

#ifdef __cplusplus
extern "C"
{
#endif
#define MPC_LOWCOMM_OFI_REQ_NB 10
#define MPC_LOWCOMM_OFI_EAGER_SIZE (4096)
#define MPC_LOWCOMM_OFI_RDV_SIZE (sizeof(mpc_lowcomm_ofi_rdv_key_t) + sizeof(mpc_lowcomm_ofi_rdv_addr_t))
#define MPC_LOWCOMM_OFI_MSG_HDR 128 /* this should be sizeof(mpc_lowcomm_ptp_message_body_t) */
#define MPC_LOWCOMM_OFI_TYPE_STR(u) (ofi_req_type_decoder[u])

extern char* ofi_req_type_decoder[];
typedef uint64_t mpc_lowcomm_ofi_rdv_key_t;
typedef uint64_t mpc_lowcomm_ofi_rdv_addr_t;

typedef struct mpc_lowcomm_ofi_rdma_cell_eager_s
{
	char hdr[MPC_LOWCOMM_OFI_MSG_HDR];
	char buffer[MPC_LOWCOMM_OFI_EAGER_SIZE];
} mpc_lowcomm_ofi_rdma_cell_eager_t;

typedef struct mpc_lowcomm_ofi_rdma_cell_rdv_s
{
	char hdr[MPC_LOWCOMM_OFI_MSG_HDR];
	char buffer[MPC_LOWCOMM_OFI_RDV_SIZE];
} mpc_lowcomm_ofi_rdma_cell_rdv_t;

typedef struct mpc_lowcomm_ofi_rdma_cell_custom_s
{
	void* opaque;
} mpc_lowcomm_ofi_rdma_cell_custom_t;

typedef union mpc_lowcomm_ofi_rdma_cell_spec_u
{
	struct mpc_lowcomm_ofi_rdma_cell_eager_s eager;
	struct mpc_lowcomm_ofi_rdma_cell_rdv_s rdv;
	struct mpc_lowcomm_ofi_rdma_cell_custom_s custom;
} mpc_lowcomm_ofi_rdma_cell_spec_t;

typedef struct mpc_lowcomm_ofi_rma_ctx_rdma_s
{

} mpc_lowcomm_ofi_rma_ctx_rdma_t;

/**
 * @brief request types.
 * 
 * To keep synced with stringify struct in mpc_lowcomm_ofi_ib_rdma.c
 */
typedef enum mpc_lowcomm_ofi_req_type_e 
{
	MPC_LOWCOMM_OFI_TYPE_EAGER,
	MPC_LOWCOMM_OFI_TYPE_RDV_READY,
	MPC_LOWCOMM_OFI_TYPE_RDV_READ,
	MPC_LOWCOMM_OFI_TYPE_RDV_ACK,
	MPC_LOWCOMM_OFI_TYPE_RDMA,
	MPC_LOWCOMM_OFI_TYPE_NB
} mpc_lowcomm_ofi_ctx_type_t;

typedef struct mpc_lowcomm_ofi_rdma_ctx_req_s
{
	union mpc_lowcomm_ofi_rdma_cell_spec_u data;
	enum mpc_lowcomm_ofi_req_type_e type;
} mpc_lowcomm_ofi_rdma_ctx_req_t;

typedef struct mpc_lowcomm_ofi_rdma_endpoint_s
{
	fi_addr_t av_root;
} mpc_lowcomm_ofi_rdma_endpoint_t;

typedef struct mpc_lowcomm_ofi_rail_spec_rdma_s {
	struct fid_av* av;
	struct fid_ep *sep;
	struct fid_cq *cq_recv;
	struct fid_cq *cq_send;
} mpc_lowcomm_ofi_rail_spec_rdma_t;

#ifdef __cplusplus
}
#endif

#endif
