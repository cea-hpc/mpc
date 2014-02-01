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
void sctk_network_send_message_default (sctk_thread_ptp_message_t * msg){
  not_reachable();
}
static void (*sctk_network_send_message_ptr) (sctk_thread_ptp_message_t *) = sctk_network_send_message_default;
void sctk_network_send_message (sctk_thread_ptp_message_t * msg){
  sctk_network_send_message_ptr(msg);
}
void sctk_network_send_message_set(void (*sctk_network_send_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_send_message_ptr = sctk_network_send_message_val;
}

/********** NOTIFY_RECV ************/
void sctk_network_notify_recv_message_default (sctk_thread_ptp_message_t * msg){

}
static void (*sctk_network_notify_recv_message_ptr) (sctk_thread_ptp_message_t *) = sctk_network_notify_recv_message_default;
void sctk_network_notify_recv_message (sctk_thread_ptp_message_t * msg){
  sctk_network_notify_recv_message_ptr(msg);
}
void sctk_network_notify_recv_message_set(void (*sctk_network_notify_recv_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_notify_recv_message_ptr = sctk_network_notify_recv_message_val;
}

/********** NOTIFY_MATCHING ************/
void sctk_network_notify_matching_message_default (sctk_thread_ptp_message_t * msg){

}
static void (*sctk_network_notify_matching_message_ptr) (sctk_thread_ptp_message_t *) = sctk_network_notify_matching_message_default;
void sctk_network_notify_matching_message (sctk_thread_ptp_message_t * msg){
  sctk_network_notify_matching_message_ptr(msg);
}
void sctk_network_notify_matching_message_set(void (*sctk_network_notify_matching_message_val) (sctk_thread_ptp_message_t *)){
  sctk_network_notify_matching_message_ptr = sctk_network_notify_matching_message_val;
}

/********** NOTIFY_PERFORM ************/
static void sctk_network_notify_perform_message_default (int remote_proces, int remote_task_id, int polling_task_id, int blocking){

}
static void (*sctk_network_notify_perform_message_ptr) (int,int,int,int) =
  sctk_network_notify_perform_message_default;
void sctk_network_notify_perform_message (int remote_process, int remote_task_id, int polling_task_id, int blocking){
  sctk_network_notify_perform_message_ptr(remote_process, remote_task_id, polling_task_id, blocking);
}
void sctk_network_notify_perform_message_set(void (*sctk_network_notify_perform_message_val) (int,int,int,int)){
  sctk_network_notify_perform_message_ptr = sctk_network_notify_perform_message_val;
}

/********** NOTIFY_IDLE ************/
void sctk_network_notify_idle_message_default (){

}
static void (*sctk_network_notify_idle_message_ptr) () = sctk_network_notify_idle_message_default;
void sctk_network_notify_idle_message (){
  sctk_network_notify_idle_message_ptr();
}
void sctk_network_notify_idle_message_set(void (*sctk_network_notify_idle_message_val) ()){
  sctk_network_notify_idle_message_ptr = sctk_network_notify_idle_message_val;
}

/********** NOTIFY_ANY_SOURCE ************/
static void sctk_network_notify_any_source_message_default (int polling_task_id,int blocking){

}
static void (*sctk_network_notify_any_source_message_ptr) (int,int) =
  sctk_network_notify_idle_message_default;
void sctk_network_notify_any_source_message (int polling_task_id,int blocking){
  sctk_network_notify_any_source_message_ptr(polling_task_id,blocking);
}
void sctk_network_notify_any_source_message_set(void (*sctk_network_notify_any_source_message_val) (int polling_task_id,int blocking)){
  sctk_network_notify_any_source_message_ptr = sctk_network_notify_any_source_message_val;
}

#define FIRST_TRY_DRIVER(dr_name,func,topo) if(strcmp(name,SCTK_STRING(dr_name)) == 0){func(name,topo)
#define TRY_DRIVER(dr_name,func,topo) } else if(strcmp(name,SCTK_STRING(dr_name)) == 0){func(name,topo)
#define DEFAUT_DRIVER() } else {sctk_network_not_implemented(name);}(void)(0)

static void sctk_network_not_implemented(char* name){
  sctk_error("No configuration found for the network '%s'. Please check you '-net=' argument"
    " and your configuration file", name);
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
sctk_net_init_driver (char* name)
{
  if(sctk_process_number > 1){
    int j, k, l;
    int rails_nb = 0;
    struct sctk_runtime_config_struct_net_cli_option * cli_option = NULL;
    char * option_name = sctk_runtime_config_get()->modules.low_level_comm.network_mode;

    if (name != NULL) {
      option_name = name;
    }

    sctk_nodebug("Run with driver %s", option_name);
    for (k=0; k<sctk_net_get_config()->cli_options_size; ++k) {
      if (strcmp(option_name, sctk_net_get_config()->cli_options[k].name) == 0) {
        cli_option = &sctk_net_get_config()->cli_options[k];
        break;
      }
    }

    if (cli_option == NULL) {
      sctk_error("No configuration found for the network '%s'. Please check you '-net=' argument"
          " and your configuration file", option_name);
      sctk_abort();
    }

    /* Set the number of rails used for the routing interface */
    sctk_route_set_rail_nb(cli_option->rails_size);

    /* Compute the number of rails for each type:
     * TODO: I think we could simplify this loop ... */
    int nb_rails_infiniband = 0;
    int nb_rails_tcp = 0;
    int nb_rails_tcpoib = 0;

    for (k=0; k<cli_option->rails_size; ++k) {
      struct sctk_runtime_config_struct_net_rail * rail = NULL;
      for (l=0; l<sctk_net_get_config()->rails_size; ++l){
        if (strcmp(cli_option->rails[k], sctk_net_get_config()->rails[l].name) == 0) {
          rail = &sctk_net_get_config()->rails[l];
          break;
        }
      }

      if (rail == NULL) {
        sctk_error("Rail with name '%s' not found in config!", cli_option->rails[k]);
        sctk_abort();
      }

      /* Try to find the rail associated to the configuration */
      for (j=0; j<sctk_net_get_config()->configs_size; ++j) {
        if (strcmp(rail->config, sctk_net_get_config()->configs[j].name) == 0) {
          char* topology = rail->topology;

          /* Switch on the driver to use */
          switch (sctk_net_get_config()->configs[j].driver.type) {
#ifdef MPC_USE_INFINIBAND
            case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
              nb_rails_infiniband ++ ;
              break;
#endif
            case SCTK_RTCFG_net_driver_tcp: /* TCP */
              nb_rails_tcp ++ ;
              break;
            case SCTK_RTCFG_net_driver_tcpoib: /* TCP */
              nb_rails_tcpoib ++ ;
              break;
            default:
              sctk_network_not_implemented(option_name);
              break;
          }
        }
      }
    }
    
    /* End of rails computing. Now allocate ! */

    for (k=0; k<cli_option->rails_size; ++k) {
      struct sctk_runtime_config_struct_net_rail * rail = NULL;
      for (l=0; l<sctk_net_get_config()->rails_size; ++l){
        if (strcmp(cli_option->rails[k], sctk_net_get_config()->rails[l].name) == 0) {
          rail = &sctk_net_get_config()->rails[l];
          break;
        }
      }

      if (rail == NULL) {
        sctk_error("Rail with name '%s' not found in config!", cli_option->rails[k]);
        sctk_abort();
      }
      sctk_nodebug("Found rail '%s' to init", rail->name);

      /* Try to find the rail associated to the configuration */
      for (j=0; j<sctk_net_get_config()->configs_size; ++j) {
        if (strcmp(rail->config, sctk_net_get_config()->configs[j].name) == 0) {
          char* topology = rail->topology;
          /* Set infos for the current rail */
          sctk_route_set_rail_infos(k, rail,
            &sctk_net_get_config()->configs[j]);

          /* Switch on the driver to use */
          switch (sctk_net_get_config()->configs[j].driver.type) {
#ifdef MPC_USE_INFINIBAND
            case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
              sctk_network_init_multirail_ib(k, nb_rails_infiniband);
              break;
#endif
            case SCTK_RTCFG_net_driver_tcp: /* TCP */
              sctk_network_init_multirail_tcp(k, nb_rails_tcp);
              break;
            case SCTK_RTCFG_net_driver_tcpoib: /* TCP */
              sctk_network_init_multirail_tcpoib(k, nb_rails_tcpoib);
              break;
            default:
              sctk_network_not_implemented(option_name);
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
/* Hybrid MPI+X                                                    */
/********************************************************************/
static int is_mode_hybrid = 0;

int sctk_net_is_mode_hybrid () {
  return is_mode_hybrid;
}

int sctk_net_set_mode_hybrid () {
  is_mode_hybrid = 1;
  return 0;
}
/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/
size_t sctk_net_memory_allocation_hook(size_t size_origin) {
  size_t aligned_size;
#ifdef MPC_USE_INFINIBAND
  if (sctk_network_is_ib_used()) {
    return sctk_network_memory_allocator_hook_ib (size_origin);
  }
#endif
  return 0;
}

