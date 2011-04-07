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
#include "sctk_hybrid_comm.h"
#include "sctk_ib_allocator.h"

#ifndef __SCTK__INFINIBAND_LIB_H_
#define __SCTK__INFINIBAND_LIB_H_

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

UNUSED static void
sctk_net_ibv_send_msg_to_mpc(sctk_thread_ptp_message_t* msg_header, void* msg, int src_process,
    int origin, void* ptr) {

  sctk_net_ibv_allocator->entry[src_process].nb_ptp_msg_received++;

  msg_header->net_mesg = msg;
  msg_header->channel_type = origin;
  msg_header->struct_ptr = ptr;
  sctk_nodebug("\t\t\t\tSent message to mpc (struct_ptr : %p)", msg_header->struct_ptr);
  sctk_send_message (msg_header);

}

#define max(a,b) \
  ({ typeof (a) _a = (a); \
   typeof (b) _b = (b); \
   _a > _b ? _a : _b; })

#endif
#endif
