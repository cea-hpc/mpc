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

#ifdef MPC_USE_INFINIBAND
#include "sctk_ib.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_prof.h"
#include "sctk_asm.h"
#include "utlist.h"
#include "sctk_ib_mpi.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_topology.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CP"
#include "sctk_ib_toolkit.h"
#include "math.h"

#define SCTK_IB_PROFILER

extern sctk_request_t *blocked_request;

typedef struct sctk_ib_cp_s
{
	/* Void for now */
} sctk_ib_cp_t;

struct numa_s;

typedef struct vp_s
{
	int number; 				/**< Number */
	sctk_ib_cp_task_t *tasks;
	struct numa_s *node;		/**< Pointer to the numa node where the VP is located */
	/* For CDL list */
	struct vp_s *prev;
	struct vp_s *next;
	char pad[128];
} vp_t;

typedef struct numa_s
{

	int number;					/**< Number */
	struct vp_s *vps;			/**< Circular list of VPs */
	int     vp_nb;				/**< Number of VPs on the numa node*/
	/* For CDL list */
	struct numa_s *prev;
	struct numa_s *next;
	int tasks_nb;
	sctk_ib_cp_task_t *tasks;	/** CDL of tasks for a NUMA node */
	char pad[128];				/**< Padding avoiding false sharing */
} numa_t;

/** CDL list of numa nodes */
static numa_t *numa_list = NULL;

/** Array of numa nodes */
static sctk_spinlock_t numas_lock = SCTK_SPINLOCK_INITIALIZER;
static volatile numa_t *volatile *numas = NULL;
__thread numa_t **numas_copy = NULL;

static int numa_number = -1;
int numa_registered = 0;

/** Array of vps */
static vp_t   **vps = NULL;
__thread vp_t **loc_vps = NULL;
static int vp_number = -1;
static sctk_spinlock_t vps_lock = SCTK_SPINLOCK_INITIALIZER;

static sctk_ib_cp_task_t *all_tasks = NULL;
__thread unsigned int seed;

/** NUMA node where the task is located */
__thread int task_node_number = -1;

/** Pointer to the NUMA node on VP */
__thread numa_t *numa_on_vp = NULL;

#define CHECK_AND_QUIT(rail_ib) do {  \
  LOAD_CONFIG(rail_ib);                             \
  int steal = config->steal;                    \
  if (steal < 0) return; }while(0)

extern volatile int sctk_online_program;
#define CHECK_ONLINE_PROGRAM do {         \
  if (sctk_online_program != 1) return 0; \
}while(0);

OPA_int_t cp_nb_pending_msg;

void sctk_ib_cp_incr_nb_pending_msg()
{
	OPA_incr_int ( &cp_nb_pending_msg );
}

void sctk_ib_cp_decr_nb_pending_msg()
{
	OPA_decr_int ( &cp_nb_pending_msg );
}

int sctk_ib_cp_get_nb_pending_msg()
{
	return OPA_load_int ( &cp_nb_pending_msg );
}

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
#if 0
sctk_ib_cp_task_t *sctk_ib_cp_get_task ( int rank )
{
	sctk_ib_cp_task_t *task = NULL;

	/* XXX: Do not support thread migration */
	HASH_FIND ( hh_all, all_tasks, &rank, sizeof ( int ), task );
	ib_assume ( task );

	return task;
}
#endif

/* XXX: is called before 'cp_init_task' */
void sctk_ib_cp_init ( struct sctk_ib_rail_info_s *rail_ib )
{
	CHECK_AND_QUIT ( rail_ib );
	sctk_ib_cp_t *cp;

	cp = sctk_malloc ( sizeof ( sctk_ib_cp_t ) );
	ib_assume ( cp );
	rail_ib->cp = cp;
	OPA_store_int ( &cp_nb_pending_msg, 0 );
}

void sctk_ib_cp_init_task ( int rank, int vp )
{
	sctk_ib_cp_task_t *task = NULL;
	int node =  sctk_get_node_from_cpu( vp );
	task_node_number = node;
	/* Process specific list of messages */
	static sctk_ibuf_t *volatile __global_ibufs_list = NULL;
	static sctk_spinlock_t __global_ibufs_list_lock = SCTK_SPINLOCK_INITIALIZER;

	sctk_spinlock_lock ( &vps_lock );
	/* If the task has already been added, we do not add it once again and skip */
	HASH_FIND ( hh_all, all_tasks, &rank, sizeof ( int ), task );

	if ( task )
	{
		sctk_spinlock_unlock ( &vps_lock );
		return;
	}


	task = sctk_malloc ( sizeof ( sctk_ib_cp_task_t ) );
	memset ( task, 0, sizeof ( sctk_ib_cp_task_t ) );
	ib_assume ( task );
	task->node =  node;
	task->vp = vp;
	task->rank = rank;
	task->local_ibufs_list_lock = SCTK_SPINLOCK_INITIALIZER;
	task->lock_timers = SCTK_SPINLOCK_INITIALIZER;
	task->ready = 0;
	task->global_ibufs_list = &__global_ibufs_list;
	task->global_ibufs_list_lock = &__global_ibufs_list_lock;

	/* Initial allocation of structures */
        sctk_spinlock_lock(&numas_lock);
        if (!numas) {
          numa_number = sctk_get_numa_node_number();

          if (numa_number == 0)
            numa_number = 1;

          numas = sctk_malloc(sizeof(numa_t) * numa_number);

          int i;

          for (i = 0; i < numa_number; ++i) {
            numas[i] = NULL;
          }

          ib_assume(numas);

          vp_number = sctk_get_cpu_number();
          vps = sctk_malloc(sizeof(vp_t *) * vp_number);
          ib_assume(vps);
          memset(vps, 0, sizeof(vp_t *) * vp_number);
          sctk_nodebug("vp: %d - numa: %d", sctk_get_cpu_number(), numa_number);
        }

        /* Add NUMA node if not already added */
        if (numas[node] == NULL) {
          numa_t *tmp_numa = sctk_malloc(sizeof(numa_t));
          memset(tmp_numa, 0, sizeof(numa_t));
          CDL_PREPEND(numa_list, tmp_numa);
          numa_registered += 1;
          tmp_numa->number = node;
          numas[node] = tmp_numa;
        }
        sctk_spinlock_unlock(&numas_lock);

        /* Add the VP to the CDL list of the correct NUMA node if it not already
         * done */
        if (vps[vp] == NULL) {
          vp_t *tmp_vp = sctk_malloc(sizeof(vp_t));
          memset(tmp_vp, 0, sizeof(vp_t));
          tmp_vp->node = numas[node];
          numa_on_vp = numas[node];
          CDL_PREPEND(numas[node]->vps, tmp_vp);
          tmp_vp->number = vp;
          vps[vp] = tmp_vp;
          numas[node]->tasks_nb++;
        }

        HASH_ADD(hh_vp, vps[vp]->tasks, rank, sizeof(int), task);
        HASH_ADD(hh_all, all_tasks, rank, sizeof(int), task);
        CDL_PREPEND(numas[node]->tasks, task);
        sctk_spinlock_unlock(&vps_lock);
        sctk_ib_debug("Task %d registered on VP %d (numa:%d:tasks_nb:%d)", rank,
                      vp, node, numas[node]->tasks_nb);

        /* Initialize seed for steal */
        seed = getpid() * time(NULL);

        task->ready = 1;
}

void sctk_ib_cp_finalize_task ( int rank )
{
	sctk_ib_cp_task_t *task = NULL;

	HASH_FIND ( hh_all, all_tasks, &rank, sizeof ( int ), task );
	ib_assume ( task );
}

static inline int __cp_poll ( const sctk_rail_info_t const *rail, struct sctk_ib_polling_s *poll, sctk_ibuf_t *volatile *list, sctk_spinlock_t *lock, sctk_ib_cp_task_t *task, char from_global )
{
	sctk_ibuf_t *ibuf = NULL;
	int nb_found = 0;

retry:

	if ( *list != NULL )
	{
		if ( sctk_spinlock_trylock ( lock ) == 0 )
		{
			if ( *list != NULL )
			{
				ibuf = *list;
				DL_DELETE ( *list, ibuf );
				sctk_spinlock_unlock ( lock );
				sctk_ib_cp_decr_nb_pending_msg();

#ifdef SCTK_IB_PROFILER
				PROF_TIME_START ( rail, cp_time_own );
#endif

				/* Run the polling function according to the type of message */
				if ( ibuf->cq == SCTK_IB_RECV_CQ )
				{
					//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_recv_polled,
					//            (sctk_ib_prof_get_time_stamp() - ibuf->polled_timestamp));
					sctk_network_poll_recv_ibuf ( ( sctk_rail_info_t * ) ibuf->rail, ibuf, 1, poll );
				}
				else
				{
					//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_send_polled,
					//            (sctk_ib_prof_get_time_stamp() - ibuf->polled_timestamp));
					sctk_network_poll_send_ibuf ( ( sctk_rail_info_t * ) ibuf->rail, ibuf, 1, poll );
				}

				POLL_RECV_OWN ( poll );

#ifdef SCTK_IB_PROFILER
				PROF_TIME_END ( rail, cp_time_own );
				PROF_INC ( rail, cp_counter_own );
#endif
				nb_found++;
				goto retry;
			}
			else
			{
				sctk_spinlock_unlock ( lock );
			}
		}
	}

	return nb_found;
}

int sctk_ib_cp_poll_global_list ( const struct sctk_rail_info_s const *rail, struct sctk_ib_polling_s *poll )
{
	sctk_ib_cp_task_t *task = NULL;
	int vp = sctk_thread_get_vp();

	if ( vp < 0 )
		return 0;

	CHECK_ONLINE_PROGRAM;

	if ( vps[vp] == NULL )
		return 0;

	task = vps[vp]->tasks;

	if ( !task )
		return 0;

	ib_assume ( task );

	return __cp_poll ( rail, poll, task->global_ibufs_list, task->global_ibufs_list_lock, task, 1 );
}

__thread vp_t *tls_vp = NULL;

int sctk_ib_cp_poll ( struct sctk_rail_info_s *rail, struct sctk_ib_polling_s *poll, int task_id )
{
	sctk_ib_cp_task_t *task = NULL;
#ifdef HAVE_PROGRESS_THREAD
	if ( task_id < 0 || 1)
#else
	if ( task_id < 0 )
#endif
	{
		sctk_ib_cp_steal ( rail, poll, 1 );
		return 0;
	}

	if ( tls_vp == NULL )
	{
		sctk_ib_cp_task_t *task = NULL;
		sctk_nodebug ( "Try to find task %d", task_id );
		HASH_FIND ( hh_all, all_tasks, &task_id, sizeof ( int ), task );
		assume ( task );

		int vp_num = task->vp;
		assume ( vp_num >= 0 );

		CHECK_ONLINE_PROGRAM;

		tls_vp = vps[vp_num];
	}

	for ( task = tls_vp->tasks; task; task = task->hh_vp.next )
	{
		__cp_poll ( rail, poll, & ( task->local_ibufs_list ), & ( task->local_ibufs_list_lock ), task, 0 );
	}

	return 0;
}

static inline int __cp_steal ( const struct sctk_rail_info_s const *rail, struct sctk_ib_polling_s *poll, sctk_ibuf_t *volatile *list, sctk_spinlock_t *lock, sctk_ib_cp_task_t *task, sctk_ib_cp_task_t *stealing_task )
{
	sctk_ibuf_t *ibuf = NULL;
	int nb_found = 0;

	if ( *list != NULL )
	{
		if ( sctk_spinlock_trylock ( lock ) == 0 )
		{
			if ( *list != NULL )
			{
				ibuf = *list;
				DL_DELETE ( *list, ibuf );
				sctk_spinlock_unlock ( lock );
				sctk_ib_cp_decr_nb_pending_msg();

#ifdef SCTK_IB_PROFILER
				PROF_TIME_START ( rail, cp_time_steal );
#endif

				/* Run the polling function */
				if ( ibuf->cq == SCTK_IB_RECV_CQ )
				{
					/* Profile the time to handle an ibuf once it has been polled
					* from the CQ */
					//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_recv_polled,
					//            (sctk_ib_prof_get_time_stamp() - ibuf->polled_timestamp));
					sctk_network_poll_recv_ibuf ( ibuf->rail, ibuf, 2, poll );
				}
				else
				{
					/* Profile the time to handle an ibuf once it has been polled
					* from the CQ */
					//          SCTK_PROFIL_END_WITH_VALUE(ib_ibuf_send_polled,
					//            (sctk_ib_prof_get_time_stamp() - ibuf->polled_timestamp));
					sctk_network_poll_send_ibuf ( ibuf->rail, ibuf, 2, poll );
				}

				POLL_RECV_OTHER ( poll );

#ifdef SCTK_IB_PROFILER
				PROF_TIME_END ( rail, cp_time_steal );

				/* Same node */
                                if (stealing_task == NULL) {
                                  PROF_INC(rail, cp_counter_steal_other_node);
                                } else {
                                  if (task->node == stealing_task->node) {
                                    PROF_INC(rail, cp_counter_steal_same_node);
                                  } else {
                                    PROF_INC(rail, cp_counter_steal_other_node);
                                  }
                                }

#endif
				nb_found++;
				goto exit;
			}

			sctk_spinlock_unlock ( lock );
		}
	}

exit:
	return nb_found;
}

int sctk_ib_cp_steal ( const sctk_rail_info_t const *rail, struct sctk_ib_polling_s *poll, char other_numa )
{
	int nb_found = 0;
	int vp = sctk_thread_get_vp();
	sctk_ib_cp_task_t *stealing_task = NULL;
	sctk_ib_cp_task_t *task = NULL;
	vp_t *tmp_vp;
	numa_t *numa;

	if ( vp < 0 )
		return 0;

	CHECK_ONLINE_PROGRAM;

	if ( numas_copy == NULL )
	{
          sctk_spinlock_lock(&numas_lock);

          assume(numa_number != -1);

          if (numas_copy == NULL) {
            numas_copy = malloc(sizeof(numa_t) * numa_number);
            memcpy(numas_copy, numas, sizeof(numa_t) * numa_number);
          }

          sctk_spinlock_unlock(&numas_lock);
        }

        if (vps[vp] != NULL) {
          /* XXX: Not working with oversubscribing */
          stealing_task = vps[vp]->tasks;

          /* In PTHREAD mode, the idle task do not call the cp_init_task
          * function.
          * If task_node_number not initialized, we do it now */
          if (task_node_number < 0)
            task_node_number = sctk_get_node_from_cpu(vp);

          /* First, try to steal from the same NUMA node*/
          assume((task_node_number < numa_number));
          if (task_node_number >= 0) {
            numa = numas_copy[task_node_number];

            if (numa != NULL) {
              CDL_FOREACH(numa->vps, tmp_vp) {
                for (task = tmp_vp->tasks; task; task = task->hh_vp.next) {
                  nb_found += __cp_steal(rail, poll, &(task->local_ibufs_list),
                                         &(task->local_ibufs_list_lock), task,
                                         stealing_task);
                }

                /* Return if message stolen */
                //        if (nb_found) return nb_found;
              }
            } else {
              sctk_spinlock_lock(&numas_lock);
              memcpy(numas_copy, numas, sizeof(numa_t) * numa_number);
              sctk_spinlock_unlock(&numas_lock);
            }
          }
        }

        if (nb_found)
          return nb_found;

        if (other_numa && numas_copy) {
          int random = rand_r(&seed) % numa_registered;

          numa = numas_copy[random];
          if (numa != NULL) {
            CDL_FOREACH(numa->vps, tmp_vp) {
              for (task = tmp_vp->tasks; task; task = task->hh_vp.next) {
                nb_found += __cp_steal(rail, poll, &(task->local_ibufs_list),
                                       &(task->local_ibufs_list_lock), task,
                                       stealing_task);

                if (nb_found)
                  return nb_found;
              }
            }
          }
        }

        return nb_found;
}

#define CHECK_IS_READY(x)  do {\
  if (task->ready != 1) return 0; \
}while(0);
#define IBUF_GET_RDMA_TYPE(x) ((x)->type)

int sctk_ib_cp_handle_message ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, int dest_task, int target_task )
{
	sctk_ib_cp_task_t *task = NULL;

	CHECK_ONLINE_PROGRAM;

	/* XXX: Do not support thread migration */
	HASH_FIND ( hh_all, all_tasks, &target_task, sizeof ( int ), task );

	if ( !task )
	{
		sctk_ib_rdma_t *rdma_header;
		rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
		int type = IBUF_GET_RDMA_TYPE ( rdma_header );
		//    sctk_error("Indirect message!! (target task:%d dest_task:%d) %d process %d", target_task, dest_task, type, sctk_process_rank);
		/* We return without error -> indirect message that we need to handle */
		return 0;
	}

	ib_assume ( task );
	ib_assume ( task->rank == target_task );

	/* Process specific message */
	if ( dest_task < 0 )
	{
		sctk_nodebug ( "Received global msg" );
		sctk_spinlock_lock ( task->global_ibufs_list_lock );
		DL_APPEND ( * ( task->global_ibufs_list ), ibuf );
		sctk_spinlock_unlock ( task->global_ibufs_list_lock );
		sctk_ib_cp_incr_nb_pending_msg();

		return 1;
	}
	else
	{
		sctk_nodebug ( "Received local msg from task %d", target_task );
		sctk_spinlock_lock ( &task->local_ibufs_list_lock );
		DL_APPEND ( task->local_ibufs_list, ibuf );
		sctk_spinlock_unlock ( &task->local_ibufs_list_lock );
		sctk_ib_cp_incr_nb_pending_msg();
		sctk_nodebug ( "Add message to task %d (%d)", dest_task, target_task );
		return 1;
	}
}


/** Collaborative Polling Init and release */

void sctk_network_notify_idle_message_multirail_ib_wait_send ()
{
	int i;

	for ( i = 0; i < sctk_rail_count(); i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->runtime_config_driver_config->driver.type != SCTK_RTCFG_net_driver_infiniband )
			continue;
		
		rail->notify_idle_message ( rail );
	}
}



void sctk_network_initialize_task_collaborative_ib ( int rank, int vp )
{
	if ( sctk_process_number > 1 && sctk_network_is_ib_used() )
	{
		/* Register task for collaborative polling */
		sctk_ib_cp_init_task ( rank, vp );
	}

	int i;

	for ( i = 0; i < sctk_rail_count(); i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		/* It can happen for topological rails */
		if( ! rail->runtime_config_driver_config )
			continue;
		
		/* Skip topological rails */
		if( 0 < rail->subrail_count )
			continue;
		
		if( rail->runtime_config_driver_config->driver.type != SCTK_RTCFG_net_driver_infiniband )
			continue;
		
		/* Register task for topology infos */
		sctk_ib_topology_init_task ( rail, vp );
	}
}


void sctk_network_finalize_task_collaborative_ib ( int rank )
{
	if ( sctk_process_number > 1 && sctk_network_is_ib_used() )
	{
		sctk_ib_cp_finalize_task ( rank );
	}
}




#endif
