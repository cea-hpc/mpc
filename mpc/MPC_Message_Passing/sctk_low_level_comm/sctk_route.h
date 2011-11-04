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
#ifndef __SCTK_ROUTE_H_
#define __SCTK_ROUTE_H_

#include <sctk_inter_thread_comm.h>
#include <uthash.h>

typedef struct sctk_rail_info_s sctk_rail_info_t;

#include <sctk_tcp.h>

typedef struct{
  int destination;
  int rail;
}sctk_route_key_t;


typedef union{
  sctk_tcp_data_t tcp;
}sctk_route_data_t;

typedef union{
  sctk_tcp_rail_info_t tcp;
}sctk_rail_info_spec_t;

struct sctk_rail_info_s{
  sctk_rail_info_spec_t network;
  void (*send_message) (sctk_thread_ptp_message_t *,struct sctk_rail_info_s*);
  void (*notify_recv_message) (sctk_thread_ptp_message_t * ,struct sctk_rail_info_s*);
  void (*notify_matching_message) (sctk_thread_ptp_message_t * ,struct sctk_rail_info_s*);
  void (*notify_perform_message) (int ,struct sctk_rail_info_s*);
  void (*notify_idle_message) (struct sctk_rail_info_s*);
  void (*notify_any_source_message) (struct sctk_rail_info_s*);
  void (*send_message_from_network) (sctk_thread_ptp_message_t * );
  int (*route)(int , sctk_rail_info_t* );
  void (*route_init)();
  char* network_name;
  char* topology_name;
  int rail_number;
};

typedef struct sctk_route_table_s{
  sctk_route_key_t key;

  sctk_route_data_t data;
  
  sctk_rail_info_t* rail;

  UT_hash_handle hh;
} sctk_route_table_t;

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);

/*THREAD SAFE use to add a route at compute time*/
void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);

sctk_route_table_t* sctk_get_route(int dest, sctk_rail_info_t* rail);
sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail);

void sctk_route_set_rail_nb(int i);
sctk_rail_info_t* sctk_route_get_rail(int i);

/* Routes */
void sctk_route_init_in_rail(sctk_rail_info_t* rail, char* topology);
void sctk_route_ring_init();
int sctk_route_ring(int dest, sctk_rail_info_t* rail);

void sctk_route_tree_init();
int sctk_route_tree(int dest, sctk_rail_info_t* rail);
#endif
