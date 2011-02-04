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
#include <sctk_spinlock.h>
#include "sctk_infiniband.h"
#include "sctk_infiniband_mmu.h"
#include "sctk_inter_thread_comm.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>

/* TODO: Don't change the order of entries */
typedef enum
{
  /* ptp */
  RC_SR_EAGER = 0,
  RC_SR_RDVZ_REQUEST = 1,
  RC_SR_RDVZ_ACK = 2,
  RC_SR_RDVZ_DONE = 3,
  /* collectives */
  RC_SR_BCAST = 4,
  RC_SR_REDUCE = 5
} sctk_net_ibv_rc_sr_msg_type_t;
/* number of different msg type (see the list above) */

/* nb entries in buffers */
  #define IBV_PENDING_SEND_PTP  500
  #define IBV_PENDING_SEND_COLL 500
  #define IBV_PENDING_RECV      500

/*  EAGER  */
typedef struct
{
  sctk_net_ibv_rc_sr_msg_type_t     msg_type;
  int     slot_id;
  size_t  size;
  uint32_t psn;
  unsigned int src_process;

  char    payload;
} sctk_net_ibv_rc_sr_msg_header_t;

typedef enum{
  RC_SR_WR_SEND,
  RC_SR_WR_RECV,
} sctk_net_ibv_rc_sr_wr_type_t;

typedef struct
{
  int                              id;   /* slot's ID */
  union
  {
    struct ibv_send_wr             send_wr;
    struct ibv_recv_wr             recv_wr;
  } wr;
  volatile int                     used; /* if slot used */
  sctk_net_ibv_mmu_entry_t         *mmu_entry; /* MMU entry */
  sctk_net_ibv_rc_sr_msg_header_t  *msg_header;
  struct ibv_sge                   list;
} sctk_net_ibv_rc_sr_entry_t;


typedef struct
{
  int     msg_type;
  int     slot_id;
  size_t  size;
  uint32_t psn;
  unsigned int src_process;
} ibv_buff_msg_header_dummy_t;
#define MSG_HEADER_SIZE sizeof(ibv_buff_msg_header_dummy_t)

typedef struct
{
  int             buff_type;
  int             slot_nb;
  int             slot_size;
  int             current_nb;
  sctk_thread_mutex_t lock;
  uint32_t        wr_begin;
  uint32_t        wr_end;
  sctk_net_ibv_rc_sr_entry_t* headers;
} sctk_net_ibv_rc_sr_buff_t;

typedef sctk_net_ibv_qp_remote_t sctk_net_ibv_rc_sr_process_t;

/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/

sctk_net_ibv_rc_sr_buff_t*
sctk_net_ibv_comp_rc_sr_new(int slot_nb, int slot_size);

void
sctk_net_ibv_comp_rc_sr_free(sctk_net_ibv_rc_sr_buff_t* entry);

/*-----------------------------------------------------------
 * Functions
 *----------------------------------------------------------*/
sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_sr_create_local(sctk_net_ibv_qp_rail_t* rail);

sctk_net_ibv_rc_sr_entry_t*
sctk_net_ibv_comp_rc_sr_pick_header(sctk_net_ibv_rc_sr_buff_t* buff);

void
sctk_net_ibv_comp_rc_sr_send(
  sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_rc_sr_entry_t* entry,
 size_t size, sctk_net_ibv_rc_sr_msg_type_t type, uint32_t* psn);

int
sctk_net_ibv_comp_rc_sr_post_recv(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_buff_t* buff, int i);

void
sctk_net_ibv_comp_rc_sr_init(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_buff_t* buff, sctk_net_ibv_rc_sr_wr_type_t type);

  sctk_net_ibv_rc_sr_entry_t*
sctk_net_ibv_comp_rc_sr_match_entry(sctk_net_ibv_rc_sr_buff_t* buff, void* ptr);

void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_sr_buff_t* buff,
    void* msg, int dest_process, size_t size, sctk_net_ibv_rc_sr_msg_type_t type);

sctk_net_ibv_qp_remote_t*
sctk_net_ibv_comp_rc_sr_check_and_connect(int dest_process);

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
void
sctk_net_ibv_rc_sr_poll_send(
  struct ibv_wc* wc,
  sctk_net_ibv_rc_sr_buff_t* ptp_buff,
  sctk_net_ibv_rc_sr_buff_t* coll_buff);

int
sctk_net_ibv_rc_sr_poll_recv(
  struct ibv_wc* wc,
  sctk_net_ibv_qp_rail_t      *rail,
  sctk_net_ibv_qp_local_t     *rc_sr_local,
  sctk_net_ibv_rc_sr_buff_t   *rc_sr_recv_buff,
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

void
sctk_net_ibv_comp_rc_sr_free_header(sctk_net_ibv_rc_sr_buff_t* buff, sctk_net_ibv_rc_sr_entry_t* entry);

#endif
