/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <sctk_debug.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_low_level_comm.h>
#include <sctk_multirail_ib.h>
#include <sctk_route.h>
#include <sctk_tcp.h>
#include <opa_primitives.h>
#include <sctk_checksum.h>
#include "sctk_ib_fallback.h"
#include "sctk_ib_mpi.h"
#include "sctk_ib_config.h"
#include "sctk_ib_toolkit.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_cp.h"

static int rails_nb = 0;
static sctk_rail_info_t** rails = NULL;
static char is_ib_used = 0;

struct sctk_rail_info_s** sctk_network_get_rails() {
  return rails;
}

static void
sctk_network_send_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i ;

  /* XXX:Calculating checksum */
#ifdef SCTK_USE_CHECKSUM
  sctk_checksum_register(msg);
#endif
  sctk_prepare_send_message_to_network_reorder(msg);

  const specific_message_tag_t tag = msg->body.header.specific_message_tag;
  if (IS_PROCESS_SPECIFIC_CONTROL_MESSAGE(tag)) {
    i = 1;
  } else {
    i = 0;
  }
  /* Always send using the MPI network */
  sctk_nodebug("Send message using rail %d", i);
  rails[i]->send_message(msg,rails[i]);
}

static void
sctk_network_notify_recv_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_recv_message(msg,rails[i]);
  }
}

static void
sctk_network_notify_matching_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;
  for(i = 0; i < rails_nb; i++){
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
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_idle_message(rails[i]);
  }
}

static void
sctk_network_notify_any_source_message_multirail_ib (){
  int i;
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_any_source_message(rails[i]);
  }
}

static
void sctk_send_message_from_network_multirail_ib (sctk_thread_ptp_message_t * msg){
  if(sctk_send_message_from_network_reorder(msg) == REORDER_NO_NUMBERING){
    /*
      No reordering
    */
    sctk_send_message_try_check(msg,1);
  }
}

/************ INIT ****************/
/* XXX: polling thread used for 'fully-connected' topology initialization. Because
 * of PMI lib, when a barrier occurs, the IB polling cannot be executed.
 * This thread is disbaled when the route is initialized */
static void* __polling_thread(void *arg) {
  sctk_rail_info_t* rail = (sctk_rail_info_t*) arg;
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;

  /* XXX hack to disable CP when fully is used */
  config->ibv_steal = -1;
  while(1) {
    if (sctk_route_is_finalized())
    {
      config->ibv_steal = steal;
      break;
    }
    sctk_network_poll_all(rail);
  }
  return NULL;
}

static void sctk_network_init_polling_thread (sctk_rail_info_t* rail) {
  sctk_thread_t pidt;
  sctk_thread_attr_t attr;

  sctk_thread_attr_init (&attr);
  sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
  sctk_user_thread_create (&pidt, &attr, __polling_thread, (void*) rail);
}

/* Choose the topology of the signalization network (ring, torus) */
static char *__get_signalization_topology(char* topo, size_t size) {
    char *value;

    if ( (value = getenv("MPC_IBV_SIGN_TOPO")) != NULL )
        snprintf(topo, size, "%s", value);
    else  /* Torus is the default */
        snprintf(topo, size, "%s", "torus");

    return topo;
}

/************ INIT ****************/
void sctk_network_init_multirail_ib(int rail_id){
  int i;
  static int init_once = 0;

  rails = sctk_realloc(rails, (rails_nb+1)*sizeof(sctk_rail_info_t*));
  /* Initialize the newly allocated memory */
  memset((rails+rails_nb), 0, sizeof(sctk_rail_info_t*));
  is_ib_used = 1;

  rails[rails_nb] = sctk_route_get_rail(rail_id);
  struct sctk_runtime_config_struct_net_rail * rail = rails[rails_nb]->runtime_config_rail;
  struct sctk_runtime_config_struct_net_driver_config * config = rails[rails_nb]->runtime_config_driver_config;
  rails[rails_nb]->rail_number = rails_nb;
  rails[rails_nb]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[rails_nb],rail->topology);

  if (config->driver.value.infiniband.network_type == 1) {
    /* MPI IB network */
    if (strcmp(rail->topology, "ondemand") && strcmp(rail->topology, "fully")) {
      sctk_error("IB requires the 'ondemand' or the 'fully' topology for the data network! Exiting... Topology provided: %s", rail->topology);
      sctk_abort();
    }
    sctk_network_ib_set_rail_data(rails_nb);
    sctk_network_init_mpi_ib(rails[rails_nb]);
  } else if ( config->driver.value.infiniband.network_type == 0) {
    /* Fallback IB network */
    sctk_network_ib_set_rail_signalization(rails_nb);
    sctk_network_init_fallback_ib(rails[rails_nb]);
    if (strcmp(rail->topology, "ring") && strcmp(rail->topology, "torus")) {
      sctk_error("IB requires the 'ring' or the 'torus' topology for the signalization network! Exiting... Topology provided: %s", rail->topology);
      sctk_abort();
    }
  } else {
    sctk_error("You must provide a network's type equivalent to 'data' or 'signalization'. Value provided:%d", config->driver.value.infiniband.network_type);
    sctk_abort();
  }
  rails_nb++;

  if (init_once == 0) {
    sctk_ib_prof_init();
    sctk_network_send_message_set(sctk_network_send_message_multirail_ib);
    sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_multirail_ib);
    sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_multirail_ib);
    sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_multirail_ib);
    sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_multirail_ib);
    sctk_network_notify_any_source_message_set(sctk_network_notify_any_source_message_multirail_ib);
  }
  init_once=1;
}

char sctk_network_is_ib_used() {
  return is_ib_used;
}

/************ FINALIZE ****************/
void sctk_network_finalize_multirail_ib (){
/* Do not report timers */
  int i;
  if (rails) {
    for(i = 0; i < NB_RAILS; i++){
      TODO("Check those *_info_t typedef coherency");
      sctk_ib_prof_finalize((sctk_ib_rail_info_t *)rails[i]);
    }
  }
}

/************ FINALIZE ****************/
void sctk_network_finalize_task_multirail_ib (int rank){
  if(sctk_process_number > 1){
    sctk_ib_cp_finalize_task(rank);
    sctk_ib_prof_qp_finalize_task(rank);
  }
}
#endif
