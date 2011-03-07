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

#ifndef __SCTK__INFINIBAND_ALLOCATOR_H_
#define __SCTK__INFINIBAND_ALLOCATOR_H_

#include "sctk_list.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_qp.h"
#include "sctk.h"

sctk_net_ibv_qp_rail_t   *rail;

/* RC SR structures */
sctk_net_ibv_qp_local_t *rc_sr_local;

/* RC RDMA structures */
sctk_net_ibv_qp_local_t   *rc_rdma_local;

typedef struct
{
  sctk_net_ibv_rc_sr_process_t        *rc_sr;
  sctk_net_ibv_rc_rdma_process_t      *rc_rdma;

  /* sequence numbers */
  uint32_t                esn;    /* expected sequence number */
  uint32_t                psn;    /* packet sequence number */

  /* for debug */
  uint32_t nb_ptp_msg_transfered;
  uint32_t nb_ptp_msg_received;

  /* locks */
  sctk_thread_mutex_t rc_sr_lock;
  sctk_thread_mutex_t rc_rdma_lock;

} sctk_net_ibv_allocator_entry_t;

typedef struct
{
  sctk_net_ibv_allocator_entry_t* entry;
} sctk_net_ibv_allocator_t;

//static int ibv_rc_sr_entry_nb   = 0;
//static int ibv_rc_rdma_entry_nb = 0;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

sctk_net_ibv_allocator_t *sctk_net_ibv_allocator_new();

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry, sctk_net_ibv_allocator_type_t type);

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_rc_rdma_process_next_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma);

void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process );

void
sctk_net_ibv_allocator_send_coll_message(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_sr_msg_type_t type);

/*-----------------------------------------------------------
 *  SEARCH FUNCTIONS
 *----------------------------------------------------------*/

  sctk_net_ibv_rc_sr_process_t*
sctk_net_ibv_alloc_rc_sr_find_from_qp_num(uint32_t qp_num);

  sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(uint32_t qp_num);

  sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_rank(int rank);

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_allocator_ptp_poll_all();

void sctk_net_ibv_allocator_ptp_lookup_all(int dest);

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest);

  char*
sctk_net_ibv_tcp_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port);

int sctk_net_ibv_tcp_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote);

  void sctk_net_ibv_tcp_server();
#endif
