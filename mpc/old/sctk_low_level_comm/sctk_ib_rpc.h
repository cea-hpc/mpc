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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__INFINIBAND_RPC_H_
#define __SCTK__INFINIBAND_RPC_H_

#include <stdint.h>
#include "sctk_ib_ibufs.h"

typedef struct
{
  void ( *func ) ( void * );
  size_t arg_size;
  void* arg;
} sctk_net_ibv_rpc_t;

typedef struct
{
  uint32_t key;
} sctk_net_ibv_rpc_request_t;

typedef struct
{
  int src_process;
  void* ack;
  int lock;
  struct sctk_list_header list_header;
} sctk_net_ibv_rpc_ack_t;

  void
sctk_net_rpc_init();

  void*
thread_rpc(void* arg);
  void
sctk_net_rpc_register(void* addr, size_t size, int process, int is_retrieve, uint32_t* rkey);

  void
sctk_net_rpc_unregister(void* addr, size_t size, int process, int is_retrieve);

void
sctk_net_rpc_driver ( void ( *func ) ( void * ), int destination, void *arg, size_t arg_size );

void
sctk_net_rpc_retrive_driver ( void *dest, void *src, size_t arg_size,
    int process, int *ack, uint32_t rkey );

 void
sctk_net_rpc_send_driver ( void *dest, void *src, size_t arg_size, int process,
    int *ack, uint32_t rkey );

  void
sctk_net_rpc_receive(sctk_net_ibv_ibuf_t* ibuf);

  void
sctk_net_rpc_send_ack(sctk_net_ibv_ibuf_t* ibuf);
#endif
#endif
