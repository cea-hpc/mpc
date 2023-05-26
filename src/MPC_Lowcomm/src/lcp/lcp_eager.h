#ifndef LCP_EAGER_H
#define LCP_EAGER_H

#include "lcp_def.h"
#include "lcr/lcr_def.h"
#include "lcp_types.h"

int lcp_send_eager_bcopy(lcp_request_t *req, 
                         lcr_pack_callback_t pack,
                         lcr_completion_t *comp,
                         uint8_t am_id);

int lcp_send_eager_zcopy(lcp_request_t *req, uint8_t am_id,
                         void *hdr, size_t hdr_size,
                         struct iovec *iov, size_t iovcnt,
                         lcr_completion_t *comp);

#endif
