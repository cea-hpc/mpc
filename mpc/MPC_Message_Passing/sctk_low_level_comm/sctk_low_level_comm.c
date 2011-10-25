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

#include <sctk_low_level_comm.h>
#include <sctk.h>
#include <sctk_pmi.h>
#include <string.h>

 /*Networks*/
#include <sctk_simple_tcp.h>

int sctk_is_net_migration_available(){
  if(sctk_migration_mode == 1){
    not_implemented();
  }
}

/********** SEND ************/
static void sctk_network_send_message_default (sctk_thread_ptp_message_t * msg){
  not_implemented();
}
static void (*sctk_network_send_message_ptr) (sctk_thread_ptp_message_t *) = 
  sctk_network_send_message_default;
void sctk_network_send_message (sctk_thread_ptp_message_t * msg){
  sctk_network_send_message_ptr(msg);
}
void sctk_network_send_message_set(void (*sctk_network_send_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_send_message_ptr = sctk_network_send_message_val;
}

/********** NOTIFY_RECV ************/
static void sctk_network_notify_recv_message_default (sctk_thread_ptp_message_t * msg){

}
static void (*sctk_network_notify_recv_message_ptr) (sctk_thread_ptp_message_t *) = 
  sctk_network_notify_recv_message_default;
void sctk_network_notify_recv_message (sctk_thread_ptp_message_t * msg){
  sctk_network_notify_recv_message_ptr(msg);
}
void sctk_network_notify_recv_message_set(void (*sctk_network_notify_recv_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_notify_recv_message_ptr = sctk_network_notify_recv_message_val;
}

/********** NOTIFY_MATCHING ************/
static void sctk_network_notify_matching_message_default (sctk_thread_ptp_message_t * msg){

}
static void (*sctk_network_notify_matching_message_ptr) (sctk_thread_ptp_message_t *) = 
  sctk_network_notify_matching_message_default;
void sctk_network_notify_matching_message (sctk_thread_ptp_message_t * msg){
  sctk_network_notify_matching_message_ptr(msg);
}
void sctk_network_notify_matching_message_set(void (*sctk_network_notify_matching_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_notify_matching_message_ptr = sctk_network_notify_matching_message_val;
}

/********** NOTIFY_PERFORM ************/
static void sctk_network_notify_perform_message_default (int msg){

}
static void (*sctk_network_notify_perform_message_ptr) (int) = 
  sctk_network_notify_perform_message_default;
void sctk_network_notify_perform_message (int msg){
  sctk_network_notify_perform_message_ptr(msg);
}
void sctk_network_notify_perform_message_set(void (*sctk_network_notify_perform_message_val) (int)){
  sctk_network_notify_perform_message_ptr = sctk_network_notify_perform_message_val;
}

/********** NOTIFY_IDLE ************/
static void sctk_network_notify_idle_message_default (){
  
}
static void (*sctk_network_notify_idle_message_ptr) () = 
  sctk_network_notify_idle_message_default;
void sctk_network_notify_idle_message (){
  sctk_network_notify_idle_message_ptr();
}
void sctk_network_notify_idle_message_set(void (*sctk_network_notify_idle_message_val) ()){
  sctk_network_notify_idle_message_ptr = sctk_network_notify_idle_message_val;
}

/********** NOTIFY_ANY_SOURCE ************/
static void sctk_network_notify_any_source_message_default (){
  
}
static void (*sctk_network_notify_any_source_message_ptr) () = 
  sctk_network_notify_any_source_message_default;
void sctk_network_notify_any_source_message (){
  sctk_network_notify_any_source_message_ptr();
}
void sctk_network_notify_any_source_message_set(void (*sctk_network_notify_any_source_message_val) ()){
  sctk_network_notify_any_source_message_ptr = sctk_network_notify_any_source_message_val;
}

#define TRY_DRIVER(dr_name,func) if(strcmp(name,SCTK_STRING(dr_name)) == 0){func(name);} else { (void)(0)
#define DEFAUT_DRIVER() sctk_network_not_implemented(name);}(void)(0)

static void sctk_network_not_implemented(char* name){
  sctk_error("Network %s not available",name);
  sctk_abort();
}

void
sctk_net_init_driver (char *name)
{
  if(sctk_process_number > 1){
    sctk_pmi_get_process_rank(&sctk_process_rank);
    sctk_pmi_get_process_number(&sctk_process_number);

    sctk_nodebug("Use network %s",name);

    TRY_DRIVER(tcp,sctk_network_init_simple_tcp);
    DEFAUT_DRIVER();
  }
}
