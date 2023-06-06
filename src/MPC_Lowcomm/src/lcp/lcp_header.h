#ifndef LCP_HEADER_H
#define LCP_HEADER_H

#include <stdint.h>
#include <stddef.h>

typedef struct lcp_am_hdr {
	uint8_t am_id; //FIXME: not needed ?
} lcp_am_hdr_t;

/**
 * @brief tag matched message header
 * 
 */
typedef struct lcp_tag_hdr {
	int32_t  src_tid;  /* source task identifier         */
	int32_t  dest_tid; /* destination process identifier */
	int32_t  tag;      /* tag                            */
	int16_t  seqn;     /* message sequence number        */
	uint16_t comm;     /* communicator identifier        */
} lcp_tag_hdr_t;

/**
 * @brief rendez-vous message header
 * 
 */
typedef struct lcp_rndv_hdr {
	lcp_tag_hdr_t base;
	int32_t       src_pid; /* source process identifier   */
	size_t        size;    /* message size                */
} lcp_rndv_hdr_t;

/**
 * @brief acknowledgement message header
 * 
 */
typedef struct lcp_ack_hdr {
	int32_t  src;
	uint64_t msg_id;
} lcp_ack_hdr_t;

#endif
