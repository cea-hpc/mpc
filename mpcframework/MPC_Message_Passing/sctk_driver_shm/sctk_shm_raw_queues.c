#include <stdio.h>
#include "sctk_debug.h"
#include "sctk_shm_raw_queues.h"
#include "sctk_shm_raw_queues_helpers.h"
#include "sctk_shm_raw_queues_internals.h"

#include "sctk_raw_freelist_mthreads.h"

static sctk_shm_region_infos_t **sctk_shm_regions_infos = NULL;

static sctk_shm_region_infos_t * 
sctk_shm_get_region_infos(int dest)
{
    return sctk_shm_regions_infos[dest];
}

void sctk_shm_init_regions_infos(int participants)
{
    int i;
    sctk_shm_regions_infos = (sctk_shm_region_infos_t **) 
        sctk_malloc(sizeof(sctk_shm_region_infos_t *)*participants); 

    assume(sctk_shm_regions_infos != NULL);
    for(i = 0; i < participants; i++)
        sctk_shm_regions_infos[i] = NULL;
}

void 
sctk_shm_add_region_infos(char* shmem_base,size_t shmem_size,int cells_num, int rank )
{
    assume(sctk_shm_regions_infos != NULL);
    assume(sctk_shm_regions_infos[rank] == NULL);
    sctk_shm_regions_infos[rank]=sctk_shm_set_region_infos(shmem_base,shmem_size,cells_num);
    return; 
}

void sctk_shm_reset_process_queues(int process_rank)
{
    sctk_shm_reset_region_queues(sctk_shm_get_region_infos(process_rank),process_rank);
}

static volatile sctk_shm_list_t *
sctk_shm_get_queue_by_type(sctk_shm_list_type_t type, int process_rank)
{
    sctk_shm_region_infos_t *shmem = sctk_shm_get_region_infos(process_rank);
    volatile sctk_shm_list_t *queue = NULL;

    switch(type)
    {
        case SCTK_SHM_CELLS_QUEUE_SEND:
            queue = shmem->send_queue;
            break;
        case SCTK_SHM_CELLS_QUEUE_RECV:
            queue = shmem->recv_queue;
            break;
        case SCTK_SHM_CELLS_QUEUE_CMPL:
            queue = shmem->cmpl_queue;
            break;
        case SCTK_SHM_CELLS_QUEUE_FREE:
            queue = shmem->free_queue;
            break;
        default:
            printf("PROBLEM\n");
            sctk_abort();
    }
    return queue; 
}


sctk_shm_cell_t * 
sctk_shm_pop_cell_dest(sctk_shm_list_type_t type, int process_rank)
{
    sctk_shm_region_infos_t *item_shm_infos;
    sctk_shm_item_t * item;
    volatile sctk_shm_list_t *queue;
    
    item_shm_infos = sctk_shm_get_region_infos(process_rank);
    queue = sctk_shm_get_queue_by_type(type,process_rank);
    item = sctk_shm_dequeue_mt(queue,item_shm_infos->sctk_shm_asymm_addr);
    return sctk_shm_item_to_cell(item);
}

void
sctk_shm_push_cell_dest(sctk_shm_list_type_t type,sctk_shm_cell_t * cell, int process_rank)
{
    sctk_shm_item_t * item;
    volatile sctk_shm_list_t *queue;
    char *dest_asymm_addr, *item_asymm_addr;
    sctk_shm_region_infos_t *dest_shm_infos, *item_shm_infos;

    queue = sctk_shm_get_queue_by_type(type,process_rank);
    item = sctk_shm_cell_to_item(cell);
    item_shm_infos = sctk_shm_get_region_infos(item->src);
    item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;
    
    /* push item in its origin queue so asymm addr are equal */
    if(process_rank == item->src)
    {
        sctk_shm_enqueue_mt(queue,item_asymm_addr,item,item_asymm_addr); 
        return;
    }

    dest_shm_infos = sctk_shm_get_region_infos(process_rank);
    dest_asymm_addr = dest_shm_infos->sctk_shm_asymm_addr;
    sctk_shm_enqueue_mt(queue,dest_asymm_addr,item,item_asymm_addr);
}

void 
sctk_shm_push_cell_origin(sctk_shm_list_type_t type, sctk_shm_cell_t *cell)
{
    sctk_shm_item_t * item;
    volatile sctk_shm_list_t *queue;
    char *item_asymm_addr;
    sctk_shm_region_infos_t *item_shm_infos;

    item = sctk_shm_cell_to_item(cell);
    queue = sctk_shm_get_queue_by_type(type,item->src);
    item_shm_infos = sctk_shm_get_region_infos(item->src);
    item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;
    sctk_shm_enqueue_mt(queue,item_asymm_addr,item,item_asymm_addr);
}

int 
sctk_shm_isempty_process_queue(sctk_shm_list_type_t type, int process_rank)
{
    return sctk_shm_queue_isempty(sctk_shm_get_queue_by_type(type,process_rank));
}
