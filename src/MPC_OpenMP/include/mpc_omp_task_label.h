#ifndef MPC_OMP_TASK_LABEL_H
# define MPC_OMP_TASK_LABEL_H

# include <mpc_omp.h>

static char MPC_OMP_TASK_LABEL_BUFFER_NAME[MPC_OMP_TASK_LABEL_MAX_LENGTH];
#  define MPC_OMP_TASK_SET_LABEL(...)    do {\
    snprintf(MPC_OMP_TASK_LABEL_BUFFER_NAME, MPC_OMP_TASK_LABEL_MAX_LENGTH, __VA_ARGS__);           \
    mpc_omp_task_label(MPC_OMP_TASK_LABEL_BUFFER_NAME);                                             \
} while(0)

#endif
