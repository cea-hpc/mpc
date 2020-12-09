#ifndef MPC_MPI_CONFIG_H_
#define MPC_MPI_CONFIG_H_

#include <mpc_conf.h>

struct _mpc_mpi_config_nbc
{
    /* NBC Thread */
    int progress_thread;
    int thread_basic_prio;
    char thread_bind_function_name[MPC_CONF_STRING_SIZE];
    void (*thread_bind_function)();

    /* NBC EGREQ */
    int use_egreq_barrier;
    int use_egreq_bcast;
    int use_egreq_gather;
    int use_egreq_scatter;
    int use_egreq_reduce;

};




struct _mpc_mpi_config
{
    int message_buffering;
    struct _mpc_mpi_config_nbc nbc;
    
};

struct _mpc_mpi_config * _mpc_mpi_config(void);


void _mpc_mpi_config_init(void);
void _mpc_mpi_config_check();

#endif