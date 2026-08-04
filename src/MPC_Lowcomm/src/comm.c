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

#include <comm.h>

#include <mpc_arch.h>
// #include "lowcomm.h"
#include <mpc_common_helper.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <checksum.h>
#include <reorder.h>
#include "mpc_keywords.h"
#include <mpc_launch.h>
#include <mpc_common_profiler.h>
#include <sctk_alloc.h>
#include <coll.h>
#include <mpc_common_flags.h>
#include <mpc_topology.h>

#include "alloc_mem.h"

#include "communicator.h"
#include "datatypes_common.h"

#include "mpc_lowcomm_types.h"
#include <mpc_lowcomm_datatypes.h>
#include <mpc_common_progress.h>


#include "one-sided/lowcomm_osc.h"
#include "lowcomm_config.h"
#include "monitor.h"
#include "pset.h"
#include "lowcomm_request.h"
#include "lowcomm_protocol.h"

/********************************************************************/
/*Structures                                                        */
/********************************************************************/

typedef struct
{
	mpc_lowcomm_communicator_id_t comm_id;
	int                           rank;
} mpc_comm_dest_key_t;

/********************************************************************/
/*Functions                                                         */
/********************************************************************/


int mpc_lowcomm_check_type_compat(mpc_lowcomm_datatype_t src, mpc_lowcomm_datatype_t dest)
{
	if (src != dest)
	{
		if ((src != MPC_DATATYPE_IGNORE) && (dest != MPC_DATATYPE_IGNORE))
		{
			/* See page 33 of 3.0 PACKED and BYTE are exceptions */
			if ((src != MPC_LOWCOMM_PACKED) && (dest != MPC_LOWCOMM_PACKED))
			{
				if ((src != MPC_LOWCOMM_BYTE) && (dest != MPC_LOWCOMM_BYTE))
				{
					if (mpc_lowcomm_datatype_is_common(src)
					    && mpc_lowcomm_datatype_is_common(dest))
					{
						return MPC_LOWCOMM_ERR_TYPE;
					}
				}
			}
		}
	}



	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_commit_status_from_request(mpc_lowcomm_request_t *request,
                                           mpc_lowcomm_status_t *status)
{
	if (!request)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	if (request->request_type == REQUEST_GENERALIZED)
	{
		mpc_lowcomm_status_t static_status;

		/* You must provide a valid status to the query function */
		if (status == MPC_LOWCOMM_STATUS_NULL)
		{
			status = &static_status;
		}

		memset(status, 0, sizeof(mpc_lowcomm_status_t));
		/* Fill in the status info */
		(request->query_fn)(request->extra_state, status);
		/* Free the request */
		(request->free_fn)(request->extra_state);
	}
	else if (status != MPC_LOWCOMM_STATUS_NULL)
	{
		status->MPC_SOURCE = request->recv_info.src;
		status->MPC_TAG    = request->recv_info.tag;
		status->MPC_ERROR  = request->status_error;
		status->size       = (int)request->recv_info.length;

		if (request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED)
		{
			status->cancelled = 1;
		}
		else
		{
			status->cancelled = 0;
		}
	}

	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_lowcomm_comm_get_lists(int rank, mpc_lowcomm_ptp_message_lists_t **lists, int *list_count)
{
	UNUSED(rank);
	UNUSED(lists);
	UNUSED(list_count);
	return MPC_LOWCOMM_SUCCESS;
}

_mpc_lowcomm_reorder_list_t *_mpc_comm_ptp_array_get_reorder(mpc_lowcomm_communicator_t communicator, int rank)
{
	UNUSED(communicator);
	UNUSED(rank);
	return NULL;
}

void _mpc_comm_ptp_message_reinit_comm(mpc_lowcomm_ptp_message_t *msg)
{
	UNUSED(msg);
}

/* This is the exported version */
int mpc_lowcomm_is_remote_rank(int dest)
{
	UNUSED(dest);
	not_reachable();
	return 0;
}

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */
int ___mpi_windows_in_use = 0;

static int (*___notify_request_completion_trampoline)(mpc_lowcomm_request_t *req) = NULL;

void mpc_lowcomm_set_request_completion_trampoline(int trampoline(mpc_lowcomm_request_t *req))
{
	___notify_request_completion_trampoline = trampoline;
}

void mpc_lowcomm_set_request_completion_callback(mpc_lowcomm_request_t *req, int trampoline(mpc_lowcomm_request_t *req))
{
	req->request_completion_fn = trampoline;
}

int mpc_lowcomm_notify_request_completion(mpc_lowcomm_request_t *req)
{
	if (___notify_request_completion_trampoline)
	{
		return (___notify_request_completion_trampoline)(req);
	}

	return 0;
}

void mpc_lowcomm_ptp_message_complete_and_free(mpc_lowcomm_ptp_message_t *msg)
{
	UNUSED(msg);
}

void _mpc_comm_ptp_message_commit_request(mpc_lowcomm_ptp_message_t *send,
                                          mpc_lowcomm_ptp_message_t *recv)
{
	UNUSED(send);
	UNUSED(recv);
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/
void __mpc_comm_ptp_message_pack_free(void *);

void mpc_lowcomm_ptp_message_header_clear(mpc_lowcomm_ptp_message_t *tmp,
                                          mpc_lowcomm_ptp_message_type_t msg_type, void (*free_memory)(void *),
                                          void (*message_copy)(mpc_lowcomm_ptp_message_content_to_copy_t *))
{
	UNUSED(tmp);
	UNUSED(msg_type);
	UNUSED(free_memory);
	UNUSED(message_copy);
	not_reachable();
}

__attribute__((deprecated(
	"Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
mpc_lowcomm_ptp_message_t *mpc_lowcomm_ptp_message_header_create(mpc_lowcomm_ptp_message_type_t msg_type)
{
	UNUSED(msg_type);
	not_reachable();
	return NULL;
}

__attribute__((deprecated(
	"Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void mpc_lowcomm_ptp_message_set_contiguous_addr(mpc_lowcomm_ptp_message_t *restrict msg,
                                                 const void *restrict addr, const size_t size)
{
	UNUSED(msg);
	UNUSED(addr);
	UNUSED(size);
	not_reachable();
}

__attribute__((deprecated(
	"Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void mpc_lowcomm_ptp_message_header_init(mpc_lowcomm_ptp_message_t *msg,
                                         const int message_tag,
                                         const mpc_lowcomm_communicator_t communicator,
                                         const mpc_lowcomm_peer_uid_t source,
                                         const mpc_lowcomm_peer_uid_t destination,
                                         mpc_lowcomm_request_t *request,
                                         const size_t count,
                                         mpc_lowcomm_ptp_message_class_t message_class,
                                         mpc_lowcomm_datatype_t datatype,
                                         mpc_lowcomm_request_type_t request_type)
{
	UNUSED(msg);
	UNUSED(message_tag);
	UNUSED(communicator);
	UNUSED(source);
	UNUSED(destination);
	UNUSED(request);
	UNUSED(count);
	UNUSED(message_class);
	UNUSED(datatype);
	UNUSED(request_type);
}

/********************************************************************/
/* Searching functions                                              */
/********************************************************************/

static __thread int       did_mprobe = 0;
static __thread OPA_int_t m_probe_id;
static __thread OPA_int_t m_probe_id_task;

void sctk_m_probe_matching_init()
{
	OPA_store_int(&m_probe_id,      0);
	OPA_store_int(&m_probe_id_task, -1);
}

void sctk_m_probe_matching_set(int value)
{
	did_mprobe = 1;

	while (OPA_cas_int(&m_probe_id, 0, value) != 0)
	{
		mpc_common_nodebug("CAS %d", OPA_load_int(&m_probe_id));
		mpc_thread_yield();
	}

	int thread_id = mpc_common_get_thread_id();

	mpc_common_nodebug("THREAD ID %d", thread_id);
	OPA_store_int(&m_probe_id_task, thread_id + 1);
}

void sctk_m_probe_matching_reset()
{
	OPA_store_int(&m_probe_id,      0);
	OPA_store_int(&m_probe_id_task, -1);
}

int sctk_m_probe_matching_get()
{
	if (!did_mprobe)
	{
		return -1;
	}

	int thread_id = mpc_common_get_thread_id();

	if (OPA_load_int(&m_probe_id_task) != (thread_id + 1))
	{
		return -1;
	}

	return OPA_load_int(&m_probe_id);
}

void mpc_lowcomm_ptp_msg_wait_init(struct mpc_lowcomm_ptp_msg_progress_s *wait,
                                   mpc_lowcomm_request_t *request, int blocking)
{
	UNUSED(wait);
	UNUSED(request);
	UNUSED(blocking);
	not_reachable();
}

// NOTE: cannot be static since needed in mpi_partitioned.c


// static inline void __mpc_comm_ptp_msg_wait(struct mpc_lowcomm_ptp_msg_progress_s *wait);


/*
 * Function called for performing message from a specific request.
 *
 * Firstly, we check if the message to receive is an intra-node message.
 * If it is, we walk on the pending list of the receive task in order
 * to find a message to match.
 *
 * If the message is an inter-node message, we call the inter-node
 * polling function according to the source of the message to
 * receive (MPC_ANY_SOURCE or not).
 *
 */


/* This is the exported version */
void mpc_lowcomm_ptp_msg_progress(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
	UNUSED(wait);
	not_reachable();
}

/********************************************************************/
/* Send messages                                                    */
/********************************************************************/

/*
 * Function called when sending a message. The message can be forwarded to
 * another process
 * using the '_mpc_lowcomm_multirail_send_message' function. If the message
 * matches, we add it to the corresponding pending list
 * */
__attribute__((deprecated(
	"Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void _mpc_comm_ptp_message_send_check(mpc_lowcomm_ptp_message_t *msg, int poll_receiver_end)
{
	UNUSED(msg);
	UNUSED(poll_receiver_end);
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
__attribute__((deprecated(
	"Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void mpc_lowcomm_ptp_message_send(mpc_lowcomm_ptp_message_t *msg)
{
	UNUSED(msg);
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void _mpc_comm_ptp_message_recv_check(mpc_lowcomm_ptp_message_t *msg,
                                      int perform_check)
{
	UNUSED(msg);
	UNUSED(perform_check);
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_recv(mpc_lowcomm_ptp_message_t *msg)
{
	UNUSED(msg);
}

/********************************************************************/
/*Probe                                                             */
/********************************************************************/

static inline void
__mpc_comm_probe_source_tag_class_func(int destination, int source, int tag,
                                       mpc_lowcomm_ptp_message_class_t class,
                                       const mpc_lowcomm_communicator_t comm,
                                       int *status,
                                       mpc_lowcomm_ptp_message_header_t *msg)
{
	UNUSED(destination);
	UNUSED(tag);
	UNUSED(class);
	UNUSED(comm);
	UNUSED(status);
	UNUSED(msg);
	UNUSED(source);
}

void mpc_lowcomm_message_probe_any_tag(int destination, int source,
                                       const mpc_lowcomm_communicator_t comm, int *status,
                                       mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, source, MPC_ANY_TAG,
		MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe_any_source(int destination, const mpc_lowcomm_communicator_t comm,
                                          int *status, mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE,
		msg->message_tag, MPC_LOWCOMM_P2P_MESSAGE, comm,
		status, msg);
}

void mpc_lowcomm_message_probe_any_source_any_tag(int destination,
                                                  const mpc_lowcomm_communicator_t comm, int *status,
                                                  mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE, MPC_ANY_TAG,
		MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe(int destination, int source,
                               const mpc_lowcomm_communicator_t comm, int *status,
                               mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, source, msg->message_tag,
		MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe_any_source_class_comm(int destination, int tag,
                                                     mpc_lowcomm_ptp_message_class_t class,
                                                     const mpc_lowcomm_communicator_t comm,
                                                     int *status,
                                                     mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE, tag, class,
		comm, status, msg);
}

/********************************************************************/
/*Message Interface                                                 */
/********************************************************************/

void mpc_lowcomm_request_init(mpc_lowcomm_request_t *request,
                              int count, mpc_lowcomm_datatype_t datatype,
                              mpc_lowcomm_complete_callback_func_t cb,
                              unsigned flags)
{
	request->header.source_task = 0;
	request->datatype           = datatype;
	request->count            = count;
	request->request_complete = cb;
	request->flags            = flags;
	request->dt_magic         = 0;
}

int mpc_lowcomm_request_cancel(mpc_lowcomm_request_t *msg)
{
	UNUSED(msg);
	return MPC_LOWCOMM_SUCCESS;
}

static inline int __mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size,
                                                int tag, mpc_lowcomm_communicator_t comm,
                                                mpc_lowcomm_ptp_message_class_t class,
                                                mpc_lowcomm_request_t *req)
{
	UNUSED(src);
	UNUSED(dest);
	UNUSED(data);
	UNUSED(size);
	UNUSED(tag);
	UNUSED(comm);
	UNUSED(class);
	UNUSED(req);
	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size,
                                int tag, mpc_lowcomm_communicator_t comm,
                                mpc_lowcomm_ptp_message_class_t class,
                                mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class_src(src, dest, data, size, tag, comm, class, req);
}

static inline int __mpc_lowcomm_isend_class(int dest, const void *data, size_t size, int tag,
                                            mpc_lowcomm_communicator_t comm,
                                            mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class_src(mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank()),
		dest,
		data,
		size,
		tag,
		comm,
		class,
		req);
}

int mpc_lowcomm_isend_class(int dest, const void *data, size_t size, int tag,
                            mpc_lowcomm_communicator_t comm,
                            mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class(dest,
		data,
		size,
		tag,
		comm,
		class,
		req);
}

static inline int __mpc_lowcomm_irecv_class_dest(int src, int dest, void *buffer, size_t size,
                                                 int tag, mpc_lowcomm_communicator_t comm,
                                                 mpc_lowcomm_ptp_message_class_t class,
                                                 mpc_lowcomm_request_t *req)
{
	UNUSED(src);
	UNUSED(dest);
	UNUSED(buffer);
	UNUSED(size);
	UNUSED(tag);
	UNUSED(comm);
	UNUSED(class);
	UNUSED(req);
	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_irecv_class_dest(int src, int dest, void *buffer, size_t size,
                                 int tag, mpc_lowcomm_communicator_t comm,
                                 mpc_lowcomm_ptp_message_class_t class,
                                 mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class_dest(src, dest, buffer, size, tag, comm, class, req);
}

static inline int __mpc_lowcomm_irecv_class(int src, void *buffer, size_t size, int tag,
                                            mpc_lowcomm_communicator_t comm,
                                            mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class_dest(src, mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank()),
		buffer, size, tag, comm, class, req);
}

int mpc_lowcomm_irecv_class(int src, void *buffer, size_t size, int tag,
                            mpc_lowcomm_communicator_t comm,
                            mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class(src, buffer, size, tag, comm, class, req);
}

/*************************
 * PUBLIC COMM INTERFACE *
 *************************/

/**************
 * RANK QUERY *
 **************/

int mpc_lowcomm_get_rank()
{
	return mpc_common_get_task_rank();
}

int mpc_lowcomm_get_size()
{
	return mpc_common_get_task_count();
}

int mpc_lowcomm_get_comm_rank(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_rank_of(communicator, mpc_lowcomm_get_rank());
}

int mpc_lowcomm_get_comm_size(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_size(communicator);
}

int mpc_lowcomm_get_process_count()
{
	return mpc_common_get_process_count();
}

int mpc_lowcomm_get_process_rank()
{
	return mpc_common_get_process_rank();
}

typedef struct
{
	int                        flag;
	int                        source;
	int                        destination;
	int                        tag;
	mpc_lowcomm_communicator_t comm;
	mpc_lowcomm_status_t *     status;
} __mpc_probe_t;

static void __mpc_probe_poll(__mpc_probe_t *arg)
{
	mpc_lowcomm_iprobe_src_dest(arg->source, arg->destination, arg->tag, arg->comm, &(arg->flag), arg->status);
}

int mpc_lowcomm_probe(int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *status)
{
	__mpc_probe_t probe_struct;

	if (source != MPC_ANY_SOURCE)
	{
		if (mpc_lowcomm_communicator_is_intercomm(comm))
		{
			probe_struct.source = mpc_lowcomm_communicator_remote_world_rank(comm, source);
		}
		else
		{
			probe_struct.source = mpc_lowcomm_communicator_world_rank_of(comm, source);
		}
	}
	else
	{
		probe_struct.source = MPC_ANY_SOURCE;
	}

	probe_struct.destination = mpc_common_get_task_rank();
	probe_struct.tag         = tag;
	probe_struct.comm        = comm;
	probe_struct.status      = status;
	mpc_lowcomm_iprobe_src_dest(probe_struct.source, probe_struct.destination, tag, comm,
		&probe_struct.flag, status);

	if (probe_struct.flag != 1)
	{
		mpc_lowcomm_perform_idle(
			&probe_struct.flag, 1, (void (*)(void *)) __mpc_probe_poll, &probe_struct);
	}

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_get_req_source(mpc_lowcomm_request_t *req)
{
	return mpc_lowcomm_peer_get_rank(req->header.source);
}

size_t mpc_lowcomm_get_req_size(mpc_lowcomm_request_t *req)
{
	return req->header.msg_size;
}

/**********************************
 * UNIVERSE COMMUNICATION SUPPORT *
 **********************************/

int mpc_lowcomm_universe_isend(mpc_lowcomm_peer_uid_t dest,
                               const void *data,
                               size_t size,
                               int tag,
                               mpc_lowcomm_communicator_t remote_world,
                               mpc_lowcomm_request_t *req)
{
	if (mpc_lowcomm_peer_get_rank(dest) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, size, NULL, NULL, 0);
		return MPC_LOWCOMM_SUCCESS;
	}

	if (!mpc_lowcomm_monitor_peer_exists(dest))
	{
		mpc_common_debug_warning("%s : peer %s does not appear to exist",
			__FUNCTION__,
			mpc_lowcomm_peer_format(dest));
		return MPC_LOWCOMM_ERROR;
	}

	mpc_lowcomm_ptp_message_t *msg = mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, data, size);


	mpc_lowcomm_ptp_message_header_init(msg,
		tag,
		remote_world,
		mpc_lowcomm_monitor_get_uid(),
		dest,
		req,
		size,
		MPC_LOWCOMM_MESSAGE_UNIVERSE,
		MPC_DATATYPE_IGNORE,
		REQUEST_SEND);

	// mpc_common_debug_error("MSG TO comm %llu (LOCAL %llu)", SCTK_MSG_COMMUNICATOR_ID(msg),
	// mpc_lowcomm_get_comm_world_id());

	mpc_lowcomm_ptp_message_send(msg);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_universe_irecv(mpc_lowcomm_peer_uid_t src,
                               const void *data,
                               size_t size,
                               int tag,
                               mpc_lowcomm_request_t *req)
{
	if (mpc_lowcomm_peer_get_rank(src) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, size, NULL, NULL, 0);
		return MPC_LOWCOMM_SUCCESS;
	}

	if (mpc_lowcomm_peer_get_rank(src) != MPC_ANY_SOURCE)
	{
		if (!mpc_lowcomm_monitor_peer_exists(src))
		{
			mpc_common_debug_warning("%s : peer %s does not appear to exist",
				__FUNCTION__,
				mpc_lowcomm_peer_format(src));
			return MPC_LOWCOMM_ERROR;
		}
	}

	mpc_lowcomm_ptp_message_t *msg = mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, data, size);


	mpc_lowcomm_ptp_message_header_init(msg,
		tag,
		MPC_COMM_WORLD,
		src,
		mpc_lowcomm_monitor_get_uid(),
		req,
		size,
		MPC_LOWCOMM_MESSAGE_UNIVERSE,
		MPC_DATATYPE_IGNORE,
		REQUEST_RECV);

	mpc_lowcomm_ptp_message_recv(msg);

	return MPC_LOWCOMM_SUCCESS;
}

/*******************
 * PORT MANAGEMENT *
 *******************/

int mpc_lowcomm_open_port(char *port_name, int port_name_len)
{
	return mpc_lowcomm_monitor_open_port(port_name, port_name_len);
}

int mpc_lowcomm_close_port(const char *port_name)
{
	return mpc_lowcomm_monitor_close_port(port_name);
}

/*********************
 * Name Publishing   *
 *********************/

static inline int __publish_op(mpc_lowcomm_monitor_command_naming_t cmd,
                               const char *service_name,
                               const char *port_name)
{
	/* Publishing is setting a value in the root rank of my process set */
	mpc_lowcomm_monitor_retcode_t mret;

	mpc_lowcomm_monitor_response_t resp =
		mpc_lowcomm_monitor_naming(mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(),
			0),
			cmd,
			mpc_lowcomm_monitor_get_uid(),
			service_name,
			port_name,
			&mret);

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	int err = 0;


	if ((content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	    || (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS))
	{
		err = 1;
	}

	mpc_lowcomm_monitor_response_free(resp);

	return err ? MPC_LOWCOMM_ERROR : MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_publish_name(const char *service_name,
                             const char *port_name)
{
	return __publish_op(MPC_LOWCOMM_MONITOR_NAMING_PUT, service_name, port_name);
}

int mpc_lowcomm_unpublish_name(const char *service_name,
                               __UNUSED__ const char *port_name)
{
	return __publish_op(MPC_LOWCOMM_MONITOR_NAMING_DEL, service_name, "");
}

int mpc_lowcomm_lookup_name(const char *service_name,
                            char *port_name,
                            int port_name_len)
{
	int roots_count = 0;
	mpc_lowcomm_peer_uid_t *set_roots = _mpc_lowcomm_get_set_roots(&roots_count);

	int i;
	mpc_lowcomm_monitor_retcode_t mret = MPC_LOWCOMM_MONITOR_RET_ERROR;


	for (i = 0 ; i < roots_count && mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS; i++)
	{
		/* Now try to resolve starting from ourselves */
		mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_naming(set_roots[i],
			MPC_LOWCOMM_MONITOR_NAMING_GET,
			mpc_lowcomm_monitor_get_uid(),
			service_name,
			"",
			&mret);

		if (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			continue;
		}

		mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

		if (content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			mret = content->naming.retcode;
			continue;
		}

		snprintf(port_name, port_name_len, "%s", content->naming.port_name);
		mpc_lowcomm_monitor_response_free(resp);
	}

	_mpc_lowcomm_free_set_roots(set_roots);

	if (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_warning("%s could not find any matching peer for service %s",
			__func__, service_name);

		return MPC_LOWCOMM_ERROR;
	}

	return MPC_LOWCOMM_SUCCESS;
}

/********************
 * SETUP & TEARDOWN *
 ********************/

void mpc_launch_init_runtime();

void mpc_lowcomm_init()
{
#ifndef MPC_Threads
		mpc_launch_init_runtime();
		mpc_common_init_trigger("VP Thread Start");
#endif
	mpc_lowcomm_barrier(MPC_COMM_WORLD);
}

void mpc_lowcomm_release()
{
	mpc_lowcomm_barrier(MPC_COMM_WORLD);
#ifndef MPC_Threads
		mpc_common_init_trigger("VP Thread End");
		mpc_common_init_trigger("After Ending VPs");
		mpc_launch_release_runtime();
#endif
}

/* Old interface for retro-compat */

void mpc_lowcomm_libmode_init()
{
	mpc_lowcomm_init();
}

void mpc_lowcomm_libmode_release()
{
	mpc_lowcomm_release();
}

static void __lowcomm_init_per_task()
{
	int task_rank = mpc_common_get_task_rank();

	/* We call for all threads as some
	 * progress threads may need buffered headers */
	if (task_rank >= 0)
	{
		_mpc_lowcomm_datatype_init_common();

		// Create Self and World communicators first !
		mpc_lowcomm_termination_barrier();
		mpc_lowcomm_init_protocol_per_task();
		_mpc_lowcomm_communicator_init_task(task_rank);
		mpc_lowcomm_termination_barrier();

		_mpc_lowcomm_pset_bootstrap();

		mpc_lowcomm_allocmem_pool_init();
		lowcomm_osc_init();
		mpc_lowcomm_termination_barrier();
	}
}

static void __lowcomm_release_per_task()
{
	int task_rank = mpc_common_get_task_rank();

	if (task_rank >= 0)
	{
		mpc_common_nodebug("mpc_lowcomm_termination_barrier");
		mpc_lowcomm_termination_barrier();

		lowcomm_osc_deinit();
		_mpc_lowcomm_release_psets();

		_mpc_lowcomm_communicator_release_task();

		mpc_lowcomm_release_protocol_per_task();

		mpc_common_nodebug("mpc_lowcomm_termination_barrier done");
		_mpc_lowcomm_monitor_teardown_per_task();
	}
	else
	{
		not_reachable();
	}
}

/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/

#ifdef MPC_Allocator

static size_t __mpc_memory_allocation_hook(size_t size_origin)
{
	UNUSED(size_origin);
	return 0;
}

void __mpc_memory_free_hook(void *ptr, size_t size)
{
	// mpc_common_debug_error("FREE %p size %ld", ptr, size);

	if (mpc_lowcomm_allocmem_is_in_pool(ptr))
	{
		// mpc_common_debug_error("FREE pool");
		mpc_lowcomm_allocmem_pool_free_size(ptr, size);
	}

	UNUSED(ptr);
	UNUSED(size);
}

#endif



static void __initialize_drivers()
{
#ifdef MPC_Allocator
		sctk_net_memory_allocation_hook_register(__mpc_memory_allocation_hook);
		sctk_net_memory_free_hook_register(__mpc_memory_free_hook);
#endif


	int rc;

	_mpc_lowcomm_monitor_setup();
	_mpc_lowcomm_checksum_init();

	mpc_lowcomm_protocol_init_driver();
	mpc_common_progress_register(mpc_comm_progress);
	__init_request_null();

	/* This creates the world group */
	_mpc_lowcomm_group_create_world();
}

static void __finalize_driver()
{
	// FIXME: error checking
	_mpc_lowcomm_communicator_release();
	_mpc_lowcomm_group_release_world();
	mpc_lowcomm_protocol_finalize_driver();
	_mpc_lowcomm_monitor_teardown();
}

#ifdef MPC_USE_DMTCP
static void __initialize_ft(void)
{
	sctk_ft_init();
}

#endif



void mpc_lowcomm_registration() __attribute__((constructor));

void mpc_lowcomm_registration()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Base Runtime Init Done", "LowComm Init", __initialize_drivers, 16);

	    mpc_common_init_callback_register("Base Runtime Finalize",
		"Release LowComm",
		__finalize_driver,
		16);

	    mpc_common_init_callback_register("Base Runtime Init Done",
		"Block sigpipe",
		mpc_common_helper_ignore_sigpipe,
		0);

	    mpc_common_init_callback_register("VP Thread Start",
		"MPC Message Passing Init per Task",
		__lowcomm_init_per_task,
		0);

	    mpc_common_init_callback_register("VP Thread End",
		"MPC Message Passing Release",
		__lowcomm_release_per_task,
		0);

	    mpc_common_init_callback_register("Config Sources",
		"MPC Lowcomm Config Registration",
		_mpc_lowcomm_config_register,
		128);

	    mpc_common_init_callback_register("Config Checks",
		"MPC Lowcomm Config Validation",
		_mpc_lowcomm_config_validate,
		128);
#ifdef MPC_USE_DMTCP
		mpc_common_init_callback_register("Base Runtime Init with Config",
		"Initialize Fault-Tolerance",
		__initialize_ft,
		77);
#endif
}
