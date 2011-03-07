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
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_config.h"
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
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;

    default: assume(0); break;
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_local, lookup);
}

void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, lookup, dest);
}

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

    default: assume(0); break;
  }
}

void sctk_net_ibv_allocator_ptp_poll_all()
{
  sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_SR);
  sctk_net_ibv_sched_poll_pending();
}


void sctk_net_ibv_allocator_ptp_lookup_all(int dest)
{
  /* poll pending messages */
  sctk_net_ibv_sched_poll_pending();

  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_SR);
}

/**
 *  \brief Fonction which sends a PTP message
 */
void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  DBG_S(1);
  size_t size;
  sctk_net_ibv_rc_rdma_process_t*     rc_rdma_entry = NULL;
  int need_connection = 0;

  size = sctk_net_determine_message_size(msg);
  /* determine message number */

 sctk_net_ibv_allocator->entry[dest_process].nb_ptp_msg_transfered++;

  /* EAGER MODE */
  if ( (size + sizeof(sctk_thread_ptp_message_t)) + RC_SR_HEADER_SIZE <= ibv_eager_threshold)
  {
   sctk_nodebug("Send EAGER message to %d and size %lu", dest_process, size);
    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, msg,
        dest_process, size, RC_SR_EAGER);
    /* RDVZ MODE */
  } else {

    sctk_net_ibv_allocator_lock(dest_process, IBV_CHAN_RC_RDMA);
   rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);

  /*
   * check if the dest process exists in the RDVZ remote list.
   * If it doesn't, we create a structure for the RC_RDMA entry
   */
   if (!rc_rdma_entry)
   {
    sctk_nodebug("Need connection to process %d", dest_process);

     rc_rdma_entry = sctk_net_ibv_comp_rc_rdma_allocate_init(dest_process, rc_rdma_local);

     sctk_net_ibv_allocator_register(dest_process, rc_rdma_entry, IBV_CHAN_RC_RDMA);
   }
   sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_RDMA);

   sctk_net_ibv_comp_rc_rdma_send_request (
       rail,
       rc_sr_local,
       rc_rdma_local,
       msg,
       dest_process, size, rc_rdma_entry);
  }
  DBG_E(1);
}


/**
 *  Send a collective message to a specific process. According to the size of
 *  the process, the message will use eager or rdvz protocol
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_coll_message(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_sr_msg_type_t type)
{

  /* EAGER MODE */
  if ( (size + sizeof(sctk_thread_ptp_message_t)) + RC_SR_HEADER_SIZE <= ibv_eager_threshold)
  {
    sctk_nodebug("Send EAGER collective");

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, msg,
        dest_process, size, type);

  } else { /* RDVZ MODE */
    sctk_nodebug("Send RDVZ collective");
    sctk_net_ibv_rc_rdma_process_t* rc_rdma_entry;

    sctk_net_ibv_allocator_lock(dest_process, IBV_CHAN_RC_RDMA);
   rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);
  /*
   * check if the dest process exists in the RDVZ remote list.
   * If it doesn't, we create a structure for the RC_RDMA entry
   */
   if (!rc_rdma_entry)
   {
     rc_rdma_entry = sctk_net_ibv_comp_rc_rdma_allocate_init(dest_process, rc_rdma_local);
     sctk_net_ibv_allocator_register(dest_process, rc_rdma_entry, IBV_CHAN_RC_RDMA);
   }
   sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_RDMA);

   sctk_net_ibv_comp_rc_rdma_send_coll_request(
     rail, local_rc_sr, msg,
     dest_process, size, rc_rdma_entry, type);
  }
  DBG_E(1);
}
