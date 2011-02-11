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
#include "sctk_hybrid_comm.h"
#include "sctk_debug.h"
#include "sctk_bootstrap.h"
#include "sctk_thread.h"
#include "sctk_mpcrun_client.h"
#include <stdlib.h>
#include <slurm/pmi.h>

enum bootstrap_mode {
  PMI,
  TCP,
};
enum bootstrap_mode mode;

static int process_size;
static int process_rank;
static int local_process_size;
static int local_process_rank;
static int key_max;
static int val_max;
static int spawn;
static int appnum;
static int name_max;
static char* kvsname;

#define TCP_KEY_MAX 256
#define TCP_VAL_MAX 256

static void sctk_bootstrap_slurm_env()
{
  char* env;

  env = getenv("SLURM_NODEID");
  if (env) {
    sctk_node_rank = atoi(env);
  } else {
    sctk_node_rank = -1;
  }
  env = getenv("SLURM_NNODES");
  if (env) {
    sctk_node_number = atoi(env);
  } else {
    sctk_node_number = -1;
  }
  env = getenv("SLURM_LOCALID");
  if (env) {
    sctk_local_process_rank = atoi(env);
  } else {
    sctk_local_process_rank = -1;
  }
}

static void sctk_bootstrap_pmi_init()
{
  int res;
  int i;

  /*  get environment variables */
  sctk_bootstrap_slurm_env();

  sctk_nodebug("Init PMI");
  res = PMI_Init(&spawn);
  assume (res == PMI_SUCCESS);

  res = PMI_Get_size(&process_size);
  assume (res == PMI_SUCCESS);

  res = PMI_Get_rank(&process_rank);
  assume (res == PMI_SUCCESS);

  res = PMI_Get_clique_size(&local_process_size);
  assume (res == PMI_SUCCESS);

  sctk_local_process_number = local_process_size;
  sctk_process_rank = process_rank;
  sctk_process_number = process_size;

  res = PMI_Get_appnum(&appnum);
  assume (res == PMI_SUCCESS);

  if(sctk_process_rank == 0)
  {
    sctk_nodebug("spawn %d, size %d, rank %d, appnum %d", spawn, process_size, process_rank, appnum);
  }

  res = PMI_KVS_Get_name_length_max(&name_max);
  assume (res == PMI_SUCCESS);
  sctk_nodebug("Name max : %d", name_max);

  res = PMI_KVS_Get_key_length_max(&key_max);
  assume (res == PMI_SUCCESS);
  sctk_nodebug("Key max : %d", key_max);

  res = PMI_KVS_Get_value_length_max(&val_max);
  assume(res == PMI_SUCCESS);
  sctk_nodebug("Val max : %d", val_max);

  kvsname = sctk_malloc(name_max);
  res = PMI_KVS_Get_my_name(kvsname,name_max);
  assume (res == PMI_SUCCESS);
  sctk_nodebug("KVS name %s",kvsname);
}

void
sctk_bootstrap_register(char* pkey, char* pval, int size)
{
  int res;

  switch(mode)
  {
    case PMI:
      res = PMI_KVS_Put(kvsname, pkey, pval);
      assume(res == PMI_SUCCESS);

      res = PMI_KVS_Commit(kvsname);
      assume(res == PMI_SUCCESS);
      break;

    case TCP:
      sctk_mpcrun_client_register_shmfilename (pkey, pval, TCP_KEY_MAX, size);
      break;
  }
}

  void
sctk_bootstrap_get(char* pkey, char* pval, int size)
{
  int res;

  switch(mode)
  {
    case PMI:
      assume(size <= val_max);
      res = PMI_KVS_Get(kvsname, pkey, pval, size);
      assume(res == PMI_SUCCESS);
      break;

    case TCP:
      sctk_mpcrun_client_get_shmfilename (pkey, pval, TCP_KEY_MAX, size);
      break;
  }
}

  int
sctk_bootstrap_get_max_key_len()
{
  switch(mode)
  {
    case PMI:
      return key_max;
      break;

    case TCP:
      return TCP_KEY_MAX;
      break;
  }
}

  int
sctk_bootstrap_get_max_val_len()
{
  switch(mode)
  {
    case PMI:
      return val_max;
      break;

    case TCP:
      return TCP_VAL_MAX;
      break;
  }
}


void
  sctk_bootstrap_barrier() {
    switch (mode)
    {
      case PMI:
        PMI_Barrier();
        break;

      case TCP:
        sctk_mpcrun_barrier ();
        break;
    }
}

void sctk_bootstrap_init() {
  char* env = NULL;

  env = getenv("SLURM_JOBID");
  if ( env ) {
    mode = PMI;
  }
  else
    mode = TCP;

  switch (mode)
  {
    case PMI:
      sctk_bootstrap_pmi_init();
      sctk_mpcrun_client_create_recv_socket ();
      sctk_mpcrun_client_init_host_port();
      sctk_nodebug("Process rank : %d", sctk_process_rank);
      /* we need to send the number of processes to the
       * TCP server */
      if (sctk_process_rank == 0)
      {
        sctk_mpcrun_client_set_process_number();
      }
      PMI_Barrier();
      if (sctk_process_rank == 0)
        sctk_debug("Launch mode : PMI Launcher used");
      break;

    case TCP:
      /* connect the process to the TCP server.
       * Permits to grab the process_rank, node_number, local_process_rank */
      sctk_mpcrun_client_create_recv_socket ();
      sctk_mpcrun_client_init_host_port();
      sctk_mpcrun_client_get_global_consts();
      sctk_mpcrun_barrier ();

      /* also grab the number of processes in the node */
      sctk_mpcrun_client_get_local_consts();
      if (sctk_process_rank == 0)
        sctk_debug("Launch mode : TCP Launcher used");
      break;
  }
}
