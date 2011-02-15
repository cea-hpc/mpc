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


#include "sctk_list.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_infiniband_coll.h"

static int *sctk_comm_world_array;

/* rail */
extern  sctk_net_ibv_qp_rail_t   *rail;
/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;
/* collectives send buffers */
extern sctk_net_ibv_rc_sr_buff_t*     rc_sr_coll_send_buff;

/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  inline void
  sctk_net_ibv_collective_init()
{
  int i;

  sctk_list_new(&broadcast_fifo, 1);
  sctk_list_new(&reduce_fifo, 1);
  sctk_list_new(&init_barrier_fifo, 1);

  sctk_comm_world_array = sctk_malloc(sctk_process_number * sizeof(unsigned int));

  for (i = 0; i < sctk_process_number; ++i)
  {
    sctk_comm_world_array[i] = i;
  }
}

   void*
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

  sctk_net_ibv_rc_sr_msg_header_t*
sctk_net_ibv_collective_lookup_src(struct sctk_list* list, const int src)
{
  struct sctk_list_elem* elem;
  sctk_net_ibv_rc_sr_msg_header_t* msg;

  if (!list || src < 0)
    return NULL;

  sctk_list_lock(list);
  elem = list->head;
  while(elem)
  {
    msg = (sctk_net_ibv_rc_sr_msg_header_t*) elem->elem;
    if (msg->src_process == src)
    {
      sctk_list_remove(list, elem);
      sctk_list_unlock(list);
      return msg;
    }
    elem = elem->p_next;
  }
  sctk_list_unlock(list);
  return NULL;
}

  static inline void
sctk_net_ibv_broadcast_recv(void* data, size_t size, int* array,
    const int relative_rank, int* mask, struct sctk_list* list)
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
        msg = sctk_net_ibv_collective_lookup_src(list, src);
        sctk_thread_yield();
      } while(msg == NULL);

      sctk_nodebug("Broadcast received from process %d size %lu", src, msg->size);
      memcpy(data,
        &msg->payload, size);
      sctk_free(msg);
      break;
    }
    *mask <<=1;
  }
}

  static inline void
sctk_net_ibv_broadcast_send(sctk_collective_communications_t * com,
    void* data, size_t size, int* array, unsigned int relative_rank, int* mask, sctk_net_ibv_rc_sr_msg_type_t type)
{
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
          dest, size, type);
    }
  *mask >>= 1;
  }
}

void
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

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, &broadcast_fifo);

  if ( root == sctk_process_rank )
  {
    if(my_vp->data.data_out)
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );

    sctk_net_ibv_broadcast_send(com, my_vp->data.data_in, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);

  } else {
    sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);
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

void
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

  sctk_net_ibv_broadcast_recv(my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, &broadcast_fifo);

  sctk_net_ibv_broadcast_send(com, my_vp->data.data_out, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST);

  sctk_nodebug ( "Broadcast received : %d", ((int*) my_vp->data.data_out)[0] );

}

/*-----------------------------------------------------------
 *  BARRIER
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
  uint32_t lkey;

  struct ibv_send_wr   wr;
  struct ibv_sge       list;

} remote_entry_t;

typedef struct
{
  uint32_t rkey;
  uint32_t lkey;

} barrier_local_t;


local_entry_t   local_entry;
remote_entry_t*  remote_entry;

#define N_WAY_DISSEMINATION 1

typedef struct
{
  int* entry;
  uint32_t rkey;
} request_msg_t;

/* FIXME Problem with uint8_t */
static uint64_t counter = 0;

static int is_barrier_initialized = 0;

void
sctk_net_ibv_broadcast_barrier (
    void* data, const size_t size, const int root) {

  int relative_rank;
  int mask;

  /* FIXME: Size greather than SCTK_EAGER_THRESHOLD */
  assume (size <= SCTK_EAGER_THRESHOLD);

  sctk_nodebug("Broadcast barrier init from root %d", root);

  mask = 0x1;
  /* compute relative rank for process */
  relative_rank = (sctk_process_rank >= root) ?
    sctk_process_rank - root :
    sctk_process_rank - root + sctk_process_number;

  sctk_net_ibv_broadcast_recv(data, size, sctk_comm_world_array, relative_rank, &mask, &init_barrier_fifo);

  sctk_nodebug("SEND");
  sctk_net_ibv_broadcast_send(NULL, data, size, sctk_comm_world_array, relative_rank, &mask, RC_SR_BCAST_INIT_BARRIER);

  sctk_nodebug("Leave broadcast");
}

 static void
sctk_net_ibv_barrier_post_recv(local_entry_t* local)
{

  /* allocate memory */
  posix_memalign( (void*) &local->entry,
      sctk_net_ibv_mmu_get_pagesize(), sizeof(int) * sctk_process_number);
  assume(local->entry);
  memset(local_entry.entry, 0, sizeof(int) * sctk_process_number);

  /* register memory */
  local->mmu_entry =
    sctk_net_ibv_mmu_register(rail->mmu, rc_sr_local,
        local->entry, sizeof(int) * sctk_process_number);

}

  static void
sctk_net_ibv_barrier_post_send(local_entry_t* local, remote_entry_t* remote, int i)
{

  remote[i].list.addr = (uintptr_t) &local->entry[i];
  remote[i].list.length = sizeof(int);
  remote[i].list.lkey = local->mmu_entry->mr->lkey;

  /* wr.rdma isn't filled there but when the msg is sent */

  remote[i].wr.wr_id = 2000;
  remote[i].wr.sg_list = &(remote[i].list);
  remote[i].wr.num_sge = 1;
  remote[i].wr.opcode = IBV_WR_RDMA_WRITE;
  remote[i].wr.send_flags = IBV_SEND_SIGNALED;
  remote[i].wr.next = NULL;
}

  static inline void
sctk_net_ibv_barrier_send(local_entry_t* local, remote_entry_t* remote,
    int entry_nb, int dest_process)
{
  sctk_net_ibv_qp_remote_t* remote_rc_sr;
  struct  ibv_send_wr* bad_wr = NULL;

  /* fill RMDA infos */
  remote[dest_process].list.addr = (uintptr_t) &local->entry[entry_nb];
  remote[dest_process].wr.wr.rdma.remote_addr = (uintptr_t) ((int*)remote[dest_process].entry + entry_nb);
  remote[dest_process].wr.wr.rdma.rkey      = remote[dest_process].rkey;

  /* TODO FIXME is it really necessary ? */
  remote_rc_sr = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  sctk_nodebug("Entry id %d for process %d", remote[dest_process].wr.wr_id, dest_process);
  ibv_post_send(remote_rc_sr->qp, &(remote[dest_process].wr), &bad_wr);
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

      sctk_net_ibv_barrier_post_send(&local_entry, remote_entry, i);
    }
  }

  sctk_nodebug("End init barrier");
}

  void
sctk_net_ibv_barrier ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp )
{
  int round = 0;
  uint32_t sendpeer;
  uint32_t recvpeer;
  int i;

  if (is_barrier_initialized == 0)
  {
    sctk_net_ibv_barrier_init ();
    is_barrier_initialized = 1;
  }

  if (sctk_process_rank == 0)
  {
    int i;

    for (i=0; i < sctk_process_number; ++i)
    {
      if (sctk_process_rank != i)
        sctk_nodebug("Address for process %d: %p, rkey: %lu",i, remote_entry[i].entry, remote_entry[i].rkey);
    }
  }

  counter += 1;
  if (counter == 0)
  {
    sctk_warning("COUNTER reset to 0. Bug not fixed for now");
    assume(0);
  }

  int limit = ceil(log(sctk_process_number) / log(N_WAY_DISSEMINATION+1));
  for( round = 0; round < limit ; ++round)
  {
    if (sctk_process_rank == 0)
        sctk_nodebug("Round: %d, ceil(log(sctk_process_number) / log(2): %f", round, ceil(log(sctk_process_number) / log(N_WAY_DISSEMINATION+1)));

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (sctk_process_rank) + i*pow((N_WAY_DISSEMINATION+1), round);
      sendpeer = fmod(tmp, sctk_process_number);

      if (sendpeer != sctk_process_rank)
      {
        local_entry.entry[sctk_process_rank] = counter;
        sctk_net_ibv_barrier_send(&local_entry, remote_entry, sctk_process_rank, sendpeer);

        if (sctk_process_rank == 0)
        sctk_nodebug("%d - Send to process %u", i, sendpeer);
      }
    }

    for (i=1; i <= N_WAY_DISSEMINATION; ++i)
    {
      uint32_t tmp = (sctk_process_number) + ((sctk_process_rank) - i*pow((N_WAY_DISSEMINATION+1), round));
      recvpeer = fmod(tmp, sctk_process_number);

      if (sctk_process_rank == 0)
        sctk_nodebug("%d - Recv from process %u", i, recvpeer);

      if (recvpeer != sctk_process_rank)
      {
        while(local_entry.entry[recvpeer] < counter)
        {
          sctk_thread_yield();
        }
        sctk_nodebug("Got counter : %d",local_entry.entry[recvpeer]);
      }
    }
  }
}
