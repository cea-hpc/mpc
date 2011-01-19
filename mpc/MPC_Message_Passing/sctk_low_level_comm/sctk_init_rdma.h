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
#include "sctk_hybrid_comm.h"
#include "sctk_shm_consts.h"

static char sctk_net_network_mode[4096];

static void
sctk_net_init_driver_init_modules (char *mode)
{
  char tmp[4096];

  sctk_iso_alloc_stat (tmp);

  /* different names if we use or not the SHM module */
  if (sctk_net_hybrid_is_shm_enabled)
    sprintf (sctk_net_network_mode, "SHM v%s/%s (RDMA) (%s)", SHM_VERSION, mode, tmp);
  else
    sprintf (sctk_net_network_mode, "%s (RDMA) (%s)", mode, tmp);

  sctk_network_mode = sctk_net_network_mode;
}

#ifdef SCTK_USE_STATIC_ALLOC_ONLY
static void
sctk_net_preinit_driver_init_modules ()
{
  /*
     ALLOCATIONS AND MODULES INIT
   */
  sctk_net_preinit_rpc (1);
  sctk_net_preinit_collectives (1);
  sctk_net_preinit_ptp (1);
}

#else
static void
sctk_net_preinit_driver_init_modules_no_global ()
{
  /*
     ALLOCATIONS AND MODULES INIT
   */
  sctk_net_preinit_rpc (0);
  sctk_net_preinit_collectives (0);
  sctk_net_preinit_ptp (0);
}
#endif
