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

#ifndef __SCTK__INFINIBAND_CONFIG_H_
#define __SCTK__INFINIBAND_CONFIG_H_

extern int  ibv_eager_threshold;
extern int  ibv_qp_tx_depth;
extern int  ibv_qp_rx_depth;
extern int  ibv_max_sg_sq;
extern int  ibv_max_sg_rq;
extern int  ibv_max_inline;
extern int  ibv_max_ibufs;
extern int  ibv_max_srq_ibufs;
extern int  ibv_srq_credit_limit;
extern int  ibv_size_ibufs_chunk;;

extern int  ibv_rdvz_protocol;
#define IBV_RDVZ_WRITE_PROTOCOL (1)
#define IBV_RDVZ_READ_PROTOCOL (2)

extern int  ibv_verbose_level;
extern int  ibv_wc_in_number;
extern int  ibv_wc_out_number;
extern int  ibv_max_mr;
extern int  ibv_adm_port;
extern int  ibv_rdma_depth;
extern int  ibv_rdma_dest_depth;
extern int  ibv_no_memory_limitation;

#endif
