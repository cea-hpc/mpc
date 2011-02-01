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

#ifndef __SCTK__INFINIBAND_SCHEDULING_H_
#define __SCTK__INFINIBAND_SCHEDULING_H_

#include <sctk.h>
#include <stdint.h>
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_comp_rc_rdma.h"

void sctk_net_ibv_sched_init();

int
sctk_net_ibv_sched_psn_inc (int dest);

int
sctk_net_ibv_sched_esn_inc (int dest);

int
sctk_net_ibv_sched_get_esn(int dest);

  int
sctk_net_ibv_sched_sn_check(int dest, uint64_t num);

int
sctk_net_ibv_sched_sn_check_and_inc(int dest, uint64_t num);

int
  sctk_net_ibv_sched_rc_sr_free_pending_msg(sctk_thread_ptp_message_t * item );
#endif
