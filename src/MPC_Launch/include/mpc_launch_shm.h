#ifndef MPC_LAUNCH_SHM_H_
#define MPC_LAUNCH_SHM_H_

#include <stddef.h>

typedef enum
{
    MPC_LAUNCH_SHM_USE_PMI
}mpc_launch_shm_exchange_method_t;


void * mpc_launch_shm_map(size_t size, mpc_launch_shm_exchange_method_t method);

#endif /* MPC_LAUNCH_SHM_H_ */