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

#ifndef __SCTK_IB_FALLBACK_H_
#define __SCTK_IB_FALLBACK_H_
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __SCTK_ROUTE_H_
#error "sctk_route must be included before sctk_ib_fallback.h"
#endif

#include <sctk_spinlock.h>

#define MAX_STRING_SIZE 2048
  void sctk_network_init_ib(sctk_rail_info_t* rail);
  void sctk_network_init_polling_thread (sctk_rail_info_t* rail, char* topology);

#ifdef __cplusplus
}
#endif
#endif
