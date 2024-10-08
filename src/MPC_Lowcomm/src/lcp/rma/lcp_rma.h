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

#ifndef LCP_ATOMIC_H
#define LCP_ATOMIC_H

#include <core/lcp_request.h>

enum {
        LCP_ATO_PROTO_RMA,
        LCP_ATO_PROTO_SW,
};

typedef struct lcp_atomic_proto {
        lcp_send_func_t send_fetch;
        lcp_send_func_t send_cswap;
        lcp_send_func_t send_post;
} lcp_atomic_proto_t;

// NOLINTBEGIN(clang-diagnostic-unused-function)
static inline const char *lcp_ato_sw_decode_op(lcp_atomic_op_t op_type) {
        switch (op_type) {
        case LCP_ATOMIC_OP_ADD: return "LCP_ATOMIC_OP_ADD"; break;
        case LCP_ATOMIC_OP_OR: return "LCP_ATOMIC_OP_OR"; break;
        case LCP_ATOMIC_OP_XOR: return "LCP_ATOMIC_OP_XOR"; break;
        case LCP_ATOMIC_OP_AND: return "LCP_ATOMIC_OP_AND"; break;
        case LCP_ATOMIC_OP_SWAP: return "LCP_ATOMIC_OP_SWAP"; break;
        case LCP_ATOMIC_OP_CSWAP: return "LCP_ATOMIC_OP_CSWAP"; break;
        default: return "Unknwon LCP atomic op."; break;
        }

        return NULL;
}
// NOLINTEND(clang-diagnostic-unused-function)

extern lcp_atomic_proto_t ato_sw_proto;
extern lcp_atomic_proto_t ato_rma_proto;

#endif
