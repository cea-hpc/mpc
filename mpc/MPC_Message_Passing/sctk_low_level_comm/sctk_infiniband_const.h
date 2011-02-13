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

#ifndef __SCTK__INFINIBAND_CONST_H_
#define __SCTK__INFINIBAND_CONST_H_

typedef enum
{
  IBV_CHAN_RC_SR    = 1 << 0,
  IBV_CHAN_RC_RDMA  = 1 << 1,
  IBV_CHAN_SEND  = 1 << 2,
  IBV_CHAN_RECV  = 1 << 3,
} sctk_net_ibv_allocator_type_t;

/* max number of WC extracted for the input
 * and the output completion queue */
/* FIXME cant change 1 to something else */
#define SCTK_PENDING_IN_NUMBER 1
#define SCTK_PENDING_OUT_NUMBER 10

#endif
