#ifdef MPC_USE_PORTALS

#include "sctk_ptl_async.h"
#include "sctk_ptl_iface.h"

static void* __async_thread_routine(void* arg)
{
	sctk_ptl_rail_info_t* srail = &((sctk_rail_info_t*)arg)->network.ptl;
	sctk_ptl_event_t ev;
	int ret;
	while(1)
	{
		ret = sctk_ptl_eq_poll_md(srail, &ev);
		if(ret == PTL_OK)
		{
			sctk_debug("EVENT MD: %s (fail_type = %d, target=%s)",sctk_ptl_event_decode(ev), ev.ni_fail_type, SCTK_PTL_STR_LIST(ev.ptl_list));
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
					break;
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

void sctk_ptl_async_start(sctk_rail_info_t* rail)
{
	sctk_thread_attr_t attr;

	/* Launch the polling thread */
	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );
	sctk_user_thread_create ( &rail->network.ptl.async_tid, &attr, ( void * ( * ) ( void * ) ) __async_thread_routine , rail );
}

void sctk_ptl_async_stop(sctk_rail_info_t* rail)
{
	sctk_thread_kill(rail->network.ptl.async_tid, SIGTERM);
	rail->network.ptl.async_tid = NULL;
}
#endif
