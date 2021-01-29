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

#ifndef __MPC_LOWCOMM_IB_MPI_H_
#define __MPC_LOWCOMM_IB_MPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lowcomm_types_internal.h"

#include <sctk_ibufs.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_helper.h>


struct _mpc_lowcomm_ib_ibuf_s;
struct sctk_ib_polling_s;

void sctk_network_init_mpi_ib ( sctk_rail_info_t *rail );
void sctk_network_finalize_mpi_ib(sctk_rail_info_t *rail);

void sctk_network_memory_free_hook_ib ( void * ptr, size_t size );

int sctk_network_poll_recv_ibuf ( const sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf);
int sctk_network_poll_send_ibuf ( sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf );
int sctk_network_poll_all ( sctk_rail_info_t *rail, struct sctk_ib_polling_s *poll );

int sctk_ib_device_found();

char sctk_network_is_ib_used();

#ifdef __cplusplus
}
#endif

#endif

