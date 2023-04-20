#ifndef SM_H
#define SM_H

#include <stddef.h>

typedef struct lcr_tbsm_iface_info lcr_tbsm_iface_info_t;

typedef struct mpc_lowcomm_tbsm_rma_ctx {
        void *buf;
        size_t size;
} mpc_lowcomm_tbsm_rma_ctx_t;

typedef struct {
        lcr_tbsm_iface_info_t *info;
        size_t eager_limit;
        size_t bcopy_buf_size;
        size_t max_msg_size;
} _mpc_lowcomm_tbsm_rail_info_t;

#endif
