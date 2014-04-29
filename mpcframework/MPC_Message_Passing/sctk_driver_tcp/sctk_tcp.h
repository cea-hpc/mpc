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

#ifndef __SCTK_TCP_H_
#define __SCTK_TCP_H_
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __SCTK_ROUTE_H_
#error "sctk_route must be included before sctk_tcp.h"
#endif

#include <sctk_spinlock.h>

#define MAX_STRING_SIZE 2048
  typedef struct {
    sctk_spinlock_t lock;
    int fd;
  }sctk_tcp_data_t;
  
  typedef struct {
    int sctk_use_tcp_o_ib;
    int sockfd;
    int portno;
    int rail;
    char connection_infos[MAX_STRING_SIZE];
    size_t connection_infos_size;
    void* (*tcp_thread)(struct sctk_route_table_s*);
  }sctk_tcp_rail_info_t;

  void sctk_network_init_tcp(sctk_rail_info_t* rail,int sctk_use_tcp_o_ib);

#ifdef __cplusplus
}
#endif
#endif
