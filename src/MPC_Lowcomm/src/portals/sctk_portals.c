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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_debug.h>
#include "sctk_rail.h"
#include "sctk_ptl_toolkit.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_rdma.h"
#include "endpoint.h"

#include "lcr_ptl.h"
#include <lcr/lcr_component.h>
#include <dirent.h>

#include <mpc_common_types.h>
#include <mpc_common_rank.h>

static volatile short rail_is_ready = 0;

/**
 * @brief Main entry point for sending message (called by higher layers)
 *
 * @param[in,out] msg message to send
 * @param[in]  endpoint connection handler to take for routing
 */
static void _mpc_lowcomm_portals_send_message ( mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint )
{
	sctk_ptl_send_message(msg, endpoint);
}

/**
 * Callback used to notify the driver a new 'recv' message has been locally posted.
 * \param[in] msg the posted msdg
 * \param[in] rail the Portals rail
 */
static void _mpc_lowcomm_portals_notify_receive ( mpc_lowcomm_ptp_message_t *msg, sctk_rail_info_t *rail )
{
	/* by construction, a network-received CM will generate a local recv
	 * So, in this case, we have nothing to do here
	 */
	if(_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		return;
	}

	/* in any other case... */
	sctk_ptl_notify_recv(msg, rail);
}

/**
 * @brief Poll the Portals RAIL
 *
 * @param rail the rail to poll
 * @param cnt the number of events to poll
 */
static inline void __ptl_poll(sctk_rail_info_t* rail, __UNUSED__ int cnt)
{
	sctk_ptl_eqs_poll( rail, 5 );
	sctk_ptl_mds_poll( rail, 5 );
}

/**
 * @brief NOTIFIER : routine called by idle task (hierarchical election to avoid contention)
 *
 * @param[in,out] rail driver data from current network
 */
static void _mpc_lowcomm_portals_notify_idle (sctk_rail_info_t* rail)
{
	/* '5' has been chosen arbitrarly for now */
	__ptl_poll(rail, 5);
}

/**
 * @brief Routine called when a message is to be performed
 *
 * @param remote_process the corresponding UID
 * @param remote_task_id the remote task ID
 * @param polling_task_id the task polling
 * @param blocking should we block ?
 * @param rail the rail to poll
 */
static void _mpc_lowcomm_portals_notify_perform(__UNUSED__ mpc_lowcomm_peer_uid_t remote_process, __UNUSED__ int remote_task_id, __UNUSED__ int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail)
{
	__ptl_poll(rail, 3);
}

static void _mpc_lowcomm_portals_notify_any_source (__UNUSED__ int polling_task_id, 
                                                    __UNUSED__ int blocking, 
                                                    sctk_rail_info_t* rail)
{
	__ptl_poll(rail, 2);
}



static void _mpc_lowcomm_portals_notify_probe (sctk_rail_info_t* rail, mpc_lowcomm_ptp_message_header_t* hdr, int *status)
{
	*status = sctk_ptl_pending_me_probe(rail, hdr, SCTK_PTL_ME_PROBE_ONLY);
}

/**
 * Notify the driver a new MPI communicator has been created.
 * Becasue Portals driver has been split according to MPI_Comm semantics, each
 * new communicator creation has to trigger a new PT entry creation.
 * \param[in] rail the Portals rail
 * \param[in] comm_idx the communicator ID
 * \param[in] comm_size number of processes in this comm
 */
static void _mpc_lowcomm_portals_notify_newcomm(sctk_rail_info_t* rail, mpc_lowcomm_communicator_id_t comm_idx, size_t comm_size)
{
#ifndef MPC_LOWCOMM_PROTOCOL
        sctk_ptl_comm_register(&rail->network.ptl, comm_idx, comm_size);
#endif
}

/**
 * Notify the driver a new MPI communicator has been created.
 * Becasue Portals driver has been split according to MPI_Comm semantics, each
 * new communicator creation has to trigger a new PT entry creation.
 * \param[in] rail the Portals rail
 * \param[in] comm_idx the communicator ID
 * \param[in] comm_size number of processes in this comm
 */
static void _mpc_lowcomm_portals_notify_delcomm(sctk_rail_info_t* rail, mpc_lowcomm_communicator_id_t comm_idx, size_t comm_size)
{
#ifndef MPC_LOWCOMM_PROTOCOL
        sctk_ptl_comm_delete(&rail->network.ptl, comm_idx, comm_size);
#endif
}


/**
 * @brief Routine called just before a message is forwarded to higher layer (sctk_inter_thread_comm)
 *
 * @param[in,out] msg message to forward
 *
 * @returns 1, in any case
 */
static int sctk_send_message_from_network_ptl ( mpc_lowcomm_ptp_message_t *msg )
{
	if ( _mpc_lowcomm_reorder_msg_check ( msg ) == _MPC_LOWCOMM_REORDER_NO_NUMBERING )
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check ( msg, 1 );
	}

	return 1;
}

/** 
 * Function called globally when C/R is enabled, to close the rail before checkpointing.
 * \param[in] rail the Portals rail to shut down.
 */
void sctk_network_finalize_ptl(sctk_rail_info_t* rail)
{
	sctk_ptl_fini_interface(rail);
}

/**
 * Function called at task scope, before closing the rail.
 * \param[in] rail the Portals rail
 * \param[in] taskid the Task ID to stop
 * \param[in] vp the VP the task is bound
 */
void sctk_network_finalize_task_ptl(sctk_rail_info_t* rail, int taskid, int vp)
{
	UNUSED(rail);
	UNUSED(taskid);
	UNUSED(vp);
}

/**
 * Function called at task scope, to re-init the Portals context for each task.
 * \param[in] rail the Portals rail
 * \param[in] taskid the Task ID to restart
 * \param[in] vp the VP the task is bound
 */
void sctk_network_initialize_task_ptl(sctk_rail_info_t* rail, int taskid, int vp)
{
	UNUSED(rail);
	UNUSED(taskid);
	UNUSED(vp);
}

/**
 * Proceed to establish a connection to a given destination.
 * \param[in] rail the route owner
 * \param[in] dest the remote process id
 */
static void sctk_network_connect_on_demand_ptl ( struct sctk_rail_info_s * rail , mpc_lowcomm_peer_uid_t dest )
{
	sctk_ptl_id_t id = sctk_ptl_map_id(rail, dest);
	sctk_ptl_add_route(dest, id, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
}


/**
 * Nothing to do here for Portals.
 * Because we suppose that a connect_to maps with a connect_from during the
 * first topology setup, we only have to support one end (here, connect_from().
 * The other and will respond through control messages
 */
static void sctk_network_connect_to_ptl( int from, int to, sctk_rail_info_t * rail )
{
	/* nothing to do */
	UNUSED(from);
	UNUSED(to);
	UNUSED(rail);
}

/**
 * Simply create a route to a given destination by using On-demand support.
 * \see sctk_network_connect_on_demand_ptl.
 * \param[in] from the process id initiating the request
 * \param[in] to the targeted process
 * \param[in] rail the forthcoming route owner.
 */
static void sctk_network_connect_from_ptl( int from, int to, sctk_rail_info_t * rail)
{
	UNUSED(from);
	sctk_network_connect_on_demand_ptl(rail, to);
}

/************ INIT ****************/
/**
 * Entry point to initialize a Portals rail.
 *
 * \param[in,out] rail
 */
void sctk_network_init_ptl (sctk_rail_info_t *rail)
{
	if(sctk_rail_count() > 1 && mpc_common_get_process_rank() == 0)
	{
		mpc_common_debug_warning("This Portals 4 process-based driver is not suited for multi-rail usage.");
		mpc_common_debug_warning("Please do not consider using more than one rail to avoid memory leaks.");
	}
	/* just select the type of init for this rail (ring,full..), nothing more */
	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, NULL );
	rail->network_name                 = "Portals Process-Based optimization";

	/* Register msg hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_portals_send_message;
	rail->notify_recv_message       = _mpc_lowcomm_portals_notify_receive;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = _mpc_lowcomm_portals_notify_perform;
	rail->notify_idle_message       = _mpc_lowcomm_portals_notify_idle;
	rail->notify_any_source_message = _mpc_lowcomm_portals_notify_any_source;
	rail->notify_new_comm           = _mpc_lowcomm_portals_notify_newcomm;
	rail->notify_del_comm           = _mpc_lowcomm_portals_notify_delcomm;
	rail->send_message_from_network = sctk_send_message_from_network_ptl;
	rail->notify_probe_message      = _mpc_lowcomm_portals_notify_probe;

	/* RDMA */
	rail->rail_pin_region        = sctk_ptl_pin_region;
	rail->rail_unpin_region      = sctk_ptl_unpin_region;
	rail->rdma_write             = sctk_ptl_rdma_write;
	rail->rdma_read              = sctk_ptl_rdma_read;
	rail->rdma_fetch_and_op_gate = sctk_ptl_rdma_fetch_and_op_gate;
	rail->rdma_fetch_and_op      = sctk_ptl_rdma_fetch_and_op;
	rail->rdma_cas_gate          = sctk_ptl_rdma_cas_gate;
	rail->rdma_cas               = sctk_ptl_rdma_cas;

	/* rail closing/re-opening calls */
	rail->driver_finalize         = sctk_network_finalize_ptl;
	rail->finalize_task           = sctk_network_finalize_task_ptl;
	rail->initialize_task         = sctk_network_initialize_task_ptl;
	rail->connect_to              = sctk_network_connect_to_ptl;
	rail->connect_from            = sctk_network_connect_from_ptl;
	rail->connect_on_demand       = sctk_network_connect_on_demand_ptl;

	sctk_ptl_init_interface( rail );

	rail_is_ready = 1;
	mpc_common_debug("rank %d mapped to Portals ID (nid/pid): %llu/%llu", mpc_common_get_process_rank(), rail->network.ptl.id.phys.nid, rail->network.ptl.id.phys.pid);
}

int lcr_ptl_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
        attr->iface.cap.am.max_bcopy  = rail->network.ptl.eager_limit;
        attr->iface.cap.am.max_zcopy  = 0;
        attr->iface.cap.am.max_iovecs = rail->network.ptl.ptl_info.max_iovecs;

        attr->iface.cap.tag.max_bcopy  = 0;
        attr->iface.cap.tag.max_zcopy  = rail->network.ptl.eager_limit;
        attr->iface.cap.tag.max_iovecs = rail->network.ptl.ptl_info.max_iovecs;

        attr->iface.cap.rndv.max_send_zcopy = rail->network.ptl.max_mr;
        attr->iface.cap.rndv.max_put_zcopy  = rail->network.ptl.max_put;
        attr->iface.cap.rndv.max_get_zcopy  = rail->network.ptl.max_get;
        attr->iface.cap.rndv.min_frag_size  = rail->network.ptl.min_frag_size;

        attr->iface.cap.rma.max_put_bcopy   = rail->network.ptl.eager_limit;
        attr->iface.cap.rma.max_put_zcopy   = rail->network.ptl.max_put;
        attr->iface.cap.rma.min_frag_size   = rail->network.ptl.min_frag_size;

        attr->mem.cap.max_reg = PTL_SIZE_MAX;

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_query_devices(__UNUSED__ lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        static const char *bxi_dir = "/sys/class/bxi";
        lcr_device_t *devices;
        int is_up;
        int num_devices;
        struct dirent *entry;
        DIR *dir;

        devices = NULL;
        num_devices = 0;

        /* First, try simulator */
        const char *ptl_iface_name; 
        if ((ptl_iface_name = getenv("PTL_IFACE_NAME")) != NULL) {
                devices = sctk_malloc(sizeof(lcr_device_t));
                strcpy(devices[0].name, ptl_iface_name);
                num_devices = 1;
                goto out;
        }

        /* Then, check if bxi are available in with sysfs */
        dir = opendir(bxi_dir);
        if (dir == NULL) {
                mpc_common_debug_warning("PTL: could not find any ptl device.");
                goto out;
        }

        for (;;) {
                errno = 0;
                entry = readdir(dir);
                if (entry == NULL) {
                        if (errno != 0) {
                                mpc_common_debug_error("PTL: bxi directory exists "
                                                       "but no entry found.");
                                rc = MPC_LOWCOMM_ERROR;
                                goto close_dir;
                        }
                        break;
                }

                /* avoid reading entry like . and .. */
                if (entry->d_type != DT_LNK) {
                        continue;
                }

                is_up = 1;
                //TODO: check if interface is up with bixnic -i <iface> info
                //      LINK_STATUS
                if (!is_up) {
                        continue;
                }

                devices = sctk_realloc(devices, sizeof(*devices) * (num_devices + 1));
                if (devices == NULL) {
                        mpc_common_debug_error("PTL: could not allocate devices");
                        rc = MPC_LOWCOMM_ERROR;
                        goto close_dir;
                }
                
                //FIXME: interface name should always be of the form:
                //       bxi<id> with id, 0 < id < 9 
                strcpy(devices[num_devices].name, entry->d_name);
                ++num_devices;
        }


close_dir:
        closedir(dir);

out:
        *devices_p = devices;
        *num_devices_p = num_devices;

        return rc;
}

int lcr_ptl_iface_open(char *device_name, int id,
		       lcr_rail_config_t *rail_config, 
		       lcr_driver_config_t *driver_config,
		       sctk_rail_info_t **iface_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        lcr_rail_init(rail_config, driver_config, &iface);
        if (iface == NULL) {
                mpc_common_debug_error("LCR: could not allocate tcp rail");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

	strcpy(iface->device_name, device_name);
	iface->rail_number = id; /* used as tag for pmi registration */

        sctk_network_init_ptl(iface);

        /* Add new API call */
	iface->iface_get_attr = lcr_ptl_get_attr;

        /* Active message calls */
        iface->send_am_bcopy       = lcr_ptl_send_am_bcopy;
        iface->send_am_zcopy       = lcr_ptl_send_am_zcopy;
        /* Tag calls */
        iface->send_tag_bcopy      = lcr_ptl_send_tag_bcopy;
        iface->send_tag_zcopy      = lcr_ptl_send_tag_zcopy;
        iface->send_tag_rndv_zcopy = lcr_ptl_send_tag_rndv_zcopy;
        iface->recv_tag_zcopy      = lcr_ptl_recv_tag_zcopy;
        /* RMA calls */
        iface->send_put            = lcr_ptl_send_put;
        iface->send_get            = lcr_ptl_send_get;
        iface->iface_pack_memp     = lcr_ptl_pack_rkey;
        iface->iface_unpack_memp   = lcr_ptl_unpack_rkey;
        iface->rail_pin_region     = lcr_ptl_mem_register;
        iface->rail_unpin_region   = lcr_ptl_mem_unregister;
        /* Interface progess */
        iface->iface_progress      = lcr_ptl_iface_progress;

        *iface_p = iface;
err:
        return rc; 
}

lcr_component_t ptl_component = {
        .name = { "ptl" },
        .rail_name = { "portalsmpi" },
        .query_devices = lcr_ptl_query_devices,
        .iface_open = lcr_ptl_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&ptl_component)
