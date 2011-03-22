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


#ifndef __SCTK__INFINIBAND_COMP_RC_RDMA_H_
#define __SCTK__INFINIBAND_COMP_RC_RDMA_H_

#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_infiniband_mmu.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_scheduling.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>
#include "sctk_list.h"
#include "sctk_infiniband_qp.h"
#include "sctk_list.h"

/* define which protocol use for rdnvz messages */
typedef enum
{
  RC_RDMA_REQ_WRITE                  = 1 << 0,
  RC_RDMA_REQ_READ                   = 1 << 1,
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
  volatile int                    ready;
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  sctk_net_ibv_rc_sr_msg_type_t           msg_type;

  sctk_thread_ptp_message_t       msg_header;
  sctk_thread_ptp_message_t       *msg_header_ptr;
  void                            *msg_payload_aligned_ptr;
  void                            *msg_payload_ptr;

  sctk_net_ibv_mmu_entry_t        *mmu_entry;
  sctk_net_ibv_rc_rdma_process_t  *entry_rc_rdma;

  int                             directly_pinned;
  void*                           remote_entry;
  size_t                          requested_size;
  uint32_t                        src_process;
  uint32_t                        src_task;
  uint32_t                        psn;  /* Packet Sequence Number */
  double creation_timestamp;
  struct sctk_list_elem*          list_elem;;

  union
  {
    struct
    {

    } send;

    struct
    {

    } recv;
  };
} sctk_net_ibv_rc_rdma_entry_t;


/* TODO: refactor the 2 next structures */
typedef struct
{
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  sctk_net_ibv_rc_sr_msg_type_t           msg_type;

  size_t                    requested_size;
  int                       src_process;

  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;

  struct {
      uintptr_t   address;
      uint32_t    rkey;
    } read_req;

  sctk_thread_ptp_message_t msg_header;
  uint32_t                  psn;  /* Packet Sequence Number */
} sctk_net_ibv_rc_rdma_request_t;


typedef struct
{
  sctk_net_ibv_rc_rdma_request_protocol_t protocol;
  sctk_net_ibv_rc_sr_msg_type_t           msg_type;

  size_t                    requested_size;
  int                       src_process;

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
  int       src_process;
  uint32_t  psn;
  int       if_qp_connection;
  sctk_net_ibv_qp_exchange_keys_t keys;
} sctk_net_ibv_rc_rdma_request_ack_t;

typedef struct
{
  int src_process;
  int psn;

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
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_thread_ptp_message_t * msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma);

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
sctk_net_ibv_comp_rc_rdma_send_coll_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_sr_msg_type_t type);
#endif
