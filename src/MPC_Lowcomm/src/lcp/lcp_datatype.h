#ifndef LCP_DATATYPE_H
#define LCP_DATATYPE_H

#include "lcp_def.h"
#include <stdlib.h>

int lcp_datatype_pack(lcp_context_h ctx, lcp_request_t *req, 
                      lcp_datatype_t datatype, void *dest, 
                      const void *src, size_t length);

int lcp_datatype_unpack(lcp_context_h ctx, lcp_request_t *req, 
                        lcp_datatype_t datatype, void *dest, 
                        const void *src, size_t length);

#endif 
