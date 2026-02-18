/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <lcp.h>

#include <core/lcp_header.h>
#include <core/lcp_request.h>
#include <core/lcp_context.h>
#include <core/lcp_manager.h>
#include <rma/lcp_rma.h>
#include <am/lcp_eager.h>

#include <bitmap.h>

#include "mpc_common_debug.h"

/** @addtogroup LCP_ATOMIC
 * @{
 */

/** @brief Union of the possible replies types */
typedef union lcp_atomic_reply_data_u
{
	int8_t   i8;  /**< 1 byte  signed integer */
	int16_t  i16; /**< 2 bytes signed integer */
	int32_t  i32; /**< 4 bytes signed integer */
	int64_t  i64; /**< 8 bytes signed integer */
	uint8_t  u8;  /**< 1 byte  unsigned integer */
	uint16_t u16; /**< 2 bytes unsigned integer */
	uint32_t u32; /**< 4 bytes unsigned integer */
	uint64_t u64; /**< 8 bytes unsigned integer */
	float    f32; /**< 4 bytes floating point number */
	double   f64; /**< 8 bytes floating point number */
} lcp_atomic_reply_data_t;

/** @brief Structured data to describe the reply to a fetch-like atomic operation */
typedef struct lcp_atomic_reply
{
	uint64_t                dest_uid; /**< UID of the initiator */
	uint64_t                msg_id;   /**< ID of the operation */
	lcp_atomic_reply_data_t result;   /**< Fetched data */
} lcp_atomic_reply_t;

/** Lock used for the fallback when atomic operation is not available */
mpc_common_spinlock_t lcp_atomic_global_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/**
 * @brief Packs a software atomic operation into an atomic header
 *
 * @param[out] dest Header of the communication containing the necessary information
 * @param[in]  data Request describing the data to pack into the atomic header
 *
 * @return          Size of the packed header in bytes
 */
static ssize_t lcp_atomic_pack(void *dest, void *data)
{
	lcp_request_t *req = (lcp_request_t *)data;
	lcp_ato_hdr_t *hdr = (lcp_ato_hdr_t *)dest;

	// FIXME: maybe a difference should be made between LCP protocol data and
	//       actual atomic payload.
	hdr->op          = req->send.ato.op;
	hdr->value       = req->send.ato.value;
	hdr->dest_tid    = req->task->tid;
	hdr->src_uid     = req->task->uid;
	hdr->remote_addr = req->send.ato.remote_addr;
	hdr->datatype    = req->send.ato.atomic_datatype;

	if (req->flags & LCP_REQUEST_USER_PROVIDED_REPLY_BUF)
	{
		hdr->msg_id = (uint64_t)req;
		if (req->send.ato.op == LCP_ATOMIC_OP_CSWAP)
		{
			hdr->compare = req->send.ato.compare;
		}
	}
	else
	{
		hdr->msg_id = 0;
	}


	return sizeof(*hdr);
}

/**
 * @brief Packs a reply to a software atomic operation into an atomic header
 *
 * @param[out] dest Header of the communication containing the necessary information
 * @param[in]  data Request describing the data to pack into the atomic header
 *
 * @return          Size of the packed header in bytes
 */
static ssize_t lcp_atomic_reply_pack(void *dest, void *data)
{
	lcp_request_t *      req = (lcp_request_t *)data;
	lcp_ato_reply_hdr_t *hdr = (lcp_ato_reply_hdr_t *)dest;

	hdr->msg_id = req->send.reply_ato.msg_id;
	hdr->result = req->send.reply_ato.result;

	return sizeof(*hdr);
}

/**
 * @brief Completes a software atomic operation
 *
 * @param[in] comp Completion function to call on completion
 */
static void lcp_atomic_complete(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
		state.comp);

	if ((req->flags & LCP_REQUEST_REMOTE_COMPLETED)
	    && (req->flags & LCP_REQUEST_LOCAL_COMPLETED))
	{
		req->status = MPC_LOWCOMM_SUCCESS;
		lcp_request_complete(req, send.send_cb, req->status, req->send.length);
	}
}

/**
 * @brief Generic function to send a software atomic operation
 *
 * @param[in] req Request containing the necessary information for the communication
 *
 * @retval        MPC_LOWCOMM_SUCCESS
 * @retval        MPC_LOWCOMM_ERROR
 */
int lcp_atomic_sw(lcp_request_t *req)
{
	int     rc = MPC_LOWCOMM_SUCCESS;
	ssize_t packed_size;

	mpc_common_nodebug(
		"LCP ATO SW: perform op. req=%p, task=%p, tid=%d, remote addr=%p, op=%s, compare=%llu, value=%llu",
		req,
		req->task,
		req->task->tid,
		req->send.ato.remote_addr,
		lcp_ato_sw_decode_op(req->send.ato.op),
		req->send.ato.compare,
		req->send.ato.value);

	packed_size = lcp_send_eager_bcopy(req, lcp_atomic_pack, LCP_AM_ID_ATOMIC);

	if (packed_size < 0)
	{
		rc = MPC_LOWCOMM_ERROR;
	}

	req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
	if (!(req->flags & LCP_REQUEST_USER_PROVIDED_REPLY_BUF))
	{
		req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
	}

	lcp_atomic_complete(&req->state.comp);

	return rc;
}

/**
 * @brief Generic function to send the reply of a completed fetch-like atomic operation
 *
 * @param[in] mngr      Manager used for the communication
 * @param[in] task      Task identifier of the initiator, destination of communication
 * @param[in] ato_reply Packed header containing the data to communicate
 *
 * @retval MPC_LOWCOMM_SUCCESS upon success
 * @retval errorcode otherwise
 */
static int lcp_atomic_reply(lcp_manager_h mngr, lcp_task_h task, lcp_atomic_reply_t *ato_reply)
{
	int            rc;
	ssize_t        payload_size;
	lcp_ep_h       ep;
	lcp_request_t *reply_req;

	/* Get endpoint */
	rc = lcp_ep_get_or_create(mngr, ato_reply->dest_uid, &ep, 0);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	reply_req = lcp_request_get(task);
	if (reply_req == NULL)
	{
		mpc_common_debug_error("LCP ATO SW: could not allocate reply request.");
		goto err;
	}
	reply_req->send.reply_ato.result = *(uint64_t *)&ato_reply->result;
	reply_req->send.reply_ato.msg_id = ato_reply->msg_id;
	reply_req->send.ep = ep;

	mpc_common_nodebug("LCP ATO: send atomic reply. dest_tid=%d, result=%llu", ato_reply->dest_uid, ato_reply->result);

	payload_size = lcp_send_eager_bcopy(reply_req, lcp_atomic_reply_pack, LCP_AM_ID_ATOMIC_REPLY);
	if (payload_size < 0)
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	reply_req->flags |= LCP_REQUEST_LOCAL_COMPLETED | LCP_REQUEST_REMOTE_COMPLETED;

	/* Complete ack request */
	lcp_request_complete(reply_req, send.send_cb, reply_req->status, reply_req->send.length);

err:
	return rc;
}

const lcp_atomic_proto_t ato_sw_proto =
{
	.send_fetch = lcp_atomic_sw,
	.send_cswap = lcp_atomic_sw,
	.send_post  = lcp_atomic_sw,
};

/**
 * @brief Executes the requested atomic operation with the provided data
 *
 * This function handles local atomic operations which have been provided through an Active Message or shm like
 * communication.
 *
 * @param[in] arg    Manager used for the communication
 * @param[in] data   Header of the communication containing the necessary information
 * @param[in] length Size of the datatype in bytes (UNUSED)
 * @param[in] flags  (UNUSED)
 *
 * @retval MPC_LOWCOMM_SUCCESS on success
 * @retval MPC_LOWCOMM_ERROR otherwise
 */
static int lcp_atomic_handler(void *arg, void *data, size_t length, unsigned flags)
{
	UNUSED(length);
	UNUSED(flags);
	int                i = 0, rc = MPC_LOWCOMM_SUCCESS;
	lcp_manager_h      mngr = (lcp_manager_h)arg;
	lcp_context_h      ctx  = mngr->ctx;
	lcp_ato_hdr_t *    hdr  = (lcp_ato_hdr_t *)data;
	lcp_task_h         task;
	lcp_atomic_reply_t reply;

	reply.dest_uid = hdr->src_uid;
	reply.msg_id   = hdr->msg_id;
	const lcp_atomic_reply_data_t value   = *(lcp_atomic_reply_data_t *)&hdr->value;
	lcp_atomic_reply_data_t       compare = *(lcp_atomic_reply_data_t *)&hdr->compare;

	// NOTE: In the following, we use reinterpret cast to retrieve the underlying data
	switch (hdr->op)
	{
	case LCP_ATOMIC_OP_ADD: {
		switch (hdr->datatype)
		{
#if __clang__
			case LCP_ATOMIC_DT_FLOAT:
				reply.result.f32 = atomic_fetch_add((_Atomic float *)hdr->remote_addr, value.f32);
				break;

			case LCP_ATOMIC_DT_DOUBLE:
				reply.result.f64 = atomic_fetch_add((_Atomic double *)hdr->remote_addr, value.f64);
				break;
#else
			case LCP_ATOMIC_DT_FLOAT:
				mpc_common_spinlock_lock(&lcp_atomic_global_lock);
				reply.result.f32 = *(volatile float *)hdr->remote_addr;
				*(volatile float *)hdr->remote_addr += value.f32;
				mpc_common_spinlock_unlock(&lcp_atomic_global_lock);
				break;

			case LCP_ATOMIC_DT_DOUBLE:
				mpc_common_spinlock_lock(&lcp_atomic_global_lock);
				reply.result.f64 = *(volatile float *)hdr->remote_addr;
				*(volatile float *)hdr->remote_addr += value.f64;
				mpc_common_spinlock_unlock(&lcp_atomic_global_lock);
				break;
#endif

			case LCP_ATOMIC_DT_INT8:
				reply.result.i8 = atomic_fetch_add((_Atomic int8_t *)hdr->remote_addr, value.i8);
				break;

			case LCP_ATOMIC_DT_INT16:
				reply.result.i16 = atomic_fetch_add((_Atomic int16_t *)hdr->remote_addr, value.i16);
				break;

			case LCP_ATOMIC_DT_INT32:
				reply.result.i32 = atomic_fetch_add((_Atomic int32_t *)hdr->remote_addr, value.i32);
				break;

			case LCP_ATOMIC_DT_INT64:
				reply.result.i64 = atomic_fetch_add((_Atomic int64_t *)hdr->remote_addr, value.i64);
				break;

			case LCP_ATOMIC_DT_UINT8:
				reply.result.u8 = atomic_fetch_add((_Atomic uint8_t *)hdr->remote_addr, value.u8);
				break;

			case LCP_ATOMIC_DT_UINT16:
				reply.result.u16 = atomic_fetch_add((_Atomic uint16_t *)hdr->remote_addr, value.u16);
				break;

			case LCP_ATOMIC_DT_UINT32:
				reply.result.u32 = atomic_fetch_add((_Atomic uint32_t *)hdr->remote_addr, value.u32);
				break;

			case LCP_ATOMIC_DT_UINT64:
				reply.result.u64 = atomic_fetch_add((_Atomic uint64_t *)hdr->remote_addr, value.u64);
				break;

			default:
				mpc_common_debug_fatal(
					"LCP ATO SW: datatype not supported for ADD operation with the current compiler");
				break;
		}
		break;
	}

	case LCP_ATOMIC_OP_SUB: {
		switch (hdr->datatype)
		{
#if __clang__
			case LCP_ATOMIC_DT_FLOAT:
				reply.result.f32 = atomic_fetch_sub((_Atomic float *)hdr->remote_addr, value.f32);
				break;

			case LCP_ATOMIC_DT_DOUBLE:
				reply.result.f64 = atomic_fetch_sub((_Atomic double *)hdr->remote_addr, value.f64);
				break;
#else
			case LCP_ATOMIC_DT_FLOAT:
				mpc_common_spinlock_lock(&lcp_atomic_global_lock);
				reply.result.f32 = *(volatile float *)hdr->remote_addr;
				*(volatile float *)hdr->remote_addr -= value.f32;
				mpc_common_spinlock_unlock(&lcp_atomic_global_lock);
				break;

			case LCP_ATOMIC_DT_DOUBLE:
				mpc_common_spinlock_lock(&lcp_atomic_global_lock);
				reply.result.f64 = *(volatile float *)hdr->remote_addr;
				*(volatile float *)hdr->remote_addr -= value.f64;
				mpc_common_spinlock_unlock(&lcp_atomic_global_lock);
				break;
#endif

			case LCP_ATOMIC_DT_INT8:
				reply.result.i8 = atomic_fetch_sub((_Atomic int8_t *)hdr->remote_addr, value.i8);
				break;

			case LCP_ATOMIC_DT_INT16:
				reply.result.i16 = atomic_fetch_sub((_Atomic int16_t *)hdr->remote_addr, value.i16);
				break;

			case LCP_ATOMIC_DT_INT32:
				reply.result.i32 = atomic_fetch_sub((_Atomic int32_t *)hdr->remote_addr, value.i32);
				break;

			case LCP_ATOMIC_DT_INT64:
				reply.result.i64 = atomic_fetch_sub((_Atomic int64_t *)hdr->remote_addr, value.i64);
				break;

			case LCP_ATOMIC_DT_UINT8:
				reply.result.u8 = atomic_fetch_sub((_Atomic uint8_t *)hdr->remote_addr, value.u8);
				break;

			case LCP_ATOMIC_DT_UINT16:
				reply.result.u16 = atomic_fetch_sub((_Atomic uint16_t *)hdr->remote_addr, value.u16);
				break;

			case LCP_ATOMIC_DT_UINT32:
				reply.result.u32 = atomic_fetch_sub((_Atomic uint32_t *)hdr->remote_addr, value.u32);
				break;

			case LCP_ATOMIC_DT_UINT64:
				reply.result.u64 = atomic_fetch_sub((_Atomic uint64_t *)hdr->remote_addr, value.u64);
				break;

			default:
				mpc_common_debug_fatal(
					"LCP ATO SW: datatype not supported for SUB operation with the current compiler");
				break;
		}
		break;
	}

	case LCP_ATOMIC_OP_AND: {
		switch (hdr->datatype)
		{
		case LCP_ATOMIC_DT_INT8:
			reply.result.i8 = atomic_fetch_and((_Atomic int8_t *)hdr->remote_addr, value.i8);
			break;

		case LCP_ATOMIC_DT_INT16:
			reply.result.i16 = atomic_fetch_and((_Atomic int16_t *)hdr->remote_addr, value.i16);
			break;

		case LCP_ATOMIC_DT_INT32:
			reply.result.i32 = atomic_fetch_and((_Atomic int32_t *)hdr->remote_addr, value.i32);
			break;

		case LCP_ATOMIC_DT_INT64:
			reply.result.i64 = atomic_fetch_and((_Atomic int64_t *)hdr->remote_addr, value.i64);
			break;

		case LCP_ATOMIC_DT_UINT8:
			reply.result.u8 = atomic_fetch_and((_Atomic uint8_t *)hdr->remote_addr, value.u8);
			break;

		case LCP_ATOMIC_DT_UINT16:
			reply.result.u16 = atomic_fetch_and((_Atomic uint16_t *)hdr->remote_addr, value.u16);
			break;

		case LCP_ATOMIC_DT_UINT32:
			reply.result.u32 = atomic_fetch_and((_Atomic uint32_t *)hdr->remote_addr, value.u32);
			break;

		case LCP_ATOMIC_DT_UINT64:
			reply.result.u64 = atomic_fetch_and((_Atomic uint64_t *)hdr->remote_addr, value.u64);
			break;

		default:
			mpc_common_debug_fatal("LCP ATO SW: datatype not supported for bitwise AND operation");
			break;
		}
		break;
	}

	case LCP_ATOMIC_OP_OR: {
		switch (hdr->datatype)
		{
		case LCP_ATOMIC_DT_INT8:
			reply.result.i8 = atomic_fetch_or((_Atomic int8_t *)hdr->remote_addr, value.i8);
			break;

		case LCP_ATOMIC_DT_INT16:
			reply.result.i16 = atomic_fetch_or((_Atomic int16_t *)hdr->remote_addr, value.i16);
			break;

		case LCP_ATOMIC_DT_INT32:
			reply.result.i32 = atomic_fetch_or((_Atomic int32_t *)hdr->remote_addr, value.i32);
			break;

		case LCP_ATOMIC_DT_INT64:
			reply.result.i64 = atomic_fetch_or((_Atomic int64_t *)hdr->remote_addr, value.i64);
			break;

		case LCP_ATOMIC_DT_UINT8:
			reply.result.u8 = atomic_fetch_or((_Atomic uint8_t *)hdr->remote_addr, value.u8);
			break;

		case LCP_ATOMIC_DT_UINT16:
			reply.result.u16 = atomic_fetch_or((_Atomic uint16_t *)hdr->remote_addr, value.u16);
			break;

		case LCP_ATOMIC_DT_UINT32:
			reply.result.u32 = atomic_fetch_or((_Atomic uint32_t *)hdr->remote_addr, value.u32);
			break;

		case LCP_ATOMIC_DT_UINT64:
			reply.result.u64 = atomic_fetch_or((_Atomic uint64_t *)hdr->remote_addr, value.u64);
			break;

		default:
			mpc_common_debug_fatal("LCP ATO SW: datatype not supported for bitwise OR operation");
			break;
		}
		break;
	}

	case LCP_ATOMIC_OP_XOR: {
		switch (hdr->datatype)
		{
		case LCP_ATOMIC_DT_INT8:
			reply.result.i8 = atomic_fetch_xor((_Atomic int8_t *)hdr->remote_addr, value.i8);
			break;

		case LCP_ATOMIC_DT_INT16:
			reply.result.i16 = atomic_fetch_xor((_Atomic int16_t *)hdr->remote_addr, value.i16);
			break;

		case LCP_ATOMIC_DT_INT32:
			reply.result.i32 = atomic_fetch_xor((_Atomic int32_t *)hdr->remote_addr, value.i32);
			break;

		case LCP_ATOMIC_DT_INT64:
			reply.result.i64 = atomic_fetch_xor((_Atomic int64_t *)hdr->remote_addr, value.i64);
			break;

		case LCP_ATOMIC_DT_UINT8:
			reply.result.u8 = atomic_fetch_xor((_Atomic uint8_t *)hdr->remote_addr, value.u8);
			break;

		case LCP_ATOMIC_DT_UINT16:
			reply.result.u16 = atomic_fetch_xor((_Atomic uint16_t *)hdr->remote_addr, value.u16);
			break;

		case LCP_ATOMIC_DT_UINT32:
			reply.result.u32 = atomic_fetch_xor((_Atomic uint32_t *)hdr->remote_addr, value.u32);
			break;

		case LCP_ATOMIC_DT_UINT64:
			reply.result.u64 = atomic_fetch_xor((_Atomic uint64_t *)hdr->remote_addr, value.u64);
			break;

		default:
			mpc_common_debug_fatal("LCP ATO SW: datatype not supported for bitwise XOR operation");
			break;
		}
		break;
	}

	case LCP_ATOMIC_OP_SWAP: {
		switch (hdr->datatype)
		{
		case LCP_ATOMIC_DT_FLOAT:
			reply.result.f32 = atomic_exchange((_Atomic float *)hdr->remote_addr, value.f32);
			break;

		case LCP_ATOMIC_DT_DOUBLE:
			reply.result.f64 = atomic_exchange((_Atomic double *)hdr->remote_addr, value.f64);
			break;

		case LCP_ATOMIC_DT_INT8:
			reply.result.i8 = atomic_exchange((_Atomic int8_t *)hdr->remote_addr, value.i8);
			break;

		case LCP_ATOMIC_DT_INT16:
			reply.result.i16 = atomic_exchange((_Atomic int16_t *)hdr->remote_addr, value.i16);
			break;

		case LCP_ATOMIC_DT_INT32:
			reply.result.i32 = atomic_exchange((_Atomic int32_t *)hdr->remote_addr, value.i32);
			break;

		case LCP_ATOMIC_DT_INT64:
			reply.result.i64 = atomic_exchange((_Atomic int64_t *)hdr->remote_addr, value.i64);
			break;

		case LCP_ATOMIC_DT_UINT8:
			reply.result.u8 = atomic_exchange((_Atomic uint8_t *)hdr->remote_addr, value.u8);
			break;

		case LCP_ATOMIC_DT_UINT16:
			reply.result.u16 = atomic_exchange((_Atomic uint16_t *)hdr->remote_addr, value.u16);
			break;

		case LCP_ATOMIC_DT_UINT32:
			reply.result.u32 = atomic_exchange((_Atomic uint32_t *)hdr->remote_addr, value.u32);
			break;

		case LCP_ATOMIC_DT_UINT64:
			reply.result.u64 = atomic_exchange((_Atomic uint64_t *)hdr->remote_addr, value.u64);
			break;
		}
		break;
	}

	case LCP_ATOMIC_OP_CSWAP: {
		switch (hdr->datatype)
		{
		case LCP_ATOMIC_DT_FLOAT:
			atomic_compare_exchange_strong((_Atomic float *)hdr->remote_addr, &compare.f32, value.f32);
			reply.result.f32 = compare.f32;
			break;

		case LCP_ATOMIC_DT_DOUBLE:
			atomic_compare_exchange_strong((_Atomic double *)hdr->remote_addr, &compare.f64, value.f64);
			reply.result.f64 = compare.f64;
			break;

		case LCP_ATOMIC_DT_INT8:
			atomic_compare_exchange_strong((_Atomic int8_t *)hdr->remote_addr, &compare.i8, value.i8);
			reply.result.i8 = compare.i8;
			break;

		case LCP_ATOMIC_DT_INT16:
			atomic_compare_exchange_strong((_Atomic int16_t *)hdr->remote_addr, &compare.i16, value.i16);
			reply.result.i16 = compare.i16;
			break;

		case LCP_ATOMIC_DT_INT32:
			atomic_compare_exchange_strong((_Atomic int32_t *)hdr->remote_addr, &compare.i32, value.i32);
			reply.result.i32 = compare.i32;
			break;

		case LCP_ATOMIC_DT_INT64:
			atomic_compare_exchange_strong((_Atomic int64_t *)hdr->remote_addr, &compare.i64, value.i64);
			reply.result.i64 = compare.i64;
			break;

		case LCP_ATOMIC_DT_UINT8:
			atomic_compare_exchange_strong((_Atomic uint8_t *)hdr->remote_addr, &compare.u8, value.u8);
			reply.result.u8 = compare.u8;
			break;

		case LCP_ATOMIC_DT_UINT16:
			atomic_compare_exchange_strong((_Atomic uint16_t *)hdr->remote_addr, &compare.u16, value.u16);
			reply.result.u16 = compare.u16;
			break;

		case LCP_ATOMIC_DT_UINT32:
			atomic_compare_exchange_strong((_Atomic uint32_t *)hdr->remote_addr, &compare.u32, value.u32);
			reply.result.u32 = compare.u32;
			break;

		case LCP_ATOMIC_DT_UINT64:
			atomic_compare_exchange_strong((_Atomic uint64_t *)hdr->remote_addr, &compare.u64, value.u64);
			reply.result.u64 = compare.u64;
			break;
		}

		break;
	}
	}

	if (hdr->msg_id != 0)
	{
		// FIXME: find another way to get to the task. Our only need is to get to
		//       pool of request that are only associated to the tasks for now.
		while (ctx->tasks[i] == NULL)
		{
			i++;
		}

		task = lcp_context_task_get(mngr->ctx, i);
		if (task == NULL)
		{
			mpc_common_debug_error("LCP ATO: could not find any task");
			rc = MPC_LOWCOMM_ERROR;
			goto err;
		}

		rc = lcp_atomic_reply(mngr, task, &reply);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}

err:
	return rc;
}

/**
 * @brief Handles the response of fetch-like atomic operations
 *
 * This function replies with the data to the remote initiator.
 *
 * @param[in] arg    Manager used for the communication (UNUSED)
 * @param[in] data   Header of the communication containing the necessary information
 * @param[in] length Size of the datatype in bytes (UNUSED)
 * @param[in] flags  (UNUSED)
 *
 * @return           MPC_LOWCOMM_SUCCESS
 */
static int lcp_atomic_reply_handler(void *arg, void *data, size_t length, unsigned flags)
{
	UNUSED(length);
	UNUSED(flags);
	UNUSED(arg);
	lcp_ato_reply_hdr_t *hdr = (lcp_ato_reply_hdr_t *)data;

	lcp_request_t *req = (lcp_request_t *)hdr->msg_id;

	memcpy(req->send.ato.reply_buffer, &hdr->result, req->send.ato.reply_size);

	req->flags |= LCP_REQUEST_REMOTE_COMPLETED;

	lcp_atomic_complete(&req->state.comp);

	return MPC_LOWCOMM_SUCCESS;
}

LCP_DEFINE_AM(LCP_AM_ID_ATOMIC,       lcp_atomic_handler,       0);
LCP_DEFINE_AM(LCP_AM_ID_ATOMIC_REPLY, lcp_atomic_reply_handler, 0);

/** @} */
