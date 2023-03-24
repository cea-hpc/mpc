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

#ifndef __SCTK__IB_RDMA_H_
#define __SCTK__IB_RDMA_H_

#include <infiniband/verbs.h>
#include "ib.h"
#include "ibconfig.h"
#include "ibufs.h"
#include "qp.h"
#include "mpc_launch_pmi.h"
#include "utlist.h"
#include "rail.h"

/* XXX: Packed structures */
typedef enum _mpc_lowcomm_ib_rdma_type_e
{
	MPC_LOWCOMM_IB_RDMA_RDV_REQ_TYPE = 1,
	MPC_LOWCOMM_IB_RDMA_RDV_ACK_TYPE,
	MPC_LOWCOMM_IB_RDMA_RDV_DONE_TYPE,
	MPC_LOWCOMM_IB_RDMA_RDV_WRITE_TYPE,
	MPC_LOWCOMM_IB_RDMA_WRITE,
	MPC_LOWCOMM_IB_RDMA_READ,
	MPC_LOWCOMM_IB_RDMA_FETCH_AND_ADD,
	MPC_LOWCOMM_IB_RDMA_CAS
}
_mpc_lowcomm_ib_rdma_type_t;


/*
 * ACK
 */
typedef struct _mpc_lowcomm_ib_rdma_ack_s
{
	void *                     addr;
	size_t                     size;
	uint32_t                   rkey;
	mpc_lowcomm_ptp_message_t *src_msg_header;
	mpc_lowcomm_ptp_message_t *dest_msg_header;
	int                        ht_key;
}_mpc_lowcomm_ib_rdma_ack_t;

/*
 * REQ
 */
typedef struct _mpc_lowcomm_ib_rdma_req_s
{
	/* MPC header of msg */
	mpc_lowcomm_ptp_message_body_t msg_header;
	/* remote MPC header */
	mpc_lowcomm_ptp_message_t *    dest_msg_header;
	size_t                         requested_size;
	mpc_lowcomm_ptp_message_type_t message_type;
	int                            source;
	int                            source_task;
	int                            remote_rail;
}_mpc_lowcomm_ib_rdma_req_t;

/*
 * DATA READ
 */
typedef struct _mpc_lowcomm_ib_rdma_data_read_s
{
	mpc_lowcomm_ptp_message_t *dest_msg_header;
}_mpc_lowcomm_ib_rdma_data_read_t;

/*
 * DATA WRITE
 */
typedef struct _mpc_lowcomm_ib_rdma_data_write_s
{
	mpc_lowcomm_ptp_message_t *src_msg_header;
}_mpc_lowcomm_ib_rdma_data_write_t;


/*
 * DONE
 */
typedef struct _mpc_lowcomm_ib_rdma_done_s
{
	mpc_lowcomm_ptp_message_t *dest_msg_header;
}_mpc_lowcomm_ib_rdma_done_t;


typedef struct _mpc_lowcomm_ib_rdma_header_s
{
	size_t              payload_size;
	_mpc_lowcomm_ib_rdma_type_t type;
	void *              msg_pointer;
} _mpc_lowcomm_ib_rdma_header_t;

typedef struct _mpc_lowcomm_ib_rdma_s
{
	_mpc_lowcomm_ib_ibuf_header_t ibuf;
	_mpc_lowcomm_ib_rdma_header_t rdma_header;

	union
	{
		_mpc_lowcomm_ib_rdma_ack_t        ack;
		_mpc_lowcomm_ib_rdma_req_t        req;
		_mpc_lowcomm_ib_rdma_data_read_t  read;
		_mpc_lowcomm_ib_rdma_data_write_t write;
		_mpc_lowcomm_ib_rdma_done_t       done;
	};
}_mpc_lowcomm_ib_rdma_t;


/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/


/* Rendez-vous */

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_rdma_rendezvous_prepare_req(sctk_rail_info_t *rail,
                                                            _mpc_lowcomm_ib_qp_t *remote, mpc_lowcomm_ptp_message_t *msg, size_t size);

void _mpc_lowcomm_ib_rdma_rendezvous_prepare_send_msg(mpc_lowcomm_ptp_message_t *msg, size_t size);

/* RDMA Read-Write */

void _mpc_lowcomm_ib_rdma_write(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                        void *src_addr, struct sctk_rail_pin_ctx_list *local_key,
                        void *dest_addr, struct sctk_rail_pin_ctx_list *remote_key,
                        size_t size);

void _mpc_lowcomm_ib_rdma_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                       void *src_addr, struct  sctk_rail_pin_ctx_list *remote_key,
                       void *dest_addr, struct  sctk_rail_pin_ctx_list *local_key,
                       size_t size);

/* RDMA Atomics */
int _mpc_lowcomm_ib_rdma_fetch_and_op_gate(sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type);

void _mpc_lowcomm_ib_rdma_fetch_and_op(sctk_rail_info_t *rail,
                               mpc_lowcomm_ptp_message_t *msg,
                               void *fetch_addr,
                               struct  sctk_rail_pin_ctx_list *local_key,
                               void *remote_addr,
                               struct  sctk_rail_pin_ctx_list *remote_key,
                               void *add,
                               RDMA_op op,
                               RDMA_type type);

void _mpc_lowcomm_ib_rdma_cas(sctk_rail_info_t *rail,
                      mpc_lowcomm_ptp_message_t *msg,
                      void *res_addr,
                      struct  sctk_rail_pin_ctx_list *local_key,
                      void *remote_addr,
                      struct  sctk_rail_pin_ctx_list *remote_key,
                      void *comp,
                      void *new,
                      RDMA_type type);

int _mpc_lowcomm_ib_rdma_cas_gate(sctk_rail_info_t *rail, size_t size, RDMA_type type);


int
_mpc_lowcomm_ib_rdma_poll_recv(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf);

int
_mpc_lowcomm_ib_rdma_poll_send(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf);


void _mpc_lowcomm_ib_rdma_print(mpc_lowcomm_ptp_message_t *msg);

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_rdma_eager_prepare_msg(_mpc_lowcomm_ib_rail_info_t *rail_ib,
                                                       _mpc_lowcomm_ib_qp_t *remote, mpc_lowcomm_ptp_message_t *msg, size_t size);

mpc_lowcomm_ptp_message_t *_mpc_lowcomm_ib_rdma_recv_done_remote_imm(sctk_rail_info_t *rail, int imm_data);

#endif
