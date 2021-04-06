#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mpc_common_rank.h>
#include "mpc_common_debug.h"
#include "sctk_shm_raw_queues_internals.h"
#include "sctk_raw_freelist_mthreads.h"

static void * 
sctk_shm_get_region_queue_base(char *ptr, sctk_shm_list_type_t type)
{
    return ptr + ( type * sizeof( sctk_shm_list_t ) );
}

static char *
sctk_shm_get_region_items_asymm_addr(char *ptr)
{
    /* First offset after all shmem queues */
    return ptr + (SCTK_SHM_CELLS_LIST_COUNT * sizeof(sctk_shm_list_t));
}

size_t 
sctk_shm_get_region_size(int cells_num)
{
    size_t size;
    /* shmem_queue size */
    size = SCTK_SHM_CELLS_LIST_COUNT * sizeof(sctk_shm_list_t);
    size += 128 + (cells_num + 5 + 1) * sizeof(sctk_shm_item_t);
    size += 4*1024;
    return size;
// ( ( uintptr_t ) page_align( size ) );
}

void 
sctk_shm_reset_region_queues(sctk_shm_region_infos_t *shmem, int rank)
{
    int i;
    sctk_shm_item_t * abs_item;
    char *item_asymm_addr;
    char *abs_ptr_item;

    item_asymm_addr = shmem->sctk_shm_asymm_addr;
    memset((sctk_shm_list_t*) shmem->send_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->recv_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->cmpl_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t*) shmem->free_queue, 0, sizeof(sctk_shm_list_t));
    memset((sctk_shm_list_t *)shmem->ctrl_queue, 0, sizeof(sctk_shm_list_t));
    //    memset((sctk_shm_list_t*) shmem->buff_queue, 0,
    //    sizeof(sctk_shm_list_t));

    abs_ptr_item = item_asymm_addr + (size_t) 128;

    for(i = 0; i < shmem->cells_num; i++)
    {
        abs_ptr_item += (size_t) sizeof(sctk_shm_item_t);
        assume_m((size_t)abs_ptr_item+sizeof(sctk_shm_item_t) < (size_t) shmem->max_addr, 
                            "Too small shmem memory region");
        abs_item = (sctk_shm_item_t *) abs_ptr_item;
        abs_item->src = rank;
        abs_item->next = 0;
        sctk_shm_enqueue_mt(shmem->free_queue,item_asymm_addr,abs_item,item_asymm_addr);
    }

    for (i = 0; i < 5; i++) {
      abs_ptr_item += (size_t)sizeof(sctk_shm_item_t);
      assume_m((size_t)abs_ptr_item + sizeof(sctk_shm_item_t) <
                   (size_t)shmem->max_addr,
               "Too small shmem memory region");
      abs_item = (sctk_shm_item_t *)abs_ptr_item;
      abs_item->src = rank;
      abs_item->next = 0;
      sctk_shm_enqueue_mt(shmem->ctrl_queue, item_asymm_addr, abs_item,
                          item_asymm_addr);
    }
}

/* */
sctk_shm_region_infos_t *
sctk_shm_set_region_infos(void *shmem_base, size_t shmem_size,int cells_num)
{
    sctk_shm_region_infos_t *shmem = NULL;

    shmem = (sctk_shm_region_infos_t*) sctk_malloc(sizeof(sctk_shm_region_infos_t));
    assume( shmem != NULL );
    
    shmem->cells_num = cells_num;
    shmem->all_shm_base = sctk_malloc( mpc_common_get_local_process_count() * sizeof(char*));
    shmem->shm_base = shmem_base;
    shmem->max_addr = shmem->shm_base + shmem_size;

    mpc_common_spinlock_init(&shmem->global_lock, 0);

    shmem->send_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_SEND);
    shmem->recv_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_RECV);
    shmem->cmpl_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_CMPL);
    shmem->free_queue = sctk_shm_get_region_queue_base(shmem_base,SCTK_SHM_CELLS_QUEUE_FREE);
    shmem->ctrl_queue = sctk_shm_get_region_queue_base(shmem_base, SCTK_SHM_CELLS_QUEUE_CTRL);

    assume_m((char *)shmem->ctrl_queue < shmem->max_addr, "Too small shmem memory region");

    shmem->sctk_shm_asymm_addr = sctk_shm_get_region_items_asymm_addr(shmem_base);
    return shmem;
}

void sctk_shm_internals_check(void)
{
    sctk_shm_cell_t* test1 = (sctk_shm_cell_t*) 2;
    sctk_shm_item_t* test2 = (sctk_shm_item_t*) 5;
    char * test3 = (char*) 2;

    assume_m(sctk_shm_item_to_cell(sctk_shm_cell_to_item(test1))==test1,"1 container of failed");
    assume_m(sctk_shm_cell_to_item(sctk_shm_item_to_cell(test2))==test2,"2 container of failed");
    assume_m(sctk_shm_abs_to_rel(test3,sctk_shm_rel_to_abs(test3,test2))==test2,"1 abs/rel failed");    assume_m(sctk_shm_rel_to_abs(test3,sctk_shm_abs_to_rel(test3,test2))==test2,"2 abs/rel failed");}
