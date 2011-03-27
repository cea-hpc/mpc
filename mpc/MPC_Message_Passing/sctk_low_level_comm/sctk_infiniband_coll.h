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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */


#ifndef __SCTK_INFINIBAND_COLL_H_
#define __SCTK_INFINIBAND_COLL_H_

#include "sctk_list.h"
#include "sctk_config.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_comp_rc_rdma.h"
/* list of broadcast entries */
struct sctk_list  broadcast_fifo;
struct sctk_list  init_barrier_fifo;
struct sctk_list  reduce_fifo;

typedef struct
{
  unsigned int src_process;
  size_t size;
  void* payload;
} sctk_net_ibv_collective_pending_t;

/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  void
  sctk_net_ibv_collective_init();

  void*
  sctk_net_ibv_collective_push(struct sctk_list* list, sctk_net_ibv_ibuf_header_t* msg);

  sctk_net_ibv_collective_pending_t*
sctk_net_ibv_collective_lookup_src(struct sctk_list* list, const int src);

  void
sctk_net_ibv_broadcast ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root);


  void
sctk_net_ibv_allreduce ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type );

   void*
  sctk_net_ibv_collective_push_rc_rdma(struct sctk_list* list, sctk_net_ibv_rc_rdma_entry_t* entry);

  void
sctk_net_ibv_allocator_send_coll_message(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_msg_type_t type);


/*-----------------------------------------------------------
 *  BARRIER
 *----------------------------------------------------------*/
#define N_WAY_DISSEMINATION 1

  void
sctk_net_ibv_barrier_init ();

  void
sctk_net_ibv_barrier ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp );

#endif
