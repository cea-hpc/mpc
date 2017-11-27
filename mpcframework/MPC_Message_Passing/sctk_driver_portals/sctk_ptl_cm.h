#ifdef MPC_USE_PORTALS
#ifndef __SCTK_PTL_CM_H_
#define __SCTK_PTL_CM_H_

void sctk_ptl_cm_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void sctk_ptl_cm_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
void sctk_ptl_cm_send_message_forward(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
#endif /* ifndef __SCTK_PTL_CM_H_ */
#endif
