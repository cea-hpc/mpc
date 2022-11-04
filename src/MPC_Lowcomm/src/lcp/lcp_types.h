#ifndef LCP_TYPES_H
#define LCP_TYPES_H

#include <stdint.h>

/* Chanel */
#define LCP_MAX_CHANNELS 6
#define LCP_NULL_CHANNEL ((lcp_chnl_idx_t)-1)
typedef uint8_t lcp_chnl_idx_t; /* Communication channel index */

typedef enum {
	LCP_RNDV_GET = 1,
	LCP_RNDV_PUT,
	LCP_RNDV_SEND
} lcp_rndv_mode_t;

#endif
