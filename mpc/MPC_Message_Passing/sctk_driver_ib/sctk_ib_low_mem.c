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
#include "sctk_ib.h"
#include "sctk_ib_low_mem.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_prof.h"
#include "sctk_asm.h"
#include "utlist.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "LOW M"
#include "sctk_ib_toolkit.h"
#include "math.h"

#define LOW_MEM_REQ_TAG 2


static inline void sctk_ib_low_mem_request_send(const sctk_rail_info_t* rail, sctk_route_table_t* table){
  sctk_ib_data_t *route;
  /* Message to exchange to the peer */
  sctk_ib_low_mem_t msg;

  assume(table);
  route=&table->data.ib;
  assume(route);

  sctk_ib_debug("[%d] Sending a LOW MEM message to %d", rail->rail_number, route->remote->rank);
  sctk_route_messages_send(sctk_process_rank,
      route->remote->rank,low_mem_specific_message_tag,
      LOW_MEM_REQ_TAG, &msg, sizeof(sctk_ib_low_mem_t));
  return;
}

void sctk_ib_low_mem_request_recv(sctk_rail_info_t* rail, sctk_ib_low_mem_t *msg, int src){
  sctk_route_table_t *table;
  sctk_ib_data_t *route;

  table = sctk_get_route_to_process(src,rail);
  route = &table->data.ib;
  /* We cannot receive more than one low memory message from the same task*/
  assume (sctk_route_is_low_memory_mode_remote(table) == 0);
  /* Set the task as low memory mode */
  sctk_route_set_low_memory_mode_remote(table, 1);

  sctk_ib_debug("[%d] Recv LOW MEM message from %d", rail->rail_number, src);
  return;
}


int sctk_ib_low_mem_recv(sctk_rail_info_t *rail,
    sctk_thread_ptp_message_t *msg, sctk_ibuf_t* ibuf, int recopy) {
  const sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int process_dest;
  int process_src;
  sctk_ib_low_mem_t* payload;

  if (config->ibv_low_memory == 0) not_reachable();

  payload = (sctk_ib_low_mem_t*) IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer);
  process_dest = msg->sctk_msg_get_destination;
  process_src = msg->sctk_msg_get_source;
  /* If destination of the message */
  if (process_dest == sctk_process_rank) {

    /* retreive message */
    sctk_ib_low_mem_request_recv(rail, payload, process_src);

    sctk_ibuf_release(&rail->network.ib, ibuf);
    PROF_INC(rail, free_mem);
    sctk_free(msg);
    return 1;
  } else {
    sctk_debug("Forward request to process %d for process %d", process_dest, process_src);
    sctk_ib_eager_recv_free(rail, msg, ibuf, recopy);
    rail->send_message_from_network(msg);
    return 0;
  }
}

/* TODO: A process may have an entry but it is not connected */
void sctk_ib_low_mem_broadcast_func(const sctk_rail_info_t* rail, sctk_route_table_t* table) {
  sctk_ib_low_mem_request_send(rail, table);
}

void sctk_ib_low_mem_broadcast(sctk_rail_info_t* rail) {
  const sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);

  if (config->ibv_low_memory == 0) return;

  sctk_warning("Send signal to use low mem mode");
  sctk_walk_all_routes(rail, sctk_ib_low_mem_broadcast_func);
}

#endif
