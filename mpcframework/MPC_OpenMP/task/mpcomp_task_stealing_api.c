#if 0
#define MPCOMP_VICTIM_NULL -1

struct mpcomp_task_stealing_funcs_s
{
    int (*initialize)(mpcomp_node_t* n, int globalRank);
    int (*selection)(int globalRank, int index, mpcomp_tasklist_type_t type);
};

typedef struct mpcomp_task_stealing_funcs_s mpcomp_task_stealing_funcs_t;

static mpcomp_task_stealing_funcs_t* __mpcomp_task_stealing_generic_funcs = NULL;

int
__mpcomp_task_init_victim_generic(mpcomp_node_t *n, int globalRank)
{
    sctk_assert(__mpcomp_task_stealing_generic_funcs != NULL);
    if(__mpcomp_task_stealing_generic_funcs->initialize)
        return __mpcomp_task_stealing_generic_funcs->initialize(n, globalRank);
    return 1;
}

int 
__mpcomp_task_get_victim_generic(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    sctk_assert(__mpcomp_task_stealing_generic_funcs != NULL);
    if(__mpcomp_task_stealing_generic_funcs->selection)
        return __mpcomp_task_stealing_generic_funcs->selection(globalRank, index, type);
   return MPCOMP_VICTIM_NULL;  // no victim found
}

int 
__mpcomp_task_set_stealing_policy(void)
{
    /* Source generic pointer */
}
#endif 
