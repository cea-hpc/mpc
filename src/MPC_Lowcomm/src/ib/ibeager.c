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

#include <sctk_alloc.h>
#include <utlist.h>

#include "ibeager.h"
#include "sctk_net_tools.h"
#include "sctk_rail.h"
/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/

/* Number of buffered MPC headers. With the eager protocol, we
 * need to proceed to an dynamic allocation to copy the MPC header with
 * its tail. The body is transfered by the network */
#define EAGER_BUFFER_SIZE    512

/**
 * @brief Initialize eager protocol bufferred message headers
 *
 * @param rail_ib The Infiniband rail
 */
void _mpc_lowcomm_ib_eager_init(_mpc_lowcomm_ib_rail_info_t *rail_ib)
{
	int i;

	rail_ib->eager_buffered_ptp_message = NULL;
	mpc_common_spinlock_init(&rail_ib->eager_lock_buffered_ptp_message, 0);

	mpc_lowcomm_ptp_message_t *tmp = sctk_calloc(EAGER_BUFFER_SIZE, sizeof(mpc_lowcomm_ptp_message_t) );
	assume(tmp != NULL);

	for(i = 0; i < EAGER_BUFFER_SIZE; ++i)
	{
		mpc_lowcomm_ptp_message_t *entry = &tmp[i];
		entry->from_buffered = 1;
		/* Add the entry to the list */
		LL_PREPEND(rail_ib->eager_buffered_ptp_message, entry);
	}
	rail_ib->eager_buffered_start_addr = tmp;
}

/**
 * @brief Release  eager protocol bufferred message headers
 *
 * @param rail_ib The Infiniband rail
 */
void _mpc_lowcomm_ib_eager_finalize(_mpc_lowcomm_ib_rail_info_t *rail_ib)
{
	mpc_lowcomm_ptp_message_t *elt, *tmp;

	/* Do not free blocks from the initial allocated segment */
	void *buff_start = rail_ib->eager_buffered_start_addr;
	void *buff_end   = buff_start + (EAGER_BUFFER_SIZE * sizeof(mpc_lowcomm_ptp_message_t) );

	mpc_common_spinlock_lock(&rail_ib->eager_lock_buffered_ptp_message);

	LL_FOREACH_SAFE(rail_ib->eager_buffered_ptp_message, elt, tmp)
	{
		/* if the ptp_message has been dynamically allocated */
		if( ( (void *)elt < buff_end) && (buff_start <= (void *)elt) )
		{
			/* This element is part of the initiallly
			 * allocated buffered message segment no free */
		}
		else
		{
			/* This messages was allocated after
			 *  and then added to the list */
			sctk_free(elt);
		}

		LL_DELETE(rail_ib->eager_buffered_ptp_message, elt);
	}

	mpc_common_spinlock_unlock(&rail_ib->eager_lock_buffered_ptp_message);

	/* free the static segment just once */
	sctk_free(rail_ib->eager_buffered_start_addr);

	rail_ib->eager_buffered_start_addr  = NULL;
	rail_ib->eager_buffered_ptp_message = NULL;
}

/**
 * @brief Pick a buffered message header and if none found allocate a new on
 *
 * @param rail The Inbfiniband rail context
 * @return mpc_lowcomm_ptp_message_t* a message header which is now in buffered list
 */
static inline mpc_lowcomm_ptp_message_t *__pick_buffered_ptp_msg(sctk_rail_info_t *rail)
{
	_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;

	mpc_lowcomm_ptp_message_t *tmp = NULL;

	mpc_common_spinlock_lock(&rail_ib->eager_lock_buffered_ptp_message);

	if(rail_ib->eager_buffered_ptp_message)
	{
		tmp = rail_ib->eager_buffered_ptp_message;
		LL_DELETE(rail_ib->eager_buffered_ptp_message, rail_ib->eager_buffered_ptp_message);
	}

	mpc_common_spinlock_unlock(&rail_ib->eager_lock_buffered_ptp_message);

	/* If no more entries are available in the buffered list, we allocate */
	if(tmp == NULL)
	{
		tmp = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) );
		/* This header must be freed after use */
		tmp->from_buffered = 0;
	}

	return tmp;
}

/**
 * @brief Release an MPC header according to its origin (buffered or allocated)
 *
 * @param rail The Inbfiniband rail context
 * @param msg The message header to be relased
 */
static inline void __release_buffered_ptp_msg(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg)
{
	if(msg->from_buffered)
	{
		_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;

		mpc_common_spinlock_lock(&rail_ib->eager_lock_buffered_ptp_message);
		LL_PREPEND(rail_ib->eager_buffered_ptp_message, msg);
		mpc_common_spinlock_unlock(&rail_ib->eager_lock_buffered_ptp_message);
	}
	else
	{
		/* We can simply free the buffer because it was malloc'ed :-) */
		sctk_free(msg);
	}
}

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_eager_prepare_msg(_mpc_lowcomm_ib_rail_info_t *rail_ib,
                                                          _mpc_lowcomm_ib_qp_t *remote,
                                                          mpc_lowcomm_ptp_message_t *msg,
                                                          size_t size,
                                                          char is_control_message)
{
	_mpc_lowcomm_ib_ibuf_t *ibuf = NULL;

	size_t ibuf_size = size + IBUF_GET_EAGER_SIZE;

	if(is_control_message)
	{
		ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(rail_ib);
		_mpc_lowcomm_ib_ibuf_prepare(remote, ibuf, ibuf_size);

	}
	else
	{
		ibuf = _mpc_lowcomm_ib_ibuf_pick_send(rail_ib, remote, &ibuf_size);
	}

	/* We cannot pick an ibuf. We should try the buffered eager protocol */
	if(ibuf == NULL)
	{
		return NULL;
	}

	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );
	IBUF_SET_POISON(ibuf->buffer);

	_mpc_lowcomm_ib_eager_t *eager_msg = (_mpc_lowcomm_ib_eager_t *)ibuf->buffer;

	/* Copy header */
	memcpy(&eager_msg->msg, msg, sizeof(mpc_lowcomm_ptp_message_body_t) );
	eager_msg->eager_header.payload_size = size - sizeof(mpc_lowcomm_ptp_message_body_t);
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_EAGER_PROTOCOL);

	/* Copy payload */
	sctk_net_copy_in_buffer(msg, eager_msg->payload);

	return ibuf;
}

/*-----------------------------------------------------------
*  Internal free functions
*----------------------------------------------------------*/
static void __free_no_recopy(void *arg)
{
	mpc_lowcomm_ptp_message_t *msg  = (mpc_lowcomm_ptp_message_t *)arg;
	_mpc_lowcomm_ib_ibuf_t *   ibuf = NULL;

	/* Assume msg not recopies */
	ib_assume(!msg->tail.ib.eager.recopied);
	ibuf = msg->tail.ib.eager.ibuf;

	_mpc_lowcomm_ib_ibuf_release(ibuf->region->rail, ibuf, 0);
	__release_buffered_ptp_msg(ibuf->region->rail->rail, msg);
}

static void __free_with_recopy(void *arg)
{
	mpc_lowcomm_ptp_message_t *msg  = (mpc_lowcomm_ptp_message_t *)arg;
	_mpc_lowcomm_ib_ibuf_t *   ibuf = NULL;

	ibuf = msg->tail.ib.eager.ibuf;
	__release_buffered_ptp_msg(ibuf->region->rail->rail, msg);
}

/*-----------------------------------------------------------
*  Ibuf free function
*----------------------------------------------------------*/


static void __eager_recv_msg_no_recopy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	mpc_lowcomm_ptp_message_t *send = tmp->msg_send;

	/* Assume msg not recopied */
	ib_assume(!send->tail.ib.eager.recopied);

	_mpc_lowcomm_ib_ibuf_t *ibuf = send->tail.ib.eager.ibuf;
	ib_assume(ibuf);

	_mpc_lowcomm_ib_eager_t *eager_msg = (_mpc_lowcomm_ib_eager_t *)ibuf->buffer;
	void *body = eager_msg->payload;
	sctk_net_message_copy_from_buffer(body, tmp, 1);
}

static inline void __eager_recv_free(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_ib_ibuf_t *ibuf, int recopy)
{
	/* Read from recopied buffer */
	if(recopy)
	{
		_mpc_comm_ptp_message_set_copy_and_free(msg, __free_with_recopy, sctk_net_message_copy);
		_mpc_lowcomm_ib_ibuf_release(&rail->network.ib, ibuf, 0);
		/* Read from network buffer  */
	}
	else
	{
		_mpc_comm_ptp_message_set_copy_and_free(msg, __free_no_recopy, __eager_recv_msg_no_recopy);
	}
}

static inline mpc_lowcomm_ptp_message_t *__eager_recv(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf, int recopy, _mpc_lowcomm_ib_protocol_t protocol)
{
	mpc_lowcomm_ptp_message_t *msg = NULL;

	IBUF_CHECK_POISON(ibuf->buffer);
	/* XXX: select if a recopy is needed for the message */
	/* XXX: recopy is not compatible with CM */

	_mpc_lowcomm_ib_eager_t *eager_message = (_mpc_lowcomm_ib_eager_t *)ibuf->buffer;

	/* If recopy required */
	if(recopy)
	{
		size_t size = eager_message->eager_header.payload_size;
		msg = sctk_malloc(size + sizeof(mpc_lowcomm_ptp_message_t) );
		ib_assume(msg);

		void *body = (char *)msg + sizeof(mpc_lowcomm_ptp_message_t);

		/* Copy the header of the message */
		memcpy(msg, &eager_message->msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

		/* Copy the body of the message */
		memcpy(body, eager_message->payload, size);
	}
	else
	{
		msg = __pick_buffered_ptp_msg(rail);
		ib_assume(msg);

		/* Copy the header of the message */
		memcpy(msg, &eager_message->msg, sizeof(mpc_lowcomm_ptp_message_body_t) );
	}

	SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);

	msg->tail.message_type      = MPC_LOWCOMM_MESSAGE_NETWORK;
	msg->tail.ib.eager.recopied = recopy;
	msg->tail.ib.eager.ibuf     = ibuf;
	msg->tail.ib.protocol       = protocol;

	_mpc_comm_ptp_message_clear_request(msg);
	return msg;
}


int _mpc_lowcomm_ib_eager_poll_recv(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	IBUF_CHECK_POISON(ibuf->buffer);

	_mpc_lowcomm_ib_eager_t *eager_msg = (_mpc_lowcomm_ib_eager_t *)ibuf->buffer;

	mpc_lowcomm_ptp_message_body_t * msg_ibuf = &eager_msg->msg;
	const _mpc_lowcomm_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);

	int recopy = 0;

	mpc_lowcomm_ptp_message_t *msg = NULL;

	if(_mpc_comm_ptp_message_is_for_process(msg_ibuf->header.message_type) )
	{
		recopy = 1;
	}

	mpc_common_nodebug("Received IBUF %p %d", ibuf, IBUF_GET_CHANNEL(ibuf) );

	/* Normal message: we handle it */
	msg = __eager_recv(rail, ibuf, recopy, protocol);
	__eager_recv_free(rail, msg, ibuf, recopy);

	return rail->send_message_from_network(msg);
}
