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
/* #                                                                      # */
/* ######################################################################## */


#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_reorder.h>
#include <sctk_communicator.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <opa_primitives.h>
#include <comm.h>
#include <sctk_ib_buffered.h>
#include <sctk_alloc.h>

/*
 * Initialize the reordering for the list
 */
void sctk_reorder_list_init ( sctk_reorder_list_t *reorder )
{
	reorder->lock = SCTK_SPINLOCK_INITIALIZER;
	reorder->table = NULL;
}

/*
 * Add a reorder entry to the reorder table and initialize it
 */
sctk_reorder_table_t *sctk_init_task_to_reorder ( int dest )
{
	sctk_reorder_table_t *tmp;

	tmp = sctk_malloc( sizeof ( sctk_reorder_table_t ) );

	assume( tmp != NULL );

	memset( tmp, 0, sizeof ( sctk_reorder_table_t ) );

	OPA_store_int ( & ( tmp->message_number_src ), 0 );
	OPA_store_int ( & ( tmp->message_number_dest ), 0 );
	tmp->lock = SCTK_SPINLOCK_INITIALIZER;
	tmp->buffer = NULL;
	tmp->key.destination = dest;

	return tmp;
}

/*
 * Get an entry from the reorder. Add it if not exist
 */
sctk_reorder_table_t *sctk_get_task_from_reorder ( int dest, sctk_reorder_list_t *reorder )
{
	sctk_reorder_key_t key;
	sctk_reorder_table_t *tmp;

	sctk_nodebug("[%p] LOOK %d / %d", reorder, dest, process_specific );

	key.destination = dest;

	mpc_common_spinlock_lock ( &reorder->lock );

	HASH_FIND ( hh, reorder->table, &key, sizeof ( sctk_reorder_key_t ), tmp );

	if ( tmp == NULL )
	{
		tmp = sctk_init_task_to_reorder ( dest );
		/* Add the entry */
		HASH_ADD ( hh, reorder->table, key, sizeof ( sctk_reorder_key_t ), tmp );
		sctk_nodebug ( "%p Entry for task %d added to %p!!", reorder->table, dest, reorder->table );
	}

	mpc_common_spinlock_unlock ( &reorder->lock );
	assume ( tmp );

	return tmp;
}

/* Try to handle all messages with an expected sequence number */
static inline int __send_pending_messages ( sctk_reorder_table_t *tmp )
{
	int ret = REORDER_FOUND_NOT_EXPECTED;

	if ( tmp->buffer != NULL )
	{
		sctk_reorder_buffer_t *reorder;
		int key;

		do
		{
			mpc_common_spinlock_lock ( & ( tmp->lock ) );
			key = OPA_load_int ( & ( tmp->message_number_src ) );
			HASH_FIND ( hh, tmp->buffer, &key, sizeof ( int ), reorder );

			if ( reorder != NULL )
			{
				sctk_nodebug ( "Pending Send %d for %p", SCTK_MSG_NUMBER ( reorder->msg ), tmp );
				HASH_DELETE ( hh, tmp->buffer, reorder );
				mpc_common_spinlock_unlock ( & ( tmp->lock ) );
				/* We can not keep the lock during sending the message to MPC or the
				 * code will deadlock */
				_mpc_comm_ptp_message_send_check ( reorder->msg, 1 );

				OPA_fetch_and_incr_int ( & ( tmp->message_number_src ) );
				ret = REORDER_FOUND_EXPECTED;
			}
			else
			{
				mpc_common_spinlock_unlock ( & ( tmp->lock ) );
			}
		}
		while ( reorder != NULL );
	}

	return ret;
}


/*
 * Receive message from network
 * */
int sctk_send_message_from_network_reorder ( mpc_lowcomm_ptp_message_t *msg )
{
	const int src_task  = SCTK_MSG_SRC_TASK ( msg );
	const int dest_task = SCTK_MSG_DEST_TASK ( msg );

	sctk_nodebug ( "Recv message from [%d,%d] to [%d,%d] (number: %d)", SCTK_MSG_SRC_PROCESS ( msg ), src_task, SCTK_MSG_DEST_PROCESS ( msg ), dest_task, SCTK_MSG_NUMBER ( msg ) );

	int dest_process;
	int number;
	sctk_reorder_table_t *tmp = NULL;

	if ( SCTK_MSG_USE_MESSAGE_NUMBERING ( msg ) == 0 )
	{
		/* Renumbering unused */
		return REORDER_NO_NUMBERING;
	}
	else
	{
		dest_process = sctk_get_process_rank_from_task_rank ( dest_task );
		sctk_debug ( "Recv message from %d to %d (number:%d)",
			     SCTK_MSG_SRC_TASK ( msg ),
			     SCTK_MSG_DEST_TASK ( msg ), SCTK_MSG_NUMBER ( msg ) );

		sctk_nodebug("RET %d == %d", dest_process, mpc_common_get_process_rank() );

		/* Indirect messages, we do not check PSN */
		if( (mpc_common_get_process_rank() != dest_process)
		/* Process level messages we do not check the PSN */
		||  (_mpc_comm_ptp_message_is_for_process(SCTK_MSG_SPECIFIC_CLASS(msg)) ) )
		{
			return REORDER_NO_NUMBERING;
		}

                sctk_reorder_list_t *list =
                    _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), dest_task);
                sctk_nodebug("GET REORDER LIST FOR %d -> %d", src_task,
                             dest_task);
                tmp = sctk_get_task_from_reorder(src_task, list);
                assume(tmp != NULL);
                sctk_nodebug("LIST %p ENTRY %p src %d dest %d", list, tmp,
                             src_task, dest_task);

                number = OPA_load_int(&(tmp->message_number_src));
                sctk_nodebug("wait for %d recv %d", number,
                             SCTK_MSG_NUMBER(msg));

                if (number == SCTK_MSG_NUMBER(msg)) {
                  sctk_nodebug("Direct Send %d from %p", SCTK_MSG_NUMBER(msg),
                               tmp);

                  _mpc_comm_ptp_message_send_check(msg, 1);

                  OPA_fetch_and_incr_int(&(tmp->message_number_src));

                  /*Search for pending messages*/
                  __send_pending_messages(tmp);

                  return REORDER_FOUND_EXPECTED;

                } else {
                  sctk_reorder_buffer_t *reorder;

                  reorder = &(msg->tail.reorder);

                  reorder->key = SCTK_MSG_NUMBER(msg);
                  reorder->msg = msg;

                  mpc_common_spinlock_lock(&(tmp->lock));
                  HASH_ADD(hh, tmp->buffer, key, sizeof(int), reorder);
                  mpc_common_spinlock_unlock(&(tmp->lock));
                  sctk_nodebug("recv %d to %d - delay wait for %d recv %d "
                               "(expecting:%d, tmp:%p)",
                               SCTK_MSG_SRC_PROCESS(msg),
                               SCTK_MSG_DEST_PROCESS(msg), number,
                               SCTK_MSG_NUMBER(msg), number, tmp);

                  /* XXX: we *MUST* check once again if there is no pending
                  * messages. During
                  * adding a msg to the pending list, a new message with a good
                  * PSN could be
                  * received. If we omit this line, the code can deadlock */
                  return __send_pending_messages(tmp);
                }
        }

        return 0;
}

/*
 * Send message to network
 * */
int sctk_prepare_send_message_to_network_reorder ( mpc_lowcomm_ptp_message_t *msg )
{
	sctk_reorder_table_t *tmp;
	const int src_task  = SCTK_MSG_SRC_TASK ( msg );
	const int dest_task = SCTK_MSG_DEST_TASK ( msg );

	sctk_nodebug ( "Send message from %d to %d", src_task, dest_task );

	/* Indirect messages */
	int src_process;
	src_process = sctk_get_process_rank_from_task_rank ( src_task );

	if ( mpc_common_get_process_rank() != src_process )
	{
		return 0;
	}

	sctk_nodebug ( "msg->sctk_msg_get_use_message_numbering %d", SCTK_MSG_USE_MESSAGE_NUMBERING ( msg, 0 ); );

        sctk_reorder_list_t *list = _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), src_task);
        tmp = sctk_get_task_from_reorder(dest_task,
                                         list);
        assume(tmp);

        /* Local numbering */
        SCTK_MSG_USE_MESSAGE_NUMBERING_SET(msg, 1);
        SCTK_MSG_NUMBER_SET(
            msg, OPA_fetch_and_incr_int(&(tmp->message_number_dest)));
        sctk_nodebug("send %d to %d (number:%d)", SCTK_MSG_SRC_TASK(msg),
                     SCTK_MSG_DEST_TASK(msg), SCTK_MSG_NUMBER(msg));

        return 0;
}
