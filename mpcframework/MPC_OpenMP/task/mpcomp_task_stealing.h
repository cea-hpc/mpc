#if (!defined(__SCTK_MPCOMP_TASK_STEALING_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_STEALING_H__

#include "sctk_debug.h"
#include "mpcomp_alloc.h"
#include "mpcomp_types.h"

struct mpcomp_task_stealing_funcs_s {
  char *namePolicy;
  int allowMultipleVictims;
  int (*pre_init)(void);
  int (*get_victim)(int globalRank, int index, mpcomp_tasklist_type_t type);
};
typedef struct mpcomp_task_stealing_funcs_s mpcomp_task_stealing_funcs_t;

int mpcomp_task_get_victim_default(int globalRank, int index,
                                   mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_hierarchical(int globalRank, int index,
                                        mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_random(int globalRank, int index,
                                  mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_random_order(int globalRank, int index,
                                        mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_roundrobin(int globalRank, int index,
                                      mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_producer(int globalRank, int index,
                                    mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_producer_order(int globalRank, int i,
                                          mpcomp_tasklist_type_t type);

void mpcomp_task_init_victim_random( mpcomp_generic_node_t* gen_node );

static inline void mpcomp_task_allocate_larceny_order(mpcomp_thread_t *thread) {
  sctk_assert(thread);
  sctk_assert(thread->instance);
  sctk_assert(thread->instance->team);
 
  mpcomp_team_t* team = thread->instance->team;
  const int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(team, MPCOMP_TASK_TYPE_UNTIED);
  const int max_num_victims = thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
  sctk_assert(max_num_victims >= 0);

  int *larceny_order = (int *) mpcomp_alloc( max_num_victims * sizeof(int) );
  MPCOMP_TASK_THREAD_SET_LARCENY_ORDER(thread, larceny_order);
}

#endif /* __SCTK_MPCOMP_TASK_STEALING_H__ */
