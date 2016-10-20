#if (!defined(__SCTK_MPCOMP_TASK_STEALING_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_STEALING_H__

#include "mpcomp_alloc.h"
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
mpcomp_task_allocate_larceny_order(mpcomp_thread_t* thread)
{
	sctk_assert( thread );	
	sctk_assert( thread->instance );
	sctk_assert( thread->instance->team );

	const int depth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( thread->instance->team, MPCOMP_TASK_TYPE_UNTIED );
	sctk_assert( depth >= 0 );

	const int max_num_victims = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, depth ) - 1;
	sctk_assert( max_num_victims >= 0 );
	
	sctk_assert( thread->mvp );
	sctk_assert( thread->mvp->father );
	const int numa_node_id = thread->mvp->father->id_numa;
	sctk_assert( numa_node_id >= 0 );

	int* larceny_order = (int*) mpcomp_malloc( 1, max_num_victims * sizeof(int), numa_node_id );
	MPCOMP_TASK_THREAD_SET_LARCENY_ORDER( thread, larceny_order );	
}

#endif /* __SCTK_MPCOMP_TASK_STEALING_H__ */

