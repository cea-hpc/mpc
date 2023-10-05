/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

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
