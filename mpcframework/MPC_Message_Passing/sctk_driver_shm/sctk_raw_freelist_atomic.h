#ifndef __SCTK_RAW_FREELIST_ATOMIC_H__
#define __SCTK_RAW_FREELIST_ATOMIC_H__

#include "sctk_raw_freelist_queue.h"
#include "sctk_shm_raw_queues_internals.h"

static inline sctk_shm_item_t *
sctk_shm_dequeue_atomic(volatile sctk_shm_cells_queue_t *queue,char *src_base_addr)
{
    volatile sctk_shm_item_t *abs_item;
    sctk_shm_queue_t *head, *tail; 

    head = queue->shadow_head;
    if(!head)
    {
        head = queue->head;
        if(!head)
        {
            cpu_relax();
            return NULL;
        }
        else
        {
            queue->shadow_head = head;
            queue->head = 0;
        }
    }

    abs_item = sctk_shm_rel_to_abs(src_base_addr,queue->head); 

    if(abs_item->next != 0)
    {
        queue->shadow_head = abs_item->next;
    }
    else
    {
        queue->shadow_head = 0;
        tail = __sync_val_compare_and_swap(&queue->tail, head, 0);
        if(tail != head)
        {
            while(abs_item->next == 0)
            {
                cpu_relax();
            }
            queue->shadow_head = abs_item->next;
        }

    }
    abs_item->next = 0;
    return (lfq_cell_t *) abs_item;
}

static inline void
sctk_vcli_raw_lfq_enqueue_atomic(volatile sctk_shm_queue_t *lfq, lfq_cell_t *pools_ptr, lfq_cell_t *cell)
{
    sctk_shm_queue_t *new = vcli_raw_cell_to_ptr(pools_ptr, cell);
    sctk_shm_queue_t *prev;
    prev = __sync_lock_test_and_set(&lfq->tail, new);
    
    if(prev == 0)
    {
        lfq->head = new;
    }
    else
    {
        
        lfq_cell_t* abs_prev = vcli_raw_ptr_to_cell(pools_ptr, prev);     
        abs_prev->next = new;
     }
}

#endif /* __SCTK_RAW_FREELIST_ATOMIC_H__ */
