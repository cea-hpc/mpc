#ifndef LCP_H
#define LCP_H

#include <stdlib.h>

#include "lcp_def.h"
#include "lcp_common.h"

/* LCP return code */
typedef enum {
        LCP_SUCCESS     =  0,
        LCP_NO_RESOURCE = -1,
        LCP_ERROR       =  1,
} lcp_status_t;

/* Context parameter flags */
enum {
        LCP_CONTEXT_DATATYPE_OPS = LCP_BIT(0),
        LCP_CONTEXT_PROCESS_UID  = LCP_BIT(1)
};

/* Datatype */
typedef struct lcp_dt_ops {
        void (*pack)(void *context, void *buffer);
        void (*unpack)(void *context, void *buffer); 
} lcp_dt_ops_t;

/* Context */
typedef struct lcp_context_param {
        uint32_t     flags;
        uint64_t     process_uid;
        lcp_dt_ops_t dt_ops;
} lcp_context_param_t;

lcp_context_h lcp_context_get();


int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param);

void lcp_context_task_get(lcp_context_h ctx, int tid, lcp_task_h *task_p);

int lcp_context_fini(lcp_context_h ctx);

/* Tasks */
int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p);

/* Endpoint */
int lcp_ep_create(lcp_context_h ctx, 
                  lcp_ep_h *ep_p, 
		  uint64_t uid, 
                  unsigned flags);
void lcp_ep_get(lcp_context_h ctx, 
                uint64_t uid, 
		lcp_ep_h *ep);

int lcp_ep_get_or_create(lcp_context_h ctx, 
                uint64_t uid, lcp_ep_h *ep_p, 
                unsigned flags);

typedef struct lcp_tag_recv_info {
        size_t length;
        int32_t tag;
        int32_t src;
        unsigned found; //only for probing.
} lcp_tag_recv_info_t;

enum {
        LCP_REQUEST_TRY_OFFLOAD   = LCP_BIT(0),
        LCP_REQUEST_USER_DATA     = LCP_BIT(1),
        LCP_REQUEST_USER_REQUEST  = LCP_BIT(2),
        LCP_REQUEST_TAG_SYNC      = LCP_BIT(3),
        LCP_REQUEST_AM_SYNC       = LCP_BIT(4),
        LCP_REQUEST_AM_CALLBACK   = LCP_BIT(5),
};

enum lcp_dt_type {
        LCP_DATATYPE_CONTIGUOUS = LCP_BIT(0),
        LCP_DATATYPE_DERIVED    = LCP_BIT(1),
};

enum {
        LCP_AM_EAGER = LCP_BIT(0),
        LCP_AM_RNDV  = LCP_BIT(1),
};

typedef struct lcp_request_param {
        uint32_t                     flags;
        lcp_tag_recv_info_t         *recv_info;
        lcp_complete_callback_func_t cb;
        lcp_am_recv_callback_func_t  am_cb;
        void                        *user_request;
        lcp_datatype_t               datatype;
        mpc_lowcomm_request_t       *request;
        lcp_mem_h                    memh;
} lcp_request_param_t;

typedef struct lcp_am_recv_param {
        uint32_t flags;
        lcp_ep_h reply_ep;
} lcp_am_recv_param_t;

/* Send/Receive tag */
int lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                    size_t count, mpc_lowcomm_request_t *request, 
                    const lcp_request_param_t *param);

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param);

int lcp_tag_probe_nb(lcp_task_h task, const int src, 
                     const int tag, const uint64_t comm,
                     lcp_tag_recv_info_t *recv_info);

/* Send/Receive active message */
int lcp_am_set_handler_callback(lcp_task_h task, uint8_t am_id,
                                void *arg, lcp_am_callback_t cb,
                                unsigned flags);
int lcp_am_send_nb(lcp_ep_h ep, lcp_task_h task, int32_t dest_tid, 
                   uint8_t am_id, void *hdr, size_t hdr_size, const void *buffer, 
                   size_t count, const lcp_request_param_t *param);
int lcp_am_recv_nb(lcp_task_h task, void *data_ctnr, void *buffer, 
                   size_t count, lcp_request_param_t *param);

/* Put/Get */
int lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, 
               size_t length, uint64_t remote_addr, lcp_mem_h rkey,
               lcp_request_param_t *param); 

/* Memory registration */
int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, 
                     size_t length);
int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem);
int lcp_mem_pack(lcp_context_h ctx, lcp_mem_h mem, 
                    void **rkey_buf_p, size_t *rkey_len);
int lcp_mem_unpack(lcp_context_h ctx, lcp_mem_h *mem_p, 
                   void *src, size_t size);
void lcp_mem_release_rkey_buf(void *rkey_buffer);

/* Progress */
int lcp_progress(lcp_context_h ctx);

#endif
