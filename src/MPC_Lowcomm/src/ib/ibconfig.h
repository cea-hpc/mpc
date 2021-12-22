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

#ifndef __SCTK__IB_CONFIG_H_
#define __SCTK__IB_CONFIG_H_

#include "infiniband/verbs.h"

/* Config Struct For Reference

typedef struct sctk_ib_config_s
{
  / Network name /
  char *network_name;
  / MMU /
  unsigned int  size_mr_chunk;
  unsigned int  mmu_cache_enabled;
  unsigned int  mmu_cache_entries;
  / IBUFS /
  unsigned int  init_ibufs;
  unsigned int  init_recv_ibufs;

  unsigned int  eager_limit;
  unsigned int  buffered_limit;
  unsigned int  max_rdma_connections;
  unsigned int  rdma_resizing;
  unsigned int  qp_tx_depth;
  unsigned int  qp_rx_depth;
  unsigned int  cq_depth;
  unsigned int  max_sg_sq;
  unsigned int  max_sg_rq;
  unsigned int  max_inline;
  unsigned int  max_srq_ibufs_posted;
  unsigned int  max_srq_ibufs;
  unsigned int  srq_credit_thread_limit;
  unsigned int  max_srq_wr_handle_by_thread;
  unsigned int  size_ibufs_chunk;;
  unsigned int  size_recv_ibufs_chunk;
  unsigned int  rdvz_protocol;

  unsigned int  verbose_level;
  unsigned int  wc_in_number;
  unsigned int  wc_out_number;
  unsigned int  adm_port;
  unsigned int  rdma_depth;
  unsigned int  rdma_depth;
  unsigned int  no_memory_limitation;
  unsigned int  adaptive_polling;
  unsigned int  secure_polling;
  unsigned int  steal;
  unsigned int  quiet_crash;
  unsigned int  match;
  unsigned int  async_thread;

  / For RDMA /
  unsigned int  rdma_min_size;
  unsigned int  rdma_max_size;
  unsigned int  rdma_min_nb;
  unsigned int  rdma_max_nb;

  unsigned int  rdma_resizing_min_size;
  unsigned int  rdma_resizing_max_size;
  unsigned int  rdma_resizing_min_nb;
  unsigned int  rdma_resizing_max_nb;

  / DEVICE /
  struct ibv_device_attr *device_attr;
} sctk_ib_config_t;

*/

struct sctk_ib_rail_info_s;

void sctk_ib_config_init ( struct sctk_ib_rail_info_s *rail_ib, char *network_name );
void sctk_ib_config_print ( struct sctk_ib_rail_info_s *rail_ib );

#endif
