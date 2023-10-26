#ifndef LCR_TBSM_H
#define LCR_TBSM_H

#include <stddef.h>
#include <mpc_mempool.h>

#include "mpc_common_spinlock.h"

#define  LCR_TBSM_MAX_QUEUES 1024

typedef struct mpc_lowcomm_tbsm_rma_ctx {
        void *buf;
        size_t size;
} mpc_lowcomm_tbsm_rma_ctx_t;

typedef struct {
        mpc_common_spinlock_t poll_lock; 
        mpc_common_spinlock_t conn_lock; //FIXME: change name to queue lock
        size_t eager_limit;
        size_t bcopy_buf_size;
        size_t max_msg_size;
} _mpc_lowcomm_tbsm_rail_info_t;

#endif
