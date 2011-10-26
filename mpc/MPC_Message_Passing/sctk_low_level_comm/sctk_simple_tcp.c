/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */


#include <sctk_debug.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_low_level_comm.h>
#include <sctk_simple_tcp.h>
#include <sctk_route.h>
#include <sctk_tcp.h>

static sctk_rail_info_t* rail_0 = NULL;

static void 
sctk_network_send_message_simple_tcp (sctk_thread_ptp_message_t * msg){
  rail_0->send_message(msg,rail_0);
}

static void 
sctk_network_notify_recv_message_simple_tcp (sctk_thread_ptp_message_t * msg){
  rail_0->notify_recv_message(msg,rail_0);
}

static void 
sctk_network_notify_matching_message_simple_tcp (sctk_thread_ptp_message_t * msg){
  rail_0->notify_matching_message(msg,rail_0);
}

static void 
sctk_network_notify_perform_message_simple_tcp (int remote){
  rail_0->notify_perform_message(remote,rail_0);
}

static void 
sctk_network_notify_idle_message_simple_tcp (){
  rail_0->notify_idle_message(rail_0);
}

static void 
sctk_network_notify_any_source_message_simple_tcp (){
  rail_0->notify_any_source_message(rail_0);
}


/************ INIT ****************/
void sctk_network_init_simple_tcp(char* name){
  static char net_name[4096];

  sctk_route_set_rail_nb(1);

  rail_0 = sctk_route_get_rail(0);
  rail_0->rail_number = 0;
  rail_0->send_message_from_network = sctk_send_message;
  sctk_network_init_tcp(rail_0,0);
  sprintf(net_name,"[0:%s]",rail_0->network_name);

  sctk_network_send_message_set(sctk_network_send_message_simple_tcp);
  sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_simple_tcp);
  sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_simple_tcp);
  sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_simple_tcp);
  sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_simple_tcp);
  sctk_network_notify_any_source_message_set(sctk_network_notify_any_source_message_simple_tcp);
  sctk_network_mode = net_name;
}
