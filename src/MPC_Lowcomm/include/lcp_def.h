#ifndef LCP_DEF_H
#define LCP_DEF_H

/* Handles */
typedef struct lcp_ep               *lcp_ep_h;
typedef struct lcp_context          *lcp_context_h;
typedef struct lcp_request           lcp_request_t;
typedef struct lcp_unexp_ctnr        lcp_unexp_ctnr_t;
typedef struct lcp_mem              *lcp_mem_h;
typedef struct lcp_tag_recv_info     lcp_tag_recv_info_t;
typedef struct lcp_request_param     lcp_request_param_t;
typedef struct mpc_lowcomm_request_s mpc_lowcomm_request_t;

typedef int (*lcp_complete_callback_func_t)(mpc_lowcomm_request_t *req);

#endif
