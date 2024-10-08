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

#ifndef LCP_EP_H
#define LCP_EP_H

#include <lcp_def.h>
#include <core/lcp_types.h>
#include <lcr/lcr_def.h>
#include "bitmap.h"

typedef uint16_t lcp_ep_flags_t;

enum {
	LCP_EP_FLAG_CONNECTED = 0,
	LCP_EP_FLAG_CONNECTING,
	LCP_EP_FLAG_USED,
	LCP_EP_FLAG_CLOSED
};

/**
 * @brief configuration of rendez-vous for a given endpoint (specifies fragmentation threshold)
 *
 */
typedef struct lcp_ep_rndv_config {
	size_t frag_thresh;
} lcp_ep_rndv_config_t;

/**
 * @brief configuration of endpoint
 *
 */
typedef struct lcp_ep_config {
        struct {
                size_t max_bcopy;
                size_t max_zcopy;
                size_t max_iovecs;
        } am;

        struct {
                size_t max_bcopy;
                size_t max_zcopy;
                size_t max_iovecs;
        } tag;
        int offload;

        struct {
                size_t max_put_zcopy;
                size_t max_get_zcopy;
        } rndv;

        struct {
                size_t max_put_bcopy;
                size_t max_put_zcopy;
                size_t max_get_bcopy;
                size_t max_get_zcopy;
        } rma;

        struct {
                size_t max_fetch_size;
                size_t max_post_size;
        } ato;

        size_t rndv_threshold;

} lcp_ep_config_t;

struct lcp_ep {
        lcp_ep_config_t config;
        unsigned cap; /* Endpoint capabilities. */

        lcp_chnl_idx_t am_chnl; /* Priority channel for AM. */
        bmap_t         am_bmap; /* Send map for AM. */
        lcp_chnl_idx_t tag_chnl; /* Priority channel for tag offload */
        bmap_t         tag_bmap; /* Send map for TAG. */
        lcp_chnl_idx_t rma_chnl; /* Channel for RMA operations */
        bmap_t         rma_bmap; /* Send map for RMA. */
        lcp_chnl_idx_t ato_chnl; /* Channel for Atomic operations */

        lcp_ep_flags_t flags;
        int state;

        lcp_manager_h mngr; /* Back reference to manager */

        uint64_t  uid; /* Remote peer uid */

        int num_chnls; /* Number of channels */
        _mpc_lowcomm_endpoint_t **lct_eps; //FIXME: rename (lct not ok)
        bmap_t avail_map; /* Bitmap of usable transport endpoints */
        bmap_t conn_map;  /* Bitmap of connected transport endpoints */
};


int lcp_context_ep_create(lcp_manager_h ctx, lcp_ep_h *ep_p,
			  uint64_t uid, unsigned flags);
int lcp_ep_progress_conn(lcp_manager_h ctx, lcp_ep_h ep);
void lcp_ep_delete(lcp_ep_h ep);
int lcp_ep_get_next_cc(lcp_ep_h ep, lcp_chnl_idx_t cc, bmap_t send_map);

#endif
