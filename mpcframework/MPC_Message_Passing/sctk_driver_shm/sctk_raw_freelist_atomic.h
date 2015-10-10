#ifndef __SCTK_RAW_FREELIST_ATOMIC_H__
#define __SCTK_RAW_FREELIST_ATOMIC_H__

#include "sctk_raw_freelist_queue.h"
#include "sctk_shm_raw_queues_internals.h"

static inline lfq_cell_t *
lfq_dequeue_atomic(volatile sctk_shm_cells_queue_t *lfq, lfq_cell_t *pools_ptr)
{
    volatile lfq_cell_t *cell;
    sctk_shm_queue_t *head = lfq->shadow_head;
    sctk_shm_queue_t *tail;

    if(head == 0)
    {
        head = lfq->head;
        if(head == 0)
        {
            cpu_relax();
            return NULL;
        }
        else
        {
            lfq->shadow_head = head;
            lfq->head = 0;
        }
    }

    cell = vcli_raw_ptr_to_cell(pools_ptr, head); 

    if(cell->next != 0)
    {
        lfq->shadow_head = cell->next;
    }
    else
    {
        lfq->shadow_head = 0;
        tail = __sync_val_compare_and_swap(&lfq->tail, head, 0);
        if(tail != head)
        {
            while(cell->next == 0)
            {
                cpu_relax();
            }
            lfq->shadow_head = cell->next;
        }

    }
    cell->next = 0;
    return (lfq_cell_t *) cell;
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
