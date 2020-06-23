#ifndef __SCTK_RAW_FREELIST_MTHREADS_H__
#define __SCTK_RAW_FREELIST_MTHREADS_H__

#include "sctk_raw_freelist_queue.h"
#include "sctk_shm_raw_queues_internals.h"

static inline sctk_shm_item_t *
sctk_shm_dequeue_mt(sctk_shm_list_t *queue, char *src_base_addr)
{
	volatile sctk_shm_item_t *abs_item;
	sctk_shm_item_t *         head;

	if( queue->head == NULL )
	{
		return NULL;
	}

	if(mpc_common_spinlock_trylock(&(queue->lock) ) != 0)
	{
		return NULL;
	}

	head = queue->head;

	if(!head)
	{
		mpc_common_spinlock_unlock(&(queue->lock) );
		return NULL;
	}

	abs_item = sctk_shm_rel_to_abs(src_base_addr, queue->head);

	if(abs_item->next)
	{
		queue->head = abs_item->next;
	}
	else
	{
		queue->tail = 0;
		queue->head = 0;
	}
	mpc_common_spinlock_unlock(&(queue->lock) );
	abs_item->next = 0;
	return (sctk_shm_item_t *)abs_item;
}

static inline int
sctk_shm_enqueue_mt(sctk_shm_list_t *queue,
                    char *dest_base_addr,
                    sctk_shm_item_t *abs_new_item,
                    char *src_base_addr)
{
	sctk_shm_item_t *rel_new_item, *rel_prev_item, *abs_prev_item;

	rel_new_item = sctk_shm_abs_to_rel(src_base_addr, abs_new_item);

	mpc_common_spinlock_lock(&(queue->lock) );

	rel_prev_item = queue->tail;
	queue->tail   = rel_new_item;

	if(rel_prev_item == 0)
	{
		queue->head = rel_new_item;
	}
	else
	{
		abs_prev_item       = sctk_shm_rel_to_abs(dest_base_addr, rel_prev_item);
		abs_prev_item->next = rel_new_item;
	}
	mpc_common_spinlock_unlock(&(queue->lock) );

	OPA_write_barrier();

	return 0;
}

#endif /* __SCTK_RAW_FREELIST_MTHREADS_H__ */
