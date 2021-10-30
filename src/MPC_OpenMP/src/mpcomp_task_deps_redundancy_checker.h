#ifndef __MPCOMP_TASK_DEPS_REDUNDANCY_CHECKER_H__
# define __MPCOMP_TASK_DEPS_REDUNDANCY_CHECKER_H__

# include "uthash.h"

typedef enum    mpc_omp_task_deps_redundancy_checker_type_t
{
    MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_LIST,
    MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_HMAP,
    MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_MAX
}               mpc_omp_task_deps_redundancy_checker_type_t;

typedef struct  mpc_omp_task_deps_redundancy_checker_t
{
    mpc_omp_task_deps_redundancy_checker_type_t type;
    uintptr_t ndeps;
    uintptr_t uniq;
}               mpc_omp_task_deps_redundancy_checker_t;

/* list checker, O(n) check complexity */
typedef struct  mpc_omp_task_deps_redundancy_checker_list_t
{
    mpc_omp_task_deps_redundancy_checker_t parent;
    void ** addrs;
    uintptr_t capacity;
}               mpc_omp_task_deps_redundancy_checker_list_t;

/* hmap checker, O(1) check complexity */
typedef struct  mpc_omp_task_deps_redundancy_checker_hmap_node_t
{
    void            * addr;
    UT_hash_handle  hh;
}               mpc_omp_task_deps_redundancy_checker_hmap_node_t;

typedef struct  mpc_omp_task_deps_redundancy_checker_hmap_t
{
    mpc_omp_task_deps_redundancy_checker_t parent;
    mpc_omp_task_deps_redundancy_checker_hmap_node_t * nodes;
    uintptr_t nnodes;
    mpc_omp_task_deps_redundancy_checker_hmap_node_t * map;
}               mpc_omp_task_deps_redundancy_checker_hmap_t;

struct mpc_omp_thread_s;

/**
 * Retrieve the appropriate task dependency redundancy checker,
 * for the given number of dependencies to handle, on given thread.
 * Also resets the checker.
 * \param thread : the thread procuding the task
 * \param ndeps : number of dependencies
 * \return the task dependency checker
 */
mpc_omp_task_deps_redundancy_checker_t * _mpc_omp_task_deps_redundancy_checker_reset(struct mpc_omp_thread_s * thread, uintptr_t ndeps);

/**
 * Check if 'addr' was alraedy checked since
 * last '_mpc_omp_task_deps_redundancy_checker_get' call
 * \param checker : the checker from '_mpc_omp_task_deps_redundancy_checker_get'
 * \param addr : the address to check
 * \return 1 if the address was checked previously, 0 otherwise
 */
int _mpc_omp_task_deps_redundancy_checker_check(mpc_omp_task_deps_redundancy_checker_t * checker, void * addr);

/* to be called on thread init */
void _mpc_omp_task_deps_redundancy_checker_init(struct mpc_omp_thread_s * thread);

/* to be called on thread deinit */
void _mpc_omp_task_deps_redundancy_checker_deinit(struct mpc_omp_thread_s * thread);

#endif /* __MPCOMP_TASK_DEPS_REDUNDANCY_CHECKER_H__ */
