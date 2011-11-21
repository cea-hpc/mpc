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
#include "sctk_ib_sr.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_net_tools.h"

#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ibuf_t* sctk_ib_sr_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* route_data, sctk_thread_ptp_message_t * msg) {
  size_t size;
  sctk_ibuf_t *ibuf;
  sctk_ib_sr_t *sr_header;

  ibuf = sctk_ibuf_pick(rail_ib, 1, 0);
  sctk_nodebug("Picked buffer %p", ibuf);

  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);
  /* Copy header */
  memcpy(IBUF_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_MSG_PAYLOAD(ibuf->buffer));

  /* Initialization of the buffer */
  sctk_ibuf_send_init(ibuf, size);

  sr_header = IBUF_SR_HEADER(ibuf->buffer);
  sr_header->eager.payload_size = size;

  return ibuf;
}

void sctk_ib_sr_free_msg_no_recopy(sctk_thread_ptp_message_t * msg) {
  sctk_ibuf_t *ibuf = NULL;

  /* Assume msg not recopies */
  assume(!msg->tail.ib.eager.recopied);
  ibuf = msg->tail.ib.eager.ibuf;
  sctk_ibuf_release(ibuf->region->rail, ibuf, 1);
}


void sctk_ib_sr_recv_msg_no_recopy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  int recopy;
  sctk_ibuf_t *ibuf;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  /* Assume msg not recopies */
  assume(!send->tail.ib.eager.recopied);

  ibuf = send->tail.ib.eager.ibuf;
  assume(ibuf);
  sctk_net_message_copy_from_buffer(tmp, IBUF_MSG_PAYLOAD(ibuf->buffer));
}

#endif
