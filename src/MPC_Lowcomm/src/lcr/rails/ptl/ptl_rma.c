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

#include "mpc_config.h"

#ifdef MPC_USE_PORTALS

#include "ptl.h"

#include "rail.h"

	/*################################################
	 * Memory registration.
	 *################################################*/

	int lcr_ptl_mem_register(struct sctk_rail_info_s *rail,
	                         struct sctk_rail_pin_ctx_list *list,
	                         const void *addr, size_t size, unsigned flags)
	{
		int rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_rail_info_t *srail = &rail->network.ptl;
		lcr_ptl_mem_ctx_t *  ctx   = &list->pin.ptl;

		/* First, check flags error. */
		if (!(flags & (LCR_IFACE_REGISTER_MEM_DYN
		               | LCR_IFACE_REGISTER_MEM_STAT)))
		{
			mpc_common_debug_error("LCR PTL: flags mush specify at least "
				                   "on registration type.");
			rc = MPC_LOWCOMM_ERROR;
			goto err;
		}
		else if (flags & LCR_IFACE_REGISTER_MEM_DYN
		         && flags & LCR_IFACE_REGISTER_MEM_STAT)
		{
			mpc_common_debug_error("LCR PTL: flags are mutually exclusive. Choose "
				                   "either MEM_DYN or MEM_STAT.");
			rc = MPC_LOWCOMM_ERROR;
			goto err;
		}

		/* Init packable data. */
		ctx->size  = size;
		ctx->start = addr;

		if (flags & LCR_IFACE_REGISTER_MEM_DYN)
		{
			ctx->mem = srail->net.rma.dynamic_mem;
		}
		else
		{
			ctx->mem = mpc_mpool_pop(srail->net.rma.mem_mp);
			if (ctx->mem == NULL)
			{
				mpc_common_debug_error("LCR PTL: could not allocate memory handle.");
				rc = MPC_LOWCOMM_ERROR;
				goto err;
			}

			rc = lcr_ptl_mem_activate(srail,
				LCR_PTL_MEM_STATIC,
				(void *)addr,
				size,
				ctx->mem);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			mpc_common_spinlock_lock(&srail->net.rma.lock);
			mpc_list_push_head(&srail->net.rma.poll_list, &ctx->mem->elem);
			mpc_common_spinlock_unlock(&srail->net.rma.lock);
		}

		ctx->match = ctx->mem->match;

		mpc_common_nodebug("LCR PTL: memory registration. srail=%p, mem=%p, addr=%p, size=%llu, match=%llu",
			srail, ctx->mem, addr, size, ctx->match);
err:
		return rc;
	}

	int lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail,
	                           struct sctk_rail_pin_ctx_list *list)
	{
		int rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_rail_info_t *srail = &rail->network.ptl;
		lcr_ptl_mem_ctx_t *  ctx   = &list->pin.ptl;

		mpc_common_nodebug("LCR PTL: memory deregistration. srail=%p, mem=%p, addr=%p, size=%llu, match=%llu",
			srail, ctx->mem, ctx->start, ctx->size, ctx->match);

		if (ctx->mem->lifetime == LCR_PTL_MEM_STATIC)
		{
			mpc_common_spinlock_lock(&srail->net.rma.lock);
			mpc_list_del(&ctx->mem->elem);
			mpc_common_spinlock_unlock(&srail->net.rma.lock);

			// NOTE: make sure resources associated with the memory are being freed
			//      after it has been removed from the list.
			rc = lcr_ptl_mem_deactivate(ctx->mem);

			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			mpc_mpool_push(ctx->mem);
		}

err:
		return rc;
	}

	int lcr_ptl_pack_rkey(sctk_rail_info_t *rail, lcr_memp_t *memp, void *dest)
	{
		UNUSED(rail);
		void *p = dest;

		/* Serialize data. */
		*(uint64_t *)p = (uint64_t)memp->pin.ptl.start;
		p += sizeof(uint64_t);
		*(ptl_match_bits_t *)p = memp->pin.ptl.match;

		return sizeof(uint64_t) + sizeof(ptl_match_bits_t);
	}

	int lcr_ptl_unpack_rkey(sctk_rail_info_t *rail, lcr_memp_t *memp, void *dest)
	{
		UNUSED(rail);
		void *p = dest;

		/* deserialize data */
		memp->pin.ptl.start = (void *)(*(uint64_t *)p);
		p += sizeof(uint64_t);
		memp->pin.ptl.match = *(ptl_match_bits_t *)p;

		return sizeof(uint64_t) + sizeof(ptl_match_bits_t);
	}

	/*################################################
	 * Flush operations.
	 *################################################*/

	int64_t lcr_ptl_flush_txq(lcr_ptl_mem_t *mem, lcr_ptl_txq_t *txq, int64_t completed)
	{
		UNUSED(mem);
		lcr_ptl_op_t *   op;
		mpc_queue_iter_t iter;
		int64_t          still_pending = 0;

		mpc_common_spinlock_lock(&txq->lock);

		/* Loop over pending operations and complete if identifier is lower than number of completed operations. */
		mpc_queue_for_each_safe(op, iter, lcr_ptl_op_t, &txq->ops, elem)
		{
			if (op->type == LCR_PTL_OP_RMA_FLUSH)
			{
				// If the flush operations is first in queue
				if (still_pending == 0)
				{
					// Remove it from the queue and complete it
					mpc_queue_del_iter(&txq->ops, iter);
					lcr_ptl_complete_op(op);
				}
			}
			else
			{
				// If operation is completed
				if (op->id < completed)
				{
					// Remove it from the queue
					mpc_queue_del_iter(&txq->ops, iter);
				}
				else
				{
					// Non completed ops still in the queue (the next flush op shall not be completed)
					still_pending++;
				}
			}
		}

		mpc_common_spinlock_unlock(&txq->lock);

		return still_pending;
	}

	/** @brief Performs the flush operation on the requested queue
	 *
	 * This function only adds the flush operation to the queue if it is not empty
	 *
	 * @param[in] mem      Memory to flush
	 * @param[in] ep       Endpoint to flush
	 * @param[in] flush_op Flush operation to track the flushing progress
	 *
	 * @retval MPC_LOWCOMM_IN_PROGRESS if the flush operation has been added to the queue
	 * @retval MPC_LOWCOMM_SUCCESS     if the flush operation has been completed
	 */
	static int lcr_ptl_exec_flush_mem_ep(lcr_ptl_mem_t *mem, lcr_ptl_ep_info_t *ep, lcr_ptl_op_t *flush_op)
	{
		int rc = MPC_LOWCOMM_SUCCESS;

		lcr_ptl_txq_t *txq = &mem->txqt[ep->idx];

		const int64_t completed     = lcr_ptl_poll_mem(mem);
		const int64_t still_pending = lcr_ptl_flush_txq(mem, txq, completed);

		// If the queue is not empty
		if (still_pending > 0)
		{
			// Append the flush op to the queue
			mpc_queue_push(&txq->ops, &flush_op->elem);
			rc = MPC_LOWCOMM_IN_PROGRESS;
		}
		else
		{
			// Complete instantly the flush op
			lcr_ptl_complete_op(flush_op);
		}

		return rc;
	}

	int lcr_ptl_flush_mem_ep(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *ep,
	                         struct sctk_rail_pin_ctx_list *list, lcr_completion_t *comp, unsigned flags)
	{
		UNUSED(flags);
		int                  rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_op_t *       op;
		lcr_ptl_rail_info_t *srail  = &rail->network.ptl;
		lcr_ptl_ep_info_t *  ptl_ep = &ep->data.ptl;
		lcr_ptl_mem_t *      mem    = list->pin.ptl.mem;

		op = mpc_mpool_pop(srail->iface_ops);
		if (op == NULL)
		{
			mpc_common_debug_warning("LCR PTL: maximum outstanding operations.");
			rc = MPC_LOWCOMM_NO_RESOURCE;
			goto err;
		}

		assert(mem != NULL && ep != NULL);

		_lcr_ptl_init_op_common(op, 0, mem->mdh,
			&mem->success,
			ptl_ep->addr.id,
			ptl_ep->addr.pte.rma,
			LCR_PTL_OP_RMA_FLUSH,
			0, comp,
			NULL);

		mpc_common_spinlock_lock(&ptl_ep->lock);
		// Only one queue to flush
		op->flush.outstandings = 1;

		rc = lcr_ptl_exec_flush_mem_ep(mem, ptl_ep, op);

		mpc_common_spinlock_unlock(&ptl_ep->lock);
err:
		return rc;
	}

	/** @brief Performs the flush operation on the requested endpoint
	 *
	 * This function only adds the flush operation to the queues associated with the endpoint if not empty
	 *
	 * @param[in] srail    Rail information
	 * @param[in] ep       Endpoint to flush
	 * @param[in] flush_op Flush operation to track the flushing progress
	 *
	 * @retval MPC_LOWCOMM_IN_PROGRESS if the flush operation has been added to at least one queue
	 * @retval MPC_LOWCOMM_SUCCESS     if the flush operation has been completed (all queues are empty)
	 */
	static int lcr_ptl_exec_flush_ep(lcr_ptl_rail_info_t *srail, lcr_ptl_ep_info_t *ep, lcr_ptl_op_t *flush_op)
	{
		int            rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_mem_t *mem;

		mpc_list_for_each(mem, &srail->net.rma.poll_list, lcr_ptl_mem_t, elem)
		{
			/* Poll memory. */
			const int64_t completed = lcr_ptl_poll_mem(mem);

			// Progress the queue and see if it is empty
			const int64_t still_pending = lcr_ptl_flush_txq(mem, &mem->txqt[ep->idx], completed);

			// If the queue is not empty
			if (still_pending > 0)
			{
				// Push the flush op to the tail of the queue
				mpc_queue_push(&mem->txqt->ops, &flush_op->elem);
				rc = MPC_LOWCOMM_IN_PROGRESS;
			}
			else
			{
				// Complete one time the flush op (it has to be completed the number of queues time)
				lcr_ptl_complete_op(flush_op);
			}
		}

		return rc;
	}

	int lcr_ptl_flush_ep(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *ep, lcr_completion_t *comp, unsigned flags)
	{
		UNUSED(flags);
		int                  rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_op_t *       op;
		lcr_ptl_rail_info_t *srail  = &rail->network.ptl;
		lcr_ptl_ep_info_t *  ptl_ep = &ep->data.ptl;

		op = mpc_mpool_pop(srail->iface_ops);
		if (op == NULL)
		{
			mpc_common_debug_warning("LCR PTL: maximum outstanding "
				                     "operations.");
			rc = MPC_LOWCOMM_NO_RESOURCE;
			goto err;
		}

		_lcr_ptl_init_op_common(op, 0, PTL_INVALID_HANDLE,
			NULL,
			ptl_ep->addr.id,
			ptl_ep->addr.pte.rma,
			LCR_PTL_OP_RMA_FLUSH,
			0, comp,
			NULL);

		assert(ep != NULL);

		mpc_common_spinlock_lock(&srail->net.rma.lock);
		// Compute the number of queues to flush
		const int outstandings = mpc_list_length(&srail->net.rma.poll_list);
		op->flush.outstandings = outstandings > 1 ? outstandings : 1;

		// If no queue has to be flushed
		if (outstandings == 0)
		{
			// Complete instantly the flush op
			lcr_ptl_complete_op(op);
		}
		else
		{
			// Flush the found queues
			rc = lcr_ptl_exec_flush_ep(srail, ptl_ep, op);
		}

		mpc_common_spinlock_unlock(&srail->net.rma.lock);
err:
		return rc;
	}

	/** @brief Performs the flush operation on the requested memory
	 *
	 * This function only adds the flush operation to the queues associated with the memory if not empty
	 *
	 * @param[in] srail    Rail information
	 * @param[in] mem      Memory to flush
	 * @param[in] flush_op Flush operation to track the flushing progress
	 *
	 * @retval MPC_LOWCOMM_IN_PROGRESS if the flush operation has been added to at least one queue
	 * @retval MPC_LOWCOMM_SUCCESS     if the flush operation has been completed (all queues are empty)
	 */
	static int lcr_ptl_exec_flush_mem(lcr_ptl_rail_info_t *srail, lcr_ptl_mem_t *mem, lcr_ptl_op_t *flush_op)
	{
		int rc = MPC_LOWCOMM_SUCCESS;
		int i, num_txqs = 0;

		const int64_t completed = lcr_ptl_poll_mem(mem);

		// Get the number of operation queues using this memory
		num_txqs = atomic_load(&srail->num_eps);
		// Loop over all endpoints
		for (i = 0; i < num_txqs; i++)
		{
			// Progress the queue and see if it is empty
			const int64_t still_pending = lcr_ptl_flush_txq(mem, &mem->txqt[i], completed);
			// If the queue is not empty
			if (still_pending > 0)
			{
				// Push the flush operation to the queue
				mpc_queue_push(&mem->txqt->ops, &flush_op->elem);
				rc = MPC_LOWCOMM_IN_PROGRESS;
			}
			else
			{
				// Complete one time the flush op (it has to be completed the number of queues time)
				lcr_ptl_complete_op(flush_op);
			}
		}

		return rc;
	}

	int lcr_ptl_flush_mem(sctk_rail_info_t *rail, struct sctk_rail_pin_ctx_list *list,
	                      lcr_completion_t *comp, unsigned flags)
	{
		UNUSED(flags);
		int                  rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_op_t *       op;
		lcr_ptl_rail_info_t *srail = &rail->network.ptl;
		lcr_ptl_mem_t *      mem   = list->pin.ptl.mem;

		op = mpc_mpool_pop(srail->iface_ops);
		if (op == NULL)
		{
			mpc_common_debug_warning("LCR PTL: maximum outstanding operations.");
			rc = MPC_LOWCOMM_NO_RESOURCE;
			goto err;
		}

		assert(mem != NULL);
		_lcr_ptl_init_op_common(op, 0, mem->mdh,
			&mem->success,
			LCR_PTL_PROCESS_ANY,
			srail->net.rma.pti,
			LCR_PTL_OP_RMA_FLUSH,
			0, comp,
			NULL);

		mpc_common_spinlock_lock(&mem->lock);
		// Compute the number of queues to flush
		const int outstandings = atomic_load(&srail->num_eps);
		op->flush.outstandings = outstandings > 1 ? outstandings : 1;

		// If no queue has to be flushed
		if (outstandings == 0)
		{
			// Complete instantly the flush op
			lcr_ptl_complete_op(op);
		}
		else
		{
			// Flush the found queues
			rc = lcr_ptl_exec_flush_mem(srail, mem, op);
		}

		mpc_common_spinlock_unlock(&mem->lock);
err:
		return rc;
	}

	/** @brief Performs the flush operation on all registered memories
	 *
	 * This function only adds the flush operation to the queues if not empty
	 *
	 * @param[in] srail    Rail information
	 * @param[in] flush_op Flush operation to track the flushing progress
	 *
	 * @retval MPC_LOWCOMM_IN_PROGRESS if the flush operation has been added to at least one queue
	 * @retval MPC_LOWCOMM_SUCCESS     if the flush operation has been completed (all queues are empty)
	 */
	static int lcr_ptl_exec_flush_iface(lcr_ptl_rail_info_t *srail, lcr_ptl_op_t *flush_op)
	{
		int            rc = MPC_LOWCOMM_SUCCESS;
		int            i;
		lcr_ptl_mem_t *mem;

		mpc_common_spinlock_lock(&srail->net.rma.lock);
		const int num_eps = atomic_load(&srail->num_eps);

		/* Loop on all registered memories. */
		mpc_list_for_each(mem, &srail->net.rma.poll_list, lcr_ptl_mem_t, elem)
		{
			const int64_t completed = lcr_ptl_poll_mem(mem);

			/* Loop on all TX Queues that has been linked on this memory. */
			for (i = 0; i < num_eps; i++)
			{
				// Progress the queue and see if it is empty
				const int64_t still_pending = lcr_ptl_flush_txq(mem, &mem->txqt[i], completed);
				// If the queue is not empty
				if (still_pending > 0)
				{
					// Push the flush operation to the queue
					mpc_queue_push(&mem->txqt->ops, &flush_op->elem);
					rc = MPC_LOWCOMM_IN_PROGRESS;
				}
				else
				{
					// Complete one time the flush op (it has to be completed the number of queues time)
					lcr_ptl_complete_op(flush_op);
				}
			}
		} /* End loop registered memories */
		mpc_common_spinlock_unlock(&srail->net.rma.lock);

		return rc;
	}

	int lcr_ptl_flush_iface(sctk_rail_info_t *rail, lcr_completion_t *comp, unsigned flags)
	{
		UNUSED(flags);
		int                  rc = MPC_LOWCOMM_SUCCESS;
		lcr_ptl_op_t *       op;
		lcr_ptl_rail_info_t *srail = &rail->network.ptl;

		op = mpc_mpool_pop(srail->iface_ops);
		if (op == NULL)
		{
			mpc_common_debug_warning("LCR PTL: maximum outstanding operations.");
			rc = MPC_LOWCOMM_NO_RESOURCE;
			goto err;
		}

		// FIXME: no need for underscore in function name. To be removed.
		_lcr_ptl_init_op_common(op, 0, PTL_INVALID_HANDLE,
			NULL,
			LCR_PTL_PROCESS_ANY,
			srail->net.rma.pti,
			LCR_PTL_OP_RMA_FLUSH,
			0, comp,
			NULL);

		mpc_common_spinlock_lock(&srail->net.rma.lock);
		// Compute the number of queues to flush
		const int     num_eps      = atomic_load(&srail->num_eps);
		const int64_t num_mem      = mpc_list_length(&srail->net.rma.poll_list);
		const int64_t outstandings = num_eps * num_mem;
		op->flush.outstandings = outstandings > 1 ? outstandings : 1;
		mpc_common_spinlock_unlock(&srail->net.rma.lock);

		// If no queue has to be flushed
		if (outstandings == 0)
		{
			// Complete instantly the flush op
			lcr_ptl_complete_op(op);
		}
		else
		{
			// Flush the found queues
			rc = lcr_ptl_exec_flush_iface(srail, op);
		}

err:
		return rc;
	}

#endif
