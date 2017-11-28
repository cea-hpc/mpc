#ifdef MPC_USE_PORTALS
#ifndef SCTK_PTL_TOOLKIT_H_
#define SCTK_PTL_TOOLKIT_H_

#include "sctk_route.h"
void sctk_ptl_create_ring(sctk_rail_info_t *rail);
sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, int dest);
void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state);
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, int threshold);
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, int threshold);
void sctk_ptl_free_memory(void* msg);
void sctk_ptl_message_copy(sctk_message_to_copy_t);
void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, int comm_idx, size_t comm_size);
void sctk_ptl_init_interface(sctk_rail_info_t* rail);
void sctk_ptl_fini_interface(sctk_rail_info_t* rail);

void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
void sctk_ptl_notify_recv(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail);

#endif
#endif
