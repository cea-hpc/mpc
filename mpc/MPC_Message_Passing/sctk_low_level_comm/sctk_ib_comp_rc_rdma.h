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

#ifndef __SCTK__INFINIBAND_COMP_RC_RDMA_H_
#define __SCTK__INFINIBAND_COMP_RC_RDMA_H_

#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_ib_mmu.h"
#include "sctk_ib_comp_rc_sr.h"
#include "sctk_ib_scheduling.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>
#include "sctk_list.h"
#include "sctk_ib_qp.h"
#include "sctk_list.h"

struct sctk_net_ibv_allocator_request_s;

/* define which protocol use for rdnvz messages */
typedef enum
{
  RC_RDMA_REQ_WRITE                  = 1 << 0,
  RC_RDMA_REQ_READ                   = 1 << 1,
  RC_FRAG                            = 1 << 2,
} sctk_net_ibv_rc_rdma_request_protocol_t;

/* define which type is used for a rdvz entry */
typedef enum
{
  SEND_ENTRY  = 0,
  RECV_ENTRY  = 1,
} sctk_net_ibv_rc_rdma_entry_type_t;

/* process entry */
typedef struct
{
  sctk_thread_mutex_t lock;
  /* where are stored send infos */
  struct sctk_list send;
  /* where are stored recv ifos */
  struct sctk_list recv;
  int                     ready;     /* if the qp is ready */
} sctk_net_ibv_rc_rdma_process_t;

/* structure where information about rdvz message are stored */
typedef struct
{
  /*  if the message is ready. If not, we wait */
  volatile int                    ready;
  /* protocol used: rdma read, write, frag msg */
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  /* type of msg, reduce, bcast, ptp, etc... */
  sctk_net_ibv_ibuf_ptp_type_t    ptp_type;

  /* MPC msg header */
  sctk_thread_ptp_message_t       msg_header;
  /* ptr to a MPC msg header */
  sctk_thread_ptp_message_t       *msg_header_ptr;

  void                            *msg_payload_aligned_ptr;
  void                            *msg_payload_ptr;

  /* mmu entry if pinned memory */
  sctk_net_ibv_mmu_entry_t        *mmu_entry;
  sctk_net_ibv_rc_rdma_process_t  *entry_rc_rdma;

  /* if msg directly pinned in memory */
  int                             directly_pinned;
  void*                           remote_entry;

  /* requested size for msg */
  size_t                          requested_size;
  /* source process */
  int                             src_process;
  /* source task */
  uint32_t                        src_task;
  /* source task */
  uint32_t                        dest_task;
  /* Packet Sequence Number */
  uint32_t                        psn;
  /* Communicator ID */
  int                             com_id;
  double creation_timestamp;
  /* ptr to the list entry (help for removing an entry from
   * a list) */
  struct sctk_list_elem*          list_elem;;

#if 0
  union
  {
    union
    {
      struct
      {

      } send;

      struct
      {

      } recv;
    } rdma;
    union
    {
      size_t current_copied;

    } frag_eager;
  };
#endif
} sctk_net_ibv_rc_rdma_entry_t;


/* TODO: refactor the 2 next structures */
typedef struct
{
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  sctk_net_ibv_ibuf_ptp_type_t           ptp_type;

  size_t                    requested_size;

  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;

  struct {
      uintptr_t   address;
      uint32_t    rkey;
    } read_req;

  sctk_thread_ptp_message_t msg_header;
} sctk_net_ibv_rc_rdma_request_t;


typedef struct
{
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  sctk_net_ibv_ibuf_ptp_type_t           ptp_type;

  size_t                    requested_size;
//  int                       src_process;

  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;

  struct {
      uintptr_t   address;
      uint32_t    rkey;
    } read_req;
} sctk_net_ibv_rc_rdma_coll_request_t;


typedef struct
{
  void*     dest_ptr;
  uint32_t  dest_rkey;
} sctk_net_ibv_rc_rdma_request_ack_t;

typedef struct
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;

} sctk_net_ibv_rc_rdma_done_t;

/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_allocate_init(
    unsigned int rank,
    sctk_net_ibv_qp_local_t *local);

void
sctk_net_ibv_comp_rc_rdma_send_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    struct sctk_net_ibv_allocator_request_s req,
    uint8_t directly_pinned);

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_analyze_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf);

void
sctk_net_ibv_com_rc_rdma_read_finish(
    sctk_net_ibv_ibuf_t* ibuf,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    int lookup_mode);

void
sctk_net_ibv_com_rc_rdma_recv_done(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_ibuf_t* ibuf);

void
sctk_net_ibv_comp_rc_rdma_send_finish(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf);

#endif
#endif
