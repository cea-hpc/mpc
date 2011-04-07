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

#ifdef MPC_USE_INFINIBAND

#include "sctk_list.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_ib_coll.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_config.h"
#include "sctk_ib_comp_rc_rdma.h"
#include "sctk_ib_profiler.h"
#include "sctk_ib_allocator.h"


/* rail */
extern  sctk_net_ibv_qp_rail_t   *rail;
/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;

sctk_net_ibv_com_entry_t* com_entries;

int* comm_world_ranks;

/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  inline void
  sctk_net_ibv_collective_init()
{
  int i;

  com_entries = sctk_calloc(sizeof(char), SCTK_MAX_COMMUNICATOR_NUMBER * sizeof(sctk_net_ibv_com_entry_t));
  assume(com_entries);

  /* init the comm world array */
  comm_world_ranks = sctk_malloc(sctk_process_number * sizeof(int));
  for (i=0; i < sctk_process_number; ++i)
  {
    comm_world_ranks[i] = i;
  }
}


/**
 *  Function which updates a communicator. In fact, it resets sequence numbers
 *  to 0.
 *  \param
 *  \return
 */
void sctk_net_ibv_collective_new_com ( const sctk_internal_communicator_t * __com,
                             const int nb_involved, const int *task_list ){
 sctk_nodebug("UPDATE for com %di (envolved %d)",
     __com->communicator_number, nb_involved);
 sctk_net_ibv_com_entry_t* com_entry;


 /* We create pending msg queues if they don't exist
  * TODO: Clear all previous entries from queue */
  com_entry = &com_entries[__com->communicator_number];
  sctk_spinlock_lock(&com_entry->lock);
  if (!&com_entry->broadcast_fifo)
  {
    sctk_list_new(&com_entry->broadcast_fifo, 0, 0);
    assume(sctk_list_is_empty(&com_entry->broadcast_fifo));
  }
  if (!&com_entry->reduce_fifo)
  {
    sctk_list_new(&com_entry->reduce_fifo, 0, 0);
    assume(sctk_list_is_empty(&com_entry->reduce_fifo));
  }
  if (!&com_entry->init_barrier_fifo)
  {
    sctk_list_new(&com_entry->init_barrier_fifo, 0, 0);
    assume(sctk_list_is_empty(&com_entry->init_barrier_fifo));
  }
  sctk_spinlock_unlock(&com_entry->lock);

/* We reset counters for the communicator */
 sctk_net_ibv_sched_coll_reset (__com->communicator_number);
}


/**
 *  Push an eager and split eager to a collective pending queue.
 *  \param
 *  \return
 */
  void*
  sctk_net_ibv_collective_push_rc_sr(struct sctk_list* list, sctk_net_ibv_ibuf_t* ibuf, int *release_buffer)
{
  sctk_net_ibv_collective_pending_t* pending = NULL;
  sctk_net_ibv_ibuf_header_t* msg_header;
  struct sctk_list_elem *tmp = NULL;

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  /* if this is a fragmented buffer */
  if (msg_header->total_buffs != 1)
  {
    /* we allocate the entry for the split eager if this is
     * the first fragment */
    if (msg_header->buff_nb == 1)
    {
      sctk_nodebug("%p - allocated memory and copy msg for psn %d", list, msg_header->psn);
      pending = sctk_malloc(sizeof(sctk_net_ibv_collective_pending_t) + msg_header->size);
      assume(pending);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);

      pending->payload = (char*) pending + sizeof(sctk_net_ibv_collective_pending_t);
      pending->size = msg_header->payload_size;
      pending->src_process = msg_header->src_process;
      pending->type = RC_SR_FRAG_EAGER;
      pending->psn = msg_header->psn;
      pending->current_copied = 0;
      pending->ready = 0;

      /* push the message to the list */
      sctk_list_lock(list);
      sctk_list_push(list, pending);
      sctk_list_unlock(list);
    }

    /*
     * if this is the first segment, we don't need to look for the entry
     */
    if (!pending)
    {
      sctk_list_lock(list);
      tmp = list->head;
      while (tmp) {

        pending = (sctk_net_ibv_collective_pending_t*) tmp->elem;

        if ( (msg_header->psn == pending->psn) && (msg_header->src_process == pending->src_process))
        {
          sctk_nodebug("FOUND %lu", pending->psn);
          memcpy( (char*) pending->payload + pending->current_copied, RC_SR_PAYLOAD(ibuf->buffer),
              msg_header->payload_size);
          pending->current_copied += msg_header->payload_size;

          sctk_nodebug("%p - Frag msg copied (size: %lu, psn %lu, copied %lu, payload %lu)", list,
              msg_header->size, msg_header->psn, pending->current_copied, msg_header->payload_size);

          break;
        }

        tmp = tmp->p_next;
      }
    }
    sctk_list_unlock(list);

    if (!pending)
    {
      sctk_nodebug("%p - Frag entry not found (size: %lu, psn %lu, payload %lu)",
          list, msg_header->size, msg_header->psn, msg_header->payload_size);
      assume(0);
    }

    /*
     * If this is the last segment of buffer, we set it as 'ready'
     */
    if (msg_header->buff_nb == msg_header->total_buffs)
    {
      sctk_nodebug("Msg ready !! (psn:%lu,src:%lu,list:%p)", pending->psn, pending->src_process, list);
      pending->src_process = msg_header->src_process;
      pending->ready = 1;
    }

    /* We release buffers now */
    *release_buffer = 1;
  } else {
    sctk_nodebug("src:%lu,psn:%lu", msg_header->src_process, msg_header->psn);

    pending = sctk_malloc(sizeof(sctk_net_ibv_collective_pending_t));
    assume(pending);
    sctk_ibv_profiler_inc(IBV_MEM_TRACE);

    pending->payload = ibuf;
    pending->size = msg_header->payload_size;
    pending->src_process = msg_header->src_process;
    pending->type = RC_SR_EAGER;
    pending->psn = msg_header->psn;
    pending->ready = 1;

    sctk_list_lock(list);
    sctk_list_push(list, pending);
    sctk_list_unlock(list);

    /* We don't release the current buffer*/
    *release_buffer = 0;
  }

  return pending;
}

  void*
sctk_net_ibv_collective_push_rc_rdma(struct sctk_list* list, sctk_net_ibv_rc_rdma_entry_t* entry)
{
  sctk_net_ibv_collective_pending_t* pending;
  sctk_net_ibv_rc_rdma_process_t *entry_rdma;
  struct sctk_list_elem* rc;

  pending = sctk_malloc(sizeof(sctk_net_ibv_collective_pending_t));
  assume(pending);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  pending->payload      = entry->msg_payload_ptr;
  pending->src_process  = entry->src_process;
  pending->type         = RC_SR_RDVZ_READ;
  pending->size         = entry->requested_size;
  pending->psn          = entry->psn;
  pending->ready = 1;
  entry_rdma            = entry->entry_rc_rdma;
  sctk_nodebug("Push with src %d, size %lu, ptr %p", entry->src_process, entry->requested_size, entry->msg_payload_ptr);

  /* we remove the entry from the rendezvous send list ... */
  sctk_list_lock(&entry_rdma->send);
  rc = sctk_list_remove(&entry_rdma->send, entry->list_elem);
  sctk_list_unlock(&entry_rdma->send);
  assume(rc);

  sctk_nodebug("pushed");

  /* ...and we push the received collective into the pending list */
  sctk_list_lock(list);
  sctk_list_push(list, pending);
  sctk_list_unlock(list);

  sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);
  sctk_free(entry);
  sctk_ibv_profiler_dec(IBV_MEM_TRACE);

  return pending;
}


#define COLLECTIVE_LOOKUP(list, src, psn, msg) \
do {    \
  msg = sctk_net_ibv_collective_lookup(list, src, psn);    \
  sctk_net_ibv_allocator_ptp_poll_all();    \
/*        sctk_thread_yield(); */   \
} while(msg == NULL);   \

/**
 *  Look for a pending collective entry according to the src process and the psn
 *  \param
 *  \return
 */
  sctk_net_ibv_collective_pending_t*
sctk_net_ibv_collective_lookup(struct sctk_list* list, const int src, const int psn)
{
  struct sctk_list_elem* elem;
  sctk_net_ibv_collective_pending_t* msg;
  int i = 0;

  if (!list || src < 0)
    return NULL;

  sctk_list_lock(list);
  elem = list->head;
  while(elem)
  {
    msg = (sctk_net_ibv_collective_pending_t*) elem->elem;
    sctk_nodebug("THERE msg %p src process %d", msg, src);

    if ( (msg->src_process == src) && (msg->psn == psn) && (msg->ready))
    {
      sctk_nodebug("Found msg %p src process %d(src %d), psn %lu(%lu)", msg, msg->src_process, src, msg->psn, psn);
      sctk_list_remove(list, elem);
      sctk_list_unlock(list);
      return msg;
    }
    sctk_nodebug("i=%d", i);
    i++;
    elem = elem->p_next;
  }
  sctk_list_unlock(list);
  return NULL;
}

  static inline void
sctk_net_ibv_broadcast_recv(void* data, size_t size,
    const int relative_rank, int* mask, struct sctk_list* list, uint32_t psn, int* local_ranks, int local_rank, int com_size)
{
  int src;
  sctk_net_ibv_collective_pending_t* msg;
  sctk_net_ibv_ibuf_t* ibuf;

  sctk_nodebug("RECV psn; %lu", psn);

  while(*mask < com_size)
  {
    if (relative_rank & *mask)
    {
      src = local_rank - *mask;
      if (src < 0)
        src += com_size;

      sctk_nodebug("Waiting broadcast from process %d", local_ranks[src]);
      /* received from process src */
      COLLECTIVE_LOOKUP(list, local_ranks[src], psn, msg);

      sctk_nodebug("Broadcast received from process %d size %lu", local_ranks[src], msg->size);

      if (msg->type == RC_SR_EAGER)
      {
        ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;
        memcpy(data,
            RC_SR_PAYLOAD(ibuf->buffer), size);

        /* restore buffers */
        sctk_net_ibv_ibuf_release(ibuf, 1);
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
      } else if (msg->type == RC_SR_FRAG_EAGER) {
        memcpy(data, msg->payload, size);
      } else {
        memcpy(data,
            msg->payload, size);
        free(msg->payload);
      }

      sctk_free(msg);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;
    }
    *mask <<=1;
  }
}

  static inline void
sctk_net_ibv_broadcast_send(sctk_collective_communications_t * com,
    void* data, size_t size, unsigned int relative_rank, int* mask, sctk_net_ibv_ibuf_type_t type, uint32_t psn, int* local_ranks, int local_rank, int com_size)
{
  int dest;

  sctk_nodebug("Broadcast send to process %d, relative_rank %d, mask %d (com_size:%d)", local_ranks[dest], relative_rank, *mask, com_size);

  *mask >>= 1;
  sctk_nodebug("mask %d", *mask);
  while(*mask > 0)
  {
    if(relative_rank + *mask < com_size)
    {
      dest = local_rank + *mask;
      if (dest >= com_size) dest -= com_size;

      sctk_net_ibv_allocator_send_coll_message(
          rail, rc_sr_local, com, data, local_ranks[dest], size, type, psn);

    }
    *mask >>= 1;
  }
  sctk_nodebug("End of broadcast send");
}

  static void
sctk_net_ibv_broadcast_binomial ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root, const int psn)
{
  int size;
  int relative_rank;
  int mask;
  int root_process;
  int com_size;
  int *local_ranks;
  int local_rank = -1;
  int i;
  int index = 0;

  /* We convert all global ranks to local ranks
   * for the current comminucator. If an entry is != in the
   * array 'involved_nb', the process corresponding to the entry
   * is a part of the allreduce */
  com_size = com->involved_nb;
  local_ranks = sctk_malloc(com_size * sizeof(int));
  for (i=0; i < sctk_process_number; ++i)
  {
    if (com->involved_list[i])
    {
      /* We save the local rank of the current process */
      if (sctk_process_rank == i)
      {
        local_rank = index;
      }
      local_ranks[index] = i;
      index++;
    }
  }


  size = elem_size * nb_elem;

  sctk_nodebug("Begin broadcast (size:%lu,psn:%d,size:%d)", size, psn, com_size);

  /*
   * we have to compute where the task is located, in which process.
   * The root argument given to the function returns the rank of the MPI
   * task
   */
  root_process = sctk_get_ptp_process_localisation(root);
  sctk_nodebug("Broadcast from root %d (com_size %d)", root_process, com_size);

  sctk_nodebug("my_vp->data.data_out %s, my_vp->data.data_in %s", my_vp->data.data_out, my_vp->data.data_in);

  mask = 0x1;
  /* compute relative rank for process */

  relative_rank = (local_rank - root + com_size) % com_size;

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, relative_rank,
      &mask, &com_entries[com->id].broadcast_fifo, psn, local_ranks,
      local_rank, com_size);

  if ( root_process == sctk_process_rank )
  {
    if(my_vp->data.data_out)
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );

    sctk_net_ibv_broadcast_send(com, my_vp->data.data_in, size, relative_rank, &mask, IBV_BCAST, psn, local_ranks, local_rank, com_size);

  } else {
    sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, relative_rank, &mask, IBV_BCAST, psn, local_ranks, local_rank, com_size);
  }
  sctk_nodebug("End of broadcast %d",psn);
}

uint64_t bcast_scatter_nb=0;
  static void
sctk_net_ibv_broadcast_scatter ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root, const int psn)
{
  int relative_rank, mask;
  int scatter_size, curr_size, recv_size = 0, send_size;
  int root_process;
  int src, dst;
  sctk_net_ibv_collective_pending_t* msg;
  sctk_net_ibv_ibuf_t* ibuf;
  size_t size;
  struct sctk_list *list = &com_entries[com->id].broadcast_fifo;

  size = elem_size * nb_elem;

  bcast_scatter_nb++;

  /*
   * we have to compute where the task is located, in which process.
   * The root argument given to the function returns the rank of the MPI
   * task
   */
  root_process = sctk_get_ptp_process_localisation(root);
  sctk_nodebug("%d - broadcast scatter bcast from root %d", bcast_scatter_nb,
      root_process);

  sctk_nodebug("my_vp->data.data_out %s, my_vp->data.data_in %s", my_vp->data.data_out, my_vp->data.data_in);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root_process) ?
    sctk_process_rank - root_process :
    sctk_process_rank - root_process + sctk_process_number;

  /* compute the scatter size */
  scatter_size = (size + sctk_process_number - 1) / sctk_process_number;
  sctk_nodebug("Scatter size : %lu (size %lu)", scatter_size, size);

  /* * root starts with the whole data */
  curr_size = (sctk_process_rank == root_process) ? size : 0;

  while(mask < sctk_process_number)
  {
    if (relative_rank & mask)
    {
      src = sctk_process_rank - mask;
      if (src < 0) src += sctk_process_number;
      recv_size = size - relative_rank * scatter_size;

      if (recv_size <= 0)
      {
        /* the process doesn't receive any data because of the uneven
         * division */
        curr_size = 0;
      }
      else
      {
        sctk_nodebug("Received bcast msg from process %d", src);
        COLLECTIVE_LOOKUP(list, src, psn, msg);

        sctk_nodebug("Broadcast received from process %d size %lu (psn:%d)", src, msg->size, msg->psn);

        if (msg->type == RC_SR_EAGER)
        {
          ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;

          memcpy( (char*) my_vp->data.data_out +
              relative_rank * scatter_size,
              RC_SR_PAYLOAD(ibuf->buffer), recv_size);

          /* restore buffers */
          sctk_net_ibv_ibuf_release(ibuf, 1);
          sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
        } else {
          if( recv_size != msg->size)
          {
            sctk_nodebug("\t\t========== Problem : %lu - %lu", recv_size, msg->size);
            sctk_abort();
          }
          //          assume(recv_size == msg->size);
          //          assume (0);
          memcpy(
              (char*) my_vp->data.data_out +
              relative_rank * scatter_size,
              msg->payload, recv_size);
          free(msg->payload);
        }
        sctk_free(msg);

        curr_size = msg->size;
        sctk_nodebug("current size : %lu", curr_size);

      }
      break;
    }
    sctk_nodebug("New mask %d", mask);
    mask <<= 1;
  }

  sctk_nodebug("Sendind phase %d", mask);


  if ( root_process == sctk_process_rank )
  {
    if(my_vp->data.data_out)
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );
  }

  mask >>= 1;
  while(mask > 0)
  {
    sctk_nodebug("%d - %d", relative_rank + mask, sctk_process_number);
    if (relative_rank + mask < sctk_process_number)
    {
      send_size = curr_size - scatter_size * mask;

      sctk_nodebug("Send size : %d", send_size);

      if (send_size > 0)
      {
        dst = sctk_process_rank + mask;
        if (dst >= sctk_process_number) dst -= sctk_process_number;

        if ( root_process == sctk_process_rank )
        {
          sctk_nodebug("1 - Send bcast to process %d (size: %lu, psn: %lu)", dst, send_size, psn);
          sctk_net_ibv_allocator_send_coll_message(
              rail, rc_sr_local, com, my_vp->data.data_in + scatter_size*(relative_rank+mask),
              dst, send_size, IBV_BCAST, psn);

        } else {
          sctk_nodebug("2 - Send bcast to process %d (psn %lu)", dst, psn);
          sctk_net_ibv_allocator_send_coll_message(
              rail, rc_sr_local, com, my_vp->data.data_out + scatter_size*(relative_rank+mask),
              dst, send_size, IBV_BCAST, psn);
        }

        curr_size -= send_size;
      }
    }
    sctk_nodebug("New mask %d", mask);
    mask >>= 1;
  }
  sctk_nodebug("%d Exit bcast", psn);
}


/**
 *  Send a broadcast message to a set of processes. According
 *  to the size of the message, the broadcast method is different.
 *  \param
 *  \return
 */
  void
sctk_net_ibv_broadcast ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root)
{
  int size;
  uint32_t psn;

  /* FIXME: Other communicators */
  //  assume(com->id == 0);

  sctk_nodebug("Begin broadcast (size:%lu)", size);

  size = elem_size * nb_elem;
  psn = sctk_net_ibv_sched_coll_psn_inc(IBV_BCAST, com->id);

  //  if (size + RC_SR_HEADER_SIZE <= ibv_eager_threshold)
  sctk_net_ibv_broadcast_binomial (com, my_vp, elem_size, nb_elem, root, psn);
  //  else
  //    sctk_net_ibv_broadcast_scatter (com, my_vp, elem_size, nb_elem, root, psn);

  sctk_nodebug("Leave broadcast %d", psn);
}

void
sctk_net_ibv_allreduce ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type) {

  int size;
  int i;
  sctk_net_ibv_collective_pending_t* msg;
  sctk_net_ibv_ibuf_t* ibuf;
  int mask;
  int relative_rank;
  /* root process is process 0 */
  int root = 0;
  int psn;
  int *local_ranks;
  int local_rank = -1;
  int index = 0;
  int source;
  int com_size;

  /* We convert all global ranks to local ranks
   * for the current comminucator. If an entry is != in the
   * array 'involved_nb', the process corresponding to the entry
   * is a part of the allreduce */
  com_size = com->involved_nb;
  local_ranks = sctk_malloc(com_size * sizeof(int));
  for (i=0; i < sctk_process_number; ++i)
  {
    if (com->involved_list[i])
    {
      /* We save the local rank of the current process */
      if (sctk_process_rank == i)
      {
        local_rank = index;
      }
      local_ranks[index] = i;
      index++;
    }
  }
  assume(local_rank!=-1);

  for (i=0; i < com_size; ++i)
  {
    sctk_nodebug("local %d - global %d", i, local_ranks[i]);
  }

  /* compute relative rank for the current process */
  relative_rank = (local_rank - root + com_size) % com_size;
  sctk_nodebug("relative rank : %d", relative_rank);

  size = elem_size * nb_elem;
  psn = sctk_net_ibv_sched_coll_psn_inc(IBV_REDUCE, com->id);
  mask = 0x1;
  sctk_nodebug("Begin of reduce operation with psn %lu", psn);

  while (mask < com_size)
  {
    if ((mask & relative_rank) == 0)
    {
      source = (relative_rank | mask);

      if (source < com_size) {
        source = (source + root) % com_size;

        sctk_nodebug("Receive from local:%d (global:%d,list:%p)", source, local_ranks[source], &reduce_fifo);

        COLLECTIVE_LOOKUP(&com_entries[com->id].reduce_fifo, local_ranks[source], psn, msg);
        ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;

        sctk_nodebug("GOT %d", msg->psn);

        /* read the content of the message */
        if (msg->type == RC_SR_EAGER)
        {
          /* function execution */
          func ( RC_SR_PAYLOAD(ibuf->buffer), my_vp->data.data_out, nb_elem, data_type );
          /* restore buffers */
          sctk_net_ibv_ibuf_release(ibuf, 1);
          sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
        } else if (msg->type == RC_SR_FRAG_EAGER) {
          func ( msg->payload, my_vp->data.data_out, nb_elem, data_type );
        } else {
          func ( msg->payload, my_vp->data.data_out, nb_elem, data_type );
          free(msg->payload);
        }

        free(msg);
        sctk_ibv_profiler_dec(IBV_MEM_TRACE);

      }
    } else {
      /* We receive reduce message from all expected processes.
       * Send the message to the parent */
      source = ((relative_rank & (~ mask)) + root) % com_size;

      sctk_nodebug("Send to local:%d (global:%d,psn:%d)", source, local_ranks[source],psn);
      sctk_net_ibv_allocator_send_coll_message(
          rail, rc_sr_local, com,
          my_vp->data.data_out,
          local_ranks[source], size, IBV_REDUCE, psn);
    }
    mask <<= 1;
  }

  sctk_nodebug("Broadcast phase");

  /* broadcast */
  mask = 0x1;

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, relative_rank,
      &mask, &com_entries[com->id].broadcast_fifo, psn, local_ranks, local_rank, com_size);

  sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, relative_rank, &mask, IBV_BCAST, psn, local_ranks, local_rank, com_size);

  sctk_nodebug ( "Broadcast received", ((int*) my_vp->data.data_out)[0] );

  sctk_nodebug("End of reduce operation");
}

/*-----------------------------------------------------------
 *                        BARRIER
 *----------------------------------------------------------*/
typedef struct
{
  int* entry;

  sctk_net_ibv_mmu_entry_t        *mmu_entry;

  struct ibv_recv_wr  wr;
  struct ibv_sge      list;

} local_entry_t;


typedef struct
{
  int* entry;
  uint32_t rkey;

  sctk_net_ibv_ibuf_t ibuf;

} remote_entry_t;

typedef struct
{
  uint32_t rkey;
  uint32_t lkey;

} barrier_local_t;


local_entry_t   local_entry;
remote_entry_t*  remote_entry;

#define N_WAY_DISSEMINATION 2

typedef struct
{
  int* entry;
  uint32_t rkey;
} request_msg_t;

/* FIXME Problem with uint8_t */
//static uint64_t counter = 0;

static int is_barrier_initialized = 0;

void
sctk_net_ibv_broadcast_barrier (
    void* data, const size_t size, const int root) {

  int relative_rank;
  int mask;

  sctk_nodebug("Broadcast barrier init from root %d", root);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(data, size, relative_rank, &mask, &com_entries[0].init_barrier_fifo, 0, comm_world_ranks, sctk_process_rank, sctk_process_number);

  sctk_net_ibv_broadcast_send(NULL, data, size, relative_rank, &mask, IBV_BCAST_INIT_BARRIER, 0, comm_world_ranks, sctk_process_rank, sctk_process_number);

  sctk_nodebug("Leave broadcast");
}

  static void
sctk_net_ibv_barrier_post_recv(local_entry_t* local)
{

  /* allocate memory */
  sctk_posix_memalign( (void*) &local->entry,
      sctk_net_ibv_mmu_get_pagesize(), sizeof(int) * sctk_process_number);
  assume(local->entry);
  memset(local_entry.entry, 0, sizeof(int) * sctk_process_number);

  /* register memory */
  local->mmu_entry =
    sctk_net_ibv_mmu_register(rail, rc_sr_local,
        local->entry, sizeof(int) * sctk_process_number);

}

  static inline void
sctk_net_ibv_barrier_send(local_entry_t* local, remote_entry_t* remote,
    int entry_nb, int dest_process)
{

  sctk_net_ibv_ibuf_barrier_send_init(&remote[dest_process].ibuf,
      &local->entry[entry_nb],
      local->mmu_entry->mr->lkey,
      ((int*)remote[dest_process].entry + entry_nb),
      remote[dest_process].rkey,
      sizeof(int));

  sctk_nodebug("Entry id %d for process %d", remote[dest_process].wr.wr_id, dest_process);

  sctk_net_ibv_qp_send_get_wqe(dest_process, &(remote[dest_process].ibuf));

  sctk_nodebug("sent");
}

  void
sctk_net_ibv_barrier_init ()
{
  int i;
  request_msg_t request_msg;

  sctk_nodebug("Begin init barrier");

  remote_entry = sctk_malloc(sizeof(remote_entry_t) * sctk_process_number);

  sctk_net_ibv_barrier_post_recv(&local_entry);

  /* Exchange barrier addresses between peers */
  for (i=0; i < sctk_process_number; ++i)
  {
    if (i == sctk_process_rank)
    {
      request_msg.entry = local_entry.entry;
      request_msg.rkey  = local_entry.mmu_entry->mr->rkey;

      sctk_nodebug("Send entry %p (rkey: %lu)", local_entry.entry, request_msg.rkey);
      sctk_net_ibv_broadcast_barrier(&request_msg, sizeof(request_msg_t), i);
    }
    else
    {
      sctk_net_ibv_broadcast_barrier(&request_msg, sizeof(request_msg_t), i);

      remote_entry[i].entry = request_msg.entry;
      remote_entry[i].rkey = request_msg.rkey;
    }
  }

  sctk_nodebug("End init barrier");
}

static uint64_t counter = 0;
void
sctk_net_ibv_n_way_dissemination_barrier_rc_sr( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp )
{
    int mask;
    int i;
    int *local_ranks;
    sctk_net_ibv_ibuf_t* ibuf;
    int local_rank = -1;
    int index = 0;
    int com_size;
    uint32_t psn;
    sctk_net_ibv_collective_pending_t* msg;
    int dst, src;

    psn = sctk_net_ibv_sched_coll_psn_inc(IBV_BARRIER, com->id);
    sctk_nodebug("BEGIN barrier %d (com %d)", psn, com->id);
    /* We convert all global ranks to local ranks
     * for the current comminucator. If an entry is != in the
     * array 'involved_nb', the process corresponding to the entry
     * is a part of the allreduce */
    com_size = com->involved_nb;
    local_ranks = sctk_malloc(com_size * sizeof(int));
    for (i=0; i < sctk_process_number; ++i)
    {
      if (com->involved_list[i])
      {
        /* We save the local rank of the current process */
        if (sctk_process_rank == i)
        {
          local_rank = index;
        }
        local_ranks[index] = i;
        index++;
      }
    }
    assume(local_rank!=-1);

    sctk_nodebug("Barrier");

    mask = 0x1;
    while (mask < com_size) {
      dst = (local_rank + mask) % com_size;
      src = (local_rank - mask + com_size) % com_size;

      /* send */
      sctk_net_ibv_allocator_send_coll_message(
          rail, rc_sr_local, com,
          NULL,
          local_ranks[dst], 0, IBV_BCAST_INIT_BARRIER, psn);

      /* receive */
      sctk_nodebug("Wait from comm %d", com->id);
      COLLECTIVE_LOOKUP(&com_entries[com->id].init_barrier_fifo, local_ranks[src], psn, msg);

      /* read the content of the message */
      if (msg->type == RC_SR_EAGER)
      {
        ibuf = (sctk_net_ibv_ibuf_t*) msg->payload;
        /* restore buffers */
        sctk_net_ibv_ibuf_release(ibuf, 1);
        sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
      }

      mask <<= 1;
    }
    sctk_nodebug("END barrier %d (com:%d)", psn, com->id);

}



  void
sctk_net_ibv_n_way_dissemination_barrier_rc_rdma ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp )
{
  int round = 0;
  uint32_t sendpeer;
  uint32_t recvpeer;
  int i;
  int *local_ranks;
  int local_rank = -1;
  int index = 0;
  int source;
  int com_size;
  uint32_t psn;

  /* We convert all global ranks to local ranks
   * for the current comminucator. If an entry is != in the
   * array 'involved_nb', the process corresponding to the entry
   * is a part of the allreduce */
  com_size = com->involved_nb;
  local_ranks = sctk_malloc(com_size * sizeof(int));
  for (i=0; i < sctk_process_number; ++i)
  {
    if (com->involved_list[i])
    {
      /* We save the local rank of the current process */
      if (sctk_process_rank == i)
      {
        local_rank = index;
      }
      local_ranks[index] = i;
      index++;
    }
  }
  assume(local_rank!=-1);



  if (is_barrier_initialized == 0)
  {
    sctk_net_ibv_barrier_init ();
    is_barrier_initialized = 1;
  }

  counter += 1;
  sctk_nodebug("Begin barrier with nb_involved %d (com_id:%d, psn:%d)", com_size, com->id, counter);
  if (counter == 0)
  {
    sctk_warning("COUNTER reset to 0. Bug not fixed for now");
    assume(0);
  }

  int limit = ceil(log(com_size) / log(N_WAY_DISSEMINATION+1));
  for( round = 0; round < limit ; ++round)
  {
    if (local_rank == 0)
      sctk_nodebug("Round: %d, ceil(log(com->involved_nb) / log(2): %f", round, ceil(log(com_size) / log(N_WAY_DISSEMINATION+1)));

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (local_rank) + i*pow((N_WAY_DISSEMINATION+1), round);
      sendpeer = fmod(tmp, com_size);

      if (sendpeer != local_rank)
      {
        local_entry.entry[local_ranks[local_rank]] = counter;

        sctk_nodebug("%d - Send to process %u", i, sendpeer);
        sctk_net_ibv_barrier_send(&local_entry, remote_entry, local_ranks[local_rank], local_ranks[sendpeer]);

      }
    }

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (com_size) + ((local_rank) - i*pow((N_WAY_DISSEMINATION+1), round));
      recvpeer = fmod(tmp, com_size);

      sctk_nodebug("%d - Recv from process %u", i, recvpeer);

      if (recvpeer != local_rank)
      {
        while(local_entry.entry[local_ranks[recvpeer]] < counter)
        {
          sctk_net_ibv_allocator_ptp_poll_all();
          //          sctk_thread_yield();
        }
        sctk_nodebug("Got counter : %d",local_entry.entry[recvpeer]);
      }
    }
  }
  sctk_nodebug("End of barrier %d", counter);

}


  void
sctk_net_ibv_barrier ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp )
{
  if (com->id == 0)
  {
    sctk_net_ibv_n_way_dissemination_barrier_rc_rdma(com, my_vp);
  } else {
    sctk_net_ibv_n_way_dissemination_barrier_rc_sr(com, my_vp);
  }
}

#endif
