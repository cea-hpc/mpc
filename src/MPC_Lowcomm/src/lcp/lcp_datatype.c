#include "lcp_def.h"
#include "lcp.h"
#include "lcp_context.h"
#include "lcp_request.h"

#include <stdio.h>
#include <string.h>

int lcp_datatype_pack(lcp_context_h ctx, lcp_request_t *req, 
                      lcp_datatype_t datatype, void *dest, 
                      const void *src, size_t length)
{
        size_t packed_len = 0;
        if (!length) {
                return length;
        }

        switch (datatype) {
        case LCP_DATATYPE_CONTIGUOUS:
                assert(src);
                memcpy(dest, src, length); 
                packed_len = length;
                break;
        case LCP_DATATYPE_DERIVED:
                ctx->dt_ops.pack(req->request, dest);
                //FIXME: verify packed length
                packed_len = length;
                break;
        default:
                mpc_common_debug_error("LCP: unknown datatype");
                packed_len = -1;
                break;
        }
        
        return packed_len;
}

//FIXME: Ambiguity because for DERIVED, dest argument could have any value set
//       by the calling function which would not have any impact.
//       This is not readable...
int lcp_datatype_unpack(lcp_context_h ctx, lcp_request_t *req, 
                        lcp_datatype_t datatype, void *dest, 
                        const void *src, size_t length)
{
        size_t unpacked_len = 0;
        if (!length) {
                return length;
        }

        switch (datatype) {
        case LCP_DATATYPE_CONTIGUOUS:
               memcpy(dest, src, length); 
               unpacked_len = length;
               break;
        case LCP_DATATYPE_DERIVED:
               ctx->dt_ops.unpack(req->request, (void *)src);
               unpacked_len = length;
               break;
        default:
               mpc_common_debug_error("LCP: unknown datatype");
               unpacked_len = -1;
               break;
        }
        
        return unpacked_len;
}
