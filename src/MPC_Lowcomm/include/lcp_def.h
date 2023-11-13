
#ifndef LCP_DEF_H
#define LCP_DEF_H

#include <stdint.h> 
#include <stddef.h>

/* Handles */
typedef uint64_t                     lcp_datatype_t;
typedef struct lcp_ep               *lcp_ep_h;
typedef struct lcp_context          *lcp_context_h;
typedef struct lcp_task             *lcp_task_h;
typedef struct lcp_request           lcp_request_t;
typedef struct lcp_unexp_ctnr        lcp_unexp_ctnr_t;
typedef struct lcp_mem              *lcp_mem_h;
typedef struct lcp_tag_recv_info     lcp_tag_recv_info_t;
typedef struct lcp_request_param     lcp_request_param_t;
typedef struct lcp_am_recv_param     lcp_am_recv_param_t;
typedef struct lcp_task_completion   lcp_task_completion_t;

typedef struct mpc_lowcomm_request_s                        mpc_lowcomm_request_t;
typedef struct _mpc_lowcomm_config_struct_net_driver_config lcr_driver_config_t;
typedef struct _mpc_lowcomm_config_struct_net_rail          lcr_rail_config_t;

typedef int (*lcp_complete_callback_func_t)(mpc_lowcomm_request_t *req);

typedef int (*lcp_am_recv_callback_func_t)(size_t sent, void *user_data);

typedef int (*lcp_am_callback_t)(void *arg, const void *user_hdr, const size_t hdr_size,
                                 void *data, size_t length,
				 lcp_am_recv_param_t *param);


#endif
