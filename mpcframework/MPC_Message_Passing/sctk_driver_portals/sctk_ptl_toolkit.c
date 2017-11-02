#ifdef MPC_USE_PORTALS

#include "sctk_route.h"
#include "sctk_net_tools.h"
#include "sctk_io_helper.h" /* for MAX_STRING_SIZE */
#include "sctk_ptl_types.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_eager.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_cm.h"

void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state)
{
	sctk_endpoint_t * route;

	route = sctk_malloc(sizeof(sctk_endpoint_t));
	assert(route);

	sctk_endpoint_init(route, dest, rail, origin);
	sctk_endpoint_set_state(route, state);

	route->data.ptl.dest = id;

	if(origin == ROUTE_ORIGIN_STATIC)
	{
		sctk_rail_add_static_route (  rail, dest, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, dest, route );
	}
}

void sctk_ptl_recv_message(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail)
{
	sctk_ptl_rail_info_t* srail     = &rail->network.ptl;
	void* start                     = NULL;
	size_t size                     = 0;
	sctk_ptl_matchbits_t match      = SCTK_PTL_MATCH_INIT;
	sctk_ptl_matchbits_t ign        = SCTK_PTL_MATCH_INIT;
	int flags                       = 0;
	sctk_ptl_pte_t* pte             = NULL;
	sctk_ptl_local_data_t* user_ptr = NULL;
	sctk_ptl_id_t remote            = SCTK_PTL_ANY_PROCESS;

	/* pre-actions */
	assert(msg);
	assert(rail);
	assert(srail);
	assert(SCTK_MSG_COMMUNICATOR(msg) >= 0 && SCTK_MSG_COMMUNICATOR(msg) < srail->nb_entries);
	
	/* Build the match_bits */
	match.data.tag = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_TASK(msg);
	ign.data.tag  = (match.data.tag  == SCTK_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank = (match.data.rank == SCTK_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;

	assert(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS); /* temp */
	start = msg->tail.message.contiguous.addr;
	size = SCTK_MSG_SIZE(msg);
	pte = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	flags = SCTK_PTL_ME_PUT_FLAGS;

	user_ptr = sctk_ptl_me_create(start, size, remote, match, ign, flags); assert(user_ptr);
	user_ptr->msg = msg;
	user_ptr->list = SCTK_PTL_OVERFLOW_LIST;
	sctk_ptl_me_register(srail, user_ptr, pte);
	/* We've done here... anything else will be handled when the event will be polled */
	sctk_error("Posted a recv from %d!", SCTK_MSG_SRC_PROCESS(msg));
}

void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	int process_rank = sctk_get_process_rank();
	sctk_ptl_rail_info_t* srail = &endpoint->rail->network.ptl;
	sctk_ptl_route_info_t* infos = &endpoint->data.ptl;
	sctk_ptl_id_t dest;


	/* If it is a control_message: send it through a Put() (can be a routed msg) */
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		sctk_ptl_cm_send_message(msg, endpoint);
		return;
	}

	if(SCTK_MSG_SIZE(msg) < srail->eager_limit)
	{
		sctk_ptl_eager_send_message(msg, endpoint);
	}
	else
	{
		sctk_ptl_rdv_send_message(msg, endpoint);
	}
	sctk_error("Posted a send to %d!", SCTK_MSG_DEST_PROCESS(msg));
}

void sctk_ptl_poll_initiated(sctk_rail_info_t* rail, int threshold)
{
	int ret = -1;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_event_t ev;
	sctk_ptl_eq_poll_md(srail, &ev);
}

void sctk_ptl_poll_targeted(sctk_rail_info_t* rail, int threshold)
{
	int ret = -1;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	ret = sctk_ptl_rdv_poll(srail);
	ret = sctk_ptl_eager_poll(srail);

}

void sctk_ptl_poll_cm(sctk_rail_info_t* rail)
{
	int ret = -1;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	ret = sctk_ptl_cm_poll(srail);
}

void sctk_ptl_create_ring ( sctk_rail_info_t *rail )
{
	int right_rank, left_rank, tmp_ret;
	sctk_ptl_id_t right_id, left_id;
	char right_rank_connection_infos[MAX_STRING_SIZE];
	char left_rank_connection_infos[MAX_STRING_SIZE];
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;

	/** Portals initialization : Ring topology.
	 * 1. bind to right process through PMI
	 * 2. if process number > 2 create a route to the process to the left
	 * => Bidirectional ring
	 */
	
	/* serialize the id_t to a string, compliant with PMI handling */
	srail->connection_infos_size = sctk_ptl_data_serialize(
			&srail->id,              /* the process it to serialize */
			sizeof (sctk_ptl_id_t),  /* size of the Portals ID struct */
			srail->connection_infos, /* the string to store the serialization */
			MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	/* register the serialized id into the PMI */
	tmp_ret = sctk_pmi_put_connection_info (
			srail->connection_infos,      /* the string to publish */
			srail->connection_infos_size, /* string size */
			rail->rail_number             /* rail ID: PMI tag */
	);
	assert(tmp_ret == 0);

	/* what are my neighbour ranks ? */
	right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
	left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

	/* wait for each process to register its own serialized ID */
	sctk_pmi_barrier();

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			right_rank_connection_infos, /* the recv buffer */
			MAX_STRING_SIZE,             /* the recv buffer max size */
			rail->rail_number,           /* rail IB: PMI tag */
			right_rank                   /* which process we are targeting */
	);
	assert(tmp_ret == 0);
	
	/* de-serialize the string to retrive the real ptl_id_t */
	sctk_ptl_data_deserialize(
			right_rank_connection_infos, /* the buffer containing raw data */
			&right_id,                   /* the target struct */
			sizeof ( right_id )          /* target struct size */
	);

	//if we need an initialization to the left process (bidirectional ring)
	if(sctk_process_number > 2)
	{

		/* retrieve the left neighbour id struct */
		tmp_ret = sctk_pmi_get_connection_info (
				left_rank_connection_infos, /* the recv buffer */
				MAX_STRING_SIZE,             /* the recv buffer max size */
				rail->rail_number,           /* rail IB: PMI tag */
				left_rank                   /* which process we are targeting */
				);
		assert(tmp_ret == 0);

		/* de-serialize the string to retrive the real ptl_id_t */
		sctk_ptl_data_deserialize(
				left_rank_connection_infos, /* the buffer containing raw data */
				&left_id,                   /* the target struct */
				sizeof ( left_id )          /* target struct size */
				);
	}

	/* add routes */
	sctk_ptl_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	if ( sctk_process_number > 2 )
	{
		sctk_ptl_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	}

	//Wait for all processes to complete the ring topology init */
	sctk_pmi_barrier();
}

sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, int dest)
{
	int tmp_ret;
	char connection_infos[MAX_STRING_SIZE];
	sctk_ptl_id_t id = SCTK_PTL_ANY_PROCESS;

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			connection_infos,  /* the recv buffer */
			MAX_STRING_SIZE,   /* the recv buffer max size */
			rail->rail_number, /* rail IB: PMI tag */
			dest               /* which process we are targeting */
	);

	assert(tmp_ret == 0);
	
	sctk_ptl_data_deserialize(
			connection_infos, /* the buffer containing raw data */
			&id,               /* the target struct */
			sizeof (id )      /* target struct size */
	);

	return id;
}

void sctk_ptl_init_interface(sctk_rail_info_t* rail)
{
	rail->network.ptl = sctk_ptl_hardware_init();
	rail->network.ptl.eager_limit = rail->runtime_config_driver_config->driver.value.portals.eager_limit;
	rail->network.ptl.nb_entries = rail->runtime_config_driver_config->driver.value.portals.max_comms;

	(void)sctk_ptl_software_init( &rail->network.ptl);

	sctk_assert(rail->network.ptl.pt_entries);
	sctk_assert(rail->network.ptl.nb_entries == rail->network.ptl.nb_entries);

}

void sctk_ptl_fini_interface(sctk_rail_info_t* rail)
{
}

#endif
