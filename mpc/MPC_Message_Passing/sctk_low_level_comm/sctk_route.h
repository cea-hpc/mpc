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

#include <sctk_simple_tcp.h>
#include <uthash.h>

typedef struct{
  int destination;
  int rail;
}sctk_route_key_t;


typedef union{
  sctk_simple_tcp_data_t simple_tcp;
}sctk_route_data_t;

typedef struct sctk_route_table_s{
  sctk_route_key_t key;

  sctk_route_data_t data;  

  /*
    This function can be used to store different send functions according to 
    the destination route (useful for multiple network configuration)
   */
  void (*send_func) (sctk_thread_ptp_message_t * ,struct sctk_route_table_s*);

  UT_hash_handle hh;
} sctk_route_table_t;

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_add_static_route(int dest, sctk_route_table_t* tmp, int rail,
			   void (*send_func) (sctk_thread_ptp_message_t * ));

/*THREAD SAFE use to add a route at compute time*/
void sctk_add_dynamic_route(int dest, sctk_route_table_t* tmp, int rail,
			    void (*send_func) (sctk_thread_ptp_message_t * ));

sctk_route_table_t* sctk_get_route(int dest, int rail);
sctk_route_table_t* sctk_get_route_to_process(int dest, int rail);
#endif
