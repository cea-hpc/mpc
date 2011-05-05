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

#include "sctk.h"
#include "sctk_list.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_config.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_profiler.h"
#include "sctk_ib_mmu.h"
#include "sctk_spinlock.h"

/* lock on ibuf interface */
static sctk_spinlock_t                    ibuf_lock = SCTK_SPINLOCK_INITIALIZER;
static struct sctk_net_ibv_ibuf_region_s  *ibuf_begin_region = NULL;
static struct sctk_net_ibv_ibuf_region_s  *ibuf_last_region = NULL;

/* list of free buffers. Once a buffer is freed, it is added to this list */
static struct sctk_net_ibv_ibuf_s         *ibuf_free_header = NULL;

uint32_t                     ibuf_free_ibuf_nb = 0;
uint32_t                     ibuf_got_ibuf_nb = 0;
uint32_t                     ibuf_free_srq_nb = 0;
uint32_t                     ibuf_got_srq_nb = 0;
uint32_t                     ibuf_thread_activation_nb = 0;

/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;

void sctk_net_ibv_ibuf_new()
{
  /* TODO: move out lock */
//  ibuf_lock = SCTK_SPINLOCK_INITIALIZER;
//  ibuf_free_ibuf_nb = 0;
//  ibuf_got_ibuf_nb = 0;
//  ibuf_free_srq_nb = 0;
//  ibuf_got_srq_nb = 0;

//  ibv_got_recv_wqe = 0;
//  ibv_free_recv_wqe = 0;
//  ibv_got_send_wqe = 0;
//  ibv_free_send_wqe = 0;
}

void sctk_net_ibv_ibuf_init( sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local, int nb_ibufs, uint8_t debug)
{
  sctk_nodebug("BEGIN ibuf initialization");

  sctk_net_ibv_ibuf_region_t *region = NULL;
  void* ptr;
  void* ibuf;
  sctk_net_ibv_ibuf_t* ibuf_ptr;
  size_t page_size = sctk_net_ibv_mmu_get_pagesize();
  int i;

  region = sctk_malloc(sizeof(sctk_net_ibv_ibuf_region_t));
  assume(region);

  sctk_posix_memalign( (void**) &ptr, page_size,
      nb_ibufs * ibv_eager_threshold);
  assume(ptr);

  sctk_nodebug("size: %lu", nb_ibufs * sizeof(sctk_net_ibv_ibuf_region_t));

  sctk_posix_memalign(&ibuf, page_size, nb_ibufs * sizeof(sctk_net_ibv_ibuf_t));
  assume(ibuf);

  memset (ibuf, 0, nb_ibufs * sizeof(sctk_net_ibv_ibuf_t));
  memset (ptr, 0, nb_ibufs * ibv_eager_threshold);

  region->ibuf = ibuf;
  region->next_region = NULL;
  region->nb = nb_ibufs;

  if (ibuf_begin_region == NULL)
    ibuf_begin_region = region;

  if (!ibuf_last_region)
  {
    /* first allocation */
    ibuf_last_region = region;
    sctk_ibv_profiler_add(IBV_IBUF_TOT_SIZE, nb_ibufs * (ibv_eager_threshold+sizeof(sctk_net_ibv_ibuf_t)));
  } else {
    /* not the first allocation */
    ibuf_last_region = ibuf_last_region->next_region;
    sctk_ibv_profiler_inc(IBV_IBUF_CHUNKS);
    sctk_ibv_profiler_add(IBV_IBUF_TOT_SIZE, nb_ibufs * (ibv_eager_threshold+sizeof(sctk_net_ibv_ibuf_t)));
  }

  /* TODO: register memory for each region */
  sctk_nodebug("Local : %p", local);
  region->mmu_entry = sctk_net_ibv_mmu_register(rail, local, ptr,
      nb_ibufs * ibv_eager_threshold);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);
  assume (region->mmu_entry);

  /* TODO: sourround by locks for multiple function use */
  ibuf_free_ibuf_nb += nb_ibufs;

  for(i=0; i < nb_ibufs - 1; ++i)
  {
    ibuf_ptr = (sctk_net_ibv_ibuf_t*) ibuf + i;

    ibuf_ptr->region = region;
    ibuf_ptr->desc.next = ((sctk_net_ibv_ibuf_t*) ibuf + i + 1);

    ibuf_ptr->size = 0;
    ibuf_ptr->buffer = (unsigned char*) ((char*) ptr + (i*ibv_eager_threshold));
    assume(ibuf_ptr->buffer);
    ibuf_ptr->flag = FREE_FLAG;
  }

  /* init last ibuf */
  ibuf_ptr = (sctk_net_ibv_ibuf_t*) ibuf + nb_ibufs - 1;
  ibuf_ptr->region = region;
  ibuf_ptr->desc.next = ibuf_free_header;
  ibuf_ptr->size = 0;
  ibuf_ptr->buffer = (unsigned char*) ((char*) ptr + (nb_ibufs - 1) * ibv_eager_threshold);
  assume(ibuf_ptr->buffer);
  ibuf_ptr->flag = FREE_FLAG;

  ibuf_free_header = (sctk_net_ibv_ibuf_t*) ibuf;

  sctk_nodebug("END ibuf initialization");

  if (debug)
  {
    PRINT_DEBUG(1, "[ibuf] Allocation of %d more buffers (ibuf_free_ibuf_nb %d - ibuf_got_ibuf_nb %d)", ibv_size_ibufs_chunk, ibuf_free_ibuf_nb, ibuf_got_ibuf_nb, ibuf_free_header);
  }
}

sctk_net_ibv_ibuf_t* sctk_net_ibv_ibuf_pick(int return_on_null, int need_lock)
{
  sctk_net_ibv_ibuf_t* ibuf;

  while (1)
  {

    if (need_lock)
      sctk_spinlock_lock(&ibuf_lock);

    if (ibuf_free_header != NULL)
    {
      goto resume;
    } else {
      sctk_net_ibv_ibuf_init(rail, rc_sr_local, ibv_size_ibufs_chunk, 1);
    }

    if (need_lock)
      sctk_spinlock_unlock(&ibuf_lock);

  }

resume:
  if(sctk_process_rank == 0)
    sctk_nodebug("Next free_header: %p, nb %d (srq %d)", ibuf_free_header, ibuf_free_ibuf_nb, ibuf_free_srq_nb);

  ibuf = ibuf_free_header;
  ibuf_free_header = ibuf_free_header->desc.next;

  assume(ibuf != NULL);
  assume(ibuf->flag == FREE_FLAG);

  ibuf->flag = BUSY_FLAG;
  --ibuf_free_ibuf_nb;
  ++ibuf_got_ibuf_nb;

  if (need_lock)
    sctk_spinlock_unlock(&ibuf_lock);

  return ibuf;
}

int sctk_net_ibv_ibuf_srq_check_and_post(
    sctk_net_ibv_qp_local_t* local, int limit, int need_lock)
{
  int size;
  int nb_posted;

  sctk_nodebug("ibv_max_srq_ibufs %d - ibuf_free_srq_nb %d",
      ibv_max_srq_ibufs, ibuf_free_srq_nb);

  if ( (ibuf_free_srq_nb <= limit) || (limit == -1))
  {
    if (need_lock)
      sctk_spinlock_lock(&ibuf_lock);


    if (limit == -1)
    {
      ++ibuf_thread_activation_nb;
      /* register more srq buffers */
      if (ibuf_thread_activation_nb >= 5)
      {
        /* FIXME: poll cq AFTER posting buffers */
//        sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, ibv_max_srq_ibufs, sctk_net_ibv_rc_sr_recv_cq, IBV_CHAN_RECV);

        ibv_max_srq_ibufs += 100;
        ibuf_thread_activation_nb = 0;
        PRINT_DEBUG(1, "[srq] SRQ entries expanded up to: %d", ibv_max_srq_ibufs);
      }
    }

    //    if (ibuf_free_ibuf_nb > ibv_max_srq_ibufs)
    //      size = ibv_max_srq_ibufs - ibuf_free_srq_nb;
    //    else
    size = ibv_max_srq_ibufs - ibuf_free_srq_nb;

    if (need_lock)
      sctk_spinlock_unlock(&ibuf_lock);

    nb_posted =  sctk_net_ibv_ibuf_srq_post
      (local, size);

    if (sctk_process_rank == 0)
      sctk_nodebug("srq_post: post %d buffers", nb_posted);
    return nb_posted;
  }

  return 0;
}

int sctk_net_ibv_ibuf_srq_post(
    sctk_net_ibv_qp_local_t* local,
    int nb_ibufs)
{
  int i;
  sctk_net_ibv_ibuf_t* ibuf;
  int rc;

  sctk_spinlock_lock(&ibuf_lock);

  for (i=0; i < nb_ibufs; ++i)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 0);

    if (!ibuf)
      break;

    sctk_net_ibv_ibuf_recv_init(ibuf);

    rc = ibv_post_srq_recv(local->srq, &(ibuf->desc.wr.recv), &(ibuf->desc.bad_wr.recv));
    if (rc != 0)
    {
      /* try to post more recv buffers */
      rc = sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit, 0);

      /* if it's still impossible, we fail */
      if (rc != 0)
      {
        sctk_error("Cannot post more srq buffers.\n Please increase the value of MPC_IBV_EAGER_THRESHOLD");
        assume(0);
      }
    }
  }

  ibuf_free_srq_nb+=i;
  sctk_spinlock_unlock(&ibuf_lock);
  return i;
}


void sctk_net_ibv_ibuf_release(sctk_net_ibv_ibuf_t* ibuf, int is_srq)
{
  sctk_spinlock_lock(&ibuf_lock);

  ibuf->flag = FREE_FLAG;
  ++ibuf_free_ibuf_nb;
  --ibuf_got_ibuf_nb;

  if (is_srq)
    --ibuf_free_srq_nb;

  ibuf->desc.next = ibuf_free_header;
  ibuf_free_header = ibuf;

  sctk_spinlock_unlock(&ibuf_lock);
}

/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_net_ibv_ibuf_recv_init(
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  //  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  //  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = ibv_eager_threshold;

  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = NORMAL_IBUF_FLAG;
}

void sctk_net_ibv_ibuf_rdma_recv_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = ibv_eager_threshold;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = NORMAL_IBUF_FLAG;
}

void sctk_net_ibv_ibuf_barrier_send_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = BARRIER_IBUF_FLAG;
}


void sctk_net_ibv_ibuf_send_init(
    sctk_net_ibv_ibuf_t* ibuf, size_t size)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_SEND;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = size;
  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = NORMAL_IBUF_FLAG;
}


void sctk_net_ibv_ibuf_rdma_write_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = RDMA_WRITE_IBUF_FLAG;
}

void sctk_net_ibv_ibuf_rdma_read_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, void* supp_ptr, int dest_process)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_READ;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = RDMA_READ_IBUF_FLAG;
  ibuf->supp_ptr = supp_ptr;
  ibuf->dest_process = dest_process;
}
#endif
