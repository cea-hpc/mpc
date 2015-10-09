#include <stdio.h>
#include "sctk_debug.h"
#include "sctk_shm_raw_queues.h"
#include "sctk_shm_raw_queues_helpers.h"
#include "sctk_shm_raw_queues_internals.h"

static vcli_raw_state_t **sctk_vcli_raw_infos = NULL;

uint32_t sctk_shm_get_raw_queues_size(int vcli_cells_num)
{
    return sctk_shm_raw_queues_size(vcli_cells_num);
}

void *sctk_vcli_get_raw_infos( vcli_queue_t queue )
{
 //   assume( sctk_vcli_raw_infos != NULL );
 //   assume( sctk_vcli_raw_infos[queue] != NULL );
    return sctk_vcli_raw_infos[queue];
}

void sctk_vcli_raw_infos_init(int participants)
{
    int i;
    sctk_vcli_raw_infos = (vcli_raw_state_t **) 
        sctk_malloc(sizeof(vcli_raw_state_t *)*participants); 
    assume(sctk_vcli_raw_infos != NULL);
    for(i=0; i < participants; i++)
        sctk_vcli_raw_infos[i] = NULL;
}

void sctk_vcli_raw_infos_add(void* shmem_base, size_t shmem_size, 
                                int cells_num, int vcli_master )
{
    assume(sctk_vcli_raw_infos != NULL);
    assume(sctk_vcli_raw_infos[vcli_master] == NULL);
    sctk_vcli_raw_infos[vcli_master] = 
    sctk_vcli_raw_get_state(shmem_base,shmem_size,cells_num);
    assume(sctk_vcli_raw_infos[vcli_master] != NULL);
    return; 
}

void vcli_raw_reset_infos_reset( vcli_queue_t queue )
{
    sctk_vcli_raw_queue_reset(sctk_vcli_get_raw_infos(queue),queue);
}

volatile lf_queue_t *
vcli_raw_get_queue_by_type(vcli_raw_shm_queue_type_t type, vcli_queue_t queue)
{
    vcli_raw_state_t *vcli = sctk_vcli_get_raw_infos(queue);
    volatile lf_queue_t *type_queue = NULL;

    switch(type)
    {
        case SCTK_QEMU_SHM_SEND_QUEUE_ID:
            type_queue = vcli->send_queue;
            break;
        case SCTK_QEMU_SHM_RECV_QUEUE_ID:
            type_queue = vcli->recv_queue;
            break;
        case SCTK_QEMU_SHM_CMPL_QUEUE_ID:
            type_queue = vcli->cmpl_queue;
            break;
        case SCTK_QEMU_SHM_FREE_QUEUE_ID:
            type_queue = vcli->free_queue;
            break;
        default:
            printf("PROBLEM\n");
    }
    return type_queue; 
}


vcli_cell_t * 
vcli_raw_pop_cell(vcli_raw_shm_queue_type_t type, vcli_queue_t queue)
{
    vcli_cell_t * _cell;
    vcli_raw_state_t *vcli = sctk_vcli_get_raw_infos(queue);
    volatile lf_queue_t *type_queue = vcli_raw_get_queue_by_type(type, queue);
    _cell = lfq_to_vcli(lfq_dequeue(type_queue,vcli->cells_pool));

#ifdef SCTK_SHM_RAW_QUEUE_DEBUG
    if( _cell != NULL && type == SCTK_QEMU_SHM_FREE_QUEUE_ID)
    {
        sctk_spinlock_lock( &(type_queue->lock));
        fprintf(stderr, "[%d] get empty cell cell : %lu / %lu \n", queue, type_queue->current_cellules, 255);
        sctk_spinlock_unlock( &(type_queue->lock));
    }
#endif /* SCTK_SHM_RAW_QUEUE_DEBUG */
    return _cell;
}

void 
vcli_raw_push_cell(vcli_raw_shm_queue_type_t type, vcli_cell_t * _cell)
{
    lfq_cell_t *cell = vcli_to_lfq(_cell);
    vcli_raw_state_t *vcli = sctk_vcli_get_raw_infos(cell->queue);
    volatile lf_queue_t *type_queue = vcli_raw_get_queue_by_type(type,cell->queue);
    sctk_vcli_raw_lfq_enqueue(type_queue,vcli->cells_pool,cell);
#ifdef SCTK_SHM_RAW_QUEUE_DEBUG
    if( type == SCTK_QEMU_SHM_FREE_QUEUE_ID)
    {
        sctk_spinlock_lock( &(type_queue->lock));
        fprintf(stderr, "[%d] release cell : %lu / %lu \n", cell->queue, type_queue->current_cellules, 255);
        sctk_spinlock_unlock( &(type_queue->lock));
    }
#endif /* SCTK_SHM_RAW_QUEUE_DEBUG */
}

void
vcli_raw_push_cell_dest(vcli_raw_shm_queue_type_t type, vcli_cell_t * _cell,vcli_queue_t queue)
{
    lfq_cell_t *cell = vcli_to_lfq(_cell);
    vcli_raw_state_t *vcli = sctk_vcli_get_raw_infos(queue);
    volatile lf_queue_t *type_queue = vcli_raw_get_queue_by_type(type,queue);
    assume_m( type != SCTK_QEMU_SHM_FREE_QUEUE_ID, "Release free cell in wrong queue")
    sctk_vcli_raw_lfq_enqueue(type_queue,vcli->cells_pool,cell);
}

int 
vcli_raw_empty_queue(vcli_raw_shm_queue_type_t type,vcli_queue_t queue)
{
    volatile lf_queue_t *type_queue = vcli_raw_get_queue_by_type(type,queue);
    return lfq_empty(type_queue);
}
