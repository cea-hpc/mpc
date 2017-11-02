#include "sctk_route.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"

int sctk_ptl_rdv_poll(sctk_ptl_rail_info_t* srail, int threshold)
{

	return 0;
}

void sctk_ptl_rdv_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	/* pack the data if necessary */
	void* user_data;
	if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		user_data = msg->tail.message.contiguous.addr;
	}
	else
	{
		user_data = sctk_malloc(SCTK_MSG_SIZE(msg));
		sctk_net_copy_in_buffer(msg, user_data);
	}

}
