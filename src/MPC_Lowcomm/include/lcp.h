#ifndef LCP_H
#define LCP_H

#include <stdlib.h>

#include "lcp_def.h"
#include "lcp_common.h"

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
//FIXME: hack for portals pte entry
int lcp_context_has_comm(lcp_context_h ctx, uint64_t comm_key);
int lcp_context_add_comm(lcp_context_h ctx, uint64_t comm_key);

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
        LCP_REQUEST_USER_REQUEST  = LCP_BIT(2)
};

enum lcp_dt_type {
        LCP_DATATYPE_CONTIGUOUS = LCP_BIT(0),
        LCP_DATATYPE_DERIVED    = LCP_BIT(1),
};

typedef struct lcp_request_param {
        uint32_t                     flags;
        lcp_tag_recv_info_t         *recv_info;
        lcp_complete_callback_func_t cb;
        lcp_datatype_t               datatype;
        mpc_lowcomm_request_t       *request;
        lcp_mem_h                    memh;
} lcp_request_param_t;

/* Send/Receive */
int lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                    size_t count, mpc_lowcomm_request_t *request, 
                    const lcp_request_param_t *param);

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param);

int lcp_tag_probe_nb(lcp_task_h task, const int src, 
                     const int tag, const uint64_t comm,
                     lcp_tag_recv_info_t *recv_info);

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
