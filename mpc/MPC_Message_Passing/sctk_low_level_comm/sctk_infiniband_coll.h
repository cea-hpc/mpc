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

/* list of broadcast entries */
struct sctk_list broadcast_fifo;
struct sctk_list           reduce_fifo;
static int *sctk_comm_world_array;

/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  static inline void
  sctk_net_ibv_collective_init()
{
  int i;

  sctk_list_new(&broadcast_fifo, 1);
  sctk_list_new(&reduce_fifo, 1);

  sctk_comm_world_array = sctk_malloc(sctk_process_number * sizeof(unsigned int));

  for (i = 0; i < sctk_process_number; ++i)
  {
    sctk_comm_world_array[i] = i;
  }
}

   static void*
  sctk_net_ibv_collective_push(struct sctk_list* list, sctk_net_ibv_rc_sr_msg_header_t* msg)
{
  void* ptr;
  ptr = sctk_malloc(msg->size);
  memcpy(ptr, msg, msg->size);

  sctk_list_lock(list);
  sctk_list_push(list, ptr);
  sctk_list_unlock(list);

  return ptr;
}

  static inline sctk_net_ibv_rc_sr_msg_header_t*
sctk_net_ibv_collective_lookup_src(struct sctk_list* list, const int src)
{
  struct sctk_list_elem* elem;
  sctk_net_ibv_rc_sr_msg_header_t* msg;

  if (!list || src < 0)
    return NULL;

  elem = list->head;
  while(elem)
  {
    msg = (sctk_net_ibv_rc_sr_msg_header_t*) elem->elem;
    if (msg->src_process == src)
    {
      sctk_list_lock(list);
      sctk_list_remove(list, elem);
      sctk_list_unlock(list);
      return msg;
    }
    elem = elem->p_next;
  }
  return NULL;
}

  static inline void
sctk_net_ibv_broadcast_recv(sctk_virtual_processor_t * my_vp, size_t size, int* array,
    const int relative_rank, int* mask)
{
  int src;
  sctk_net_ibv_rc_sr_msg_header_t* msg;

  sctk_nodebug("Broadcast recv");

  while(*mask < sctk_process_number)
  {
    if (relative_rank & *mask)
    {
      src = sctk_process_rank - *mask;
      if (src < 0)
        src += sctk_process_number;

      sctk_nodebug("Waiting broadcast from process %d", src);
      /* received from process src */
      do {
        msg = sctk_net_ibv_collective_lookup_src(&broadcast_fifo, src);
        sctk_thread_yield();
      } while(msg == NULL);

      sctk_nodebug("Broadcast received from process %d size %lu", src, msg->size);
      memcpy(my_vp->data.data_out,
        &msg->payload, size);
      sctk_free(msg);
      break;
    }
    *mask <<=1;
  }
}

  static inline void
sctk_net_ibv_broadcast_send(sctk_collective_communications_t * com,
    void* data, size_t size, int* array, unsigned int relative_rank, int* mask)
{
  int i;
  int dest;

  sctk_nodebug("Broadcast send, relative_rank %d, mask %d", relative_rank, *mask);

  *mask >>= 1;
  sctk_nodebug("mask %d", *mask);
  while(*mask > 0)
  {
    if(relative_rank + *mask < sctk_process_number)
    {
      dest = sctk_process_rank + *mask;
      if (dest >= sctk_process_number) dest -= sctk_process_number;

      sctk_nodebug("Send broadcast msg to process %d", dest);
      sctk_net_ibv_comp_rc_sr_send_ptp_message (
          rc_sr_local,
          rc_sr_coll_send_buff, data,
          dest, size, RC_SR_BCAST);
    }
  *mask >>= 1;
  }
}

static void
sctk_net_ibv_broadcast ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem, const int root) {
  int size;
  int relative_rank;
  int mask;

  size = elem_size * nb_elem;
  /* FIXME: Size greather than SCTK_EAGER_THRESHOLD */
  assume (size <= SCTK_EAGER_THRESHOLD);
  /* FIXME: Other communicators */
  assume(com->id == 0);

  sctk_nodebug("Broadcast from root %d", root);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(my_vp, size, sctk_comm_world_array, relative_rank, &mask);

  if ( root == sctk_process_rank )
  {
    if(my_vp->data.data_out)
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );

    sctk_net_ibv_broadcast_send(com, my_vp->data.data_in, size, sctk_comm_world_array, relative_rank, &mask);

  } else {
    sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask);
  }

  sctk_nodebug("Leave broadcast");
}

static void*
walk_list(void* elem, int cond)
{
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  msg = (sctk_net_ibv_rc_sr_msg_header_t* ) elem;

  if (msg->src_process == cond)
  {
    sctk_nodebug("Found for process %d", cond);
    return msg;
  }

  return NULL;
}

static inline void
sctk_net_ibv_allreduce ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type ) {

  int size;
  int i;
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  int mask;
  int relative_rank;
  /* root process is process 0 */
  int root = 0;
  int child;
  int parent;

  sctk_nodebug("reduce operation");

  size = elem_size * nb_elem;
  /* FIXME: Size greather than SCTK_EAGER_THRESHOLD */
  assume (size <= SCTK_EAGER_THRESHOLD);
  /* FIXME: Other communicators */
  assume(com->id == 0);

  /* receive from children */
  for (i = 1; i <= 2; ++i)
  {
    child = 2*sctk_process_rank + i;
    if (child < sctk_process_number)
    {
      sctk_nodebug("Receive from process %d", child);

      while(! (msg = sctk_list_walk_on_cond(&reduce_fifo, child, walk_list, 1)))
      {
//        sctk_net_ibv_allocator_ptp_poll_all();
        sctk_thread_yield();
      }
      sctk_nodebug("Done msg from %d", child);

      /* an entry is ready to be read : we pop the element from list */
//      msg = sctk_list_pop(&reduce_fifo);
      sctk_nodebug("Msg from %d", msg->src_process);

      /* function execution */
      func ( &msg->payload, my_vp->data.data_out, nb_elem, data_type );
    }
  }

  /* send to parent */
  parent = (sctk_process_rank - 1) / 2;
  if (parent >= 0)
  {
    if (parent != sctk_process_rank)
    {
      sctk_nodebug("Send to parent %d", parent);
      sctk_net_ibv_comp_rc_sr_send_ptp_message (
          rc_sr_local,
          rc_sr_coll_send_buff, my_vp->data.data_out,
          parent, size, RC_SR_REDUCE);
    }
  }

  sctk_nodebug("Finished for process");

  /* broadcast */
  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(my_vp, size, sctk_comm_world_array, relative_rank, &mask);

  sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask);

  sctk_nodebug ( "Broadcast received : %d", ((int*) my_vp->data.data_out)[0] );

}

#endif
