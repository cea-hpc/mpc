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

#ifndef __SCTK__INFINIBAND_CONFIG_H_
#define __SCTK__INFINIBAND_CONFIG_H_

extern unsigned int  ibv_eager_limit;
extern unsigned int  ibv_frag_eager_limit;
extern unsigned int  ibv_qp_tx_depth;
extern unsigned int  ibv_qp_rx_depth;
extern unsigned int  ibv_cq_depth;
extern unsigned int  ibv_max_sg_sq;
extern unsigned int  ibv_max_sg_rq;
extern unsigned int  ibv_max_inline;
extern unsigned int  ibv_max_ibufs;
extern unsigned int  ibv_max_srq_ibufs;
extern unsigned int  ibv_srq_credit_limit;
extern unsigned int  ibv_srq_credit_thread_limit;
extern unsigned int  ibv_size_ibufs_chunk;;

extern unsigned int  ibv_rdvz_protocol;
#define IBV_RDVZ_WRITE_PROTOCOL (1)
#define IBV_RDVZ_READ_PROTOCOL (2)

extern unsigned int  ibv_verbose_level;
extern unsigned int  ibv_wc_in_number;
extern unsigned int  ibv_wc_out_number;
extern unsigned int  ibv_max_mr;
extern unsigned int  ibv_size_mr_chunk;
extern unsigned int  ibv_adm_port;
extern unsigned int  ibv_rdma_depth;
extern unsigned int  ibv_rdma_dest_depth;
extern unsigned int  ibv_no_memory_limitation;
extern unsigned int  ibv_adaptive_polling;

void sctk_net_ibv_config_init();
#endif
#endif
