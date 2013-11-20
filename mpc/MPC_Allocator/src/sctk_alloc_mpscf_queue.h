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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #   - Inspirated from OpenPA queue                                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_MPSCF_QUEUE_H
#define SCTK_ALLOC_MPSCF_QUEUE_H

/*********************************** DOC ************************************/
/**
 * @file sctk_alloc_mpsfc_queue.h This file provide a simple implementation of
 * atomic queue for remote free support in allocator.
 * MPSCF = Multi-Producer and Single Consumer with Flush
 * It imply :
 *    - multiple producer (threads) to insert elements concurrently
 *    - A unique (or protect by lock) consumer.
 *    - Consumer will flush the whole queue in one request, not element by element.
 *
 * It was inspirated from OpenPA queue. But as we flushed the whole list in one call
 * we didn't need the trick of shadow pointer. Maybe we can just re-implement
 * the sctk_mpscft_queue_dequeue_all() onto their implementation, it may work if
 * we take care of their shadow pointer.
**/

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include "sctk_alloc_debug.h"

/********************************* MACROS ***********************************/
#ifdef MPC_Common
	#include <sctk_atomics.h>
#elif defined(_WIN32)
	#include <Windows.h>
	//define the equiavelent of OPA_ptr_t for windows VCC
	typedef PVOID volatile sctk_atomics_ptr;

	//emulate OPA function for windows VCC (consider 64bit intel x86 atch here)
	#define	sctk_atomics_load_ptr(target) (*(target))
	#define sctk_atomics_store_ptr(target,value) ((*(target)) = (value))
	#define sctk_atomics_swap_ptr(target,value) InterlockedExchangePointer((target),(value))
	//TODO find something to do 'pause'
	#define sctk_atomics_pause() do{} while(0)
#else //MPC_Common
	#include <opa_primitives.h>
	// Rename the OPA types into sctk_atomics ones
	typedef OPA_ptr_t sctk_atomics_ptr;

	// Rename the OPA functions into sctk_atomics ones
	#define sctk_atomics_load_ptr            OPA_load_ptr
	#define sctk_atomics_store_ptr           OPA_store_ptr
	#define sctk_atomics_swap_ptr            OPA_swap_ptr
	#define sctk_atomics_pause               OPA_busy_wait
#endif //MPC_Common

/********************************* MACROS ***********************************/
/**
 * Define the required alignment for pointer type to permit atomic manipulations.
**/
#define SCTK_ALLOC_POINTER_ATOMIC_ALIGN (sizeof(void*))

/******************************** STRUCTURE *********************************/
/**
 * An entry of the MPSCF queue. Place this structure at first position in your
 * own one.
**/
struct sctk_mpscf_queue_entry
{
	/** Point the next element of the single chained list. NULL if none. **/
	struct sctk_mpscf_queue_entry * volatile next;
};

/******************************** STRUCTURE *********************************/
/**
 * Base structure of the queue.
**/
struct sctk_mpscf_queue
{
	sctk_atomics_ptr head;
	sctk_atomics_ptr tail;
};

/********************************* FUNCTION *********************************/
/*Quick list of user functions to manipulate the list. */
static __inline__ void sctk_mpscf_queue_init(struct sctk_mpscf_queue * queue);
static __inline__ bool sctk_mpscf_queue_is_empty(struct sctk_mpscf_queue * queue);
static __inline__ void sctk_mpscf_queue_insert(struct sctk_mpscf_queue * queue, struct sctk_mpscf_queue_entry * entry);
static __inline__ void sctk_mpscf_queue_wait_until_end_is(struct sctk_mpscf_queue_entry * head,struct sctk_mpscf_queue_entry * expected_tail);
static __inline__ struct sctk_mpscf_queue_entry * sctk_mpscft_queue_dequeue_all(struct sctk_mpscf_queue * queue);

/********************************* FUNCTION *********************************/
static __inline__ void sctk_mpscf_queue_init(struct sctk_mpscf_queue * queue)
{
	/* errors */
	assert(queue != NULL);

	/* setup default values */
	sctk_atomics_store_ptr(&queue->head,NULL);
	sctk_atomics_store_ptr(&queue->tail,NULL);
}

/********************************* FUNCTION *********************************/
/**
 * Cleanup function to call when we finished to use the queue. It done nothing for now
 * but for better semantic implementation prefer to call it.
 */
static __inline__ void sctk_mpscf_queue_destroy(struct sctk_mpscf_queue * queue)
{
	/* nothing to do here up to now */
	assume_m(sctk_mpscf_queue_is_empty(queue),"Can't cleanup a non empty queue.");
}

/********************************* FUNCTION *********************************/
/**
 * Check is the queue is empty or not.
 */
static __inline__ bool sctk_mpscf_queue_is_empty(struct sctk_mpscf_queue * queue)
{
	/* errors */
	assert(queue != NULL);

	/* check status */
	return (sctk_atomics_load_ptr(&queue->head) == NULL);
}

/********************************* FUNCTION *********************************/
/**
 * This method is thread-safe and can be used concurrently
**/
static __inline__ void sctk_mpscf_queue_insert(struct sctk_mpscf_queue * queue, struct sctk_mpscf_queue_entry * entry)
{
	/* vars */
	struct sctk_mpscf_queue_entry * prev;

	/* errors */
	assert(queue != NULL);
	assert(entry != NULL);
	assert((unsigned long long)queue % SCTK_ALLOC_POINTER_ATOMIC_ALIGN == 0);
	assert((unsigned long long)entry % SCTK_ALLOC_POINTER_ATOMIC_ALIGN == 0);

	/* this is the new last element, so next is NULL */
	entry->next = NULL;

	/* update tail with swap first */
	prev = (struct sctk_mpscf_queue_entry *)sctk_atomics_swap_ptr(&queue->tail,entry);

	/* Then update head if required or update prev->next
	   This operation didn't required atomic ops as long as we are aligned in memory*/
	if (prev == NULL) {
		/* in theory atomic isn't required for this write otherwise we can do atomic write */
		sctk_atomics_store_ptr(&queue->head,entry);
	} else {
		prev->next = entry;
	}
}

/********************************* FUNCTION *********************************/
/**
 * Loop until last element and spin on it until next was setup to the given expected tail.
**/
static __inline__ void sctk_mpscf_queue_wait_until_end_is(struct sctk_mpscf_queue_entry * head,struct sctk_mpscf_queue_entry * expected_tail)
{
	/* vars */
	volatile struct sctk_mpscf_queue_entry * current = head;

	/* errors */
	assert(current != NULL);
	assert(expected_tail != NULL);

	/* loop until we find tail */
	while (current != expected_tail)
	{
		if (current->next != NULL)
			current = current->next;
		else
			sctk_atomics_pause();
	}

	/* check that we have effectively the last element otherwise it's a bug. */
	assert(current->next == NULL);
}

/********************************* FUNCTION *********************************/
/**
 * CAUTION, this method must be called in critical section.
**/
static __inline__ struct sctk_mpscf_queue_entry * sctk_mpscft_queue_dequeue_all(struct sctk_mpscf_queue * queue)
{
	/* vars */
	struct sctk_mpscf_queue_entry * head;
	struct sctk_mpscf_queue_entry * tail;

	/* errors */
	assert(queue != NULL);

	/* read head and mark it as NULL */
	head = (struct sctk_mpscf_queue_entry *)sctk_atomics_load_ptr(&queue->head);

	/* if has entry, need to clear the current list */
	if (head != NULL)
	{
		/* Mark head as empty, in theory it's ok without atomic write here.
		   At register time, head is write only for first element.
		   as we have one, produced work only on tail.
		   We will flush tail after this, so it's ok with cache coherence if the two next
		   ops are not reorder.*/
		sctk_atomics_store_ptr(&queue->head,NULL);
		//OPA_write_barrier();

		/* swap tail to make it NULL */
		tail = (struct sctk_mpscf_queue_entry *)sctk_atomics_swap_ptr(&queue->tail,NULL);

		/* we have head, so NULL tail is abnormal */
		assert(tail != NULL);

		/* walk on the list until last element and check that it was
		   tail, otherwise, another thread is still in registering the tail entry
		   but didn't finish to setup ->next, so wait unit next became tail*/
		sctk_mpscf_queue_wait_until_end_is(head,tail);
	}

	/* now we can return */
	return head;
}

#endif /* SCTK_ALLOC_MPSCF_QUEUE_H */
