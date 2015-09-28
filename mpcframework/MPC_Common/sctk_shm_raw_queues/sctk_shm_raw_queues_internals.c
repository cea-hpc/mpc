#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sctk_debug.h"

#include "sctk_shm_raw_queues_internals.h"

lfq_cell_t *lfq_dequeue(volatile lf_queue_t *lfq, lfq_cell_t *pools_ptr)
{
    volatile lfq_cell_t *cell;
#ifndef ATOMIC_QUEUE
    lfq_ptr_t head = lfq->shadow_head;
    lfq_ptr_t tail;

    sctk_spinlock_lock( &(lfq->lock));
    cell = vcli_raw_ptr_to_cell(pools_ptr,lfq->head); 
    
    if(lfq->head == 0)
    {
        sctk_spinlock_unlock( &(lfq->lock));
        return NULL;
    }

    if(cell->next != 0)
    {
        lfq->head = cell->next;
    }
    else
    {
        lfq->tail = 0;
        lfq->head = 0;
    }
    sctk_spinlock_unlock( &(lfq->lock));
#else /* ATOMIC_QUEUE */
    lfq_ptr_t head = lfq->shadow_head;

    lfq_ptr_t tail;

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
#endif /* ATOMIC_QUEUE */
    cell->next = 0;
    return (lfq_cell_t *) cell;
}

void sctk_vcli_raw_lfq_enqueue(volatile lf_queue_t *lfq, lfq_cell_t *pools_ptr, lfq_cell_t *cell)
{
    lfq_ptr_t new = vcli_raw_cell_to_ptr(pools_ptr, cell);
    lfq_ptr_t prev;
#ifndef ATOMIC_QUEUE
    sctk_spinlock_lock( &(lfq->lock));
    prev = lfq->tail;
    lfq->tail = new;
    if(prev == 0)
    {
        lfq->head = new;
    }
    else
    {
        lfq_cell_t* abs_prev = vcli_raw_ptr_to_cell(pools_ptr, prev);     
        abs_prev->next = new;
    }
    sctk_spinlock_unlock( &(lfq->lock));
#else /*  ATOMIC_QUEUE */
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
#endif /* ATOMIC_QUEUE */
}

/* */
vcli_raw_state_t *
sctk_vcli_raw_get_state(void *shmem_base, size_t shmem_size,int cells_num)
{
    int i;
    vcli_raw_state_t *vcli = NULL;

#ifdef VCLI_RAW_WITH_MPC
    vcli = (vcli_raw_state_t*) sctk_malloc(sizeof(vcli_raw_state_t));
    assume( vcli != NULL );
#else /* VCLI_RAW_WITH_MPC */
    vcli = (vcli_raw_state_t*) malloc(sizeof(vcli_raw_state_t));
#endif /* VCLI_RAW_WITH_MPC */
    
    const void *max_addr = (void*) ((char*) shmem_base + shmem_size);
    vcli->cells_num = cells_num;
    vcli->raw_queue_pop_lock = SCTK_SPINLOCK_INITIALIZER;
    vcli->send_queue = vcli_raw_queue_base(shmem_base,SCTK_QEMU_SHM_SEND_QUEUE_ID);
    vcli->recv_queue = vcli_raw_queue_base(shmem_base,SCTK_QEMU_SHM_RECV_QUEUE_ID);
    vcli->cmpl_queue = vcli_raw_queue_base(shmem_base,SCTK_QEMU_SHM_CMPL_QUEUE_ID);
    vcli->free_queue = vcli_raw_queue_base(shmem_base,SCTK_QEMU_SHM_FREE_QUEUE_ID);
    vcli->cells_pool  = vcli_raw_cell_pool_queue_base(shmem_base);
    assume((char*) vcli->cells_pool < (char*) max_addr);
    
    return vcli;
}

void 
sctk_vcli_raw_queue_reset(vcli_raw_state_t *vcli, vcli_queue_t queue)
{
    int i;
    lfq_cell_t *cell;
    memset((lf_queue_t*) vcli->send_queue, 0, sizeof(lf_queue_t));
    memset((lf_queue_t*) vcli->recv_queue, 0, sizeof(lf_queue_t));
    memset((lf_queue_t*) vcli->cmpl_queue, 0, sizeof(lf_queue_t));
    memset((lf_queue_t*) vcli->free_queue, 0, sizeof(lf_queue_t));
     
    for(i = 0; i < vcli->cells_num; i++)
    {
        cell = (lfq_cell_t *)(vcli->cells_pool)+i;
        cell->queue = queue;
	fprintf(stdout, "Add cell to queue: %d\n", queue);
        cell->next = 0;
        sctk_vcli_raw_lfq_enqueue(vcli->free_queue, vcli->cells_pool, cell);
        
    }
}
