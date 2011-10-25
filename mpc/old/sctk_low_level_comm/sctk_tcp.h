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
#ifndef __SCTK__TCP_H_
#define __SCTK__TCP_H_
#include "sctk_hybrid_comm.h"
#ifdef __cplusplus
extern "C"
{
#endif

  void sctk_net_init_driver_tcp (int *argc, char ***argv);
  void sctk_net_preinit_driver_tcp (sctk_net_driver_pointers_functions_t* pointers);
  void sctk_net_init_driver_ipoib (int *argc, char ***argv);
  void sctk_net_preinit_driver_ipoib (sctk_net_driver_pointers_functions_t* pointers);
  void* sctk_tcp_net_get_adm_poll_func();

#ifdef __cplusplus
}
#endif
#endif
