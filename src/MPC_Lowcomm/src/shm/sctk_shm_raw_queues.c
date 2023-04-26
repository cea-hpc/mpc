#include <stdio.h>
#include "mpc_common_debug.h"
#include "sctk_shm_raw_queues.h"
#include "sctk_shm_raw_queues_helpers.h"
#include "sctk_shm_raw_queues_internals.h"
#include <mpc_common_rank.h>
#include "sctk_raw_freelist_mthreads.h"

static sctk_shm_region_infos_t **sctk_shm_regions_infos = NULL;

static sctk_shm_region_infos_t * __get_region(int rank)
{
	return sctk_shm_regions_infos[rank];
}

void sctk_shm_init_regions_infos(int participants)
{
	int i;

	sctk_shm_regions_infos = (sctk_shm_region_infos_t **)
	                         sctk_malloc(sizeof(sctk_shm_region_infos_t *) * participants);

	assume(sctk_shm_regions_infos != NULL);
	for(i = 0; i < participants; i++)
	{
		sctk_shm_regions_infos[i] = NULL;
	}
}

void sctk_shm_free_regions_infos()
{
	sctk_free(sctk_shm_regions_infos);
	sctk_shm_regions_infos = NULL;
}

void
sctk_shm_add_region_infos(char *shmem_base, size_t shmem_size, int cells_num, int rank)
{
	assume(sctk_shm_regions_infos != NULL);
	assume(sctk_shm_regions_infos[rank] == NULL);
	sctk_shm_regions_infos[rank] = sctk_shm_set_region_infos(shmem_base, shmem_size, cells_num);
}

void sctk_shm_reset_process_queues(int rank)
{
	sctk_shm_reset_region_queues(__get_region(rank), rank);
}

static sctk_shm_list_t * __get_queue(sctk_shm_region_infos_t *shmem, sctk_shm_list_type_t type)
{
	sctk_shm_list_t *        queue = NULL;

	switch(type)
	{

		case SCTK_SHM_CELLS_QUEUE_RECV:
			queue = shmem->recv_queue;
			break;

		case SCTK_SHM_CELLS_QUEUE_FREE:
			queue = shmem->free_queue;
			break;

		case SCTK_SHM_CELLS_QUEUE_CTRL:
			queue = shmem->ctrl_queue;
			break;

		default:
			printf("PROBLEM\n");
			mpc_common_debug_abort();
	}
	return queue;
}

sctk_shm_cell_t * sctk_shm_pop_cell_dest(sctk_shm_list_type_t type, int rank)
{
	sctk_shm_region_infos_t *item_shm_infos;
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;

	item_shm_infos = __get_region(rank);
	queue          = __get_queue(item_shm_infos, type);
	item           = sctk_shm_dequeue_mt(queue, item_shm_infos->sctk_shm_asymm_addr);
	return sctk_shm_item_to_cell(item);
}

sctk_shm_cell_t * sctk_shm_recv_cell(void)
{
	sctk_shm_region_infos_t *item_shm_infos;
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;

	item_shm_infos = __get_region(mpc_common_get_local_process_rank() );
	queue          = __get_queue(item_shm_infos, SCTK_SHM_CELLS_QUEUE_RECV);
	item           = sctk_shm_dequeue_mt(queue, item_shm_infos->sctk_shm_asymm_addr);

	return sctk_shm_item_to_cell(item);
}

sctk_shm_cell_t * sctk_shm_get_cell(int dest, int is_control_msg)
{
	sctk_shm_region_infos_t *item_shm_infos;
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;

	item_shm_infos = __get_region(dest);
	queue          = __get_queue(item_shm_infos, SCTK_SHM_CELLS_QUEUE_FREE);
	item           = sctk_shm_dequeue_mt(queue, item_shm_infos->sctk_shm_asymm_addr);

	if(!item && is_control_msg)
	{
		queue = __get_queue(item_shm_infos, SCTK_SHM_CELLS_QUEUE_CTRL);
		item  = sctk_shm_dequeue_mt(queue, item_shm_infos->sctk_shm_asymm_addr);
	}

	return sctk_shm_item_to_cell(item);
}

void sctk_shm_send_cell(sctk_shm_cell_t *cell)
{
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;
	char *                   item_asymm_addr;
	sctk_shm_region_infos_t *item_shm_infos;

	item            = sctk_shm_cell_to_item(cell);
	item_shm_infos  = __get_region(item->src);
	item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;
	queue           = __get_queue(item_shm_infos, SCTK_SHM_CELLS_QUEUE_RECV);
	sctk_shm_enqueue_mt(queue, item_asymm_addr, item, item_asymm_addr);
}

void sctk_shm_release_cell(sctk_shm_cell_t *cell)
{
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;
	char *                   item_asymm_addr;
	sctk_shm_region_infos_t *item_shm_infos;

	item            = sctk_shm_cell_to_item(cell);
	item_shm_infos  = __get_region(item->src);
	item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;
	queue           = __get_queue(item_shm_infos, SCTK_SHM_CELLS_QUEUE_FREE);
	sctk_shm_enqueue_mt(queue, item_asymm_addr, item, item_asymm_addr);
}

void sctk_shm_push_cell_dest(sctk_shm_list_type_t type, sctk_shm_cell_t *cell, int process_rank)
{
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;
	char *                   dest_asymm_addr, *item_asymm_addr;
	sctk_shm_region_infos_t *dest_shm_infos, *item_shm_infos;

	item = sctk_shm_cell_to_item(cell);
	assume_m( (unsigned int)process_rank == item->src, "Empty cell must be get from dest queue");

	dest_shm_infos  = __get_region(process_rank);
	queue = __get_queue(dest_shm_infos, type);

	item_shm_infos  = __get_region(item->src);
	item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;


	dest_asymm_addr = dest_shm_infos->sctk_shm_asymm_addr;

	sctk_shm_enqueue_mt(queue, dest_asymm_addr, item, item_asymm_addr);
}

void
sctk_shm_push_cell_origin(sctk_shm_list_type_t type, sctk_shm_cell_t *cell)
{
	sctk_shm_item_t *        item;
	sctk_shm_list_t *        queue;
	char *                   item_asymm_addr;
	sctk_shm_region_infos_t *item_shm_infos;

	item            = sctk_shm_cell_to_item(cell);

	item_shm_infos  = __get_region(item->src);
	queue           = __get_queue(item_shm_infos, type);

	item_asymm_addr = item_shm_infos->sctk_shm_asymm_addr;
	sctk_shm_enqueue_mt(queue, item_asymm_addr, item, item_asymm_addr);
}

int
sctk_shm_isempty_process_queue(sctk_shm_list_type_t type, int process_rank)
{
	sctk_shm_region_infos_t * item_shm_infos  = __get_region(process_rank);
	return sctk_shm_queue_isempty(__get_queue(item_shm_infos, type) );
}
