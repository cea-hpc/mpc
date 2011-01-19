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
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_rpc.h"
#include "sctk_infiniband.h"
#include "sctk_mpcrun_client.h"

#ifdef MPC_USE_INFINIBAND

#include "sctk_rpc.h"
#include <infiniband/verbs.h>
#include "sctk_alloc.h"
#include "sctk_iso_alloc.h"
#include "sctk_net_tools.h"


#define SCTK_PENDING_RDMA_NUMBER 20
#define SCTK_MAX_RAIL_NUMBER 4
#define SCTK_MULTIRAIL_THRESHOLD (4*1024)

#define IB_TX_DEPTH SCTK_PENDING_RDMA_NUMBER
#define IB_RX_DEPTH SCTK_PENDING_RDMA_NUMBER
#define IB_MAX_SG_SQ 4
#define IB_MAX_SG_RQ 4
#define IB_MAX_INLINE 128
#define IB_RMDA_DEPTH 4

static inline void sctk_connection_check(int process);

/* #define SCTK_USE_STATIC_ALLOC_ONLY  */

typedef enum
{
  ibv_entry_used, ibv_entry_perm, ibv_entry_free
} entry_status_t;

typedef struct
{
  void *ptr;
  size_t size;
  struct ibv_mr *mr;
  entry_status_t status;
  uint32_t rkey;
  uint32_t lkey;
} ibv_soft_mmu_entry_t;

static int num_devices;

#define MAX_HOST_SIZE 255
static char ibv_hostname[MAX_HOST_SIZE];

#define IBV_ADM_PORT 1

typedef struct
{
  struct ibv_context *context;
  struct ibv_pd *protection_domain;
  struct ibv_port_attr port_attr;
  struct ibv_device_attr device_attr;
  struct ibv_qp **qp;
  struct ibv_cq **of_cq;
  struct ibv_cq **if_cq;
} ibv_device_t;

static ibv_device_t *device_list;

typedef struct
{
  uint16_t rank;
  int device_nb;

  uint16_t lid[SCTK_MAX_RAIL_NUMBER];
  uint32_t *qpn;
  uint32_t psn[SCTK_MAX_RAIL_NUMBER];
  uint32_t rkey[SCTK_MAX_RAIL_NUMBER];
  uint32_t lkey[SCTK_MAX_RAIL_NUMBER];

} ibv_ident_data_t;

static void
ibv_print_ident (ibv_ident_data_t * ident)
{
  int i;
  sctk_nodebug ("Rank %d", ident->rank);
  sctk_nodebug ("device_nb %d", ident->device_nb);
  for (i = 0; i < ident->device_nb; i++)
    {
      int j;
      sctk_nodebug ("device %d rkey %d", i, ident->rkey[i]);
      sctk_nodebug ("device %d lkey %d", i, ident->lkey[i]);
      sctk_nodebug ("device %d lid %d", i, ident->lid[i]);
      sctk_nodebug ("device %d psn %d", i, ident->psn[i]);
      for (j = 0; j < sctk_process_number; j++)
	{
	  sctk_nodebug ("device %d qpn[%d] %d", i, j,
			ident->qpn[i + j * SCTK_MAX_RAIL_NUMBER]);
	}
    }
}

static ibv_ident_data_t **peer_list;

/******************************************************************/
/* SOFT MMU                                                       */
/******************************************************************/

static int ibv_soft_mmu_entry_nb = 0;
static ibv_soft_mmu_entry_t **ibv_soft_mmu = NULL;
static sctk_spinlock_t ibv_soft_mmu_lock = 0;
static volatile ibv_soft_mmu_entry_t *ibv_soft_mmu_dest = NULL;

static void
ibv_init_soft_mmu ()
{
  int i;
  int maxval = 0;
  ibv_soft_mmu =
    sctk_iso_malloc (num_devices * sizeof (ibv_soft_mmu_entry_t *));
  for (i = 0; i < num_devices; i++)
    {
      int j;
      ibv_soft_mmu[i] =
	sctk_iso_malloc (device_list[i].device_attr.max_mr *
			 sizeof (ibv_soft_mmu_entry_t));
      if (maxval < device_list[i].device_attr.max_mr)
	{
	  maxval = device_list[i].device_attr.max_mr;
	}
      for (j = 0; j < device_list[i].device_attr.max_mr; j++)
	{
	  ibv_soft_mmu[i][j].status = ibv_entry_free;
	}
    }
  ibv_soft_mmu_dest =
    sctk_iso_malloc (maxval * sizeof (ibv_soft_mmu_entry_t));
}
static unsigned long page_size;

#warning "Handle multiple dclaration of the same pointer"
static ibv_soft_mmu_entry_t *
ibv_register_soft_mmu_intern (char *orig_ptr, size_t orig_size,
			      int device_number, entry_status_t status)
{
  int i;
  char *ptr;
  size_t size;

  ptr = orig_ptr;
  size = orig_size;

  size = size + (page_size - (size % page_size));
  ptr = ptr - (((unsigned long) ptr) % page_size);

  sctk_nodebug ("START %p -> %p", orig_ptr, ptr);
  sctk_nodebug ("END   %p -> %p", orig_ptr + orig_size, ptr + size);

  assume ((unsigned long) ptr <= (unsigned long) orig_ptr);
  assume ((unsigned long) (ptr + size) >=
	  (unsigned long) (orig_ptr + orig_size));

  sctk_spinlock_lock (&ibv_soft_mmu_lock);
  for (i = 0; i < device_list[device_number].device_attr.max_mr; i++)
    {
      if (ibv_soft_mmu[device_number][i].status == ibv_entry_free)
	{
	  ibv_soft_mmu[device_number][i].mr =
	    ibv_reg_mr (device_list[device_number].protection_domain, ptr,
			size,
			IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
			IBV_ACCESS_REMOTE_READ);
	  sctk_nodebug ("Register %p-%p lkey %d rkey %d", ptr,
			(char *) ptr + size,
			ibv_soft_mmu[device_number][i].mr->lkey,
			ibv_soft_mmu[device_number][i].mr->rkey);
	  if (ibv_soft_mmu[device_number][i].mr == NULL)
	    {
	      sctk_error ("Cannot register adm MR.");
	      sctk_abort();
	    }
	  ibv_soft_mmu[device_number][i].lkey =
	    ibv_soft_mmu[device_number][i].mr->lkey;
	  ibv_soft_mmu[device_number][i].rkey =
	    ibv_soft_mmu[device_number][i].mr->rkey;
	  ibv_soft_mmu[device_number][i].ptr = ptr;
	  ibv_soft_mmu[device_number][i].size = size;
	  ibv_soft_mmu[device_number][i].status = status;

	  ibv_soft_mmu_entry_nb++;
	  sctk_spinlock_unlock (&ibv_soft_mmu_lock);
	  return &(ibv_soft_mmu[device_number][i]);
	}
    }
  sctk_spinlock_unlock (&ibv_soft_mmu_lock);
  assume (0);
  return NULL;
}

static ibv_soft_mmu_entry_t *
ibv_register_soft_mmu (void *ptr, size_t size, int device_number)
{
  return ibv_register_soft_mmu_intern (ptr, size, device_number,
				       ibv_entry_used);
}

static ibv_soft_mmu_entry_t *
ibv_register_perm_soft_mmu (void *ptr, size_t size, int device_number)
{
  return ibv_register_soft_mmu_intern (ptr, size, device_number,
				       ibv_entry_perm);
}

static int
ibv_unregister_soft_mmu (void *ptr, size_t size, int device_number)
{
  int i;
  sctk_spinlock_lock (&ibv_soft_mmu_lock);
  for (i = 0; i < device_list[device_number].device_attr.max_mr; i++)
    {
      if (ibv_soft_mmu[device_number][i].ptr == ptr &&
	  ibv_soft_mmu[device_number][i].size == size &&
	  ibv_soft_mmu[device_number][i].status == ibv_entry_used)
	{
	  ibv_dereg_mr (ibv_soft_mmu[device_number][i].mr);
	  sctk_nodebug ("UnRegister %p-%p lkey %d rkey %d", ptr,
			(char *) ptr + size,
			ibv_soft_mmu[device_number][i].mr->lkey,
			ibv_soft_mmu[device_number][i].mr->rkey);
	  ibv_soft_mmu_entry_nb--;
	  ibv_soft_mmu[device_number][i].status = ibv_entry_free;
	  ibv_soft_mmu[device_number][i].ptr = NULL;
	  ibv_soft_mmu[device_number][i].size = 0;
	  sctk_spinlock_unlock (&ibv_soft_mmu_lock);
	  return 0;
	}
    }
  sctk_spinlock_unlock (&ibv_soft_mmu_lock);
  return 1;
}

static int
ibv_is_register_soft_mmu_intern (void *ptr, size_t size, uint32_t * lkey,
				 uint32_t * rkey,
				 ibv_soft_mmu_entry_t * ibv_soft_mmu_device,
				 int nb)
{
  int i;
  unsigned long ptr_start;
  unsigned long ptr_end;


  ptr_start = (unsigned long) ptr;
  ptr_end = ptr_start + size;
  for (i = 0; i < nb; i++)
    {
      if (ibv_soft_mmu_device[i].status != ibv_entry_free)
	{
	  unsigned long ptr_mmu_start;
	  unsigned long ptr_mmu_end;

	  ptr_mmu_start = (unsigned long) ibv_soft_mmu_device[i].ptr;
	  ptr_mmu_end = ptr_mmu_start + ibv_soft_mmu_device[i].size;

	  if ((ptr_start >= ptr_mmu_start) && (ptr_end <= ptr_mmu_end))
	    {
	      if (lkey)
		*lkey = ibv_soft_mmu_device[i].lkey;
	      if (rkey)
		*rkey = ibv_soft_mmu_device[i].rkey;
	      return 1;
	    }
	}
    }
  sctk_nodebug ("UNABLE to Find key for %p-%p", (char *) ptr,
		(char *) ptr + size);
  return 0;
}

static int
ibv_is_register_soft_mmu (void *ptr, size_t size, int device_number,
			  uint32_t * lkey)
{
  int tmp;
  sctk_spinlock_lock (&ibv_soft_mmu_lock);
  tmp =
    ibv_is_register_soft_mmu_intern (ptr, size, lkey, NULL,
				     ibv_soft_mmu[device_number],
				     device_list[device_number].device_attr.
				     max_mr);
  sctk_spinlock_unlock (&ibv_soft_mmu_lock);
  sctk_nodebug ("lkey %d", *lkey);
  return tmp;
}

static void
ibv_register_soft_mmu_all (void *ptr, size_t size)
{
  int i;
  sctk_nodebug ("Register %p %lu", ptr, size);
  for (i = 0; i < num_devices; i++)
    {
      ibv_register_soft_mmu (ptr, size, i);
    }
  sctk_nodebug ("Register %p %lu done", ptr, size);
}
static void
ibv_unregister_soft_mmu_all (void *ptr, size_t size)
{
  int i;
  sctk_nodebug ("UnRegister %p %lu", ptr, size);
  for (i = 0; i < num_devices; i++)
    {
      ibv_unregister_soft_mmu (ptr, size, i);
    }
  sctk_nodebug ("UnRegister %p %lu done", ptr, size);
}

/**************************************************************************/
/* RDMA DRIVER                                                            */
/**************************************************************************/

typedef struct
{
  volatile int ack_number;
  volatile char ack[SCTK_MAX_RAIL_NUMBER];
} sctk_net_rdma_ack_t;

typedef struct
{
  volatile int used;
  struct ibv_sge list;
  struct ibv_send_wr wr;
  struct ibv_send_wr *bad_wr;
  sctk_net_rdma_ack_t *local_ack;
  sctk_net_rdma_ack_t dist_ack;
  size_t size;
} sctk_net_rdma_send_t;

typedef struct
{
  volatile int used;
  struct ibv_sge list;
  struct ibv_recv_wr wr;
  struct ibv_recv_wr *bad_wr;
  sctk_net_rdma_ack_t *local_ack;
  sctk_net_rdma_ack_t dist_ack;
} sctk_net_rdma_recv_t;

typedef struct
{
  sctk_spinlock_t lock;
  sctk_net_rdma_send_t headers[SCTK_PENDING_RDMA_NUMBER];
} sctk_net_rdma_rail_t;

static sctk_net_rdma_rail_t *sctk_rdma_rails;

static char *
sctk_ibv_print_status (enum ibv_wc_status status)
{
  switch (status)
    {
    case IBV_WC_SUCCESS:
      return ("IBV_WC_SUCCESS");
      break;
    case IBV_WC_LOC_LEN_ERR:
      return ("IBV_WC_LOC_LEN_ERR");
      break;
    case IBV_WC_LOC_QP_OP_ERR:
      return ("IBV_WC_LOC_QP_OP_ERR");
      break;
    case IBV_WC_LOC_EEC_OP_ERR:
      return ("IBV_WC_LOC_EEC_OP_ERR");
      break;
    case IBV_WC_LOC_PROT_ERR:
      return ("IBV_WC_LOC_PROT_ERR");
      break;
    case IBV_WC_WR_FLUSH_ERR:
      return ("IBV_WC_WR_FLUSH_ERR");
      break;
    case IBV_WC_MW_BIND_ERR:
      return ("IBV_WC_MW_BIND_ERR");
      break;
    case IBV_WC_BAD_RESP_ERR:
      return ("IBV_WC_BAD_RESP_ERR");
      break;
    case IBV_WC_LOC_ACCESS_ERR:
      return ("IBV_WC_LOC_ACCESS_ERR");
      break;
    case IBV_WC_REM_INV_REQ_ERR:
      return ("IBV_WC_REM_INV_REQ_ERR");
      break;
    case IBV_WC_REM_ACCESS_ERR:
      return ("IBV_WC_REM_ACCESS_ERR");
      break;
    case IBV_WC_REM_OP_ERR:
      return ("IBV_WC_REM_OP_ERR");
      break;
    case IBV_WC_RETRY_EXC_ERR:
      return ("IBV_WC_RETRY_EXC_ERR");
      break;
    case IBV_WC_RNR_RETRY_EXC_ERR:
      return ("IBV_WC_RNR_RETRY_EXC_ERR");
      break;
    case IBV_WC_LOC_RDD_VIOL_ERR:
      return ("IBV_WC_LOC_RDD_VIOL_ERR");
      break;
    case IBV_WC_REM_INV_RD_REQ_ERR:
      return ("IBV_WC_REM_INV_RD_REQ_ERR");
      break;
    case IBV_WC_REM_ABORT_ERR:
      return ("IBV_WC_REM_ABORT_ERR");
      break;
    case IBV_WC_INV_EECN_ERR:
      return ("IBV_WC_INV_EECN_ERR");
      break;
    case IBV_WC_INV_EEC_STATE_ERR:
      return ("IBV_WC_INV_EEC_STATE_ERR");
      break;
    case IBV_WC_FATAL_ERR:
      return ("IBV_WC_FATAL_ERR");
      break;
    case IBV_WC_RESP_TIMEOUT_ERR:
      return ("IBV_WC_RESP_TIMEOUT_ERR");
      break;
    case IBV_WC_GENERAL_ERR:
      return ("IBV_WC_GENERAL_ERR");
      break;
    }
  return ("");
}

static void
sctk_net_poll_rdma (int rail, int need_lock)
{
  struct ibv_wc wc[SCTK_PENDING_RDMA_NUMBER];
  int ne;
  int i;
  int j;
  static volatile size_t required = 0;
  static volatile size_t sended = 0;

  if (need_lock)
    sctk_spinlock_lock (&(sctk_rdma_rails[rail].lock));

  for (j = 0; j < sctk_process_number; j++)
    {
      if (j != sctk_process_rank)
	{
	  ne =
	    ibv_poll_cq (device_list[rail].of_cq[j], SCTK_PENDING_RDMA_NUMBER,
			 wc);
	  assume (ne >= 0);

#if 0
	  if (ne == 0)
	    {
	      int todo = 0;
	      for (i = 0; i < SCTK_PENDING_RDMA_NUMBER; i++)
		{
		  if (sctk_rdma_rails[rail].headers[i].used == 1
		      && sctk_rdma_rails[rail].headers[i].size > 0)
		    {
		      sctk_nodebug
			("Message %d of size (%lu)  %lu %p->%p NOT DONE", i,
			 sctk_rdma_rails[rail].headers[i].size,
			 sctk_rdma_rails[rail].headers[i].list.length,
			 sctk_rdma_rails[rail].headers[i].list.addr,
			 sctk_rdma_rails[rail].headers[i].wr.wr.rdma.
			 remote_addr);
		      todo = 1;
		    }
		}
	    }
#endif


	  for (i = 0; i < ne; i++)
	    {
	      sctk_net_rdma_send_t *header;
	      int id;
	      if (wc[i].status != IBV_WC_SUCCESS)
		{
		  sctk_error ("ERROR Vendor: %d", wc[i].vendor_err);
		  sctk_error ("Byte_len: %d", wc[i].byte_len);
		}
	      assume (wc[i].status == IBV_WC_SUCCESS);

	      id = wc[i].wr_id;
	      assume (id >= 0);
	      assume (id < SCTK_PENDING_RDMA_NUMBER);
	      header = &(sctk_rdma_rails[rail].headers[id]);

	      required += sctk_rdma_rails[rail].headers[id].size;
	      sended += wc[i].byte_len;
	      sctk_nodebug
		("Message %d of size (%lu/%lu)  (%lu/%lu) %lu %p->%p", id,
		 wc[i].byte_len, sctk_rdma_rails[rail].headers[id].size,
		 sended, required,
		 sctk_rdma_rails[rail].headers[id].list.length,
		 sctk_rdma_rails[rail].headers[id].list.addr,
		 sctk_rdma_rails[rail].headers[id].wr.wr.rdma.remote_addr);

	      if (sctk_rdma_rails[rail].headers[id].local_ack != NULL)
		{
		  int k;
		  int ack_done = 0;
		  sctk_net_rdma_ack_t *arg;


		  arg = sctk_rdma_rails[rail].headers[id].local_ack;

		  arg->ack[rail] = 1;
		  for (k = 0; k < SCTK_MAX_RAIL_NUMBER; k++)
		    {
		      if (arg->ack[k] == 1)
			{
			  ack_done++;
			}
		    }
		  if (ack_done == arg->ack_number)
		    {
		      assume (sctk_rdma_rails[rail].headers[id].used == 1);
		      sctk_nodebug ("Ack message nb %d on %d", id, rail);
		      sctk_rdma_rails[rail].headers[id].local_ack = NULL;
		      sctk_rdma_rails[rail].headers[id].used = 0;
		    }
		}
	      else
		{
		  assume (sctk_rdma_rails[rail].headers[id].used == 1);
		  sctk_nodebug ("Ack message nb %d on %d", id, rail);
		  sctk_rdma_rails[rail].headers[id].local_ack = NULL;
		  sctk_rdma_rails[rail].headers[id].used = 0;
		}
	    }

	  ne =
	    ibv_poll_cq (device_list[rail].if_cq[j], SCTK_PENDING_RDMA_NUMBER,
			 wc);
	  assume (ne >= 0);
	  if (ne > 0)
	    {
	      not_reachable ();
	    }
	}
    }

  if (need_lock)
    sctk_spinlock_unlock (&(sctk_rdma_rails[rail].lock));
}

static sctk_net_rdma_send_t *
sctk_net_get_rdma_header (int rail)
{
  sctk_net_rdma_send_t *tmp = NULL;
  int i;
  sctk_spinlock_lock (&(sctk_rdma_rails[rail].lock));
  do
    {
      for (i = 0; i < SCTK_PENDING_RDMA_NUMBER; i++)
	{
	  if (sctk_rdma_rails[rail].headers[i].used == 0)
	    {
	      tmp = &(sctk_rdma_rails[rail].headers[i]);
	      memset (tmp, 0, sizeof (sctk_net_rdma_send_t));
	      tmp->wr.wr_id = i;
	      tmp->wr.next = NULL;
	      tmp->used = 1;
	      break;
	    }
	}
      sctk_net_poll_rdma (rail, 0);
    }
  while (tmp == NULL);

  sctk_spinlock_unlock (&(sctk_rdma_rails[rail].lock));
  return tmp;
}

static sctk_net_rdma_send_t *
sctk_net_rdma_ib_write (int rail, void *local_addr, uint32_t lkey,
			void *dist_addr, uint32_t rkey, size_t size,
			int process, sctk_net_rdma_ack_t * local_ack)
{
  sctk_net_rdma_send_t *header;
  int rc;

  ibv_ident_data_t *dest;
  ibv_ident_data_t *loc;

  dest = peer_list[process];
  loc = peer_list[sctk_process_rank];

  header = sctk_net_get_rdma_header (rail);
  header->local_ack = local_ack;

  header->list.addr = (long) local_addr;
  header->list.lkey = lkey;

  header->list.length = size;
  header->size = size;
  sctk_nodebug ("SEND message %lu", size);

  header->wr.sg_list = &(header->list);
  header->wr.num_sge = 1;
  header->wr.opcode = IBV_WR_RDMA_WRITE;
  header->wr.send_flags = IBV_SEND_SIGNALED;
  header->wr.imm_data = 1;

  header->wr.wr.rdma.remote_addr = (long) dist_addr;
  header->wr.wr.rdma.rkey = rkey;

  header->bad_wr = NULL;

  sctk_nodebug ("Remote key %d Local key %d", header->wr.wr.rdma.rkey,
		header->list.lkey);
  sctk_nodebug ("SEND id %d rail %d ptr local %p-%p (%d) dist %p-%p (%d)",
		header->wr.wr_id, rail, local_addr,
		(char *) local_addr + size, lkey, dist_addr,
		(char *) dist_addr + size, rkey);
  rc =
    ibv_post_send (device_list[rail].qp[process], &(header->wr),
		   &(header->bad_wr));
  assume (rc == 0);
  return header;
}

static sctk_net_rdma_send_t *
sctk_net_rdma_ib_read (int rail, void *local_addr, uint32_t lkey,
		       void *dist_addr, uint32_t rkey, size_t size,
		       int process, sctk_net_rdma_ack_t * local_ack)
{
  sctk_net_rdma_send_t *header;
  int rc;

  ibv_ident_data_t *dest;
  ibv_ident_data_t *loc;

  dest = peer_list[process];
  loc = peer_list[sctk_process_rank];

  header = sctk_net_get_rdma_header (rail);
  header->local_ack = local_ack;


  header->list.addr = (long) local_addr;
  header->list.lkey = lkey;

  header->list.length = size;
  sctk_nodebug ("READ message %lu", size);

  header->wr.sg_list = &(header->list);
  header->wr.num_sge = 1;
  header->wr.opcode = IBV_WR_RDMA_READ;
  header->wr.send_flags = IBV_SEND_SIGNALED;
  header->wr.imm_data = 1;

  header->wr.wr.rdma.remote_addr = (long) dist_addr;
  header->wr.wr.rdma.rkey = rkey;
  header->bad_wr = NULL;

  sctk_nodebug ("Remote key %d Local key %d", header->wr.wr.rdma.rkey,
		header->list.lkey);
  sctk_nodebug ("READ id %d rail %d ptr local %p-%p (%d) dist %p-%p (%d)",
		header->wr.wr_id, rail, local_addr,
		(char *) local_addr + size, lkey, dist_addr,
		(char *) dist_addr + size, rkey);
  rc =
    ibv_post_send (device_list[rail].qp[process], &(header->wr),
		   &(header->bad_wr));
  assume (rc == 0);
  return header;
}

static void
sctk_net_rdma_ib_set_ack_single (int rail, int process, void *dist_ack)
{
  sctk_net_rdma_send_t *header;
  int rc;

  ibv_ident_data_t *dest;
  ibv_ident_data_t *loc;

  dest = peer_list[process];
  loc = peer_list[sctk_process_rank];

  header = sctk_net_get_rdma_header (rail);
  header->local_ack = NULL;

  memset ((char *) (header->dist_ack.ack), 0,
	  SCTK_MAX_RAIL_NUMBER * sizeof (char));
  header->dist_ack.ack_number = 1;
  header->dist_ack.ack[rail] = 1;


  header->list.addr = (long) &(header->dist_ack);
  header->list.length = sizeof (sctk_net_rdma_ack_t);
  header->size = sizeof (sctk_net_rdma_ack_t);
  sctk_nodebug ("SEND ACK message %lu", header->size);

  header->list.lkey = loc->lkey[rail];

  header->wr.sg_list = &(header->list);
  header->wr.num_sge = 1;
  header->wr.opcode = IBV_WR_RDMA_WRITE;
  header->wr.send_flags = IBV_SEND_SIGNALED;
  header->wr.imm_data = 1;
  header->wr.wr.rdma.remote_addr = (long) dist_ack;
  header->wr.wr.rdma.rkey = dest->rkey[rail];
  header->bad_wr = NULL;

  sctk_nodebug ("SEND ACK id %d rail %d ack local %p-%p dist %p-%p",
		header->wr.wr_id, rail, (char *) header->list.addr,
		(char *) header->list.addr + header->list.length,
		(char *) header->wr.wr.rdma.remote_addr,
		(char *) header->wr.wr.rdma.remote_addr +
		header->list.length);
  rc =
    ibv_post_send (device_list[rail].qp[process], &(header->wr),
		   &(header->bad_wr));
  assume (rc == 0);
}

static int
sctk_net_rdma_select_rail ()
{
#warning "should be optimize"
  return 0;
}

static void sctk_net_rdma_wait (sctk_net_rdma_ack_t * ack);

static int
sctk_net_rdma_read (void *local_addr, void *dist_addr, size_t size,
		    int process, sctk_net_rdma_ack_t * local_ack);

static void
sctk_net_rdma_read_distant_key (void *ptr, size_t size, int device_number,
				uint32_t * rkey, int process, int rail)
{
  sctk_net_rdma_send_t *header;
  sctk_net_rdma_ack_t local_ack;
  size_t tab_size;
  uint32_t lkey;
  uint32_t tab_rkey;
  ibv_ident_data_t *dest;
  ibv_ident_data_t *loc;
  static sctk_spinlock_t lock = 0;


  sctk_spinlock_lock (&lock);
  tab_size = device_list[rail].device_attr.max_mr *
    sizeof (ibv_soft_mmu_entry_t);

  dest = peer_list[process];
  loc = peer_list[sctk_process_rank];

  lkey = loc->lkey[rail];
  tab_rkey = dest->rkey[rail];

  local_ack.ack_number = 1;

  memset ((void *) ibv_soft_mmu_dest, 0, tab_size);

  header =
    sctk_net_rdma_ib_read (0, (void *) ibv_soft_mmu_dest, lkey,
			   ibv_soft_mmu[rail], tab_rkey, tab_size, process,
			   &local_ack);
  sctk_net_rdma_wait (&local_ack);

  sctk_nodebug ("Check result distant key");
  if (!ibv_is_register_soft_mmu_intern
      (ptr, size, NULL, rkey, (void *) ibv_soft_mmu_dest,
       device_list[rail].device_attr.max_mr))
    {
      if (!ibv_is_register_soft_mmu_intern
	  (ptr, size, NULL, rkey, (void *) ibv_soft_mmu_dest,
	   device_list[rail].device_attr.max_mr))
	{
	  not_implemented ();
	}
    }
  sctk_nodebug ("Check result distant key DONE");

  sctk_spinlock_unlock (&lock);
  sctk_nodebug ("Key %d", *rkey);
}

static inline int
sctk_net_rdma_write_on_rail (void *local_addr, void *dist_addr, size_t size,
			     int process, sctk_net_rdma_ack_t * local_ack,
			     sctk_net_rdma_ack_t * dist_ack, int rail)
{
  sctk_net_rdma_send_t *header;
  uint32_t lkey = 0;
  uint32_t rkey = 0;

  if (((unsigned long) local_addr >= (unsigned long) sctk_isoalloc_buffer) &&
      ((unsigned long) local_addr <
       (unsigned long) sctk_isoalloc_buffer + SCTK_MAX_ISO_ALLOC_SIZE)
      && ((unsigned long) dist_addr >= (unsigned long) sctk_isoalloc_buffer)
      && ((unsigned long) dist_addr <
	  (unsigned long) sctk_isoalloc_buffer + SCTK_MAX_ISO_ALLOC_SIZE))
    {
      ibv_ident_data_t *dest;
      ibv_ident_data_t *loc;

      dest = peer_list[process];
      loc = peer_list[sctk_process_rank];

      lkey = loc->lkey[rail];
      rkey = dest->rkey[rail];
    }
  else
    {
      sctk_net_rdma_read_distant_key (dist_addr, size, rail, &rkey, process,
				      rail);
      if (!ibv_is_register_soft_mmu (local_addr, size, rail, &lkey))
	{
	  not_implemented ();
	}
    }


  sctk_nodebug ("WRITE %p on %d from %p on %d", dist_addr, process,
		local_addr, sctk_process_rank);

  header =
    sctk_net_rdma_ib_write (rail, local_addr, lkey, dist_addr, rkey, size,
			    process, local_ack);
  return 0;
}

static int
sctk_net_rdma_write (void *local_addr, void *dist_addr, size_t size,
		     int process, sctk_net_rdma_ack_t * local_ack,
		     sctk_net_rdma_ack_t * dist_ack)
{
  if (local_ack != NULL)
    {
      memset ((char *) (local_ack->ack), 0, SCTK_MAX_RAIL_NUMBER);
    }

  sctk_nodebug ("PREPARE WRITE");

  if ((size <= SCTK_MULTIRAIL_THRESHOLD) || (num_devices <= 1))
    {
      int rail;

      /*select a rail */
      rail = sctk_net_rdma_select_rail ();

      if (local_ack != NULL)
	{
	  local_ack->ack_number = 1;
	}

      sctk_net_rdma_write_on_rail (local_addr, dist_addr, size, process,
				   local_ack, dist_ack, rail);

      if (dist_ack != NULL)
	{
	  sctk_nodebug ("ACK %p", dist_ack);
	  sctk_net_rdma_ib_set_ack_single (rail, process, dist_ack);
	}
    }
  else
    {
      /*Multirail approach */
      not_implemented ();
    }

  sctk_nodebug ("WRITE DONE");
  return 0;
}

static int
sctk_net_rdma_read (void *local_addr, void *dist_addr, size_t size,
		    int process, sctk_net_rdma_ack_t * local_ack)
{
  if (local_ack != NULL)
    {
      memset ((char *) (local_ack->ack), 0, SCTK_MAX_RAIL_NUMBER);
    }

  assume (local_ack);

  sctk_nodebug ("PREPARE READ");

  if ((size <= SCTK_MULTIRAIL_THRESHOLD) || (num_devices <= 1))
    {
      /*Single rail approach */
      sctk_net_rdma_send_t *header;
      int rail;
      uint32_t lkey = 0;
      uint32_t rkey = 0;

      /*select a rail */
      rail = sctk_net_rdma_select_rail ();

      if (((unsigned long) local_addr >= (unsigned long) sctk_isoalloc_buffer)
	  && ((unsigned long) local_addr <
	      (unsigned long) sctk_isoalloc_buffer + SCTK_MAX_ISO_ALLOC_SIZE)
	  && ((unsigned long) dist_addr >=
	      (unsigned long) sctk_isoalloc_buffer)
	  && ((unsigned long) dist_addr <
	      (unsigned long) sctk_isoalloc_buffer + SCTK_MAX_ISO_ALLOC_SIZE))
	{
	  ibv_ident_data_t *dest;
	  ibv_ident_data_t *loc;

	  dest = peer_list[process];
	  loc = peer_list[sctk_process_rank];

	  lkey = loc->lkey[rail];
	  rkey = dest->rkey[rail];
	}
      else
	{
	  sctk_net_rdma_read_distant_key (dist_addr, size, rail, &rkey,
					  process, rail);
	  if (!ibv_is_register_soft_mmu (local_addr, size, rail, &lkey))
	    {
	      not_implemented ();
	    }
	}

      if (local_ack != NULL)
	{
	  local_ack->ack_number = 1;
	}

      sctk_nodebug ("READ %p on %d to %p on %d", dist_addr, process,
		    local_addr, sctk_process_rank);
      header =
	sctk_net_rdma_ib_read (rail, local_addr, lkey, dist_addr, rkey, size,
			       process, local_ack);

    }
  else
    {
      /*Multirail approach */
      not_implemented ();
    }
  sctk_nodebug ("READ DONE");
  return 0;
}

typedef struct
{
  sctk_net_rdma_ack_t *ack;
  int done;
} sctk_net_rdma_wait_t;

static void
sctk_net_rdma_wait_poll_func (sctk_net_rdma_wait_t * arg)
{
  int i;
  int ack_done = 0;

  for (i = 0; i < num_devices; i++)
    {
      sctk_net_poll_rdma (i, 1);
    }

  if (arg->ack->ack_number <= 0)
    {
      arg->done = 0;
      return;
    }

  for (i = 0; i < SCTK_MAX_RAIL_NUMBER; i++)
    {
      if (arg->ack->ack[i] == 1)
	{
	  ack_done++;
	}
    }
  if (ack_done == arg->ack->ack_number)
    {
      arg->done = 1;
    }
}

static void
sctk_net_rdma_wait (sctk_net_rdma_ack_t * ack)
{
  sctk_net_rdma_wait_t wait_arg;
  wait_arg.ack = ack;
  wait_arg.done = 0;
  sctk_thread_wait_for_value_and_poll (&(wait_arg.done), 1,
				       (void (*)(void *))
				       sctk_net_rdma_wait_poll_func,
				       &wait_arg);

  sctk_nodebug ("%d %d/%d/%d/%d", ack->ack_number, ack->ack[0], ack->ack[1],
		ack->ack[2], ack->ack[3]);
  /* WARNING have to reinit the sctk_net_rdma_ack_t structure */
  memset ((char *) (ack->ack), 0, SCTK_MAX_RAIL_NUMBER * sizeof (char));
  ack->ack_number = -1;
}

static int
sctk_net_rdma_test (sctk_net_rdma_ack_t * ack)
{
  sctk_net_rdma_wait_t wait_arg;
  wait_arg.ack = ack;
  wait_arg.done = 0;

  sctk_net_rdma_wait_poll_func (&wait_arg);
  return wait_arg.done;
}

/******************************************************************/
/* RPC                                                            */
/******************************************************************/
#include "sctk_rpc_rdma.h"

/******************************************************************/
/* COLLECTIVES                                                    */
/******************************************************************/
/* Max number of nodes 2^16=65536*/
#define collective_op_steps 16
#define collective_buffer_size (1024)
#include "sctk_collectives_rdma.h"

/******************************************************************/
/* PT 2 PT                                                        */
/******************************************************************/
#define SCTK_NET_MESSAGE_NUMBER 128
#define SCTK_NET_MESSAGE_SPLIT_THRESHOLD (32*1024)
#define SCTK_NET_MESSAGE_BUFFER (SCTK_NET_MESSAGE_SPLIT_THRESHOLD*SCTK_NET_MESSAGE_NUMBER)
#include "sctk_pt2pt_rdma.h"

/******************************************************************/
/* DSM                                                            */
/******************************************************************/
#include "sctk_dsm_rdma.h"

/******************************************************************/
/* CONNECTION MAP                                                 */
/******************************************************************/
typedef struct {
  int process;
  void* tmp;
} sctk_connection_map_rpc_t;

typedef sctk_connection_map_rpc_t sctk_connection_map_t;

static sctk_connection_map_t  * volatile *sctk_connection_map = NULL;
static sctk_thread_mutex_t *sctk_connection_map_lock = NULL;

static inline void sctk_connection_check_no_lock(int process){
  assume(sctk_connection_map[process] == NULL);
  if(process < sctk_process_rank){
    size_t max_obj_size;
    sctk_connection_map_rpc_t msg;
    sctk_connection_map_t* buf;
    void* tmp = NULL;
    buf = sctk_malloc(sizeof(sctk_connection_map_t));
    
    msg.process = sctk_process_rank;
    
    max_obj_size = sctk_get_max_rpc_size_comm ();
    sctk_nodebug("Need connection to %d, max_size %lu",process,max_obj_size);
    
    assume(max_obj_size >= sizeof(sctk_connection_map_t));
    sctk_perform_rpc ((sctk_rpc_t) sctk_net_sctk_specific_adm_rpc_remote, process,
		      &msg, sizeof (sctk_connection_map_rpc_t));
    
    tmp = sctk_malloc(sizeof (sctk_net_ptp_slot_t));
#warning "Register tmp buffer for RDMAs"
    
    /* Prepare local pt2pt points*/
    buf->tmp = tmp;
    
    sctk_mpcrun_send_to_process(buf,sizeof(sctk_connection_map_t),process);
    sctk_mpcrun_read_to_process(buf,sizeof(sctk_connection_map_t),process);
    
    sctk_nodebug("Remote tab %p",buf->tmp);
    sctk_net_init_ptp_per_process(process,tmp,buf->tmp);
    
    sctk_connection_map[process] = buf;
    sctk_nodebug("Need connection to %d, max_size %lu done",process,max_obj_size);
  } else {
    sctk_connection_map_rpc_t msg;
    sctk_nodebug("Need connection to %d: ASK",process);
    msg.process = sctk_process_rank;
    sctk_perform_rpc ((sctk_rpc_t) sctk_net_sctk_specific_adm_rpc_remote, process,
		      &msg, sizeof (sctk_connection_map_rpc_t));
    while(sctk_connection_map[process] == NULL) sctk_thread_yield();
  }
  sctk_nodebug("Conenction to %d done",process);
}

static inline void sctk_connection_check(int process){
  assume(process != sctk_process_rank);
#ifndef SCTK_USE_STATIC_ALLOC_ONLY
  if(sctk_connection_map[process] == NULL){
    sctk_thread_mutex_lock(&(sctk_connection_map_lock[process]));
    if(sctk_connection_map[process] == NULL){
      sctk_connection_check_no_lock(process);
    }
    sctk_thread_mutex_unlock(&(sctk_connection_map_lock[process]));
  }
#endif
}

#ifndef SCTK_USE_STATIC_ALLOC_ONLY
static void
sctk_ib_sctk_specific_adm_rpc_remote (sctk_connection_map_rpc_t * arg)
{
  if(arg->process > sctk_process_rank){
    void* tmp = NULL;
    sctk_connection_map_t* buf;
    if(sctk_connection_map[arg->process] == NULL){
      size_t max_obj_size;
      max_obj_size = sctk_get_max_rpc_size_comm ();
      buf = sctk_malloc(sizeof(sctk_connection_map_t));
      sctk_nodebug("Need connection from %d, max_size %lu",arg->process,max_obj_size);
      
      sctk_mpcrun_read_to_process( buf,sizeof(sctk_connection_map_t),arg->process);
      
      sctk_nodebug("Remote tab %p %d %d",buf->tmp,buf->process, arg->process);
      
      tmp = sctk_malloc(sizeof (sctk_net_ptp_slot_t));      
#warning "Register tmp buffer for RDMAs"

      /* Prepare local pt2pt points*/
      sctk_net_init_ptp_per_process(arg->process,tmp,buf->tmp);
      
      buf->tmp = tmp;
      
      sctk_mpcrun_send_to_process(buf,sizeof(sctk_connection_map_t),arg->process);
      
      sctk_connection_map[arg->process] = buf;
    }
  } else {
    sctk_nodebug("Remote connection required");
    if(sctk_thread_mutex_trylock(&(sctk_connection_map_lock[arg->process])) == 0){
      if(sctk_connection_map[arg->process] == NULL){
	sctk_nodebug("Remote connection accepted");
	sctk_connection_check_no_lock(arg->process);
      } else {
	sctk_nodebug("Remote connection rejected");
      }
      sctk_thread_mutex_unlock(&(sctk_connection_map_lock[arg->process]));
    } else {
      sctk_nodebug("Remote connection rejected");
    }
  }
}
#endif


/******************************************************************/
/* INIT/END                                                       */
/******************************************************************/
#include "sctk_init_rdma.h"

void
sctk_net_init_driver_infiniband (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
#ifdef SCTK_USE_STATIC_ALLOC_ONLY
  sctk_net_init_ptp (1);
#else
  sctk_net_init_ptp (0);
#endif
  sctk_net_init_driver_init_modules ("IB");

  sctk_mpcrun_barrier ();
}

#define PRINT_ATTR(name_s,name) \
  sctk_nodebug("\t- %s %lu",name_s,(unsigned long)device_list[i].device_attr.name);


static ibv_ident_data_t *ibv_ident_data;

static void
sctk_net_connect_infiniband (int j){
  int i;
  for (i = 0; i < num_devices; i++)
    {
      int offset;

      offset = sctk_process_rank;
      sctk_nodebug ("OFFSET %d", offset);

      assume (device_list[i].qp[j] != NULL);

      struct ibv_qp_attr attr;
      {
	/*IB connection QP step RTR */
	int flags;

	memset (&attr, 0, sizeof (struct ibv_qp_attr));

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_512;
	attr.dest_qp_num =
	  peer_list[j]->qpn[i + offset * SCTK_MAX_RAIL_NUMBER];

	/*
	  DANGER
	  We need to operate on sctk_process_rank, otherwise it does not work 
	  but we don't know why!!!!
	*/
	sctk_silent_debug ("%d", sctk_process_rank);

	sctk_nodebug
	  ("CONNECT i = %d %d to %d dest_qp_num %d defined %d", i,
	   sctk_process_rank, j, attr.dest_qp_num,
	   device_list[i].qp[j]->qp_num);
	assume (attr.dest_qp_num != 0);

	attr.rq_psn = peer_list[j]->psn[i];
	sctk_nodebug ("PSN to %d device %d psn %d", j, i,
		      attr.rq_psn);
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = IB_RMDA_DEPTH;
	attr.min_rnr_timer = 12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = peer_list[j]->lid[i];
	sctk_nodebug ("LID to %d device %d lid %d", j, i,
		      attr.ah_attr.dlid);
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = IBV_ADM_PORT;

	flags = IBV_QP_STATE |
	  IBV_QP_AV |
	  IBV_QP_PATH_MTU |
	  IBV_QP_DEST_QPN |
	  IBV_QP_RQ_PSN |
	  IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	assume (ibv_modify_qp (device_list[i].qp[j], &attr, flags) ==
		0);
	sctk_nodebug ("ibv_modify_qp 2");
      }

      {
	/*IB connection QP step RTS */
	int flags;

	memset (&attr, 0, sizeof (struct ibv_qp_attr));

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 14;
	attr.retry_cnt = 7;
	attr.rnr_retry = 7;
	attr.sq_psn = ibv_ident_data->psn[i];
	attr.sq_psn = 0;
	attr.max_rd_atomic = IB_RMDA_DEPTH;

	flags = IBV_QP_STATE |
	  IBV_QP_TIMEOUT |
	  IBV_QP_RETRY_CNT |
	  IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

	assume (ibv_modify_qp (device_list[i].qp[j], &attr, flags) ==
		0);
	sctk_nodebug ("ibv_modify_qp 3");
      }
    }
}


void
sctk_net_preinit_driver_infiniband (void)
{
  int i;
  int j;
  /*
     Do not touch the following variable: infiniband issue if dev_list is global
   */
  struct ibv_device **dev_list;

  page_size = getpagesize ();
#ifdef SCTK_USE_STATIC_ALLOC_ONLY
  sctk_mpcrun_client_init ();
#else
  sctk_mpcrun_client_init_connect ();
#endif

  gethostname (ibv_hostname, MAX_HOST_SIZE);

  sctk_nodebug ("Init IB driver on %s %d nodes", ibv_hostname,
		sctk_process_number);

  ibv_ident_data = sctk_malloc (sizeof (ibv_ident_data_t) +
				SCTK_MAX_RAIL_NUMBER * sctk_process_number *
				sizeof (uint32_t));
  ibv_ident_data->qpn =
    (void *) (((char *) ibv_ident_data) + sizeof (ibv_ident_data_t));

  dev_list = ibv_get_device_list (&num_devices);
  if (!dev_list)
    {
      sctk_error ("Infiniband: No device found!!!");
    }
  assume (SCTK_MAX_RAIL_NUMBER >= num_devices);
  for (i = 0; i < num_devices; i++)
    {
      ibv_get_device_name (dev_list[i]);
      sctk_nodebug ("Found device %d %s", i,
		    ibv_get_device_name (dev_list[i]));
    }

  device_list = sctk_malloc (num_devices * sizeof (ibv_device_t));
  for (i = 0; i < num_devices; i++)
    {
      device_list[i].context = ibv_open_device (dev_list[i]);
      sctk_nodebug ("ibv_open_device");
      if (device_list[i].context == NULL)
	{
	  sctk_error ("Cannot get IB context.");
	}
      if (ibv_query_device
	  (device_list[i].context, &device_list[i].device_attr) != 0)
	{
	  sctk_error ("Unable to get device attr.");
	}
      sctk_nodebug ("DEVICE %d:", i);
      sctk_nodebug ("\t- %s %s", "fw_ver", device_list[i].device_attr.fw_ver);
      PRINT_ATTR ("max_mr", max_mr);
    }

  ibv_init_soft_mmu ();


  sctk_register_ptr (ibv_register_soft_mmu_all, ibv_unregister_soft_mmu_all);

  for (i = 0; i < num_devices; i++)
    {
      device_list[i].protection_domain =
	ibv_alloc_pd (device_list[i].context);
      sctk_nodebug ("ibv_alloc_pd");
      if (device_list[i].protection_domain == NULL)
	{
	  sctk_error ("Cannot get IB protection domain.");
	}
    }

  for (i = 0; i < num_devices; i++)
    {
      device_list[i].of_cq =
	malloc (sctk_process_number * sizeof (struct ibv_cq *));
      memset (device_list[i].of_cq, 0,
	      sctk_process_number * sizeof (struct ibv_cq *));

      device_list[i].if_cq =
	malloc (sctk_process_number * sizeof (struct ibv_cq *));
      memset (device_list[i].if_cq, 0,
	      sctk_process_number * sizeof (struct ibv_cq *));

      device_list[i].qp =
	malloc (sctk_process_number * sizeof (struct ibv_qp *));
      memset (device_list[i].qp, 0,
	      sctk_process_number * sizeof (struct ibv_qp *));
    }

  sctk_connection_map = sctk_malloc(sctk_process_number*sizeof(sctk_connection_map_t**));
  for (j = 0; j < sctk_process_number; j++)
    {
      sctk_connection_map[j] = NULL;
    }
  

  for (j = 0; j < sctk_process_number; j++)
    {
      if (j != sctk_process_rank)
	{
	  for (i = 0; i < num_devices; i++)
	    {
	      device_list[i].of_cq[j] =
		ibv_create_cq (device_list[i].context, IB_TX_DEPTH, NULL,
			       NULL, 0);
	      assume (device_list[i].of_cq[j]);
	      device_list[i].if_cq[j] =
		ibv_create_cq (device_list[i].context, IB_RX_DEPTH, NULL,
			       NULL, 0);
	      assume (device_list[i].if_cq[j]);
	      sctk_nodebug ("ibv_create_cq x2");
	      {
		struct ibv_qp_init_attr qp_init_attr;
		memset (&qp_init_attr, 0, sizeof (struct ibv_qp_init_attr));
		qp_init_attr.send_cq = device_list[i].of_cq[j];
		qp_init_attr.recv_cq = device_list[i].if_cq[j];
		qp_init_attr.cap.max_send_wr = IB_TX_DEPTH;
		qp_init_attr.cap.max_recv_wr = IB_RX_DEPTH;
		qp_init_attr.cap.max_send_sge = IB_MAX_SG_SQ;
		qp_init_attr.cap.max_recv_sge = IB_MAX_SG_RQ;
		qp_init_attr.cap.max_inline_data = IB_MAX_INLINE;
		qp_init_attr.qp_type = IBV_QPT_RC;
		qp_init_attr.sq_sig_all = 1;

		sctk_nodebug ("create qp device_list[%d].qp[%d] (%p)", i, j,
			      device_list[i].protection_domain);
		device_list[i].qp[j] =
		  ibv_create_qp (device_list[i].protection_domain,
				 &qp_init_attr);
		sctk_nodebug ("create qp device_list[%d].qp[%d]=%p", i, j,
			      device_list[i].qp[j]);
		assume (device_list[i].qp[j]);
		sctk_nodebug ("ibv_create_qp");
	      }

	      {
		int flags;
		struct ibv_qp_attr qp_attr;
		qp_attr.qp_state = IBV_QPS_INIT;
		qp_attr.pkey_index = 0;
		qp_attr.port_num = IBV_ADM_PORT;
		qp_attr.qp_access_flags =
		  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
		  IBV_ACCESS_REMOTE_READ;

		flags = IBV_QP_STATE |
		  IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

		assume (ibv_modify_qp (device_list[i].qp[j], &qp_attr, flags)
			== 0);
		sctk_nodebug ("ibv_modify_qp 1");
	      }
	      ibv_ident_data->qpn[i + j * SCTK_MAX_RAIL_NUMBER] =
		device_list[i].qp[j]->qp_num;
	      sctk_nodebug ("CREATE QP %d->%d qp_num %d", sctk_process_rank,
			    j, device_list[i].qp[j]->qp_num);
	    }
	}
    }
  for (i = 0; i < num_devices; i++)
    {
      if (ibv_query_port
	  (device_list[i].context, IBV_ADM_PORT, &device_list[i].port_attr))
	{
	  sctk_error ("Couldn't get port attr");
	}
      sctk_nodebug ("ibv_query_port");
      sctk_nodebug ("LID %d", device_list[i].port_attr.lid);
    }

  ibv_ident_data->rank = sctk_process_rank;
  ibv_ident_data->device_nb = num_devices;

  srand48 (getpid () * time (NULL));

  for (i = 0; i < num_devices; i++)
    {
      void *ptr;
      size_t size;
      ibv_soft_mmu_entry_t *entry;

      ptr = sctk_isoalloc_buffer;
      size = SCTK_MAX_ISO_ALLOC_SIZE;
      entry = ibv_register_perm_soft_mmu (ptr, size, i);
      ibv_ident_data->rkey[i] = entry->mr->rkey;
      ibv_ident_data->lkey[i] = entry->mr->lkey;
    }

  for (i = 0; i < num_devices; i++)
    {
      ibv_ident_data->lid[i] = device_list[i].port_attr.lid;
      ibv_ident_data->psn[i] = lrand48 () & 0xffffff;	/*a corriger */
    }
/*   ibv_print_ident (ibv_ident_data); */

  assume (sctk_mpcrun_client
	  (MPC_SERVER_REGISTER, NULL, 0, ibv_ident_data,
	   sizeof (ibv_ident_data_t) +
	   SCTK_MAX_RAIL_NUMBER * sctk_process_number * sizeof (uint32_t)) ==
	  0);

  peer_list = sctk_malloc (sctk_process_number * sizeof (ibv_ident_data_t *));
  for (i = 0; i < sctk_process_number; i++)
    {
      peer_list[i] = sctk_malloc (sizeof (ibv_ident_data_t) +
				  SCTK_MAX_RAIL_NUMBER * sctk_process_number *
				  sizeof (uint32_t));
      memset (peer_list[i], 0,
	      sizeof (ibv_ident_data_t) +
	      SCTK_MAX_RAIL_NUMBER * sctk_process_number * sizeof (uint32_t));
    }

  sctk_rdma_rails =
    sctk_iso_malloc (SCTK_MAX_RAIL_NUMBER * sizeof (sctk_net_rdma_rail_t));

  /*Implicit barrier */
  for (i = 0; i < sctk_process_number; i++)
    {
      while (sctk_mpcrun_client
	     (MPC_SERVER_GET, peer_list[i], sizeof (ibv_ident_data_t) +
	      SCTK_MAX_RAIL_NUMBER * sctk_process_number * sizeof (uint32_t),
	      &i, sizeof (int)) != 0)
	{
	  sleep(1);
	}
      peer_list[i]->qpn =
	(void *) (((char *) peer_list[i]) + sizeof (ibv_ident_data_t));
      ibv_print_ident (peer_list[i]);
    }

  for (j = 0; j < sctk_process_number; j++)
    {
      if (j != sctk_process_rank)
	{
	  sctk_net_connect_infiniband(j);
	}
      else
	{
	  for (i = 0; i < num_devices; i++)
	    {
	      assume (device_list[i].qp[j] == NULL);
	    }
	}
    }


  sctk_nodebug ("allocations");
#ifdef SCTK_USE_STATIC_ALLOC_ONLY
  sctk_net_preinit_driver_init_modules ();
#else
  sctk_net_sctk_specific_adm_rpc_remote_ptr = sctk_ib_sctk_specific_adm_rpc_remote;
  sctk_net_preinit_driver_init_modules_no_global ();

#endif
  sctk_free (ibv_ident_data);
}

#else
void
sctk_net_init_driver_infiniband (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);
  not_available ();
}

void
sctk_net_preinit_driver_infiniband (void)
{
  not_available ();
}
#endif
