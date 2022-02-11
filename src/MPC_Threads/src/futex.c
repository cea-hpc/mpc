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
#include "futex.h"
#include "mpc_common_debug.h"
#include "mpc_thread.h"

#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

#include "thread.h"


/************************************************************************/
/* Futex Cell                                                         */
/************************************************************************/

struct futex_cell
{
	int *do_wait;
	int  freed;
	int  bitmask;
	int  skip;
	int  orig_op;
};

struct futex_cell *futex_cell_new(int bitmask, int orig_op);
int  futex_cell_match(struct futex_cell *cell, int bitmask);
void futex_cell_detach(struct futex_cell *cell);


struct futex_bitset_iterator_desc
{
	int bitmask;
	int to_process;
};


int futex_cell_apply_bitmask(void *elem /* struct futex_cell */,
                             void *arg /* struct futex_bitset_iterator_desc */);

/************************************************************************/
/* Futex Queues                                                         */
/************************************************************************/


struct futex_queue
{
	mpc_common_spinlock_t  queue_is_wake_tainted;
	struct mpc_common_fifo wait_list;
	int *                  futex_key;
};

struct futex_queue *futex_queue_new(int *futex_key);
int futex_queue_release(struct futex_queue *fq);
int *futex_queue_push(struct futex_queue *fq, int bitmask, int orig_op);
int futex_queue_wake(struct futex_queue *fq, int bitmask, int use_mask, int count, int op);

/************************************************************************/
/* Futex HT                                                             */
/************************************************************************/

struct futex_queue_HT
{
	OPA_int_t                   queue_table_is_being_manipulated;

	struct mpc_common_hashtable queue_hash_table;
	unsigned int                queue_count;
	unsigned int                queue_cleanup_ratio;
};

int futex_queue_HT_init(struct futex_queue_HT *ht);
int futex_queue_HT_release(struct futex_queue_HT *ht);
int *futex_queue_HT_register_thread(struct futex_queue_HT *ht, int *futex_key, int bitmask, int orig_op);
int futex_queue_HT_wake_threads(struct futex_queue_HT *ht, int *futex_key, int bitmask, int use_mask, int count, int op);

/********************
 * FUTEX CONVERTERS *
 ********************/

#if SCTK_FUTEX_SUPPORTED

#include <linux/futex.h>
#include <sys/syscall.h>

static inline int __translate_futex_op(int OPCODE)
{
	/* Always remove the private flag
	 * as it does not mean anything in our context */
	OPCODE &= ~(FUTEX_PRIVATE_FLAG);

	switch(OPCODE)
	{
	case FUTEX_WAIT:
		return SCTK_FUTEX_WAIT;

		break;

	case FUTEX_WAIT_BITSET:
		return SCTK_FUTEX_WAIT_BITSET;

		break;

	case FUTEX_WAKE:
		return SCTK_FUTEX_WAKE;

		break;

	case FUTEX_WAKE_BITSET:
		return SCTK_FUTEX_WAKE_BITSET;

		break;

	case FUTEX_WAKE_OP:
		return SCTK_FUTEX_WAKE_OP;

		break;

	case FUTEX_REQUEUE:
		return SCTK_FUTEX_REQUEUE;

		break;

	case FUTEX_CMP_REQUEUE:
		return SCTK_FUTEX_CMP_REQUEUE;

		break;

	case FUTEX_LOCK_PI:
		return SCTK_FUTEX_LOCK_PI;

		break;

	case FUTEX_TRYLOCK_PI:
		return SCTK_FUTEX_TRYLOCK_PI;

		break;

	case FUTEX_UNLOCK_PI:
		return SCTK_FUTEX_UNLOCK_PI;

		break;

	case FUTEX_CMP_REQUEUE_PI:
		return SCTK_FUTEX_CMP_REQUEUE_PI;

		break;

	case FUTEX_WAIT_REQUEUE_PI:
		return SCTK_FUTEX_WAIT_REQUEUE_PI;

		break;

	default:
		mpc_common_debug_error("Not implemented YET :=)");
	}

	return OPCODE;
}

/* For _OP operations */
static inline int __translate_futex_opcode(int opcode)
{
	switch(opcode)
	{
	case FUTEX_OP_SET:
		return SCTK_FUTEX_OP_SET;

		break;

	case FUTEX_OP_ADD:
		return SCTK_FUTEX_OP_ADD;

		break;

	case FUTEX_OP_OR:
		return SCTK_FUTEX_OP_OR;

		break;

	case FUTEX_OP_ANDN:
		return SCTK_FUTEX_OP_ANDN;

		break;

	case FUTEX_OP_XOR:
		return SCTK_FUTEX_OP_XOR;

		break;

	default:
		mpc_common_debug_error("Failled to convert FUTEX OP");
	}

	return opcode;
}

/* May not be defined */
#ifndef FUTEX_OP_ARG_SHIFT
#define FUTEX_OP_ARG_SHIFT    8
#endif

static inline int __translate_futex_opcode_for_shift(int opcode)
{
	return opcode & FUTEX_OP_ARG_SHIFT;
}

int __translate_futex_waiter()
{
	return FUTEX_WAITERS;
}

int __translate_futex_cmp(int cmp)
{
	switch(cmp)
	{
	case FUTEX_OP_CMP_EQ:
		return SCTK_FUTEX_OP_CMP_EQ;

		break;

	case FUTEX_OP_CMP_NE:
		return SCTK_FUTEX_OP_CMP_NE;

		break;

	case FUTEX_OP_CMP_LT:
		return SCTK_FUTEX_OP_CMP_LT;

		break;

	case FUTEX_OP_CMP_LE:
		return SCTK_FUTEX_OP_CMP_LE;

		break;

	case FUTEX_OP_CMP_GT:
		return SCTK_FUTEX_OP_CMP_GT;

		break;

	case FUTEX_OP_CMP_GE:
		return SCTK_FUTEX_OP_CMP_GE;

		break;

	default:
		mpc_common_debug_fatal("No such cmp operation");
	}

	return cmp;
}

#else

/* No need to translate as values are already
 * internal symbols */

static inline int __translate_futex_op(int OPCODE)
{
	return OPCODE;
}

/* For _OP operations */
static inline int __translate_futex_opcode(int opcode)
{
	return opcode;
}

int __translate_futex_opcode_for_shift(int opcode)
{
	return opcode & SCTK_FUTEX_OP_ARG_SHIFT;
}

int __translate_futex_waiter()
{
	return SCTK_FUTEX_WAITERS;
}

int __translate_futex_cmp(int cmp)
{
	return cmp;
}
#endif


/************************************************************************/
/* Futex Cell                                                         */
/************************************************************************/

struct futex_cell *futex_cell_new(int bitmask, int orig_op)
{
	struct futex_cell *cell = calloc(1, sizeof(struct futex_cell) );

	cell->do_wait = malloc(sizeof(int) );

	if(!cell->do_wait)
	{
		perror("calloc");
		mpc_common_debug_fatal("alloc failed");
	}

	*cell->do_wait = 1;

	cell->bitmask = bitmask;
	cell->orig_op = orig_op;

	cell->skip = 0;

	return cell;
}

int  futex_cell_match(struct futex_cell *cell, int bitmask)
{
	if(!cell)
	{
		return 0;
	}

	return cell->bitmask & bitmask;
}

int futex_cell_apply_bitmask(void *elem, void *arg)
{
	struct futex_cell *cell = (struct futex_cell *)elem;
	struct futex_bitset_iterator_desc *desc =
	        (struct futex_bitset_iterator_desc *)arg;

	if(desc->to_process == 0)
	{
		return 0;
	}

	if(cell->skip == 0)
	{
		if(futex_cell_match(cell, desc->bitmask) )
		{
			/* We matched the bitset */
			*cell->do_wait = 0;
			cell->skip     = 1;
			desc->to_process--;

			return 1;
		}
	}

	/* Did not process */
	return 0;
}

void  futex_cell_detach(struct futex_cell *cell)
{
	int *ret = cell->do_wait;

	*ret = 0;
	free(cell);
}

/************************************************************************/
/* Futex Queues                                                         */
/************************************************************************/


struct futex_queue *futex_queue_new(int *futex_key)
{
	struct futex_queue *ret = malloc(sizeof(struct futex_queue) );

	if(!ret)
	{
		perror("malloc");
		return NULL;
	}

	mpc_common_spinlock_init(&ret->queue_is_wake_tainted, 0);

	mpc_common_fifo_init(&ret->wait_list, 32, sizeof(struct futex_cell *) );

	ret->futex_key = futex_key;

	return ret;
}

void *futex_queue_new_from_key(uint64_t key)
{
	return (void *)futex_queue_new( (int *)key);
}

int futex_queue_release(struct futex_queue *fq)
{
	mpc_common_fifo_release(&fq->wait_list);

	free(fq);

	return 0;
}

int *futex_queue_push(struct futex_queue *fq, int bitmask, int orig_op)
{
	/* Make sure we do not push when poping
	 * to avoid repopping previously popped threads */
	mpc_common_spinlock_lock(&fq->queue_is_wake_tainted);

	struct futex_cell *cell = futex_cell_new(bitmask, orig_op);

	mpc_common_fifo_push(&fq->wait_list, &cell);

	mpc_common_spinlock_unlock(&fq->queue_is_wake_tainted);

	return cell->do_wait;
}

int *futex_queue_repush_no_lock(struct futex_queue *fq, struct futex_cell *to_repush)
{
	mpc_common_fifo_push(&fq->wait_list, &to_repush);

	return to_repush->do_wait;
}

int *futex_queue_repush(struct futex_queue *fq, struct futex_cell *to_repush)
{
	int *ret = NULL;

	mpc_common_spinlock_lock(&fq->queue_is_wake_tainted);

	ret = futex_queue_repush_no_lock(fq, to_repush);

	mpc_common_spinlock_unlock(&fq->queue_is_wake_tainted);

	return ret;
}

int futex_check_op_compat(int op, int orig_op)
{
	/* Check mistmatching TYPES */
	if( (op == SCTK_FUTEX_UNLOCK_PI) ||
	    (op == SCTK_FUTEX_CMP_REQUEUE_PI) ||
	    (op == SCTK_FUTEX_WAIT_REQUEUE_PI) )
	{
		if( (orig_op == SCTK_FUTEX_WAIT) ||
		    (orig_op == SCTK_FUTEX_WAIT_BITSET) )
		{
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

int futex_queue_wake(struct futex_queue *fq, int bitmask, int use_mask, int count, int op)
{
	int popped = 0;

	mpc_common_spinlock_lock(&fq->queue_is_wake_tainted);

	/* Wake indiferently from the mask */
	if(use_mask == 0)
	{
		while(count)
		{
			struct futex_cell *to_wake = NULL;


			if(!mpc_common_fifo_pop(&fq->wait_list, (void *)&to_wake) )
			{
				/* No more elem to pop */
				break;
			}
			else
			{
				/* Wake up the thread */
				if(to_wake)
				{
					if(*to_wake->do_wait == 2)
					{
						free(to_wake->do_wait);
					}
					else
					{
						if(!to_wake->skip)
						{
							if(futex_check_op_compat(op, to_wake->orig_op) < 0)
							{
								futex_queue_repush_no_lock(fq, to_wake);
								popped = -1;
								break;
							}

							futex_cell_detach(to_wake);
							popped++;
							count--;
						}
						else
						{
							free(to_wake->do_wait);
						}
					}
				}
				else
				{
					mpc_common_debug_error("Popped an empty elem ??");
				}
			}
		}
	}
	else
	{
		/* We only wake those which are matching the mask */
		struct futex_bitset_iterator_desc desc;

		desc.bitmask    = bitmask;
		desc.to_process = count;

		popped = mpc_common_fifo_process(&fq->wait_list, futex_cell_apply_bitmask, (void *)&desc);
	}

	mpc_common_spinlock_unlock(&fq->queue_is_wake_tainted);

	return popped;
}

int futex_queue_requeue(struct futex_queue *fq, struct futex_queue *out, int count, int op)
{
	int popped = 0;

	mpc_common_spinlock_lock(&fq->queue_is_wake_tainted);

	struct futex_cell *to_wake = NULL;

	while(count)
	{
		if(!mpc_common_fifo_pop(&fq->wait_list, (void *)&to_wake) )
		{
			/* No more elem to pop */
			break;
		}
		else
		{
			/* Wake up the thread */
			if(to_wake)
			{
				if(*to_wake->do_wait == 2)
				{
					free(to_wake->do_wait);
				}
				else
				{
					if(!to_wake->skip)
					{
						if(futex_check_op_compat(op, to_wake->orig_op) < 0)
						{
							futex_queue_repush_no_lock(fq, to_wake);
							popped = -1;
							break;
						}

						futex_cell_detach(to_wake);
						popped++;
						count--;
					}
					else
					{
						free(to_wake->do_wait);
					}
				}
			}
			else
			{
				mpc_common_debug_error("Popped an empty elem ??");
			}
		}
	}

	if(popped < 0)
	{
		/* We already encountered an error */
		mpc_common_spinlock_unlock(&fq->queue_is_wake_tainted);
		return -1;
	}

	if(fq != out)
	{
		/* Queues are different, lets push on the new one */
		while(mpc_common_fifo_pop(&fq->wait_list, (void *)&to_wake) )
		{
			futex_queue_repush(out, to_wake);
			//sched_yield();
		}
	}
	/* ELSE Nothing to do we are already on the same queue */

	mpc_common_spinlock_unlock(&fq->queue_is_wake_tainted);


	return popped;
}

/************************************************************************/
/* Futex HT                                                             */
/************************************************************************/

int futex_queue_HT_init(struct futex_queue_HT *ht)
{
	mpc_common_hashtable_init(&ht->queue_hash_table, 128);
	ht->queue_count         = 0;
	ht->queue_cleanup_ratio = 128;
	OPA_store_int(&ht->queue_table_is_being_manipulated, 0);

	return 0;
}

int futex_queue_HT_release(struct futex_queue_HT *ht)
{
	mpc_common_hashtable_release(&ht->queue_hash_table);
	ht->queue_count         = 0;
	ht->queue_cleanup_ratio = 0;
	OPA_store_int(&ht->queue_table_is_being_manipulated, 0);

	return 0;
}

int *futex_queue_HT_register_thread(struct futex_queue_HT *ht, int *futex_key, int bitmask, int orig_op)
{
	/* Here we are protecting a table manipulation */
	while(OPA_load_int(&ht->queue_table_is_being_manipulated) < 0)
	{
		sched_yield();
	}

	/* This is the Recycling policy to avoid wasting memory
	 * if the user keeps creating different futexes */
	if(ht->queue_cleanup_ratio < ht->queue_count)
	{
		/* Flag the table as being manipulated dy decrementing by one */
		OPA_decr_int(&ht->queue_table_is_being_manipulated);

		/* Wait of all the threads in the critical section to leave */
		while(0 <= OPA_load_int(&ht->queue_table_is_being_manipulated) )
		{
			sched_yield();
		}

		/* At this point no thread shall enter */
		void *pfutex_queue = NULL;
		MPC_HT_ITER(&ht->queue_hash_table, pfutex_queue)
		{
			struct futex_queue *queue = (struct futex_queue *)pfutex_queue;

			if(!mpc_common_fifo_count(&queue->wait_list) )
			{
				/* This is an empty wait list then POP */
				uint64_t key = (uint64_t)queue->futex_key;
				mpc_common_hashtable_delete(&ht->queue_hash_table, key);
				ht->queue_count--;

				/* Free it */
				futex_queue_release(queue);
			}
		}
		MPC_HT_ITER_END(&ht->queue_hash_table)


		/* This done we leave the negative locking phase */
		OPA_incr_int(&ht->queue_table_is_being_manipulated);

		/* As we are not sure to be the first one entering
		 * we simply recursively call the function */
		return futex_queue_HT_register_thread(ht, futex_key, bitmask, orig_op);
	}


	/* Now that resource exhaustion has been managed
	 * lets focus on ressource allocation */

	int *ret = NULL;

	/* Acquire the read lock */
	OPA_incr_int(&ht->queue_table_is_being_manipulated);

	/* Here we first create a futex queue if
	 * it does not exists yet */
	uint64_t new_key = (uint64_t)futex_key;

	int new_queue_created  = 0;
	struct futex_queue *fq = (struct futex_queue *)
	                         mpc_common_hashtable_get_or_create(&ht->queue_hash_table,
	                                                            new_key,
	                                                            futex_queue_new_from_key,
	                                                            &new_queue_created);
	/* Was a new queue created ? */
	if(new_queue_created)
	{
		ht->queue_count++;
	}

	/* Did it work ? */
	if(fq)
	{
		/* Here we are now manipulating a futex queue */

		/* Push a new wait entry */
		ret = futex_queue_push(fq, bitmask, orig_op);
	}

	/* Release the read lock */
	OPA_decr_int(&ht->queue_table_is_being_manipulated);

	sched_yield();

	return ret;
}

int futex_queue_HT_requeue_threads(struct futex_queue_HT *ht,
                                   int *in_futex_key,
                                   int *out_futex_key,
                                   int count,
                                   int op)
{
	int ret = 0;

	/* Acquire the read lock */
	OPA_incr_int(&ht->queue_table_is_being_manipulated);

	/* Here we first create a futex queue if
	 * it does not exists yet */
	uint64_t in_key  = (uint64_t)in_futex_key;
	uint64_t out_key = (uint64_t)out_futex_key;

	int new_queue_created     = 0;
	struct futex_queue *in_fq = (struct futex_queue *)
	                            mpc_common_hashtable_get_or_create(&ht->queue_hash_table,
	                                                               in_key,
	                                                               futex_queue_new_from_key,
	                                                               &new_queue_created);
	/* Was a new queue created ? */
	if(new_queue_created)
	{
		ht->queue_count++;
	}

	new_queue_created = 0;
	struct futex_queue *out_fq = (struct futex_queue *)
	                             mpc_common_hashtable_get_or_create(&ht->queue_hash_table,
	                                                                out_key,
	                                                                futex_queue_new_from_key,
	                                                                &new_queue_created);
	/* Was a new queue created ? */
	if(new_queue_created)
	{
		ht->queue_count++;
	}

	/* Did it work ? */
	if(in_fq && out_fq)
	{
		/* Here we are now manipulating a futex queue */
		ret = futex_queue_requeue(in_fq, out_fq, count, op);
	}

	/* Release the read lock */
	OPA_decr_int(&ht->queue_table_is_being_manipulated);

	return ret;
}

int futex_queue_HT_wake_threads(struct futex_queue_HT *ht, int *futex_key, int bitmask, int use_mask, int count, int op)
{
	/* Here we are protecting a table manipulation */
	while(OPA_load_int(&ht->queue_table_is_being_manipulated) < 0)
	{
		sched_yield();
	}

	int ret = 0;

	/* Acquire the read lock */
	OPA_incr_int(&ht->queue_table_is_being_manipulated);

	uint64_t            key = (uint64_t)futex_key;
	struct futex_queue *fq  = (struct futex_queue *)
	                          mpc_common_hashtable_get(&ht->queue_hash_table, key);

	if(!fq)
	{
		/* No such queue */
		errno = EINVAL;
		OPA_decr_int(&ht->queue_table_is_being_manipulated);
		return -1;
	}
	else
	{
		/* Perform the wake */
		ret = futex_queue_wake(fq, bitmask, use_mask, count, op);
	}

	/* Release the read lock */
	OPA_decr_int(&ht->queue_table_is_being_manipulated);

	return ret;
}

/************************************************************************/
/* Futex Context                                                        */
/************************************************************************/

static struct futex_queue_HT ___futex_queue_HT;
static volatile int          context_init_done = 0;

void _mpc_thread_futex_context_init()
{
	if(context_init_done)
	{
		return;
	}

	context_init_done = 1;

	futex_queue_HT_init(&___futex_queue_HT);
}

void _mpc_thread_futex_context_release()
{
	futex_queue_HT_release(&___futex_queue_HT);
}

/************************************************************************/
/* Futex Ops                                                                */
/************************************************************************/

int _mpc_thread_futex_WAIT_no_timer(void *addr1, int op, int val3)
{
	int *wait_ticket = futex_queue_HT_register_thread(&___futex_queue_HT, addr1, val3, op);

	if(!wait_ticket)
	{
		return -1;
	}

	mpc_thread_wait_for_value(wait_ticket, 0);

	/* If we are here we are done waiting */
	free(wait_ticket);

	return 0;
}

int _mpc_thread_futex_WAIT(void *addr1, int op, int val1, struct timespec *timeout, int bitmask)
{
	/* First check the value */
	OPA_int_t *pold_val = (OPA_int_t *)addr1;

	int old_val = OPA_load_int(pold_val);

	/* Val is different directly return */
	if(old_val != val1)
	{
		errno = EWOULDBLOCK;
		return -1;
	}


	/* Do we need to jump to the blocking implementation ? */
	if(!timeout)
	{
		return _mpc_thread_futex_WAIT_no_timer(addr1, op, bitmask);
	}

	/* Here we consider the timed implementation */

	int *wait_ticket = futex_queue_HT_register_thread(&___futex_queue_HT, addr1, bitmask, op);

	if(!wait_ticket)
	{
		return -1;
	}

	unsigned int time_to_wait = timeout->tv_sec * 1e6 + timeout->tv_nsec;

	if(mpc_thread_timed_wait_for_value(wait_ticket, 0, time_to_wait) )
	{
		/* We timed out */
		errno        = ETIMEDOUT;
		*wait_ticket = 2;
		return -1;
	}

	/* If we are here we are done waiting */
	free(wait_ticket);

	return 0;
}

int _mpc_thread_futex_WAKE(void *addr1, int op, int val1)
{
	return futex_queue_HT_wake_threads(&___futex_queue_HT, addr1, 0, 0 /* No bitmask */, val1, op);
}

int _mpc_thread_futex_WAKE_BITSET(void *addr1, int op, int val1, int val3)
{
	return futex_queue_HT_wake_threads(&___futex_queue_HT, addr1, val3, 1, val1, op);
}

void _mpc_thread_futex_WAKE_OP_decode(int val3, int *op, int *cmp, int *oparg)
{
	int opcode, opargcode, cmpargcode;

	opcode     = (val3 >> 28) & 0xf;
	opargcode  = (val3 >> 12) & 0xfff;
	cmpargcode = val3 & 0xfff;

	*cmp = __translate_futex_cmp(cmpargcode);
	*op  = __translate_futex_opcode(opcode);

	if(__translate_futex_opcode_for_shift(opcode) == 0)
	{
		*oparg = opargcode;
	}
	else
	{
		/* FUTEX_OP_ARG_SHIFT was set */
		*oparg = (1 << opargcode);
	}
}

int _mpc_thread_futex_WAKE_OP_do_op(int oldval, int val3)
{
	int op, cmp, oparg;

	_mpc_thread_futex_WAKE_OP_decode(val3, &op, &cmp, &oparg);

	switch(op)
	{
	case SCTK_FUTEX_OP_SET:
		return oparg;

		break;

	case SCTK_FUTEX_OP_ADD:
		return oldval + oparg;

		break;

	case SCTK_FUTEX_OP_OR:
		return oldval | oparg;

		break;

	case SCTK_FUTEX_OP_ANDN:
		return oldval & oparg;

		break;

	case SCTK_FUTEX_OP_XOR:
		return oldval ^ oparg;

		break;

	default:
		mpc_common_debug_fatal("No such operation");
	}

	return oparg;
}

int _mpc_thread_futex_WAKE_OP_do_cmp(int oldval, int val3)
{
	int op, cmp, oparg;

	_mpc_thread_futex_WAKE_OP_decode(val3, &op, &cmp, &oparg);

	switch(op)
	{
	case SCTK_FUTEX_OP_CMP_EQ:
		return oldval == oparg;

		break;

	case SCTK_FUTEX_OP_CMP_NE:
		return oldval != oparg;

		break;

	case SCTK_FUTEX_OP_CMP_LT:
		return oldval < oparg;

		break;

	case SCTK_FUTEX_OP_CMP_LE:
		return oldval <= oparg;

		break;

	case SCTK_FUTEX_OP_CMP_GT:
		return oparg < oldval;

		break;

	case SCTK_FUTEX_OP_CMP_GE:
		return oparg <= oldval;

		break;

	default:
		mpc_common_debug_fatal("No such operation");
	}

	return oparg;
}

int _mpc_thread_futex_WAKE_OP(void *addr1, void *addr2, int val1, int val2, int val3)
{
	int ret     = 0;
	int tmp_ret = 0;

	int oldval = *(int *)addr2;

	*( (int *)addr2) = _mpc_thread_futex_WAKE_OP_do_op(oldval, val3);

	tmp_ret = _mpc_thread_futex_WAKE(addr1, SCTK_FUTEX_WAKE, val1);

	if(0 <= tmp_ret)
	{
		ret += tmp_ret;
	}

	if(_mpc_thread_futex_WAKE_OP_do_cmp(oldval, val3) )
	{
		tmp_ret = _mpc_thread_futex_WAKE(addr2, SCTK_FUTEX_WAKE, val2);

		if(0 <= tmp_ret)
		{
			ret += tmp_ret;
		}
	}

	return ret;
}

int _mpc_thread_futex_REQUEUE(void *addr1, int op, int val1, void *addr2)
{
	return futex_queue_HT_requeue_threads(&___futex_queue_HT,
	                                      addr1,
	                                      addr2,
	                                      val1,
	                                      op);
}

int _mpc_thread_futex_CMPREQUEUE(void *addr1, int op, int val1, void *addr2, int val3)
{
	/* First check the value */
	OPA_int_t *pold_val = (OPA_int_t *)addr1;

	int old_val = OPA_load_int(pold_val);

	/* Val is different directly return */
	if(old_val != val3)
	{
		errno = EWOULDBLOCK;
		return -1;
	}

	return futex_queue_HT_requeue_threads(&___futex_queue_HT,
	                                      addr1,
	                                      addr2,
	                                      val1,
	                                      op);
}

int _mpc_thread_futex_CMPREQUEUE_PI(void *addr1, int val1, void *addr2, int val3)
{
	if(val1 != 1)
	{
		errno = EINVAL;
		return -1;
	}
	else
	{
		return _mpc_thread_futex_CMPREQUEUE(addr1, SCTK_FUTEX_CMP_REQUEUE_PI, val1, addr2, val3);
	}
}

int _mpc_thread_futex_TRYLOCKPI(int *volatile futex)
{
	int ret = mpc_common_spinlock_trylock( (mpc_common_spinlock_t *)futex);

	/* Flag as waited for */
	*futex = *futex & __translate_futex_waiter();

	if(ret != 0)
	{
		return -1;
	}

	return 0;
}

int _mpc_thread_futex_LOCKPI(int *volatile futex, struct timespec *timeout)
{
	/* Flag as waited for */
	*futex = *futex & __translate_futex_waiter();

	if(!timeout)
	{
		if(_mpc_thread_futex_TRYLOCKPI(futex) != 0)
		{
			_mpc_thread_futex_WAIT_no_timer(futex, SCTK_FUTEX_LOCK_PI, 0);
		}
	}
	else
	{
		unsigned int time_to_wait = timeout->tv_sec * 1e6 + timeout->tv_nsec;

		if(mpc_thread_timed_wait_for_value(futex, 0, time_to_wait) )
		{
			/* We timed out */
			errno = ETIMEDOUT;
			return -1;
		}

		if(_mpc_thread_futex_TRYLOCKPI(futex) != 0)
		{
			/* We failled to take the lock just mimic timeout */
			errno = ETIMEDOUT;
			return -1;
		}
	}

	return 0;
}

int _mpc_thread_futex_UNLOCKPI(int *volatile futex)
{
	mpc_common_spinlock_unlock( (mpc_common_spinlock_t *)futex);
	_mpc_thread_futex_WAKE(futex, SCTK_FUTEX_UNLOCK_PI, 1);

	return 0;
}

int _mpc_thread_futex(void *addr1, int op, int val1,
               struct timespec *timeout, void *addr2, int val3)
{
	int ret = 0;

	/* We need to convert only if futexes
	 * are supported by the system
	 * othewise the value is already the
	 * internal symbol this is handled
	 * inside this function */
	op = __translate_futex_op(op);

	unsigned long tmp_val2 = (unsigned long)timeout;
	int           val2     = (int)tmp_val2;

	switch(op)
	{
	case SCTK_FUTEX_WAIT:
		ret = _mpc_thread_futex_WAIT(addr1, SCTK_FUTEX_WAIT, val1, timeout, 0);
		break;

	case SCTK_FUTEX_WAIT_BITSET:
		ret = _mpc_thread_futex_WAIT(addr1, SCTK_FUTEX_WAIT_BITSET, val1, timeout, val3);
		break;

	case SCTK_FUTEX_WAKE:
		ret = _mpc_thread_futex_WAKE(addr1, op, val1);
		break;

	case SCTK_FUTEX_WAKE_BITSET:
		ret = _mpc_thread_futex_WAKE_BITSET(addr1, op, val1, val3);
		break;

	case SCTK_FUTEX_WAKE_OP:
		ret = _mpc_thread_futex_WAKE_OP(addr1, addr2, val1, val2, val3);
		break;

	case SCTK_FUTEX_REQUEUE:
		ret = _mpc_thread_futex_REQUEUE(addr1, SCTK_FUTEX_REQUEUE, val1, addr2);
		break;

	case SCTK_FUTEX_CMP_REQUEUE:
		ret = _mpc_thread_futex_CMPREQUEUE(addr1, SCTK_FUTEX_CMP_REQUEUE, val1, addr2, val3);
		break;

	case SCTK_FUTEX_CMP_REQUEUE_PI:
		ret = _mpc_thread_futex_CMPREQUEUE_PI(addr1, val1, addr2, val3);
		break;

	case SCTK_FUTEX_LOCK_PI:
		ret = _mpc_thread_futex_LOCKPI(addr1, timeout);
		break;

	case SCTK_FUTEX_TRYLOCK_PI:
		ret = _mpc_thread_futex_TRYLOCKPI(addr1);
		break;

	case SCTK_FUTEX_UNLOCK_PI:
		ret = _mpc_thread_futex_UNLOCKPI(addr1);
		break;

	case SCTK_FUTEX_WAIT_REQUEUE_PI:
		ret = _mpc_thread_futex_WAIT(addr1, SCTK_FUTEX_WAIT_REQUEUE_PI, *( (int *)addr1), timeout, 0);
		break;

	default:
		mpc_common_debug_fatal("Not implemented YET");
	}

	return ret;
}
