#include <mpc_common_debug.h>
#include <mpc_config.h>
#include "endpoint.h"
#include "mpc_launch_pmi.h"
#include <mpc_launch_shm.h>
#include "sctk_alloc.h"

#include <sctk_net_tools.h>
#include "sctk_shm_raw_queues.h"
#include "sctk_shm.h"
#include "utlist.h"
#include "sctk_shm_raw_queues_internals.h"

#include "sctk_rail.h"

#include <mpc_common_rank.h>


static unsigned int sctk_shm_send_max_try = 2;

// FROM Henry S. Warren, Jr.'s "Hacker's Delight."
static long sctk_shm_roundup_powerof2(unsigned long n)
{
	assume(n < (unsigned long)(1 << 31) );
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

#if 0
void sctk_shm_network_rdma_write(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                                 void *src_addr, struct sctk_rail_pin_ctx_list *local_key,
                                 void *dest_addr, struct  sctk_rail_pin_ctx_list *remote_key,
                                 size_t size)
{
}
void sctk_shm_network_rdma_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                                void *src_addr, struct  sctk_rail_pin_ctx_list *remote_key,
                                void *dest_addr, struct  sctk_rail_pin_ctx_list *local_key,
                                size_t size)
{
}

#endif

static void
sctk_network_add_message_to_pending_shm_list(mpc_lowcomm_ptp_message_t *msg, sctk_rail_info_t *rail, int sctk_shm_dest, int with_lock)
{
	sctk_shm_msg_list_t *tmp = sctk_malloc(sizeof(sctk_shm_msg_list_t) );
	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;


	tmp->msg           = msg;
	tmp->sctk_shm_dest = sctk_shm_dest;
	if(with_lock)
	{
		mpc_common_spinlock_lock(&shm_driver_info->pending_lock);
	}
	DL_APPEND(shm_driver_info->pending_msg_list, tmp);
	shm_driver_info->pending_msg_num++;
	if(with_lock)
	{
		mpc_common_spinlock_unlock(&shm_driver_info->pending_lock);
	}
}

static int __send_message_dest(mpc_lowcomm_ptp_message_t *msg,
                               sctk_rail_info_t *rail,
                               int sctk_shm_dest,
                               int with_lock)
{
	sctk_shm_cell_t *cell = NULL;
	unsigned int     sctk_shm_send_cur_try;
	int is_message_control = 0;
	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;


	if(_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg) ) )
	{
		is_message_control = 1;
	}

	sctk_shm_send_cur_try = 0;
	while(!cell && sctk_shm_send_cur_try++ < sctk_shm_send_max_try)
	{
		cell = sctk_shm_get_cell(sctk_shm_dest, is_message_control);
	}

	if(!cell)
	{
		sctk_network_add_message_to_pending_shm_list(msg, rail, sctk_shm_dest, with_lock);
		return 1;
	}

	cell->dest = sctk_shm_dest;
	cell->src  = SCTK_MSG_SRC_PROCESS(msg);

	if(sctk_network_eager_msg_shm_send(msg, cell) )
	{
		return 0;
	}
#ifdef MPC_USE_CMA
	if(sctk_network_cma_msg_shm_send(msg, cell) && shm_driver_info->cma_enabled)
	{
		return 0;
	}
#endif  /* MPC_USE_CMA */
	if(sctk_network_frag_msg_shm_send(msg, cell) )
	{
		return 0;
	}

	sctk_shm_release_cell(cell);
	sctk_network_add_message_to_pending_shm_list(msg, rail, sctk_shm_dest, with_lock);

	return 1;
}

static void __send_msg_from_pending_list(sctk_rail_info_t *rail)
{
	sctk_shm_msg_list_t *elt = NULL;
	sctk_shm_msg_list_t *tmp = NULL;
	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;

	if(!shm_driver_info->pending_msg_num)
	{
		return;
	}

	if(mpc_common_spinlock_trylock(&shm_driver_info->pending_lock) )
	{
		return;
	}


	DL_FOREACH_SAFE(shm_driver_info->pending_msg_list, elt, tmp)
	{
		DL_DELETE(shm_driver_info->pending_msg_list, elt);
		if(elt)
		{
			if(__send_message_dest(elt->msg, rail, elt->sctk_shm_dest, 0) )
			{
				/* If we are here the element
				 * was back pushed to the pending list */

				/* mpc_lowcomm_ptp_message_t *msg = elt->msg;
				* mpc_common_debug(
				*  "!%d!  [ %d -> %d ] [ %d -> %d ] (CLASS "
				*  "%s(%d) SPE %d SIZE %d TAG %d)",
				*  mpc_common_get_process_rank(), SCTK_MSG_SRC_PROCESS(msg),
				*  SCTK_MSG_DEST_PROCESS(msg), SCTK_MSG_SRC_TASK(msg),
				*  SCTK_MSG_DEST_TASK(msg),
				*  mpc_lowcomm_ptp_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
				*  SCTK_MSG_SPECIFIC_CLASS(msg),
				*  _mpc_comm_ptp_message_is_for_process(
				*      SCTK_MSG_SPECIFIC_CLASS(msg)),
				*  SCTK_MSG_SIZE(msg), SCTK_MSG_TAG(msg)); */

				sctk_free( (void *)elt);
				shm_driver_info->pending_msg_num--;
			}
		}
	}

	mpc_common_spinlock_unlock(&shm_driver_info->pending_lock);
}

static void
_mpc_lowcomm_shm_send_message(mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	__send_message_dest(msg, endpoint->rail, endpoint->data.shm.dest, 1);
}

static void
_mpc_lowcomm_shm_notify_matching(__UNUSED__ mpc_lowcomm_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail)
{
}

static int
sctk_send_message_from_network_shm(mpc_lowcomm_ptp_message_t *msg)
{
	if(sctk_send_message_from_network_reorder(msg) == REORDER_NO_NUMBERING)
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check(msg, 1);
	}
	return 1;
}

static inline void __shm_poll(sctk_rail_info_t *rail)
{
	sctk_shm_cell_t *          cell;
	mpc_lowcomm_ptp_message_t *msg;
	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;

	if(!shm_driver_info->driver_initialized)
	{
		return;
	}

	if(mpc_common_spinlock_trylock(&shm_driver_info->polling_lock) )
	{
		return;
	}


	while(1)
	{
		cell = sctk_shm_recv_cell();
		if(!cell)
		{
			break;
		}

		switch(cell->msg_type)
		{
			case SCTK_SHM_EAGER:
				msg = sctk_network_eager_msg_shm_recv(cell, 0);
				if(msg)
				{
					sctk_send_message_from_network_shm(msg);
				}
				break;

#ifdef MPC_USE_CMA
			case SCTK_SHM_RDMA:
				msg = sctk_network_cma_msg_shm_recv(cell, 1);
				if(msg)
				{
					sctk_send_message_from_network_shm(msg);
				}
				break;

			case SCTK_SHM_CMPL:
				msg = sctk_network_cma_cmpl_msg_shm_recv(cell);
				mpc_lowcomm_ptp_message_complete_and_free(msg);
				break;
#endif                  /* MPC_USE_CMA */
			case SCTK_SHM_FIRST_FRAG:
			case SCTK_SHM_NEXT_FRAG:
				msg = sctk_network_frag_msg_shm_recv(cell);
				if(msg)
				{
					sctk_send_message_from_network_shm(msg);
				}
				break;

			default:
				abort();
		}
	}

	if(!cell)
	{
		sctk_network_frag_msg_shm_idle(1);
		__send_msg_from_pending_list(rail);
	}

	mpc_common_spinlock_unlock(&shm_driver_info->polling_lock);
}

static void _mpc_lowcomm_shm_notify_idle(sctk_rail_info_t *rail)
{
	__shm_poll(rail);
}

static void
_mpc_lowcomm_shm_notify_perform(__UNUSED__ int remote, __UNUSED__ int remote_task_id, __UNUSED__ int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail)
{
	__shm_poll(rail);
}

static void _mpc_lowcomm_shm_notify_receive(__UNUSED__ mpc_lowcomm_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail)
{
}

static void
_mpc_lowcomm_shm_notify_anysource(__UNUSED__ int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail)
{
	__shm_poll(rail);
}

/********************************************************************/
/* SHM Init                                                         */
/********************************************************************/


static void sctk_shm_add_route(int dest, int shm_dest, sctk_rail_info_t *rail)
{
	_mpc_lowcomm_endpoint_t *new_route;

	/* Allocate a new route */
	new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
	assume(new_route != NULL);

	_mpc_lowcomm_endpoint_init(new_route, dest, rail, _MPC_LOWCOMM_ENDPOINT_STATIC);

	new_route->data.shm.dest = shm_dest;

	/* Add the new route */
	sctk_rail_add_static_route(rail, new_route);

	/* set the route as connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	//fprintf(stdout, "Add route to %d with endpoint %d\n", dest, shm_dest);
	return;
}

static void sctk_shm_init_raw_queue(size_t size, int cells_num, int rank)
{
	void *shm_base = NULL;

	shm_base = mpc_launch_shm_map(size, MPC_LAUNCH_SHM_USE_PMI, NULL);

	sctk_shm_add_region_infos(shm_base, size, cells_num, rank);

	if(mpc_common_get_local_process_rank() == rank)
	{
		sctk_shm_reset_process_queues(rank);
	}
}

static void sctk_shm_free_raw_queue(__UNUSED__ int i)
{
}

void sctk_shm_check_raw_queue(int local_process_number)
{
	int i;

	for(i = 0; i < local_process_number; i++)
	{
		assume_m(sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_SEND, i), "Queue must be empty")
		assume_m(sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_RECV, i), "Queue must be empty")
		assume_m(sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_CMPL, i), "Queue must be empty")
		assume_m(!sctk_shm_isempty_process_queue(SCTK_SHM_CELLS_QUEUE_FREE, i), "Queue must be full")
	}
}

void sctk_network_finalize_shm(__UNUSED__ sctk_rail_info_t *rail)
{
	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;

	if(!shm_driver_info->driver_initialized)
	{
		return;
	}

	/** complementary condition to init func: @see sctk_network_inif_shm */
	sctk_network_frag_shm_interface_free();
#ifdef MPC_USE_CMA
	sctk_shm_network_cma_shm_interface_free();
#endif
	int nb_process = -1, i = -1;
	for(i = 0; i < nb_process; ++i)
	{
		sctk_shm_free_raw_queue(i);
	}
	sctk_shm_free_regions_infos();
	/*TODO: move every global data into sctk_shm_rail_info_t struct */
}

/*! \brief Generate filename with localhost and pid
 * @param option No option implemented
 */

void sctk_network_init_shm(sctk_rail_info_t *rail)
{
	int    i, local_process_rank, local_process_number;
	size_t sctk_shmem_size;
	int    sctk_shmem_cells_num;

	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_shm_send_message;
	rail->notify_recv_message       = _mpc_lowcomm_shm_notify_receive;
	rail->notify_matching_message   = _mpc_lowcomm_shm_notify_matching;
	rail->notify_any_source_message = _mpc_lowcomm_shm_notify_anysource;
	rail->notify_perform_message    = _mpc_lowcomm_shm_notify_perform;
	rail->notify_idle_message       = _mpc_lowcomm_shm_notify_idle;
	rail->send_message_from_network = sctk_send_message_from_network_shm;

	rail->network_name    = "SHM";
	rail->driver_finalize = sctk_network_finalize_shm;
//   if( strcmp(rail->runtime_config_rail->topology, "none"))
//	mpc_common_nodebug("SHM topology must be 'none'");

	sctk_shm_rail_info_t *shm_driver_info = &rail->network.shm;

	/* Init driver info */

	shm_driver_info->driver_initialized = 0;
	shm_driver_info->pending_msg_num = 0;
	mpc_common_spinlock_init(&shm_driver_info->polling_lock, 0);
	mpc_common_spinlock_init(&shm_driver_info->pending_lock, 0);
	shm_driver_info->pending_msg_list = NULL;
	/* Base init done */

	sctk_rail_init_route(rail, "none", NULL);

	sctk_shmem_cells_num = rail->runtime_config_driver_config->driver.value.shm.cells_num;
	sctk_shmem_size      = sctk_shm_get_region_size(sctk_shmem_cells_num);
	sctk_shmem_size      = sctk_shm_roundup_powerof2(sctk_shmem_size);

	local_process_rank   = mpc_common_get_local_process_rank();
	local_process_number = mpc_common_get_local_process_count();

	if(local_process_number == 1 || mpc_common_get_node_count() > 1)
	{
		shm_driver_info->driver_initialized = 0;
		return;
	}

	struct mpc_launch_pmi_process_layout *nodes_infos = NULL;
	mpc_launch_pmi_get_process_layout(&nodes_infos);

	int node_rank = mpc_common_get_node_rank();

	struct mpc_launch_pmi_process_layout *tmp;

	HASH_FIND_INT(nodes_infos, &node_rank, tmp);
	assert(tmp != NULL);

	sctk_shm_init_regions_infos(local_process_number);
	for(i = 0; i < tmp->nb_process; i++)
	{
		sctk_shm_init_raw_queue(sctk_shmem_size, sctk_shmem_cells_num, i);
		if(i != local_process_rank)
		{
			sctk_shm_add_route(tmp->process_list[i], i, rail);
		}
	}


#ifdef MPC_USE_CMA
	shm_driver_info->cma_enabled = rail->runtime_config_driver_config->driver.value.shm.cma_enable;
	(void)sctk_network_cma_shm_interface_init(0);
#else
	shm_driver_info->cma_enabled = 0;
#endif  /* MPC_USE_CMA */

	sctk_network_frag_shm_interface_init();
	//sctk_shm_check_raw_queue(local_process_number);
	shm_driver_info->driver_initialized = 1;
}
