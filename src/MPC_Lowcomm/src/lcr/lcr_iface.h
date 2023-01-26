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
                                             unsigned flags);

typedef int (*lcr_send_tag_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                         lcr_tag_t tag,
                                         uint64_t imm,
                                         const struct iovec *iov,
                                         size_t iovcnt,
                                         unsigned flags,
                                         lcr_completion_t *ctx);

typedef int (*lcr_post_tag_zcopy_func_t)(sctk_rail_info_t *rail,
                                         lcr_tag_t tag,
                                         lcr_tag_t ign_tag,
                                         const struct iovec *iov,
                                         size_t iovcnt,
                                         unsigned flags,
                                         lcr_tag_context_t *ctx);

typedef int (*lcr_unpost_tag_zcopy_func_t)(sctk_rail_info_t *rail,
                                           lcr_tag_t tag);

typedef int (*lcr_put_bcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                    lcr_pack_callback_t pack,
                                    void *arg,
                                    uint64_t remote_addr,
                                    lcr_memp_t *remote_key);

typedef int (*lcr_put_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                    uint64_t local_addr,
                                    uint64_t remote_addr,
                                    lcr_memp_t *remote_key,
                                    size_t size,
                                    lcr_completion_t *ctx);

typedef int (*lcr_get_bcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                    lcr_unpack_callback_t unpack,
                                    void *arg, size_t size,
                                    uint64_t remote_addr,
                                    lcr_memp_t *remote_key,
                                    lcr_completion_t *ctx);

typedef int (*lcr_get_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                    uint64_t local_addr,
                                    uint64_t remote_addr,
                                    lcr_memp_t *remote_key,
                                    size_t size,
                                    lcr_completion_t *ctx);

typedef int (*lcr_get_tag_zcopy_func_t)(_mpc_lowcomm_endpoint_t *ep,
                                        lcr_tag_t tag,
                                        uint64_t local_offset,
                                        uint64_t remote_offset,
                                        size_t size,
                                        lcr_completion_t *ctx);

// Interface functions
typedef int (*lcr_iface_get_attr_func_t)(sctk_rail_info_t *rail,
                                         lcr_rail_attr_t *attr);
typedef int (*lcr_iface_progress_func_t)(sctk_rail_info_t *rail);

typedef int (*lcr_iface_pack_memp_func_t)(sctk_rail_info_t *rail,
                                          lcr_memp_t *memp, 
                                          void *dest);

typedef int (*lcr_iface_unpack_memp_func_t)(sctk_rail_info_t *rail,
                                            lcr_memp_t *memp, 
                                            void *dest);

#endif
