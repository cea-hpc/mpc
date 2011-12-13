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

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CP"
#include "sctk_ib_toolkit.h"
#include "math.h"

typedef struct sctk_ib_cp_s{
  /* Void for now */
} sctk_ib_cp_t;

struct numa_s;

typedef struct vp_s{
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
  /* Circular list of VPs */
  struct vp_s *vps;
  /* Number of VPs on the numa node*/
  int     vp_nb;
  /* For CDL list */
  struct numa_s *prev;
  struct numa_s *next;
  /* If the NODE has already been added */
  char  added;
} numa_t;

/* CDL list of numa nodes */
static numa_t *numa_list = NULL;
/* Array of numa nodes */
static numa_t *numas = NULL;
static int numa_number = -1;
/* Array of vps */
static vp_t   *vps = NULL;
static int vp_number = -1;
static sctk_spinlock_t vps_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_ib_cp_task_t* all_tasks = NULL;
__thread unsigned int seed;

#define CHECK_AND_QUIT(rail_ib) do {  \
  LOAD_CONFIG(rail_ib);                             \
  int steal = config->ibv_steal;                    \
  if (steal < 0) return; }while(0)


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ib_cp_task_t *sctk_ib_cp_get_task(int rank) {
  sctk_ib_cp_task_t *task = NULL;

  /* XXX: Do not support thread migration */
  HASH_FIND(hh_all,all_tasks,&rank, sizeof(int),task);
  assume(task);

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

  task = sctk_malloc(sizeof(sctk_ib_cp_task_t));
  assume(task);
  memset(task, 0, sizeof(sctk_ib_cp_task_t));
  task->node =  node;
  task->vp = vp;
  task->rank = rank;
  task->lock = SCTK_SPINLOCK_INITIALIZER;
  task->lock_timers = SCTK_SPINLOCK_INITIALIZER;

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
    memset(vps, 0, sizeof(vp_t) * vp_number);
    assume(vps);
    sctk_nodebug("vp: %d - numa: %d", sctk_get_cpu_number(), numa_number);
  }
  /* Add the VP to the CDL list of the correct NUMA node if it not already done */
  if (vps[vp].added == 0) {
    vps[vp].node = &numas[node];
    CDL_PREPEND( numas[node].vps, &(vps[vp]));
    vps[vp].added = 1;
  }
  if (numas[node].added == 0) {
    CDL_PREPEND( numa_list, &(numas[node]));
    numas[node].added = 1;
  }
  HASH_ADD(hh_vp, vps[vp].tasks,rank,sizeof(int),task);
  HASH_ADD(hh_all, all_tasks,rank,sizeof(int),task);
  sctk_spinlock_unlock(&vps_lock);
  sctk_ib_debug("Task %d registered on VP %d (numa:%d)", rank, vp, node);

  /* Initialize seed for steal */
  seed = getpid() * time(NULL);
}

void sctk_ib_cp_finalize_task(int rank) {
  sctk_ib_cp_task_t *task = NULL;

  HASH_FIND(hh_all,all_tasks,&rank, sizeof(int),task);
  assume(task);

//  fprintf(stderr, "[%2d] matched:%d not_matched:%d poll_own:%d poll_stolen:%d poll_steals:%d\n", rank,
//      CP_PROF_PRINT(task, matched),
//      CP_PROF_PRINT(task, not_matched),
//      CP_PROF_PRINT(task, poll_own),
//      CP_PROF_PRINT(task, poll_stolen),
//      CP_PROF_PRINT(task, poll_steals)
//      );
}

int __cp_poll(struct sctk_rail_info_s* rail,struct sctk_ib_polling_s *poll, sctk_ibuf_t** volatile list, sctk_ib_cp_task_t *task,
    int (*func) (struct sctk_rail_info_s *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll)){
  sctk_ibuf_t *ibuf = NULL;
  int nb_found = 0;

retry:
  if ( *list != NULL) {
    if (sctk_spinlock_trylock(&task->lock) == 0) {
      if ( *list != NULL) {
        ibuf = *list;
        sctk_nodebug("Found ibuf %p", ibuf);
        DL_DELETE(*list, ibuf);
        sctk_spinlock_unlock(&task->lock);
        /* Run the polling function */
        func(rail, ibuf, 1, poll);

        CP_PROF_INC(task,poll_own);
        nb_found++;
        poll->recv_found_own++;
        goto retry;
      }
      sctk_spinlock_unlock(&task->lock);
    }
  }
  return nb_found;
}


int sctk_ib_cp_poll(struct sctk_rail_info_s* rail,struct sctk_ib_polling_s *poll, enum sctk_ib_cp_poll_cq_e cq,
    int (*func) (struct sctk_rail_info_s *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll)){
  int vp = sctk_thread_get_vp();
  sctk_ib_cp_task_t *task = NULL;
  int nb_found = 0;

  if (vps == NULL) return 0;

  if (cq == recv_cq) {
    for (task=vps[vp].tasks; task; task=task->hh_vp.next)
      nb_found += __cp_poll(rail, poll, &(task->recv_ibufs), task, func);
  } else if (cq == send_cq) {
    for (task=vps[vp].tasks; task; task=task->hh_vp.next)
      nb_found += __cp_poll(rail, poll, &(task->send_ibufs), task, func);
  } else not_reachable();

  return nb_found;
}

static inline int __cp_steal(struct sctk_rail_info_s* rail,struct sctk_ib_polling_s *poll, sctk_ibuf_t** volatile list, sctk_ib_cp_task_t *task, sctk_ib_cp_task_t* stealing_task,
    int (*func) (struct sctk_rail_info_s *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll)){
  sctk_ibuf_t *ibuf = NULL;
  int nb_found = 0;
  double s, e;

retry:
  if (*list != NULL) {
    if (sctk_spinlock_trylock(&task->lock) == 0) {
      if ( *list != NULL) {
        ibuf = *list;
        DL_DELETE(*list, ibuf);
        sctk_spinlock_unlock(&task->lock);
        /* Run the polling function */
        s = sctk_atomics_get_timestamp();
        func(rail, ibuf, 2, poll);
        e = sctk_atomics_get_timestamp();

        /* Set timers */
        sctk_spinlock_lock(&task->lock_timers);
        task->time_stolen += e - s;
        sctk_spinlock_unlock(&task->lock_timers);
        /* End of set timers */
        sctk_spinlock_lock(&stealing_task->lock_timers);
        stealing_task->time_steals += e - s;
        sctk_spinlock_unlock(&stealing_task->lock_timers);

        CP_PROF_INC(task,poll_stolen);
        CP_PROF_INC(stealing_task,poll_steals);
        nb_found++;
        goto retry;
      }
      sctk_spinlock_unlock(&task->lock);
    }
  }
  return nb_found;
}

#define STEAL_CURRENT_NODE(x) do {  \
    CDL_FOREACH(&vps[vp], tmp_vp) { \
      for (task=tmp_vp->tasks; task; task=task->hh_vp.next) { \
        nb_found += __cp_steal(rail, poll, x, task, stealing_task, func);  \
      } \
      /* Return if message stolen */  \
      if (nb_found) return nb_found;  \
    } \
} while (0)

#define STEAL_OTHER_NODE(x) do { \
    CDL_FOREACH(numa_list, tmp_numa) {  \
      CDL_FOREACH(tmp_numa->vps, tmp_vp) {  \
        for (task=tmp_vp->tasks; task; task=task->hh_vp.next) { \
          nb_found += __cp_steal(rail, poll, x, task, stealing_task, func);  \
        } \
      } \
      /* steal a restricted number of NUMA nodes */ \
      ++i; if(i >= max_numa_to_steal) return nb_found;  \
      /* Return if message stolen */  \
      if (nb_found) return nb_found;  \
    } \
} while (0)

int sctk_ib_cp_steal( sctk_rail_info_t* rail, struct sctk_ib_polling_s *poll, enum sctk_ib_cp_poll_cq_e cq,
    int (*func) (sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll)) {
  int nb_found = 0;
  int vp = sctk_thread_get_vp();
  sctk_ib_cp_task_t *stealing_task = NULL;
  sctk_ib_cp_task_t *task = NULL;
  const int max_numa_to_steal = 1;
  vp_t* tmp_vp;
  numa_t* tmp_numa;
  int i = 0;

  if (vps == NULL) return 0;

  /* XXX: Not working with oversubscribing */
  stealing_task = vps[sctk_thread_get_vp()].tasks;
  if (cq == recv_cq) {
    /* First, try to steal from the same NUMA node*/
    STEAL_CURRENT_NODE(&task->recv_ibufs);
    /* Secondly, try to steal from another NUMA node  */
    STEAL_OTHER_NODE(&task->recv_ibufs);
  } else if (cq == send_cq) {
    /* First, try to steal from the same NUMA node*/
    STEAL_CURRENT_NODE(&task->send_ibufs);
    /* Secondly, try to steal from another NUMA node  */
    STEAL_OTHER_NODE(&task->send_ibufs);
  }
  return nb_found;
}

sctk_ib_cp_task_t *sctk_ib_cp_get_polling_task() {
  sctk_ib_cp_task_t* task;
  /* XXX: Not working with oversubscribing */
  task = vps[sctk_thread_get_vp()].tasks;
  assume(task);
  return task;
}

int sctk_ib_cp_handle_message(sctk_rail_info_t* rail,
    sctk_ibuf_t *ibuf, int dest_task, enum sctk_ib_cp_poll_cq_e cq) {
  sctk_ib_cp_task_t *task = NULL;
  int vp = sctk_thread_get_vp();

  HASH_FIND(hh_vp,vps[vp].tasks,&dest_task, sizeof(int),task);
  /* The task is on the current VP, we handle the message */
  if (task) {
    sctk_nodebug("Task %d found on VP %d", task->rank, vp);
    CP_PROF_INC(task,matched);
    return 0;
  }
  /* XXX: Do not support thread migration */
  HASH_FIND(hh_all,all_tasks,&dest_task, sizeof(int),task);
  if (!task) {
//    sctk_error("Task %d not found !", dest_task);
    /* We return without error -> indirect message */
    return 0;
  }
  CP_PROF_INC(task,not_matched);

  sctk_nodebug("Ibuf %p added (dest (rank:%d,vp:%d) polling vp:%d)", ibuf, task->rank, task->vp, vp);
  /* Add the ibuf to the pending list */
  /* Choose the right ibufs pending list */
  sctk_spinlock_lock(&task->lock);
  if (cq == recv_cq) {
    DL_APPEND(task->recv_ibufs, ibuf);
  } else if (cq == send_cq) {
    DL_APPEND(task->send_ibufs, ibuf);
  }
  sctk_spinlock_unlock(&task->lock);
  return 1;
}

#endif
