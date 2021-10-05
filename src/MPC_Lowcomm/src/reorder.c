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

#include <reorder.h>

#include <mpc_common_debug.h>
#include <group.h>
#include <sctk_alloc.h>
#include <communicator.h>

#include <comm.h>
#include <mpc_common_rank.h>

/*
 * Initialize the reordering for the list
 */
void _mpc_lowcomm_reorder_list_init(_mpc_lowcomm_reorder_list_t *reorder)
{
	mpc_common_spinlock_init(&reorder->lock, 0);
	reorder->table = NULL;
}

/*
 * Add a reorder entry to the reorder table and initialize it
 */
static inline _mpc_lowcomm_reorder_table_t *___new_reorder_entry(mpc_lowcomm_peer_uid_t uid, int rank)
{
	_mpc_lowcomm_reorder_table_t *tmp;

	tmp = sctk_malloc(sizeof(_mpc_lowcomm_reorder_table_t) );

	assume(tmp != NULL);

	memset(tmp, 0, sizeof(_mpc_lowcomm_reorder_table_t) );

	OPA_store_int(&(tmp->message_number_src), 0);
	OPA_store_int(&(tmp->message_number_dest), 0);
	mpc_common_spinlock_init(&tmp->lock, 0);
	tmp->buffer          = NULL;
	tmp->key.destination = rank;
	tmp->key.uid         = uid;

	return tmp;
}

/*
 * Get an entry from the reorder. Add it if not exist
 */
static inline _mpc_lowcomm_reorder_table_t *__get_reorder_entry(mpc_lowcomm_peer_uid_t uid,
                                                                int rank,
                                                                _mpc_lowcomm_reorder_list_t *reorder)
{
	_mpc_lowcomm_reorder_key_t    key = { 0 };
	_mpc_lowcomm_reorder_table_t *tmp;

	key.destination = rank;
	key.uid         = uid;

	mpc_common_spinlock_lock(&reorder->lock);

	HASH_FIND(hh, reorder->table, &key, sizeof(_mpc_lowcomm_reorder_key_t), tmp);

	if(tmp == NULL)
	{
		tmp = ___new_reorder_entry(uid, rank);
		/* Add the entry */
		HASH_ADD(hh, reorder->table, key, sizeof(_mpc_lowcomm_reorder_key_t), tmp);
		mpc_common_tracepoint_fmt("%p Entry for task (%lld ; %d) added to %p!!", reorder->table, uid, rank, reorder->table);
	}

	mpc_common_spinlock_unlock(&reorder->lock);
	assume(tmp);

	return tmp;
}

/* Try to handle all messages with an expected sequence number */
static inline int __send_pending_messages(_mpc_lowcomm_reorder_table_t *tmp)
{
	int ret = _MPC_LOWCOMM_REORDER_FOUND_NOT_EXPECTED;

	if(tmp->buffer != NULL)
	{
		mpc_lowcomm_reorder_buffer_t *reorder;
		int key;

		do
		{
			mpc_common_spinlock_lock(&(tmp->lock) );
			key = OPA_load_int(&(tmp->message_number_src) );
			HASH_FIND_INT(tmp->buffer, &key, reorder);

			if(reorder != NULL)
			{
				mpc_common_tracepoint_fmt("Pending Send %d for %p", SCTK_MSG_NUMBER(reorder->msg), tmp);
				HASH_DELETE(hh, tmp->buffer, reorder);
				mpc_common_spinlock_unlock(&(tmp->lock) );

				/* We can not keep the lock during sending the message to MPC or the
				 * code will deadlock */
				_mpc_comm_ptp_message_send_check(reorder->msg, 1);

				OPA_fetch_and_incr_int(&(tmp->message_number_src) );
				ret = _MPC_LOWCOMM_REORDER_FOUND_EXPECTED;
			}
			else
			{
				mpc_common_spinlock_unlock(&(tmp->lock) );
			}
		} while(reorder != NULL);
	}

	return ret;
}

/*
 * Receive message from network
 * */
int _mpc_lowcomm_reorder_msg_check(mpc_lowcomm_ptp_message_t *msg)
{
	const mpc_lowcomm_peer_uid_t src_uid = SCTK_MSG_SRC_PROCESS_UID(msg);
	const int src_task  = SCTK_MSG_SRC_TASK(msg);
	const int dest_task = SCTK_MSG_DEST_TASK(msg);

	mpc_common_tracepoint_fmt("Recv message from [%llu,%d] to [%llu,%d] (number: %d)", SCTK_MSG_SRC_PROCESS_UID(msg), src_task, SCTK_MSG_DEST_PROCESS_UID(msg), dest_task, SCTK_MSG_NUMBER(msg) );

	_mpc_comm_ptp_message_reinit_comm(msg);

	int dest_process;
	int number;
	_mpc_lowcomm_reorder_table_t *tmp = NULL;

	if(SCTK_MSG_USE_MESSAGE_NUMBERING(msg) == 0)
	{
		/* Renumbering unused */
		return _MPC_LOWCOMM_REORDER_NO_NUMBERING;
	}
	else
	{
		if(0 <= dest_task)
		{
			dest_process = mpc_lowcomm_group_process_rank_from_world(dest_task);
		}
		else
		{
			dest_process = -1;
		}

		/* Indirect messages, we do not check PSN */
		if( (mpc_common_get_process_rank() != dest_process)
		    /* Process level messages we do not check the PSN */
		    || (_mpc_comm_ptp_message_is_for_process(SCTK_MSG_SPECIFIC_CLASS(msg) ) ) )
		{
			return _MPC_LOWCOMM_REORDER_NO_NUMBERING;
		}

		_mpc_lowcomm_reorder_list_t *list = _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), dest_task);

		tmp = __get_reorder_entry(src_uid, src_task, list);
		assume(tmp != NULL);

		number = OPA_load_int(&(tmp->message_number_src) );


		mpc_common_tracepoint_fmt("LIST %p ENTRY %p @ %d (incoming number %d @ %p)", list, tmp, number, SCTK_MSG_NUMBER(msg), &(tmp->message_number_src) );


		if(number == SCTK_MSG_NUMBER(msg) )
		{
			_mpc_comm_ptp_message_send_check(msg, 1);

			OPA_fetch_and_incr_int(&(tmp->message_number_src) );

			/*Search for pending messages*/
			__send_pending_messages(tmp);

			return _MPC_LOWCOMM_REORDER_FOUND_EXPECTED;
		}
		else
		{
			mpc_lowcomm_reorder_buffer_t *reorder;

			reorder = &(msg->tail.reorder);

			reorder->key = SCTK_MSG_NUMBER(msg);
			reorder->msg = msg;

			mpc_common_spinlock_lock(&(tmp->lock) );
			HASH_ADD_INT(tmp->buffer, key, reorder);
			mpc_common_spinlock_unlock(&(tmp->lock) );
			mpc_common_tracepoint_fmt("DELAYED recv %llu to %llu - delay wait counter is %d had %d "
			                          "(tmp:%p)",
			                          SCTK_MSG_SRC_PROCESS_UID(msg),
			                          SCTK_MSG_DEST_PROCESS_UID(msg), number,
			                          SCTK_MSG_NUMBER(msg), tmp);

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
int _mpc_lowcomm_reorder_msg_register(mpc_lowcomm_ptp_message_t *msg)
{
	_mpc_lowcomm_reorder_table_t *tmp;
	const int src_task  = SCTK_MSG_SRC_TASK(msg);
	const int dest_task = SCTK_MSG_DEST_TASK(msg);
	const mpc_lowcomm_peer_uid_t dest_uid = SCTK_MSG_DEST_PROCESS_UID(msg);

	/* Indirect messages */
	int src_process;

	src_process = mpc_lowcomm_group_process_rank_from_world(src_task);

	if(mpc_common_get_process_rank() != src_process)
	{
		return 0;
	}

	_mpc_lowcomm_reorder_list_t *list = _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), src_task);
	tmp = __get_reorder_entry(dest_uid, dest_task, list);
	assume(tmp);


	if(SCTK_MSG_SPECIFIC_CLASS(msg) != MPC_LOWCOMM_MESSAGE_UNIVERSE)
	{
		/* Local numbering */
		SCTK_MSG_USE_MESSAGE_NUMBERING_SET(msg, 1);
		SCTK_MSG_NUMBER_SET(
			msg, OPA_fetch_and_incr_int(&(tmp->message_number_dest) ) );
		mpc_common_tracepoint_fmt("send %d to (%llu, %d) number:%d", src_task, dest_uid, dest_task, SCTK_MSG_NUMBER(msg) );
	}
	else
	{
		SCTK_MSG_USE_MESSAGE_NUMBERING_SET(msg, 0);
	}


	return 0;
}
