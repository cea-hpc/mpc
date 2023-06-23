#ifndef MPC_OFI_H
#define MPC_OFI_H

#include "mpc_ofi_context.h"
#include <stdint.h>

typedef struct _mpc_lowcomm_ofi_rail_info_s
{
   struct mpc_ofi_context_t ctx;
   struct mpc_ofi_view_t view;
}_mpc_lowcomm_ofi_rail_info_t;


typedef struct lcr_ofi_am_hdr {
	uint8_t am_id;
	size_t length;
   char * data[0];
} lcr_ofi_am_hdr_t;





#endif /* MPC_OFI_H */