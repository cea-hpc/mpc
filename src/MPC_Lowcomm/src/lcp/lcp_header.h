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

#ifndef LCP_HEADER_H
#define LCP_HEADER_H

#include "lcp.h"
#include <stdint.h>
#include <stddef.h>

typedef struct lcp_am_hdr {
	uint8_t  am_id;
        int32_t  dest_tid;
        uint32_t hdr_size; /* size of user header */
        uint64_t src_uid;
} lcp_am_hdr_t;

/**
 * @brief tag matched message header
 *
 */
typedef struct lcp_tag_hdr {
	int32_t  src_tid;  /* source task identifier         */
	int32_t  dest_tid; /* destination process identifier */
	int32_t  tag;      /* tag                            */
	uint16_t seqn;     /* message sequence number        */
	uint16_t comm;     /* communicator identifier        */
} lcp_tag_hdr_t;

/**
 * @brief tag matched sync message header
 *
 */
typedef struct lcp_tag_sync_hdr {
        lcp_tag_hdr_t base;
        uint64_t msg_id;
        uint64_t src_uid;
} lcp_tag_sync_hdr_t;

/**
 * @brief rendez-vous message header
 *
 */
typedef struct lcp_rndv_hdr {
        union {
                lcp_tag_hdr_t tag;
                lcp_am_hdr_t  am;
        };
        uint64_t msg_id;  /* message id   */
	uint64_t src_uid; /* source uid   */
        uint64_t remote_addr; /* remote address */
	size_t   size;    /* message size */
} lcp_rndv_hdr_t;

/**
 * @brief acknowledgement message header
 *
 */
typedef struct lcp_ack_hdr {
	int32_t  src;
	uint64_t msg_id;
        uint64_t remote_addr;
} lcp_ack_hdr_t;

typedef struct lcp_ato_hdr {
        int      dest_tid;
	uint64_t src_uid; /* source uid   */
        uint64_t msg_id;
        lcp_atomic_op_t op;
        uint64_t value;
        uint64_t remote_addr;
        uint64_t compare;
} lcp_ato_hdr_t;

typedef struct lcp_ato_reply_hdr {
        uint64_t result;
        uint64_t msg_id;
} lcp_ato_reply_hdr_t;

#endif
