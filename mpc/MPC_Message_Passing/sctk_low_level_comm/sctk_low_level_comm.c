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
#include "sctk_runtime_config.h"

 /*Networks*/
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

const struct sctk_runtime_config_struct_networks * sctk_net_get_config() {
  return (struct sctk_runtime_config_struct_networks*) &sctk_runtime_config_get()->networks;
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
sctk_net_init_driver ()
{
  if(sctk_process_number > 1){
    int i, j;
    int rails_nb = 0;

    /* Set the number of rails used for the routing interface */
    sctk_route_set_rail_nb(sctk_net_get_config()->rails_size);

    for (i=0; i<sctk_net_get_config()->rails_size; ++i) {
      char* config_name = sctk_net_get_config()->rails[i].config;

      /* Try to find the associated configuration */
      for (j=0; j<sctk_net_get_config()->configs_size; ++j) {
        if (strcmp(config_name, sctk_net_get_config()->configs[j].name) == 0) {
          char* topology = sctk_net_get_config()->rails[i].topology;
          /* Set infos for the current rail */
          sctk_route_set_rail_infos(i, &sctk_net_get_config()->rails[i],
            &sctk_net_get_config()->configs[j]);

          /* Switch on the driver to use */
          switch (sctk_net_get_config()->configs[j].driver.type) {
#ifdef MPC_USE_INFINIBAND
            case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
              sctk_network_init_multirail_ib(i);
              break;
#endif
            case SCTK_RTCFG_net_driver_tcp: /* TCP */
              sctk_network_init_multirail_tcp(i);
              break;
            case SCTK_RTCFG_net_driver_tcpoib: /* TCP */
              sctk_network_init_multirail_tcpoib(i);
              break;
            default:
              sctk_network_not_implemented("");
              break;
          }
          /* Increment the number of rails used */
          rails_nb++;
        }
      }
    }

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
