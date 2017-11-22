#ifdef MPC_USE_PORTALS

#include "sctk_ptl_async.h"
#include "sctk_ptl_iface.h"

/**
 * The asynchronous polling thread make locally-initiated request progress.
 * Here, only MD-specific events are processed.
 * \param[in] arg the Portals rail, to cast before use.
 * \return NULL
 */
static void* __async_thread_routine(void* arg)
{
	sctk_rail_info_t* rail = (sctk_rail_info_t*)arg;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_event_t ev;
	sctk_ptl_local_data_t* user_ptr;
	sctk_thread_ptp_message_t* msg;
	int ret;
	while(1)
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

	return NULL;

}

/**
 * Start the asynchronous thread.
 * \param[in] rail the Portals rail
 */
void sctk_ptl_async_start(sctk_rail_info_t* rail)
{
	sctk_thread_attr_t attr;

	/* Launch the polling thread */
	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );
	sctk_user_thread_create ( &rail->network.ptl.async_tid, &attr, ( void * ( * ) ( void * ) ) __async_thread_routine , rail );
}

/**
 * Stop the asynchronous polling thread.
 * \param[in] rail the Portals rail, to retrieve thread ID
 */
void sctk_ptl_async_stop(sctk_rail_info_t* rail)
{
	sctk_thread_kill(rail->network.ptl.async_tid, SIGTERM);
	rail->network.ptl.async_tid = NULL;
}
#endif
