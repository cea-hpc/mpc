/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universite de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#include "ib.h"
#include "cp.h"
#include "ibpolling.h"
#include "ibufs.h"
#include "ibmmu.h"
#include "msg_cpy.h"
#include "ibeager.h"
#include "mpc_common_asm.h"
#include "utlist.h"
#include "ibmpi.h"
#include "ibrdma.h"
#include "ibtopology.h"
#include "rail.h"

#include <mpc_topology.h>
#include <sctk_alloc.h>

#if defined MPC_LOWCOMM_IB_MODULE_NAME
#error "MPC_LOWCOMM_IB_MODULE already defined"
#endif
//#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME    "CP"
#include "ibtoolkit.h"
#include "math.h"

#define MPC_LOWCOMM_IB_PROFILER

extern mpc_lowcomm_request_t *blocked_request;
/* used to remember __thread var init for IB re-enabling */
extern volatile char *vps_reset;

typedef struct _mpc_lowcomm_ib_cp_ctx_s
{
	/* Void for now */
} _mpc_lowcomm_ib_cp_ctx_t;

struct __mpc_lowcomm_ib_numa_s;

typedef struct __mpc_lowcomm_ib_vp_s
{
	int                             number; /**< Number */
	_mpc_lowcomm_ib_cp_task_t *     tasks;
	struct __mpc_lowcomm_ib_numa_s *node;   /**< Pointer to the numa node where the VP is located */
	/* For CDL list */
	struct __mpc_lowcomm_ib_vp_s *  prev;
	struct __mpc_lowcomm_ib_vp_s *  next;
	char                            pad[128];
} __mpc_lowcomm_ib_vp_t;

typedef struct __mpc_lowcomm_ib_numa_s
{
	int                             number; /**< Number */
	struct __mpc_lowcomm_ib_vp_s *  vps;    /**< Circular list of VPs */
	int                             vp_nb;  /**< Number of VPs on the numa node*/
	/* For CDL list */
	struct __mpc_lowcomm_ib_numa_s *prev;
	struct __mpc_lowcomm_ib_numa_s *next;
	int                             tasks_nb;
	_mpc_lowcomm_ib_cp_task_t *     tasks;    /** CDL of tasks for a NUMA node */
	char                            pad[128]; /**< Padding avoiding false sharing */
} __mpc_lowcomm_ib_numa_t;

/** Array of numa nodes */
static mpc_common_spinlock_t __numas_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static __mpc_lowcomm_ib_numa_t ** numas = NULL;

static int numa_number = -1;
int        __number_of_registered_numa = 0;

/** Array of vps */
static __mpc_lowcomm_ib_vp_t **vps        = NULL;
static mpc_common_spinlock_t   __vps_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static _mpc_lowcomm_ib_cp_task_t *all_tasks = NULL;

/** NUMA node where the task is located */
/** vars used to reducde latency access */
static __thread int task_node_number = -1;
static __thread __mpc_lowcomm_ib_vp_t *   __vp_tls_save = NULL;
static __thread unsigned int              seed;
static __thread __mpc_lowcomm_ib_numa_t **__vp_numa_descriptor = NULL;

#define CHECK_AND_QUIT(rail_ib)    do {    \
		LOAD_CONFIG(rail_ib);      \
} while(0)

OPA_int_t cp_nb_pending_msg;

void _mpc_lowcomm_ib_cp_ctx_incr_nb_pending_msg()
{
	OPA_incr_int(&cp_nb_pending_msg);
}

void _mpc_lowcomm_ib_cp_ctx_decr_nb_pending_msg()
{
	OPA_decr_int(&cp_nb_pending_msg);
}

int _mpc_lowcomm_ib_cp_ctx_get_nb_pending_msg()
{
	return OPA_load_int(&cp_nb_pending_msg);
}

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/
#if 0
_mpc_lowcomm_ib_cp_task_t *_mpc_lowcomm_ib_cp_ctx_get_task(int rank)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;

	/* XXX: Do not support thread migration */
	HASH_FIND(hh_all, all_tasks, &rank, sizeof(int), task);
	ib_assume(task);

	return task;
}

#endif

/* XXX: is called before 'cp_init_task' */
void _mpc_lowcomm_ib_cp_ctx_init(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	CHECK_AND_QUIT(rail_ib);
	_mpc_lowcomm_ib_cp_ctx_t *cp;

	cp = sctk_malloc(sizeof(_mpc_lowcomm_ib_cp_ctx_t) );
	ib_assume(cp);
	rail_ib->cp = cp;
	OPA_store_int(&cp_nb_pending_msg, 0);
	numa_number = -1;
	__number_of_registered_numa = 0;
	assume(numas == NULL);
	assume(all_tasks == NULL);
}

void _mpc_lowcomm_ib_cp_ctx_finalize(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	int i;

	mpc_common_spinlock_lock(&__vps_lock);
	/* free vps struct */
	int nbvps = mpc_topology_get_pu_count();
	if(vps)
	{
		for(i = 0; i < nbvps; ++i)
		{
			if(vps[i])
			{
				int node_number = vps[i]->node->number;
				numas[node_number]->tasks_nb--;
				CDL_DELETE(numas[node_number]->vps, vps[i]);
				sctk_free(vps[i]);
			}
		}
		sctk_free(vps);
		vps = NULL;
	}

	/* free numas struct */
	int max_node = mpc_topology_get_numa_node_count();
	max_node = (max_node < 1) ? 1 : max_node;

	mpc_common_spinlock_lock(&__numas_lock);
	if(numas)
	{
		for(i = 0; i < max_node; ++i)
		{
			if(numas[i])
			{
				sctk_free( (void *)numas[i]);
				numas[i] = NULL;
			}
		}
		sctk_free( (void *)numas);
		numas = NULL;
	}
	mpc_common_spinlock_unlock(&__numas_lock);

	_mpc_lowcomm_ib_cp_task_t *tofree = NULL, *tmp = NULL;
	HASH_ITER(hh_all, all_tasks, tofree, tmp)
	{
		HASH_DELETE(hh_all, all_tasks, tofree);
		sctk_free(tofree);
	}
	assert(HASH_CNT(hh_all, all_tasks) == 0); /* all elements should be removed */
	mpc_common_spinlock_unlock(&__vps_lock);
	sctk_free(rail_ib->cp);
	rail_ib->cp = NULL;
	OPA_store_int(&cp_nb_pending_msg, 0);
}

static _mpc_lowcomm_ib_ibuf_t * volatile __global_ibufs_list      = NULL;
static mpc_common_spinlock_t   __global_ibufs_list_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

void _mpc_lowcomm_ib_cp_ctx_init_task(int rank, int vp)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;
	int node = mpc_topology_get_numa_node_from_cpu(vp);
	/* Process specific list of messages */

	mpc_common_spinlock_lock(&__vps_lock);
	/* If the task has already been added, we do not add it once again and skip */
	HASH_FIND(hh_all, all_tasks, &rank, sizeof(int), task);

	if(task)
	{
		mpc_common_spinlock_unlock(&__vps_lock);
		return;
	}

	/* check if the current __thread vars need to be reset (once per VP)
	 * This is ok without locks, because MPC do not preempt user-level threads on top of VP
	 * */
	if(vps_reset[vp] == 0)
	{
		/* re-init all __thread vars (globals are reset at finalize() time) */
		if(__vp_numa_descriptor)
		{
			sctk_free(__vp_numa_descriptor); __vp_numa_descriptor = NULL;
		}
		seed             = 0;
		task_node_number = -1;
	}

	task_node_number = node;
	task             = sctk_malloc(sizeof(_mpc_lowcomm_ib_cp_task_t) );
	memset(task, 0, sizeof(_mpc_lowcomm_ib_cp_task_t) );
	ib_assume(task);
	task->node = node;
	task->vp   = vp;
	task->rank = rank;
	mpc_common_spinlock_init(&task->local_ibufs_list_lock, 0);
	mpc_common_spinlock_init(&task->lock_timers, 0);
	task->ready                  = 0;

	/* Initial allocation of structures */
	mpc_common_spinlock_lock(&__numas_lock);
	if(!numas)
	{
		numa_number = mpc_topology_get_numa_node_count();

		if(numa_number == 0)
		{
			numa_number = 1;
		}

		numas = sctk_calloc(numa_number, sizeof(__mpc_lowcomm_ib_numa_t) );
		ib_assume(numas);

		int vp_number = mpc_topology_get_pu_count();
		vps = sctk_malloc(sizeof(__mpc_lowcomm_ib_vp_t *) * vp_number);
		ib_assume(vps);
		memset(vps, 0, sizeof(__mpc_lowcomm_ib_vp_t *) * vp_number);
		mpc_common_nodebug("vp: %d - numa: %d", mpc_topology_get_pu_count(), numa_number);
	}

	/* Add NUMA node if not already added */
	if(numas[node] == NULL)
	{
		__mpc_lowcomm_ib_numa_t *tmp_numa = sctk_malloc(sizeof(__mpc_lowcomm_ib_numa_t) );
		memset(tmp_numa, 0, sizeof(__mpc_lowcomm_ib_numa_t) );
		__number_of_registered_numa += 1;
		tmp_numa->number             = node;
		numas[node] = tmp_numa;
	}
	mpc_common_spinlock_unlock(&__numas_lock);

	/* Add the VP to the CDL list of the correct NUMA node if it not already
	 * done */
	if(vps[vp] == NULL)
	{
		__mpc_lowcomm_ib_vp_t *tmp_vp = sctk_malloc(sizeof(__mpc_lowcomm_ib_vp_t) );
		memset(tmp_vp, 0, sizeof(__mpc_lowcomm_ib_vp_t) );
		tmp_vp->node = (__mpc_lowcomm_ib_numa_t *)numas[node];
		CDL_PREPEND(numas[node]->vps, tmp_vp);
		tmp_vp->number = vp;
		vps[vp]        = tmp_vp;
		numas[node]->tasks_nb++;
	}

	HASH_ADD(hh_vp, vps[vp]->tasks, rank, sizeof(int), task);
	HASH_ADD(hh_all, all_tasks, rank, sizeof(int), task);
	CDL_PREPEND(numas[node]->tasks, task);
	mpc_common_spinlock_unlock(&__vps_lock);
	mpc_common_nodebug("Task %d registered on VP %d (numa:%d:tasks_nb:%d)", rank,
	              vp, node, numas[node]->tasks_nb);

	/* Initialize seed for steal */
	seed = getpid() * time(NULL);

	task->ready   = 1;
	__vp_tls_save = vps[vp];
}

void _mpc_lowcomm_ib_cp_ctx_finalize_task(int rank)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;
	int vp, node;

	mpc_common_spinlock_lock(&__vps_lock);
	HASH_FIND(hh_all, all_tasks, &rank, sizeof(int), task);
	if(task)
	{
		vp   = task->vp;
		node = mpc_topology_get_numa_node_from_cpu(vp);

		CDL_DELETE(numas[node]->tasks, task);
		HASH_DELETE(hh_vp, vps[vp]->tasks, task);
		HASH_DELETE(hh_all, all_tasks, task);
		task->ready = 0;
		sctk_free(task);
	}
	mpc_common_spinlock_unlock(&__vps_lock);

	seed = 0;
	if(__vp_numa_descriptor)
	{
		sctk_free(__vp_numa_descriptor);
		__vp_numa_descriptor = NULL;
	}
	task_node_number = -1;
}



static inline void __poll_ibuf(struct _mpc_lowcomm_ib_polling_s *poll, _mpc_lowcomm_ib_ibuf_t *ibuf, int no_steal)
{
		/* Run the polling function according to the type of message */
		if(ibuf->cq == MPC_LOWCOMM_IB_RECV_CQ)
		{
			//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_recv_polled,
			//            (mpc_arch_get_timestamp() - ibuf->polled_timestamp));
			sctk_network_poll_recv_ibuf( ( sctk_rail_info_t * )ibuf->rail, ibuf);
		}
		else
		{
			//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_send_polled,
			//            (mpc_arch_get_timestamp() - ibuf->polled_timestamp));
			sctk_network_poll_send_ibuf( ( sctk_rail_info_t * )ibuf->rail, ibuf);
		}

		if( no_steal )
		{
			POLL_RECV_OWN(poll);
		}
		else
		{
			POLL_RECV_OTHER(poll);
		}
}



#define POLL_LIST(poll, list, lock, no_steal, counter) do {\
								if ( list != NULL )\
								{\
									if(mpc_common_spinlock_trylock(&lock) == 0) \
									{\
										if(list != NULL)\
										{\
											_mpc_lowcomm_ib_ibuf_t *ibuf = (_mpc_lowcomm_ib_ibuf_t *)list;\
											DL_DELETE(list, ibuf);\
											mpc_common_spinlock_unlock(&lock);\
											_mpc_lowcomm_ib_cp_ctx_decr_nb_pending_msg();\
											__poll_ibuf(poll, ibuf, no_steal);\
											counter++;\
										}\
                                        else\
                                        {\
											mpc_common_spinlock_unlock(&lock);\
                                        }\
									}\
								}\
								else\
								{\
									break;\
								}\
							}while(no_steal)


int _mpc_lowcomm_ib_cp_ctx_poll_global_list(struct _mpc_lowcomm_ib_polling_s *poll)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;

	if(__vp_tls_save == NULL)
	{
		return 0;
	}

	task = __vp_tls_save->tasks;

	if(!task)
	{
		return 0;
	}

	ib_assume(task);

	int counter=0;

	POLL_LIST(poll, __global_ibufs_list, __global_ibufs_list_lock, 1, counter);

	return counter;
}

int _mpc_lowcomm_ib_cp_ctx_poll(struct _mpc_lowcomm_ib_polling_s *poll, int task_id)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;

	int vp = -1;

	if(__vp_tls_save == NULL)
	{
		vp = mpc_topology_get_pu();

		if(vp < 0 || vps == NULL)
		{
			return 0;
		}

		if(vps[vp] == NULL)
		{
			mpc_common_nodebug("Are we in hybrid mode? try to find dest task %d", task_id);
			HASH_FIND(hh_all, all_tasks, &task_id, sizeof(int), task);
			assume(task);

			vp = task->vp;
			assume(vp >= 0);

			if(vps[vp] == NULL)
			{
				return 0;
			}
		}

		if(0 <= task_id)
		{
			mpc_common_spinlock_lock(&__vps_lock);
			__vp_tls_save = vps[vp];
			mpc_common_spinlock_unlock(&__vps_lock);
		}
	}

	int nb_found = 0;

	for(task = __vp_tls_save->tasks; task; task = task->hh_vp.next)
	{
		POLL_LIST(poll, task->local_ibufs_list, task->local_ibufs_list_lock, 1, nb_found);
	}

	/* If nothing on our NUMA poll for others */
	if(nb_found == 0)
	{
		_mpc_lowcomm_ib_cp_ctx_steal(poll, 1);
	}

	return 0;
}

int _mpc_lowcomm_ib_cp_ctx_steal(struct _mpc_lowcomm_ib_polling_s *poll, char other_numa)
{
	int nb_found = 0;

	if( !numas)
	{
		return 0;
	}

	if(__vp_numa_descriptor == NULL)
	{
		mpc_common_spinlock_lock(&__numas_lock);

		assume(numa_number != -1);

		if(__vp_numa_descriptor == NULL)
		{
			__vp_numa_descriptor = sctk_malloc(sizeof(__mpc_lowcomm_ib_numa_t) * numa_number);
			memcpy(__vp_numa_descriptor, (void *)numas, sizeof(__mpc_lowcomm_ib_numa_t) * numa_number);
		}

		mpc_common_spinlock_unlock(&__numas_lock);
	}

	_mpc_lowcomm_ib_cp_task_t *task = NULL;
	__mpc_lowcomm_ib_numa_t *  numa;
	__mpc_lowcomm_ib_vp_t *    tmp_vp;

	/* In PTHREAD mode, the idle task do not call the cp_init_task
	* function.
	* If task_node_number not initialized, we do it now */
	if(task_node_number < 0)
	{
		task_node_number = mpc_topology_get_numa_node_from_cpu(__vp_tls_save->number);
	}

	/* First, try to steal from the same NUMA node*/
	assume( (task_node_number < numa_number) );
	if(task_node_number >= 0)
	{
		numa = __vp_numa_descriptor[task_node_number];

		if(numa != NULL)
		{
			CDL_FOREACH(numa->vps, tmp_vp)
			{
				for(task = tmp_vp->tasks; task; task = task->hh_vp.next)
				{
					POLL_LIST(poll, task->local_ibufs_list, task->local_ibufs_list_lock, 0, nb_found);
				}
			}
		}
		else
		{
			mpc_common_spinlock_lock(&__numas_lock);
			memcpy(__vp_numa_descriptor, (void *)numas, sizeof(__mpc_lowcomm_ib_numa_t) * numa_number);
			mpc_common_spinlock_unlock(&__numas_lock);
		}
	}

	if(nb_found)
	{
		return nb_found;
	}

	if(other_numa && __vp_numa_descriptor && (__number_of_registered_numa > 1) )
	{
		int random = rand_r(&seed) % __number_of_registered_numa;
   
        if(random == task_node_number)
        {
            random = (random + 1 ) % __number_of_registered_numa;
        }

		numa = __vp_numa_descriptor[random];

		if(numa != NULL)
		{
			CDL_FOREACH(numa->vps, tmp_vp)
			{
				for(task = tmp_vp->tasks; task; task = task->hh_vp.next)
				{
					POLL_LIST(poll, task->local_ibufs_list, task->local_ibufs_list_lock, 0, nb_found);

					if(nb_found)
					{
						return nb_found;
					}
				}
			}
		}
	}

	return nb_found;
}

#define CHECK_IS_READY(x)        do {             \
		if(task->ready != 1){ return 0; } \
} while(0);


int _mpc_lowcomm_ib_cp_ctx_handle_message(_mpc_lowcomm_ib_ibuf_t *ibuf, int dest_task, int target_task)
{
	_mpc_lowcomm_ib_cp_task_t *task = NULL;

	/* XXX: Do not support thread migration */
	HASH_FIND(hh_all, all_tasks, &target_task, sizeof(int), task);

	if(!task)
	{
		//_mpc_lowcomm_ib_rdma_t *rdma_header;
		//rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
		//    mpc_common_debug_error("Indirect message!! (target task:%d dest_task:%d) %d process %d", target_task, dest_task, type, mpc_common_get_process_rank());
		/* We return without error -> indirect message that we need to handle */
		return 0;
	}

	ib_assume(task);
	ib_assume(task->rank == target_task);

	/* Process specific message */
	if(dest_task < 0)
	{
		mpc_common_nodebug("Received global msg");
		mpc_common_spinlock_lock(&__global_ibufs_list_lock);
		DL_APPEND(__global_ibufs_list, ibuf);
		mpc_common_spinlock_unlock(&__global_ibufs_list_lock);
		_mpc_lowcomm_ib_cp_ctx_incr_nb_pending_msg();

		return 1;
	}
	else
	{
		mpc_common_nodebug("Received local msg from task %d", target_task);
		mpc_common_spinlock_lock(&task->local_ibufs_list_lock);
		DL_APPEND(task->local_ibufs_list, ibuf);
		mpc_common_spinlock_unlock(&task->local_ibufs_list_lock);
		_mpc_lowcomm_ib_cp_ctx_incr_nb_pending_msg();
		mpc_common_nodebug("Add message to task %d (%d)", dest_task, target_task);
		return 1;
	}
}

/** Collaborative Polling Init and release */

void _mpc_lowcomm_ib_notify_idle_wait_send()
{
	int i;

	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->runtime_config_driver_config->driver.type != SCTK_RTCFG_net_driver_infiniband)
		{
			continue;
		}

		rail->notify_idle_message(rail);
	}
}

void sctk_network_initialize_task_collaborative_ib(sctk_rail_info_t *rail, int rank, int vp)
{
	if(mpc_common_get_process_count() > 1 && sctk_network_is_ib_used() )
	{
		/* Register task for collaborative polling */
		_mpc_lowcomm_ib_cp_ctx_init_task(rank, vp);
	}

	/* It can happen for topological rails */
	if(!rail->runtime_config_driver_config)
	{
		return;
	}

	/* Skip topological rails */
	if(0 < rail->subrail_count)
	{
		return;
	}

	/* Register task for topology infos */
	_mpc_lowcomm_ib_topology_init_task(rail, vp);

	vps_reset[vp] = 1;
}

void sctk_network_finalize_task_collaborative_ib(sctk_rail_info_t *rail, int taskid, int vp)
{
	/* It can happen for topological rails */
	if(!rail->runtime_config_driver_config)
	{
		return;
	}

	/* Skip topological rails */
	if(0 < rail->subrail_count)
	{
		return;
	}
	/* Register task for topology infos */
	_mpc_lowcomm_ib_topology_free_task(rail);

	if(mpc_common_get_process_count() > 1 && sctk_network_is_ib_used() )
	{
		_mpc_lowcomm_ib_cp_ctx_finalize_task(taskid);
	}

	vps_reset[vp] = 0;
}
