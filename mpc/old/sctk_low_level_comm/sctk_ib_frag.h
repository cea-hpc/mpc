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

#ifndef __SCTK__INFINIBAND_FRAG_H_
#define __SCTK__INFINIBAND_FRAG_H_

#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include "sctk_ib.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_const.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_scheduling.h"
#include "sctk_ib_coll.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <infiniband/verbs.h>

void
sctk_net_ibv_comp_rc_sr_send_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req);

sctk_net_ibv_msg_entry_t* sctk_net_ibv_frag_allocate(
    sctk_net_ibv_ibuf_header_t* msg_header);

  int
sctk_net_ibv_frag_copy_msg(sctk_net_ibv_ibuf_header_t* msg_header,
    struct sctk_list_header* list, sctk_net_ibv_msg_entry_t* entry);

void
sctk_net_ibv_frag_free_msg(sctk_net_ibv_msg_entry_t* entry);

  sctk_net_ibv_msg_entry_t*
sctk_net_ibv_frag_search_msg(sctk_net_ibv_ibuf_header_t* msg_header, struct sctk_list_header *list);

  void
sctk_net_ibv_comp_rc_sr_frag_recv(sctk_net_ibv_ibuf_header_t* msg_header);

void
sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req);


#endif
#endif
