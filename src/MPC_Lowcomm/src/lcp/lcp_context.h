#ifndef LCP_CONTEXT_H
#define LCP_CONTEXT_H

#include "sctk_rail.h" /* defines lcr_am_callback_t */
#include "lowcomm_config.h"

#include "lcp_ep.h"
#include "lcp_pending.h"
#include "lcp_tag_matching.h"
#include "lcp_tag_offload.h"

#include "uthash.h"

#define LCP_CONTEXT_LOCK(_ctx) \
	mpc_common_spinlock_lock(&((_ctx)->ctx_lock))
#define LCP_CONTEXT_UNLOCK(_ctx) \
	mpc_common_spinlock_unlock(&((_ctx)->ctx_lock))

typedef struct lcp_am_handler {
	lcr_am_callback_t cb;
	uint64_t flags;
} lcp_am_handler_t;

#define LCP_DEFINE_AM(_id, _cb, _flags) \
	MPC_STATIC_INIT { \
		lcp_am_handlers[_id].cb = _cb; \
		lcp_am_handlers[_id].flags = _flags; \
	}

extern lcp_am_handler_t lcp_am_handlers[];

typedef struct lpc_comm_ctx {
	UT_hash_handle hh;

	mpc_lowcomm_communicator_id_t comm_key;
} lcp_comm_ctx_t;

typedef struct lpc_ep_ctx {
	UT_hash_handle hh;

	mpc_lowcomm_peer_uid_t ep_key;
	lcp_ep_h ep;
} lcp_ep_ctx_t;

typedef struct lcp_rsc_desc {
	char name[MPC_CONF_STRING_SIZE];
	sctk_rail_info_t *iface; /* Rail interface */
	int priority;
	lcr_rail_config_t *iface_config;
	lcr_driver_config_t *driver_config;
        lcr_component_h component;
} lcp_rsc_desc_t;

typedef struct lcp_context_config {
        int multirail_enabled;
        int multirail_heterogeneous_enabled;
        int user_defined;
        int num_selected_components;
        char **selected_components;
        int num_selected_devices;
        char **selected_devices;
} lcp_context_config_t;

struct lcp_context {
	int priority_rail;

        lcp_context_config_t config;

	unsigned flags;
        
        lcr_component_h *cmpts; /* available component handles */
        unsigned num_cmpts; /* number of components */

        lcr_device_t *devices; /* available device descriptors */
        unsigned num_devices; /* number of devices */
        
	int num_resources; /* number of resources (iface) */
	lcp_rsc_desc_t *resources; /* opened resources (iface) */

	//FIXME: hack for portals. Needed to add or remove communicator
	lcp_comm_ctx_t *comm_ht;	

	mpc_common_spinlock_t ctx_lock; /* Context lock */

	lcp_tm_t tm;

	int num_eps; /* number of endpoints created */
	lcp_ep_ctx_t *ep_ht; /* Hash table of created endpoint */

	lcp_pending_table_t *pend_send_req; /* LCP send requests */
	lcp_pending_table_t *pend_recv_req; /* LCP recv requests */

	lcp_prq_match_table_t *prq_table; /* Posted Receive Queue */
	lcp_umq_match_table_t *umq_table; /* Unexpected Message Queue */
};


#endif 
