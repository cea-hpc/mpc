#ifndef LCP_CONTEXT_H
#define LCP_CONTEXT_H

#include "lcp_header.h"
#include "sctk_rail.h" /* defines lcr_am_callback_t */
#include "lowcomm_config.h"

#include "lcp_ep.h"
#include "lcp_pending.h"
#include "lcp_tag_matching.h"
#include "lcp_types.h"

#include "uthash.h"
#include "opa_primitives.h"

#define LCP_CONTEXT_LOCK(_ctx) \
	mpc_common_spinlock_lock(&((_ctx)->ctx_lock))
#define LCP_CONTEXT_UNLOCK(_ctx) \
	mpc_common_spinlock_unlock(&((_ctx)->ctx_lock))

/**
 * @brief handler holding a callback and a flag
 * 
 */
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

/**
 * @brief communicator
 * 
 */
typedef struct lpc_comm_ctx {
	UT_hash_handle hh;

	mpc_lowcomm_communicator_id_t comm_key;
} lcp_comm_ctx_t;

/**
 * @brief endpoint
 * 
 */
typedef struct lcp_ep_ctx {
	UT_hash_handle hh;

	mpc_lowcomm_peer_uid_t ep_key;
	lcp_ep_h ep;
} lcp_ep_ctx_t;

typedef struct lcp_match_ctx {
	UT_hash_handle hh;

	int muid_key;
	lcp_request_t *req;
} lcp_match_ctx_t;

typedef struct lcp_task_entry {
	UT_hash_handle hh;
        
        int task_key;
        lcp_task_h task;
} lcp_task_entry_t;

typedef struct lcp_task_table {
        mpc_common_spinlock_t lock;
        lcp_task_entry_t *table;
} lcp_task_table_t;

typedef struct lcp_rsc_desc {
	char name[MPC_CONF_STRING_SIZE];
	sctk_rail_info_t *iface; /* Rail interface */
	int priority;
	lcr_rail_config_t *iface_config;
	lcr_driver_config_t *driver_config;
        lcr_component_h component;
} lcp_rsc_desc_t;

/**
 * @brief context configuration
 * 
 */
typedef struct lcp_context_config {
        int multirail_enabled;
        int multirail_heterogeneous_enabled;
        int user_defined;
        int num_selected_components;
        char **selected_components;
        int num_selected_devices;
        char **selected_devices;
	lcp_rndv_mode_t rndv_mode;
        int offload;
} lcp_context_config_t;

/**
 * @brief context (configuration, devices, resources, queues)
 * 
 */
struct lcp_context {
	int priority_rail;

        lcp_context_config_t config;

	unsigned flags;
        
        OPA_int_t msg_id; /* unique message identifier */
        
        OPA_int_t muid; /* matching unique identifier */
	lcp_pending_table_t *match_ht; /* ht of matching request */
        
        lcr_component_h *cmpts; /* available component handles */
        unsigned num_cmpts; /* number of components */

        lcr_device_t *devices; /* available device descriptors */
        unsigned num_devices; /* number of devices */
        
	int num_resources; /* number of resources (iface) */
	lcp_rsc_desc_t *resources; /* opened resources (iface) */

	mpc_common_spinlock_t ctx_lock; /* Context lock */

	int num_eps; /* number of endpoints created */
	lcp_ep_ctx_t *ep_ht; /* Hash table of created endpoint */

        uint64_t process_uid; /* process uid used for endpoint creation */

        mpc_queue_head_t pending_queue;
	lcp_pending_table_t *pend; /* LCP send requests */

        lcp_task_table_t *tasks; /* LCP tasks (per thread data) */

        lcp_dt_ops_t dt_ops; /* pack/unpack functions */
};

uint64_t mpc_lowcomm_tag_get_endpoint_address(lcp_tag_hdr_t *hdr);

#endif 
