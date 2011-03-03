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

typedef enum
{
  RC_RDMA_REQ_WRITE                  = 0,
  RC_RDMA_REQ_READ                   = 1,
} sctk_net_ibv_rc_rdma_request_type_t;

typedef enum
{
  SEND_ENTRY  = 0,
  RECV_ENTRY  = 1,
} sctk_net_ibv_rc_rdma_entry_type_t;

typedef struct
{
  sctk_thread_mutex_t lock;
  /* where are stored send infos */
  struct sctk_list send;
  /* where are stored recv ifos */
  struct sctk_list recv;
  int                     ready;     /* if the qp is ready */
} sctk_net_ibv_rc_rdma_process_t;



/* RC_RDMA entry */
typedef struct
{
  volatile int                    ready;
  /* MPC message header */
  sctk_thread_ptp_message_t       msg_header;
  sctk_net_ibv_mmu_entry_t        *mmu_entry;
  sctk_thread_ptp_message_t       *msg_ptr;
  void                            *aligned_ptr;

  sctk_net_ibv_rc_rdma_process_t  *entry_rc_rdma;
  sctk_net_ibv_rc_rdma_request_type_t type;

  int                             directly_pinned;
  void*                           remote_entry;
  void*                           *ptr; /* pointer */
  size_t                          requested_size;
  int                             src_process;
  uint32_t                        psn;  /* Packet Sequence Number */

  union
  {
    int directly_pinned;

  } send;
  union
  {

  } recv;
} sctk_net_ibv_rc_rdma_entry_t;



typedef struct
{
  size_t                    requested_size;
  sctk_thread_ptp_message_t msg_header;
  int                       src_process;
  uint32_t                  psn;  /* Packet Sequence Number */
  sctk_net_ibv_rc_rdma_request_type_t type;

  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;

  struct {
      uintptr_t   address;
      uint32_t    rkey;
    } read_req;
} sctk_net_ibv_rc_rdma_request_t;


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

//void
//sctk_net_ibv_comp_rc_rdma_free(sctk_net_ibv_rc_rdma_process_t* entry);

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
#if 0

/*-----------------------------------------------------------
 * Functions
 *----------------------------------------------------------*/
sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_rdma_create_local(sctk_net_ibv_qp_rail_t* rail);

void
sctk_net_ibv_comp_rc_rdma_prepare_ack(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_ack_t* ack,
    sctk_net_ibv_rc_rdma_entry_recv_t *entry_recv);

void
sctk_net_ibv_comp_rc_rdma_send_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t  *local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_sr_buff_t* buff_rc_sr,
    sctk_thread_ptp_message_t * msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    int need_connection);

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_add_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request);

  sctk_net_ibv_rc_rdma_entry_recv_t*
sctk_net_ibv_comp_rc_rdma_match_read_msg(int src_process);

void
sctk_net_ibv_comp_rc_rdma_read_msg(
    sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv, int type);

sctk_net_ibv_rc_rdma_entry_recv_t *
sctk_net_ibv_comp_rc_rdma_check_pending_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma);

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_send_msg(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_entry_t* entry);

void
sctk_net_ibv_comp_rc_rdma_done(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_sr_buff_t  *buff_rc_sr,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma);

void
sctk_net_ibv_comp_rc_rdma_post_recv(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv,
    void **ptr, uint32_t *rkey);

/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/

void
sctk_net_ibv_rc_rdma_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_rc_sr_buff_t   *rc_sr_recv_buff,
    int lookup_mode);

void sctk_net_ibv_rc_rdma_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_rc_sr_buff_t* buff_rc_sr);


/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_allocate_init(
    unsigned int rank,
    sctk_net_ibv_qp_local_t *local);

/*-----------------------------------------------------------
 *  ERROR HANDLING
 *----------------------------------------------------------*/

  void
  sctk_net_ibv_comp_rc_rdma_error_handler_send(struct ibv_wc wc);

  void
  sctk_net_ibv_comp_rc_rdma_error_handler_recv(struct ibv_wc wc);
#endif

#endif
