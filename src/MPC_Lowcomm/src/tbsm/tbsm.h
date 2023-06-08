#ifndef LCR_TBSM_H
#define LCR_TBSM_H

#include <stddef.h>
#include "queue.h"

#include "mpc_common_spinlock.h"

typedef struct mpc_lowcomm_tbsm_rma_ctx {
        void *buf;
        size_t size;
} mpc_lowcomm_tbsm_rma_ctx_t;

typedef struct {
        mpc_queue_head_t queue;
        mpc_common_spinlock_t lock; 
        size_t eager_limit;
        size_t bcopy_buf_size;
        size_t max_msg_size;
} _mpc_lowcomm_tbsm_rail_info_t;

#endif
