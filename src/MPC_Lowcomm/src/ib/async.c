/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include "ibconfig.h"
#include "endpoint.h"
#include <mpc_topology.h>

#ifdef MPC_Threads
#include <mpc_thread.h>
#else
#include <pthread.h>
#endif

/* IB debug macros */
#if defined MPC_LOWCOMM_IB_MODULE_NAME
#error "MPC_LOWCOMM_IB_MODULE already defined"
#endif
#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME "ASYNC"
#include "ibtoolkit.h"

#include "sctk_rail.h"
#include "ib.h"
#include "async.h"
#include "qp.h"
#include <errno.h>

/********************************************************************/
/* Defines                                                          */
/********************************************************************/
#define DESC_EVENT(config, event, desc, level, fatal)  do { \
  if ( (level != -1) && ( (level <= (config)->verbose_level) || fatal )) \
    _mpc_lowcomm_ib_debug(event":\t"desc); \
  if (fatal) mpc_common_debug_abort(); \
  } while(0)

/********************************************************************/
/* Async Thread                                                     */
/********************************************************************/

/** \brief This functions continuously probes async messages from the HCA
 *
 * It is running in its own system scope thread
 */
void *async_thread ( void *arg )
{
	sctk_rail_info_t *rail = ( sctk_rail_info_t * ) arg;
	_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	struct ibv_async_event event;
	struct ibv_srq_attr mod_attr;
	int rc;
 mpc_topology_bind_to_cpu(-1);

	_mpc_lowcomm_ib_nodebug ( "Async thread running on context %p", device->context );

	while ( 1 )
	{
		/* Get an async event from the card (this is a blocking call) */
		if ( ibv_get_async_event ( ( struct ibv_context * ) device->context, &event ) )
		{
			mpc_common_debug_error ( "[async thread] cannot get event" );
		}

		/* All these events are described in the Mellanox RDMA verbs API
		 * they correspond to asynchronous  events produced by the card */
		switch ( event.event_type )
		{
				/* Completion Queue Overrun => Triggers  IBV_EVENT_QP_FATAL */
			case IBV_EVENT_CQ_ERR:
				DESC_EVENT ( config, "IBV_EVENT_CQ_ERR", "CQ is in error (CQ overrun)", 1, 1 );
				break;

				/* A fatal event occured on the QP */
			case IBV_EVENT_QP_FATAL:
				DESC_EVENT ( config, "IBV_EVENT_QP_FATAL", "Error occurred on a QP and it transitioned to error state", 1, 1 );
				break;

				/* RDMA Layer detected a transport error violation (unsuported or reserved opcode) */
			case IBV_EVENT_QP_REQ_ERR:
				DESC_EVENT ( config, "IBV_EVENT_QP_REQ_ERR", "Invalid Request Local Work Queue Error", 1, 1 );
				break;

				/* Request error :
				 * - Misaligned atomic request
				 * - Too many RDMA read or atomic
				 * - R_Key violation
				 * - Length error without data */
			case IBV_EVENT_QP_ACCESS_ERR:
				DESC_EVENT ( config, "IBV_EVENT_QP_ACCESS_ERR", "Local access violation error", 1, 1 );
				break;

				/* In RC (sometimes in UD) this events notifies new connection on the QP */
			case IBV_EVENT_COMM_EST:
				DESC_EVENT ( config, "IBV_EVENT_COMM_EST", "Communication was established on a QP", -1, 0 );
				break;

				/* The send queue is empty and can be modified */
			case IBV_EVENT_SQ_DRAINED:
				DESC_EVENT ( config, "IBV_EVENT_SQ_DRAINED", "Send Queue was drained of outstanding messages in progress", 1, 0 );
				break;

				/* The QP sucessfully migrated to an alternative path */
			case IBV_EVENT_PATH_MIG:
				DESC_EVENT ( config, "IBV_EVENT_PATH_MIG", "A connection failed to migrate to the alternate path", 1, 0 );
				break;

				/* Path migration change has failed */
			case IBV_EVENT_PATH_MIG_ERR:
				DESC_EVENT ( config, "IBV_EVENT_PATH_MIG_ERR", "A connection failed to migrate to the alternate path", 1, 0 );
				break;

				/* Catastrophic error happened in the HCA, close the process NOW */
			case IBV_EVENT_DEVICE_FATAL:
				DESC_EVENT ( config, "IBV_EVENT_DEVICE_FATAL", "Error occurred on a QP and it transitioned to error state", 1, 1 );
				break;

				/* This event is trigered when a port state is modified
				 * ( generated only if IBV_DEVICE_PORT_ACTIVE_EVENT is set in the dev_cap.device_cap_flag ) */
			case IBV_EVENT_PORT_ACTIVE:
				DESC_EVENT ( config, "IBV_EVENT_PORT_ACTIVE", "Link became active on a port", 1, 0 );
				break;

				/* Trigerred when a port is disconnected (link failure) */
			case IBV_EVENT_PORT_ERR:
				DESC_EVENT ( config, "IBV_EVENT_PORT_ERR", "Link became unavailable on a port ", 1, 1 );
				break;

				/* LID has been changed values cached from ibv_query_port() must be flushed */
			case IBV_EVENT_LID_CHANGE:
				DESC_EVENT ( config, "IBV_EVENT_LID_CHANGE", "LID was changed on a port", 1, 0 );
				break;

				/* Pkey has been changed */
			case IBV_EVENT_PKEY_CHANGE:
				DESC_EVENT ( config, "IBV_EVENT_PKEY_CHANGE", "P_Key table was changed on a port", 1, 0 );
				break;

				/* SM was changed on a port */
			case IBV_EVENT_SM_CHANGE:
				DESC_EVENT ( config, "IBV_EVENT_SM_CHANGE", "SM was changed on a port", 1, 0 );
				break;

				/* SRQ encountered a serious error */
			case IBV_EVENT_SRQ_ERR:
				DESC_EVENT ( config, "IBV_EVENT_SRQ_ERR", "Error occurred on an SRQ", 1, 1 );
				break;

				/* event triggered when the limit given by ibv_srq_credit_thread_limit is reached */
			case IBV_EVENT_SRQ_LIMIT_REACHED:
				DESC_EVENT ( config, "IBV_EVENT_SRQ_LIMIT_REACHED", "SRQ limit was reached", 1, 0 );

				/* We re-arm the limit for the SRQ. */
				config->max_srq_ibufs_posted += 100;
				_mpc_lowcomm_ib_ibuf_srq_post ( rail_ib );

				mod_attr.srq_limit = config->srq_credit_thread_limit;

				mpc_common_debug( "Update with max_qr %d and srq_limit %d",
				             config->max_srq_ibufs_posted, mod_attr.srq_limit );
				rc = ibv_modify_srq ( device->srq, &mod_attr, IBV_SRQ_LIMIT );
				assume ( rc == 0 );
				break;

				/* The QP associated with an SRQ transitionned to the ERR state and no more WQE will be consumed from the SRQ by this QP */
			case IBV_EVENT_QP_LAST_WQE_REACHED:
				DESC_EVENT ( config, "IBV_EVENT_QP_LAST_WQE_REACHED", "Last WQE Reached on a QP associated with an SRQ CQ events", 1, 1 );
				break;

				/* SM requested re-registration (only is supported by device) */
			case IBV_EVENT_CLIENT_REREGISTER:
				DESC_EVENT ( config, "IBV_EVENT_CLIENT_REREGISTER", "SM sent a CLIENT_REREGISTER request to a port CA events", 1, 0 );
				break;

			default:
				DESC_EVENT ( config, "UNKNOWN_EVENT", "unknown event received", 1, 0 );
				break;
		}

		/* Acknowledge ASYNC event */
		ibv_ack_async_event ( &event );
	}

	mpc_common_nodebug ( "Async thread exits..." );
	return NULL;
}


/********************************************************************/
/* Async thread Init                                                */
/********************************************************************/

#ifdef MPC_Threads
	static mpc_thread_t async_pidt;
#else
	static pthread_t async_pidt;
#endif

void _mpc_lowcomm_ib_async_init ( sctk_rail_info_t *rail )
{
	_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );

	/* Activate or not the async thread */
	if ( config->async_thread )
	{
#ifdef MPC_Threads
		mpc_thread_attr_t attr;
		mpc_thread_attr_init ( &attr );
		/* The thread *MUST* be in a system scope (calls a blocking call) */
		mpc_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );
		mpc_thread_core_thread_create ( &async_pidt, &attr, async_thread, rail );
#else
		pthread_create ( &async_pidt, NULL, async_thread, rail );
#endif
	}
}

void _mpc_lowcomm_ib_async_finalize( sctk_rail_info_t *rail)
{
	_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );

	if(config->async_thread)
	{
#ifdef MPC_Threads
		mpc_thread_kill(&async_pidt, 15);
		async_pidt = NULL;
#else
		pthread_kill(async_pidt, 15);
#endif
	}


}
