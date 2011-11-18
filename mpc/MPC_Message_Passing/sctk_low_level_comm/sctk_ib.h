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

#ifndef __SCTK_IB_H_
#define __SCTK_IB_H_
#ifdef __cplusplus
extern "C"
{
#endif

  struct sctk_ibuf_pool_s;
  struct sctk_ib_mmu_s;
  struct sctk_ib_config_s;
  struct sctk_ib_device_s;

  typedef struct sctk_ib_rail_info_s {
    struct sctk_ibuf_pool_s *pool_buffers;
    struct sctk_ib_mmu_s    *mmu;
    struct sctk_ib_config_s *config;
    struct sctk_ib_device_s *device;
  } sctk_ib_rail_info_t;

  typedef struct sctk_ib_data_s {

  } sctk_ib_data_t;

#include <sctk_route.h>

  void sctk_network_init_ib_all(sctk_rail_info_t* rail,
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*));
#ifdef __cplusplus
}
#endif
#endif
