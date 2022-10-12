#ifndef LCR_PTL_H
#define LCR_PTL_H

#include <lcr/lcr_def.h>

#include "sctk_rail.h"

ssize_t lcr_ptl_send_tag_bcopy(_mpc_lowcomm_endpoint_t *ep,
			      lcr_tag_t tag,
			      uint64_t imm,
			      lcr_pack_callback_t pack,
			      void *arg,
			      unsigned cflags,
			      lcr_tag_context_t *ctx);

int lcr_ptl_send_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
			   lcr_tag_t tag,
			   uint64_t imm,
			   const struct iovec *iov,
			   size_t iovcnt,
			   unsigned cflags,
			   lcr_tag_context_t *ctx);

int lcr_ptl_recv_tag_zcopy(sctk_rail_info_t *rail,
			   lcr_tag_t tag, lcr_tag_t ign_tag,
			   const struct iovec *iov, 
			   size_t iovcnt, /* only one iov supported */
			   lcr_tag_context_t *ctx); 

#endif
