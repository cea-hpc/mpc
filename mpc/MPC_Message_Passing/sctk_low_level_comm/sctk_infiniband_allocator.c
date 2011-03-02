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

#include "sctk_list.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_const.h"
#include "sctk_bootstrap.h"
#include "sctk.h"
#include "sctk_thread.h"
#include "sctk_config.h"

sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;

  /* FIXME: change to HashTables */
  size_t size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);

  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t));

  sctk_net_ibv_allocator->entry = sctk_malloc(size);
  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_sr_lock, NULL);
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_rdma_lock, NULL);
  }

  /* init pending queues */
  sctk_net_ibv_sched_pending_init(IBV_CHAN_RC_SR);
  sctk_net_ibv_sched_pending_init(IBV_CHAN_RC_RDMA);
  return sctk_net_ibv_allocator;
}

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      if (sctk_net_ibv_allocator->entry[rank].rc_sr)
      {
        sctk_error("Process %d already registered to sctk_net_ibv_allocator RC_SR", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_sr = entry;
      break;

    case IBV_CHAN_RC_RDMA:
      if (sctk_net_ibv_allocator->entry[rank].rc_rdma)
      {
        sctk_error("Process %d already registered to chan RC_RDMA", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_rdma = entry;
      sctk_nodebug("Registering %p", &sctk_net_ibv_allocator->entry[rank]);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;

    default: assume(0); break;
  }
}

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      return sctk_net_ibv_allocator->entry[rank].rc_sr;
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_nodebug("Geting %p", &sctk_net_ibv_allocator->entry[rank]);
      sctk_nodebug("Allocator get : %p", sctk_net_ibv_allocator->entry[rank].rc_rdma);
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;

    default: assume(0); break;
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  RDMA
 *----------------------------------------------------------*/
#if 0
void
sctk_net_ibv_allocator_rc_rdma_process_next_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma)
{
  sctk_net_ibv_rc_rdma_request_ack_t* ack;
  sctk_net_ibv_qp_remote_t* remote_rc_sr;
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_entry_recv_t *last_entry_recv = NULL;


  sctk_list_lock(&entry_rc_rdma->recv);
  last_entry_recv = sctk_net_ibv_comp_rc_rdma_check_pending_request(entry_rc_rdma);

  /* if we have pop an element */
  if ( last_entry_recv )
  {
    sctk_nodebug("\t\tEntry with PSN %lu set to used=1", last_entry_recv->psn);
    last_entry_recv->used = 1;

    sctk_list_unlock(&entry_rc_rdma->recv);

    sctk_nodebug("there is one message to perform (dest %d PSN %d)!",
        last_entry_recv->src_process, last_entry_recv->psn);

    sctk_nodebug("Element to pop");
    entry = sctk_net_ibv_comp_rc_sr_pick_header(rc_sr_ptp_send_buff);
    ack = (sctk_net_ibv_rc_rdma_request_ack_t* ) &(entry->msg_header->payload);

    /* post buffer and register ptr*/
    sctk_net_ibv_comp_rc_rdma_post_recv(
        rail,
        entry_rc_rdma,
        rc_rdma_local, last_entry_recv,
        &(ack->dest_ptr), &(ack->dest_rkey) );


    sctk_net_ibv_comp_rc_rdma_prepare_ack(rail, entry_rc_rdma, ack,
        last_entry_recv);

    /* send msg RC_SR_RDVZ_ACK */
    remote_rc_sr = sctk_net_ibv_allocator_get(last_entry_recv->src_process, IBV_CHAN_RC_SR);

    sctk_net_ibv_comp_rc_sr_send(remote_rc_sr, entry, sizeof(sctk_thread_ptp_message_t), RC_SR_RDVZ_ACK, NULL);
  } else {
    sctk_list_unlock(&entry_rc_rdma->recv);
  }
}


/*-----------------------------------------------------------
 *  SEARCH FUNCTIONS
 *----------------------------------------------------------*/

  sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(uint32_t qp_num)
{
  sctk_net_ibv_rc_rdma_process_t          *entry_rdma = NULL;

  int i;
  for(i=0; i < sctk_process_number; ++i)
  {
    entry_rdma = sctk_net_ibv_allocator_get(i, IBV_CHAN_RC_RDMA);

    if (entry_rdma
        && (entry_rdma->remote.qp->qp_num == qp_num))
    {
      sctk_nodebug("FOUND process %d", entry_rdma->remote.rank);
      return entry_rdma;
    }
  }
  return NULL;
}
#endif

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc);
}

void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, lookup, dest);
}


/*-----------------------------------------------------------
 *  POLLING RC RDMA
 *----------------------------------------------------------*/
#if 0
void sctk_net_ibv_rc_rdma_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_rdma_poll_recv(wc,
      rc_sr_local,
      rc_sr_ptp_send_buff,
      lookup);
}

void sctk_net_ibv_rc_rdma_send_cq(struct ibv_wc* wc, int lookup, int dest)
{

  sctk_net_ibv_rc_rdma_poll_send(wc,
      rail, rc_sr_local,
      rc_sr_ptp_send_buff);
}
#endif

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
void sctk_net_ibv_allocator_ptp_poll(sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, SCTK_PENDING_IN_NUMBER, sctk_net_ibv_rc_sr_recv_cq, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_poll(rc_sr_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_sr_send_cq, type | IBV_CHAN_SEND);
      break;
#if 0
    case IBV_CHAN_RC_RDMA :
      sctk_net_ibv_cq_poll(rc_rdma_local->recv_cq, SCTK_PENDING_IN_NUMBER, sctk_net_ibv_rc_rdma_recv_cq, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_poll(rc_rdma_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_rdma_send_cq, type | IBV_CHAN_SEND);
      break;
#endif

    default: assume(0); break;
  }
}

void
sctk_net_ibv_allocator_ptp_lookup(int dest, sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_lookup(rc_sr_local->recv_cq, 1, sctk_net_ibv_rc_sr_recv_cq, dest, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_lookup(rc_sr_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_sr_send_cq, dest, type | IBV_CHAN_SEND);
      break;
#if 0
    case IBV_CHAN_RC_RDMA :
      sctk_net_ibv_cq_lookup(rc_rdma_local->recv_cq, 1, sctk_net_ibv_rc_rdma_recv_cq, dest, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_lookup(rc_rdma_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_rdma_send_cq, dest, type | IBV_CHAN_SEND);
      break;
#endif

    default: assume(0); break;
  }
}

void sctk_net_ibv_allocator_ptp_poll_all()
{
  sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_SR);
//  sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_RDMA);

  sctk_net_ibv_sched_poll_pending();
}


void sctk_net_ibv_allocator_ptp_lookup_all(int dest)
{
  /* poll pending messages */
  sctk_net_ibv_sched_poll_pending();

  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_SR);
//  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_RDMA);
}


/* * * * * * * * * * * * * * *
 * TCP SERVER
 * * * * * * * * * * * * * * */


