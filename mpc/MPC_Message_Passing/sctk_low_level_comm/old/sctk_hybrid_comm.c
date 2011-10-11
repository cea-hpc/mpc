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
/* #   - PERACHE Marc     marc.perache@cea.fr                             # */
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                    # */
/* #                                                                      # */
/* ######################################################################## */



#include "sctk_low_level_comm.h"
#include "sctk_hybrid_comm.h"
#include "sctk_bootstrap.h"
#include "sctk_debug.h"
#include "sctk_rpc.h"
#include "sctk_shm.h"
#include "sctk_pmi.h"
#include "sctk_ib.h"
#include "sctk_bootstrap.h"
#include "sctk_tcp.h"
#include "sctk_shm_mem_struct_funcs.h"
#include "sctk_mpcrun_client.h"

#define MAX_MODULES_LEVELS 5

/* intra and inter comm counters */
#if SCTK_HYBRID_DEBUG == 1
static int nb_intra_comm = 0;
static int nb_inter_comm = 0;
#endif

/* if a system thread is needed for RDMA messages */
int rdma_system_thread_needed = 1;

//sctk_net_driver_pointers_functions_t sctk_net_driver_pointers_functions_array[MAX_MODULES_LEVELS];
static int number_modules_levels = 0;
/* hybrid modue */
static int shm_enabled = 1;

 sctk_net_driver_pointers_functions_t pointers_inter  = INIT_NET_DRIVER_POINTERS_FUNCTIONS;
  sctk_net_driver_pointers_functions_t pointers_hybrid = INIT_NET_DRIVER_POINTERS_FUNCTIONS;
  sctk_net_driver_pointers_functions_t pointers_tmp    = INIT_NET_DRIVER_POINTERS_FUNCTIONS;
#ifdef SCTK_SHM
  sctk_net_driver_pointers_functions_t pointers_intra  = INIT_NET_DRIVER_POINTERS_FUNCTIONS;
#endif

/* hooks on communicators creation and destruction */
void sctk_net_hybrid_init_new_com(sctk_internal_communicator_t* comm,
    int nb_involved, int* task_list)
{
#ifdef SCTK_SHM
  if (shm_enabled)
  {
    if (pointers_intra.net_new_comm != NULL)
      pointers_intra.net_new_comm(comm, nb_involved, task_list);
  }
#endif
  if (pointers_inter.net_new_comm != NULL)
    pointers_inter.net_new_comm(comm, nb_involved, task_list);
}
void sctk_net_hybrid_free_com(int com_id)
{
#ifdef SCTK_SHM
  if (shm_enabled)
  {
    if (pointers_intra.net_free_comm != NULL)
      pointers_intra.net_free_comm(com_id);
  }
#endif
  if (pointers_inter.net_free_comm != NULL)
    pointers_inter.net_free_comm(com_id);
}


  /*-----------------------------------------------------------
   *  Polling functions
   *----------------------------------------------------------*/
  void
  sctk_net_hybrid_ptp_poll(void* arg)
{
#ifdef SCTK_SHM
  if (shm_enabled)
  {
    if (pointers_intra.net_ptp_poll != NULL)
      pointers_intra.net_ptp_poll(arg);
  }
#endif
  if (pointers_inter.net_ptp_poll != NULL)
    pointers_inter.net_ptp_poll(arg);
}

  void
  sctk_net_hybrid_adm_poll(void* arg)
{
#ifdef SCTK_SHM
  if (shm_enabled)
  {
    if (pointers_intra.net_adm_poll != NULL)
      pointers_intra.net_adm_poll(arg);
  }
#endif
  if (rdma_system_thread_needed == 1) {
    if (pointers_inter.net_adm_poll != NULL)
        pointers_inter.net_adm_poll(arg);
  }
}


#define INTRA_BEGIN(process)  \
  int local_rank;             \
  int com_type;               \
  if (!shm_enabled)           \
    goto inter;               \
                              \
  com_type = sctk_shm_translate_global_to_local(process, &local_rank);  \
  if ( (com_type == SCTK_SHM_INTRA_NODE_COMM) )                         \
  {                                                                     \

#define INTRA_END \
  }else {         \
    inter:        \

#define INTER_END \
  }               \

  static void
sctk_net_rpc_driver (void (*func) (void *), int destination, void *arg,
    size_t arg_size)
{
#ifdef SCTK_SHM
  INTRA_BEGIN(destination)
#if SCTK_HYBRID_DEBUG == 1
  ++nb_intra_comm;
#endif
  sctk_nodebug("INTRA net_rpc_driver | dest %d | local %d", destination, local_rank);
  pointers_intra.rpc_driver(func, local_rank, arg, arg_size);
  INTRA_END
#endif
#if SCTK_HYBRID_DEBUG == 1
  ++nb_inter_comm;
#endif
  sctk_nodebug("INTER net_rpc_driver");
  pointers_inter.rpc_driver(func, destination, arg, arg_size);
#ifdef SCTK_SHM
  INTER_END
#endif
}

  static void
sctk_net_rpc_send_driver (void *dest, void *src, size_t arg_size, int process,
    int *ack, uint32_t rkey)
{
#ifdef SCTK_SHM
  INTRA_BEGIN(process)
#if SCTK_HYBRID_DEBUG == 1
  ++nb_intra_comm;
#endif
  sctk_nodebug("INTRA net_rpc_send_driver");
  pointers_intra.rpc_driver_retrive(dest, src, arg_size, local_rank, ack, rkey);
  INTRA_END
#endif
#if SCTK_HYBRID_DEBUG == 1
  ++nb_inter_comm;
#endif
  sctk_nodebug("INTER net_rpc_send_driver");
  pointers_inter.rpc_driver_retrive(dest, src, arg_size, process, ack, rkey);
#ifdef SCTK_SHM
  INTER_END
#endif
}

  static void
sctk_net_rpc_retrieve_driver (void *dest, void *src, size_t arg_size,
    int process, int *ack, uint32_t rkey)
{
#ifdef SCTK_SHM
  INTRA_BEGIN(process)
#if SCTK_HYBRID_DEBUG == 1
  ++nb_intra_comm;
#endif
  sctk_nodebug("INTRA net_rpc_retrieve_driver");
  pointers_intra.rpc_driver_retrive(dest, src, arg_size, local_rank, ack, rkey);
  INTRA_END
#endif
#if SCTK_HYBRID_DEBUG == 1
  ++nb_inter_comm;
#endif
  sctk_nodebug("INTER net_rpc_retrieve_driver");
  pointers_inter.rpc_driver_retrive(dest, src, arg_size, process, ack, rkey);
#ifdef SCTK_SHM
  INTER_END
#endif
}

  static void
sctk_net_collective_op_driver (sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    int root,
    void (*func) (const void *, void *, size_t,
      sctk_datatype_t),
    const sctk_datatype_t data_type)
{
#ifdef SCTK_SHM
    if ( (shm_enabled) && (sctk_node_number == 1) )
    {
#if SCTK_HYBRID_DEBUG == 1
      ++nb_intra_comm;
#endif
      sctk_nodebug("INTRA collective");
      pointers_intra.collective(com, my_vp, elem_size, nb_elem, root, func, data_type);
    } else {
#endif
#if SCTK_HYBRID_DEBUG == 1
  ++nb_inter_comm;
#endif
    sctk_nodebug("INTER collective");
    pointers_inter.collective(com, my_vp, elem_size, nb_elem, root, func, data_type);
#ifdef SCTK_SHM
  }
#endif
  sctk_nodebug("End collective");
}

  static void
sctk_net_send_ptp_message_driver (sctk_thread_ptp_message_t * msg,
    int dest_process)
{
#ifdef SCTK_SHM
  msg->sent_by_shm = 1;
  INTRA_BEGIN(dest_process)
#if SCTK_HYBRID_DEBUG == 1
  ++nb_intra_comm;
#endif
  sctk_nodebug("INTRA net_send_ptp | local %d", local_rank);
  pointers_intra.net_send_ptp_message(msg, local_rank);
  INTRA_END
#endif
#if SCTK_HYBRID_DEBUG == 1
  ++nb_inter_comm;
#endif
  msg->sent_by_shm = 0;
    sctk_nodebug("INTER net_send_ptp process %d", dest_process);
  pointers_inter.net_send_ptp_message(msg, dest_process);
#ifdef SCTK_SHM
  INTER_END
#endif
    sctk_nodebug("END PTP");
}

  static void
sctk_net_copy_message_func_driver (sctk_thread_ptp_message_t * restrict dest,
    sctk_thread_ptp_message_t * restrict src)
{
  dest->sent_by_shm = 0;
  sctk_nodebug("INTRA net_copy_message");
  pointers_inter.net_copy_message(dest, src);
  sctk_nodebug("INTER net_copy_message");
}

  static void
sctk_net_free_func_driver (sctk_thread_ptp_message_t * item)
{
/*  / ! \ According to the module used to send the message, we need to use
 *  a different 'free' function. There was no way to find out if SHM or TCP
 *  was used to send the message.
 *  In order to achieve this, an entry 'sent_by_shm' has been added to
 *  the structure 'sctk_thread_ptp_message_t' */

#ifdef SCTK_SHM
  if ( (shm_enabled) && (item->sent_by_shm == 1) )
  {
    sctk_nodebug("INTRA net_free_func");
    pointers_intra.net_free(item);
    sctk_nodebug("INTRA net_free_func");
  }
  else
  {
    sctk_nodebug("INTER net_free_func");
    pointers_inter.net_free(item);
    sctk_nodebug("INTER net_free_func");
  }
#else
  sctk_nodebug("INTER net_free_func");
  pointers_inter.net_free(item);
  sctk_nodebug("END net_free_func");
#endif
}

void
sctk_net_hybrid_finalize()
{
#ifdef SCTK_SHM
#if SCTK_HYBRID_DEBUG == 1
   if ( (shm_enabled) && (sctk_process_rank == 0) )
   {
      fprintf(stderr, "\n# ------------------HYBRID FINALIZE-------------------------\n"
          "# Number of intra communications : %d\n"
          "# Number of inter communications : %d\n"
          "# ---------------------------------------------------------\n",
          nb_intra_comm, nb_inter_comm);
   }
#endif
#endif

#ifdef MPC_USE_INFINIBAND
   sctk_net_ibv_finalize();
#endif

}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_inter_thread_rpc
 *  Description:  Function used for the inter-comm module.
 *                / ! \ Cannot put this function into the the rpc polling
 *                function because code blocks.
 * ==================================================================
 */
  static void *
sctk_inter_comm_thread(void *arg)
{
  while(1)
  {
    pointers_inter.net_adm_poll();
    usleep(5);
  }
  return NULL;
}

int
sctk_net_is_shm_enabled()
{
  return shm_enabled;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_preinit_driver_hybrid
 *  Description:  Pre-init the driver.
 * ==================================================================
 */
/* init a new gen driver */
#define GENDRIVER(dr_name,gen_name)			\
  sctk_net_preinit_driver_##dr_name(&pointers_tmp);		\

  void
sctk_net_preinit_driver_hybrid ()
{
  sctk_bootstrap_init();

  /* Determine if SHM module should be activated */
#ifdef SCTK_SHM
  if (strcmp(sctk_module_name, "tcp_only") == 0 ||
      strcmp(sctk_module_name, "ipoib_only") == 0 ||
      strcmp(sctk_module_name, "ib_only") == 0)
  {
    shm_enabled = 0;
  } else {
    shm_enabled = 1;
  }
#else
  shm_enabled = 0;
#endif

  /* initialize intra communication module with local size and rank as
   * parameters */
#ifdef SCTK_SHM
  if (shm_enabled)
  {
    sctk_nodebug("Init driver SHM");
    sctk_net_preinit_driver_shm(&pointers_tmp);
    shm_local_to_global_translation_table = sctk_shm_get_local_to_global_process_translation_table();
    shm_global_to_local_translation_table = sctk_shm_get_global_to_local_process_translation_table();
    number_modules_levels++;
    pointers_intra = pointers_tmp;
    sctk_nodebug("pointer = %p", pointers_intra.rpc_driver);
  }
#endif

  if ( (strcmp(sctk_module_name, "tcp") == 0) ||
      (strcmp(sctk_module_name, "tcp_only") == 0))
  {
#ifdef MPC_USE_TCP
    GENDRIVER(tcp, tcp);
#else
    sctk_debug_root ("ERROR: Network mode |%s| not available.\n"
        "Please compile MPC with TCP support by passing\n"
        "the argument \"--use-network=%s\" to the MPC configure script.", sctk_module_name, sctk_module_name);
    exit(1);
#endif

  }
  else if ( (strcmp(sctk_module_name, "ib") == 0) ||
      (strcmp(sctk_module_name, "ib_only") == 0))
  {
#ifdef MPC_USE_INFINIBAND
    if (sctk_pmi_is_initialized() == PMI_TRUE)
    {
      GENDRIVER(infiniband, infiniband);
    } else {
      sctk_debug_root("ERROR: The Infiniband module _MUST_ be initialized with the SLURM job manager.\n"
          "Please pass the argument -l=srun to your mpcrun command line. \n"
          "Other job manager are not currently supported.");
      exit(1);
    }
#else
    sctk_debug_root ("ERROR: Network mode |%s| not available.\n"
        "Please compile MPC with Infiniband support by passing\n"
        "the argument \"--use-network=%s\" to the MPC configure script.", sctk_module_name, sctk_module_name);
    exit(1);
#endif
  }
  else if ( (strcmp(sctk_module_name, "ipoib") == 0) ||
      (strcmp(sctk_module_name, "ipoib_only") == 0))
  {
    GENDRIVER(ipoib, ipoib);
  }
  else
  {
    if (sctk_process_rank == 0)
    {
      sctk_error ("Network mode |%s| not available", sctk_module_name);
    }
    exit(1);
  }

  number_modules_levels++;
  pointers_inter = pointers_tmp;


  pointers_hybrid.rpc_driver          = sctk_net_rpc_driver;
  pointers_hybrid.rpc_driver_retrive  = sctk_net_rpc_retrieve_driver;
  pointers_hybrid.rpc_driver_send     = sctk_net_rpc_send_driver;
  pointers_hybrid.net_send_ptp_message= sctk_net_send_ptp_message_driver;
  pointers_hybrid.net_copy_message    = sctk_net_copy_message_func_driver;
  pointers_hybrid.net_free            = sctk_net_free_func_driver;
  pointers_hybrid.collective          = sctk_net_collective_op_driver;

  pointers_hybrid.net_new_comm        = sctk_net_hybrid_init_new_com;
  pointers_hybrid.net_free_comm       = sctk_net_hybrid_free_com;

  sctk_net_adm_poll = sctk_net_hybrid_adm_poll;	/* RPC */
  sctk_net_ptp_poll = sctk_net_hybrid_ptp_poll;	/* PTP */

  sctk_rpc_init_func (
      pointers_hybrid.rpc_driver,
      pointers_hybrid.rpc_driver_retrive,
      pointers_hybrid.rpc_driver_send);

  sctk_send_init_func (
      pointers_hybrid.net_send_ptp_message,
      pointers_hybrid.net_copy_message,
      pointers_hybrid.net_free);

  sctk_collective_init_func (
      pointers_hybrid.collective);

  sctk_set_net_val(sctk_net_init_driver_hybrid);

  sctk_nodebug("Number of modules initialized : %d", number_modules_levels);
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_init_driver_hybrid
 *  Description:  Init the driver
 * ==================================================================
 */
  void
sctk_net_init_driver_hybrid (int *argc, char ***argv)
{
  sctk_thread_t pidt_inter_rdma;
  sctk_thread_attr_t attr_inter_rdma;


#ifdef SCTK_SHM
  if (shm_enabled)
  {
    sctk_net_init_driver_shm(argc, argv);
  }
#endif

  if ( (strcmp(sctk_module_name, "tcp") == 0) ||
      (strcmp(sctk_module_name, "tcp_only") == 0))
  {
    /* thread for TCP/IPoIB DMA
     * / ! \ : we create a thread instead of polling the function. Interblocking issues.*/
    sctk_thread_attr_init (&attr_inter_rdma);
    sctk_thread_attr_setscope (&attr_inter_rdma, SCTK_THREAD_SCOPE_SYSTEM);
    sctk_user_thread_create (&pidt_inter_rdma, &attr_inter_rdma, sctk_inter_comm_thread,
        NULL);

    rdma_system_thread_needed = 0;

    sctk_net_init_driver_tcp(argc, argv);
  }
  else if ( (strcmp(sctk_module_name, "ipoib") == 0) ||
      (strcmp(sctk_module_name, "ipoib_only") == 0))
  {
    /* thread for TCP/IPoIB DMA
     * / ! \ : we create a thread instead of polling the function. Interblocking issues.*/
    sctk_thread_attr_init (&attr_inter_rdma);
    sctk_thread_attr_setscope (&attr_inter_rdma, SCTK_THREAD_SCOPE_SYSTEM);
    sctk_user_thread_create (&pidt_inter_rdma, &attr_inter_rdma, sctk_inter_comm_thread,
        NULL);
    rdma_system_thread_needed = 0;

    sctk_net_init_driver_ipoib(argc, argv);
  }
  else if ( (strcmp(sctk_module_name, "ib") == 0) ||
      (strcmp(sctk_module_name, "ib_only") == 0))
  {
#ifdef MPC_USE_INFINIBAND
    sctk_net_init_driver_infiniband(argc, argv);
#endif
  }
}
