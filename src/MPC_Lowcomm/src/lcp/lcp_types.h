#ifndef LCP_TYPES_H
#define LCP_TYPES_H

#include <stdint.h>
#include <bitmap.h>

/* Chanel */
#define LCP_MAX_CHANNELS 6
#define LCP_NULL_CHANNEL ((lcp_chnl_idx_t)-1)
typedef uint8_t lcp_chnl_idx_t; /* Communication channel index */

/* Forward declaration */
typedef struct lcp_request lcp_request_t;

#endif
