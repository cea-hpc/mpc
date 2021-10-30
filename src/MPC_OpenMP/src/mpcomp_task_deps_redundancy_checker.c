# include "mpcomp_core.h"
# include "mpcomp_types.h"

mpc_omp_task_deps_redundancy_checker_t *
_mpc_omp_task_deps_redundancy_checker_reset(mpc_omp_thread_t * thread, uintptr_t ndeps)
{
    assert(thread);
    assert(ndeps);

    mpc_omp_task_deps_redundancy_checker_t * checker;

    /* if there is only few dependencies, use a single list */
    if (ndeps < mpc_omp_conf_get()->task_deps_hmap_threshold)
    {
        mpc_omp_task_deps_redundancy_checker_list_t * list = &(thread->task_infos.deps_redundancy_checkers.list);
        if (list->capacity < ndeps)
        {
            if (list->addrs) free(list->addrs);
            list->capacity  = ndeps;
            list->addrs     = (void **) malloc(sizeof(void *) * ndeps);
            assert(list->addrs);
        }
        checker = (mpc_omp_task_deps_redundancy_checker_t *) list;
    }
    /* otherwise, use a hmap to check redundancy */
    else
    {
        mpc_omp_task_deps_redundancy_checker_hmap_t * hmap = &(thread->task_infos.deps_redundancy_checkers.hmap);
        if (hmap->nnodes < ndeps)
        {
            if (hmap->nodes) free(hmap->nodes);
            hmap->map    = NULL;
            hmap->nnodes = ndeps;
            hmap->nodes  = (mpc_omp_task_deps_redundancy_checker_hmap_node_t *) malloc(sizeof(mpc_omp_task_deps_redundancy_checker_hmap_node_t) * ndeps);
            assert(hmap->nodes);
        }
        else
        {
            HASH_CLEAR(hh, hmap->map);
        }
        checker = (mpc_omp_task_deps_redundancy_checker_t *) hmap;
    }

    checker->ndeps  = ndeps;
    checker->uniq   = 0;

    return checker;
}

int
_mpc_omp_task_deps_redundancy_checker_check(mpc_omp_task_deps_redundancy_checker_t * checker, void * addr)
{
    switch (checker->type)
    {
        /* list check */
        case (MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_LIST):
        {
            mpc_omp_task_deps_redundancy_checker_list_t * list = (mpc_omp_task_deps_redundancy_checker_list_t *) checker;
            assert(checker->uniq < list->capacity);

            TODO("unroll this list");
            uintptr_t i;
            for (i = 0 ; i < checker->uniq ; ++i)
            {
                if (list->addrs[i] == addr) return 1;
            }
            list->addrs[checker->uniq] = addr;
            break ;
        }

        /* hmap check */
        case (MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_HMAP):
        {
            mpc_omp_task_deps_redundancy_checker_hmap_t * hmap = (mpc_omp_task_deps_redundancy_checker_hmap_t *) checker;
            mpc_omp_task_deps_redundancy_checker_hmap_node_t * node;
            HASH_FIND_PTR(hmap->map, &addr, node);
            if (node)  return 1;
            node = hmap->nodes + checker->uniq;
            node->addr = addr;
            HASH_ADD_PTR(hmap->map, addr, node);
            break ;
        }

        default:
        {
            assert(0);
        }
    }
    ++checker->uniq;
    return 0;
}

void
_mpc_omp_task_deps_redundancy_checker_init(mpc_omp_thread_t * thread)
{
    /* the list checker */
    {
        mpc_omp_task_deps_redundancy_checker_list_t * list = &(thread->task_infos.deps_redundancy_checkers.list);
        list->parent.type   = MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_LIST;
        list->addrs         = NULL;
        list->capacity      = 0;
    }

    /* the hmap checker */
    {
        mpc_omp_task_deps_redundancy_checker_hmap_t * hmap = &(thread->task_infos.deps_redundancy_checkers.hmap);
        hmap->parent.type   = MPC_OMP_TASK_DEPS_REDUNDANCY_CHECKER_TYPE_HMAP;
        hmap->nodes         = NULL;
        hmap->nnodes        = 0;
        hmap->map           = NULL;
    }
}

void
_mpc_omp_task_deps_redundancy_checker_deinit(mpc_omp_thread_t * thread)
{
    /* the list checker */
    {
        mpc_omp_task_deps_redundancy_checker_list_t * checker = &(thread->task_infos.deps_redundancy_checkers.list);
        if (checker->addrs) free(checker->addrs);
    }

    /* the hmap checker */
    {
        mpc_omp_task_deps_redundancy_checker_hmap_t * checker = &(thread->task_infos.deps_redundancy_checkers.hmap);
        if (checker->nodes) free(checker->nodes);
    }
}
