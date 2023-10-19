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

#ifndef LCP_TAG_H
#define LCP_TAG_H

#include "lcp_def.h"
#include "lcp_header.h"

#include <stdint.h>
#include <mpc_math.h>

struct lcp_tag_data {
        size_t length;
        int tag;
        int32_t src_tid;
        int32_t dest_tid;
        uint16_t comm;
        uint64_t seqn;
};
//FIXME: move tag header over here.

//NOTE: choose the maximum size of all header. Optimal would be to check if
//      request is sync etc... But did not want to add this logic.
static inline size_t lcp_send_get_total_tag_payload(size_t data_length)
{
        size_t hdr_size = MPC_MAX(sizeof(lcp_tag_hdr_t), 
                                  MPC_MAX(sizeof(lcp_tag_sync_hdr_t),
                                          sizeof(lcp_rndv_hdr_t)));
        return data_length + hdr_size;
}

int lcp_send_eager_sync_ack(lcp_request_t *super, void *data);
int lcp_send_task_tag_zcopy(lcp_request_t *req);
int lcp_send_task_tag_bcopy(lcp_request_t *req);
int lcp_send_eager_tag_zcopy(lcp_request_t *req);
int lcp_send_eager_tag_bcopy(lcp_request_t *req);
int lcp_send_rndv_tag_start(lcp_request_t *req);

int lcp_recv_eager_tag_data(lcp_request_t *req, struct lcp_tag_data *tag_data, 
                            void *data, size_t length);
void lcp_recv_rndv_tag_data(lcp_request_t *req, void *data);

#endif
