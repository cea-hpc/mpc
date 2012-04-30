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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */
#ifdef MPC_USE_INFINIBAND

#ifndef __SCTK__IB_CONFIG_H_
#define __SCTK__IB_CONFIG_H_

#include "infiniband/verbs.h"

#define IBV_RDVZ_WRITE_PROTOCOL (1)
#define IBV_RDVZ_READ_PROTOCOL (2)
typedef struct sctk_ib_config_s
{
  /* Network name */
  char *network_name;
  /* MMU */
  unsigned int  ibv_size_mr_chunk;
  unsigned int  ibv_mmu_cache_enabled;
  unsigned int  ibv_mmu_cache_entries;
  /* IBUFS */
  unsigned int  ibv_init_ibufs;

  unsigned int  ibv_eager_limit;
  unsigned int  ibv_eager_rdma_limit;
  unsigned int  ibv_frag_eager_limit;
  unsigned int  ibv_max_rdma_ibufs;
  unsigned int  ibv_qp_tx_depth;
  unsigned int  ibv_qp_rx_depth;
  unsigned int  ibv_cq_depth;
  unsigned int  ibv_max_sg_sq;
  unsigned int  ibv_max_sg_rq;
  unsigned int  ibv_max_inline;
  unsigned int  ibv_max_srq_ibufs_posted;
  unsigned int  ibv_max_srq_ibufs;
  unsigned int  ibv_srq_credit_limit;
  unsigned int  ibv_srq_credit_thread_limit;
  unsigned int  ibv_max_srq_wr_handle_by_thread;
  unsigned int  ibv_size_ibufs_chunk;;
  unsigned int  ibv_rdvz_protocol;

  unsigned int  ibv_verbose_level;
  unsigned int  ibv_wc_in_number;
  unsigned int  ibv_wc_out_number;
  unsigned int  ibv_init_mr;
  unsigned int  ibv_adm_port;
  unsigned int  ibv_rdma_depth;
  unsigned int  ibv_rdma_dest_depth;
  unsigned int  ibv_no_memory_limitation;
  unsigned int  ibv_adaptive_polling;
  unsigned int  ibv_secure_polling;
  unsigned int  ibv_steal;
  unsigned int  ibv_low_memory;
  unsigned int  ibv_quiet_crash;
  unsigned int  ibv_match;

  /* DEVICE */
  struct ibv_device_attr *device_attr;
} sctk_ib_config_t;

struct sctk_ib_rail_info_s;
void sctk_ib_config_init(struct sctk_ib_rail_info_s *rail_ib, char *network_name);
void sctk_ib_config_print(struct sctk_ib_rail_info_s *rail_ib);


#endif
#endif
