/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
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
#include "sctk_ib_sr.h"
#include "sctk_ib_prof.h"
#include "sctk_asm.h"
#include "utlist.h"
#include "sctk_ib_fallback.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CP"
#include "sctk_ib_toolkit.h"
#include "math.h"

//#define DEBUG_CP
#ifdef DEBUG_CP
#warning "DEBUG MODE ACTIVATED"
#endif

typedef struct sctk_ib_cp_s{
  /* Void for now */
} sctk_ib_cp_t;

struct numa_s;

typedef struct vp_s{
  /* Number */
  int number;
  sctk_ib_cp_task_t *tasks;

  /* Pointer to the numa node where the VP is located */
  struct numa_s *node;
  /* For CDL list */
  struct vp_s *prev;
  struct vp_s *next;
  /* If the VP has already been added */
  char  added;
} vp_t;

typedef struct numa_s{
  /* Number */
  int number;
  /* Circular list of VPs */
  struct vp_s *vps;
  /* Number of VPs on the numa node*/
  int     vp_nb;
  /* For CDL list */
  struct numa_s *prev;
  struct numa_s *next;
  /* If the NODE has already been added */
  char  added;
  int tasks_nb;
  /* Padding for avoiding false sharing */
  char pad[64];
  /* CDL of tasks for a NUMA node */
  sctk_ib_cp_task_t *tasks;
} numa_t;

/* CDL list of numa nodes */
static numa_t *numa_list = NULL;
/* Array of numa nodes */
static numa_t *numas = NULL;
static int numa_number = -1;
int numa_registered = 0;
/* Array of vps */
static vp_t   *vps = NULL;
static int vp_number = -1;
static sctk_spinlock_t vps_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_ib_cp_task_t* all_tasks = NULL;
__thread unsigned int seed;
/* NUMA node where the task is located */
__thread int task_node_number = -1;
static const int ibv_cp_profiler = 1;

/* Counters */
__thread double time_poll_cq = 0;
__thread double time_steals = 0;
__thread double time_own = 0;
__thread double time_ptp = 0;
__thread double time_coll = 0;
__thread long poll_steals = 0;
__thread long poll_steals_failed = 0;
__thread long poll_steals_success = 0;
__thread long poll_steal_same_node = 0;
__thread long poll_steal_other_node = 0;
__thread long poll_own = 0;
__thread long poll_own_failed = 0;
__thread long poll_own_success = 0;
__thread long call_to_polling = 0;
__thread long poll_cq = 0;

#define CHECK_AND_QUIT(rail_ib) do {  \
  LOAD_CONFIG(rail_ib);                             \
  int steal = config->ibv_steal;                    \
  if (steal < 0) return; }while(0)

extern volatile int sctk_online_program;
#define CHECK_ONLINE_PROGRAM do {         \
  if (sctk_online_program != 1) return 0; \
}while(0);

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ib_cp_task_t *sctk_ib_cp_get_task(int rank) {
  sctk_ib_cp_task_t *task = NULL;

  /* XXX: Do not support thread migration */
  HASH_FIND(hh_all,all_tasks,&rank, sizeof(int),task);
#ifdef DEBUG_CP
  assume(task);
#endif

  return task;
}

/* XXX: is called before 'cp_init_task' */
void sctk_ib_cp_init(struct sctk_ib_rail_info_s* rail_ib) {
  CHECK_AND_QUIT(rail_ib);
  sctk_ib_cp_t* cp;

  cp = sctk_malloc(sizeof(sctk_ib_cp_t));
  assume(cp);
  rail_ib->cp = cp;
}

void sctk_ib_cp_init_task(int rank, int vp) {
  sctk_ib_cp_task_t *task = NULL;
  int node =  sctk_get_node_from_cpu(vp);
  task_node_number = node;
  /* Process specific list of messages */
  static sctk_ibuf_t * volatile __global_ibufs_list = NULL;
  static sctk_spinlock_t __global_ibufs_list_lock = SCTK_SPINLOCK_INITIALIZER;

  task = sctk_malloc(sizeof(sctk_ib_cp_task_t));
  assume(task);
  memset(task, 0, sizeof(sctk_ib_cp_task_t));
  task->node =  node;
  task->vp = vp;
  task->rank = rank;
  task->local_ibufs_list_lock = SCTK_SPINLOCK_INITIALIZER;
  task->lock_timers = SCTK_SPINLOCK_INITIALIZER;
  task->ready = 0;
  task->global_ibufs_list = &__global_ibufs_list;
  task->global_ibufs_list_lock = &__global_ibufs_list_lock;

  sctk_spinlock_lock(&vps_lock);
  /* Initial allocation of structures */
  if (!numas) {
    numa_number = sctk_get_numa_node_number();
    if (numa_number == 0) numa_number = 1;
    numas = sctk_malloc(sizeof(numa_t) * numa_number);
    memset(numas, 0, sizeof(numa_t) * numa_number);
    assume(numas);

    vp_number = sctk_get_cpu_number();
    vps = sctk_malloc(sizeof(vp_t) * vp_number);
    assume(vps);
    memset(vps, 0, sizeof(vp_t) * vp_number);
    sctk_nodebug("vp: %d - numa: %d", sctk_get_cpu_number(), numa_number);
  }
  /* Add NUMA node if not already added */
  if (numas[node].added == 0) {
    CDL_PREPEND( numa_list, &(numas[node]));
    numa_registered += 1;
    numas[node].number = node;
    numas[node].added = 1;
  }
  /* Add the VP to the CDL list of the correct NUMA node if it not already done */
  if (vps[vp].added == 0) {
    vps[vp].node = &numas[node];
    CDL_PREPEND( numas[node].vps, &(vps[vp]));
    vps[vp].number = vp;
    vps[vp].added = 1;
    numas[node].tasks_nb++;
  }
  HASH_ADD(hh_vp, vps[vp].tasks,rank,sizeof(int),task);
  HASH_ADD(hh_all, all_tasks,rank,sizeof(int),task);
  CDL_PREPEND(numas[node].tasks, task);
  sctk_spinlock_unlock(&vps_lock);
  sctk_ib_debug("Task %d registered on VP %d (numa:%d:tasks_nb:%d)", rank, vp, node, numas[node].tasks_nb);

  /* Initialize seed for steal */
  seed = getpid() * time(NULL);

  task->ready = 1;
}

void sctk_ib_cp_finalize_task(int rank) {
  sctk_ib_cp_task_t *task = NULL;

  HASH_FIND(hh_all,all_tasks,&rank, sizeof(int),task);
  assume(task);

#if 0
  fprintf(stderr, "%2d %d %d %d\n", rank,
      CP_PROF_PRINT(task, poll_own),
      CP_PROF_PRINT(task, poll_own_failed),
      CP_PROF_PRINT(task, poll_own_lock_failed));
#endif

#if 0
  if (ibv_cp_profiler) {
    if (rank == 0) {
      fprintf(stderr, "#poll_own poll_stolen poll_steals poll_steal_same_node poll_steal_other_node time_stolen time_steals time_own\n");

    }
    fprintf(stderr, "%2d %d %d %d %d %0.5f %0.5f\n", rank,
        CP_PROF_PRINT(task, poll_own),
        CP_PROF_PRINT(task, poll_steals),
        CP_PROF_PRINT(task, poll_steal_same_node),
        CP_PROF_PRINT(task, poll_steal_other_node),
        time_steals/CYCLES_PER_SEC,
        time_own/CYCLES_PER_SEC);
  }
#endif

}

static inline int __cp_poll(const struct sctk_rail_info_s const* rail, struct sctk_ib_polling_s *poll, sctk_ibuf_t * volatile * const list, sctk_spinlock_t *lock, sctk_ib_cp_task_t *task){
  sctk_ibuf_t *ibuf = NULL;
  int nb_found = 0;
  char update_timers=0;
  double s, e;
  double _time_own = 0;
  int _poll_own = 0;

retry:
  if ( *list != NULL) {
    if (sctk_spinlock_trylock(lock) == 0) {
      if ( *list != NULL) {
        ibuf = *list;
        DL_DELETE(*list, ibuf);
        sctk_spinlock_unlock(lock);

        if (ibv_cp_profiler) {
          s = sctk_atomics_get_timestamp();
        }

        /* Run the polling function according to the type of message */
        if (ibuf->cq == recv_cq) {
          sctk_network_poll_recv_ibuf(rail, ibuf, 1, poll);
        } else {
          sctk_network_poll_send_ibuf(rail, ibuf, 1, poll);
        }

        if (ibv_cp_profiler) {
          update_timers=1;
          e = sctk_atomics_get_timestamp();
          _time_own += e - s;
          _poll_own++;
        }
        nb_found++;
        goto retry;
      } else {
        sctk_spinlock_unlock(lock);
      }
    }
  }

  if (update_timers) {
    poll_own_success ++;
    time_own += _time_own;
    poll_own += _poll_own;
#if 0
    CP_PROF_ADD(task, poll_own, _poll_own);
#endif
    poll->recv_found_own = _poll_own;
  } else {
    poll_own_failed ++;
#if 0
    CP_PROF_INC(task, poll_own_failed);
#endif
  }
  return nb_found;
}

int sctk_ib_cp_poll_all(const struct sctk_rail_info_s const* rail, struct sctk_ib_polling_s *poll){
  int vp = sctk_thread_get_vp();
  sctk_ib_cp_task_t *task = NULL;
  sctk_ib_cp_task_t *tmp_task = NULL;
  sctk_ib_cp_task_t *current_task = NULL;
  int nb_found = 0;
  vp_t* tmp_vp;

  CHECK_ONLINE_PROGRAM;

  HASH_ITER(hh_all, all_tasks, current_task, tmp_task) {
    for (task=current_task; task; task=task->hh_vp.next) {
      nb_found += __cp_poll(rail, poll, &(task->local_ibufs_list), &(task->local_ibufs_list_lock), task);
    }
  }

  return nb_found;
}

int sctk_ib_cp_poll_global_list(const struct sctk_rail_info_s const * rail, struct sctk_ib_polling_s *poll){
  sctk_ib_cp_task_t *task = NULL;
  int nb_found = 0;
  int vp = sctk_thread_get_vp();

  CHECK_ONLINE_PROGRAM;

  task = vps[vp].tasks;
  if (!task) return 0;
#ifdef DEBUG_CP
  assume(task);
#endif

  nb_found += __cp_poll(rail, poll, task->global_ibufs_list, task->global_ibufs_list_lock, task);

  return nb_found;
}


int sctk_ib_cp_poll(const struct sctk_rail_info_s const* rail, struct sctk_ib_polling_s *poll){
  int vp = sctk_thread_get_vp();
  sctk_ib_cp_task_t *task = NULL;
  int nb_found = 0;

  CHECK_ONLINE_PROGRAM;

  for (task=vps[vp].tasks; task; task=task->hh_vp.next) {
#ifdef DEBUG_CP
    assume(vp == task->vp);
    assume(vps[vp].node->number == task->node);
#endif
    nb_found += __cp_poll(rail, poll, &(task->local_ibufs_list), &(task->local_ibufs_list_lock), task);
  }

  return nb_found;
}

static inline int __cp_steal(struct sctk_rail_info_s* rail,struct sctk_ib_polling_s *poll, sctk_ibuf_t** volatile list, sctk_spinlock_t *lock, sctk_ib_cp_task_t *task, sctk_ib_cp_task_t* stealing_task) {
  sctk_ibuf_t *ibuf = NULL;
  int nb_found = 0;
  /* For CP profiling */
  double s, e;
  double _time_steals = 0;
  int _poll_steals=0;
  int _poll_steal_same_node=0;
  int _poll_steal_other_node=0;
  char update_timers=0;
  int max_retry = 1;

retry:
  if (*list != NULL) {
    if (sctk_spinlock_trylock(lock) == 0) {
      if ( *list != NULL) {
        ibuf = *list;
        DL_DELETE(*list, ibuf);
        sctk_spinlock_unlock(lock);

        if (ibv_cp_profiler) {
          s = sctk_atomics_get_timestamp();
        }

        /* Run the polling function */
        if (ibuf->cq == recv_cq) {
          sctk_network_poll_recv_ibuf(rail, ibuf, 2, poll);
        } else {
          sctk_network_poll_send_ibuf(rail, ibuf, 2, poll);
        }

        if (ibv_cp_profiler) {
          e = sctk_atomics_get_timestamp();
          update_timers=1;
          /* End of set timers */
          _time_steals += e - s;

          _poll_steals++;
          /* Same node */
          if (task->node == stealing_task->node)
            _poll_steal_same_node++;
          else
            _poll_steal_other_node++;
        }
        nb_found++;
        /* We can retry once again */
        if (nb_found < max_retry)
          goto retry;
        else goto exit;
      }
      sctk_spinlock_unlock(lock);
    }
  }

exit:
  if(update_timers) {
    poll_steals_success ++;
    time_steals += _time_steals;
    poll->recv_found_other = _poll_steals;
    poll_steals += _poll_steals;
    poll_steal_same_node += _poll_steal_same_node;
    poll_steal_other_node += _poll_steal_other_node;
#if 0
    CP_PROF_ADD(stealing_task,poll_steals, _poll_steals);
    CP_PROF_ADD(stealing_task,poll_steal_same_node,_poll_steal_same_node);
    CP_PROF_ADD(stealing_task,poll_steal_other_node,_poll_steal_other_node);
#endif
  } else {
    poll_steals_failed ++;
#if 0
    CP_PROF_INC(stealing_task, poll_steals_failed);
#endif
  }

  return nb_found;
}

#define STEAL_CURRENT_NODE(x) do {  \
    CDL_FOREACH(&vps[vp], tmp_vp) { \
      for (task=tmp_vp->tasks; task; task=task->hh_vp.next) { \
        nb_found += __cp_steal(rail, poll, x, &(task->local_ibufs_list_lock), task, stealing_task);  \
      } \
      /* Return if message stolen */  \
      if (nb_found) return nb_found;  \
    } \
} while (0)

#define STEAL_OTHER_NODE(x) do { \
  int rand = rand_r(&seed) % numa_registered; \
  tmp_numa = &numas[(task_node_number+rand)%numa_registered]; \
   CDL_FOREACH(tmp_numa->vps, tmp_vp) {  \
    for (task=tmp_vp->tasks; task; task=task->hh_vp.next) { \
     nb_found += __cp_steal(rail, poll, x, &(task->local_ibufs_list_lock), task, stealing_task);  \
    } \
  } \
  if (nb_found) return nb_found;  \
} while (0)

#define STEAL_OTHER_NODE_OLD(x) do { \
    CDL_FOREACH(numa_list, tmp_numa) {  \
    if(i >= max_numa_to_steal) return nb_found;  \
      CDL_FOREACH(tmp_numa->vps, tmp_vp) {  \
        for (task=tmp_vp->tasks; task; task=task->hh_vp.next) { \
          nb_found += __cp_steal(rail, poll, x, &(task->local_ibufs_list_lock), task, stealing_task);  \
        } \
      } \
      /* steal a restricted number of NUMA nodes */ \
      ++i; \
      /* Return if message stolen */  \
      if (nb_found) return nb_found;  \
    } \
} while (0)

int sctk_ib_cp_steal( sctk_rail_info_t* rail, struct sctk_ib_polling_s *poll) {
  int nb_found = 0;
  int vp = sctk_thread_get_vp();
  sctk_ib_cp_task_t *stealing_task = NULL;
  sctk_ib_cp_task_t *task = NULL;
//  const int max_numa_to_steal = 1;
  vp_t* tmp_vp;
  numa_t* tmp_numa;
  int i = 0;

  CHECK_ONLINE_PROGRAM;

  /* XXX: Not working with oversubscribing */
  stealing_task = vps[vp].tasks;

  /* First, try to steal from the same NUMA node*/
  STEAL_CURRENT_NODE(&task->local_ibufs_list);
  /* Secondly, try to steal from another NUMA node  */
  STEAL_OTHER_NODE(&task->local_ibufs_list);
  return nb_found;
}

#define CHECK_IS_READY(x)  do {\
  if (task->ready != 1) return 0; \
}while(0);


int sctk_ib_cp_handle_message(sctk_rail_info_t* rail,
    sctk_ibuf_t *ibuf, int dest_task, int target_task, enum sctk_ib_cp_poll_cq_e cq) {
  sctk_ib_cp_task_t *task = NULL;
  int vp = sctk_thread_get_vp();

  CHECK_ONLINE_PROGRAM;

  /* XXX: Do not support thread migration */
  HASH_FIND(hh_all,all_tasks,&target_task, sizeof(int),task);
  if (!task) {
    sctk_nodebug("Indirect message!!");
    /* We return without error -> indirect message that we need to handle */
    return 0;
  }

resume:
#ifdef DEBUG_CP
  assume(task);
  assume(task->rank == target_task);
#endif
  /* Process specific message */
  if (dest_task < 0) {
    goto enqueue_global;
  } else {
    goto enqueue_local;
  }

  /* Add the ibuf to the pending list */
enqueue_local:
  sctk_nodebug("Received local msg from task %d", target_task);
  sctk_spinlock_lock(&task->local_ibufs_list_lock);
  DL_APPEND(task->local_ibufs_list, ibuf);
  sctk_spinlock_unlock(&task->local_ibufs_list_lock);
  return 1;
enqueue_global:
  sctk_nodebug("Received global msg");
  sctk_spinlock_lock(task->global_ibufs_list_lock);
  DL_APPEND( *(task->global_ibufs_list), ibuf);
  sctk_spinlock_unlock(task->global_ibufs_list_lock);
  return 1;
}

#endif
