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


/************************************************************************/
/* Rail Info                                                            */
/************************************************************************/

/** \brief Network dependent RAIL informations */
typedef union
{
	_mpc_lowcomm_tcp_rail_info_t tcp; /**< TCP Rail Info */
	sctk_shm_rail_info_t         shm; /**< SHM Rail Info */
	_mpc_lowcomm_tbsm_rail_info_t tbsm;

#ifdef MPC_USE_PORTALS
	sctk_ptl_rail_info_t         ptl; /**< Portals Info */
#endif
#ifdef MPC_USE_OFI
	mpc_lowcomm_ofi_rail_info_t  ofi;         /**< OFI info */
#endif
} sctk_rail_info_spec_t;

/************************************************************************/
/* Rail Pin CTX                                                         */
/************************************************************************/

#ifdef MPC_USE_PORTALS
#include "sctk_ptl_types.h"
#endif

#ifdef MPC_USE_OFI
#include "ofi_types.h"
#endif

typedef union
{
        struct mpc_lowcomm_tbsm_rma_ctx sm;

#ifdef MPC_USE_PORTALS
	struct sctk_ptl_rdma_ctx       ptl;
#endif /* MPC_USE_PORTALS */
#ifdef MPC_USE_OFI
	struct mpc_lowcomm_ofi_rma_ctx ofi;
#endif /* MPC_USE_OFI */
}sctk_rail_pin_ctx_internal_t;

struct sctk_rail_pin_ctx_list
{
	sctk_rail_pin_ctx_internal_t pin;
	int                          rail_id;
};
typedef struct sctk_rail_pin_ctx_list lcr_memp_t;

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
#define LCR_BIT(i)    (1ul << (i) )

enum {
	LCR_IFACE_TM_OVERFLOW       = LCR_BIT(0),
	LCR_IFACE_TM_NOVERFLOW      = LCR_BIT(1),
	LCR_IFACE_TM_PERSISTANT_MEM = LCR_BIT(2),
	LCR_IFACE_TM_SEARCH         = LCR_BIT(3),
	LCR_IFACE_TM_ERROR          = LCR_BIT(4),
	LCR_IFACE_SM_REQUEST        = LCR_BIT(5),
};

enum {
        LCR_IFACE_CAP_RMA     = LCR_BIT(0),
        LCR_IFACE_CAP_SELF    = LCR_BIT(1),
        LCR_IFACE_CAP_REMOTE  = LCR_BIT(2),
        LCR_IFACE_CAP_OFFLOAD = LCR_BIT(3),
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
                                size_t min_frag_size;
                        } rma;

                        uint64_t flags;
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
 *  All rails informations are stored in the sctk_route file
 *  using the \ref sctk_route_set_rail_infos function
 */
struct sctk_rail_info_s
{
	/* Global Info */
	int                                                  rail_number;  /**< ID of this rail */
	int                                                  priority;     /**< Priority of this rail */
	char *                                               network_name; /**< Name of this rail */
	mpc_topology_device_t *                              rail_device;  /**< Device associated with the rail */
	sctk_rail_state_t                                    state;        /**< is this rail usable ? */
	lcr_am_handler_t                                     am[LCR_AM_ID_MAX];

	/* Network Infos */
	sctk_rail_info_spec_t                                network;                 /**< Network dependent rail info */
	char                                                 on_demand;               /**< If the rail allows on demand-connexions */
	char                                                 is_rdma;                 /**< If the rail supports RDMA operations */

	/* Configuration Info */
	struct _mpc_lowcomm_config_struct_net_rail *         runtime_config_rail;          /**< Rail config */
	struct _mpc_lowcomm_config_struct_net_driver_config *runtime_config_driver_config; /**< Driver config */

	/* Route table */
	_mpc_lowcomm_endpoint_table_t *                      route_table;

	char                                                 device_name[LCR_DEVICE_NAME_MAX];

	/* rail capabilities */
	unsigned cap;

	/* Endpoint API */
	lcr_send_am_bcopy_func_t                             send_am_bcopy;
	lcr_send_am_zcopy_func_t                             send_am_zcopy;
	lcr_send_tag_bcopy_func_t                            send_tag_bcopy;
	lcr_send_tag_zcopy_func_t                            send_tag_zcopy;
	lcr_put_bcopy_func_t                                 put_bcopy;
	lcr_put_zcopy_func_t                                 put_zcopy;
	lcr_get_bcopy_func_t                                 get_bcopy;
	lcr_get_zcopy_func_t                                 get_zcopy;
	lcr_get_tag_zcopy_func_t                             get_tag_zcopy;
	lcr_post_tag_zcopy_func_t                            post_tag_zcopy;
	lcr_unpost_tag_zcopy_func_t                          unpost_tag_zcopy;
	/* Interface API */
	lcr_iface_get_attr_func_t                            iface_get_attr;
	lcr_iface_progress_func_t                            iface_progress;
	lcr_iface_pack_memp_func_t                           iface_pack_memp;
	lcr_iface_unpack_memp_func_t                         iface_unpack_memp;

	/* Task Init and release */
	void                                                 ( *finalize_task ) (struct sctk_rail_info_s *, int taskid, int rank);
	void                                                 ( *initialize_task ) (struct sctk_rail_info_s *, int taskid, int rank);

	/* Network interactions */
	void                                                 ( *send_message_endpoint ) (mpc_lowcomm_ptp_message_t *, _mpc_lowcomm_endpoint_t *);

	void                                                 ( *notify_recv_message ) (mpc_lowcomm_ptp_message_t *, struct sctk_rail_info_s *);
	void                                                 ( *notify_matching_message ) (mpc_lowcomm_ptp_message_t *, struct sctk_rail_info_s *);
	void                                                 ( *notify_perform_message ) (mpc_lowcomm_peer_uid_t, int, int, int, struct sctk_rail_info_s *);
	void                                                 ( *notify_idle_message ) (struct sctk_rail_info_s *);
	void                                                 ( *notify_any_source_message ) (int, int, struct sctk_rail_info_s *);
	void                                                 ( *notify_probe_message) (struct sctk_rail_info_s *, mpc_lowcomm_ptp_message_header_t *, int *);
	void                                                 ( *notify_new_comm)(struct sctk_rail_info_s *, mpc_lowcomm_communicator_id_t, size_t);
	void                                                 ( *notify_del_comm)(struct sctk_rail_info_s *, mpc_lowcomm_communicator_id_t, size_t);


	int                                                  ( *send_message_from_network ) (mpc_lowcomm_ptp_message_t *);
	void                                                 ( *connect_on_demand ) (struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest);

	/* RDMA Ops */

	int                                                  (*rdma_fetch_and_op_gate)(sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type);


	void                                                 (*rdma_fetch_and_op)(sctk_rail_info_t *rail,
	                                                                          mpc_lowcomm_ptp_message_t *msg,
	                                                                          void *fetch_addr,
	                                                                          struct  sctk_rail_pin_ctx_list *local_key,
	                                                                          void *remote_addr,
	                                                                          struct  sctk_rail_pin_ctx_list *remote_key,
	                                                                          void *add,
	                                                                          RDMA_op op,
	                                                                          RDMA_type type);



	int                                                  (*rdma_cas_gate)(sctk_rail_info_t *rail, size_t size, RDMA_type type);

	void                                                 (*rdma_cas)(sctk_rail_info_t *rail,
	                                                                 mpc_lowcomm_ptp_message_t *msg,
	                                                                 void *res_addr,
	                                                                 struct  sctk_rail_pin_ctx_list *local_key,
	                                                                 void *remote_addr,
	                                                                 struct  sctk_rail_pin_ctx_list *remote_key,
	                                                                 void *comp,
	                                                                 void *new,
	                                                                 RDMA_type type);

	void                                                 (*rdma_write)(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
	                                                                   void *src_addr, struct sctk_rail_pin_ctx_list *local_key,
	                                                                   void *dest_addr, struct  sctk_rail_pin_ctx_list *remote_key,
	                                                                   size_t size);

	void                                                 (*rdma_read)(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
	                                                                  void *src_addr, struct  sctk_rail_pin_ctx_list *remote_key,
	                                                                  void *dest_addr, struct  sctk_rail_pin_ctx_list *local_key,
	                                                                  size_t size);

	/* Pinning */
	void                                                 (*rail_pin_region)(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list, void *addr, size_t size);
	void                                                 (*rail_unpin_region)(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list);

	void                                                 (*driver_finalize)(sctk_rail_info_t *);
};

int lcr_rail_init(lcr_rail_config_t *rail_config,
                  lcr_driver_config_t *driver_config,
                  struct sctk_rail_info_s **rail_p);

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



void sctk_rail_allocate(int count);

sctk_rail_info_t *sctk_rail_register(struct _mpc_lowcomm_config_struct_net_rail *runtime_config_rail,
                                     struct _mpc_lowcomm_config_struct_net_driver_config *runtime_config_driver_config);
void sctk_rail_unregister(sctk_rail_info_t *rail);
int sctk_rail_count();
sctk_rail_info_t *sctk_rail_get_by_id(int i);
int sctk_rail_get_rdma_id();
sctk_rail_info_t *sctk_rail_get_rdma();
void sctk_rail_commit();
int sctk_rail_committed();
void sctk_rail_enable(sctk_rail_info_t *rail);
void sctk_rail_disable(sctk_rail_info_t *rail);
void sctk_rail_dump_routes();

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
