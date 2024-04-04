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

#ifndef LCP_TAG_MATCH_H
#define LCP_TAG_MATCH_H

#include "lcp_request.h" //FIXME: find a way to remove this dependency.
#include "lcp_header.h"

/* NOLINTBEGIN(clang-diagnostic-unused-function) */
static inline lcp_tag_hdr_t * lcp_ctnr_get_tag(lcp_unexp_ctnr_t *ctnr) {
        return (lcp_tag_hdr_t *)(ctnr + 1);
}
/* NOLINTEND(clang-diagnostic-unused-function) */

void *lcp_match_prqueue(mpc_queue_head_t *prqs, uint16_t comm_id, int32_t tag, int32_t src);
void lcp_append_prqueue(mpc_queue_head_t *prqs, mpc_queue_elem_t *elem, uint16_t comm_id);
void *lcp_match_umqueue(mpc_queue_head_t *umqs,
                        uint16_t comm_id, int32_t tag, int32_t tmask,
                        int32_t src, int32_t smask);
void lcp_append_umqueue(mpc_queue_head_t *umqs, mpc_queue_elem_t *elem, uint16_t comm_id);
void *lcp_search_umqueue(mpc_queue_head_t *umqs,
                         uint16_t comm_id, int32_t tag, int32_t tmask,
                         int32_t src, int32_t smask);

#endif
