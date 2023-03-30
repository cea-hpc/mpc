#ifndef LCP_HEADER_H
#define LCP_HEADER_H

#include <stdint.h>
#include <stddef.h>

typedef struct lcp_am_hdr {
	uint8_t am_id; //FIXME: not needed ?
} lcp_am_hdr_t;

typedef struct lcp_tag_hdr {
	int32_t src;
	int32_t dest;
	int32_t tag;
	int16_t seqn;
	uint16_t comm;
} lcp_tag_hdr_t;

typedef struct lcp_rndv_hdr {
	lcp_tag_hdr_t base;
	uint64_t      msg_id;
	size_t        size;
} lcp_rndv_hdr_t;

typedef struct lcp_rndv_ack_hdr {
	uint64_t msg_id;
	int32_t  src;
} lcp_rndv_ack_hdr_t;

typedef struct lcp_frag_hdr {
	lcp_am_hdr_t base;
	uint64_t msg_id;
	uint64_t dest;
	size_t offset;
} lcp_frag_hdr_t;

#endif
