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
#include "sctk_checksum.h"

 /*Networks*/
#include <sctk_simple_tcp.h>
#include <sctk_multirail_tcp.h>
#include <sctk_multirail_ib.h>
#include <sctk_route.h>

int sctk_is_net_migration_available(){
  if(sctk_migration_mode == 1){
    not_implemented();
  }
  return sctk_migration_mode;
}

/********** SEND ************/
static void sctk_network_send_message_default (sctk_thread_ptp_message_t * msg){
  not_reachable();
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
static void sctk_network_notify_perform_message_default (int remote_proces, int remote_task_id){

}
static void (*sctk_network_notify_perform_message_ptr) (int,int) =
  sctk_network_notify_perform_message_default;
void sctk_network_notify_perform_message (int remote_process, int remote_task_id){
  sctk_network_notify_perform_message_ptr(remote_process, remote_task_id);
}
void sctk_network_notify_perform_message_set(void (*sctk_network_notify_perform_message_val) (int,int)){
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

#define FIRST_TRY_DRIVER(dr_name,func,topo) if(strcmp(name,SCTK_STRING(dr_name)) == 0){func(name,topo)
#define TRY_DRIVER(dr_name,func,topo) } else if(strcmp(name,SCTK_STRING(dr_name)) == 0){func(name,topo)
#define DEFAUT_DRIVER() } else {sctk_network_not_implemented(name);}(void)(0)

static void sctk_network_not_implemented(char* name){
  sctk_error("Network %s not available",name);
  sctk_abort();
}

void
sctk_net_init_pmi() {
  if(sctk_process_number > 1){
    /* Initialize topology informations from PMI */
    sctk_pmi_get_process_rank(&sctk_process_rank);
    sctk_pmi_get_process_number(&sctk_process_number);
    sctk_pmi_get_process_on_node_rank(&sctk_local_process_rank);
    sctk_pmi_get_process_on_node_number(&sctk_local_process_number);
  }
}

void
sctk_net_init_driver (char *name)
{
  if(sctk_process_number > 1){
    char *topo = "ring";
    int i;

    for(i= 0; i < strlen(name); i++){
      if(name[i] == ':'){
	name[i] = '\0';
	topo = &(name[i+1]);
      }
    }

    sctk_nodebug("Use network %s",name);

    FIRST_TRY_DRIVER(tcp,sctk_network_init_simple_tcp,topo);
    TRY_DRIVER(tcpoib,sctk_network_init_simple_tcp_o_ib,topo);
    TRY_DRIVER(simple_tcp,sctk_network_init_simple_tcp,topo);
    TRY_DRIVER(multirail_tcp,sctk_network_init_multirail_tcp,topo);
    TRY_DRIVER(multirail_tcpoib,sctk_network_init_multirail_tcpoib,topo);

#ifdef MPC_USE_INFINIBAND
    /* Driver for Infiniband */
    TRY_DRIVER(multirail_ib,sctk_network_init_multirail_ib,topo);
    /* FIXME: For backward compatibility. Should not more be used */
    TRY_DRIVER(ib,sctk_network_init_ib,"ondemand");
#endif

    DEFAUT_DRIVER();

    sctk_route_finalize();
    sctk_checksum_init();
  }
}

/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/
size_t sctk_net_memory_allocation_hook(size_t size_origin) {
  size_t aligned_size;
#ifdef MPC_USE_INFINIBAND
  return sctk_network_memory_allocator_hook_ib (size_origin);
#endif
  return 0;
}
