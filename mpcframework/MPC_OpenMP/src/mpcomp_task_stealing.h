#if (!defined(__SCTK_MPCOMP_TASK_STEALING_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_STEALING_H__

#include "mpcomp_internal.h"

struct mpcomp_task_stealing_funcs_s
{
    char *namePolicy;
    int allowMultipleVictims;
    int (*pre_init)(void); 
    int (*get_victim)(int globalRank, int index, mpcomp_tasklist_type_t type);
};
typedef struct mpcomp_task_stealing_funcs_s mpcomp_task_stealing_funcs_t;

int mpcomp_task_get_victim_default(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_random(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_random_order(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_roundrobin(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_producer(int globalRank, int index, mpcomp_tasklist_type_t type);
int mpcomp_task_get_victim_producer_order(int globalRank, int i, mpcomp_tasklist_type_t type);

static inline void 
mpcomp_task_allocate_larceny_order(mpcomp_thread_t *t)
{
     int depth = t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED];
     int nbMaxLists = t->instance->tree_level_size[depth];

     t->larceny_order = mpcomp_malloc(1, (nbMaxLists-1) * sizeof(int), t->mvp->father->id_numa);
}

#endif /* __SCTK_MPCOMP_TASK_STEALING_H__ */

