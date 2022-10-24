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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <mpc_lowcomm_msg.h>
#include <mpc_config.h>
#include <mpc_common_debug.h>
#include <mpc_lowcomm.h>
#include <mpc_common_asm.h>


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Messages Types                                               */
/************************************************************************/


static inline int _mpc_comm_ptp_message_is_for_process( mpc_lowcomm_ptp_message_class_t type )
{
	switch ( type )
	{
		case MPC_LOWCOMM_CLASS_NONE:
			not_implemented();

		case MPC_LOWCOMM_CANCELLED_SEND:
		case MPC_LOWCOMM_CANCELLED_RECV:
		case MPC_LOWCOMM_P2P_MESSAGE:
		case MPC_LOWCOMM_P2P_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_MESSAGE:
		case MPC_LOWCOMM_RDV_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_PUT_MESSAGE:
		case MPC_LOWCOMM_RDV_PUT_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_GET_MESSAGE:
		case MPC_LOWCOMM_RDV_GET_TM_MESSAGE:
		case MPC_LOWCOMM_ACK_MESSAGE:
		case MPC_LOWCOMM_ACK_TM_MESSAGE:
		case MPC_LOWCOMM_FRAG_MESSAGE:
		case MPC_LOWCOMM_FRAG_TM_MESSAGE:
		case MPC_LOWCOMM_RDMA_MESSAGE:
		case MPC_LOWCOMM_BARRIER_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_MESSAGE:
		case MPC_LOWCOMM_ALLREDUCE_MESSAGE:
		case MPC_LOWCOMM_CONTROL_MESSAGE_FENCE:
		case MPC_LOWCOMM_RDMA_WINDOW_MESSAGES: /* Note that the RDMA win message
                                           is not process specific to force
                                           on-demand connections between the
                                           RDMA peers prior to emitting RDMA */
			return 0;

		case MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL:
		case MPC_LOWCOMM_ALLREDUCE_HETERO_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_HETERO_MESSAGE:
		case MPC_LOWCOMM_BARRIER_HETERO_MESSAGE:
		case MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_REDUCE_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_ALLREDUCE_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_MESSAGE_UNIVERSE:
			return 1;

		case MPC_LOWCOMM_MESSAGE_CLASS_COUNT:
			return 0;
		case MPC_LOWCOMM_MSG_LAST:
			return 0;
	}

	return 0;
}

static inline int _mpc_comm_ptp_message_is_for_control( mpc_lowcomm_ptp_message_class_t type )
{
	switch ( type )
	{
		case MPC_LOWCOMM_CLASS_NONE:
			not_implemented();

		case MPC_LOWCOMM_CANCELLED_SEND:
		case MPC_LOWCOMM_CANCELLED_RECV:
		case MPC_LOWCOMM_P2P_MESSAGE:
		case MPC_LOWCOMM_P2P_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_MESSAGE:
		case MPC_LOWCOMM_RDV_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_PUT_MESSAGE:
		case MPC_LOWCOMM_RDV_PUT_TM_MESSAGE:
		case MPC_LOWCOMM_RDV_GET_MESSAGE:
		case MPC_LOWCOMM_RDV_GET_TM_MESSAGE:
		case MPC_LOWCOMM_ACK_MESSAGE:
		case MPC_LOWCOMM_ACK_TM_MESSAGE:
		case MPC_LOWCOMM_FRAG_MESSAGE:
		case MPC_LOWCOMM_FRAG_TM_MESSAGE:
		case MPC_LOWCOMM_RDMA_MESSAGE:
		case MPC_LOWCOMM_BARRIER_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_MESSAGE:
		case MPC_LOWCOMM_ALLREDUCE_MESSAGE:
		case MPC_LOWCOMM_ALLREDUCE_HETERO_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_HETERO_MESSAGE:
		case MPC_LOWCOMM_BARRIER_HETERO_MESSAGE:
		case MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_REDUCE_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_ALLREDUCE_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_RDMA_WINDOW_MESSAGES:
		case MPC_LOWCOMM_CONTROL_MESSAGE_FENCE:
		case MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL:
		case MPC_LOWCOMM_MESSAGE_UNIVERSE:
			return 0;


		case MPC_LOWCOMM_MESSAGE_CLASS_COUNT:
			return 0;
		case MPC_LOWCOMM_MSG_LAST:
			return 0;
	}

	return 0;
}


/************************************************************************/
/* mpc_lowcomm_ptp_message_t Point to Point message descriptor          */
/************************************************************************/


void _mpc_comm_ptp_message_send_check( mpc_lowcomm_ptp_message_t *msg, int perform_check );
void _mpc_comm_ptp_message_recv_check( mpc_lowcomm_ptp_message_t *msg, int perform_check );
void _mpc_comm_ptp_message_commit_request( mpc_lowcomm_ptp_message_t *send, mpc_lowcomm_ptp_message_t *recv );

static inline void _mpc_comm_ptp_message_set_copy_and_free( mpc_lowcomm_ptp_message_t *tmp,
        void ( *free_memory )( void * ),
        void ( *message_copy )( mpc_lowcomm_ptp_message_content_to_copy_t * ) )
{
	tmp->tail.free_memory = free_memory;
	tmp->tail.message_copy = message_copy;
	tmp->tail.buffer_async = NULL;
	memset( &tmp->tail.message.pack, 0, sizeof( tmp->tail.message.pack ) );
}

/** mpc_lowcomm_ptp_message_header_t GETTERS and Setters */

#define SCTK_MSG_USE_MESSAGE_NUMBERING( msg ) msg->body.header.use_message_numbering
#define SCTK_MSG_USE_MESSAGE_NUMBERING_SET( msg, num ) \
	do                                                 \
	{                                                  \
		msg->body.header.use_message_numbering = num;  \
	} while ( 0 )

#define SCTK_MSG_SRC_PROCESS_UID( msg ) msg->body.header.source
#define SCTK_MSG_SRC_PROCESS_UID_SET( msg, uid )  msg->body.header.source = uid
#define SCTK_MSG_SRC_PROCESS( msg ) mpc_lowcomm_peer_get_rank(SCTK_MSG_SRC_PROCESS_UID(msg))
#define SCTK_MSG_SRC_PROCESS_SET( msg, src ) \
	do                                       \
	{                                        \
		msg->body.header.source = mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), src);       \
	} while ( 0 )
	

#define SCTK_MSG_DEST_PROCESS_UID( msg ) msg->body.header.destination
#define SCTK_MSG_DEST_PROCESS_UID_SET( msg, uid )  msg->body.header.destination = uid
#define SCTK_MSG_DEST_PROCESS( msg ) mpc_lowcomm_peer_get_rank(SCTK_MSG_DEST_PROCESS_UID(msg))
#define SCTK_MSG_DEST_PROCESS_SET( msg, dest ) \
	do                                         \
	{                                          \
		msg->body.header.destination = mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), dest);   \
	} while ( 0 )


#define SCTK_MSG_SRC_TASK( msg ) msg->body.header.source_task
#define SCTK_MSG_SRC_TASK_SET( msg, src )   \
	do                                      \
	{                                       \
		msg->body.header.source_task = src; \
	} while ( 0 )

#define SCTK_MSG_DEST_TASK( msg ) msg->body.header.destination_task
#define SCTK_MSG_DEST_TASK_SET( msg, dest )       \
	do                                            \
	{                                             \
		msg->body.header.destination_task = dest; \
	} while ( 0 )

#define SCTK_MSG_COMMUNICATOR_ID( msg ) msg->body.header.communicator_id
#define SCTK_MSG_COMMUNICATOR_ID_SET( msg, id ) \
	do                                         \
	{                                          \
		msg->body.header.communicator_id = id; \
	} while ( 0 )


#define SCTK_MSG_COMMUNICATOR( msg ) msg->tail.communicator
#define SCTK_MSG_COMMUNICATOR_SET( msg, comm ) \
	do                                         \
	{                                          \
		msg->body.header.communicator_id = comm->id;  \
		msg->tail.communicator = comm;  \
	} while ( 0 )

#define SCTK_DATATYPE( msg ) msg->body.header.datatype
#define SCTK_MSG_DATATYPE_SET( msg, type ) \
	do                                     \
	{                                      \
		msg->body.header.datatype = type;  \
	} while ( 0 )

#define SCTK_MSG_TAG( msg ) msg->body.header.message_tag
#define SCTK_MSG_TAG_SET( msg, tag )        \
	do                                      \
	{                                       \
		msg->body.header.message_tag = tag; \
	} while ( 0 )

#define SCTK_MSG_NUMBER( msg ) msg->body.header.message_number
#define SCTK_MSG_NUMBER_SET( msg, number )        \
	do                                            \
	{                                             \
		msg->body.header.message_number = number; \
	} while ( 0 )

#define SCTK_MSG_MATCH( msg ) OPA_load_int( &msg->tail.matching_id )
#define SCTK_MSG_MATCH_SET( msg, match ) OPA_store_int( &msg->tail.matching_id, match )

#define SCTK_MSG_SPECIFIC_CLASS( msg ) msg->body.header.message_type
#define SCTK_MSG_SPECIFIC_CLASS_SET( msg, specific_tag )   \
	do                                                     \
	{                                                      \
		msg->body.header.message_type = specific_tag; \
	} while ( 0 )



#define SCTK_MSG_HEADER( msg ) &msg->body.header

#define SCTK_MSG_SIZE( msg ) msg->body.header.msg_size
#define SCTK_MSG_SIZE_SET( msg, size )    \
	do                                    \
	{                                     \
		msg->body.header.msg_size = size; \
	} while ( 0 )

#define SCTK_MSG_REQUEST( msg ) msg->tail.request

#define SCTK_MSG_COMPLETION_FLAG( msg ) msg->tail.completion_flag
#define SCTK_MSG_COMPLETION_FLAG_SET( msg, completion ) \
	do                                                  \
	{                                                   \
		msg->tail.completion_flag = completion;         \
	} while ( 0 )



/************************************************************************/
/* General Functions                                                    */
/************************************************************************/

struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get( mpc_lowcomm_communicator_t comm, int rank );
_mpc_lowcomm_reorder_list_t *_mpc_comm_ptp_array_get_reorder( mpc_lowcomm_communicator_t communicator, int rank );

void _mpc_comm_ptp_message_reinit_comm(mpc_lowcomm_ptp_message_t *msg);

#define SCTK_PARALLEL_COMM_QUEUES_NUMBER 8

/************************************************************************/
/* Specific Message Tagging	                                          */
/************************************************************************/

/** Message for a process with ordering and a tag */
static inline int sctk_is_process_specific_message( mpc_lowcomm_ptp_message_header_t *header )
{
	mpc_lowcomm_ptp_message_class_t class = header->message_type;
	return _mpc_comm_ptp_message_is_for_process( class );
}


/** Message for a process with ordering and a tag */
static inline int _mpc_lowcomm_message_is_for_universe( mpc_lowcomm_ptp_message_header_t *header )
{
	return (header->message_type == MPC_LOWCOMM_MESSAGE_UNIVERSE);
}


static inline void _mpc_comm_ptp_message_clear_request( mpc_lowcomm_ptp_message_t *msg )
{
	msg->tail.request = NULL;
    msg->tail.internal_ptp = NULL;

	if (sctk_is_process_specific_message(SCTK_MSG_HEADER(msg)))
	{
		return;
#if 0
	} else {
		if (SCTK_MSG_SRC_PROCESS(msg) == MPC_ANY_SOURCE) {
			/* Source task not available */
			SCTK_MSG_SRC_TASK_SET(msg, -1);
		}
#endif
	}
}


#ifdef __cplusplus
}
#endif

#endif
