#include "mpc_common_rank.h"

/* Context switched task rank storage */
__thread int __mpc_task_rank = -2;

/* OS Process Count Getters */

int __process_count = 1;
int __process_rank = 0;

void mpc_common_set_process_count(int process_count)
{
    __process_count = process_count;
}

void mpc_common_set_process_rank(int process_rank)
{
    __process_rank = process_rank;
}

/* MPC local_process Count Getters */

int __local_process_count = 1;
int __local_process_rank = 0;

void mpc_common_set_local_process_count(int local_process_count)
{
    __local_process_count = local_process_count;
}

void mpc_common_set_local_process_rank(int local_process_rank)
{
    __local_process_rank = local_process_rank;
}

/* Node Count Getters */

int __node_count = 1;
int __node_rank = 0;

void mpc_common_set_node_count(int node_count)
{
    __node_count = node_count;
}

void mpc_common_set_node_rank(int node_rank)
{
    __node_rank = node_rank;
}



