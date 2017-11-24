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
#include "sctk_ptl_toolkit.h"

/**
 * Add a route to the multirail, for Portals use.
 * \param[in] dest the remote process rank
 * \param[in] id the associated Portals-related ID
 * \param[in] origin route type
 * \param[in] state route initial state
 */
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

/**
 * Routine called when the current process posted a RECV call.
 * As we are in a single-rail configuration, we can optimize msg incomings.
 * \param[in] msg the local request
 * \param[in] rail the Portals rail
 */
void sctk_ptl_notify_recv(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail)
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
	sctk_assert(msg);
	sctk_assert(rail);
	sctk_assert(srail);
	sctk_assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg)));
	
	/* Build the match_bits */
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_TASK(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);

	/* apply the mask, depending on request infos
	 * The UID is always ignored, it we only be used to make RDV requests consistent
	 */
	ign.data.tag  = (match.data.tag  == SCTK_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank = (match.data.rank == SCTK_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;
	ign.data.uid  = SCTK_PTL_IGN_UID;

	/* is the message contiguous ? */
	if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		start = msg->tail.message.contiguous.addr;
	}
	else
	{
		/* if not, we map a temporary buffer, ready for network serialized data */
		start = sctk_malloc(SCTK_MSG_SIZE(msg));
		sctk_net_copy_in_buffer(msg, start);
		/* and we flag the copy to 1, to remember some buffers will have to be recopied */
		msg->tail.ptl.copy = 1;
	}

	/* complete the ME data, this ME will be appended to the PRIORITY_LIST */
	size     = SCTK_MSG_SIZE(msg);
	pte      = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));
	flags    = SCTK_PTL_ME_PUT_FLAGS;
	user_ptr = sctk_ptl_me_create(start, size, remote, match, ign, flags); assert(user_ptr);

	sctk_assert(user_ptr);
	sctk_assert(pte);

	user_ptr->msg          = msg;
	user_ptr->list         = SCTK_PTL_PRIORITY_LIST;
	msg->tail.ptl.user_ptr = user_ptr;
	sctk_ptl_me_register(srail, user_ptr, pte);
	
	sctk_debug("PORTALS: POSTED-RECV from %d (idx=%llu, match=%s, ign=%llu start=%p, sz=%llu)", SCTK_MSG_SRC_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), __sctk_ptl_match_str(malloc(32), 32, ign.raw), start, size);
}

/**
 * Send a message through Portals API.
 * This is the main entry point for sending message, when requested by the low_level_comm
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	int process_rank            = sctk_get_process_rank();
	sctk_ptl_rail_info_t* srail = &endpoint->rail->network.ptl;
	int nb_entries              = sctk_atomics_load_int(&srail->nb_entries);

	sctk_assert(SCTK_PTL_PTE_EXIST(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg)));

	/* specific cases: control messages */
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		sctk_ptl_cm_send_message(msg, endpoint);
		return;
	}

	/* if the message is lower than the fixed boundary, send it as an eager */
	if(SCTK_MSG_SIZE(msg) < srail->eager_limit)
	{
		sctk_ptl_eager_send_message(msg, endpoint);
	}
	else
	{
		sctk_ptl_rdv_send_message(msg, endpoint);
	}
}

/**
 * Routine called periodically to notify upper-layer from incoming messages.
 * \param[in] rail the Portals rail
 * \param[in] threshold max number of messages to poll, if any
 */
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, int threshold)
{
	sctk_ptl_event_t ev;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_local_data_t* user_ptr;
	sctk_ptl_pte_t* cur_pte;

	size_t i = 0, size = sctk_atomics_load_int(&srail->nb_entries) + SCTK_PTL_PTE_HIDDEN;
	int ret, max = 0;
	while(max++ < threshold && i < size)
	{
		cur_pte = SCTK_PTL_PTE_ENTRY(srail->pt_table, ((i++)%size));
		if(!cur_pte)
		{
			/* current PTE is empty, try the next one */
			continue;
		}

		ret = sctk_ptl_eq_poll_me(srail, cur_pte , &ev);
		user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;

		if(ret == PTL_OK)
		{
			sctk_debug("PORTALS: EQS EVENT '%s' idx=%d, match=%s from %s, sz=%llu)!", sctk_ptl_event_decode(ev), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), SCTK_PTL_STR_LIST(((sctk_ptl_local_data_t*)ev.user_ptr)->list), ev.mlength);
			/* we only consider Portals-succeded events */
			if(ev.ni_fail_type != PTL_NI_OK) sctk_fatal("Failed targeted event !!");
			switch(ev.type)
			{
				case PTL_EVENT_GET: /* a Get() reached the local process */
					break;
				case PTL_EVENT_GET_OVERFLOW: /* a previous received GET matched a just appended ME */
					not_reachable();
					break;

				case PTL_EVENT_PUT_OVERFLOW: /* a previous received PUT matched a just appended ME */
				case PTL_EVENT_PUT: /* a Put() reached the local process */
					/* we don't care about unexpected messaged reaching the OVERFLOW_LIST, we will just wait for their local counter-part */
					if(user_ptr->list == SCTK_PTL_OVERFLOW_LIST)
					{
					sctk_ptl_pte_t fake = (sctk_ptl_pte_t){.idx = ev.pt_index};
					sctk_ptl_me_feed_overflow(srail,  &fake,  srail->eager_limit, 1);
						break;
					}
					/* Multiple scenario can trigger this event:
					 *  1 - An incoming header put() (RDV or eager)
					 *  2 - an incoming RDMA request
					 *  3 - an incoming CM request
					 *  4 - an incoming FLOW_CTRL request
					 *  Selection is done depending on the target PTE.
					 */
					{
						/* indexes from 0 to SCTK_PTL_PTE_HIDDEN-1 maps RECOVERY, CM & RDMA queues
						 * indexes from SCTK_PTL_PTE_HIDDEN to N maps communicators
						 */
						if(ev.pt_index >= SCTK_PTL_PTE_HIDDEN) /* 'normal header */
						{
							/* ev.rlength is the size as requested by the remote */
							if(ev.rlength < rail->network.ptl.eager_limit)
							{
								sctk_ptl_eager_recv_message(rail, ev);
							}
							else
							{
								sctk_ptl_rdv_recv_message(rail, ev);
							}
							break;
						}
						else if(ev.pt_index == SCTK_PTL_PTE_CM) /* Control message */
						{
							sctk_ptl_cm_recv_message(srail, ev);
							break;
						}
						not_reachable();
					}

				case PTL_EVENT_ATOMIC: /* an Atomic() reached the local process */
					assume(ev.pt_index == SCTK_PTL_PTE_RDMA); /* RDMA */
					sctk_debug("It is an RDMA message !");
					not_implemented();
					break;
				case PTL_EVENT_ATOMIC_OVERFLOW: /* a previously received ATOMIC matched a just appended one */
					not_implemented();
					break;

				case PTL_EVENT_FETCH_ATOMIC: /* a FetchAtomic() reached the local process */
					assume(ev.pt_index == SCTK_PTL_PTE_RDMA); /* RDMA */
					sctk_debug("It is an RDMA message !");
					not_implemented();
					break;

				case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
					not_implemented();
					break;
				case PTL_EVENT_PT_DISABLED: /* ERROR: The local PTE is disabeld (FLOW_CTRL) */
					break;
				case PTL_EVENT_LINK: /* MISC: A new ME has been linked, (maybe not useful) */
					not_reachable();
					break;

				case PTL_EVENT_AUTO_UNLINK: /* an USE_ONCE ME has been automatically unlinked */
					break;
				case PTL_EVENT_AUTO_FREE: /* an USE_ONCE ME can be now reused */
					/* can be helpful to know if the OVERFLOW ME don't have
					 * remaining unexpected header to match */
					break;
				case PTL_EVENT_SEARCH: /* a PtlMESearch completed */
					/* probably nothing to do here */
					break;
				default:
					sctk_fatal("Portals ME event not recognized: %d", ev.type);
					break;
			}
		}
	}
}

/**
 * Make locally-initiated request progress.
 * Here, only MD-specific events are processed.
 * \param[in] arg the Portals rail, to cast before use.
 */
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, int threshold)
{
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_event_t ev;
	sctk_ptl_local_data_t* user_ptr;
	sctk_thread_ptp_message_t* msg;
	int ret, max = 0;
	while(max++ < threshold)
	{
		ret = sctk_ptl_eq_poll_md(srail, &ev);

		if(ret == PTL_OK)
		{
			user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
			msg = (sctk_thread_ptp_message_t*) user_ptr->msg;
			sctk_assert(user_ptr != NULL);
			sctk_assert(msg != NULL);
			sctk_assert(msg->tail.ptl.user_ptr == user_ptr);
			sctk_debug("PORTALS: ASYNC MD '%s' from %s",sctk_ptl_event_decode(ev), SCTK_PTL_STR_LIST(ev.ptl_list));
			/* we only care about Portals-sucess events */
			if(ev.ni_fail_type != PTL_NI_OK) sctk_fatal("Failed event %d: %d!", ev.type, ev.ni_fail_type);
			switch(ev.type)
			{
				case PTL_EVENT_SEND: /* a Put() left the local process */
					/* Here, nothing to do for now */
					break;
				case PTL_EVENT_ACK: /* a Put() reached the remote process (don't mean it suceeded !) */
					/* Here, it depends on message type (in case of success) 
					 *   1 - it is a "normal" message" --> RDV || eager ?
					 *   2 - it is a routing CM --> forward the put() (OD ?)
					 *   3 - It is a RDMA message --> nothing to do (probably)
					 *   4 - it is a recovery (flow-control) message
					 */
					if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg))) /* Control message */
					{
						break;
					}
					else if(SCTK_MSG_COMMUNICATOR(msg) != SCTK_ANY_COMM) /* 'normal header */
					{
						/* was the msg a eager one ? */
						if(SCTK_MSG_SIZE(msg) < rail->network.ptl.eager_limit)
						{
							/* In case of MD request, this attribute means that
							 * the user buffer has been copied in a temporary one 
							 * and need to be freed (ex: non-contiguous request
							 */
							sctk_ptl_md_release(user_ptr);
							if(msg->tail.ptl.copy)
							{
								sctk_free(user_ptr->slot.md.start);
							}
							/* tag the message as completed */
							sctk_complete_and_free_message((sctk_thread_ptp_message_t*)user_ptr->msg);
						}
						else
						{
							/* it is a RDV msg, we wait for the remote Get().
							 * Probably nothing to do
							 */
						}
						
						break;
					}
					else /* RDMA (should have an 'else if' construct */
					{
						break;
					}

					not_reachable();
				case PTL_EVENT_REPLY: /* a Get() reply reached the local process */
					/* It depends on the message type (=Portals entry)
					 *   1 - it is a "normal" message --> RDV: complete_and_free
					 *   2 - it is a RDMA message --> nothing to to
					 *   3 - A CM could be big enough to ben send through RDV protocool ?
					 */
					break;
				default:
					sctk_fatal("Unrecognized MD event: %d", ev.type);
					break;
			}

		}
	}
}

/**
 * Create the initial Portals topology: the ring.
 * \param[in] rail the just-initialized Portals rail
 */
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

/** 
 * Retrieve the ptl_process_id object, associated with a given MPC process rank.
 * \param[in] rail the Portals rail
 * \param[in] dest the MPC process rank
 * \return the Portals process id
 */
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

void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, int comm_idx, size_t comm_size)
{
	if(!SCTK_PTL_PTE_EXIST(srail->pt_table, comm_idx))
	{
		sctk_ptl_pte_t* new_entry = sctk_malloc(sizeof(sctk_ptl_pte_t));
		sctk_ptl_pte_create(srail, new_entry, comm_idx + SCTK_PTL_PTE_HIDDEN);
		sctk_warning("PORTALS: register comm %d (size: %d)", comm_idx, comm_size);

	}
}

/**
 * Initialize the Portals API and main structs.
 * \param[in,out] rail the Portals rail
 */
void sctk_ptl_init_interface(sctk_rail_info_t* rail)
{
	rail->network.ptl             = sctk_ptl_hardware_init();
	rail->network.ptl.eager_limit = rail->runtime_config_driver_config->driver.value.portals.eager_limit;
	sctk_atomics_store_int(&rail->network.ptl.nb_entries, rail->runtime_config_driver_config->driver.value.portals.min_comms);

	sctk_ptl_software_init( &rail->network.ptl);
}

/**
 * Close the Portals API.
 * \param[in] rail the rail to close
 */
void sctk_ptl_fini_interface(sctk_rail_info_t* rail)
{
}

void sctk_ptl_free_memory(void* msg)
{
}

#endif
