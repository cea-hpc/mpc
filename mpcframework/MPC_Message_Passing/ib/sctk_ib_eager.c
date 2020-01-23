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
#include "sctk_route.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_net_tools.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_topology.h"
#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
/* Number of buffered MPC headers. With the eager protocol, we
 * need to proceed to an dynamic allocation to copy the MPC header with
 * its tail. The body is transfered by the network */
#define EAGER_BUFFER_SIZE 100

/*
 * Initialize the eager protocol
 */
void sctk_ib_eager_init ( sctk_ib_rail_info_t *rail_ib )
{
	int i;

	rail_ib->eager_buffered_ptp_message = NULL;
	rail_ib->eager_lock_buffered_ptp_message = SCTK_SPINLOCK_INITIALIZER;

	mpc_lowcomm_ptp_message_t *tmp = sctk_malloc ( EAGER_BUFFER_SIZE * sizeof ( mpc_lowcomm_ptp_message_t ) );

	for ( i = 0; i < EAGER_BUFFER_SIZE; ++i )
	{
		mpc_lowcomm_ptp_message_t *entry = &tmp[i];
		entry->from_buffered = 1;
		/* Add the entry to the list */
		LL_PREPEND ( rail_ib->eager_buffered_ptp_message, entry );
	}
	rail_ib->eager_buffered_start_addr = tmp;
}

void sctk_ib_eager_finalize(sctk_ib_rail_info_t *rail_ib)
{
	mpc_lowcomm_ptp_message_t *elt, *tmp;

	LL_FOREACH_SAFE(rail_ib->eager_buffered_ptp_message, elt, tmp)
	{
		/* if the ptp_message has been dynamically allocated */
		if(((unsigned long int)elt) > ((unsigned long int) (rail_ib->eager_buffered_start_addr + (EAGER_BUFFER_SIZE * sizeof( mpc_lowcomm_ptp_message_t)))))
			sctk_free(elt);

		LL_DELETE(rail_ib->eager_buffered_ptp_message, elt);
	}

	/* free the static segment just once */
	sctk_free(rail_ib->eager_buffered_start_addr);
	rail_ib->eager_buffered_start_addr = NULL;
	rail_ib->eager_buffered_ptp_message = NULL;
}

/*
 * Try to pick a buffered MPC header or
 * if not found, allocate one
 */
static inline mpc_lowcomm_ptp_message_t *sctk_ib_eager_pick_buffered_ptp_message ( sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	mpc_lowcomm_ptp_message_t *tmp = NULL;

	if ( rail_ib->eager_buffered_ptp_message )
	{
		mpc_common_spinlock_lock ( &rail_ib->eager_lock_buffered_ptp_message );

		if ( rail_ib->eager_buffered_ptp_message )
		{
			tmp = rail_ib->eager_buffered_ptp_message;
			LL_DELETE ( rail_ib->eager_buffered_ptp_message, rail_ib->eager_buffered_ptp_message );
		}

		mpc_common_spinlock_unlock ( &rail_ib->eager_lock_buffered_ptp_message );
	}

	/* If no more entries are available in the buffered list, we allocate */
	if ( tmp == NULL )
	{
		PROF_INC ( rail, ib_alloc_mem );
		tmp = sctk_malloc ( sizeof ( mpc_lowcomm_ptp_message_t ) );
		/* This header must be freed after use */
		tmp->from_buffered = 0;
	}

	return tmp;
}

/*
 * Release an MPC header according to its origin (buffered or allocated)
 */
void sctk_ib_eager_release_buffered_ptp_message ( sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg )
{
	if ( msg->from_buffered )
	{
		sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

		mpc_common_spinlock_lock ( &rail_ib->eager_lock_buffered_ptp_message );
		LL_PREPEND ( rail_ib->eager_buffered_ptp_message, msg );
		mpc_common_spinlock_unlock ( &rail_ib->eager_lock_buffered_ptp_message );
	}
	else
	{
		/* We can simply free the buffer because it was malloc'ed :-) */
		PROF_INC ( rail, ib_free_mem );
		sctk_free ( msg );
	}
}


sctk_ibuf_t *sctk_ib_eager_prepare_msg ( sctk_ib_rail_info_t *rail_ib,  sctk_ib_qp_t *remote, mpc_lowcomm_ptp_message_t *msg, size_t size, char is_control_message )
{
	sctk_ibuf_t *ibuf;
	sctk_ib_eager_t *eager_header;

	size_t ibuf_size = size + IBUF_GET_EAGER_SIZE;


	if ( is_control_message )
	{
		ibuf = sctk_ibuf_pick_send_sr ( rail_ib );
		sctk_ibuf_prepare ( remote, ibuf, ibuf_size );
	}
	else
	{
		ibuf = sctk_ibuf_pick_send ( rail_ib, remote, &ibuf_size );
	}

	/* We cannot pick an ibuf. We should try the buffered eager protocol */
	if ( ibuf == NULL )
		return NULL;

	IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
	IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );
	IBUF_SET_POISON ( ibuf->buffer );

	PROF_TIME_START ( rail_ib->rail, ib_ibuf_memcpy );
	/* Copy header */
	memcpy ( IBUF_GET_EAGER_MSG_HEADER ( ibuf->buffer ), msg, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
	/* Copy payload */
	sctk_net_copy_in_buffer ( msg, IBUF_GET_EAGER_MSG_PAYLOAD ( ibuf->buffer ) );
	PROF_TIME_END ( rail_ib->rail, ib_ibuf_memcpy );

	eager_header = IBUF_GET_EAGER_HEADER ( ibuf->buffer );
	eager_header->payload_size = size - sizeof ( mpc_lowcomm_ptp_message_body_t );
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_EAGER_PROTOCOL );

	return ibuf;
}


/*-----------------------------------------------------------
 *  Internal free functions
 *----------------------------------------------------------*/
static void __free_no_recopy ( void *arg )
{
	mpc_lowcomm_ptp_message_t *msg = ( mpc_lowcomm_ptp_message_t * ) arg;
	sctk_ibuf_t *ibuf = NULL;

	/* Assume msg not recopies */
	ib_assume ( !msg->tail.ib.eager.recopied );
	ibuf = msg->tail.ib.eager.ibuf;
	sctk_ibuf_release ( ibuf->region->rail, ibuf, 0 );
	sctk_ib_eager_release_buffered_ptp_message ( ibuf->region->rail->rail, msg );
}

static void __free_with_recopy ( void *arg )
{
	mpc_lowcomm_ptp_message_t *msg = ( mpc_lowcomm_ptp_message_t * ) arg;
	sctk_ibuf_t *ibuf = NULL;

	ibuf = msg->tail.ib.eager.ibuf;
	sctk_ib_eager_release_buffered_ptp_message ( ibuf->region->rail->rail, msg );
}

/*-----------------------------------------------------------
 *  Ibuf free function
 *----------------------------------------------------------*/
void sctk_ib_eager_recv_free ( sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,  sctk_ibuf_t *ibuf, int recopy )
{
	/* Read from recopied buffer */
	if ( recopy )
	{
		_mpc_comm_ptp_message_set_copy_and_free ( msg, __free_with_recopy, sctk_net_message_copy );
		sctk_ibuf_release ( &rail->network.ib, ibuf, 0 );
		/* Read from network buffer  */
	}
	else
	{
		_mpc_comm_ptp_message_set_copy_and_free ( msg, __free_no_recopy, sctk_ib_eager_recv_msg_no_recopy );
	}
}


static mpc_lowcomm_ptp_message_t *sctk_ib_eager_recv ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, int recopy,  sctk_ib_protocol_t protocol )
{
	size_t size;
	mpc_lowcomm_ptp_message_t *msg;
	void *body;
	IBUF_CHECK_POISON ( ibuf->buffer );
	/* XXX: select if a recopy is needed for the message */
	/* XXX: recopy is not compatible with CM */

	/* If recopy required */
	if ( recopy )
	{
		sctk_ib_eager_t *eager_header;
		eager_header = IBUF_GET_EAGER_HEADER ( ibuf->buffer );

		size = eager_header->payload_size;
		msg = sctk_malloc ( size + sizeof ( mpc_lowcomm_ptp_message_t ) );
		PROF_INC ( rail, ib_alloc_mem );
		ib_assume ( msg );

		body = ( char * ) msg + sizeof ( mpc_lowcomm_ptp_message_t );
		/* Copy the header of the message */
		memcpy ( msg, IBUF_GET_EAGER_MSG_HEADER ( ibuf->buffer ), sizeof ( mpc_lowcomm_ptp_message_body_t ) );

		/* Copy the body of the message */
		memcpy ( body, IBUF_GET_EAGER_MSG_PAYLOAD ( ibuf->buffer ), size );
	}
	else
	{
		msg = sctk_ib_eager_pick_buffered_ptp_message ( rail );
		ib_assume ( msg );

		/* Copy the header of the message */
		memcpy ( msg, IBUF_GET_EAGER_MSG_HEADER ( ibuf->buffer ), sizeof ( mpc_lowcomm_ptp_message_body_t ) );
	}

	SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
	msg->tail.message_type = SCTK_MESSAGE_NETWORK;
	msg->tail.ib.eager.recopied = recopy;
	msg->tail.ib.eager.ibuf = ibuf;
	msg->tail.ib.protocol = protocol;

	_mpc_comm_ptp_message_clear_request ( msg );
	return msg;
}

void sctk_ib_eager_recv_msg_no_recopy ( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	mpc_lowcomm_ptp_message_t *send;

	sctk_ibuf_t *ibuf;
	void *body;


	send = tmp->msg_send;
	/* Assume msg not recopied */
	ib_assume ( !send->tail.ib.eager.recopied );
	ibuf = send->tail.ib.eager.ibuf;
	ib_assume ( ibuf );
	body = IBUF_GET_EAGER_MSG_PAYLOAD ( ibuf->buffer );

	sctk_net_message_copy_from_buffer ( body, tmp, 1 );
}

int sctk_ib_eager_poll_recv ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf )
{
	mpc_lowcomm_ptp_message_body_t *msg_ibuf = IBUF_GET_EAGER_MSG_HEADER ( ibuf->buffer );
	const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL ( ibuf->buffer );
	int recopy = 0;

	mpc_lowcomm_ptp_message_t *msg = NULL;

	if( _mpc_comm_ptp_message_is_for_process ( msg_ibuf->header.message_type.type ) )
	{
		recopy = 1;
	}

	sctk_nodebug ( "Received IBUF %p %d", ibuf, IBUF_GET_CHANNEL ( ibuf ) );

	/* Normal message: we handle it */
	msg = sctk_ib_eager_recv ( rail, ibuf, recopy, protocol );
	sctk_ib_eager_recv_free ( rail, msg, ibuf, recopy );

	return rail->send_message_from_network ( msg );
}

#endif
