#ifndef SCTK_RANK_H
#define SCTK_RANK_H

#include <sctk_config.h>

/* OS Process Count Getters */

void set_process_count(int process_count);
void set_process_rank(int process_rank);

extern int __process_count;
extern int __process_rank;

static inline int get_process_count(void)
{
    return __process_count;
}

static inline int get_process_rank(void)
{
    return __process_rank;
}

/* OS local_process Count Getters */

void set_local_process_count(int local_process_count);
void set_local_process_rank(int local_process_rank);

extern int __local_process_count;
extern int __local_process_rank;

static inline int get_local_process_count(void)
{
    return __local_process_count;
}

static inline int get_local_process_rank(void)
{
    return __local_process_rank;
}

/* Node Count Getters */

void set_node_count(int node_count);
void set_node_rank(int node_rank);

extern int __node_count;
extern int __node_rank;

static inline int get_node_count(void)
{
    return __node_count;
}

static inline int get_node_rank(void)
{
    return __node_rank;
}

#ifndef MPC_Threads
    /* These are the generic task rank and count
       getters when MPC_Threads is not available */

    static inline int get_task_rank(void)
    {
        return 0;
    }


    static inline int get_task_count(void)
    {
        return 1;
    }

#endif /* MPC_Thread */


#endif /* SCTK_RANK_H */