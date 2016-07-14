
#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>

#include "mpcomp_internal.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_task_stealing.h"

/* Compare nb element in first entry of tab
 */
static inline int 
mpcomp_task_cmp_decr_func( const void * opq_a, const void *opq_b ) 
{
    const int * a = (int*) opq_a;
    const int * b = (int*) opq_b;
    return (a[0] == b[0]) ? b[1] - a[1] : b[0] - a[0];
}  

static inline int
mpcomp_task_get_depth_from_global_rank(struct mpcomp_mvp_s *mvp, const int globalRank)
{

    return  (mpcomp_is_leaf(globalRank)) ? 
            mvp->tree_array[globalRank]->father->depth + 1:
            mvp->tree_array[globalRank]->depth;
}

int 
mpcomp_task_get_victim_default(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    return globalRank;
}

/** Hierachical stealing policy **/

void
mpcomp_task_init_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

static void
mpcomp_task_prepare_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

int 
mpcomp_task_get_victim_hierarchical(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    return mpcomp_get_neighbour(globalRank, index);
}

/** Random stealing policy **/

void
mpcomp_task_init_victim_random(mpcomp_node_t *n, int globalRank)
{
    sctk_assert(n != NULL); 
    if(n->tasklist_randBuffer)
        return; 

    n->tasklist_randBuffer = mpcomp_malloc(1, sizeof(struct drand48_data), n->id_numa);
    srand48_r(sctk_get_time_stamp() * globalRank, n->tasklist_randBuffer);
}

static void
mpcomp_task_prepare_victim_random(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    return;
}

int 
mpcomp_task_get_victim_random(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    int victim, n;
    long int value;
    mpcomp_thread_t *t;
    struct drand48_data *randBuffer;

    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    /* Only victim id and value while change */
    const int __globalRank = globalRank;
    const int __depth = (mpcomp_is_leaf(__globalRank)) ? 
            t->mvp->tree_array[globalRank]->father->depth + 1: 
            t->mvp->tree_array[globalRank]->depth;
    const int __first_rank = t->instance->tree_array_first_rank[__depth];
    const int __level_size = t->instance->tree_level_size[__depth];
    randBuffer = t->mvp->tree_array[__globalRank]->tasklist_randBuffer;
    
    do{
        /* Get a random value */
        lrand48_r(randBuffer, &value);
        /* Select randomly a globalRank among neighbourhood */
        victim = (value % __level_size + __first_rank);
      } while(victim == __globalRank); /* Retry random draw if the victim is the thief */ 
   
   return victim;
}

/** Random order stealing policy **/

static void
mpcomp_task_prepare_victim_random_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    int i, j, tmp;
    long int value;
    int *order;
    struct drand48_data *randBuffer;
    mpcomp_thread_t *t;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    if(!t->larceny_order)
        mpcomp_task_allocate_larceny_order(t);

    const int __globalRank = globalRank;
    const int __depth = (mpcomp_is_leaf(__globalRank)) ? 
            t->mvp->tree_array[globalRank]->father->depth + 1: 
            t->mvp->tree_array[globalRank]->depth;
    const int __first_rank = t->instance->tree_array_first_rank[__depth];
    const int __nbVictims = t->instance->tree_level_size[__depth] - 1; 

    randBuffer = t->mvp->tree_array[__globalRank]->tasklist_randBuffer;
    order = t->larceny_order;

    for(i = 0; i < __nbVictims; i++)
    {
        order[i] = __first_rank + i;
        order[i] = (order[i] >= __globalRank) ? order[i] + 1 : order[i]; 
    }
    
    for(i = 0; i < __nbVictims; i++) 
    {
        lrand48_r(randBuffer, &value);
        j = value % (__nbVictims - i);
        tmp = order[i]; 
        order[i] = order[i+j];
        order[i+j] = tmp;
    }
}

int 
mpcomp_task_get_victim_random_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);
 
    if(index == 1)
         mpcomp_task_prepare_victim_random_order(globalRank, index, type);

    sctk_assert(t->larceny_order);
    return t->larceny_order[index-1];
}

static void
mpcomp_task_prepare_victim_roundrobin(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    int i;
    int *order;
    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    const int __globalRank = globalRank;
    const int __depth = (mpcomp_is_leaf(__globalRank)) ? 
            t->mvp->tree_array[globalRank]->father->depth + 1: 
            t->mvp->tree_array[globalRank]->depth;

    const int __first_rank = t->instance->tree_array_first_rank[__depth];
    const int __nbVictims = t->instance->tree_level_size[__depth] - 1; 

    if(!t->larceny_order)
        mpcomp_task_allocate_larceny_order(t);
    
    order = t->larceny_order;
    const int __decal = (__globalRank - __first_rank + 1) % (__nbVictims + 1);
    for(i = 0; i < __nbVictims; i++)
    {
        order[i] = __first_rank + (__decal + i) % (__nbVictims + 1); 
        order[i] = (order[i] >= __globalRank) ? order[i] + 1 : order[i]; 
    }
}

int 
mpcomp_task_get_victim_roundrobin(int globalRank, int index, mpcomp_tasklist_type_t type)
{     
    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);
    
    if(index == 1)
         mpcomp_task_prepare_victim_roundrobin(globalRank, index, type);

    sctk_assert(t->larceny_order);
    return t->larceny_order[index-1];
}

int 
mpcomp_task_get_victim_producer(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    int max_elt, victim, i;

    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    const int __globalRank = globalRank;
    const int __depth = (mpcomp_is_leaf(__globalRank)) ? 
            t->mvp->tree_array[globalRank]->father->depth + 1: 
            t->mvp->tree_array[globalRank]->depth;

    const int __first_rank = t->instance->tree_array_first_rank[__depth];
    const int __nbVictims = t->instance->tree_level_size[__depth] - 1; 
    
    /* On recherche la liste avec le plus de tache a voler */
    max_elt = 0;
    victim = (globalRank == __first_rank) ? __first_rank + 1 : __first_rank;
    
    for(i = 0; i < __nbVictims; i++)
    {
        int rank, nb_elt;
        struct mpcomp_task_list_s *list;

        rank = i + __first_rank;
        rank += (rank >= __globalRank) ? 1 : 0;
        list = (struct mpcomp_task_list_s *) mpcomp_task_get_list(rank, type);        
        nb_elt = sctk_atomics_load_int(&list->nb_elements); 
        
        if(max_elt < nb_elt)
        {
            max_elt = nb_elt;
            victim = rank;
        }
    } 

    return victim;
}

int 
mpcomp_task_prepare_victim_producer_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    int i;
    
    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    const int __globalRank = globalRank;
    const int __depth = (mpcomp_is_leaf(__globalRank)) ? 
            t->mvp->tree_array[globalRank]->father->depth + 1: 
            t->mvp->tree_array[globalRank]->depth;

    const int __first_rank = t->instance->tree_array_first_rank[__depth];
    const int __nbVictims = t->instance->tree_level_size[__depth] - 1; 

    if (!t->larceny_order)
           mpcomp_task_allocate_larceny_order(t);
    
    int *order = t->larceny_order;
    int nbElements[__nbVictims][2];

    for(i = 0; i < __nbVictims; i++)
    {
        struct mpcomp_task_list_s *list;
        nbElements[i][1] = i + __first_rank; 
        nbElements[i][1] += (nbElements[i][1] >= __globalRank) ? 1 : 0;
        list = (struct mpcomp_task_list_s *) mpcomp_task_get_list(nbElements[i][1], type);        
        nbElements[i][0] = sctk_atomics_load_int(&list->nb_elements);
    } 

    void * extra_arg;
    qsort_r(nbElements, __nbVictims, 2*sizeof(int), mpcomp_task_cmp_decr_func,extra_arg);  
    
    for(i = 0; i < __nbVictims; i++)
        order[i] = nbElements[i][1];
}

int 
mpcomp_task_get_victim_producer_order(int globalRank, int index, mpcomp_tasklist_type_t type)
{
    mpcomp_thread_t *t;
    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);
    
    if(index == 1)
         mpcomp_task_prepare_victim_producer_order(globalRank, index, type);

    sctk_assert(t->larceny_order);
    return t->larceny_order[index-1];
}


