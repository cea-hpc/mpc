#ifndef MPC_OFI_H
#define MPC_OFI_H

#include <stdint.h>

#include "mpc_ofi_context.h"
#include <mpc_mempool.h>

typedef struct _mpc_lowcomm_ofi_rail_info_s
{
   struct mpc_ofi_context_t ctx;
   struct mpc_ofi_view_t view;
}_mpc_lowcomm_ofi_rail_info_t;

#define MPC_OFI_EP_MEMPOOL_MIN 10
#define MPC_OFI_EP_MEMPOOL_MAX 100

struct mpc_ofi_domain_buffer_manager_t;
typedef struct _mpc_lowcomm_ofi_endpoint_info_s
{
   mpc_mempool bsend;
   mpc_mempool deffered;
}_mpc_lowcomm_ofi_endpoint_info_t;


typedef struct lcr_ofi_am_hdr {
	uint8_t am_id;
	size_t length;
   char * data[0];
} lcr_ofi_am_hdr_t;

struct mpc_ofi_shared_pinning_context
{
	uint64_t ofi_remote_mr_key;
   void * addr;
   size_t size;
};

struct mpc_ofi_pinning_context
{
	struct fid_mr *ofi;
   struct mpc_ofi_shared_pinning_context shared;
};



#endif /* MPC_OFI_H */