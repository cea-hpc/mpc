/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_RAIL_H
#define SCTK_RAIL_H

#include <comm.h>
#include <mpc_config.h>

#include "mpc_lowcomm_types.h"
#include <mpc_topology_device.h>

#include "lowcomm_types_internal.h"

#include "lcr/lcr_iface.h"

#include "endpoint.h"

#include "lowcomm_config.h"

#include <mpc_lowcomm_monitor.h>
#include <stdint.h>
#include <list.h>


/************************************************************************/
/* Rail Info                                                            */
/************************************************************************/

/** \brief Network dependent RAIL informations */
typedef union
{
	_mpc_lowcomm_tcp_rail_info_t tcp; /**< TCP Rail Info */
	_mpc_lowcomm_tbsm_rail_info_t tbsm;
#ifdef MPC_USE_PORTALS
	_mpc_lowcomm_ptl_rail_info_t  ptl; /**< Portals Info */
#endif
#ifdef MPC_USE_OFI
	_mpc_lowcomm_ofi_rail_info_t ofi;
#endif
	_mpc_lowcomm_shm_rail_info_t shm;
} sctk_rail_info_spec_t;

/************************************************************************/
/* Rail Pin CTX                                                         */
/************************************************************************/

#ifdef MPC_USE_OFI
#include <rdma/fi_domain.h>
#endif


typedef union
{
#ifdef MPC_USE_PORTALS
	lcr_ptl_mem_ctx_t       ptl;
#endif /* MPC_USE_PORTALS */
#ifdef MPC_USE_OFI
struct _mpc_ofi_pinning_context ofipin;
#endif
        mpc_lowcomm_tbsm_rma_ctx_t  tbsm;
	_mpc_lowcomm_shm_pinning_ctx_t shm;
}sctk_rail_pin_ctx_internal_t;

struct sctk_rail_pin_ctx_list
{
	sctk_rail_pin_ctx_internal_t pin;
};
typedef struct sctk_rail_pin_ctx_list lcr_memp_t; //TODO: change name

#define SCTK_PIN_LIST_SIZE    4

typedef struct
{
	struct sctk_rail_pin_ctx_list list[SCTK_PIN_LIST_SIZE];
	int                           size;
}sctk_rail_pin_ctx_t;

typedef enum
{
	SCTK_RAIL_ST_INIT = 0,
	SCTK_RAIL_ST_ENABLED,
	SCTK_RAIL_ST_DISABLED,
	SCTK_RAIL_ST_COUNT
} sctk_rail_state_t;

void sctk_rail_pin_ctx_init(sctk_rail_pin_ctx_t *ctx, void *addr, size_t size);
void sctk_rail_pin_ctx_release(sctk_rail_pin_ctx_t *ctx);

/************************************************************************/
/* Protocol                                                             */
/************************************************************************/


//FIXME: not used as a flags right now
enum {
        LCR_IFACE_FLUSH_EP          = MPC_BIT(0),
        LCR_IFACE_FLUSH_MEM         = MPC_BIT(1),
        LCR_IFACE_FLUSH_EP_MEM      = MPC_BIT(2),
        LCR_IFACE_FLUSH_IFACE       = MPC_BIT(3),
        LCR_IFACE_REGISTER_MEM_DYN  = MPC_BIT(4),
        LCR_IFACE_REGISTER_MEM_STAT = MPC_BIT(5),
};

enum {
	LCR_IFACE_TM_OVERFLOW       = MPC_BIT(0),
	LCR_IFACE_TM_NOVERFLOW      = MPC_BIT(1),
	LCR_IFACE_TM_PERSISTANT_MEM = MPC_BIT(2),
	LCR_IFACE_TM_SEARCH         = MPC_BIT(3),
	LCR_IFACE_TM_ERROR          = MPC_BIT(4),
        LCR_IFACE_AM_LAYOUT_BUFFER  = MPC_BIT(5),
        LCR_IFACE_AM_LAYOUT_IOV     = MPC_BIT(6),
};

//TODO: doc to doxygen.
/* Interface feature requested during initialization. If feature not supported
 * by interface capabitities, then error is returned. */
//FIXME: see how to factorize with LCP_MANAGER_FEATURE_{AM,TAG,RMA}. Also,
//       should they be called LCP_HINTS_{...} instead?
enum {
        LCR_FEATURE_TS       = MPC_BIT(0),
        LCR_FEATURE_OS       = MPC_BIT(1),
};

enum {
        LCR_IFACE_CAP_RMA         = MPC_BIT(0),
        LCR_IFACE_CAP_LOOPBACK    = MPC_BIT(1),
        LCR_IFACE_CAP_REMOTE      = MPC_BIT(2),
        LCR_IFACE_CAP_TAG_OFFLOAD = MPC_BIT(3),
        LCR_IFACE_CAP_ATOMICS     = MPC_BIT(4),
};

/* Active message handler table entry */
typedef struct lcr_am_handler
{
	lcr_am_callback_t cb;
	void *            arg;
	uint64_t          flags;
} lcr_am_handler_t;

int lcr_iface_set_am_handler(sctk_rail_info_t *rail, uint8_t id,
                             lcr_am_callback_t cb, void *arg, uint64_t flags);

typedef struct lcr_completion
{
        int                       count;
	size_t                    sent;
	lcr_completion_callback_t comp_cb;
} lcr_completion_t;

/* tag offloading context */
typedef struct lcr_tag_context
{
	void *           start; //NOTE: buffer returned by tm interface
	void *           req;   //NOTE: contains lcp_request_t
	uint64_t         imm;
	lcr_tag_t        tag;   //NOTE: needed by LCP to get msg_id and
	                        //      find corresponding request
	lcr_completion_t comp;  //NOTE: needed by send when portals ack
	                        //      is received to complete the request.
	unsigned         flags;
} lcr_tag_context_t;

/************************************************************************/
/* Rail                                                                 */
/************************************************************************/
#define SCTK_RAIL_TYPE(r)         ( (r)->runtime_config_driver_config->driver.type)

#define LCR_IFACE_IS_TM(iface)    ( (iface)->runtime_config_rail->offload)
struct lcr_rail_attr {
        struct {
                struct {
                        struct {
                                size_t max_bcopy;
                                size_t max_zcopy;
                                size_t max_iovecs;
                        } am;

                        struct {
                                size_t max_bcopy;
                                size_t max_zcopy;
                                size_t max_iovecs;
                        } tag;

                        struct {
                                size_t max_send_zcopy;
                                size_t max_put_zcopy;
                                size_t max_get_zcopy;
                                size_t min_frag_size;
                        } rndv;

                        struct {
                                size_t max_put_bcopy;
                                size_t max_put_zcopy;
                                size_t max_get_bcopy;
                                size_t max_get_zcopy;
                                size_t min_frag_size;
                        } rma;

                        struct {
                                size_t max_fetch_size;
                                size_t max_post_size;
                        } ato;

                        unsigned flags; /* Interface capabilities. */
                } cap;
        } iface;

        struct {
                struct {
                        size_t max_reg;
                } cap;

                size_t size_packed_mkey;
        } mem;
};


/** This structure gathers all informations linked to a network rail
 *
 *  All rail information is stored in the sctk_route file
 *  using the \ref sctk_route_set_rail_infos function
 */
struct sctk_rail_info_s
{
	/* Global Info */
	int                                                  rail_number;  /**< ID of this rail */
        int                                                  pmi_tag;      /**< Tag of the rail within PMI key value store. Used for endpoint creation. */
	int                                                  priority;     /**< Priority of this rail */
	char *                                               network_name; /**< Name of this rail */
	mpc_topology_device_t *                              rail_device;  /**< Device associated with the rail */
	sctk_rail_state_t                                    state;        /**< is this rail usable? */
	lcr_am_handler_t                                     am[LCR_AM_ID_MAX];

	/* Network Info */
	sctk_rail_info_spec_t                                network;                 /**< Network dependent rail info */
	char                                                 on_demand;               /**< If the rail allows on-demand connections */
	char                                                 is_rdma;                 /**< If the rail supports RDMA operations */

	/* Configuration Info */
	struct _mpc_lowcomm_config_struct_net_rail *         runtime_config_rail;          /**< Rail config */
	struct _mpc_lowcomm_config_struct_net_driver_config *runtime_config_driver_config; /**< Driver config */

	/* Route table */
	_mpc_lowcomm_endpoint_table_t *                      route_table;

	/* rail capabilities */
	unsigned cap;

	/* Endpoint AM API */
	lcr_send_am_bcopy_func_t                             send_am_bcopy;
	lcr_send_am_zcopy_func_t                             send_am_zcopy;

	/* Endpoint TAG API */
	lcr_send_tag_bcopy_func_t                            send_tag_bcopy;
	lcr_send_tag_zcopy_func_t                            send_tag_zcopy;
	lcr_get_tag_zcopy_func_t                             get_tag_zcopy;
	lcr_post_tag_zcopy_func_t                            post_tag_zcopy;
	lcr_unpost_tag_zcopy_func_t                          unpost_tag_zcopy;

	/* Endpoint RMA API */
	lcr_put_bcopy_func_t                                 put_bcopy;
	lcr_put_zcopy_func_t                                 put_zcopy;
	lcr_get_bcopy_func_t                                 get_bcopy;
	lcr_get_zcopy_func_t                                 get_zcopy;

        /* Endpoint Atomic API */
        lcr_atomic_post_func_t                               atomic_post;
        lcr_atomic_fetch_func_t                              atomic_fetch;
        lcr_atomic_cswap_func_t                              atomic_cswap;

	/* RMA Sync API */
        lcr_flush_mem_ep_func_t                              flush_mem_ep;
        lcr_flush_ep_func_t                                  flush_ep;
        lcr_flush_mem_func_t                                 flush_mem;
        lcr_flush_iface_func_t                               flush_iface;

	/* Interface API */
	lcr_iface_get_attr_func_t                            iface_get_attr;
	lcr_iface_progress_func_t                            iface_progress;
	lcr_iface_pack_memp_func_t                           iface_pack_memp;
	lcr_iface_unpack_memp_func_t                         iface_unpack_memp;
        lcr_iface_is_reachable_func_t                        iface_is_reachable;

        /* Interface memory API */
        lcr_iface_register_mem_func_t                        iface_register_mem;
        lcr_iface_unregister_mem_func_t                      iface_unregister_mem;

	/* Interface Sync API */
        lcr_iface_fence_func_t                               iface_fence;
        mpc_list_elem_t progress;

	/* Task Init and release */
	void                                                 ( *finalize_task ) (struct sctk_rail_info_s *, int taskid, int rank);
	void                                                 ( *initialize_task ) (struct sctk_rail_info_s *, int taskid, int rank);
	int                                                  ( *send_message_from_network ) (mpc_lowcomm_ptp_message_t *);
	void                                                 ( *connect_on_demand ) (struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest);


	void                                                 (*driver_finalize)(sctk_rail_info_t *);
};

int lcr_rail_init(lcr_rail_config_t *rail_config,
                  lcr_driver_config_t *driver_config,
                  struct sctk_rail_info_s **rail_p);

int lcr_rail_build_pmi_tag(int mngr_id, int rail_id);
/* Rail  Array                                                          */


/** This structure gathers all rails in an array */
struct sctk_rail_array
{
	/** Dynamic array storing rails */
	sctk_rail_info_t *rails;
	/** Number of rails */
	int               rail_number;
	int               rail_current_id;
	/** Id of the RDMA rail */
	int               rdma_rail;
};



int sctk_rail_count();
sctk_rail_info_t *sctk_rail_get_by_id(int i);
int sctk_rail_get_rdma_id();
sctk_rail_info_t *sctk_rail_get_rdma();

void sctk_rail_enable(sctk_rail_info_t *rail);
void sctk_rail_disable(sctk_rail_info_t *rail);

void sctk_rail_dump_routes();

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
/** Retrieve the HWLOC device associated with a rail */
static inline hwloc_obj_t sctk_rail_get_device_hwloc_obj(sctk_rail_info_t *rail)
{
	if(!rail->rail_device)
	{
		return NULL;
	}

	return rail->rail_device->obj;
}

/** Return the name of the device (as put in the config) */
static inline char *sctk_rail_get_device_name(sctk_rail_info_t *rail)
{
	if(!rail)
	{
		return NULL;
	}

	return rail->runtime_config_rail->device;
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/************************************************************************/
/* Add Routes to Rail                                                   */
/************************************************************************/

void sctk_rail_add_static_route(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *tmp);
void sctk_rail_add_dynamic_route(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *tmp);
void sctk_rail_add_dynamic_route_no_lock(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *tmp);

_mpc_lowcomm_endpoint_t *sctk_rail_add_or_reuse_route_dynamic(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest, _mpc_lowcomm_endpoint_t *(*create_func)(), void (*init_func)(mpc_lowcomm_peer_uid_t dest, sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table, int ondemand), int *added, char is_initiator);

/************************************************************************/
/* Get Routes From RAIL                                                 */
/************************************************************************/

_mpc_lowcomm_endpoint_t *sctk_rail_get_static_route_to_process(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *sctk_rail_get_any_route_to_process(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *sctk_rail_get_dynamic_route_to_process(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *sctk_rail_get_any_route_to_task_or_on_demand(sctk_rail_info_t *rail, int dest_task);
_mpc_lowcomm_endpoint_t *sctk_rail_get_any_route_to_process_or_forward(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *sctk_rail_get_static_route_to_process_or_forward(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
_mpc_lowcomm_endpoint_t *sctk_rail_get_any_route_to_process_or_on_demand(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);


#endif /* SCTK_RAIL_H */
