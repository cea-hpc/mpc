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

#ifndef __SCTK__INFINIBAND_COMP_RC_SR_H_
#define __SCTK__INFINIBAND_COMP_RC_SR_H_

#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_ib.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_const.h"
#include "sctk_inter_thread_comm.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>

/*  EAGER  */
#define RC_SR_HEADER(buffer) \
  ((sctk_net_ibv_ibuf_header_t*) buffer)

#define RC_SR_PAYLOAD(buffer) \
  ((char*) buffer + sizeof(sctk_net_ibv_ibuf_header_t))

#define RC_SR_HEADER_SIZE \
  sizeof (sctk_net_ibv_ibuf_header_t)

struct sctk_net_ibv_allocator_request_s;

typedef enum{
  RC_SR_WR_SEND,
  RC_SR_WR_RECV,
} sctk_net_ibv_rc_sr_wr_type_t;

typedef struct
{
  uint32_t psn;
  int src_process;
  unsigned int src_task;
  unsigned int dest_task;
  void* msg;
  struct sctk_list_elem *list_elem;
  size_t current_copied;
  /* FIXME: for debug */
  int buff_nb;
  int total_buffs;
  size_t size;
} sctk_net_ibv_frag_eager_entry_t;

typedef sctk_net_ibv_qp_remote_t sctk_net_ibv_rc_sr_process_t;
struct sctk_net_ibv_allocator_request_s;

/*-----------------------------------------------------------
 * Functions
 *----------------------------------------------------------*/

/*-----------------------------------------------------------
 *  FRAG MSG
 *----------------------------------------------------------*/
void
sctk_net_ibv_comp_rc_sr_send_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    struct sctk_net_ibv_allocator_request_s req);

void
sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    struct sctk_net_ibv_allocator_request_s req);

sctk_net_ibv_frag_eager_entry_t* sctk_net_ibv_comp_rc_sr_frag_allocate(
    sctk_net_ibv_ibuf_header_t* msg_header);

  void
sctk_net_ibv_comp_rc_sr_free_frag_msg(sctk_net_ibv_frag_eager_entry_t* entry);

/*-----------------------------------------------------------
 *  NORMAL MSG
 *----------------------------------------------------------*/
sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_sr_create_local(sctk_net_ibv_qp_rail_t* rail);

void
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_ibuf_t* ibuf,
    struct sctk_net_ibv_allocator_request_s req, sctk_net_ibv_ibuf_type_t type);

int
sctk_net_ibv_comp_rc_sr_post_recv(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_ibuf_t* buff, int i);

void
sctk_net_ibv_comp_rc_sr_init(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_ibuf_t* buff, sctk_net_ibv_rc_sr_wr_type_t type);

void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    struct sctk_net_ibv_allocator_request_s req);

sctk_net_ibv_qp_remote_t*
sctk_net_ibv_comp_rc_sr_check_and_connect(int dest_process);

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/

void
sctk_net_ibv_rc_sr_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    int lookup_mode);

void
sctk_net_ibv_rc_sr_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t      *rail,
    sctk_net_ibv_qp_local_t     *rc_sr_local,
    sctk_net_ibv_qp_local_t     *rc_rdma_local,
    int                         lookup_mode,
    int                         dest);

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/

sctk_net_ibv_qp_remote_t *
sctk_net_ibv_comp_rc_sr_allocate_init(int rank,
    sctk_net_ibv_qp_local_t* local);

void
sctk_net_ibv_comp_rc_sr_allocate_send(
    sctk_net_ibv_qp_rail_t* rail,
    sctk_net_ibv_qp_remote_t *remote);

void
sctk_net_ibv_comp_rc_sr_allocate_recv(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t *remote,
    int rank);

#endif
#endif
