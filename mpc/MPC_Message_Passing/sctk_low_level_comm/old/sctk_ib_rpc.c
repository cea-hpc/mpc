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

#include "sctk_low_level_comm.h"
#include "sctk_ib_rpc.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_allocator.h"

/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;
/* rail */
extern  sctk_net_ibv_qp_rail_t   *rail;

typedef struct
{
  void* arg;
  size_t arg_size;
  int src_process;
  struct sctk_list_header list_header;
  void ( *func ) ( void * );
} rpc_req_list_entry_t;

typedef struct
{
  void* addr;
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  struct sctk_list_header list_header;
} rpc_reg_mr_list_entry_t;

struct sctk_list_header rpc_req_list;
struct sctk_list_header rpc_ack_list;
struct sctk_list_header rpc_reg_mr_list;

/*-----------------------------------------------------------
 *  RPC
 *----------------------------------------------------------*/

  void*
thread_rpc(void* arg)
{
  void* ret;
  rpc_req_list_entry_t* req;
  sctk_update_communicator_t *msg;

  while(1)
  {
    if (sctk_ib_list_trylock(&rpc_req_list) == 0)
    {
      do {
        ret = sctk_ib_list_pop(&rpc_req_list);
        sctk_ib_list_unlock(&rpc_req_list);

        if (ret)
        {
          req = sctk_ib_list_get_entry(ret, rpc_req_list_entry_t, list_header);
          sctk_nodebug("POP %p", req);
          sctk_nodebug("arg_size: %lu", req->arg_size);
          sctk_nodebug("Element poped (size:%lu)", req->arg_size);
          sctk_rpc_execute(req->func, req->arg);
          sctk_free(req);
        }
      } while (ret);
    }
    /* TODO: use pthread_cond_signal instead of usleep */
    usleep(200);
  }
  return NULL;
}

  void
sctk_net_rpc_init()
{
  sctk_thread_t pidt_rpc;
  sctk_thread_attr_t attr_rpc;

  SCTK_LIST_HEAD_INIT(&rpc_req_list);
  SCTK_LIST_HEAD_INIT(&rpc_ack_list);
  SCTK_LIST_HEAD_INIT(&rpc_reg_mr_list);

  /* thread for RPC */
  sctk_thread_attr_init ( &attr_rpc );
  /* / ! \ There are some troubles when we are using
   * a system thread. Don't decomment the following line. */
  //  sctk_thread_attr_setscope ( &attr_rpc, SCTK_THREAD_SCOPE_SYSTEM );
  sctk_user_thread_create ( &pidt_rpc, &attr_rpc, thread_rpc, NULL );
}


/**
 *  Register a memory region
 *  \param
 *  \return
 */
  void
sctk_net_rpc_register(void* addr, size_t size, int process, int is_retrieve, uint32_t* rkey)
{
  sctk_nodebug("BEGIN rpc_register %d %d (addr:%p,rkey:%p)", process, is_retrieve, addr, rkey);
  size_t aligned_size;
  void*   aligned_ptr;
  size_t page_size;
  sctk_net_ibv_mmu_entry_t *mmu_entry;
  rpc_reg_mr_list_entry_t* reg_mr_entry;

  if (!addr || !is_retrieve) return;

  page_size = sctk_net_ibv_mmu_get_pagesize();

  aligned_size = size + (((unsigned long) addr) % page_size);
  aligned_ptr = addr - (((unsigned long) addr) % page_size);

  mmu_entry = sctk_net_ibv_mmu_register(rail, rc_sr_local,
      aligned_ptr, aligned_size);
  reg_mr_entry = sctk_malloc(sizeof(rpc_reg_mr_list_entry_t));
  assume(reg_mr_entry);
  reg_mr_entry->mmu_entry = mmu_entry;
  reg_mr_entry->addr = addr;
  sctk_ib_list_lock(&rpc_reg_mr_list);
  sctk_ib_list_push_tail(&rpc_reg_mr_list, &reg_mr_entry->list_header);
  sctk_ib_list_unlock(&rpc_reg_mr_list);

  if (rkey)
  {
    *rkey = mmu_entry->mr->rkey;
    sctk_nodebug("SEND keys %lu to process %d (addr:%p, size:%lu)", *rkey, process, addr, size);
  }

  sctk_nodebug("END rpc_register");
}


/**
 *  Unregister a memory region
 *  \param
 *  \return
 */
  void
sctk_net_rpc_unregister(void* addr, size_t size, int process, int is_retrieve)
{
  rpc_reg_mr_list_entry_t* reg_mr_entry = NULL;
  uint8_t found = 0;
  struct sctk_list_header* tmp;

  sctk_nodebug("BEGIN rpc_unregister %d", process, is_retrieve);
  if (!addr || !is_retrieve) return;

  sctk_ib_list_lock(&rpc_reg_mr_list);
  sctk_ib_list_foreach(tmp,&rpc_reg_mr_list)
  {
    reg_mr_entry = sctk_ib_list_get_entry(tmp, rpc_reg_mr_list_entry_t, list_header);

    if( reg_mr_entry->addr == addr)
    {
      sctk_nodebug("Found addr %p <-> %p", reg_mr_entry->addr, addr);
      sctk_ib_list_remove(tmp);
      found = 1;
      break;
    }
  }
  sctk_ib_list_unlock(&rpc_reg_mr_list);
  assume(found);

  /* unregister memory and remove list entry */
  sctk_net_ibv_mmu_unregister(rail->mmu, reg_mr_entry->mmu_entry);
  sctk_free(reg_mr_entry);

  sctk_nodebug("END rpc_unregister");
}


/**
 *  Send a RPC message (arg) to a dest process.
 *  \param
 *  \return
 */
void
sctk_net_rpc_driver ( void ( *func ) ( void * ), int destination, void *arg, size_t arg_size ) {
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_rpc_t* rpc;
  sctk_net_ibv_allocator_request_t req;

  sctk_nodebug("BEGIN rpc_driver (size: %lu, dest:%lu)", arg_size, destination);
  /* TODO: check if the size doesn't exceed the size of an eager msg */

  req.size = sizeof(sctk_net_ibv_allocator_request_t) + arg_size;
  req.dest_process = destination;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);

  /* fill the RPC request payload */
  rpc = (sctk_net_ibv_rpc_t*)
    RC_SR_PAYLOAD(ibuf->buffer);
  memcpy(&rpc->arg, arg, arg_size);
  rpc->func = func;
  rpc->arg_size = arg_size;

  sctk_net_ibv_ibuf_send_init(ibuf, req.size + RC_SR_HEADER_SIZE);

  req.buff_nb     = 1;
  req.total_buffs = 1;
  req.payload_size = req.size;
  sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_RPC);

  sctk_nodebug("END rpc_driver %d", destination);
}

void
sctk_net_rpc_retrive_driver ( void *dest, void *src, size_t arg_size,
    int process, int *ack, uint32_t rkey ) {

  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_allocator_request_t req;
  sctk_net_ibv_mmu_entry_t *mmu_entry;
  sctk_net_ibv_rpc_ack_t* rpc_ack;
  sctk_net_ibv_rpc_ack_t* send_rpc;
  size_t aligned_size;
  void*   aligned_ptr;
  size_t page_size;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  if(src)
  {
    sctk_nodebug("BEGIN rpc_retrive_driver");
    /* we wait the RPC request from the dest process */

    page_size = sctk_net_ibv_mmu_get_pagesize();

    aligned_size = arg_size + (((unsigned long) dest) % page_size);
    aligned_ptr = dest - (((unsigned long) dest) % page_size);

    sctk_nodebug("read from %p (%p) rkey:%lu size:%lu (ack:%p)", dest, src, rkey, arg_size, ack);

    mmu_entry = sctk_net_ibv_mmu_register(rail, rc_sr_local,
        aligned_ptr, aligned_size);

    sctk_net_ibv_ibuf_rdma_read_init(
        ibuf, dest, mmu_entry->mr->lkey,
        src, rkey, arg_size, NULL, process);

    sctk_nodebug("MSG : %d", src);

    rpc_ack = sctk_malloc(sizeof(sctk_net_ibv_rpc_ack_t));
    assume(rpc_ack);
    rpc_ack->src_process = process;
    rpc_ack->ack = ack;
    rpc_ack->lock = 0;

    sctk_ib_list_lock(&rpc_ack_list);
    sctk_ib_list_push_tail(&rpc_ack_list, &rpc_ack->list_header);
    sctk_ib_list_unlock(&rpc_ack_list);

    sctk_nodebug("\t\t\tread from %p (%p) (dest:%d) with key %lu. size:%lu", src, dest, process, rkey, arg_size);

    req.dest_process = process;
    sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_RPC_READ);

    sctk_thread_wait_for_value ( ( int * ) &rpc_ack->lock, 1 );

    sctk_net_ibv_mmu_unregister(rail->mmu, mmu_entry);

    sctk_nodebug("END rpc_retrive_driver");
  } else {
    req.size = sizeof(sctk_net_ibv_rpc_ack_t);
    sctk_net_ibv_ibuf_send_init(ibuf, req.size + RC_SR_HEADER_SIZE);

    send_rpc = (sctk_net_ibv_rpc_ack_t*)
      RC_SR_PAYLOAD(ibuf->buffer);
    send_rpc->ack = ack;

    req.dest_process = process;
    req.buff_nb     = 1;
    req.total_buffs = 1;
    req.payload_size = req.size;
    sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_RPC_ACK);
  }
}

void
sctk_net_rpc_send_driver ( void *dest, void *src, size_t arg_size, int process,
    int *ack ) {
  sctk_nodebug("BEGIN rpc_send_driver");

  not_reachable ();
  assume ( dest );
  assume ( src );
  assume ( arg_size );
  assume ( process );
  assume ( ack );

  sctk_nodebug("END rpc_send_driver");
}
  void
sctk_net_rpc_receive(sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_rpc_t* rpc;
  rpc_req_list_entry_t* req;

  sctk_net_ibv_ibuf_header_t* msg_header;

  sctk_nodebug("There is an element to the list");
  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  rpc = (sctk_net_ibv_rpc_t*) RC_SR_PAYLOAD(ibuf->buffer);
  sctk_nodebug("Arg size : %lu", rpc->arg_size);

  req = sctk_malloc(sizeof(rpc_req_list_entry_t));
  assume(req);
  req->src_process = msg_header->src_process;
  req->arg = sctk_malloc(rpc->arg_size);
  assume(req->arg);
  req->func = rpc->func;
  sctk_nodebug("FUNC : %d", req->func);
  req->arg_size = rpc->arg_size;

  memcpy(req->arg, &rpc->arg, rpc->arg_size);

  /* We don't execute the RPC for now (we can't block
   * the polling thread). We push the request to a list. This list
   * is polled by a system thread */
  sctk_ib_list_lock(&rpc_req_list);
  sctk_ib_list_push_tail(&rpc_req_list, &req->list_header);
  sctk_nodebug("NEW %p, arg_size :%d", req, rpc->arg_size);
  sctk_ib_list_unlock(&rpc_req_list);
}

  void
sctk_net_rpc_send_ack(sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_ibuf_t* ibuf_ack;
  sctk_net_ibv_ibuf_header_t* msg_header;
  sctk_net_ibv_rpc_ack_t* rpc = NULL;
  sctk_net_ibv_rpc_ack_t* send_rpc;
  sctk_net_ibv_allocator_request_t req;
  struct sctk_list_header *tmp;

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);


  sctk_ib_list_foreach(tmp, &rpc_ack_list)
  {
    rpc = sctk_ib_list_get_entry(tmp, sctk_net_ibv_rpc_ack_t, list_header);
    if (rpc->src_process == ibuf->dest_process)
    {
      sctk_ib_list_remove(tmp);
      break;
    }
  }
  assume(rpc);

  /* We ack the rpc thread */
  rpc->lock = 1;
  sctk_nodebug("\t\t\tibuf from %d", ibuf->dest_process);

  ibuf_ack = sctk_net_ibv_ibuf_pick(0, 1);
  req.size = sizeof(sctk_net_ibv_rpc_ack_t);
  sctk_net_ibv_ibuf_send_init(ibuf_ack, req.size + RC_SR_HEADER_SIZE);

  send_rpc = (sctk_net_ibv_rpc_ack_t*)
    RC_SR_PAYLOAD(ibuf_ack->buffer);
  send_rpc->ack = rpc->ack;

  req.dest_process = rpc->src_process;
  req.buff_nb     = 1;
  req.total_buffs = 1;
  req.payload_size = req.size;
  sctk_net_ibv_comp_rc_sr_send(ibuf_ack, req, RC_SR_RPC_ACK);
}

#endif
