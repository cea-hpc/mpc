#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sctk_debug.h"

#include "sctk_shm_raw_queues_internals.h"
#include "sctk_raw_freelist_mthreads.h"

static void * 
sctk_shm_get_region_queue_base(char *ptr,sctk_shm_list_type_t type)
{
    return ptr + ( type * sizeof( sctk_shm_list_t ) );
}

static char *
sctk_shm_get_region_items_asymm_addr(char *ptr)
{
    /* First offset after all shmem queues */
    return ptr + (4 * sizeof(sctk_shm_list_t));
}

size_t 
sctk_shm_get_region_size(int cells_num)
{
    void *size = NULL;
    size_t tmp;
    /* shmem_queue size */
    tmp = 4 * sizeof(sctk_shm_list_t);
    size += tmp;
    /* shmem_cell_pool size */
    tmp = cells_num * sizeof(sctk_shm_item_t);
    size += tmp;
    return ( ( uintptr_t ) page_align( size ) );
}

void 
sctk_shm_reset_region_queues(sctk_shm_region_infos_t *shmem, int process_rank)
{
    int i;
    sctk_shm_item_t * abs_item;
    char * item_asymm_addr;

    item_asymm_addr = shmem->sctk_shm_asymm_addr;

    memset((sctk_shm_list_t*) shmem->send_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->recv_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->cmpl_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->free_queue, 0, sizeof(sctk_shm_list_t));

    for(i = 0; i < shmem->cells_num; i++)
    {
        abs_item = (sctk_shm_item_t *)(item_asymm_addr)+i;
        abs_item->src = process_rank;
        abs_item->next = 0;
        sctk_shm_enqueue_mt(shmem->free_queue,item_asymm_addr,abs_item,item_asymm_addr);
    }
}

/* */
sctk_shm_region_infos_t *
sctk_shm_set_region_infos(void *shmem_base, size_t shmem_size,int cells_num)
{
    int i;
    sctk_shm_region_infos_t *shmem = NULL;

    shmem = (sctk_shm_region_infos_t*) sctk_malloc(sizeof(sctk_shm_region_infos_t));
    assume( shmem != NULL );
    
    shmem->cells_num = cells_num;
    shmem->global_lock = SCTK_SPINLOCK_INITIALIZER;
    shmem->send_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_SEND);
    shmem->recv_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_RECV);
    shmem->cmpl_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_CMPL);
    shmem->free_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_FREE);
    shmem->sctk_shm_asymm_addr = sctk_shm_get_region_items_asymm_addr(shmem_base);
    return shmem;
}

