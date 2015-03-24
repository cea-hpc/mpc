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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_control_messages.h"

#include <stdio.h>

#include <mpcmp.h>
#include <sctk_low_level_comm.h>
#include <sctk_communicator.h>
#include <mpc_datatypes.h>
#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <sctk_asm.h>
#include <sctk_checksum.h>
#include <mpc_common.h>
#include <sctk_reorder.h>
#include <sctk_rail.h>

/************************************************************************/
/* Control Messages Context                                             */
/************************************************************************/

static struct sctk_control_message_context __ctrl_msg_context = { NULL };

static inline struct sctk_control_message_context * sctk_control_message_ctx()
{
	return &__ctrl_msg_context;
}

void sctk_control_message_context_set_user( void (*fn)( int , int , char , char , void * ) )
{
	sctk_control_message_ctx()->sctk_user_control_message = fn;
}


/************************************************************************/
/* Process Level Messages                                               */
/************************************************************************/

void sctk_control_message_process_level( int source_process, int source_rank, char subtype, char param, void * data )
{
	
	
	
	
	
	
}

/************************************************************************/
/* Control Messages Send Recv                                           */
/************************************************************************/

static void sctk_free_control_messages ( void *ptr )
{

}

void sctk_control_messages_send ( int dest, sctk_message_class_t message_class, int subtype, int param, void *buffer, size_t size )
{

	sctk_communicator_t communicator = SCTK_COMM_WORLD;
	sctk_communicator_t tag = 0;
	
	sctk_request_t request;
	sctk_thread_ptp_message_t msg;
	
	if( !sctk_message_class_is_process_specific( message_class ) )
	{
		sctk_fatal("Cannot send a non-process specific message using this function");
	}
	
	/* Fill in control message context (note that class is handled by set_header_in_message) */
	SCTK_MSG_SPECIFIC_CLASS_SET_SUBTYPE( (&msg) , subtype );
	SCTK_MSG_SPECIFIC_CLASS_SET_PARAM( (&msg) , param );
	
	sctk_init_header ( &msg, SCTK_MESSAGE_CONTIGUOUS, sctk_free_control_messages, sctk_message_copy );
	
	sctk_add_adress_in_message ( &msg, buffer, size );
	
	sctk_set_header_in_message ( &msg, tag, communicator,  sctk_get_process_rank(), dest,  &request, size, message_class, MPC_DATATYPE_IGNORE );
	
	sctk_send_message ( &msg );
	
	sctk_wait_message ( &request );
}

void sctk_control_messages_incoming( sctk_thread_ptp_message_t * msg )
{
	sctk_message_class_t class = SCTK_MSG_SPECIFIC_CLASS( msg );

	if( !sctk_message_class_is_process_specific( class ) )
	{
		sctk_fatal("Cannot process a non-process specific message using this function");
	}
	
	
	int source_process = SCTK_MSG_SRC_PROCESS( msg );
	int source_rank = SCTK_MSG_SRC_TASK( msg );
	short subtype = SCTK_MSG_SPECIFIC_CLASS_SUBTYPE( msg );
	short param = SCTK_MSG_SPECIFIC_CLASS_PARAM( msg );
	
	
	
	void * tmp_contol_buffer = sctk_malloc( SCTK_MSG_SIZE ( msg ) );
	assume( tmp_contol_buffer != NULL );
	
	/* Generate the paired recv message to fill the buffer in
	 * a portable way (idependently from driver) */
	
	sctk_thread_ptp_message_t recvmsg;
	sctk_request_t request;
	sctk_init_header ( &recvmsg, SCTK_MESSAGE_CONTIGUOUS, sctk_free_control_messages, sctk_message_copy );
	sctk_add_adress_in_message ( &recvmsg, tmp_contol_buffer, SCTK_MSG_SIZE( msg ) );
	sctk_set_header_in_message ( &recvmsg, 0, SCTK_COMM_WORLD, source_rank, sctk_get_process_rank(),  &request, SCTK_MSG_SIZE( msg ), class, MPC_DATATYPE_IGNORE );
	
	/* Trigger the receive task (as if we matched) */
	sctk_message_to_copy_t copy_task;
	copy_task.msg_send = msg;
	copy_task.msg_recv = &recvmsg;
	copy_task.prev = NULL;
	copy_task.next = NULL;
	
	/* Trigger the copy from network task */
	copy_task.msg_send->tail.message_copy ( &copy_task );
	
	void * data =  tmp_contol_buffer;
	
	char rail_id = SCTK_MSG_SPECIFIC_CLASS_RAILID( msg );
	
	switch( class )
	{
		case SCTK_CONTROL_MESSAGE_RAIL:
		{
			sctk_rail_info_t * rail = sctk_rail_get_by_id ( rail_id );
			
			if(!rail)
			{
				sctk_fatal("No such rail %d", rail_id );
			}
			
			if( !rail->control_message_handler )
			{
				sctk_warning("No handler was set to rail (%d = %s) control messages, set it prior to send such messages", rail_id, rail->network_name );
				return;
			}
			
			(rail->control_message_handler)( rail, source_process, source_rank, subtype, param,  data );
		}
		break;
		case SCTK_CONTROL_MESSAGE_PROCESS:
		{
			sctk_control_message_process_level( source_process, source_rank, subtype, param,  data );
		}
		break;
		case SCTK_CONTROL_MESSAGE_USER:
		{
			struct sctk_control_message_context *ctx = sctk_control_message_ctx();
			
			if( !ctx->sctk_user_control_message )
			{
				sctk_warning("No handler was set to process user level control messages, set it prior to send such messages");
				return;
			}
			
			(ctx->sctk_user_control_message)( source_process, source_rank, subtype, param,  data );
		}
		break;
		default:
			not_reachable();
	}
	
	/* Free the TMP buffer */
	sctk_free( tmp_contol_buffer );
}


