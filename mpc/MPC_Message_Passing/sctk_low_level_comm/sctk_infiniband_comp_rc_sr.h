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

#ifndef __SCTK__INFINIBAND_COMP_RC_SR_H_
#define __SCTK__INFINIBAND_COMP_RC_SR_H_

#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_infiniband.h"
#include "sctk_infiniband_mmu.h"
#include "sctk_infiniband_ibufs.h"
#include "sctk_infiniband_const.h"
#include "sctk_inter_thread_comm.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>

/* TODO: Don't change the order of entries */
typedef enum
{
  /* ptp */
  RC_SR_EAGER =               1 << 0,
  RC_SR_RDVZ_REQUEST =        1 << 1,
  RC_SR_RDVZ_ACK =            1 << 2,
  RC_SR_RDVZ_DONE =           1 << 3,
  /* collectives */
  RC_SR_BCAST =               1 << 4,
  RC_SR_REDUCE =              1 << 5,
  RC_SR_BCAST_INIT_BARRIER =  1 << 6,
  RC_SR_RDVZ_READ =           1 << 7
} sctk_net_ibv_rc_sr_msg_type_t;
/* number of different msg type (see the list above) */

/*  EAGER  */
struct sctk_net_ibv_rc_sr_msg_header_s
{
  sctk_net_ibv_rc_sr_msg_type_t     msg_type;
  uint32_t psn;
  size_t  size;
  size_t  payload_size;
  size_t  buffer_size;
  int buff_nb;
  int total_buffs;
  unsigned int src_process;

  char payload;
} __attribute__ ((aligned (64)));

typedef struct sctk_net_ibv_rc_sr_msg_header_s
sctk_net_ibv_rc_sr_msg_header_t;

#define RC_SR_HEADER(ibuf) \
  ((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer)

#define RC_SR_PAYLOAD(ibuf) \
  ((void*) &(((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer)->payload))

#define RC_SR_HEADER_SIZE \
  sizeof (sctk_net_ibv_rc_sr_msg_header_t)

typedef enum{
  RC_SR_WR_SEND,
  RC_SR_WR_RECV,
} sctk_net_ibv_rc_sr_wr_type_t;


typedef struct
{
  uint32_t psn;
  int src;
  void* msg;
  struct sctk_list_elem *list_elem;
  size_t current_copied;
} sctk_net_ibv_frag_eager_entry_t;

typedef sctk_net_ibv_qp_remote_t sctk_net_ibv_rc_sr_process_t;

/*-----------------------------------------------------------
 * Functions
 *----------------------------------------------------------*/
sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_sr_create_local(sctk_net_ibv_qp_rail_t* rail);

uint32_t
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_ibuf_t* ibuf, size_t size, size_t buffer_size,
    sctk_net_ibv_rc_sr_msg_type_t type, uint32_t* psn, const int buff_nb, const int total_buffs);

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
    void* msg, int dest_process, size_t size, sctk_net_ibv_rc_sr_msg_type_t type);

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

/*-----------------------------------------------------------
 *  ERROR HANDLING
 *----------------------------------------------------------*/

void
  sctk_net_ibv_comp_rc_sr_error_handler(struct ibv_wc wc);

 sctk_net_ibv_frag_eager_entry_t*
sctk_net_ibv_comp_rc_sr_copy_msg(void* buffer, int src, size_t size, uint32_t psn);

#endif
