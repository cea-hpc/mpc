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
#ifndef __SCTK__IB_EAGER_H_
#define __SCTK__IB_EAGER_H_

#include <infiniband/verbs.h>
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "utlist.h"

struct sctk_rail_info_s;

typedef struct sctk_ib_eager_s {
  size_t payload_size;
} __attribute__ ((packed))
 sctk_ib_eager_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ibuf_t* sctk_ib_eager_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* route_data, sctk_thread_ptp_message_t * msg, size_t size, int low_memory_mode, char is_control_message);

void sctk_ib_eager_free_msg_no_recopy(void* arg);

void sctk_ib_eager_recv_msg_no_recopy(sctk_message_to_copy_t* tmp);

void
sctk_ib_eager_recv_free(struct sctk_rail_info_s* rail, sctk_thread_ptp_message_t *msg,
    sctk_ibuf_t *ibuf, int recopy);

void sctk_ib_buffered_poll_recv(struct sctk_rail_info_s* rail, sctk_ibuf_t *ibuf);

void sctk_ib_eager_init(struct sctk_ib_rail_info_s* rail_ib);
void
sctk_ib_eager_poll_recv(struct sctk_rail_info_s* rail, sctk_ibuf_t *ibuf);

#endif
#endif
