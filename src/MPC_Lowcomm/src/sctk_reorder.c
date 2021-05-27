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


#include <mpc_common_debug.h>

#include <sctk_reorder.h>
#include <communicator.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <opa_primitives.h>
#include <comm.h>
#include <mpc_common_rank.h>

#ifdef MPC_USE_INFINIBAND
#include <sctk_ib_buffered.h>
#endif

#include <group.h>

#include <sctk_alloc.h>


/*
 * Initialize the reordering for the list
 */
void sctk_reorder_list_init(sctk_reorder_list_t *reorder)
{
	mpc_common_spinlock_init(&reorder->lock, 0);
	reorder->table = NULL;
}

/*
 * Add a reorder entry to the reorder table and initialize it
 */
sctk_reorder_table_t *sctk_init_task_to_reorder(mpc_lowcomm_peer_uid_t uid, int rank)
{
	sctk_reorder_table_t *tmp;

	tmp = sctk_malloc(sizeof(sctk_reorder_table_t) );

	assume(tmp != NULL);

	memset(tmp, 0, sizeof(sctk_reorder_table_t) );

	OPA_store_int(&(tmp->message_number_src), 0);
	OPA_store_int(&(tmp->message_number_dest), 0);
	mpc_common_spinlock_init(&tmp->lock, 0);
	tmp->buffer          = NULL;
	tmp->key.destination = rank;
	tmp->key.uid = uid;

	return tmp;
}

/*
 * Get an entry from the reorder. Add it if not exist
 */
sctk_reorder_table_t *sctk_get_task_from_reorder(mpc_lowcomm_peer_uid_t uid, int rank, sctk_reorder_list_t *reorder)
{
	sctk_reorder_key_t    key = { 0 };
	sctk_reorder_table_t *tmp;

	key.destination = rank;
	key.uid = uid;

	mpc_common_spinlock_lock(&reorder->lock);

	HASH_FIND(hh, reorder->table, &key, sizeof(sctk_reorder_key_t), tmp);

	if(tmp == NULL)
	{
		tmp = sctk_init_task_to_reorder(uid, rank);
		/* Add the entry */
		HASH_ADD(hh, reorder->table, key, sizeof(sctk_reorder_key_t), tmp);
		mpc_common_nodebug("%p Entry for task %d added to %p!!", reorder->table, rank, reorder->table);
	}

	mpc_common_spinlock_unlock(&reorder->lock);
	assume(tmp);

	return tmp;
}

/* Try to handle all messages with an expected sequence number */
static inline int __send_pending_messages(sctk_reorder_table_t *tmp)
{
	int ret = REORDER_FOUND_NOT_EXPECTED;

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
				mpc_common_nodebug("Pending Send %d for %p", SCTK_MSG_NUMBER(reorder->msg), tmp);
				HASH_DELETE(hh, tmp->buffer, reorder);
				mpc_common_spinlock_unlock(&(tmp->lock) );

				/* We can not keep the lock during sending the message to MPC or the
				 * code will deadlock */
				_mpc_comm_ptp_message_send_check(reorder->msg, 1);

				OPA_fetch_and_incr_int(&(tmp->message_number_src) );
				ret = REORDER_FOUND_EXPECTED;
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
int sctk_send_message_from_network_reorder(mpc_lowcomm_ptp_message_t *msg)
{
	const mpc_lowcomm_peer_uid_t src_uid = SCTK_MSG_SRC_PROCESS_UID(msg);
	const int src_task  = SCTK_MSG_SRC_TASK(msg);
	const int dest_task = SCTK_MSG_DEST_TASK(msg);

	mpc_common_nodebug("Recv message from [%llu,%d] to [%llu,%d] (number: %d)", SCTK_MSG_SRC_PROCESS_UID(msg), src_task, SCTK_MSG_DEST_PROCESS_UID(msg), dest_task, SCTK_MSG_NUMBER(msg) );

	_mpc_comm_ptp_message_reinit_comm(msg);

	int dest_process;
	int number;
	sctk_reorder_table_t *tmp = NULL;

	if(SCTK_MSG_USE_MESSAGE_NUMBERING(msg) == 0)
	{
		/* Renumbering unused */
		return REORDER_NO_NUMBERING;
	}
	else
	{
		if( 0 <= dest_task)
		{
			dest_process = mpc_lowcomm_group_process_rank_from_world(dest_task);
		}
		else
		{
			dest_process = -1;
		}

		mpc_common_debug("Recv message from %d to %d (number:%d)",
		                 SCTK_MSG_SRC_TASK(msg),
		                 SCTK_MSG_DEST_TASK(msg), SCTK_MSG_NUMBER(msg) );

		mpc_common_nodebug("RET %d == %d", dest_process, mpc_common_get_process_rank() );

		/* Indirect messages, we do not check PSN */
		if( (mpc_common_get_process_rank() != dest_process)
		/* Process level messages we do not check the PSN */
		|| (_mpc_comm_ptp_message_is_for_process(SCTK_MSG_SPECIFIC_CLASS(msg) ) ) )
		{
			return REORDER_NO_NUMBERING;
		}

		sctk_reorder_list_t *list = _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), dest_task);
		mpc_common_nodebug("GET REORDER LIST FOR %llu:%d", src_uid, src_task);
		tmp = sctk_get_task_from_reorder(src_uid, src_task, list);
		assume(tmp != NULL);

		number = OPA_load_int(&(tmp->message_number_src) );
		mpc_common_nodebug("wait for %d recv %d", number,
		                   SCTK_MSG_NUMBER(msg) );

		mpc_common_nodebug("LIST %p ENTRY %p @ %d (incoming number %d @ %p)", list, tmp, number, SCTK_MSG_NUMBER(msg), &(tmp->message_number_src));


		if(number == SCTK_MSG_NUMBER(msg) )
		{
			mpc_common_nodebug("Direct Send %d from %p", SCTK_MSG_NUMBER(msg),
			                   tmp);

			_mpc_comm_ptp_message_send_check(msg, 1);

			OPA_fetch_and_incr_int(&(tmp->message_number_src) );

			/*Search for pending messages*/
			__send_pending_messages(tmp);

			return REORDER_FOUND_EXPECTED;
		}
		else
		{
			mpc_lowcomm_reorder_buffer_t *reorder;

			reorder = &(msg->tail.reorder);

			reorder->key = SCTK_MSG_NUMBER(msg);
			reorder->msg = msg;

			mpc_common_spinlock_lock(&(tmp->lock) );
			HASH_ADD_INT( tmp->buffer, key, reorder);
			mpc_common_spinlock_unlock(&(tmp->lock) );
			mpc_common_debug_error("recv %llu to %llu - delay wait counter is %d had %d "
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
int sctk_prepare_send_message_to_network_reorder(mpc_lowcomm_ptp_message_t *msg)
{
	sctk_reorder_table_t *tmp;
	const int             src_task  = SCTK_MSG_SRC_TASK(msg);
	const int             dest_task = SCTK_MSG_DEST_TASK(msg);
	const mpc_lowcomm_peer_uid_t dest_uid = SCTK_MSG_DEST_PROCESS_UID(msg);

	mpc_common_nodebug("Send message from %d to %d", src_task, dest_task);

	/* Indirect messages */
	int src_process;
	src_process = mpc_lowcomm_group_process_rank_from_world(src_task);

	if(mpc_common_get_process_rank() != src_process)
	{
		return 0;
	}


	sctk_reorder_list_t *list = _mpc_comm_ptp_array_get_reorder(SCTK_MSG_COMMUNICATOR(msg), src_task);
	tmp = sctk_get_task_from_reorder(dest_uid, dest_task, list);
	assume(tmp);

	/* Local numbering */
	SCTK_MSG_USE_MESSAGE_NUMBERING_SET(msg, 1);
	SCTK_MSG_NUMBER_SET(
	        msg, OPA_fetch_and_incr_int(&(tmp->message_number_dest) ) );
	mpc_common_nodebug("send %d to %d (number:%d)", SCTK_MSG_SRC_TASK(msg),
	                   SCTK_MSG_DEST_TASK(msg), SCTK_MSG_NUMBER(msg) );

	return 0;
}
