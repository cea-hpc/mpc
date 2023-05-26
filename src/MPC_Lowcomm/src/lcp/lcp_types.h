#ifndef LCP_TYPES_H
#define LCP_TYPES_H

#include <stdint.h>
#include "bitmap.h"
#include "queue.h"
#include <sys/uio.h>

//FIXME: used to get MPC_LOWCOMM_ERROR. Define new LCP errors to remove it.
#include <mpc_lowcomm_types.h>

/* Config */
#define LCP_CONF_STRING_SIZE 512

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
