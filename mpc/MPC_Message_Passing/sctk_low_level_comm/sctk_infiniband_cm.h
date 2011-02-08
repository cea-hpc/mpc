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

#ifndef __SCTK__INFINIBAND_CM_H_
#define __SCTK__INFINIBAND_CM_H_

#include "sctk_infiniband_qp.h"
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_allocator.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

  void
sctk_net_ibv_cm_server();

  void
sctk_net_ibv_cm_request(int process,
    sctk_net_ibv_qp_remote_t *remote, char* host, int* port);

  void
sctk_net_ibv_cm_client(char* host, int port,
    int dest, sctk_net_ibv_qp_remote_t *remote);

#endif
