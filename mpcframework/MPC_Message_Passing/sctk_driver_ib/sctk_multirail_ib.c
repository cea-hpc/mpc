/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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
#include "sctk_ib_topology.h"

static int rails_nb = 0;
static sctk_rail_info_t** rails = NULL;
static char is_ib_used = 0;

struct sctk_rail_info_s** sctk_network_get_rails() {
  return rails;
}

static int ib_rail_data;          /* Data rail */
static int ib_rail_signalization; /* Signalization rail */

TODO("The following value MUST be determined dynamically!!")
#define IB_MEM_THRESHOLD_ALIGNED_SIZE (256*1024) /* RDMA threshold */
#define IB_MEM_ALIGNMENT        (4096) /* Page size */

int sctk_network_ib_get_rails_nb() {
  return rails_nb;
}

/* Return which rail is used for signalization */
int sctk_network_ib_get_rail_signalization() {
  return ib_rail_signalization;
}
/* Set which rail is used for MPI communications */
void sctk_network_ib_set_rail_data(int id) {
  ib_rail_data = id;
}
/* Set which rail is used for signalization */
void sctk_network_ib_set_rail_signalization(int id) {
  sctk_only_once();
  ib_rail_signalization = id;
  /* Set the rail as a signalization rail */
  sctk_route_set_signalization_rail(rails[id]);
}

/*-------------------------------------------------------------------
 * Rail selection
 *------------------------------------------------------------------*/
static int __thread __numa = -1;
static int __thread __vp = -1;

int sctk_network_select_recv_rail() {
  int i;
  int node_num = __numa;

#if 1
  if (__numa == -1) {
    __vp = sctk_get_cpu();
    node_num =  sctk_get_node_from_cpu(__vp);
    sctk_nodebug("Init vp %d DONE numa %d", __vp, node_num);
    __numa = node_num;
    assume(__numa >= 0);
  }

  if (rails_nb == 5) {
    i = __numa / 4;
  } else if (rails_nb == 17 ) {
    i = __numa;
  } else {
    i = 0;
  }
#endif

#if 0
  if (__numa == -1) {
    __vp = sctk_get_cpu();
    node_num =  sctk_get_node_from_cpu(__vp);
    sctk_nodebug("Init vp %d DONE numa %d", __vp, node_num);
    __numa = node_num;
    assume(__numa >= 0);
  }

  if (rails_nb == 5) {
    i = node_num / 4;
  } else if (rails_nb == 17) {
    int node_num = __numa;

    i = node_num;
  } else if (rails_nb == 3) {
    int node_num = __numa;
    i = node_num;
  } else {
    i = 0;
  }
#endif

  return i;
}

int sctk_network_select_send_rail(sctk_thread_ptp_message_t * msg) {
  int i;

#if 0
  if (rails_nb == 17)
  {
    if(msg->sctk_msg_get_glob_destination < 0) {
      assume(0);
    } else {
     i = (msg->sctk_msg_get_glob_destination % 128) / 8;
    }
  }
#endif

  if (rails_nb == 5)
  {
    int glob_dest;
    glob_dest = msg->sctk_msg_get_glob_destination;
    i = (glob_dest % 128) / 32;

    sctk_nodebug("Message with comm %d to rail %d (dest:%d numa_dest:%d glob_dest:%d)", msg->sctk_msg_get_communicator, i, msg->sctk_msg_get_destination, msg->sctk_msg_get_glob_destination, glob_dest );
  }
  else if (rails_nb == 17)
  {
    int glob_dest;
    glob_dest = msg->sctk_msg_get_glob_destination;
    i = (glob_dest % 128) / 8;
  }
  else if (rails_nb == 3)
  {
    int glob_dest;
    glob_dest = msg->sctk_msg_get_glob_destination;
    i = (glob_dest % 16) / 8;
  }
  else {
    i = 0;
  }

  return i;
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
    i = sctk_network_ib_get_rail_signalization();
  } else {
    i = sctk_network_select_send_rail(msg);
  }

  /* Always send using the MPI network */
  sctk_nodebug("Send message using rail %d", i);
  rails[i]->send_message(msg,rails[i]);
}

static void
sctk_network_notify_recv_message_multirail_ib (sctk_thread_ptp_message_t * msg){
  int i;

#if 0
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_recv_message(msg,rails[i]);
  }
#endif
}

static void
sctk_network_notify_matching_message_multirail_ib (sctk_thread_ptp_message_t * msg){
TODO("Polling Matching disabled")
#if 0
  int i;
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_matching_message(msg,rails[i]);
  }
#endif
}

static void
sctk_network_notify_perform_message_multirail_ib (int remote_process, int remote_task_id, int polling_task_id, int blocking){
  int i;

  i = sctk_network_select_recv_rail();

  if (i != -1) {
    rails[i]->notify_perform_message(remote_process, remote_task_id,polling_task_id, blocking, rails[i]);

  } else {
    assume(0);

    for(i = 0; i < rails_nb; i++){
      rails[i]->notify_perform_message(remote_process, remote_task_id,polling_task_id, blocking, rails[i]);
    }
  }
}

static void
sctk_network_notify_idle_message_multirail_ib (){
TODO("Polling Idle disabled")
#if 0
  int i;

  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_idle_message(rails[i]);
   }
#endif
}

static void
sctk_network_notify_idle_message_multirail_ib_polling (){
  int i;

  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_idle_message(rails[i]);
   }
}

 void
sctk_network_notify_idle_message_multirail_ib_wait_send (){
  int i;

  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_idle_message(rails[i]);
   }
}

static void
sctk_network_notify_any_source_message_multirail_ib (int polling_task_id, int blocking){
  int i;

  i = sctk_network_select_recv_rail();

  if (i != -1) {
    rails[i]->notify_any_source_message(polling_task_id, blocking, rails[i]);

  } else {
    for(i = 0; i < rails_nb; i++){
      rails[i]->notify_any_source_message(polling_task_id, blocking, rails[i]);
    }
  }

#if 0
  assume(0);
  for(i = 0; i < rails_nb; i++){
    rails[i]->notify_any_source_message(polling_task_id, blocking, rails[i]);
   }
#endif
}

/* Returns the status of the message polled */
static
int sctk_send_message_from_network_multirail_ib (sctk_thread_ptp_message_t * msg){
  int ret = sctk_send_message_from_network_reorder(msg);
  if(ret == REORDER_NO_NUMBERING){
    /*
      No reordering
    */
    sctk_send_message_try_check(msg,1);
  }

  return ret;
}

/************ INIT ****************/
void sctk_network_init_multirail_ib(int rail_id, int max_rails){
  static int init_once = 0;

  rails = sctk_realloc(rails, (rails_nb+1)*sizeof(sctk_rail_info_t*));
  /* Initialize the newly allocated memory */
  memset((rails+rails_nb), 0, sizeof(sctk_rail_info_t*));
  is_ib_used = 1;

  rails[rails_nb] = sctk_route_get_rail(rail_id);
  struct sctk_runtime_config_struct_net_rail * rail = rails[rails_nb]->runtime_config_rail;
  struct sctk_runtime_config_struct_net_driver_config * config = rails[rails_nb]->runtime_config_driver_config;
  rails[rails_nb]->rail_number = rail_id; /* WARNING with rails_nb */
  rails[rails_nb]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[rails_nb],rail->topology);
  /* Initialize the IB rail ID */

  #if 0
  int previous_binding;
  {
    if (config->driver.value.infiniband.network_type == 1)
      if ( (max_rails - 1) == 4) {
        previous_binding = sctk_bind_to_cpu(rail_id*32 % 128);
      } else if ( (max_rails - 1) == 2 ) {
        previous_binding = sctk_bind_to_cpu(rail_id*8);
      } else if ( (max_rails - 1) == 16 ) {
        previous_binding = sctk_bind_to_cpu(rail_id*8 % 128);
      } else if ( (max_rails - 1) == 1 ) {
        previous_binding = sctk_bind_to_cpu(0);
      } else {
        not_implemented();
      }
    else
      previous_binding = sctk_bind_to_cpu(0);
  static int init_once = 0;
  #endif

  if (config->driver.value.infiniband.network_type == 1) {
  rails[rails_nb]->send_message_from_network = sctk_send_message_from_network_multirail_ib;
  sctk_route_init_in_rail(rails[rails_nb],rail->topology);
    /* MPI IB network */
    if (strcmp(rail->topology, "ondemand") && strcmp(rail->topology, "fully")) {
      sctk_error("IB requires the 'ondemand' or the 'fully' topology for the data network! Exiting... Topology provided: %s", rail->topology);
      sctk_abort();
    }
    sctk_network_ib_set_rail_data(rails_nb);
    sctk_network_init_mpi_ib(rails[rails_nb], rails_nb);
  } else if ( config->driver.value.infiniband.network_type == 0) {
    /* Fallback IB network */
    sctk_network_ib_set_rail_signalization(rails_nb);
    sctk_network_init_fallback_ib(rails[rails_nb], rails_nb);
    if (strcmp(rail->topology, "ring") && strcmp(rail->topology, "torus")) {
      sctk_error("IB requires the 'ring' or the 'torus' topology for the signalization network! Exiting... Topology provided: %s", rail->topology);
      sctk_abort();
    }
  } else {
    sctk_error("You must provide a network's type equivalent to 'data' or 'signalization'. Value provided:%d", config->driver.value.infiniband.network_type);
    sctk_abort();
  }

  if (init_once == 0) {
    sctk_ib_prof_init();
    sctk_network_send_message_set(sctk_network_send_message_multirail_ib);
    sctk_network_notify_recv_message_set(sctk_network_notify_recv_message_multirail_ib);
    sctk_network_notify_matching_message_set(sctk_network_notify_matching_message_multirail_ib);
    sctk_network_notify_perform_message_set(sctk_network_notify_perform_message_multirail_ib);

    if (sctk_runtime_config_get()->modules.low_level_comm.enable_idle_polling == 1) {
      sctk_warning("Idle polling enabled");
      sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_multirail_ib_polling);
    } else {
      sctk_network_notify_idle_message_set(sctk_network_notify_idle_message_multirail_ib);
    }

    sctk_network_notify_any_source_message_set(sctk_network_notify_any_source_message_multirail_ib);
  }
  init_once=1;

  sctk_ib_topology_init_task(rails[rails_nb], sctk_thread_get_vp());
  rails[rails_nb]->initialize_leader_task(rails[rails_nb]);
  rails_nb++;

#if 0
  {
    /* Revert to CPU 0 */
    sctk_bind_to_cpu(previous_binding);
  }
#endif
}

char sctk_network_is_ib_used() {
  return is_ib_used;
}

/************ INITIALIZE TASK ****************/
void sctk_network_initialize_task_multirail_ib (int rank, int vp){
  if(sctk_process_number > 1 && sctk_network_is_ib_used() ){
    /* Register task for QP prof */
    sctk_ib_prof_init_task(rank, vp);
    /* Register task for collaborative polling */
    sctk_ib_cp_init_task(rank, vp);
  }

  int i;
  for(i = 0; i < rails_nb; i++){
    /* Register task for topology infos */
    sctk_ib_topology_init_task(rails[i], vp);

    rails[i]->initialize_task(rails[i]);
  }
}

/************ FINALIZE PROCESS ****************/
void sctk_network_finalize_multirail_ib (){
/* Do not report timers */
  int i;

  for(i = 0; i < rails_nb; i++){
    rails[i]->finalize_task(rails[i]);
  }
}

/************ FINALIZE TASK ****************/
void sctk_network_finalize_task_multirail_ib (int rank){
  if(sctk_process_number > 1 && sctk_network_is_ib_used() ){
    sctk_ib_cp_finalize_task(rank);
    sctk_ib_prof_qp_finalize_task(rank);
  }
}

/************ MEMORY ALLOCATOR HOOK  ****************/
size_t sctk_network_memory_allocator_hook_ib (size_t size){
    if (size > IB_MEM_THRESHOLD_ALIGNED_SIZE ) {
    return ( (size + (IB_MEM_ALIGNMENT-1) ) & ( ~ (IB_MEM_ALIGNMENT-1) ) );
  }
  return 0;
}



#endif
