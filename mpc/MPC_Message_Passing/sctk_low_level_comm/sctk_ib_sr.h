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
#ifndef __SCTK__IB_POLLING_H_
#define __SCTK__IB_POLLING_H_

#define SCTK_IB_MODULE_NAME "POLLING"
#include <infiniband/verbs.h>
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "utlist.h"

typedef struct sctk_ib_sr_s {
  union {
    struct {
      size_t payload_size;
    } eager;
    struct {

    } buffered;
    struct {

    } rdma;
  };
} sctk_ib_sr_t;

#define IBUF_SR_HEADER(ibuf) ((sctk_ib_sr_t*) ibuf)
#define IBUF_MSG_HEADER(ibuf) ((char*) ibuf + sizeof(sctk_ib_sr_t))
#define IBUF_MSG_PAYLOAD(ibuf) ((char*) ibuf + sizeof(sctk_ib_sr_t) + sizeof(sctk_thread_ptp_message_body_t))

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ibuf_t* sctk_ib_sr_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* route_data, sctk_thread_ptp_message_t * msg);

void sctk_ib_sr_free_msg_no_recopy(sctk_thread_ptp_message_t * msg);

void sctk_ib_sr_recv_msg_no_recopy(sctk_message_to_copy_t* tmp);

#endif
#endif
