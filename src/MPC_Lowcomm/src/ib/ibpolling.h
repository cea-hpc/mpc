/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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

#ifndef __SCTK__IB_POLLING_H_
#define __SCTK__IB_POLLING_H_

#include <infiniband/verbs.h>
#include <mpc_common_rank.h>

#include "ib.h"
#include "ibconfig.h"
#include "qp.h"
#include "utlist.h"

#include "mpc_conf.h"

typedef struct sctk_ib_polling_s
{
	int recv_own;
	int recv_other;
	int recv_cq;
	int recv;
} sctk_ib_polling_t;

#define  POLL_CQ_BUSY       -1
#define  POLL_CQ_SKIPPED    -2

#define HOSTNAME            2048
#define POLL_INIT(x)              do { (x)->recv_other = 0; \
		                       (x)->recv_own   = 0; \
		                       (x)->recv       = 0; \
		                       (x)->recv_cq    = 0; } while(0)

#define POLL_RECV_OWN(x)          do { (x)->recv_own++; \
		                       (x)->recv++; } while(0)

#define POLL_RECV_OTHER(x)        do { (x)->recv_other++; \
		                       (x)->recv++; } while(0)

#define POLL_RECV_CQ(x)           do { (x)->recv_cq++; \
		                       (x)->recv++; } while(0)

#define POLL_RECV_CQ(x)           do { (x)->recv_cq++; \
		                       (x)->recv++; } while(0)

#define POLL_GET_RECV(x)          ( (x)->recv)

#define POLL_GET_RECV_CQ(x)       ( (x)->recv_cq)
#define POLL_SET_RECV_CQ(x, y)    ( (x)->recv_cq = y)


#endif
