#ifndef LCR_IFACE_H
#define LCR_IFACE_H

#include <sys/uio.h>

#include "lcr_def.h"

// Endpoint functions
typedef ssize_t (*lcr_send_am_bcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
					uint8_t id,
					lcr_pack_callback_t pack,
					void *arg,
					unsigned flags);

typedef int (*lcr_send_am_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
					uint8_t id,
					const void *header,
					unsigned header_length,
					const struct iovec *iov,
					size_t iovcnt,
					unsigned flags, 
					lcr_completion_t *comp);

typedef ssize_t (*lcr_send_tag_bcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
					     lcr_tag_t tag,
					     uint64_t imm,
					     lcr_pack_callback_t pack,
					     void *arg,
					     unsigned flags, 
					     lcr_tag_context_t *ctx);

typedef int (*lcr_send_tag_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
					 lcr_tag_t tag,
					 uint64_t imm,
					 const struct iovec *iov,
					 size_t iovcnt,
					 unsigned flags,
					 lcr_tag_context_t *ctx);

typedef int (*lcr_recv_tag_zcopy_func_t)(sctk_rail_info_t *rail,
					 lcr_tag_t tag,
					 lcr_tag_t ign_tag,
					 const struct iovec *iov,
					 size_t iovcnt,
					 lcr_tag_context_t *ctx);

typedef int (*lcr_send_put_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                   uint64_t local_addr,
                                   uint64_t remote_addr,
                                   lcr_memp_t local_key,
                                   lcr_memp_t remote_key,
                                   size_t size,
                                   lcr_completion_t *comp);

typedef int (*lcr_send_get_funt_t)(_mpc_lowcomm_endpoint_t *ep,
                                   uint64_t local_addr,
                                   uint64_t remote_addr,
                                   lcr_memp_t local_key,
                                   lcr_memp_t remote_key,
                                   size_t size,
                                   lcr_completion_t *comp);

// Interface functions
typedef int (*lcr_iface_get_attr_func_t)(sctk_rail_info_t *rail,
                                         lcr_rail_attr_t *attr);

#endif
