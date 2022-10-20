#ifndef LCP_HEADER_H
#define LCP_HEADER_H

#include <stdint.h>
#include <stddef.h>

typedef struct lcp_base_hdr {
	uint64_t comm_id;
} lcp_base_hdr_t;

typedef struct lcp_am_hdr {
	lcp_base_hdr_t base;
	uint8_t am_id; //FIXME: not needed ?
} lcp_am_hdr_t;

typedef struct lcp_tag_hdr {
	lcp_base_hdr_t base;
	uint64_t       src;
	uint64_t       dest;
	int            src_tsk;
	int            dest_tsk;
	int            tag;
	int64_t        seqn;
} lcp_tag_hdr_t;

typedef struct lcp_rndv_hdr {
	lcp_tag_hdr_t base;
	uint64_t msg_id;
	uint64_t dest;
	size_t total_size;
} lcp_rndv_hdr_t;

typedef struct lcp_rndv_ack_hdr {
	lcp_am_hdr_t base;
	uint64_t msg_id;
	uint64_t src;
	uint64_t dest;
} lcp_rndv_ack_hdr_t;

typedef struct lcp_frag_hdr {
	lcp_am_hdr_t base;
	uint64_t msg_id;
	uint64_t dest;
	size_t offset;
} lcp_frag_hdr_t;

#endif
