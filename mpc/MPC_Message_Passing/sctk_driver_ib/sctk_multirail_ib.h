/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
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
#include "sctk_runtime_config.h"

struct sctk_rail_info_s;

void sctk_network_init_multirail_ib(int rail_id, int max_rails);
void sctk_network_init_ib(char* name, char* topology);

/* Initialize */
void sctk_network_initialize_task_multirail_ib (int rank, int vp);

/* Finalize */
void sctk_network_finalize_multirail_ib ();
void sctk_network_finalize_task_multirail_ib (int rank);
struct sctk_rail_info_s** sctk_network_get_rails();
char sctk_network_is_ib_used();

int sctk_network_ib_get_rail_signalization();
int sctk_network_ib_get_rails_nb();
size_t sctk_network_memory_allocator_hook_ib (size_t size);


int sctk_network_select_send_rail(sctk_thread_ptp_message_t * msg);
int sctk_network_select_recv_rail();
#ifdef __cplusplus
}
#endif
#endif
#endif
