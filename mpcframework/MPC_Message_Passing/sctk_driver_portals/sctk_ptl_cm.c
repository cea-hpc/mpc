#include "sctk_route.h"
#include "sctk_ptl_cm.h"

int sctk_ptl_cm_poll(sctk_ptl_rail_info_t* srail)
{
}

void sctk_ptl_cm_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
}

void sctk_ptl_cm_send_message_forward(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	/* first: get back the data before routing it */

	/* then, send the message */
	sctk_ptl_cm_send_message(msg, endpoint);
}
