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
#include <sctk_multirail_ib.h>
#include <sctk_route.h>
#include <sctk_tcp.h>
#include <opa_primitives.h>

#define NB_RAILS 1
static sctk_rail_info_t** rails = NULL;

static void
sctk_network_send_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i ;
  if(sctk_prepare_send_message_to_network_reorder(msg) == 0){
    /*
      Reordering available : we can use multirail
    */
    i = 0;
//  if(msg->sctk_msg_get_msg_size > 32768){
//    i = 1;
//  }
  } else {
    /*
      No reodering: we can't use multirail fall back to rail 0
    */
    i = 0;
  }
  rails[i]->send_message(msg,rails[i]);
}

static void
sctk_network_notify_recv_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_recv_message(msg,rails[i]);
  }
}

static void
sctk_network_notify_matching_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_matching_message(msg,rails[i]);
  }
}

static void
sctk_network_notify_perform_message_multirail_ib (int remote){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_perform_message(remote,rails[i]);
  }
}

static void
sctk_network_notify_idle_message_multirail_ib (){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_idle_message(rails[i]);
  }
}

static void
sctk_network_notify_any_source_message_multirail_ib (){
  int i;
  for(i = 0; i < NB_RAILS; i++){
    rails[i]->notify_any_source_message(rails[i]);
  }
}

static
void sctk_send_message_from_network_multirail_ib (sctk_thread_ptp_message_t * msg){
  if(sctk_send_message_from_network_reorder(msg) != 0){
    /*
      No reordering
    */
    sctk_send_message_try_check(msg,1);
  }
}

/************ INIT ****************/
static
void sctk_network_init_multirail_ib_all(char* name, char* topology){
  int i;

/*   sctk_set_dynamic_reordering_buffer_creation(); */
  sctk_route_set_rail_nb(NB_RAILS);
  rails = sctk_malloc(NB_RAILS*sizeof(sctk_rail_info_t*));
  memset(rails, 0, NB_RAILS*sizeof(sctk_rail_info_t*));

  /* Fallback IB network */
  i = 0;
  rails[i] = sctk_route_get_rail(i);
  rails[i]->rail_number = i;
  rails[i]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[i],topology);
  /* Allows on-demand connexions on rail */
  sctk_route_set_on_demand_in_rail(rails[i]);
  sctk_network_init_ib(rails[i]);
  sctk_network_init_polling_thread (rails[i], topology);

  sctk_network_send_message_set(sctk_network_send_message_multirail_ib);
  sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_multirail_ib);
  sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_multirail_ib);
  sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_multirail_ib);
  sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_multirail_ib);
  sctk_network_notify_any_source_message_set(sctk_network_notify_any_source_message_multirail_ib);
}
void sctk_network_init_multirail_ib(char* name, char* topology){
  sctk_network_init_multirail_ib_all(name,topology);
}
