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
#include "sctk_bootstrap.h"
#include "sctk_thread.h"
#include "sctk_mpcrun_client.h"
#include "sctk_pmi.h"
#include <stdlib.h>

static int key_max;
static int val_max;

static void sctk_bootstrap_pmi_init()
{
  int res;

  sctk_nodebug("Init PMI");
  sctk_mpcrun_client_get_global_consts();

  res = PMI_KVS_Get_key_length_max(&key_max);
  assume (res == SCTK_PMI_SUCCESS);
  sctk_nodebug("Key max : %d", key_max);

  res = PMI_KVS_Get_value_length_max(&val_max);
  assume(res == SCTK_PMI_SUCCESS);
  sctk_nodebug("Val max : %d", val_max);
}

void
sctk_bootstrap_register(char* pkey, char* pval, int size)
{
   assume(size <= val_max);
   sctk_mpcrun_client_register_shmfilename (pkey, pval, HOSTNAME_SIZE, size);
}

  void
sctk_bootstrap_get(char* pkey, char* pval, int size)
{
   assume(size <= val_max);
   sctk_mpcrun_client_get_shmfilename (pkey, pval, HOSTNAME_SIZE, size);
}

  int
sctk_bootstrap_get_max_key_len()
{
      return key_max;
}

  int
sctk_bootstrap_get_max_val_len()
{
      return val_max;
}

void
  sctk_bootstrap_barrier() {
  int res;
   res = sctk_pmi_barrier();
   assume(res == SCTK_PMI_SUCCESS);
}

void sctk_bootstrap_init() {
	int res;
      sctk_bootstrap_pmi_init();
      sctk_mpcrun_client_create_recv_socket ();
      sctk_mpcrun_client_init_host_port();
      res = sctk_pmi_barrier();
      assume(res == SCTK_PMI_SUCCESS);
       /* also grab the number of processes in the node */
      sctk_mpcrun_client_get_local_consts();
      if (sctk_process_rank == 0)
        fprintf(stderr, "MPC launcher: PMI used\n");
}
