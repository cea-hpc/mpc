#ifdef MPC_USE_PORTALS
#ifndef __SCTK_PTL_EAGER_H_
#define __SCTK_PTL_EAGER_H_
void sctk_ptl_eager_recv_message(sctk_ptl_rail_info_t* srail, sctk_ptl_event_t ev);
void sctk_ptl_eager_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
#endif /* ifndef __SCTK_PTL_EAGER_H_ */
#endif
