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

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK_MULTIRAIL_IB_H_
#define __SCTK_MULTIRAIL_IB_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_spinlock.h>

struct sctk_rail_info_s;

void sctk_network_init_multirail_ib(char* name, char* topology);
void sctk_network_init_ib(char* name, char* topology);
void sctk_network_finalize_multirail_ib ();
void sctk_network_finalize_task_multirail_ib (int rank);
struct sctk_rail_info_s** sctk_network_get_rails();
char sctk_network_is_ib_used();

#ifdef __cplusplus
}
#endif
#endif
#endif
