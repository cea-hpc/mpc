#ifndef MPC_MPI_CONFIG_H_
#define MPC_MPI_CONFIG_H_

#include <mpc_conf.h>
#include <mpc_mpi.h>
#include <mpc_common_debug.h>
#include <mpc_lowcomm_communicator.h>


/****************************
 * NON-BLOCKING COLLECTIVES *
 ****************************/

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

/***************
 * COLLECTIVES *
 ***************/

struct _mpc_mpi_config_coll_array
{
    char barrier_name[MPC_CONF_STRING_SIZE];
    int (*barrier)(MPI_Comm);

    char bcast_name[MPC_CONF_STRING_SIZE];
    int (*bcast)(void *, int, MPI_Datatype, int, MPI_Comm);

    char allgather_name[MPC_CONF_STRING_SIZE];
    int (*allgather)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);

    char allgatherv_name[MPC_CONF_STRING_SIZE];
    int (*allgatherv)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm);

    char alltoall_name[MPC_CONF_STRING_SIZE];
    int (*alltoall)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm);

    char alltoallv_name[MPC_CONF_STRING_SIZE];
    int (*alltoallv)(const void *, const int *, const int *, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm);

    char alltoallw_name[MPC_CONF_STRING_SIZE];
    int (*alltoallw)(const void *, const int *, const int *, const MPI_Datatype *, void *, const int *, const int *, const MPI_Datatype *, MPI_Comm);

    char gather_name[MPC_CONF_STRING_SIZE];
    int (*gather)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

    char gatherv_name[MPC_CONF_STRING_SIZE];
    int (*gatherv)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, int, MPI_Comm);

    char scatter_name[MPC_CONF_STRING_SIZE];
    int (*scatter)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

    char scatterv_name[MPC_CONF_STRING_SIZE];
    int (*scatterv)(const void *, const int *, const int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm);

    char reduce_name[MPC_CONF_STRING_SIZE];
    int (*reduce)(const void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm);

    char allreduce_name[MPC_CONF_STRING_SIZE];
    int (*allreduce)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

    char reduce_scatter_name[MPC_CONF_STRING_SIZE];
    int (*reduce_scatter)(const void *, void *, const int *, MPI_Datatype, MPI_Op, MPI_Comm);

    char reduce_scatter_block_name[MPC_CONF_STRING_SIZE];
    int (*reduce_scatter_block)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

    char scan_name[MPC_CONF_STRING_SIZE];
    int (*scan)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

    char exscan_name[MPC_CONF_STRING_SIZE];
    int (*exscan)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

};

struct _mpc_mpi_config_coll_algorithm_array
{
    char barrier_name[MPC_CONF_STRING_SIZE];
    int (*barrier)(MPI_Comm, int, void *, void *);

    char bcast_name[MPC_CONF_STRING_SIZE];
    int (*bcast)(void *, int, MPI_Datatype, int, MPI_Comm, int, void *, void *);

    char allgather_name[MPC_CONF_STRING_SIZE];
    int (*allgather)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm, int, void *, void *);

    char allgatherv_name[MPC_CONF_STRING_SIZE];
    int (*allgatherv)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm, int, void *, void *);

    char alltoall_name[MPC_CONF_STRING_SIZE];
    int (*alltoall)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, MPI_Comm, int, void *, void *);

    char alltoallv_name[MPC_CONF_STRING_SIZE];
    int (*alltoallv)(const void *, const int *, const int *, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, MPI_Comm, int, void *, void *);

    char alltoallw_name[MPC_CONF_STRING_SIZE];
    int (*alltoallw)(const void *, const int *, const int *, const MPI_Datatype *, void *, const int *, const int *, const MPI_Datatype *, MPI_Comm, int, void *, void *);

    char gather_name[MPC_CONF_STRING_SIZE];
    int (*gather)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm, int, void *, void *);

    char gatherv_name[MPC_CONF_STRING_SIZE];
    int (*gatherv)(const void *, int, MPI_Datatype, void *, const int *, const int *, MPI_Datatype, int, MPI_Comm, int, void *, void *);

    char scatter_name[MPC_CONF_STRING_SIZE];
    int (*scatter)(const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm, int, void *, void *);

    char scatterv_name[MPC_CONF_STRING_SIZE];
    int (*scatterv)(const void *, const int *, const int *, MPI_Datatype, void *, int, MPI_Datatype, int, MPI_Comm, int, void *, void *);

    char reduce_name[MPC_CONF_STRING_SIZE];
    int (*reduce)(const void *, void *, int, MPI_Datatype, MPI_Op, int, MPI_Comm, int, void *, void *);

    char allreduce_name[MPC_CONF_STRING_SIZE];
    int (*allreduce)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm, int, void *, void *);

    char reduce_scatter_name[MPC_CONF_STRING_SIZE];
    int (*reduce_scatter)(const void *, void *, const int *, MPI_Datatype, MPI_Op, MPI_Comm, int, void *, void *);

    char reduce_scatter_block_name[MPC_CONF_STRING_SIZE];
    int (*reduce_scatter_block)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm, int, void *, void *);

    char scan_name[MPC_CONF_STRING_SIZE];
    int (*scan)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm, int, void *, void *);

    char exscan_name[MPC_CONF_STRING_SIZE];
    int (*exscan)(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm, int, void *, void *);

};

void _mpc_mpi_config_coll_array_resolve(struct _mpc_mpi_config_coll_array *coll, char *family);


static inline void _mpc_mpi_config_coll_route(MPI_Comm comm,
                                              int (*inter)(),
                                              int (*intra_shm)(),
                                              int (*intra_shared_node)(),
                                              int (*intra)(),
                                              int (**out_func)())
{
    int is_intercomm = 0;
    int is_shm = 0;
    int is_shared_node = 0;

    mpc_lowcomm_communicator_attributes(comm,
                                        &is_intercomm,
                                        &is_shm,
                                        &is_shared_node);

    //mpc_common_debug_error("IC %d SHM %d shared %d (IC %p SH %p SHAN %p INT %p", is_intercomm, is_shm, is_shared_node, inter, intra_shm, intra_shared_node, intra);

    if(is_intercomm)
    {
        *out_func = inter;
    }
    else
    {
        if(is_shm && intra_shm)
        {
            *out_func = intra_shm;
        }
        else if(is_shared_node && intra_shared_node)
        {
            *out_func = intra_shared_node;
        }
        else
        {
            *out_func = intra;
        }
    }
}


#define MPC_MPI_CONFIG_ROUTE_COLL(pointer, comm, coll_name) do{\
                                                            _mpc_mpi_config_coll_route(comm, \
                                                                                       _mpc_mpi_config()->coll_intercomm.coll_name,  \
                                                                                       _mpc_mpi_config()->coll_intracomm_shm.coll_name, \
                                                                                       _mpc_mpi_config()->coll_intracomm_shared_node.coll_name,\
                                                                                       _mpc_mpi_config()->coll_intracomm.coll_name, \
                                                                                       (int (**)())&pointer);\
                                                            assume(pointer != NULL);\
                                                            }while(0)

/**********************************
 * TOPOLOGICAL COLLECTIVE OPTIONS *
 **********************************/

struct _mpc_mpi_config_topo_coll_opts
{
    /*
     * Use topological algorithms for all collective operations
     */
    char full[MPC_CONF_STRING_SIZE]; ///< Enable/disable topological algorithms globally.
                                     ///< Ignore other topological configuration entries
                                     ///< unless full == "auto".

    int max_level;

    /* 
     * Use topological algorithms for all persistent / non-blocking /
     * blocking operations
     */
    int persistent;
    int non_blocking;
    int blocking;

    /* 
     * Use topological algorithms for a given collective operation.
     * A topological algorithm is called if either the operation type (blocking,
     * non-blocking...) OR the collective operation (bcast, reduce...) are non-zero.
     */
    int barrier;
    int bcast;
    int allgather;
    int allgatherv;
    int alltoall;
    int alltoallv;
    int alltoallw;
    int gather;
    int gatherv;
    int scatter;
    int scatterv;
    int reduce;
    int allreduce;
    int reduce_scatter;
    int reduce_scatter_block;
    int scan;
    int exscan;
};


/**********************
 * COLLECTIVE OPTIONS *
 **********************/

struct _mpc_mpi_config_coll_opts
{
    int force_nocommute;

    /* Intra barrier */
    int barrier_intra_for_trsh;

    /* Intra reduce */
    int reduce_intra_count_trsh;
    int reduce_intra_for_trsh;

    /* Intra bcast */
    int bcast_intra_count_trsh;
    int bcast_intra_for_trsh;

    /* SHM */

    /* Reduce */
    int reduce_pipelined_blocks;
    long int reduce_pipelined_tresh;
    int reduce_interleave;

    /* Bcast */
    int bcast_interleave;

    /* TOPO */
    struct _mpc_mpi_config_topo_coll_opts topo;
};

/************************************
 * GENERAL MPI MODULE CONFIGURATION *
 ************************************/

struct _mpc_mpi_config
{
    int message_buffering;
    struct _mpc_mpi_config_nbc nbc;

    struct _mpc_mpi_config_coll_opts coll_opts;

    struct _mpc_mpi_config_coll_array coll_intercomm;
    struct _mpc_mpi_config_coll_array coll_intracomm_shm;
    struct _mpc_mpi_config_coll_array coll_intracomm_shared_node;
    struct _mpc_mpi_config_coll_array coll_intracomm;

    struct _mpc_mpi_config_coll_algorithm_array coll_algorithm_intracomm;
};

struct _mpc_mpi_config * _mpc_mpi_config(void);

void _mpc_mpi_config_init(void);
void _mpc_mpi_config_check();

#endif
