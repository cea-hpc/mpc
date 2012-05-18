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
struct sctk_ib_data_s;

#include <sctk_tcp.h>
#include <sctk_ib.h>

typedef struct{
  int destination;
  int rail;
}sctk_route_key_t;


typedef union{
  /* TCP */
  sctk_tcp_data_t tcp;
  /* IB */
  sctk_ib_data_t ib;
}sctk_route_data_t;

typedef union{
  /* TCP */
  sctk_tcp_rail_info_t tcp;
  /* IB */
  sctk_ib_rail_info_t ib;
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
  void(*connect_to)(int,int,sctk_rail_info_t*);
  void(*connect_from)(int,int,sctk_rail_info_t*);
  int (*route)(int , sctk_rail_info_t* );
  void (*route_init)(sctk_rail_info_t*);
  char* network_name;
  char* topology_name;
  char on_demand;
  /* If the rail allows on demand-connexions */
  int rail_number;
};

typedef enum {
  route_origin_dynamic  = 111,
  route_origin_static   = 222
} sctk_route_origin_t;

typedef struct sctk_route_table_s{
  sctk_route_key_t key;

  sctk_route_data_t data;

  sctk_rail_info_t* rail;

  UT_hash_handle hh;

  /* Origin of the route entry: static or dynamic route */
  sctk_route_origin_t origin;

  /* State of the route */
  OPA_int_t state;
  /* If a message "out of memory" has already been sent to the
   * process to notice him that we are out of memory */
  OPA_int_t low_memory_mode_local;
  /* If the remote process is running out of memory */
  OPA_int_t low_memory_mode_remote;
} sctk_route_table_t;

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_add_static_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);

/*THREAD SAFE use to add a route at compute time*/
void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, sctk_rail_info_t* rail);
struct sctk_route_table_s *sctk_route_dynamic_search(int dest, sctk_rail_info_t* rail);

sctk_route_table_t *sctk_route_dynamic_safe_add(int dest, sctk_rail_info_t* rail, sctk_route_table_t* (*create_func)(), void (*init_func)(int dest, sctk_rail_info_t* rail, sctk_route_table_t *route_table, int ondemand), int *added);

/* For low_memory_mode */
int sctk_route_cas_low_memory_mode_local(sctk_route_table_t* tmp, int oldv, int newv);
int sctk_route_is_low_memory_mode_remote(sctk_route_table_t* tmp);
void sctk_route_set_low_memory_mode_remote(sctk_route_table_t* tmp, int low);
int sctk_route_is_low_memory_mode_local(sctk_route_table_t* tmp);
void sctk_route_set_low_memory_mode_local(sctk_route_table_t* tmp, int low);

/* Function for getting a route */
  sctk_route_table_t* sctk_get_route(int dest, sctk_rail_info_t* rail);
sctk_route_table_t* sctk_get_route_to_process(int dest, sctk_rail_info_t* rail);
inline sctk_route_table_t* sctk_get_route_to_process_no_ondemand(int dest, sctk_rail_info_t* rail);
inline sctk_route_table_t* sctk_get_route_to_process_static(int dest, sctk_rail_info_t* rail);

void sctk_route_set_rail_nb(int i);
sctk_rail_info_t* sctk_route_get_rail(int i);

/* Routes */
void sctk_route_messages_send(int myself,int dest, specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size);
void sctk_route_messages_recv(int src, int myself,specific_message_tag_t specific_message_tag, int tag, void* buffer,size_t size);
void sctk_walk_all_routes( const sctk_rail_info_t* rail, void (*func) (const sctk_rail_info_t* rail,sctk_route_table_t* table) );

void sctk_route_init_in_rail(sctk_rail_info_t* rail, char* topology);
void sctk_route_finalize();
int sctk_route_is_finalized();



/*-----------------------------------------------------------
 *  For ondemand
 *----------------------------------------------------------*/
/* State of the QP */
typedef enum sctk_route_state_e {
  state_connected   = 111,
  state_flushing    = 222,
  state_deconnected = 333,
  state_reconnecting  = 444,
  state_reset       = 555,
} sctk_route_state_t;

__UNUSED__ static void sctk_route_set_state(sctk_route_table_t* tmp, sctk_route_state_t state){
  OPA_store_int(&tmp->state, state);
}

__UNUSED__ static int sctk_route_get_state(sctk_route_table_t* tmp){
  return (int) OPA_load_int(&tmp->state);
}

/* Return the origin of a route entry: from dynamic or static allocation */
__UNUSED__ static sctk_route_origin_t sctk_route_get_origin(sctk_route_table_t *tmp) {
  return tmp->origin;
}

#endif
