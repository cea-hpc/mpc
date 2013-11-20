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

#ifndef __SCTK_IB_MPI_H_
#define __SCTK_IB_MPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_route.h>
#include <sctk_ibufs.h>
#include <sctk_spinlock.h>

    struct sctk_ibuf_s;
    struct sctk_ib_polling_s;

#define MAX_STRING_SIZE 2048
void sctk_network_init_mpi_ib(sctk_rail_info_t* rail, int ib_rail_nb);

int sctk_network_poll_recv_ibuf(const sctk_rail_info_t const* rail, sctk_ibuf_t *ibuf,
       const char from_cp, struct sctk_ib_polling_s* poll);
int sctk_network_poll_send_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
       const char from_cp, struct sctk_ib_polling_s* poll);
int sctk_network_poll_all (sctk_rail_info_t* rail, struct sctk_ib_polling_s* poll);

#ifdef __cplusplus
}
#endif

#endif

#endif

