/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
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
typedef enum sctk_ib_rdma_type_e
{
    SCTK_IB_RDMA_RDV_REQ_TYPE = 111,
    SCTK_IB_RDMA_RDV_ACK_TYPE = 222,
    SCTK_IB_RDMA_RDV_DONE_TYPE = 333,
    SCTK_IB_RDMA_RDV_WRITE_TYPE = 555,
    SCTK_IB_RDMA_WRITE = 666,
    SCTK_IB_RDMA_READ = 777,
    SCTK_IB_RDMA_FETCH_AND_ADD = 888,
} __attribute__ ( ( packed ) )
sctk_ib_rdma_type_t;

typedef struct sctk_ib_rdma_s
{
	size_t payload_size;
	sctk_ib_rdma_type_t type;
	void * msg_pointer;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_t;

/*
 * ACK
 */
typedef struct sctk_ib_rdma_ack_s
{
	void *addr;
	size_t size;
	sctk_uint32_t  rkey;
	sctk_thread_ptp_message_t *src_msg_header;
	sctk_thread_ptp_message_t *dest_msg_header;
	int ht_key;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_ack_t;

/*
 * REQ
 */
typedef struct sctk_ib_rdma_req_s
{
	/* MPC header of msg */
	sctk_thread_ptp_message_body_t msg_header;
	/* remote MPC header */
	sctk_thread_ptp_message_t *dest_msg_header;
	size_t requested_size;
	sctk_message_type_t message_type;
	int source;
	int source_task;
	int remote_rail;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_req_t;

/*
 * DATA READ
 */
typedef struct sctk_ib_rdma_data_read_s
{
	sctk_thread_ptp_message_t *dest_msg_header;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_data_read_t;

/*
 * DATA WRITE
 */
typedef struct sctk_ib_rdma_data_write_s
{
	sctk_thread_ptp_message_t *src_msg_header;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_data_write_t;


/*
 * DONE
 */
typedef struct sctk_ib_rdma_done_s
{
	sctk_thread_ptp_message_t *dest_msg_header;
} __attribute__ ( ( packed ) )
sctk_ib_rdma_done_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
 
 
 /* Rendez-vous */

sctk_ibuf_t *sctk_ib_rdma_rendezvous_prepare_req ( sctk_rail_info_t *rail,
													sctk_ib_qp_t *remote, sctk_thread_ptp_message_t *msg, size_t size,
													int low_memory_mode );

void sctk_ib_rdma_rendezvous_prepare_send_msg ( sctk_ib_rail_info_t *rail_ib,
											    sctk_thread_ptp_message_t *msg, size_t size );

/* RDMA Read-Write */

void sctk_ib_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
                         void * dest_addr, struct sctk_rail_pin_ctx_list * remote_key,
                         size_t size );

void sctk_ib_rdma_read(   sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct  sctk_rail_pin_ctx_list * remote_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
                         size_t size );

/* RDMA Atomics */
int sctk_ib_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type );

void sctk_ib_rdma_fetch_and_op(   sctk_rail_info_t *rail,
								  sctk_thread_ptp_message_t *msg,
								  void * fetch_addr,
								  struct  sctk_rail_pin_ctx_list * local_key,
								  void * remote_addr,
								  struct  sctk_rail_pin_ctx_list * remote_key,
								  void * add,
								  RDMA_op op,
							      RDMA_type type );


int sctk_ib_rdma_swap_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type );


int
sctk_ib_rdma_poll_recv ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf );

int
sctk_ib_rdma_poll_send ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf );


void sctk_ib_rdma_print ( sctk_thread_ptp_message_t *msg );

sctk_ibuf_t *sctk_ib_rdma_eager_prepare_msg ( sctk_ib_rail_info_t *rail_ib,
                                              sctk_ib_qp_t *remote, sctk_thread_ptp_message_t *msg, size_t size );

sctk_thread_ptp_message_t * sctk_ib_rdma_recv_done_remote_imm ( sctk_rail_info_t *rail, int imm_data );
#endif
#endif
