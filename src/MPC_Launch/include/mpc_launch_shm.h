#ifndef MPC_LAUNCH_SHM_H_
#define MPC_LAUNCH_SHM_H_

#include <stddef.h>

typedef enum
{
    MPC_LAUNCH_SHM_USE_PMI,
    MPC_LAUNCH_SHM_USE_MPI
}mpc_launch_shm_exchange_method_t;

typedef union
{
    struct
    {

    }pmi;

    struct
    {
        int (*rank)(void *arg);
        int (*bcast)(void *buff, size_t len, void *arg);
        int (*barrier)(void *arg);
        void *pcomm;
    }mpi;

}mpc_launch_shm_exchange_params_t;


void * mpc_launch_shm_map(size_t size, mpc_launch_shm_exchange_method_t method, mpc_launch_shm_exchange_params_t *params);
int mpc_launch_shm_unmap(void *ptr, size_t size);


#endif /* MPC_LAUNCH_SHM_H_ */