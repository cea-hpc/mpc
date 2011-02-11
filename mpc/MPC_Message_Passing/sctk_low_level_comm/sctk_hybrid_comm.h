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
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                 # */
/* #                                                                      # */
/* ######################################################################## */
#define MPC_USE_SHM

#include "sctk_shm_mem_struct.h"
#include "sctk_mpcserver_actions.h"

#ifndef __SCTK__HYBRID_H_
#define __SCTK__HYBRID_H_
#ifdef __cplusplus
extern "C"
{
#endif

  void sctk_net_init_driver_hybrid (int *argc, char ***argv);
  void sctk_net_preinit_driver_hybrid ();

  typedef struct sctk_net_driver_pointers_functions_s
  {
    sctk_rpc_function_t          rpc_driver;
    sctk_rpc_retrive_function_t  rpc_driver_retrive;
    sctk_rpc_send_function_t     rpc_driver_send;

    void                         (*net_send_ptp_message)();
    void                         (*net_copy_message)();
    void                         (*net_free)();
    void                         (*collective)();
    void                         (*net_adm_poll) ();
    void                         (*net_ptp_poll) ();
    void                         (*net_new_comm) ();
    void                         (*net_free_comm) (int com_id);
  } sctk_net_driver_pointers_functions_t;
#define INIT_NET_DRIVER_POINTERS_FUNCTIONS {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
  /* translation table for SHM module */
  int* shm_local_to_global_translation_table;
  int* shm_global_to_local_translation_table;

  /* hooks called by sctk_low_level_comm when a creation or a destruction
   * of a communicator occures */
  void sctk_net_hybrid_init_new_com(sctk_internal_communicator_t* comm, int nb_involved, int* task_list);
  void sctk_net_hybrid_free_com(int com_id);

  void sctk_net_hybrid_finalize();

/* macro which shows informations about the SHM module */
#define SCTK_HYBRID_DEBUG 0


  /*
   * ===  FUNCTION  ===================================================
   *         Name:  sctk_shm_translate_global_to_local
   *  Description:  Function which translates the global rank of a process
   *  to a local rank.
   * ==================================================================
   */
static inline int sctk_shm_translate_global_to_local(int global_rank, int* local_rank)
{
  int i;

  sctk_nodebug("Lookup for process %d", global_rank);
  /* lockup inter-node */
  i = shm_global_to_local_translation_table[global_rank];

  if (i != -1)
  {
    sctk_nodebug("Process intra. Local rank : %d", i);
    if (local_rank != NULL)
      *local_rank = i;

    return SCTK_SHM_INTRA_NODE_COMM;
  }
  else
  {
    sctk_nodebug("Process inter");
    return SCTK_SHM_INTER_NODE_COMM;
  }
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_translate_local_to_global
 *  Description:  Function which translates the local rank of a process
 *  to a global rank.
 * ==================================================================
 */
static inline int sctk_shm_translate_local_to_global(int local_rank, int* global_rank)
{
  int i;

  sctk_nodebug("Lookup for local process %d", local_rank);
  /* lockup inter-node */
  i = shm_local_to_global_translation_table[local_rank];

  if (i != -1)
  {
    if (global_rank != NULL)
      *global_rank = i;
    return SCTK_SHM_INTRA_NODE_COMM;
  }
  else
    return SCTK_SHM_INTER_NODE_COMM;
}

int sctk_net_is_shm_enabled();

#ifdef __cplusplus
}
#endif
#endif
