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
#ifndef __SCTK__IB_RDMA_H_
#define __SCTK__IB_RDMA_H_

#include <infiniband/verbs.h>
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "sctk_route.h"
#include "utlist.h"

/* XXX: Packed structures */
typedef enum sctk_ib_rdma_type_e {
  rdma_req_type = 111,
  rdma_ack_type = 222,
  rdma_done_type = 333,
  rdma_data_read_type = 444,
  rdma_data_write_type = 555,
} __attribute__ ((packed))
sctk_ib_rdma_type_t;

typedef struct sctk_ib_rdma_s {
  size_t payload_size;
  sctk_ib_rdma_type_t type;
} __attribute__ ((packed))
sctk_ib_rdma_t;

/*
 * ACK
 */
typedef struct sctk_ib_rdma_ack_s {
  void* addr;
  size_t size;
  uint32_t  rkey;
  sctk_thread_ptp_message_t* src_msg_header;
  sctk_thread_ptp_message_t* dest_msg_header;
} __attribute__ ((packed))
sctk_ib_rdma_ack_t;

/*
 * REQ
 */
typedef struct sctk_ib_rdma_req_s {
  /* MPC header of msg */
  sctk_thread_ptp_message_body_t msg_header;
  /* remote MPC header */
  sctk_thread_ptp_message_t* dest_msg_header;
  size_t requested_size;
  sctk_message_type_t message_type;
} __attribute__ ((packed))
sctk_ib_rdma_req_t;

/*
 * DATA READ
 */
typedef struct sctk_ib_rdma_data_read_s {
  sctk_thread_ptp_message_t* dest_msg_header;
} __attribute__ ((packed))
sctk_ib_rdma_data_read_t;

/*
 * DATA WRITE
 */
typedef struct sctk_ib_rdma_data_write_s {
  sctk_thread_ptp_message_t* src_msg_header;
} __attribute__ ((packed))
sctk_ib_rdma_data_write_t;


/*
 * DONE
 */
typedef struct sctk_ib_rdma_done_s {
  sctk_thread_ptp_message_t* dest_msg_header;
} __attribute__ ((packed))
sctk_ib_rdma_done_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

sctk_ibuf_t* sctk_ib_rdma_prepare_req(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, sctk_thread_ptp_message_t * msg, size_t size);

int
sctk_ib_rdma_poll_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf);

int
sctk_ib_rdma_poll_send(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf);

void sctk_ib_rdma_prepare_send_msg (sctk_ib_rail_info_t* rail_ib,
    sctk_thread_ptp_message_t * msg, size_t size);
void sctk_ib_rdma_print(sctk_thread_ptp_message_t* msg);
#endif
#endif
