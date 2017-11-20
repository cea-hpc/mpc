#ifdef MPC_USE_PORTALS

#include "sctk_route.h"
#include "sctk_ptl_cm.h"
#include "sctk_ptl_types.h"

void sctk_ptl_cm_recv_message(sctk_ptl_rail_info_t* srail, sctk_ptl_event_t ev)
{
	not_implemented();
}

void sctk_ptl_cm_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	not_implemented();
}

void sctk_ptl_cm_send_message_forward(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	not_implemented();
	/* first: get back the data before routing it */

	/* then, send the message */
	sctk_ptl_cm_send_message(msg, endpoint);
}
#endif
