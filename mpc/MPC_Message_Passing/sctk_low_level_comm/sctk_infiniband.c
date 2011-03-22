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
#include <slurm/pmi.h>

#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_rpc.h"
#include "sctk_infiniband.h"
#include "sctk_mpcrun_client.h"
#include "sctk_buffered_fifo.h"

#include "sctk_infiniband_scheduling.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_ibufs.h"
#include "sctk_infiniband_config.h"
#include "sctk_infiniband_qp.h"
#include "sctk_infiniband_mmu.h"
#include "sctk_infiniband_cm.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_comp_rc_rdma.h"

#ifdef MPC_USE_INFINIBAND

#include "sctk_rpc.h"
#include <infiniband/verbs.h>
#include "sctk_alloc.h"
#include "sctk_iso_alloc.h"
#include "sctk_net_tools.h"
#include "sctk_infiniband_profiler.h"

extern struct sctk_list* frag_eager_list;

/* rail */
extern  sctk_net_ibv_qp_rail_t   *rail;

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

/* RC SR structures */
extern  sctk_net_ibv_qp_local_t *rc_sr_local;

/* RC RDMA structures */
extern sctk_net_ibv_qp_local_t   *rc_rdma_local;
/* list where 1 entry <-> 1 process */

/* physical port number */
#define IBV_ADM_PORT 1

#include "sctk_infiniband_lib.h"

static char sctk_net_network_mode[4096];
void
sctk_net_ibv_update_network_mode()
{
  if (sctk_net_is_shm_enabled())
    sprintf (sctk_net_network_mode, "SHM v%s/IB-NG", SHM_VERSION);
  else
    sprintf (sctk_net_network_mode, "IB-NG");
  sctk_network_mode = sctk_net_network_mode;
}

#include "sctk_infiniband_coll.h"
  void
sctk_net_init_driver_infiniband (int *argc, char ***argv)
{
  assume (argc != NULL);
  assume (argv != NULL);

  /* init soft MMU */
  sctk_net_ibv_mmu_new(rail);

  /* channel selection */
  sctk_net_ibv_allocator_new();

  /* initialization of the Connection Manager */
  sctk_net_ibv_cm_server();

  /* message numbering */
  sctk_net_ibv_sched_init();

  /* initialization of queue pairs */
//  rc_rdma_local = sctk_net_ibv_comp_rc_rdma_create_local(rail);

  PMI_Barrier();

  /* initialization of collective */
  sctk_net_ibv_collective_init();

  sctk_net_ibv_config_init();

  sctk_ibv_profiler_init();

  /* initialization of buffers  */
  rc_sr_local = sctk_net_ibv_comp_rc_sr_create_local(rail);
  sctk_net_ibv_ibuf_new();
  sctk_net_ibv_ibuf_init(rail, rc_sr_local, ibv_max_ibufs);
  sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local);

  sctk_net_ibv_async_init(rc_sr_local->context);

  PMI_Barrier();
}

#define PRINT_ATTR(name_s,name) \
  sctk_nodebug("\t- %s %lu",name_s,(unsigned long)device_list[i].device_attr.name);


void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc);

  /*-----------------------------------------------------------
 *  PTP
 *----------------------------------------------------------*/

static void
sctk_net_ibv_copy_message_func_driver ( sctk_thread_ptp_message_t * restrict dest, sctk_thread_ptp_message_t * restrict src ) {
  not_reachable ();
  assume ( dest );
  assume ( src );
}


static void
sctk_net_ibv_free_func_driver ( sctk_thread_ptp_message_t * item ) {
  DBG_S(1);

  sctk_net_ibv_rc_rdma_process_t *entry_rdma;
  sctk_net_ibv_rc_rdma_entry_t *entry = NULL;
  struct sctk_list_elem* rc;
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_frag_eager_entry_t* frag_entry;

  sctk_nodebug("FREE begin");

  switch (item->channel_type)
  {
    case IBV_RC_SR_ORIGIN:
      sctk_nodebug("Free RC_SR message");

      /* release the ibuf */
      ibuf = (sctk_net_ibv_ibuf_t*)  item->struct_ptr;
      sctk_net_ibv_ibuf_release(ibuf, 1);
      sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local);
      break;

    case IBV_RC_RDMA_ORIGIN:
      sctk_nodebug("Free RC_RDMA message");

      entry = (sctk_net_ibv_rc_rdma_entry_t *)  item->struct_ptr;
      entry_rdma = entry->entry_rc_rdma;

      sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);
      sctk_list_lock(&entry_rdma->send);
      rc = sctk_list_remove(&entry_rdma->send, entry->list_elem);
      sctk_list_unlock(&entry_rdma->send);
      assume(rc);

      sctk_free(entry->msg_payload_ptr);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    case IBV_POLL_RC_SR_ORIGIN:
      sctk_nodebug("Free POLL_RC_SR");
      sctk_free(item->struct_ptr);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    case IBV_POLL_RC_RDMA_ORIGIN:
      entry = (sctk_net_ibv_rc_rdma_entry_t *)
        item->struct_ptr;

      sctk_nodebug("POLL_RC_RDMA %p", entry);
      sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);

      sctk_free(entry->msg_payload_ptr);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    case IBV_POLL_RC_SR_FRAG_ORIGIN:
    case IBV_RC_SR_FRAG_ORIGIN:
      frag_entry = (sctk_net_ibv_frag_eager_entry_t*)  item->struct_ptr;
      sctk_net_ibv_comp_rc_sr_free_frag_msg(frag_entry);
      break;

      frag_entry = (sctk_net_ibv_frag_eager_entry_t*)  item->struct_ptr;
      sctk_net_ibv_comp_rc_sr_free_frag_msg(frag_entry);
      break;


    default:
      assume(0);
      break;
  }
  sctk_nodebug("FREE exit");
  DBG_E(1);
}


/*-----------------------------------------------------------
 *  COLLECTIVE FUNCTIONS
 *----------------------------------------------------------*/
  static void
sctk_net_ibv_collective_op_driver (sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    const int root,
    void (*func) (const void *, void *, size_t,
      sctk_datatype_t),
    const sctk_datatype_t data_type)
{
  sctk_nodebug ("begin collective from root %d", root);
  if (nb_elem == 0)
  {
    sctk_nodebug ("begin collective barrier : %d", com->id);
    if ( (com->id) == MPC_COMM_WORLD)
    {
      sctk_net_ibv_barrier (com, my_vp );
//      PMI_Barrier();
    } else {
      not_implemented();
    }
    sctk_nodebug ("end collective barrier");
  }
  else
  {
    if (func == NULL)
    {
      sctk_nodebug ("begin collective broadcast %d", root);
      sctk_net_ibv_broadcast (com, my_vp, elem_size, nb_elem, root);
      sctk_nodebug ("end collective broadcast");
    }
    else
    {
      sctk_nodebug ("begin collective reduce for root %d", root);
      sctk_net_ibv_allreduce ( com, my_vp, elem_size, nb_elem, func,
          data_type );
      sctk_nodebug ("end collective reduce");
    }
  }
  sctk_nodebug ("end collective");
}

/*-----------------------------------------------------------
 *  RPC
 *----------------------------------------------------------*/
static void
sctk_net_rpc_register(void* addr, size_t size)
{
  sctk_nodebug("BEGIN rpc_register");

  sctk_nodebug("END rpc_register");
}

static void
sctk_net_rpc_unregister(void* addr, size_t size)
{
  sctk_nodebug("BEGIN rpc_unregister");

  sctk_nodebug("END rpc_unregister");
}


static void
sctk_net_rpc_driver ( void ( *func ) ( void * ), int destination, void *arg, size_t arg_size ) {
  sctk_nodebug("BEGIN rpc_driver");

  sctk_nodebug("END rpc_driver");
}

static void
sctk_net_rpc_retrive_driver ( void *dest, void *src, size_t arg_size,
    int process, int *ack ) {
  sctk_nodebug("BEGIN rpc_retrive_driver");

  sctk_nodebug("END rpc_retrive_driver");
}

static void
sctk_net_rpc_send_driver ( void *dest, void *src, size_t arg_size, int process,
    int *ack ) {
  sctk_nodebug("BEGIN rpc_send_driver");

  DBG_S ( 0 );
   not_reachable ();
  assume ( dest );
  assume ( src );
  assume ( arg_size );
  assume ( process );
  assume ( ack );
  DBG_E ( 0 );

  sctk_nodebug("END rpc_send_driver");
}

/*-----------------------------------------------------------
 *  FUNCTIONS REGISTRATION
 *----------------------------------------------------------*/
  void
sctk_net_ibv_register_pointers_functions (sctk_net_driver_pointers_functions_t* pointers)
{
  /* save all pointers to functions */
  /* TODO: Use TCP servers for rpc messages */
  pointers->rpc_driver          = sctk_net_rpc_driver;
  pointers->rpc_driver_retrive  = sctk_net_rpc_retrive_driver;
  pointers->rpc_driver_send     = sctk_net_rpc_send_driver;

  /*
   * registration for memory registration/unregistration
   */
  sctk_register_ptr(sctk_net_rpc_register, sctk_net_rpc_unregister);

  /*
   * registration of RPC functions
   */
  pointers->net_send_ptp_message= sctk_net_ibv_allocator_send_ptp_message;

  pointers->net_copy_message    = sctk_net_ibv_copy_message_func_driver;
  pointers->net_free            = sctk_net_ibv_free_func_driver;

  pointers->collective          = sctk_net_ibv_collective_op_driver;

  //  pointers->net_adm_poll        = sctk_net_adm_poll_func;	/* RPC */
  pointers->net_ptp_poll        = sctk_net_ibv_allocator_ptp_poll_all; /*  PTP */

  pointers->net_new_comm        = NULL;
  pointers->net_free_comm       = NULL;
}

  void
sctk_net_preinit_driver_infiniband ( sctk_net_driver_pointers_functions_t* pointers )
{

  if (sctk_process_rank == 0)
    sctk_nodebug("INIT driver!");

  /* register pointers to functions */
  sctk_net_ibv_register_pointers_functions(pointers);

  //  sctk_net_ibv_qp_rail_init();
  rail = sctk_net_ibv_qp_pick_rail(0);

  /* initialization of network mode */
  sctk_net_ibv_update_network_mode();

  frag_eager_list = sctk_malloc(sctk_get_total_tasks_number() * sizeof(frag_eager_list));
  memset(frag_eager_list, 0, sctk_get_total_tasks_number() * sizeof(frag_eager_list));
  sctk_nodebug("nb total of tasks : %d", sctk_get_total_tasks_number());

  if (sctk_process_rank == 0)
    sctk_nodebug("End of driver init!");
}

  void
sctk_net_ibv_finalize()
{
  sctk_ibv_generate_report();
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
sctk_net_preinit_driver_infiniband ( sctk_net_driver_pointers_functions_t* pointers )
{
  not_available ();
}
#endif
