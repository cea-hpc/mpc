/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - PEPIN Thibaut thibaut.pepin@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */
#include <math.h>
#include <stddef.h>

#include "mpc_keywords.h"
#include "mpc_mpi.h"
#include "mpc_mpi_internal.h"
#include "mpi_conf.h"
#include "comm_lib.h"
#include "egreq_nbc.h"
#include "mpc_coll_weak.h"

#define SCHED_SIZE (sizeof(int))
#define BARRIER_SCHED_SIZE (sizeof(char))
#define COMM_SCHED_SIZE (sizeof(NBC_Fn_type) + sizeof(NBC_Args))
#define ROUND_SCHED_SIZE (SCHED_SIZE + BARRIER_SCHED_SIZE)
#define MAX_HARDWARE_LEVEL 8

#define SCHED_INFO_TOPO_COMM_ALLOWED 1

//  TODO:
//    error hanfling
//    config handling
//    add reduce_scatter(_block)_Allgather(v) algorithm, may be faster than reduce then scatter

#define ALLOW_TOPO_COMM 1
#define TOPO_MAX_LEVEL 1

/**
 * @brief This enables debugging in the MPC_COLL file
 *
 */
//#define MPC_COLL_ENABLE_DEBUG

#if defined(MPC_COLL_ENABLE_DEBUG) && defined(MPC_ENABLE_DEBUG_MESSAGES)
/* Make sure this is not un-noticed */
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");
INFO("MPC_COLL debugging is ENABLED this is incompatible with thread-based MPI");

/*
  The __debug_indentation variable is global as shared between threads on the SAME vp
  leading to underflow and underflows when used in thread-based but still
  usefull to debug the process-based collectives */

#define MPC_COLL_EXTRA_DEBUG_ENABLED
#endif

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  #define DBG_INDENT_LEN 20
  static char __debug_indentation[DBG_INDENT_LEN] = "\0";
#endif


static int __global_topo_allow = 0;

void _mpc_mpi_coll_allow_topological_comm() {
  __global_topo_allow = 1;
}

void _mpc_mpi_coll_forbid_topological_comm() {
  __global_topo_allow = 0;
}


/**
  \enum MPC_COLL_TYPE
  \brief Define the type of a collective communication operation.
  */
typedef enum MPC_COLL_TYPE {
  MPC_COLL_TYPE_BLOCKING,     /**< Blocking collective communication. */
  MPC_COLL_TYPE_NONBLOCKING,  /**< Non-blocking collective communication. */
  MPC_COLL_TYPE_PERSISTENT,   /**< Persistent collective communication. */
  MPC_COLL_TYPE_COUNT         /**< Simulated collective communication used to count the number of operations made, the necessary number of round and the needed temporary buffer. */
} MPC_COLL_TYPE;

/**
  \struct Sched_info
  \brief Hold schedule informations
  */
typedef struct {
  int pos;                                      /**< Current offset from the starting adress of the schedule. */

  int round_start_pos;                          /**< Offset from the starting adress of the schedule to the starting adress of the current round. */
  int round_comm_count;                         /**< Number of operations in the current round. */

  int round_count;                              /**< Number of rounds in the schedule. */
  int comm_count;                               /**< Number of operations in the schedule. */
  int alloc_size;                               /**< Allocation size for the schedule. */

  void *tmpbuf;                                 /**< Adress of the temporary buffer for the schedule. */
  size_t tmpbuf_size;                              /**< Size of the temporary buffer for the schedule. */
  size_t tmpbuf_pos;                               /**< Offset from the staring adress of the temporary buffer to the current position. */

  int flag;                                     /**< flag used to store initialisation states. */
  //int is_topo_comm_creation_allowed;
  //int is_hardware_algo;
  mpc_hardware_split_info_t *hardware_info_ptr; /**< ptr to structure for hardware communicator info */
} Sched_info;

#define pointer_swap(a, b, swap) \
{                                \
  (swap) = (a);                    \
  (a) = (b);                       \
  (b) = (swap);                    \
}

#ifndef RANK2VRANK
#define RANK2VRANK(rank, vrank, root) \
{ \
  (vrank) = (rank); \
  if ((rank) == 0) (vrank) = (root); \
  if ((rank) == (root)) (vrank) = 0; \
}
#endif

#ifndef VRANK2RANK
#define VRANK2RANK(rank, vrank, root) \
{ \
  (rank) = (vrank); \
  if ((vrank) == 0) (rank) = (root); \
  if ((vrank) == (root)) (rank) = 0; \
}
#endif


/**************
   COLLECTIVES PROTOTYPES
 **************/

static inline int ___collectives_ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int ___collectives_bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_bcast_scatter_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_bcast_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__collectives_bcast_topo_depth(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info, int shallowest_level);


static inline int ___collectives_ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int ___collectives_reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_reduce_scatter_allgather(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_topo_commute(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ___collectives_allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_vector_halving_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_ring(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allreduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int ___collectives_scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scatter_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int ___collectives_scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scatterv_binomial(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int ___collectives_gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_gather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__collectives_gather_topo_depth(void *tmp_sendbuf, int tmp_sendcount, MPI_Datatype tmp_sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info, int shallowest_level, void *tmpbuf);

static inline int ___collectives_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
int ___collectives_gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_gatherv_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ireduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ___collectives_reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_block_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_block_distance_halving(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ireduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ___collectives_reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_distance_doubling(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_distance_halving(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_reduce_scatter_pairwise(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int ___collectives_allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_topo_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgather_topo_gather_allgather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm);
int ___collectives_allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgatherv_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_allgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int ___collectives_alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoall_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
int ___collectives_alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallv_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallv_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm);
int ___collectives_alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallw_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_alltoallw_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ___collectives_scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ___collectives_exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


static inline int ___collectives_ibarrier (MPI_Comm comm, NBC_Handle *handle);
static inline int ___collectives_barrier_init (MPI_Comm comm, NBC_Handle* handle);
int _mpc_mpi_collectives_barrier (MPI_Comm comm);
int ___collectives_barrier_switch (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int ___collectives_barrier_reduce_broadcast (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


/**************
   OTHER PROTOTYPES
 **************/

static inline int ___collectives_rmb_index(int a);
static inline int ___collectives_fill_rmb(int a, int index);
static inline unsigned int ___collectives_floored_log2(unsigned int a);
static inline unsigned int ___collectives_ceiled_log2(unsigned int a);

static inline void ___collectives_sched_info_init(Sched_info *info, MPC_COLL_TYPE coll_type, int operation_allow_topo);
static inline int ___collectives_sched_alloc_init(NBC_Handle *handle, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_send(const void *buffer, int count, MPI_Datatype datatype, int dest, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_recv(void *buffer, int count, MPI_Datatype datatype, int source, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_barrier(NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_commit(NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_op(void *res_buf, void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_copy(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_sched_move(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info);

static inline int ___collectives_send_type(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_recv_type(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_barrier_type(MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_op_type( __UNUSED__ void *res_buf, const void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, sctk_Op mpc_op, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_copy_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);
static inline int ___collectives_move_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info);

static inline int ___collectives_create_hardware_comm_unguided(MPI_Comm comm, int vrank, int max_level, Sched_info *info);
static inline int ___collectives_create_master_hardware_comm_unguided(int vrank, int level, Sched_info *info);
static inline int ___collectives_create_childs_counts(MPI_Comm comm, Sched_info *info);
static inline int ___collectives_create_swap_array(MPI_Comm comm, Sched_info *info);
static inline int ___collectives_topo_comm_init(MPI_Comm comm, int root, int max_level, Sched_info *info);

static inline int __Get_topo_comm_allowed(MPC_COLL_TYPE coll_type, int operation_allow_topo);

/**
  \brief Find the index of the first bit set to 1 in an integer
  \param a Integer
  \return Index of the first bit set to 1 int the input integer
  */
static inline int ___collectives_rmb_index(int a) {
  if(a == 0) return -1;
  int i = 0;
  while((a & (1 << i)) != (1 << i)) i++;
  return i;
}

/**
  \brief Set all the index'th rightmost bit of an integer to 1
  \param a Input integer
  \param index
  \return
  */
static inline int ___collectives_fill_rmb(int a, int index) {
  return a | ((1 << index) - 1);
}

/**
  \brief Compute the floored log2 of an integer
  \param a Input integer
  \return
  */
static inline unsigned int ___collectives_floored_log2(unsigned int a) {

  unsigned int n = 0;

  while (a >>= 1) {
    ++n;
  }

  return n;
}

/**
  \brief Compute the ceiled log2 of an integer
  \param a Input integer
  \return
  */
static inline unsigned int ___collectives_ceiled_log2(unsigned int a) {

  unsigned int n = ___collectives_floored_log2(a);
  return (a != (1U << n))?(n+1):(n);
}

/**
  \brief Initialise the Sched_info struct with default values
  \param info The address of the Sched_info struct to initialise
  \param coll_type The collective operation type (blocking, non-blocking, persistent)
  \param operation_allow_topo Allow topological algorithms for a specific collective operation
  */
static inline void ___collectives_sched_info_init(Sched_info *info, MPC_COLL_TYPE coll_type, int operation_allow_topo) {
  info->pos = 2 * sizeof(int);

  info->round_start_pos = sizeof(int);
  info->round_comm_count = 0;

  info->round_count = 1;
  info->comm_count = 0;
  info->alloc_size = 0;

  info->tmpbuf_size = 0;
  info->tmpbuf_pos = 0;

  info->flag = 0;
  if(__Get_topo_comm_allowed(coll_type, operation_allow_topo)) {
    info->flag |= SCHED_INFO_TOPO_COMM_ALLOWED;
  }

  info->hardware_info_ptr = NULL;
}

/**
  \brief Handle the allocation of the schedule and the temporary buffer
  \param handle Handle containing the temporary buffer to allocate
  \param schedule Adress of the schedule to allocate
  \param info Information about the function to schedule
  \return Error code
  */
static inline int ___collectives_sched_alloc_init(NBC_Handle *handle, NBC_Schedule *schedule, Sched_info *info) {

  info->alloc_size = SCHED_SIZE + info->round_count * ROUND_SCHED_SIZE + info->comm_count * COMM_SCHED_SIZE;

  *schedule = sctk_malloc(info->alloc_size);
  if (*schedule == NULL) {
    return NBC_OOR;
  }

  if(info->tmpbuf_size != 0) {
    handle->tmpbuf = sctk_malloc(info->tmpbuf_size);
    if (handle->tmpbuf == NULL) {
      return NBC_OOR;
    }

    info->tmpbuf = handle->tmpbuf;
  }

  return MPI_SUCCESS;
}



/**
  \brief Add a Send in the schedule
  \param buffer Adress of the buffer used to send data
  \param count Number of element in the buffer
  \param datatype Type of the data elements in the buffer
  \param dest Target rank
  \param comm Target communicator
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_send(const void *buffer, int count, MPI_Datatype datatype, int dest, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* send_args = NULL;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = SEND;

  /* store the passed arguments */
  send_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));

  send_args->buf= (void *)buffer;
  send_args->tmpbuf=0;
  send_args->count=count;
  send_args->datatype=datatype;
  send_args->dest=dest;
  send_args->comm = comm;

  /* move pos forward */
  info->pos += COMM_SCHED_SIZE;

  info->round_comm_count += 1;

  return MPI_SUCCESS;
}

/**
  \brief Add a Recv in the schedule
  \param buffer Adress of the buffer used to recv data
  \param count Number of element in the buffer
  \param datatype Type of the data elements in the buffer
  \param dest Target rank
  \param comm Target communicator
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_recv(void *buffer, int count, MPI_Datatype datatype, int source, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* recv_args = NULL;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = RECV;

  /* store the passed arguments */
  recv_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));

  recv_args->buf=buffer;
  recv_args->tmpbuf=0;
  recv_args->count=count;
  recv_args->datatype=datatype;
  recv_args->source=source;
  recv_args->comm=comm;

  /* move pos forward */
  info->pos += COMM_SCHED_SIZE;

  info->round_comm_count += 1;

  return MPI_SUCCESS;
}

/**
  \brief Add a Barrier in the schedule
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_barrier(NBC_Schedule *schedule, Sched_info *info) {
  *((char *)*schedule + info->pos) = 1;

  info->pos += BARRIER_SCHED_SIZE;

  *(int *)((char *)*schedule + info->round_start_pos) = info->round_comm_count;

  info->round_comm_count = 0;
  info->round_start_pos = info->pos;

  info->pos += SCHED_SIZE;

  return MPI_SUCCESS;
}

/**
  \brief Commit a schedule
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_commit(NBC_Schedule *schedule, Sched_info *info) {

  *(int *)((char*)*schedule) = info->alloc_size;
  *(int *)((char *)*schedule + info->round_start_pos) = info->round_comm_count;

  /* add the barrier char (0) because this is the last round */
  *((char *)*schedule + info->pos) = 0;

  return MPI_SUCCESS;
}

/**
  \brief Add a the reduction between two elements in the schedule
  \param resbuf Adress of the buffer used to store the result
  \param left_op_buf Adress of the buffer used as left operand
  \param right_op_buf Adress of the buffer used as right operand
  \param count Number of element in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_op(void *res_buf, void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* op_args = NULL;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = OP;

  /* store the passed arguments */
  op_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));

  op_args->buf1=left_op_buf;
  op_args->buf2=right_op_buf;
  op_args->buf3=res_buf;
  op_args->tmpbuf1=0;
  op_args->tmpbuf2=0;
  op_args->tmpbuf3=0;
  op_args->count=count;
  op_args->op=op;
  op_args->datatype=datatype;

  /* move pos forward */
  info->pos += COMM_SCHED_SIZE;

  info->round_comm_count += 1;

  return MPI_SUCCESS;
}

/**
  \brief Add a Copy in the schedule
  \param src Adress of the source buffer used in the copy
  \param srccount Number of element in the source buffer
  \param srctype Type of the data elements in the source buffer
  \param tgt Adress of the destination buffer used in the copy
  \param tgtcount Number of element in the destination buffer
  \param tgttype Type of the data elements in the destination buffer
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_copy(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args *copy_args = NULL;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = COPY;

  /* store the passed arguments */
  copy_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));
  copy_args->src = (void*)src;
  copy_args->tmpsrc = 0;
  copy_args->srccount = srccount;
  copy_args->srctype = srctype;
  copy_args->tgt = tgt;
  copy_args->tmptgt = 0;
  copy_args->tgtcount = tgtcount;
  copy_args->tgttype = tgttype;

  /* move pos forward */
  info->pos += COMM_SCHED_SIZE;

  info->round_comm_count += 1;

  return MPI_SUCCESS;
}

/**
  \brief Add a Move in the schedule
  \param src Adress of the source buffer used in the move
  \param srccount Number of element in the source buffer
  \param srctype Type of the data elements in the source buffer
  \param tgt Adress of the destination buffer used in the move
  \param tgtcount Number of element in the destination buffer
  \param tgttype Type of the data elements in the destination buffer
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_sched_move(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args *copy_args = NULL;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = MOVE;

  /* store the passed arguments */
  copy_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));
  copy_args->src = (void *)src;
  copy_args->tmpsrc = 0;
  copy_args->srccount = srccount;
  copy_args->srctype = srctype;
  copy_args->tgt = tgt;
  copy_args->tmptgt = 0;
  copy_args->tgtcount = tgtcount;
  copy_args->tgttype = tgttype;

  /* move pos forward */
  info->pos += COMM_SCHED_SIZE;

  info->round_comm_count += 1;

  return MPI_SUCCESS;
}



/**
  \brief Switch between blocking, non-blocking and persitent send
    Or increase the number of operation of the schedule in the Sched_info struct
  \param buffer Adress of the buffer used to send data
  \param count Number of element in the buffer
  \param datatype Type of the data elements in the buffer
  \param dest Target rank
  \param tag Tag of the communication
  \param comm Target communicator
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_send_type(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    int peer_global_rank;
    MPI_Group local_group, global_group;
    MPI_Comm_group(comm, &local_group);
    MPI_Comm_group(MPI_COMM_WORLD, &global_group);
    MPI_Group_translate_ranks(local_group, 1, &dest, global_group, &peer_global_rank);
    mpc_common_debug("%sSEND | % 4d (% 4d) -> % 4d (% 4d) (count=%d) : %p | COMM %p", __debug_indentation, rank, global_rank, dest, peer_global_rank, count, buffer, comm);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_send(buffer, count, datatype, dest, tag, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = ___collectives_sched_send(buffer, count, datatype, dest, comm, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}

/**
  \brief Switch between blocking, non-blocking and persitent recv
    Or increase the number of operation of the schedule in the Sched_info struct
  \param buffer Adress of the buffer used to recv data
  \param count Number of element in the buffer
  \param datatype Type of the data elements in the buffer
  \param dest Target rank
  \param tag Tag of the communication
  \param comm Target communicator
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_recv_type(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    int peer_global_rank;
    MPI_Group local_group, global_group;
    MPI_Comm_group(comm, &local_group);
    MPI_Comm_group(MPI_COMM_WORLD, &global_group);
    MPI_Group_translate_ranks(local_group, 1, &source, global_group, &peer_global_rank);
    mpc_common_debug("%sRECV | % 4d (% 4d) <- % 4d (% 4d) (count=%d) : %p | COMM %p", __debug_indentation, rank, global_rank, source, peer_global_rank, count, buffer, comm);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_recv(buffer, count, datatype, source, tag, comm, MPI_STATUS_IGNORE);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = ___collectives_sched_recv(buffer, count, datatype, source, comm, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}

/**
  \brief Switch between blocking, non-blocking and persitent barrier
    Or increase the number of round of the schedule in the Sched_info struct
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_barrier_type(MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &rank);
    mpc_common_debug("%sBARR |      % 4d", __debug_indentation, rank);
  }
#endif


  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = ___collectives_sched_barrier(schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->round_count += 1;
      break;
  }
  return res;
}

/**
  \brief Switch between blocking, non-blocking and persitent reduction operation
    Or increase the number of operation of the schedule in the Sched_info struct
  \param resbuf Adress of the buffer used to store the result
  \param left_op_buf Adress of the buffer used as left operand
  \param right_op_buf Adress of the buffer used as right operand
  \param count Number of element in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param mpc_op sctk_op associated to op
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_op_type( __UNUSED__ void *res_buf, const void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, sctk_Op mpc_op, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &rank);
    mpc_common_debug("%sOP   |      % 4d", __debug_indentation, rank);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(mpc_op.u_func != NULL)
      {
        mpc_op.u_func((void*)left_op_buf, right_op_buf, &count, &datatype);
        mpc_common_debug("COLL: op done.");
      }
      else
      {
        sctk_Op_f func = NULL;
        func = sctk_get_common_function(datatype, mpc_op);
        func(left_op_buf, right_op_buf, count, datatype);
      }
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = ___collectives_sched_op(right_op_buf, (void*)left_op_buf, right_op_buf, count, datatype, op, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}

/**
  \brief Switch between blocking, non-blocking and persitent Copy
    Or increase the number of operation of the schedule in the Sched_info struct
  \param src Adress of the source buffer used in the copy
  \param srccount Number of element in the source buffer
  \param srctype Type of the data elements in the source buffer
  \param tgt Adress of the destination buffer used in the copy
  \param tgtcount Number of element in the destination buffer
  \param tgttype Type of the data elements in the destination buffer
  \param comm Target communicator
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_copy_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sCOPY | % 4d (% 4d) : %p (COUNT %d) -> %p (COUNT %d)", __debug_indentation, rank, global_rank, src, srccount, tgt, tgtcount);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      NBC_Copy(src, srccount, srctype, tgt, tgtcount, tgttype, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      ___collectives_sched_copy(src, srccount, srctype, tgt, tgtcount, tgttype, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}

/**
  \brief Switch between blocking, non-blocking and persitent Move
    Or increase the number of operation of the schedule in the Sched_info struct
  \param src Adress of the source buffer used in the move
  \param srccount Number of element in the source buffer
  \param srctype Type of the data elements in the source buffer
  \param tgt Adress of the destination buffer used in the move
  \param tgtcount Number of element in the destination buffer
  \param tgttype Type of the data elements in the destination buffer
  \param comm Target communicator
  \param coll_type Type of the communication used for the switch
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return Error code
  */
static inline int ___collectives_move_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sMOVE | % 4d (% 4d) : %p (COUNT %d) -> %p (COUNT %d)", __debug_indentation, rank, global_rank, src, srccount, tgt, tgtcount);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      NBC_Move(src, srccount, srctype, tgt, tgtcount, tgttype, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      ___collectives_sched_move(src, srccount, srctype, tgt, tgtcount, tgttype, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}



/**
  \brief Initialize hardware communicators used for persistent hardware algorithms
  \param comm communicator of the collective
  \param vrank virtual rank for the algorithm
  \param max_level Max hardware topological level to split communicators
  \param info Adress on the information structure about the schedule
  \return error code
 */
static inline int ___collectives_create_hardware_comm_unguided(MPI_Comm comm, int vrank, int max_level, Sched_info *info) {
  int res = MPI_ERR_INTERN;
  int level_num = 0;

  MPI_Comm* hwcomm = info->hardware_info_ptr->hwcomm;

  hwcomm[level_num] = comm;

  int rank = 0;
  int size = 0;
  _mpc_cl_comm_rank(comm, &rank);
  _mpc_cl_comm_size(comm, &size);

  /* create topology communicators */
  while(/*(hwcomm[level_num] != MPI_COMM_NULL) &&*/ (level_num < max_level))
  {
    res = PMPI_Comm_split_type(hwcomm[level_num],
        MPI_COMM_TYPE_HW_UNGUIDED,
        vrank,
        MPI_INFO_NULL,
        &hwcomm[level_num+1]);

    if(hwcomm[level_num+1] != MPI_COMM_NULL)
    {
      _mpc_cl_comm_rank(hwcomm[level_num + 1],&vrank);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
      _mpc_cl_comm_rank(hwcomm[level_num + 1], &tmp_rank);
      _mpc_cl_comm_size(hwcomm[level_num + 1], &tmp_size);
      mpc_common_debug("%sRANK %d / %d | SPLIT COMM | SUBRANK %d / %d", __debug_indentation, rank, size, tmp_rank, tmp_size);
#endif

      level_num++;
    } else {
      break;
    }
  }

  info->hardware_info_ptr->deepest_hardware_level = level_num;

  return res;
}

/**
  \brief Initialize hardware master communicators used for persistent hardware algorithms
  \param vrank virtual rank for the algorithm
  \param level Max hardware topological level to split communicators
  \param info Adress on the information structure about the schedule
  \return error code
 */
static inline int ___collectives_create_master_hardware_comm_unguided(int vrank, __UNUSED__ int level, Sched_info *info) {
  int res = MPI_ERR_INTERN;
  int rank_comm = -1;

  MPI_Comm* hwcomm = info->hardware_info_ptr->hwcomm;
  MPI_Comm* rootcomm = info->hardware_info_ptr->rootcomm;

  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;
  int highest_level = deepest_level+1;
  //int highest_level = 0;

  int rank = 0;
  int size = 0;
  _mpc_cl_comm_rank(hwcomm[0], &rank);
  _mpc_cl_comm_size(hwcomm[0], &size);


  int i = 0;
  //for(i = deepest_level - 1; i >= 0; i--) {
  for(i = 0; i < deepest_level; i++) {
    _mpc_cl_comm_rank(hwcomm[i+1],&rank_comm);

    int color = 0;
    if(rank_comm == 0) {
      color = 0;
    } else {
      color = MPI_UNDEFINED;
    }


    PMPI_Comm_split(hwcomm[i], color, vrank, &rootcomm[i]);

    if(color == 0) {
      // highest_level = deepest_level - i;
      // //highest_level = i;
      // _mpc_cl_comm_rank(rootcomm[i], &tmp_rank);
      // if(tmp_rank == 0) {
      //   highest_level--;
      //   //highest_level++;
      // }
      highest_level--;
    }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
    if(color == 0) {
      _mpc_cl_comm_rank(rootcomm[i], &tmp_rank);
      _mpc_cl_comm_size(rootcomm[i], &tmp_size);
      mpc_common_debug("%sRANK %d / %d | SPLIT ROOT | SUBRANK %d / %d", __debug_indentation, rank, size, tmp_rank, tmp_size);
    }
#endif
  }

  if(rootcomm[0] != MPI_COMM_NULL) {
    int tmp = 0;
    _mpc_cl_comm_rank(rootcomm[0], &tmp);
    if(tmp == 0 ){
      highest_level--;
    }
  }

  info->hardware_info_ptr->highest_local_hardware_level = highest_level;

  return res;
}

/**
  \brief Create the array containing the number of processes bellow in the topological communicator tree for each height
  \param comm Target communicator
  \param info Adress on the information structure about the schedule
  \return error code
 */
static inline int ___collectives_create_childs_counts(MPI_Comm comm, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  mpc_common_debug("%sTOPO COMM INIT CHILDS COUNT", __debug_indentation);
  strcat(__debug_indentation, "\t");
#endif

  int rank = 0;
  _mpc_cl_comm_rank(comm, &rank);

  int data_count = 0;

  //info->hardware_info_ptr->childs_data_count = sctk_malloc(sizeof(int*) * (info->hardware_info_ptr->deepest_hardware_level));
  info->hardware_info_ptr->childs_data_count = sctk_malloc(sizeof(int*) * (info->hardware_info_ptr->deepest_hardware_level + 1));
  info->hardware_info_ptr->send_data_count = sctk_malloc(sizeof(int) * (info->hardware_info_ptr->deepest_hardware_level));

  //TODO a remplacer par un allreduce pour gerer le cas du gatherv
  _mpc_cl_comm_size(info->hardware_info_ptr->hwcomm[info->hardware_info_ptr->deepest_hardware_level], &data_count);
  int i = 0;

  info->hardware_info_ptr->childs_data_count[info->hardware_info_ptr->deepest_hardware_level] = sctk_malloc(sizeof(int) * data_count);
  for(i = 0; i < data_count; i++) {
    info->hardware_info_ptr->childs_data_count[info->hardware_info_ptr->deepest_hardware_level][i] = 1;
  }

  for(i = info->hardware_info_ptr->deepest_hardware_level - 1; i >= 0; i--) {

    int rank_split = 0;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

    if(rank_split == 0) {

      info->hardware_info_ptr->send_data_count[i] = data_count;
#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
      mpc_common_debug("%sRANK %d | SEND DATA COUNT [%d] = %d", __debug_indentation, rank, i, data_count);
#endif

      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int rank_master = 0;
      int size_master = 0;
      _mpc_cl_comm_rank(master_comm, &rank_master);
      _mpc_cl_comm_size(master_comm, &size_master);

       void *buf = NULL;
       if(rank_master == 0 || i == 0) {
         info->hardware_info_ptr->childs_data_count[i] = sctk_malloc(sizeof(int) * size_master);
         buf = info->hardware_info_ptr->childs_data_count[i];
       }

      if (i == 0) { // Every top-level process receives data_count (used for top-level allgather, alltoall, ...)
        _mpc_mpi_config()->coll_algorithm_intracomm.allgather(&data_count, 1, MPI_INT, buf, 1, MPI_INT, info->hardware_info_ptr->rootcomm[i], MPC_COLL_TYPE_BLOCKING, NULL, info);
      } else {
        _mpc_mpi_config()->coll_algorithm_intracomm.gather(&data_count, 1, MPI_INT, buf, 1, MPI_INT, 0, info->hardware_info_ptr->rootcomm[i], MPC_COLL_TYPE_BLOCKING, NULL, info);
      }
      int j = 0;

      if(rank_master == 0 || i == 0) {
        data_count = 0;
        for(j = 0; j < size_master; j++) {
          data_count += info->hardware_info_ptr->childs_data_count[i][j];
          mpc_common_nodebug("RANK %d | CHILD DATA COUNT [%d] [%d] = %d\n", rank, i, j, info->hardware_info_ptr->childs_data_count[i][j]);
        }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
        char dbg_str[1024];
        sprintf(dbg_str, "RANK %d | CHILD DATA COUNT [%d] = [%d", rank, i, info->hardware_info_ptr->childs_data_count[i][0]);
        for(j = 1; j < size_master; j++) {
          sprintf(&(dbg_str[strlen(dbg_str)]), ", %d", info->hardware_info_ptr->childs_data_count[i][j]);
        }
        mpc_common_debug("%s%s", __debug_indentation, dbg_str);
#endif

      } else {
        break;
      }

    } else {
      break;
    }
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return MPI_SUCCESS;
}

/**
  \brief Create the array linking the position of processes in the topology and their rank
         on top-level processes
  \param comm Target communicator
  \param root Rank of the root of the collective
  \param info Adress on the information structure about the schedule
  \return error code
 */
static inline int ___collectives_create_swap_array(MPI_Comm comm, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  mpc_common_debug("%sTOPO COMM INIT SWAP ARRAY", __debug_indentation);
  strcat(__debug_indentation, "\t");
#endif

  int rank = 0;
  int size = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int tmp_swap_array[size];

  int *swap_array = (info->hardware_info_ptr->swap_array = sctk_malloc(sizeof(int) * size));
  int *reverse_swap_array = (info->hardware_info_ptr->reverse_swap_array = sctk_malloc(sizeof(int) * size));

  for(int i = 0; i < size; i++) {
    swap_array[i] = i;
  }

  ___collectives_allgather_topo_gather_allgather_broadcast(&rank, 1, MPI_INT,
    tmp_swap_array, 1, MPI_INT, comm, MPC_COLL_TYPE_BLOCKING, NULL, info);

  memcpy(swap_array, tmp_swap_array, size * sizeof(int));

  for(int i = 0; i < size; i++) {
    if(swap_array[i] == rank) {
      info->hardware_info_ptr->topo_rank = i;
    }
    reverse_swap_array[swap_array[i]] = i;
  }



#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  char dbg_str[1024];

  sprintf(dbg_str, "RANK %d | SWAP ARRAY = [%d", rank, info->hardware_info_ptr->swap_array[0]);
  for(int j = 1; j < size; j++) {
    sprintf(&(dbg_str[strlen(dbg_str)]), ", %d", info->hardware_info_ptr->swap_array[j]);
  }

  sprintf(&(dbg_str[strlen(dbg_str)]), "]\n%s         REVERSE SWAP ARRAY = [%d", __debug_indentation, info->hardware_info_ptr->reverse_swap_array[0]);
  for(int j = 1; j < size; j++) {
    sprintf(&(dbg_str[strlen(dbg_str)]), ", %d", info->hardware_info_ptr->reverse_swap_array[j]);
  }

  mpc_common_debug("%s%s]", __debug_indentation, dbg_str);


  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return MPI_SUCCESS;
}

/**
  \brief Initialise topological informations and create topological communicators
  \param comm Target communicator
  \param root Rank of the root of the collective
  \param level Max hardware topological level to split communicators
  \param info Adress on the information structure about the schedule
  \return error code
 */
static inline int ___collectives_topo_comm_init(MPI_Comm comm, int root, int max_level, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  mpc_common_debug("%sTOPO COMM INIT", __debug_indentation);
  strcat(__debug_indentation, "\t");
#endif

  int rank = 0;
  int size = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int vrank = 0;
  RANK2VRANK(rank, vrank, root);

  info->hardware_info_ptr = (mpc_hardware_split_info_t *)sctk_malloc(sizeof(mpc_hardware_split_info_t));
  mpc_lowcomm_topo_comm_set(comm, root, info->hardware_info_ptr);

  info->hardware_info_ptr->hwcomm = (MPI_Comm *)sctk_malloc(MAX_HARDWARE_LEVEL * sizeof(MPI_Comm));
  info->hardware_info_ptr->rootcomm = (MPI_Comm *)sctk_malloc(MAX_HARDWARE_LEVEL * sizeof(MPI_Comm));

  info->hardware_info_ptr->childs_data_count = NULL;
  info->hardware_info_ptr->send_data_count = NULL;

  info->hardware_info_ptr->topo_rank = -1;
  info->hardware_info_ptr->swap_array = NULL;
  info->hardware_info_ptr->reverse_swap_array = NULL;

  /* Create hardware communicators using unguided topological split and max split level */
  ___collectives_create_hardware_comm_unguided(comm, vrank, max_level, info);
  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;
  ___collectives_create_master_hardware_comm_unguided(vrank, deepest_level, info);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return MPI_SUCCESS;
}

static inline int __Get_topo_comm_allowed(MPC_COLL_TYPE coll_type, int operation_allow_topo) {
  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      return operation_allow_topo || _mpc_mpi_config()->coll_opts.topo.blocking;
    case MPC_COLL_TYPE_NONBLOCKING:
      return operation_allow_topo || _mpc_mpi_config()->coll_opts.topo.non_blocking;
    case MPC_COLL_TYPE_PERSISTENT:
      return operation_allow_topo || _mpc_mpi_config()->coll_opts.topo.persistent;
    default:
      mpc_common_debug_error("Unsupported coll_type provided (coll_type = %d) %s:%s:%d", coll_type, __func__, __FILE__, __LINE__);
      return 0;
  }
}




/***********
  BROADCAST
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Broadcast
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in the buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ibcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request) {

  if( _mpc_mpi_config()->nbc.use_egreq_bcast )
  {
    return MPI_Ixbcast( buffer, count, datatype, root, comm, request );
  }


  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IBCAST %d with count %d", comm, count);
  SCTK__MPI_INIT_REQUEST (request);
  int csize = 0;
  MPI_Comm_size(comm, &csize);
  if(csize == 1)
  {
    res = PMPI_Bcast (buffer, count, datatype, root, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    //tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_ibcast(buffer, count, datatype, root, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Broadcast
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ibcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle ) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.bcast);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ibcast: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Broadcast
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in the buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size = 0;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);
    if(count != 0)
    {
      mpi_check_buf(buffer, comm);
    }

  }

  MPI_internal_request_t *req = NULL;
  mpc_lowcomm_request_t *mpc_req = NULL;
  SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req = *request = mpc_lowcomm_request_alloc();
  mpc_req = _mpc_cl_get_lowcomm_request(req);
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_BCAST_INIT;
  req->persistant.info = info;

  ___collectives_bcast_init(buffer, count, datatype, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Broadcast
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle){
  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.bcast);
  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Bcast: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking bcast
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.bcast);
  return _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}

/**
  \brief Swith between the different broadcast algorithms
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sBCAST | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strcat(__debug_indentation, "\t");
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_BCAST_LINEAR,
    NBC_BCAST_BINOMIAL,
    NBC_BCAST_SCATTER_ALLGATHER,
    NBC_BCAST_TOPO
  } alg;


  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root);
    topo = 1;
  }

  if(topo) {
    alg = NBC_BCAST_TOPO;
  } else {
    //alg = NBC_BCAST_BINOMIAL;
    if(size <= 4 || (size <= 16 && ext * count > 2 << 10)) {
      alg = NBC_BCAST_LINEAR;
    } else {
      alg = NBC_BCAST_BINOMIAL;
    }
  }

  int res = 0;

  switch(alg) {
    case NBC_BCAST_LINEAR:
      res = ___collectives_bcast_linear(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_BINOMIAL:
      res = ___collectives_bcast_binomial(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_SCATTER_ALLGATHER:
      res = ___collectives_bcast_scatter_allgather(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_TOPO:
      res = ___collectives_bcast_topo(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a broadcast using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root) {
        info->comm_count += size - 1;
      } else {
        info->comm_count += 1;
      }

      return MPI_SUCCESS;
  }

  // if rank != root, wait for data from root
  if(rank != root) {
    res = ___collectives_recv_type(buffer, count, datatype, root, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    //if rank == root, send data to each rank
      int i = 0;
      for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = ___collectives_send_type(buffer, count, datatype, i, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }
  }

  return res;
}

/**
  \brief Execute or schedule a broadcast using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size = 0;
  int rank = 0;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  // Get max number of steps
  int maxr, vrank, peer;
  maxr = ___collectives_ceiled_log2(size);
  // Get virtual rank for processes by swappping rank 0 & root
  RANK2VRANK(rank, vrank, root);
  // Get the index of the right-most bit set to 1 in rank
  // The max number of sends made is equal to rmb
  int rmb = ___collectives_rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  // Wait for data
  if(rank != root) {
    VRANK2RANK(peer, vrank ^ (1 << rmb), root);

    ___collectives_recv_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

    if(rmb != 0) {
      ___collectives_barrier_type(coll_type, schedule, info);
    }
  }

  // Distribute data to rank+2^(rmb-1); ...; rank+2^0
  // Ex : root=0, size=8
  // ranks (rank_base2) : 0(000) 1(001) 2(010) 3(011) 4(100) 5(101) 6(110) 7(111)
  // (step) : sends: a -> b, ...
  // (0) : 0(000) -> 4(100)
  // (1) : 0(000) -> 2(010)
  //       4(100) -> 6(110)
  // (2) : 0(000) -> 1(001)
  //       2(010) -> 3(011)
  //       4(100) -> 5(101)
  //       6(110) -> 7(111)
  int i;
  for(i = rmb - 1; i >= 0; i--) {
    VRANK2RANK(peer, vrank | (1 << i), root);
    if(peer >= size) {
      continue;
    }

    ___collectives_send_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a broadcast using the scatter_allgather algorithm
    Or count the number of operations and rounds for the schedule
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_bcast_scatter_allgather(__UNUSED__ void *buffer, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a broadcast using the topological algorithm at levels [shallowest_level, deepest_hardware_level]
  \param buffer Address of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast (UNUSED)
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \param shallowest_level The topogical level at which bcast is started
  \return error code
  */
int __INTERNAL__collectives_bcast_topo_depth(void *buffer, int count, MPI_Datatype datatype, __UNUSED__ int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info,
                                             int shallowest_level) {

  int rank, size, res = MPI_SUCCESS;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;

  /* At each topological level, masters do binomial algorithm */
  int k;
  for (k = shallowest_level; k < deepest_level; k++) {
    int rank_split;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[k + 1], &rank_split);

    if(!rank_split) { /* if master */
      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[k];

      res = _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, 0, master_comm, coll_type, schedule, info);
      if (res != MPI_SUCCESS)
      {
      	  mpc_common_debug_error("Bcast topo: Intracomm bcast failed on level %d", k);
      }
      MPI_HANDLE_ERROR(res, comm, "Bcast topo: Intracomm bcast failed");

      ___collectives_barrier_type(coll_type, schedule, info);
    }
  }

  /* last level topology binomial bcast */
  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];
  res = _mpc_mpi_config()->coll_algorithm_intracomm.bcast(buffer, count, datatype, 0, hardware_comm, coll_type, schedule, info);

  return res;
}

/**
  \brief Execute or schedule a broadcast using the topological algorithm
    Or count the number of operations and rounds for the schedule
  \param buffer Adress of the buffer used to send/recv data during the broadcast
  \param count Number of elements in buffer
  \param datatype Type of the data elements in the buffer
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_bcast_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  int rank, size, res = MPI_SUCCESS;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
    case MPC_COLL_TYPE_COUNT:
      break;
  }

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(initial_flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "bcast");
      int res = ___collectives_bcast_switch(buffer, count, datatype, root, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  int shallowest_level = 0;

  res = __INTERNAL__collectives_bcast_topo_depth(buffer, count, datatype, root, comm, coll_type, schedule, info, shallowest_level);
  info->flag = initial_flag;

  return res;
}



/***********
  REDUCE
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Reduce
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ireduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request) {

  if( _mpc_mpi_config()->nbc.use_egreq_reduce )
  {
    return MPI_Ixreduce( sendbuf, recvbuf, count, datatype, op, root, comm, request);
  }

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IREDUCE %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize,rank;
  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm, &csize);
  if(recvbuf == sendbuf && rank == root)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root,
        comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    //tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Reduce
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ireduce: NBC_Init_handle failed");
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Reduce
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, __UNUSED__ MPI_Info info, MPI_Request *request) {

  {
    int size;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);
    if(count != 0)
    {
      mpi_check_buf(recvbuf, comm);
      mpi_check_buf(sendbuf, comm);
    }

  }
  MPI_internal_request_t *req;
  mpc_lowcomm_request_t *mpc_req;
  SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req = mpc_lowcomm_request_alloc();
  mpc_req = _mpc_cl_get_lowcomm_request(req);
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_reduce_init (sendbuf, recvbuf, count, datatype, op, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Reduce
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.reduce);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Reduce: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking reduce
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce);
  return _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different reduce algorithms
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sREDUCE | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation,"\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_REDUCE_LINEAR,
    NBC_REDUCE_BINOMIAL,
    NBC_REDUCE_REDUCE_SCATTER_ALLGATHER,
    NBC_REDUCE_TOPO,
    NBC_REDUCE_TOPO_COMMUTE
  } alg;


  int topo = 0;

  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root);
    topo = 1;
  }

  if(topo) {
    if(sctk_op_can_commute(sctk_convert_to_mpc_op(op), datatype)) {
      alg = NBC_REDUCE_TOPO_COMMUTE;
    } else {
      alg = NBC_REDUCE_TOPO;
    }
  } else {
    //alg = NBC_REDUCE_BINOMIAL;
    if(size <= 16) {
      alg = NBC_REDUCE_LINEAR;
    } else {
      alg = NBC_REDUCE_BINOMIAL;
    }
  }

  int res;

  switch(alg) {
    case NBC_REDUCE_LINEAR:
      res = ___collectives_reduce_linear(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_BINOMIAL:
      res = ___collectives_reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_REDUCE_SCATTER_ALLGATHER:
      res = ___collectives_reduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_TOPO:
      res = ___collectives_reduce_topo(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_TOPO_COMMUTE:
      res = ___collectives_reduce_topo_commute(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a reduce using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  int res = MPI_SUCCESS;

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(rank == root && size != 1) {
        mpi_op = sctk_convert_to_mpc_op(op);
        mpc_op = mpi_op->op;
        tmpbuf = sctk_malloc(ext * count * size);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(rank == root && size != 1) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += ext * count * size;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root && size != 1) {
        info->tmpbuf_size += ext * count * size;
      }
      break;
  }

  if(size == 1) {
    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
    }
    return MPI_SUCCESS;
  }



  // Gather data from all ranks
  if(sendbuf != MPI_IN_PLACE) {
    _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  } else {
    _mpc_mpi_config()->coll_algorithm_intracomm.gather(recvbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  }

  // Reduce received data
  if(rank == root) {
    ___collectives_barrier_type(coll_type, schedule, info);
    int i;
    for(i = 1; i < size; i++) {
      ___collectives_op_type(NULL, (char*)tmpbuf + (i-1)*ext*count, (char*)tmpbuf + i*ext*count, count, datatype, op, mpc_op, coll_type, schedule, info);
    }

    ___collectives_copy_type((char*)tmpbuf + (size-1)*ext*count, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING && rank == root) {
    sctk_free(tmpbuf);
  }

  return res;
}

/**
  \brief Execute or schedule a reduce using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  mpi_op = sctk_convert_to_mpc_op(op);
  mpc_op = mpi_op->op;

  int can_commute = sctk_op_can_commute(mpi_op, datatype);
  int first_access = 1;

  // We cant use virtual rank if the operation isn't commutative
  // As RANK2VRANK swap rank 0 & rank root, vroot prevent the switch if necessary
  int vrank, vroot, peer, maxr;
  vroot = root;
  if(!can_commute) {
    vroot = 0;
  }

  // Get virtual rank for processes by swappping rank 0 & vroot
  RANK2VRANK(rank, vrank, vroot);

  maxr = ___collectives_ceiled_log2(size);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(size != 1) {
        tmpbuf = sctk_malloc(2 * ext * count);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(size != 1) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += 2 * ext * count;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(size != 1) {
        info->tmpbuf_size += 2 * ext * count;
      }
      break;
  }

  if(size == 1) {
    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
    }
    return MPI_SUCCESS;
  }

  void *tmp_recvbuf = NULL;
  void *tmp_sendbuf = NULL;
  void *swap = NULL;
  tmp_recvbuf = tmpbuf;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  } else {
    tmp_sendbuf = (void*)sendbuf;
  }
  int i;
  for(i = 0; i < maxr; i++) {
    int vpeer = vrank ^ (1 << i);
    VRANK2RANK(peer, vpeer, vroot);

    if(vpeer < vrank) {
      // send reduce data to rank-2^i
      ___collectives_send_type(tmp_sendbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      break;

    } else if(peer < size) {
      // wait data from rank+2^i
      ___collectives_recv_type(tmp_recvbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);
      // reduce data
      ___collectives_op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

      if(first_access) {
        tmp_sendbuf = tmpbuf + ext * count;
        first_access = 0;
      }

      // As we receive data from higher rank & the Operation put the result in the right input
      // We must swap the send & recv pointers to respect commutativity
      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    }
  }

  // if the operation isnt commutative rank 0 send the reduced data to root
  if(!can_commute && root != 0) {
    if(rank == 0) {
      ___collectives_send_type(tmp_sendbuf, count, datatype, root, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    } else if(rank == root) {
      ___collectives_recv_type(recvbuf, count, datatype, 0, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  } else if (rank == root) {
    // Root already have the result, just copy it in revcbuf
    ___collectives_copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }



  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the reduce_scatter allgather algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_reduce_scatter_allgather(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the topological algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size, res = MPI_SUCCESS;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  MPI_Type_extent(datatype, &ext);

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  void *tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  mpi_op = sctk_convert_to_mpc_op(op);
  mpc_op = mpi_op->op;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(rank == root) {
        tmpbuf = sctk_malloc(size * count * ext);
      }
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(rank == root) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += size * count * ext;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      {
        if(rank == root) {
          info->tmpbuf_size += size * count * ext;
        }
        break;
      }
  }

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(info->flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "reduce");
      int res = ___collectives_reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  void *tmp_sendbuf = (void *)sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  }

  ___collectives_gather_topo(tmp_sendbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);

  if(rank == root) {
    int i;
    for(i = 1; i < size; i++) {
      void *leftbuf = tmpbuf + (i-1) * count * ext;
      void *rightbuf = tmpbuf + i * count * ext;

      ___collectives_op_type(NULL, leftbuf, rightbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      //___collectives_barrier_type(coll_type, schedule, info);
    }

    ___collectives_copy_type(tmpbuf + (size-1) * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  info->flag = initial_flag;

  if(coll_type == MPC_COLL_TYPE_BLOCKING && rank == root) {
    sctk_free(tmpbuf);
  }

  return res;
}

/**
  \brief Execute or schedule a reduce using the topological algorithm for commutative operators
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the reduce
  \param recvbuf Adress of the buffer used to recv data during the reduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param root Rank root of the reduce
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_topo_commute(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  int rank, size, res = MPI_SUCCESS;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  MPI_Type_extent(datatype, &ext);


  // TODO (already done ?)
  // /!\ deadlock with reduce_linear due to call to gather_topo on communicator subset with topo comm already initialised
  // will be solved with reusable topo comm & topo comm hierarchy
  // or with an index to track the current topo level

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
    case MPC_COLL_TYPE_COUNT:
        break;
  }

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(initial_flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "commutative reduce");
      int res = ___collectives_reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;

  /* last level topology binomial bcast */
  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];

  res = _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, 0, hardware_comm, coll_type, schedule, info);

  /* At each topological level, masters do binomial algorithm */
  int i;
  for(i = deepest_level - 1; i >= 0; i--) {
    int rank_split;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

    if(!rank_split) {
      ___collectives_barrier_type(coll_type, schedule, info);

      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int rank_master;
      _mpc_cl_comm_rank(master_comm, &rank_master);

      const void *buffer =  recvbuf;
      if(rank_master == 0) {
        buffer = MPI_IN_PLACE;
      }

      res = _mpc_mpi_config()->coll_algorithm_intracomm.reduce(buffer, recvbuf, count, datatype, op, 0, master_comm, coll_type, schedule, info);
    }
  }

  info->flag = initial_flag;

  return res;
}



/***********
  ALLREDUCE
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Allreduce
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iallreduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLREDUCE %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);
  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_iallreduce (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Allreduce
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.allreduce);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iallreduce: NBC_Init_handle failed");
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allreduce(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allreduce(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Allreduce
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm (comm);
    mpi_check_type (datatype, comm);
    mpi_check_count (count, comm);

    if (count != 0)
    {
      mpi_check_buf (sendbuf, comm);
      mpi_check_buf (recvbuf, comm);
    }
  }
  MPI_internal_request_t *req;
  mpc_lowcomm_request_t *mpc_req;
  SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req = *request = mpc_lowcomm_request_alloc();
  mpc_req = _mpc_cl_get_lowcomm_request(req);
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLREDUCE_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_allreduce_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Allreduce
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.allreduce);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Allreduce: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allreduce(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allreduce(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Allreduce
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.allreduce);
  return _mpc_mpi_config()->coll_algorithm_intracomm.allreduce(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different allreduce algorithms
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLREDUCE | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_ALLREDUCE_REDUCE_BROADCAST,
    NBC_ALLREDUCE_DISTANCE_DOUBLING,
    NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING,
    NBC_ALLREDUCE_BINARY_BLOCK,
    NBC_ALLREDUCE_RING,
    NBC_ALLREDUCE_TOPO
  } alg;

  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, 0);
    topo = 1;
  }

  if(topo) {
    alg = NBC_ALLREDUCE_TOPO;
  } else {
    //alg = NBC_ALLREDUCE_REDUCE_BROADCAST;
    if(size <= 64 && count * ext >= 2 << 13) {
      alg = NBC_ALLREDUCE_BINARY_BLOCK;
    } else if(size == 32 && count * ext <= 2 << 15) {
      alg = NBC_ALLREDUCE_REDUCE_BROADCAST;
    } else {
      alg = NBC_ALLREDUCE_DISTANCE_DOUBLING;
    }
  }

  int res;

  switch(alg) {
    case NBC_ALLREDUCE_REDUCE_BROADCAST:
      res = ___collectives_allreduce_reduce_broadcast(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_DISTANCE_DOUBLING:
      res = ___collectives_allreduce_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING:
      res = ___collectives_allreduce_vector_halving_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_BINARY_BLOCK:
      res = ___collectives_allreduce_binary_block(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_RING:
      res = ___collectives_allreduce_ring(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_TOPO:
      res = ___collectives_allreduce_topo(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a allreduce using the reduce broadcast algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, recvbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  //___collectives_bcast_switch(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);
  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the distance doubling algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  int first_access = 1;

  int vrank, peer, maxr;
  maxr = ___collectives_floored_log2(size);
  int group = 2 * (size - (1 << maxr));

  if(rank < group) {
    vrank = rank / 2;
  } else {
    vrank = rank - group / 2;
  }

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(2 * ext * count);
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += 2 * ext * count;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += 2 * ext * count;
      break;
  }

  void *tmp_recvbuf, *tmp_sendbuf, *swap;
  tmp_recvbuf = tmpbuf;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  } else {
    tmp_sendbuf = tmpbuf + ext * count;
    ___collectives_copy_type(sendbuf, count, datatype, tmp_sendbuf, count, datatype, comm, coll_type, schedule, info);
    first_access = 0;
  }

  // Reduce the number of rank participating ro the allreduce to the nearest lower power of 2:
  // all processes with odd rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) send their data to rank-1
  // all processes with even rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) receive and reduce data from rank+1
  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      ___collectives_send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);
      ___collectives_op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

      if(first_access) {
        tmp_sendbuf = tmpbuf + ext * count;
        first_access = 0;
      }

      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    }
  }

  if(rank >= group || !(rank & 1)) {
    // at each step, processes exchange and reduce their data with (rank XOR 2^i)
    int i;
    for(i = 0; i < maxr; i++) {

      peer = vrank ^ (1 << i);
      if(peer < group / 2) {
        peer = peer * 2;
      } else {
        peer = peer + group / 2;
      }

      // Prevent deadlock
      if(peer > rank) {
        ___collectives_send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      } else {
        ___collectives_recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }
      ___collectives_barrier_type(coll_type, schedule, info);

      // To respect commutativity, we must choose the right order for the Reduction operation
      // if we received from a lower rank: tmp_recvbuf OP tmp_sendbuf
      // if we received from a higher rank: tmp_sendbuf OP tmp_recvbuf
      if(vrank & (1 << i)) {
        ___collectives_op_type(NULL, tmp_recvbuf, tmp_sendbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      } else {
        ___collectives_op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

        if(first_access) {
          tmp_sendbuf = tmpbuf + ext * count;
          first_access = 0;
        }

        pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
      }
    }
  }

  // As some processes didnt participate, exchange data with them
  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      ___collectives_recv_type(recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
    }
  } else {
    ___collectives_copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the vector halving distance doubling algorithm
    Or count the number of operations and rounds for the schedule.
    This implementation is based on vector-halving distance-doubling
    Rabenseifner's algorithm and assumes that the operator is associative.
    Details and examples can be found at https://www.hlrs.de/mpi/myreduce.html.

  \param sendbuf Address of the buffer used to send data during the allreduce
  \param recvbuf Address of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation. This operator must be associative.
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  \see Rolf Rabenseifner: A new optimized MPI reduce algorithm.
       High-Performance Computing-Center, University of Stuttgart, Nov. 1997,
       https://www.hlrs.de/mpi/myreduce.html
  */
int ___collectives_allreduce_vector_halving_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

#define OLDRANK(new, r) ((new) < r ? (new)*2 : (new)+r)

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(2 * ext * count);
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += 2 * ext * count;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += 2 * ext * count;
      break;
  }

  void *tmp_recvbuf, *tmp_sendbuf, *swap;
  tmp_recvbuf = tmpbuf;
  tmp_sendbuf = tmpbuf + ext * count;
  ___collectives_copy_type(sendbuf == MPI_IN_PLACE ? recvbuf : sendbuf,
                           count, datatype, tmp_sendbuf, count, datatype,
                           comm, coll_type, schedule, info);

  /* Step 1 */
  int maxr = ___collectives_floored_log2(size);
  int r = size - (1 << maxr);

  /* Step 2 */
  if(rank < 2 * r) {
    if((rank & 1) == 0) {
      // Even processes reduce the first-half buffer
      // and collect the receive the reduced second-half buffer from the odd process rank + 1
      int peer = rank + 1;
      ___collectives_send_type(tmp_sendbuf + (count / 2) * ext, count - count / 2, datatype, peer,
                               MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(tmp_recvbuf, count / 2, datatype, peer,
                               MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);

      ___collectives_op_type(NULL, tmp_sendbuf, tmp_recvbuf, count / 2, datatype, op, mpc_op, coll_type, schedule, info);

      ___collectives_recv_type(tmp_recvbuf + (count / 2) * ext, count - count / 2,
                               datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);

      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    } else {
      // Elimination: odd processes only reduce the second-half buffer
      // and send it to even process rank - 1
      int peer = rank - 1;

      ___collectives_recv_type(tmp_recvbuf + (count / 2) * ext, count - count / 2, datatype, peer,
                               MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(tmp_sendbuf, count / 2, datatype, peer,
                               MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);

      ___collectives_op_type(NULL,
                             tmp_recvbuf + (count / 2) * ext,
                             tmp_sendbuf + (count / 2) * ext,
                             count - count / 2, datatype, op, mpc_op, coll_type, schedule, info);

      ___collectives_send_type(tmp_sendbuf + (count / 2) * ext, count - count / 2,
                               datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }
  }

  /* Step 3+4 */
  int vrank;
  // Only use 2^maxr processes, i.e. even processes and remaining processes such that rank >= 2*r
  if((rank >= 2*r) || ((rank % 2 == 0) && (rank < 2*r))) {
    vrank = (rank < 2*r ? rank/2 : rank-r);
  } else {
    vrank = -1;
  }

  if(vrank >= 0) {
    /* Step 5 */
    int x_start, x_count, i;
    x_start = 0;
    x_count = count;

    // There are at most 2^(8*sizeof(int)) MPI processes,
    // therefore buffer halving requires at most 8*sizeof(int) iterations
#define INT_BIT (CHAR_BIT * sizeof(int))
    int start_even[INT_BIT], start_odd[INT_BIT], count_even[INT_BIT], count_odd[INT_BIT];
#undef INT_BIT

    for(i = 0; i < maxr; i++) { /* Buffer halving */
      start_even[i] = x_start;
      count_even[i] = x_count / 2;
      start_odd[i] = x_start + count_even[i];
      count_odd[i] = x_count - count_even[i];

      int peer = OLDRANK(vrank ^ (1 << i), r);

      if((vrank & (1 << i)) == 0) { /* Even */
        x_start = start_even[i];
        x_count = count_even[i];

        ___collectives_send_type(tmp_sendbuf + start_odd[i] * ext, count_odd[i],
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(tmp_recvbuf + x_start * ext, x_count,
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);

        ___collectives_op_type(NULL,
                               tmp_sendbuf + x_start * ext,
                               tmp_recvbuf + x_start * ext,
                               x_count, datatype, op, mpc_op, coll_type, schedule, info);

        pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
      } else { /* Odd */
        x_start = start_odd[i];
        x_count = count_odd[i];

        ___collectives_recv_type(tmp_recvbuf + x_start * ext, x_count,
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(tmp_sendbuf + start_even[i] * ext, count_even[i],
                                datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);

        ___collectives_op_type(NULL,
                               tmp_recvbuf + x_start * ext,
                               tmp_sendbuf + x_start * ext,
                               x_count, datatype, op, mpc_op, coll_type, schedule, info);
      }
    }

    ___collectives_copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);


    /* ALLGATHER */
    /* Step 6 */

    for(i = maxr - 1; i >= 0; i--) { /* Buffer doubling */
      int peer = OLDRANK(vrank ^ (1 << i), r);

      if((vrank & (1 << i)) == 0) {  /* Even */
        ___collectives_send_type(recvbuf + start_even[i] * ext, count_even[i],
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(recvbuf + start_odd[i] * ext, count_odd[i],
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);
      } else { /* Odd */
        ___collectives_recv_type(recvbuf + start_even[i] * ext, count_even[i],
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(recvbuf + start_odd[i] * ext, count_odd[i],
                                  datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);
      }
    }
  } /* End of if(vrank >= 0) */

#undef OLDRANK

  /* Step 7 */
  if(rank < 2*r) { /* Propagate results to the 2^maxr-size unused processes */
    int peer;

    if((rank & 1) == 0) { /* Even */
      peer = rank + 1;
      ___collectives_send_type(recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG,
                               comm, coll_type, schedule, info);
    } else { /* Odd */
      peer = rank - 1;
      ___collectives_recv_type(recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG,
                               comm, coll_type, schedule, info);
    }

    ___collectives_barrier_type(coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the binary block algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op = NULL;

  int vrank = 0/*, peer*/;
  unsigned int maxr = ___collectives_ceiled_log2(size);
  vrank = rank;

  int block = (int)maxr;
  int previous_block = -1;
  int next_block = -1;

  while(block) {
    if(size & (1 << block)) {
      if(vrank >> block == 0) {
        int j = 0;
        for(j = block - 1; j >= 0; j--) {
          if(size & (1 << j)) {
            next_block = j;
            break;
          }
        }
        break;
      }
      vrank &= ~ (1 << block);

      previous_block = block;
    }
    block--;
  }


  int block_size = 1 << block;
  int first_rank_of_block = (size >> (block + 1)) << (block + 1);

  int previous_block_size = (previous_block != -1) ? (1 << previous_block) : 0;
  int first_rank_of_previous_block = (size >> (previous_block + 1)) << (previous_block + 1);

  int next_block_size = (next_block != -1) ? (1 << next_block) : 0;
  int first_rank_of_next_block = (size >> (next_block + 1)) << (next_block + 1);

  int target_count = previous_block_size / block_size;

  int start = 0;
  int end = count;
  int mid = 0;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(count * ext + (long)4 * block_size * sizeof(int));
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += count * ext + (long)4 * block_size * sizeof(int);
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += count * ext + (long)4 * block_size * sizeof(int);
      break;
  }


  int *group_starts      = (int*)((tmpbuf) + count * ext + (long)0 * block_size * sizeof(int));
  int *group_ends        = (int*)((tmpbuf) + count * ext + (long)1 * block_size * sizeof(int));
  int *prev_group_starts = (int*)((tmpbuf) + count * ext + (long)2 * block_size * sizeof(int));
  int *prev_group_ends   = (int*)((tmpbuf) + count * ext + (long)3 * block_size * sizeof(int));
  void *swap = NULL;


  if(coll_type != MPC_COLL_TYPE_COUNT) {
    // init group starts/ends
    int j = 0;
    for(j = 0; j < block_size; j++) {
      group_starts[j] = 0;
      group_ends[j] = count;
    }
  }


  if(sendbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }


  int send_bufsize = 0;
  int recv_bufsize = 0;

  void *tmp_sendbuf = NULL;
  void *tmp_recvbuf = NULL;
  void *tmp_opbuf = NULL;


  void *resbuf = recvbuf;
  void *recv_resbuf = tmpbuf;

  int peer = 0;
  int peer_vrank;



  //REDUCE_SCATTER
  //distance doubling and vector halving
  int i = 0;
  for( i = 0; i < block; i++) {
    peer_vrank = vrank ^ (1 << i);
    peer = first_rank_of_block + peer_vrank;

    mid = (start + end + 1) >> 1;

    if(peer_vrank > vrank) { // send second part of buffer

      tmp_sendbuf = (resbuf) + mid * ext;
      send_bufsize = end - mid;

      tmp_recvbuf = (recv_resbuf) + start * ext;
      tmp_opbuf = (resbuf) + start * ext;
      recv_bufsize = mid - start;

      if(send_bufsize) {
        ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }

      if(recv_bufsize) {
        ___collectives_recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);
        ___collectives_op_type(NULL, tmp_opbuf, tmp_recvbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);

        pointer_swap(resbuf, recv_resbuf, swap);
      }

      end = mid;
    } else { // send first part of buffer

      tmp_sendbuf = (resbuf) + start * ext;
      send_bufsize = mid - start;

      tmp_recvbuf = (recv_resbuf) + mid * ext;
      tmp_opbuf = (resbuf) + mid * ext;
      recv_bufsize = end - mid;

      if(recv_bufsize) {
        ___collectives_recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        ___collectives_barrier_type(coll_type, schedule, info);
        ___collectives_op_type(NULL, tmp_recvbuf, tmp_opbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);
      }

      if(send_bufsize) {
        ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }

      start = mid;
    }

    if(coll_type != MPC_COLL_TYPE_COUNT) {
      // update group starts/ends
      pointer_swap(prev_group_starts, group_starts, swap);
      pointer_swap(prev_group_ends, group_ends, swap);
      int j = 0;
      for(j = 0; j < block_size; j++) {

        peer = j ^ (1 << i);
        mid = (prev_group_starts[j] + prev_group_ends[j] + 1) >> 1;

        if(peer > j) {
          group_starts[j] = prev_group_starts[j];
          group_ends[j] = mid;
        } else {
          group_starts[j] = mid;
          group_ends[j] = prev_group_ends[j];
        }
      }
    }
  }


  // SEND/RECEIVE SEGMENTED DATA TO/FROM PREVIOUS/NEXT BLOCK
  int next_block_peer = -1;
  int peer_start = 0;
  int peer_end = 0;
  int peer_mid = 0;

  if(next_block != -1) {

    tmp_recvbuf = (recv_resbuf) + start * ext;
    tmp_opbuf = (resbuf) + start * ext;
    recv_bufsize = end - start;
    //next_block_peer = vrank / (1 << (block - next_block)) + first_rank_of_next_block;
    //next_block_peer = vrank % (1 << (block - next_block)) + first_rank_of_next_block;
    next_block_peer = vrank % next_block_size + first_rank_of_next_block;


    ___collectives_recv_type(tmp_recvbuf, recv_bufsize, datatype, next_block_peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    ___collectives_barrier_type(coll_type, schedule, info);
    ___collectives_op_type(NULL, tmp_opbuf, tmp_recvbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);

    pointer_swap(resbuf, recv_resbuf, swap);
  }


  if(resbuf != recvbuf) {
    ___collectives_copy_type(resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }


  if(previous_block != -1) {
    int j = 0;
    for(j = 0; j < target_count; j++) {
      peer_vrank = vrank + j * block_size;
      peer = peer_vrank + first_rank_of_previous_block;
      peer_start = 0;
      peer_end = count;
      int i = 0;
      for(i = 0; i < previous_block; i++) {
        int peer_peer_vrank = peer_vrank ^ (1 << i);

        peer_mid = (peer_start + peer_end + 1) >> 1;

        if(peer_peer_vrank > peer_vrank) {
          peer_end = peer_mid;
        } else {
          peer_start = peer_mid;
        }
      }

      tmp_sendbuf = (recvbuf) + peer_start * ext;
      send_bufsize = peer_end - peer_start;

      ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    }

    for(j = 0; j < target_count; j++) {
      peer_vrank = vrank + j * block_size;
      peer = peer_vrank + first_rank_of_previous_block;
      peer_start = 0;
      peer_end = count;
      int i = 0;
      for(i = 0; i < previous_block; i++) {
        int peer_peer_vrank = peer_vrank ^ (1 << i);

        peer_mid = (peer_start + peer_end + 1) >> 1;

        if(peer_peer_vrank > peer_vrank) {
          peer_end = peer_mid;
        } else {
          peer_start = peer_mid;
        }
      }

      tmp_sendbuf = (recvbuf) + peer_start * ext;
      send_bufsize = peer_end - peer_start;

      ___collectives_recv_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    }

    ___collectives_barrier_type(coll_type, schedule, info);
  }


  if(next_block != -1) {

    tmp_sendbuf = ((void*) recvbuf) + start * ext;
    send_bufsize = end - start;

    ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, next_block_peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
  }


  //ALLGATHER
  //distance halving and vector doubling
  for(i = block-1; i >= 0; i--) {
    int peer_vrank = vrank ^ (1 << i);
    int peer = first_rank_of_block + peer_vrank;

    tmp_sendbuf = (recvbuf) + start * ext;
    send_bufsize = end - start;

    if(coll_type != MPC_COLL_TYPE_COUNT) {
      recv_bufsize = group_ends[peer_vrank] - group_starts[peer_vrank];
    }

    if(peer_vrank > vrank) { // receive second part of buffer
      tmp_recvbuf = (recvbuf) + end * ext;

      ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);

      end += recv_bufsize;
    } else { // receive first part of buffer
      tmp_recvbuf = (recvbuf) + (start - recv_bufsize) * ext;

      ___collectives_recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);

      start -= recv_bufsize;
    }


    if(coll_type != MPC_COLL_TYPE_COUNT) {
      // update group starts/ends
      pointer_swap(prev_group_starts, group_starts, swap);
      pointer_swap(prev_group_ends, group_ends, swap);
      int j = 0;
      for(j = 0; j < block_size; j++) {

        peer = j ^ (1 << i);
        recv_bufsize = prev_group_ends[peer] - prev_group_starts[peer];

        if(peer > j) {
          group_starts[j] = prev_group_starts[j];
          group_ends[j] = prev_group_ends[j] + recv_bufsize;
        } else {
          group_starts[j] = prev_group_starts[j] - recv_bufsize;
          group_ends[j] = prev_group_ends[j];
        }
      }
    }
  }


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the ring algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_ring(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the topological algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the allreduce
  \param recvbuf Adress of the buffer used to recv data during the allreduce
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allreduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  ___collectives_reduce_topo(sendbuf, recvbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  return ___collectives_bcast_topo(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);
}



/***********
  SCATTER
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Scatter
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iscatter (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) {

  if( _mpc_mpi_config()->nbc.use_egreq_scatter )
  {
    return MPI_Ixscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
  }

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering ISCATTER %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize = 0;
  int rank = 0;
  MPI_Comm_size(comm, &csize);
  MPI_Comm_rank (comm, &rank);
  if(recvbuf == sendbuf && rank == root)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Scatter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    MPI_internal_request_t *tmp    = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Scatter
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.scatter);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iscatter: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Scatter
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size = 0;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(recvtype, comm);
    mpi_check_count(sendcount, comm);
    if(sendcount != 0)
    {
      mpi_check_buf(sendbuf, comm);
      mpi_check_buf(recvbuf, comm);
    }

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCATTER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_scatter_init(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Scatter
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.scatter);
  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Scatter: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Scatter
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.scatter);
  return _mpc_mpi_config()->coll_algorithm_intracomm.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Scatter algorithms
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size, tmp_count = recvcount;
  MPI_Datatype tmp_type = recvtype;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  if(rank == root && recvbuf == MPI_IN_PLACE) {
    tmp_type = sendtype;
    tmp_count = sendcount;
  }
  PMPI_Type_extent(tmp_type, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sSCATTER | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(sendcount == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_SCATTER_LINEAR,
    NBC_SCATTER_BINOMIAL,
    NBC_SCATTER_TOPO
  } alg;

  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root);
    topo = 1;
  }

  if(topo) {
    alg = NBC_SCATTER_TOPO;
  } else {
    //alg = NBC_SCATTER_BINOMIAL;
    if(size <= 64 && ext * tmp_count > 2 << 10) {
      alg = NBC_SCATTER_LINEAR;
    } else {
      alg = NBC_SCATTER_BINOMIAL;
    }
  }

  int res;

  switch(alg) {
    case NBC_SCATTER_LINEAR:
      res = ___collectives_scatter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTER_BINOMIAL:
      res = ___collectives_scatter_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTER_TOPO:
      res = ___collectives_scatter_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Scatter using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &ext);

  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root) {
        info->comm_count += size;
      } else {
        info->comm_count += 1;
      }

      return MPI_SUCCESS;
  }


  // if rank != root, receive data from root
  if(rank != root) {
    res = ___collectives_recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, send data to each process
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = ___collectives_send_type(sendbuf + i * ext * sendcount, sendcount, sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(recvbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf + rank * ext * sendcount, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  return res;
}

/**
  \brief Execute or schedule a Scatter using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  //const void *tmp_sendbuf = sendbuf;
  int tmp_recvcount = recvcount;
  MPI_Datatype tmp_recvtype = recvtype;
  // if sendbuf == MPI_IN_PLACE, recvcount & recvtype are ignored
  if(recvbuf == MPI_IN_PLACE) {
    //tmp_sendbuf = recvbuf;
    tmp_recvcount = sendcount;
    tmp_recvtype = sendtype;
  }

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  if(rank == root) {
    PMPI_Type_extent(sendtype, &sendext);
  }
  PMPI_Type_extent(tmp_recvtype, &recvext);

  int maxr, vrank, peer, peer_vrank;
  maxr = ___collectives_ceiled_log2(size);
  RANK2VRANK(rank, vrank, root);

  int rmb = ___collectives_rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  int range = ___collectives_fill_rmb(vrank, rmb);

  int peer_range, count;

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * recvext * tmp_recvcount);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * recvext * tmp_recvcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += size * recvext * tmp_recvcount;
      break;
  }



  if(rank == root) {
    ___collectives_copy_type(sendbuf, sendcount * size, sendtype, tmpbuf, tmp_recvcount * size, tmp_recvtype, comm, coll_type, schedule, info);

    // As we use virtual rank where rank root and rank 0 are swapped, we need to swap their data as well
    if(root != 0) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, tmpbuf + root * tmp_recvcount * recvext, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
      ___collectives_copy_type(sendbuf + root * sendcount * sendext, sendcount, sendtype, tmpbuf, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
    }
  }

  // Wait for data
  if(vrank != 0) {
    VRANK2RANK(peer, vrank ^ (1 << rmb), root);

    if(range < size) {
      count = (1 << rmb) * tmp_recvcount;
    } else {
      count = (size - peer) * tmp_recvcount;
    }

    ___collectives_recv_type(tmpbuf, count, tmp_recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    ___collectives_barrier_type(coll_type, schedule, info);
  }

  // Distribute data to rank+2^(rmb-1); ...; rank+2^0
  // See ___collectives_bcast_binomial
  int i;
  for(i = rmb - 1; i >= 0; i--) {
    peer_vrank = vrank | (1 << i);

    VRANK2RANK(peer, peer_vrank, root);
    if(peer >= size) {
      continue;
    }

    // In case the total number of processes isn't a power of 2 we need to compute the correct size of data to send
    // The range of a process is the higher rank to which this process may send data, regardless of the number of processes
    peer_range = ___collectives_fill_rmb(peer_vrank, i);
    // Trivial case, we are a power of 2 OR we are sending data to a process in the middle (peer + peer_range < size)
    // EX: 0[d0 d1 d2 d3] 1 2 3, first step, i = 1, size = 4, rank 0 send to rank 2, range(2) = 3
    // => count = 2^i * recvcount = 2
    // => 0[d0 d1] 1 2[d2 d3] 3

    // other case, we are not power of 2 AND we are sending to a process at the end (peer + peer_range >= size)
    // EX: 0[d0 d1 d2] 1 2, first step, i = 1, size = 3, rank 0 send to rank 2, range(2) = 3
    // => count = (size - range) * recvcount = 1
    // => 0[d0 d1] 1 2[d2]
    if(peer_range < size) {
      count = (1 << i) * tmp_recvcount;
    } else {
      count = (size - peer_vrank) * tmp_recvcount;
    }

    ___collectives_send_type(tmpbuf + (1 << i) * tmp_recvcount * recvext, count, tmp_recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }

  if(recvbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(tmpbuf, tmp_recvcount, recvtype, recvbuf, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
  }




  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Scatter using the topological algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the scatter
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the scatter
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Scatter
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatter_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  int rank, size, res = MPI_SUCCESS;
  MPI_Aint sendext, recvext;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  MPI_Datatype tmp_recvtype = recvtype;
  int tmp_recvcount = recvcount;

  if(recvbuf == MPI_IN_PLACE) {
    tmp_recvtype  = sendtype;
    tmp_recvcount = sendcount;
  }

  if(rank == root)
    PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(tmp_recvtype, &recvext);

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * recvext * tmp_recvcount);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * recvext * tmp_recvcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      {
        info->tmpbuf_size += size * recvext * tmp_recvcount;
        break;
      }
  }

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(initial_flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "scatter");
      int res = ___collectives_scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  if(!(info->hardware_info_ptr->childs_data_count)) {
    ___collectives_create_childs_counts(comm, info);
    ___collectives_create_swap_array(comm, info);
  }


  // 1) SWAP
  // 2) MASTER SCATTERV
  // 3) SPLIT SCATTER


  // SWAP PART

  if(rank == root) {

    int start = 0, end;

    for(end = 1; end < size; end++) {
      if(info->hardware_info_ptr->swap_array[end] != info->hardware_info_ptr->swap_array[end - 1] + 1) {
        ___collectives_copy_type(sendbuf + info->hardware_info_ptr->swap_array[start] * sendcount * sendext, sendcount * (end - start), sendtype,
                    tmpbuf + start * tmp_recvcount * recvext, tmp_recvcount * (end - start) , tmp_recvtype,
                    comm, coll_type, schedule, info);
        start = end;
      }
    }

    ___collectives_copy_type(sendbuf + info->hardware_info_ptr->swap_array[start] * sendcount * sendext   , sendcount * (end - start), sendtype,
                tmpbuf + start * tmp_recvcount * recvext, tmp_recvcount * (end - start) , tmp_recvtype,
                comm, coll_type, schedule, info);
  }

  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;


  int displs[size];
  int counts[size];
  void *scatterv_buf = NULL;

  int i;
  for(i = 0; i < deepest_level; i++) {
    scatterv_buf = tmpbuf;

    ___collectives_barrier_type(coll_type, schedule, info);

    int rank_split;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

    if(!rank_split) {

      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int size_master, rank_master;
      _mpc_cl_comm_size(master_comm, &size_master);
      _mpc_cl_comm_rank(master_comm, &rank_master);

      if(rank_master == 0) {
        displs[0] = 0;
        counts[0] = info->hardware_info_ptr->childs_data_count[i][0] * recvcount;
        int j;
        for(j = 1; j < size_master; j++) {
          displs[j] = displs[j-1] + counts[j-1];
          counts[j] = info->hardware_info_ptr->childs_data_count[i][j] * recvcount;
        }

        scatterv_buf = MPI_IN_PLACE;
      }

      res = _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(tmpbuf, counts, displs, recvtype, scatterv_buf, info->hardware_info_ptr->send_data_count[i] * recvcount, recvtype, 0, master_comm, coll_type, schedule, info);
	  if (res != MPI_SUCCESS)
	  {
		  mpc_common_debug_error("Scatter topo: Intracomm scatterv failed on level %d", i);
	  }
	  MPI_HANDLE_ERROR(res, comm, "Scatter topo: Intracomm scatterv failed");

      ___collectives_barrier_type(coll_type, schedule, info);
    }
  }

  /* last level topology binomial scatter */
  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];

  void *scatter_buf = recvbuf;

  if(rank == 0 && recvbuf == MPI_IN_PLACE) {
    scatter_buf = MPI_IN_PLACE;
  }

  res = _mpc_mpi_config()->coll_algorithm_intracomm.scatter(tmpbuf, tmp_recvcount, tmp_recvtype, scatter_buf, tmp_recvcount, tmp_recvtype, 0, hardware_comm, coll_type, schedule, info);

  info->flag = initial_flag;

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return res;
}



/***********
  SCATTERV
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Scatterv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iscatterv (const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering ISCATTERV %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize,rank;
  MPI_Comm_size(comm, &csize);
  MPI_Comm_rank (comm, &rank);
  if(recvbuf == sendbuf && rank == root)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Scatterv (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_iscatterv (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Scatterv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.scatterv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iscatterv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Scatterv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size;
    int rank;
    mpi_check_comm(comm);
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    mpi_check_count(recvcount, comm);
    int i;
    for(i = 0; i < size; i++)
    {
      mpi_check_count(sendcounts[i], comm);
    }
    if(recvcount != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }
    if(sendcounts[rank] != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCATTERV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_scatterv_init (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Scatterv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.scatterv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Scatterv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Scatterv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.scatterv);
  return _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Scatterv algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sSCATTERV | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_SCATTERV_LINEAR,
    NBC_SCATTERV_BINOMIAL
  } alg;

  alg = NBC_SCATTERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_SCATTERV_LINEAR:
      res = ___collectives_scatterv_linear(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTERV_BINOMIAL:
      res = ___collectives_scatterv_binomial(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Scatterv using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  MPI_Aint sendext;
  PMPI_Type_extent(sendtype, &sendext);

  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root) {
        info->comm_count += size;
      } else {
        info->comm_count += 1;
      }

      return MPI_SUCCESS;
  }


  // if rank != root, receive data from root
  if(rank != root) {
    res = ___collectives_recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, send data to each process
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = ___collectives_send_type(sendbuf + displs[i] * sendext, sendcounts[i], sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(recvbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf + displs[rank] * sendext, sendcounts[rank], sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  return res;
}

/**
  \brief Execute or schedule a Scatterv using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param displs Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcount Number of elements in recvbuf
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the broadcast
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scatterv_binomial(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  GATHER
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Gather
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Igather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) {

  if( _mpc_mpi_config()->nbc.use_egreq_gather )
  {
    return MPI_Ixgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
  }

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IGATHER %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize,rank;
  MPI_Comm_size(comm, &csize);
  MPI_Comm_rank (comm, &rank);
  if(recvbuf == sendbuf && rank == root)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Gather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Gather
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.gather);
  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Igather: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Gather
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    mpi_check_count(sendcount, comm);
    mpi_check_count(recvcount, comm);
    if(recvcount != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }
    if(sendcount != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_GATHER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_gather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Gather
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.gather);
  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Gather: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  ___collectives_sched_alloc_init(handle, schedule, &info);
  _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Gather
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.gather);
  return _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Gather algorithms
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size, tmp_count = sendcount;
  MPI_Datatype tmp_type = sendtype;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  if(rank == root && sendbuf == MPI_IN_PLACE) {
    tmp_type = recvtype;
    tmp_count = recvcount;
  }
  PMPI_Type_extent(tmp_type, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sGATHER | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(sendcount == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_GATHER_LINEAR,
    NBC_GATHER_BINOMIAL,
    NBC_GATHER_TOPO
  } alg;

  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root);
    topo = 1;
  }

  if(topo) {
    alg = NBC_GATHER_TOPO;
  } else {
    //alg = NBC_GATHER_LINEAR;
    if(size <= 8 || (size <= 128 && ext * tmp_count >= 2 << 20)) {
      alg = NBC_GATHER_LINEAR;
    } else {
      alg = NBC_GATHER_BINOMIAL;
    }
  }

  int res;

  switch(alg) {
    case NBC_GATHER_LINEAR:
      res = ___collectives_gather_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHER_BINOMIAL:
      res = ___collectives_gather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHER_TOPO:
      res = ___collectives_gather_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Gather using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  if(rank == root) {
    PMPI_Type_extent(recvtype, &ext);
  }

  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
    case MPC_COLL_TYPE_COUNT:
      break;
  }

  if(size == 1) {
    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
    return MPI_SUCCESS;
  }

  // if rank != root, send data to root
  if(rank != root) {

    res = ___collectives_send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);

  } else {
    // if rank == root, receive data from each process
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = ___collectives_recv_type(recvbuf + i * ext * recvcount, recvcount, recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * ext * recvcount, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  return res;
}

/**
  \brief Execute or schedule a Gather using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  const void *tmp_sendbuf = sendbuf;
  int tmp_sendcount = sendcount;
  MPI_Datatype tmp_sendtype = sendtype;
  // if sendbuf == MPI_IN_PLACE, sendcount & sendtype are ignored
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
    tmp_sendcount = recvcount;
    tmp_sendtype = recvtype;
  }

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  if(rank == root) {
    PMPI_Type_extent(recvtype, &recvext);
  }

  int maxr, vrank, peer, peer_vrank;
  maxr = ___collectives_ceiled_log2(size);
  RANK2VRANK(rank, vrank, root);

  int rmb = ___collectives_rmb_index(vrank);
  int range = ___collectives_fill_rmb(vrank, rmb);

  int peer_range, count;

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(size != 1) {
        tmpbuf = sctk_malloc(size * sendext * tmp_sendcount);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(size != 1) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += size * sendext * tmp_sendcount;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(size != 1) {
        info->tmpbuf_size += size * sendext * tmp_sendcount;
      }
      break;
  }

  if(size == 1) {
    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
    return MPI_SUCCESS;
  }


  ___collectives_copy_type(tmp_sendbuf, tmp_sendcount, tmp_sendtype, tmpbuf, tmp_sendcount, tmp_sendtype, comm, coll_type, schedule, info);

  // Receive data from rank+2^(rmb-1); ...; rank+2^0
  // See ___collectives_reduce_binomial
  int i;
  for(i = 0; i < maxr; i++) {
    peer_vrank = vrank ^ (1 << i);
    VRANK2RANK(peer, peer_vrank, root);

    if(vrank & (1 << i)) {

      if(range < size) {
        count = (1 << i) * tmp_sendcount;
      } else {
        count = (size - vrank) * tmp_sendcount;
      }

      ___collectives_barrier_type(coll_type, schedule, info);
      ___collectives_send_type(tmpbuf, count, tmp_sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);

      break;
    } else if(peer < size) {

      // In case the total number of processes isn't a power of 2 we need to compute the correct size of data to recv
      // The range of a process is the higher rank from which this process may recv data, regardless of the number of processes
      peer_range = ___collectives_fill_rmb(peer_vrank, i);
      // Trivial case, we are a power of 2 OR we are receiving data from a process in the middle (peer + peer_range < size)
      // EX: 0[d0 d1] 1 2[d2 d3] 3, second step, i = 1, size = 4, rank 0 recv from rank 2, range(2) = 3
      // => count = 2^i * recvcount = 2
      // => 0[d0 d1 d2 d3] 1 2 3

      // other case, we are not power of 2 AND we are receving from a process at the end (peer + peer_range >= size)
      // EX: 0[d0 d1] 1 2[d2], second step, i = 1, size = 3, rank 0 recv from rank 2, range(2) = 3
      // => count = (size - range) * recvcount = 1
      // => 0[d0 d1 d2] 1 2
      if(peer_range < size) {
        count = (1 << i) * tmp_sendcount;
      } else {
        count = (size - peer_vrank) * tmp_sendcount;
      }

      ___collectives_recv_type(tmpbuf + (1 << i) * tmp_sendcount * sendext, count, tmp_sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  }

  if(rank == root) {
    ___collectives_barrier_type(coll_type, schedule, info);

    ___collectives_copy_type(tmpbuf, tmp_sendcount * size, tmp_sendtype, recvbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);

    // As we use virtual rank where rank root and rank 0 are swapped, we need to swap their data as well
    if(root != 0) {
      ___collectives_copy_type(tmpbuf, tmp_sendcount, tmp_sendtype, recvbuf + root * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
      ___collectives_copy_type(tmpbuf + root * tmp_sendcount * sendext, tmp_sendcount, tmp_sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Gather using the topological algorithm at levels [shallowest_level, deepest_hardware_level]
  \param sendbuf Address of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to receive data during the Gather at level 0. Ignored if shallowest_level > 0
  \param recvcount Number of elements in the recv buffer. Ignored if shallowest_level > 0
  \param recvtype Type of the data elements in the recv buffer.
  \param root Rank root of the Gather (UNUSED)
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \param[in]  shallowest_level The topogical level at which gather is started
  \param[out] tmpbuf Address of the temporary buffer used to receive data in intermediate-level Gather operations
  \return error code
  */
int __INTERNAL__collectives_gather_topo_depth(void *tmp_sendbuf, int tmp_sendcount, MPI_Datatype tmp_sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, __UNUSED__ int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info,
                                              int shallowest_level, void *tmpbuf) {
  int rank, size, res = MPI_SUCCESS;
  MPI_Aint sendext, recvext;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  /* last level topology binomial gather */
  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;
  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];

  res = _mpc_mpi_config()->coll_algorithm_intracomm.gather(tmp_sendbuf, tmp_sendcount, tmp_sendtype, tmpbuf, tmp_sendcount, tmp_sendtype, 0, hardware_comm, coll_type, schedule, info);

  int displs[size];
  int counts[size];
  void *gatherv_buf = NULL;

  int i;
  for(i = deepest_level - 1; i >= shallowest_level; i--) {
    gatherv_buf = tmpbuf;

    ___collectives_barrier_type(coll_type, schedule, info);

    int rank_split;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

    if(!rank_split) {
      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int size_master, rank_master;
      _mpc_cl_comm_size(master_comm, &size_master);
      _mpc_cl_comm_rank(master_comm, &rank_master);

      if(rank_master == 0) {
        displs[0] = 0;
        counts[0] = info->hardware_info_ptr->childs_data_count[i][0] * tmp_sendcount;
        int j;
        for(j = 1; j < size_master; j++) {
          displs[j] = displs[j-1] + counts[j-1];
          counts[j] = info->hardware_info_ptr->childs_data_count[i][j] * tmp_sendcount;
        }

        gatherv_buf = MPI_IN_PLACE;
      }

      res = _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(gatherv_buf, info->hardware_info_ptr->send_data_count[i] * tmp_sendcount, tmp_sendtype, tmpbuf, counts, displs, tmp_sendtype, 0, master_comm, coll_type, schedule, info);
    }
  }

  // SWAP PART (only when level 0 is handled)

  if(shallowest_level == 0 && rank == root) {
    ___collectives_barrier_type(coll_type, schedule, info);

    int start = 0, end;

    for(end = start + 1; end < size; end++) {
      if(info->hardware_info_ptr->swap_array[end] != info->hardware_info_ptr->swap_array[end - 1] + 1) {
        ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext                                  , tmp_sendcount * (end - start), tmp_sendtype,
                    recvbuf + info->hardware_info_ptr->swap_array[start] * recvcount * recvext, recvcount * (end - start)    , recvtype,
                    comm, coll_type, schedule, info);
        start = end;
      }
    }

    ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext                                  , tmp_sendcount * (end - start), tmp_sendtype,
                recvbuf + info->hardware_info_ptr->swap_array[start] * recvcount * recvext, recvcount * (end - start)    , recvtype,
                comm, coll_type, schedule, info);
  }

  return res;
}

/**
  \brief Execute or schedule a Gather using the topological algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Gather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Gather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param root Rank root of the Gather
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  int rank, size, res = MPI_SUCCESS;
  MPI_Aint sendext, recvext;

  void *tmp_sendbuf = (void *)sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf + rank * sendext * tmp_sendcount;
  }

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * sendext * tmp_sendcount);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * sendext * tmp_sendcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      {
        info->tmpbuf_size += size * sendext * tmp_sendcount;
        break;
      }
  }

  _mpc_cl_comm_size(comm, &size);

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(initial_flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "gather");
      int res = ___collectives_gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  if(!(info->hardware_info_ptr->childs_data_count)) {
    ___collectives_create_childs_counts(comm, info);
    ___collectives_create_swap_array(comm, info);
  }

  int shallowest_level = 0;

  res = __INTERNAL__collectives_gather_topo_depth(tmp_sendbuf, tmp_sendcount, tmp_sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info,
                                                  shallowest_level, tmpbuf);

  info->flag = initial_flag;

  return res;
}



/***********
  GATHERV
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Gatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Igatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IGATHERV %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize,rank;
  MPI_Comm_size(comm, &csize);
  MPI_Comm_rank (comm, &rank);
  if(recvbuf == sendbuf && rank == root)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Gatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Gatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.gatherv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Igatherv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Gatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_root(root, size, comm);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    mpi_check_count(sendcount, comm);
    int i;
    for(i = 0; i < size; i++)
    {
      if(recvcounts[i] != 0)
      {
        mpi_check_buf(recvbuf, comm);
      }
    }
    if(sendcount != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_GATHERV_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_gatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Gatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.gatherv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Gatherv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Gatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.gatherv);
  return _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Gatherv algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sGATHERV | %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation,"\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_GATHERV_LINEAR,
    NBC_GATHERV_BINOMIAL
  } alg;

  alg = NBC_GATHERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_GATHERV_LINEAR:
      res = ___collectives_gatherv_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHERV_BINOMIAL:
      res = ___collectives_gatherv_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Gatherv using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(recvtype, &ext);

  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root) {
        info->comm_count += size;
      } else {
        info->comm_count += 1;
      }

      return MPI_SUCCESS;
  }

  if(size == 1) {
    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf + displs[0] * ext, recvcounts[0], recvtype, comm, coll_type, schedule, info);
    }
    return MPI_SUCCESS;
  }

  // if rank != root, send data to root
  if(rank != root) {
    res = ___collectives_send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, receive data from each process
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = ___collectives_recv_type(recvbuf + displs[i] * ext, recvcounts[i], recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf + displs[rank] * ext, recvcounts[rank], recvtype, comm, coll_type, schedule, info);
    }
  }

  return res;
}

/**
  \brief Execute or schedule a Gatherv using the binomial algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param root Rank root of the gatherv
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_gatherv_binomial(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER BLOCK
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Reduce_scatter_block
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ireduce_scatter_block (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IREDUCE_SCATTER_BLOCK %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);
  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }
  if(csize == 1)
  {
    res = PMPI_Reduce_scatter_block (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = ___collectives_ireduce_scatter_block (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Reduce_scatter_block
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ireduce_scatter_block (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce_scatter_block);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ireduce: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter_block(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter_block(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Reduce_scatter_block
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, __UNUSED__ MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm(comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);

    if(count != 0)
    {
      mpi_check_buf(recvbuf, comm);
      mpi_check_buf(sendbuf, comm);
    }
  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_SCATTER_BLOCK_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_reduce_scatter_block_init (sendbuf,  recvbuf, count, datatype, op, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Reduce_scatter_block
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.reduce_scatter_block);
  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Reduce_scatter_block: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter_block(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter_block(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Reduce_scatter_block
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce_scatter_block);
  return _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter_block(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Reduce_scatter_block algorithms
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sREDUCE SCATTER BLOCK | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER,
    NBC_REDUCE_SCATTER_BLOCK_DISTANCE_DOUBLING, // difficile a mettre en place
    NBC_REDUCE_SCATTER_BLOCK_DISTANCE_HALVING, // necessite operateur commutatif & associatif
    NBC_REDUCE_SCATTER_BLOCK_PAIRWISE // seulement en non bloquant & ne respecte pas l'associativité
  } alg;

  alg = NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER;

  int res;

  switch(alg) {
    case NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER:
      res = ___collectives_reduce_scatter_block_reduce_scatter(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_DOUBLING:
      res = ___collectives_reduce_scatter_block_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_HALVING:
      res = ___collectives_reduce_scatter_block_distance_halving(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_PAIRWISE:
      res = ___collectives_reduce_scatter_block_pairwise(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Reduce_scatter_block using the reduce scatter algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(rank == 0) {
        tmpbuf = sctk_malloc(ext * count * size);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(rank == 0) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += ext * count * size;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == 0) {
        info->tmpbuf_size += ext * count * size;
      }
      break;
  }

  if(sendbuf != MPI_IN_PLACE) {
    _mpc_mpi_config()->coll_algorithm_intracomm.reduce(sendbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info);
  } else {
    _mpc_mpi_config()->coll_algorithm_intracomm.reduce(recvbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info);
  }

  ___collectives_barrier_type(coll_type, schedule, info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scatter(tmpbuf, count, datatype, recvbuf, count, datatype, 0, comm, coll_type, schedule, info);


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter_block using the distance doubling algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_block_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter_block using the distance halving algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_block_distance_halving(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter_block using the pairwise algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Reduce_scatter_block
  \param recvbuf Adress of the buffer used to recv data during the Reduce_scatter_block
  \param count Number of elements in the buffers
  \param datatype Type of the data elements in the buffers
  \param op Operator to use in the reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  int send_peer, recv_peer;

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  int first_access = 1;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(3 * ext * count);
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += 3 * ext * count;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += 3 * ext * count;
      break;
  }

  void *tmp_recvbuf, *tmp_left_resbuf, *tmp_right_resbuf, *swap;
  tmp_recvbuf = tmpbuf;
  tmp_left_resbuf = tmpbuf + ext * count;
  tmp_right_resbuf = tmpbuf + 2 * ext * count;

  const void *initial_buf = sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    initial_buf = recvbuf;
  }

  ___collectives_copy_type(initial_buf, count, datatype, tmp_left_resbuf, count, datatype, comm, coll_type, schedule, info);
  int i;
  for(i = 1; i < size; i++) {
    // At each step receive from rank-i, & send to rank+i,
    // This order is choosed to avoid swap buffer:
    // tmp_recvbuf OP tmp_resbuf will put the result in tmp_resbuf
    send_peer = (rank + i) % size;
    recv_peer = (rank - i + size) % size;

    ___collectives_send_type(initial_buf + send_peer * ext * count, count, datatype, send_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);
    ___collectives_recv_type(tmp_recvbuf, count, datatype, recv_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);

    ___collectives_barrier_type(coll_type, schedule, info);

    // We reduce data from rank 0;...;rank in tmp_left_resbuf
    if(recv_peer < rank) {
      ___collectives_op_type(NULL, tmp_recvbuf, tmp_left_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    } else {
    // We reduce data from rank rank+1;...;size-1 in tmp_right_resbuf
      // No reduce operation needed if we received from rank size-1
      if(first_access) {
        pointer_swap(tmp_recvbuf, tmp_right_resbuf, swap);
        first_access = 0;
      } else {
        ___collectives_op_type(NULL, tmp_recvbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      }
    }
  }

  if(first_access) {
    ___collectives_copy_type(tmp_left_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  } else {
    ___collectives_op_type(NULL, tmp_left_resbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    ___collectives_copy_type(tmp_right_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Reduce_scatter
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ireduce_scatter (const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IREDUCE_SCATTER %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Reduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = ___collectives_ireduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Reduce_scatter
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ireduce_scatter (const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce_scatter);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ireduce_scatter: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Reduce_scatter
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, __UNUSED__ MPI_Info info, MPI_Request *request) {

  {
    int size;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    mpi_check_type(datatype, comm);
    int i;
    for(i = 0; i < size; i++)
    {
      mpi_check_count(recvcounts[i], comm);
      if(recvcounts[i] != 0)
      {
        mpi_check_buf(recvbuf, comm);
        mpi_check_buf(sendbuf, comm);
      }
    }
    mpi_check_type(datatype, comm);

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_SCATTER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  ___collectives_reduce_scatter_init (sendbuf,  recvbuf, recvcounts, datatype, op, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Reduce_scatter
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.reduce_scatter);
  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  MPI_HANDLE_ERROR(res, comm, "Reduce_scatter: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Reduce_scatter
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.reduce_scatter);
  return _mpc_mpi_config()->coll_algorithm_intracomm.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Reduce_scatter algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sREDUCE SCATTER | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_REDUCE_SCATTER_REDUCE_SCATTERV,
    NBC_REDUCE_SCATTER_DISTANCE_DOUBLING, // difficile a mettre en place
    NBC_REDUCE_SCATTER_DISTANCE_HALVING, // necessite operateur commutatif & associatif
    NBC_REDUCE_SCATTER_PAIRWISE // seulement en non bloquant
  } alg;

  alg = NBC_REDUCE_SCATTER_REDUCE_SCATTERV;

  int res;

  switch(alg) {
    case NBC_REDUCE_SCATTER_REDUCE_SCATTERV:
      res = ___collectives_reduce_scatter_reduce_scatterv(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_DOUBLING:
      res = ___collectives_reduce_scatter_distance_doubling(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_HALVING:
      res = ___collectives_reduce_scatter_distance_halving(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_PAIRWISE:
      res = ___collectives_reduce_scatter_pairwise(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Reduce_scatter using the reduce scatter algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void *tmpbuf = NULL;
  int *displs = NULL;

  int count = 0;
  int i;
  for(i = 0; i < size; i++) {
    count += recvcounts[i];
  }

  if(count == 0) {
    return MPI_SUCCESS;
  }

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      //if(rank == 0) {
        tmpbuf = sctk_malloc(ext * count + size * sizeof(int));
        displs = tmpbuf + ext * count;
      //}
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      //if(rank == 0) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        displs = tmpbuf + ext * count;
        info->tmpbuf_pos += ext * count + size * sizeof(int);
      //}
      break;

    case MPC_COLL_TYPE_COUNT:
      //if(rank == 0) {
        info->tmpbuf_size += ext * count + size * sizeof(int);
      //}
      break;
  }

  if(coll_type != MPC_COLL_TYPE_COUNT) {
    displs[0] = 0;
    int i;
    for(i = 1; i < size; i++) {
      displs[i] = displs[i-1] + recvcounts[i-1];
    }
  }

  const void *tmp_sendbuf = sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  }

  _mpc_mpi_config()->coll_algorithm_intracomm.reduce(tmp_sendbuf, tmpbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm, coll_type, schedule, info);

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter using the distance doubling algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter using the distance halving algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_distance_halving(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Reduce_scatter using the pairwise algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_reduce_scatter_pairwise(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLGATHER
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Allgather
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iallgather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLGATHER %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);
  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Allgather (sendbuf, sendcount, sendtype, recvbuf,
        recvcount, recvtype, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;

    tmp->nbc_handle.is_persistent = 0;
    res =  ___collectives_iallgather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Allgather
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.allgather);
  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iallgather: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Allgather
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm(comm);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    mpi_check_count(sendcount, comm);
    mpi_check_count(recvcount, comm);

    if(sendcount != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }
    if(recvcount != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }
  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLGATHER_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_allgather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Allgather
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.allgather);
  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Allgather: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Allgather
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.allgather);
  return _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Allgather algorithms
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size, tmp_count = recvcount;
  MPI_Datatype tmp_type = recvtype;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_type, &ext);

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLGATHER | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(sendcount == 0 && sendbuf != MPI_IN_PLACE) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_ALLGATHER_GATHER_BROADCAST,
    NBC_ALLGATHER_DISTANCE_DOUBLING,
    NBC_ALLGATHER_BRUCK,
    NBC_ALLGATHER_RING,
    NBC_ALLGATHER_TOPO_GATHER_BROADCAST,
    NBC_ALLGATHER_TOPO_GATHER_ALLGATHER_BROADCAST
  } alg;

  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, 0);
    topo = 1;
  }

  //TODO: refine with new bruck allgorithm
  if(topo) {
    alg = NBC_ALLGATHER_TOPO_GATHER_ALLGATHER_BROADCAST;
  } else {
    if(size <= 2 || (size <= 16 && tmp_count * ext >= 2 << 15)) {
      alg = NBC_ALLGATHER_RING;
    } else if(size >= 16 && size <= 32 && tmp_count * ext <= 2 << 10) {
      alg = NBC_ALLGATHER_GATHER_BROADCAST;
    } else {
      alg = NBC_ALLGATHER_DISTANCE_DOUBLING;
    }
  }

  int res;

  switch(alg) {
    case NBC_ALLGATHER_GATHER_BROADCAST:
      res = ___collectives_allgather_gather_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_DISTANCE_DOUBLING:
      res = ___collectives_allgather_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_BRUCK:
      res = ___collectives_allgather_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_RING:
      res = ___collectives_allgather_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_TOPO_GATHER_BROADCAST:
      res = ___collectives_allgather_topo_gather_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_TOPO_GATHER_ALLGATHER_BROADCAST:
      res = ___collectives_allgather_topo_gather_allgather_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Allgather using the gather_broadcast algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size;
  _mpc_cl_comm_size(comm, &size);

 _mpc_mpi_config()->coll_algorithm_intracomm.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm, coll_type, schedule, info);
 ___collectives_barrier_type(coll_type, schedule, info);
 //___collectives_bcast_switch(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);
 _mpc_mpi_config()->coll_algorithm_intracomm.bcast(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgather using the distance doubling algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  const void *initial_buf = sendbuf;
  int tmp_sendcount = sendcount;
  MPI_Datatype tmp_sendtype = sendtype;
  if(sendbuf == MPI_IN_PLACE) {
    initial_buf = recvbuf;
    tmp_sendcount = recvcount;
    tmp_sendtype = recvtype;
  }


  int rank = 0;
  int size = 0;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  int vrank = 0;
  int vsize = 0;
  int peer = 0;
  unsigned int maxr =  ___collectives_floored_log2(size);
  int count = 0;
  int peer_count = 0;

  vsize = (1 << maxr);
  int group = 2 * (size - (1 << maxr));

  if(rank < group) {
    vrank = rank / 2;
  } else {
    vrank = rank - group / 2;
  }

  void *tmpbuf = recvbuf + (MPI_Aint)rank * recvcount * recvext;
  void *tmpbuf2 = NULL;
  int *countbuf = NULL;
  int *prev_countbuf = NULL;
  void *swap = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf2 = sctk_malloc(vsize * sizeof(int) * 2);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf2 = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += vsize * sizeof(int) * 2;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += vsize * sizeof(int) * 2;
      break;
  }

  countbuf = (int*)tmpbuf2;
  prev_countbuf = (int*)countbuf + vsize;

  // Initialyse the array containing the amount of data in possession of each processes
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int i;
    for(i = 0; i < vsize; i++) {
      if(i < group / 2) {
        countbuf[i] = 2;
      } else {
        countbuf[i] = 1;
      }
    }
  }

  // Reduce the number of rank participating ro the allgather to the nearest lower power of 2:
  // all processes with odd rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) send their data to rank-1
  // all processes with even rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) receive data from rank+1
  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      ___collectives_send_type(initial_buf, tmp_sendcount, tmp_sendtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_recv_type(recvbuf + peer * recvcount * recvext, recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      ___collectives_barrier_type(coll_type, schedule, info);
    }
  }

  if(rank > group || !(rank & 1)) {

    if(sendbuf != MPI_IN_PLACE) {
      ___collectives_copy_type(sendbuf , sendcount, sendtype, tmpbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
    unsigned int i = 0;
    // at each step, processes exchange their data with (rank XOR 2^i)
    for(i = 0; i < maxr; i++) {
      peer = vrank ^ (1 << i);

      if(coll_type != MPC_COLL_TYPE_COUNT) {
        count = countbuf[vrank];
        peer_count = countbuf[peer];
      }

      if(peer < group / 2) {
        peer = peer * 2;
      } else {
        peer = peer + group / 2;
      }

      // To avoid deadlocks
      if(peer < rank) {
        ___collectives_send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(tmpbuf - peer_count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);

        tmpbuf -= peer_count * recvcount * recvext;
      } else {
        ___collectives_recv_type(tmpbuf + count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      }

      ___collectives_barrier_type(coll_type, schedule, info);

      // Update of the array containing the amount of data in possession of each processes
      // Could be replace by a formula to find the amound of data we need to receive from a specific process,
      // but i didnt find it
      if(coll_type != MPC_COLL_TYPE_COUNT) {
        pointer_swap(countbuf, prev_countbuf, swap);
        int j;
        for(j = 0; j < vsize; j++) {
          peer = j ^ (1 << i);
          countbuf[j] = prev_countbuf[j] + prev_countbuf[peer];
        }
      }
    }
  }

  // As some processes didnt participate, exchange data with them
  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      ___collectives_recv_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_send_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf2);
  }

  return MPI_SUCCESS;
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
/**
  \brief Execute or schedule a Allgather using the bruck algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Address of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size;
  MPI_Aint recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(recvtype, &recvext);

  int nports = _mpc_mpi_config()->coll_opts.nports;
  int d = ceil(log(size) / log(nports+1));

  // TODO: use atomic spreading if necessary

  if(sendbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
  }

  int distance, step;
  int base_distance = 1;




  for(step = 0; step < d; step++, base_distance *= nports+1) { // TODO: ignore step d-1 with atomic spreading
    for(int i = 1; i <= nports; i++) {
      distance = i * base_distance;
      if(distance >= size) {
        break;
      }

      int send_peer = (rank + distance       ) % size;
      int recv_peer = (rank - distance + size) % size;

      int data_count = MIN(base_distance,size - distance);

      int send_peer_start = (rank      - (data_count - 1) + size) % size;
      int recv_peer_start = (recv_peer - (data_count - 1) + size) % size;

      void * send_buf_start = recvbuf + send_peer_start * recvext * recvcount;
      void * recv_buf_start = recvbuf + recv_peer_start * recvext * recvcount;

      int dsend = size - send_peer_start < data_count;
      int drecv = size - recv_peer_start < data_count;

#define ALLGATHER_BRUCK_SEND() \
  do {                                                                                                                                                        \
    ___collectives_send_type(send_buf_start, (MIN(data_count, size - send_peer_start)) * recvcount, recvtype, send_peer, 0, comm, coll_type, schedule, info); \
    if(dsend) {                                                                                                                                               \
      ___collectives_send_type(recvbuf, (data_count - (size - send_peer_start)) * recvcount, recvtype, send_peer, 1, comm, coll_type, schedule, info);        \
    }                                                                                                                                                         \
  } while(0)

#define ALLGATHER_BRUCK_RECV() \
  do {                                                                                                                                                        \
    ___collectives_recv_type(recv_buf_start, (MIN(data_count, size - recv_peer_start)) * recvcount, recvtype, recv_peer, 0, comm, coll_type, schedule, info); \
    if(drecv) {                                                                                                                                               \
      ___collectives_recv_type(recvbuf, (data_count - (size - recv_peer_start)) * recvcount, recvtype, recv_peer, 1, comm, coll_type, schedule, info);        \
    }                                                                                                                                                         \
  } while(0)

      if(recv_peer > rank) {
        ALLGATHER_BRUCK_SEND();
        ALLGATHER_BRUCK_RECV();
      } else {
        ALLGATHER_BRUCK_RECV();
        ALLGATHER_BRUCK_SEND();
      }

#undef ALLGATHER_BRUCK_SEND
#undef ALLGATHER_BRUCK_RECV

    }

    ___collectives_barrier_type(coll_type, schedule, info);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgather using the ring algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += (size - 1) * 2 + 1;
      info->round_count += (size - 1);
      return MPI_SUCCESS;
  }

  if(sendbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
  }
  int i;
  // At each step, send to rank+1, recv from rank-1
  for(i = 0; i < size - 1; i++) {
    // To prevent deadlocks even ranks send then recv, odd ranks recv then send
    // When i=0 we send our own data
    // At each step we send the data received at the previous step
    if(rank & 1) {
      ___collectives_recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    }

    ___collectives_barrier_type(coll_type, schedule, info);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgather using the topological gather-broadcast algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_topo_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int size, rank;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  ___collectives_gather_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  ___collectives_bcast_topo(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgather using the topological gather-allgather-broadcast algorithm
    Or count the number of operations and rounds for the schedule.
    This algorithm is similar to the gather-broadcast algorithm, except that an allgather is
    performed at top level instead of a gather/broadcast.
  \param sendbuf Address of the buffer used to send data during the Allgather
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to send data during the Allgather
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  */
int ___collectives_allgather_topo_gather_allgather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size, res = MPI_SUCCESS;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  void *tmp_sendbuf = (void*) sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;

  int root = 0;

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf + rank * sendext * tmp_sendcount;
  }

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * sendext * tmp_sendcount);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * sendext * tmp_sendcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      {
        info->tmpbuf_size += size * sendext * tmp_sendcount;
        break;
      }
  }

  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root))) {
    if(!(info->flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here
      // We cannot create topological and they aren't already created
      // Fallback to non-topological algorithms

      mpc_common_debug_log("TOPO ALLG | RANK %d | Fallback to non-topological algorithm\n", rank);

      res = _mpc_mpi_config()->coll_algorithm_intracomm.allgather(sendbuf, tmp_sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = TOPO_MAX_LEVEL;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  if(!(info->hardware_info_ptr->childs_data_count)) {
    ___collectives_create_childs_counts(comm, info);
    ___collectives_create_swap_array(comm, info);
  }

  int shallowest_level = 1; // Gather & bcast do not handle level 0 (allgather at level 0)

  /* GATHER: levels max_depth -> 1 */
  res = __INTERNAL__collectives_gather_topo_depth(tmp_sendbuf, tmp_sendcount, tmp_sendtype,
                                                  NULL, 0, recvtype, // recvbuf is only used at level 0, not here (shallowest_level == 1)
                                                  root, comm, coll_type, schedule, info,
                                                  shallowest_level, tmpbuf);


  /* ALLGATHERV: top-level processes (level 0) */
  int i = 0;
  int displs[size];
  int counts[size];
  void *allgatherv_buf = NULL;

  allgatherv_buf = tmpbuf;
  ___collectives_barrier_type(coll_type, schedule, info);

  int rank_split;
  _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

  if(!rank_split) { // Level 0 processes
    MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

    int size_master, rank_master;
    _mpc_cl_comm_size(master_comm, &size_master);
    _mpc_cl_comm_rank(master_comm, &rank_master);

    if (rank_master == 0) {
      allgatherv_buf = MPI_IN_PLACE;
    }

    mpc_common_debug_log("TOPO ALLG | RANK %d | Allgather level 0 | master_comm: %p, rank_master: %d, size_master: %d\n",
        rank, master_comm, rank_master, size_master);

    // Every top-level process computes displs and counts, contrary to levels [deepest_level-1, 1]
    displs[0] = 0;
    counts[0] = info->hardware_info_ptr->childs_data_count[i][0] * tmp_sendcount;
    int j;
    for(j = 1; j < size_master; j++) {
      displs[j] = displs[j-1] + counts[j-1];
      counts[j] = info->hardware_info_ptr->childs_data_count[i][j] * tmp_sendcount;
    }

    res = _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(allgatherv_buf, info->hardware_info_ptr->send_data_count[i] * tmp_sendcount, sendtype, tmpbuf, counts, displs, sendtype, master_comm, coll_type, schedule, info);
    ___collectives_barrier_type(coll_type, schedule, info);

    // Swap: reorder blocks in right order
    int start = 0, end;

    for(end = start + 1; end < size; end++) {
      if(info->hardware_info_ptr->swap_array[end] != info->hardware_info_ptr->swap_array[end - 1] + 1) {
        ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext                                  , tmp_sendcount * (end - start), tmp_sendtype,
                    recvbuf + info->hardware_info_ptr->swap_array[start] * recvcount * recvext, recvcount * (end - start)    , recvtype,
                    comm, coll_type, schedule, info);
        start = end;
      }
    }

    ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext                                  , tmp_sendcount * (end - start), tmp_sendtype,
                recvbuf + info->hardware_info_ptr->swap_array[start] * recvcount * recvext, recvcount * (end - start)    , recvtype,
                comm, coll_type, schedule, info);
  }

  ___collectives_barrier_type(coll_type, schedule, info);


  /* BCAST: levels 1 -> deepest_level */
  /* At each topological level in [1, deepest_level], masters do binomial algorithm */
  res = __INTERNAL__collectives_bcast_topo_depth(recvbuf, size * recvcount, recvtype, root, comm, coll_type, schedule, info, shallowest_level);

  info->flag = initial_flag;

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return res;
}



/***********
  ALLGATHERV
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Allgatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iallgatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLGATHERV %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Allgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res =  ___collectives_iallgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(tmp->nbc_handle));
  }
  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Allgatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.allgatherv);

  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iallgatherv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Allgatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size;
    int rank;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    _mpc_cl_comm_rank(comm, &rank);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    int i;
    for(i = 0; i < size; i++)
    {
      mpi_check_count(recvcounts[i], comm);
    }
    mpi_check_count(sendcount, comm);

    if(recvcounts[rank] != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }

    if(sendcount != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }
  }

  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLGATHERV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_allgatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Allgatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.allgatherv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Allgatherv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Allgatherv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.allgatherv);
  return _mpc_mpi_config()->coll_algorithm_intracomm.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Allgatherv algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLGATHERV | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_ALLGATHERV_GATHERV_BROADCAST,
    NBC_ALLGATHERV_DISTANCE_DOUBLING,
    NBC_ALLGATHERV_BRUCK,
    NBC_ALLGATHERV_RING
  } alg;

  alg = NBC_ALLGATHERV_GATHERV_BROADCAST;

  int res;

  switch(alg) {
    case NBC_ALLGATHERV_GATHERV_BROADCAST:
      res = ___collectives_allgatherv_gatherv_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_DISTANCE_DOUBLING:
      res = ___collectives_allgatherv_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_BRUCK:
      res = ___collectives_allgatherv_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_RING:
      res = ___collectives_allgatherv_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Allgatherv using the gather_broadcast algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int count = 0;
  int size, rank;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  int i;
  for(i = 0; i < size; i++) {
    count += recvcounts[i];
  }

  const void *tmp_sendbuf = sendbuf;
  int tmp_sendcount = sendcount;
  MPI_Datatype tmp_sendtype = sendtype;

  if(sendbuf == MPI_IN_PLACE && rank != 0) {
    tmp_sendtype = recvtype;

    MPI_Aint sendext;
    PMPI_Type_extent(tmp_sendtype, &sendext);

    tmp_sendbuf = recvbuf + displs[rank] * sendext;
    tmp_sendcount = recvcounts[rank];
  }


  _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(tmp_sendbuf, tmp_sendcount, tmp_sendtype, recvbuf, recvcounts, displs, recvtype, 0, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  //___collectives_bcast_switch(recvbuf, count, recvtype, 0, comm, coll_type, schedule, info);
  _mpc_mpi_config()->coll_algorithm_intracomm.bcast(recvbuf, count, recvtype, 0, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgatherv using the distance doubling algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgatherv_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgatherv using the bruck algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgatherv_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgatherv using the ring algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcount Number of elements in sendbuf
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param displs Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_allgatherv_ring(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALL
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Alltoall
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLTOALL %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);
  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

        MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
        //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
	tmp->is_nbc = 1;
	tmp->nbc_handle.is_persistent = 0;
	res = ___collectives_ialltoall (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
	MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Alltoall
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.alltoall);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ialltoall: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Alltoall
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm (comm);
    mpi_check_type (sendtype, comm);
    mpi_check_count (sendcount, comm);

    if (sendcount != 0)
    {
      mpi_check_buf (sendbuf, comm);
    }
    if (recvcount != 0)
    {
      mpi_check_buf (recvbuf, comm);
    }
  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALL_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_alltoall_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Alltoall
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.alltoall);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Alltoall: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Alltoall
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {

  // MPI_Request req;
  // MPI_Status status;

  // MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
  // MPI_Wait(&req, &status);

  // return status.MPI_ERROR;

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.alltoall);
  return _mpc_mpi_config()->coll_algorithm_intracomm.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Allgather algorithms
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLTOALL | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(sendcount == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_ALLTOALL_CLUSTER,
    NBC_ALLTOALL_BRUCK,
    NBC_ALLTOALL_PAIRWISE,
    NBC_ALLTOALL_TOPO
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);

  int topo = 0;
  // Check if we are on a topological comm
  if(__global_topo_allow && info->flag & SCHED_INFO_TOPO_COMM_ALLOWED) {
    // Get topological communicators, or NULL if not created already
    info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, 0);
    topo = 1;
  }

  // TODO choose correctly ?
  if(topo) {
    alg = NBC_ALLTOALL_TOPO;
  } else {
    alg = NBC_ALLTOALL_CLUSTER;
  }

  int res;

  switch(alg) {
    case NBC_ALLTOALL_CLUSTER:
      res = ___collectives_alltoall_cluster(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_BRUCK:
      res = ___collectives_alltoall_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_PAIRWISE:
      res = ___collectives_alltoall_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_TOPO:
      res = ___collectives_alltoall_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Alltoall using the cluster algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Address of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint tmp_sendext, recvext;

  void *tmp_sendbuf = (void *)sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &tmp_sendext);
  PMPI_Type_extent(recvtype, &recvext);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(sendbuf == MPI_IN_PLACE) {
        tmp_sendbuf = sctk_malloc(recvext * recvcount * size);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmp_sendbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += recvext * recvcount * size;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += recvext * recvcount * size;
      }
      return MPI_SUCCESS;
  }

  if(sendbuf == MPI_IN_PLACE) {
    // Copy data to avoid erasing it by receiving other's data
    ___collectives_copy_type(recvbuf, recvcount * size, recvtype, tmp_sendbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);
  } else {
    // Copy our own data in the recvbuf
    ___collectives_copy_type(sendbuf + rank * sendcount * tmp_sendext, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
  }

  int i;
  // For each rank, send and recv the data needed
  for(i = 0; i < size; i++) {
    if(rank == i) {
      continue;
    } else if (rank < i) {
      ___collectives_send_type(tmp_sendbuf  + i * tmp_sendcount * tmp_sendext, tmp_sendcount, tmp_sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(tmp_sendbuf  + i * tmp_sendcount * tmp_sendext, tmp_sendcount, tmp_sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
  }

  if (coll_type == MPC_COLL_TYPE_BLOCKING && sendbuf == MPI_IN_PLACE) {
    sctk_free(tmp_sendbuf);
  }

  return MPI_SUCCESS;
}


#define INDEX2VINDEX(index, vindex, rank, size) \
  do { \
    vindex = ((index) - (rank) + (size)) % (size); \
  } while(0)


#define VINDEX2INDEX(index, vindex, rank, size) \
  do { \
    index = ((vindex) == (rank)) ? 0 : (((rank) - (vindex) + (size)) % (size)); \
  } while(0)

/**
  \brief Execute or schedule a Alltoall using the bruck algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Adress of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoall_bruck( const void *sendbuf,  int sendcount,  MPI_Datatype sendtype,  void *recvbuf,  int recvcount,  MPI_Datatype recvtype,  MPI_Comm comm,  MPC_COLL_TYPE coll_type,  NBC_Schedule * schedule,  Sched_info *info) {

  int rank, size;
  MPI_Aint tmp_sendext, recvext;

  void *tmp_sendbuf = (void *)sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &tmp_sendext);
  PMPI_Type_extent(recvtype, &recvext);

  void * tmp_buf1 = NULL,
       * tmp_buf2 = NULL;

  int sizelog = ___collectives_ceiled_log2(size);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmp_buf1 = sctk_malloc(recvcount * size * recvext);
      tmp_buf2 = sctk_malloc(recvcount * size * recvext);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmp_buf1 = info->tmpbuf + info->tmpbuf_pos;
      tmp_buf2 = info->tmpbuf + info->tmpbuf_pos + recvcount * size * recvext;
      info->tmpbuf_pos += recvext * recvcount * size * 2;
      break;
    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += recvext * recvcount * size * 2;
      break;
  }


  ___collectives_copy_type(
      tmp_sendbuf + rank * tmp_sendcount * tmp_sendext, (size - rank) * tmp_sendcount, tmp_sendtype,
      tmp_buf1                                        , (size - rank) * recvcount    , recvtype,
      comm, coll_type, schedule, info);

  ___collectives_copy_type(
      tmp_sendbuf                                     , rank * tmp_sendcount         , tmp_sendtype,
      tmp_buf1 + (size - rank) * recvcount * recvext  , rank * recvcount             , recvtype,
      comm, coll_type, schedule, info);

  for(int i = 0; i < sizelog; i++) {
    if(rank < (rank + (1 << i)) % size) {
      ___collectives_send_type(tmp_buf1, size * recvcount, recvtype, (rank + (1 << i)       ) % size, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(tmp_buf2, size * recvcount, recvtype, (rank - (1 << i) + size) % size, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_recv_type(tmp_buf2, size * recvcount, recvtype, (rank - (1 << i) + size) % size, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(tmp_buf1, size * recvcount, recvtype, (rank + (1 << i)       ) % size, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

    ___collectives_barrier_type(coll_type, schedule, info);

    for(int j = 0; j < size; j++) {
      if(j & (1 << i))
        ___collectives_copy_type(
            tmp_buf2 + j * recvcount * recvext, recvcount, recvtype,
            tmp_buf1 + j * recvcount * recvext, recvcount, recvtype,
            comm, coll_type, schedule, info);
    }
  }

  int index;
  for(int j = 0; j < size; j++) {
    VINDEX2INDEX(index, j, rank, size);
    ___collectives_copy_type(
        tmp_buf1 + j * recvcount * recvext, recvcount, recvtype,
        recvbuf + index * recvcount * recvext, recvcount, recvtype,
        comm, coll_type, schedule, info);
  }

  if (coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmp_buf1);
    sctk_free(tmp_buf2);
  }

  return MPI_SUCCESS;
}

#undef INDEX2VINDEX
#undef VINDEX2INDEX

/**
  \brief Execute or schedule a Alltoall using the pairwise algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Address of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint tmp_sendext, recvext;

  void *tmp_sendbuf = (void *)sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &tmp_sendext);
  PMPI_Type_extent(recvtype, &recvext);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(sendbuf == MPI_IN_PLACE) {
        tmp_sendbuf = sctk_malloc(recvext * recvcount * size);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmp_sendbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += recvext * recvcount * size;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += recvext * recvcount * size;
      }
      return MPI_SUCCESS;
  }

  if(sendbuf == MPI_IN_PLACE) {
    // Copy data to avoid erasing it by receiving other's data
    ___collectives_copy_type(recvbuf, recvcount * size, recvtype, tmp_sendbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);
  } else {
    // Copy our own data in the recvbuf
    ___collectives_copy_type(sendbuf + rank * sendcount * tmp_sendext, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
  }

  int distance;
  for(distance = 1; distance < size; distance++) {
    int src = (rank - distance + size) % size;
    int dest = (rank + distance) % size;

    if ((rank / distance) % 2 == 0) {
      ___collectives_send_type(tmp_sendbuf  + dest * tmp_sendcount * tmp_sendext, tmp_sendcount, tmp_sendtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + src * recvcount * recvext, recvcount, recvtype, src, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_recv_type(recvbuf + src * recvcount * recvext, recvcount, recvtype, src, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_send_type(tmp_sendbuf  + dest * tmp_sendcount * tmp_sendext, tmp_sendcount, tmp_sendtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
  }

  if (coll_type == MPC_COLL_TYPE_BLOCKING && sendbuf == MPI_IN_PLACE) {
    sctk_free(tmp_sendbuf);
  }

  return MPI_SUCCESS;
}




/**
  \brief Execute or schedule a Alltoall using a new weird and complicated algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Address of the buffer used to send data during the Alltoall
  \param sendcount Number of elements in the send buffer
  \param sendtype Type of the data elements in the send buffer
  \param recvbuf Address of the buffer used to send data during the Alltoall
  \param recvcount Number of elements in the recv buffer
  \param recvtype Type of the data elements in the recv buffer
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Address of the schedule
  \param info Address on the information structure about the schedule
  \return error code
  */
/*
  ALGORITHM PRINCIPLE:
    The base is a topological gather on rank 0 followed by a topological scatter.
    We add a little flavored seasoning with the feature of removing un-needed data along the gather,
    and re-inserting it during the scatter.

    For the next comments, we'll use the following topological tree:
     ________
    /   |    \
    |  _|_   |
    | / | \ / \
    1 3 0 4 2 5

    We could interpreate this tree like this:
      node 1: 1
      node 2:
        L3 1: 3
        L3 2: 0
        L3 3: 4
      node 3:
        L3 1: 2
        L3 2: 5

    The rank are ordered left to right. eg: on sub-communicator [3,0,4], 3 is rank 0 in this communicator.
    The following convention will be used: XY correspond to the data from the rank X destined to the rank Y

  DETAILED STEPS:
    1: each rank start by removing (storing elsewere) the data destined to itself
      rank 3 data: [30, 31, 32, 34, 35]
    2: to ease the data reordering that will be done during the gather phase, each rank reorder it's data to correspond to the topological tree
      rank 3 data: [31, 30, 34, 32, 35]
      See X1 to see why this is usefull
    3: We do a first gather on the lowest topological level
      gather(3,0,4): 0->3, 4->3:
        rank 3 data: [31, 30, 34, 32, 35, 01, 03, 04, 02, 05, 41, 43, 40, 42, 45]
      gather(2,5): 5->2
        rank 2 data:  [21, 23, 20, 24, 25, 51, 53, 50, 54, 52]
    4: We remove(store) the data destined to the rank under the current topological level
                        __  __              __  __              __  __
      rank 3 data: [31, 30, 34, 32, 35, 01, 03, 04, 02, 05, 41, 43, 40, 42, 45]
                => [31, 32, 35, 01, 02, 05, 41, 42, 45]
    5: For each tolopogical level we repeat steps 3 & 4, but we use a gatherv instead of a gather, no reorder at top level

    rank 1 data when the topological gather is complete: [13, 10, 14, 12, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 21, 23, 20, 24, 51, 53, 50, 54]

    6: We reorder the data to prepare the topological scatter
      rank 1 data: [31, 01, 41, 21, 51, 13, 10, 14, 23, 20, 24, 53, 50, 54, 12, 15, 32, 35, 02, 05, 42, 45]
    7: We do a scatterv at the highest topological level
      scatterv(1,3,2): 1->3, 1->2
        rank 1 data: [31, 01, 41, 21, 51]
        rank 3 data: [13, 10, 14, 23, 20, 24, 53, 50, 54]
        rank 2 data: [12, 15, 32, 35, 02, 05, 42, 45]
    8: We insert the data removed during the gather &
        we reorder the data to prepare for the next scatter
        rank 3 data: [13, 03, 43, 23, 53, 10, 30, 40, 20, 50, 14, 34, 04, 24, 54]
        rank 2 data: [12, 32, 02, 42, 52, 15, 35, 05, 45, 25]
    9: for each topological level we repeat steps 7 & 8, on the last level we use a scatter instead of a scatterv & no reorder nor insertion on last step
      rank 3 data: [13, 03, 43, 23, 53]
    10: we insert the ranks own data & we correctly reorder the data
      rank 3 data: [03, 13, 23, 33, 43, 53]


    X1: this first reorder help to reduce the number of copy for the top level reorder just before the scatter
      Here the buffer we want to get after the reorder:
        rank 1 data: [31, 01, 41, 21, 51, 13, 10, 14, 23, 20, 24, 53, 50, 54, 12, 15, 32, 35, 02, 05, 42, 45]
      I'll overline consecutive data block
      with first reorder:
                      __________  ______  __  ______  __  ______  __  ______  __  __________  __  __________
        rank 1 data: [13, 10, 14, 12, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 21, 23, 20, 24, 51, 53, 50, 54]
      with first reorder:
                      __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __
        rank 1 data: [10, 12, 13, 14, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 20, 21, 23, 24, 50, 51, 53, 54]
      Without this reorder, we'll need O(n^2) copy
      With this reorder, we'll need O(top_topological_level_size * n)
*/
int ___collectives_alltoall_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int initial_flag = info->flag;
  info->flag &= ~SCHED_INFO_TOPO_COMM_ALLOWED;

  int rank, size, res;
  MPI_Aint sendext, recvext;

  void *tmp_sendbuf = (void *)sendbuf;
  MPI_Datatype tmp_sendtype = sendtype;
  int tmp_sendcount = sendcount;


  // IN_PLACE Handling
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendtype = recvtype;
    tmp_sendcount = recvcount;
  }

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  assert(size > 0);

  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  }


  // We fetch the topological communicators if we can, if not we create them
  int root = 0;
  info->hardware_info_ptr = mpc_lowcomm_topo_comm_get(comm, root);
  if(!(info->hardware_info_ptr) && !(info->hardware_info_ptr)) {
    if(!(initial_flag & SCHED_INFO_TOPO_COMM_ALLOWED)) {
      // For some reason, we ended up here.
      // We cannot create topological communicators and they aren't created already.
      // Falling back to non-topological algorithms still prints warning
      // because this is caused by user configuration.
      mpc_common_debug_warning("Topological %s called, but topological communicators couldn't be fetched or created.\nFall back to non-topological algorithm", "alltoall");
      int res = ___collectives_alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      info->flag = initial_flag;
      return res;
    }

    /* choose max topological level on which to do hardware split */
    /*TODO choose level wisely */
    int max_level = _mpc_mpi_config()->coll_opts.topo.max_level;

    ___collectives_topo_comm_init(comm, root, max_level, info);
  }

  if(!(info->hardware_info_ptr->childs_data_count)) {
    ___collectives_create_childs_counts(comm, info);
    ___collectives_create_swap_array(comm, info);
  }


  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;
  int highest_level = info->hardware_info_ptr->highest_local_hardware_level;

  void *tmpbuf = NULL, *tmpbuf_other = NULL;
  void *keep_data_buf = NULL;
  void *swap = NULL;
  void *tmpbuf_alloc = NULL;
  unsigned long tmpbuf_count = 0;
  unsigned long keepbuf_count = 0;

  mpc_common_nodebug ("%d HIGHEST LEVEL: %d, DEEPEST LEVEL: %d",
      rank, highest_level, deepest_level);

  // We start by computing the buffer size we'll need.
  // We need 2 buffers:
  //   1 to store data at each level in topological gather
  //   1 to recv data from gather & scatter & data reorder
  if(highest_level == deepest_level + 1) {
    // if you're not in any master comm
    // we'll only need a buffer for the first & last data reorder
    tmpbuf_count = size;
  } else {
    // To get the buffer size, we loop through the topological level from deepest to highest
    // on every level we look at, the current rank is the master of the topological communicator (because we stop at the highest level)
    // We need to find the level were the most data would be received
    // At each level, we receive 'd' data blocks from every mpi process 'm-i' in the communicator,
    //   with s = size of the branch in the topological tree under m-i
    //   d = s * (input_comm_size - s)
    for(int i = deepest_level; i >= highest_level; i--) {
      int size_master = 0;

      if(i == deepest_level) {
        _mpc_cl_comm_size(info->hardware_info_ptr->hwcomm[i], &size_master);
      } else {
        _mpc_cl_comm_size(info->hardware_info_ptr->rootcomm[i], &size_master);
      }

      int j = 0;
      unsigned long tmp = 0;
      for(j = 0; j < size_master; j++) {
        tmp += (long)info->hardware_info_ptr->childs_data_count[i][j] * (size - info->hardware_info_ptr->childs_data_count[i][j]);
      }

      tmpbuf_count = (tmpbuf_count > tmp)?tmpbuf_count:tmp;
    }

    // Multiply by 2 because we need 2 buffer to be able to reorganize data from one to the other
    tmpbuf_count <<= 1;

    // TODO: fix, keepbuf, can be smaller
    int highest_level_size = 0;
    _mpc_cl_comm_size(info->hardware_info_ptr->hwcomm[highest_level + (highest_level == 0)], &highest_level_size);
    keepbuf_count = (long)highest_level_size * (highest_level_size - 1);

  }

  tmpbuf_count = tmpbuf_count + keepbuf_count;

  mpc_common_nodebug ("%d BUF COUNT: %lu, KEEP COUNT: %lu",
      rank, tmpbuf_count, keepbuf_count);

  // The buffer will looks like this:
  // | tmpbuf | tmpbuf_other | keep_data_buf |

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf_alloc = sctk_malloc(tmpbuf_count * tmp_sendcount * sendext);
      tmpbuf = tmpbuf_alloc;
      tmpbuf_other = tmpbuf_alloc + ((tmpbuf_count - keepbuf_count) * tmp_sendcount * sendext >> 1);
      keep_data_buf = tmpbuf + ((tmpbuf_count - keepbuf_count) * tmp_sendcount * sendext);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf_alloc = info->tmpbuf + info->tmpbuf_pos;
      tmpbuf = tmpbuf_alloc;
      tmpbuf_other = tmpbuf_alloc + ((tmpbuf_count - keepbuf_count) * tmp_sendcount * sendext >> 1);
      keep_data_buf = tmpbuf + ((tmpbuf_count - keepbuf_count) * tmp_sendcount * sendext);

      info->tmpbuf_pos += tmpbuf_count * tmp_sendcount * sendext;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += tmpbuf_count * tmp_sendcount * sendext;
      break;
  }


  // STEP 1 & 2: remove own data & data swap
  // each consecutive data is copied in one go
  // with the exemple tree:
  // rank 3 data: [30, 31, 32, 33, 34, 34] => [31, 30, 34, 32, 35]
  // We use the precomputed reverse swap array
  // As each rank doesn't need it's own data (XX, 33 with rank 3) in the buffer we need to skip it in the source buffer & to shift the following data in the destination buffer
  int start = 0;
  int end = 0;

  if(rank == start) {
    start++;
  }

  for(end = start + 1; end < size; end++) {
    if(info->hardware_info_ptr->reverse_swap_array[end] != info->hardware_info_ptr->reverse_swap_array[end - 1] + 1 || end == rank) {
      ___collectives_copy_type(tmp_sendbuf + (MPI_Aint)start * tmp_sendcount * sendext, tmp_sendcount * (end - start), tmp_sendtype,
          tmpbuf + (info->hardware_info_ptr->reverse_swap_array[start] - (info->hardware_info_ptr->reverse_swap_array[start] > info->hardware_info_ptr->reverse_swap_array[rank])) * recvcount * recvext, recvcount * (end - start), recvtype,
          comm, coll_type, schedule, info);

      // We skip our own data
      if(end == rank) {
        end++;
      }

      start = end;
    }
  }

  if(rank == size - 1) {
    end--;
  }

  ___collectives_copy_type(tmp_sendbuf + start * tmp_sendcount * sendext, tmp_sendcount * (end - start), tmp_sendtype,
      tmpbuf + (info->hardware_info_ptr->reverse_swap_array[start] - (info->hardware_info_ptr->reverse_swap_array[start] > info->hardware_info_ptr->reverse_swap_array[rank])) * recvcount * recvext, recvcount * (end - start), recvtype,
      comm, coll_type, schedule, info);


#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    char print_data[131072];
    print_data[0] = '\0';
    int *data = (int*) tmpbuf;

    for(int i = 0; i < size - 1; i++) {
      if(i != 0) {
        sprintf(&(print_data[strlen(print_data)]), ", ");
      }
      sprintf(&(print_data[strlen(print_data)]), "[");

      for(int j = 0; j < tmp_sendcount / 3; j++) {
        if(j != 0) {
          sprintf(&(print_data[strlen(print_data)]), ", ");
        }

        sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
            data[i * tmp_sendcount + j * 3 + 0],
            data[i * tmp_sendcount + j * 3 + 1],
            data[i * tmp_sendcount + j * 3 + 2]);
      }

      sprintf(&(print_data[strlen(print_data)]), "]");
    }
    mpc_common_nodebug ("%d SWAP DATA: %s", rank, print_data);
  }
#endif


  // STEP 3: last level gather
  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];

  int hardware_comm_rank = 0;
  int hardware_comm_size = 0;
  _mpc_cl_comm_rank(hardware_comm, &hardware_comm_rank);
  _mpc_cl_comm_size(hardware_comm, &hardware_comm_size);

  if(hardware_comm_rank == 0) {
    res = _mpc_mpi_config()->coll_algorithm_intracomm.gather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, tmpbuf, (size - 1) * recvcount, recvtype, 0, hardware_comm, coll_type, schedule, info);
	MPI_HANDLE_ERROR(res, comm, "Alltoall topo: Intracomm gather failed (with hardware_comm_rank == 0)");
  } else {
    res = _mpc_mpi_config()->coll_algorithm_intracomm.gather(tmpbuf, (size - 1) * recvcount, recvtype, NULL, 0, MPI_DATATYPE_NULL, 0, hardware_comm, coll_type, schedule, info);
	MPI_HANDLE_ERROR(res, comm, "Alltoall topo: Intracomm gather failed (with hardware_comm_rank != 0)");
  }
  ___collectives_barrier_type(coll_type, schedule, info);


#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type == MPC_COLL_TYPE_BLOCKING && hardware_comm_rank == 0) {
    char print_data[131072];
    print_data[0] = '\0';
    int *data = (int*) tmpbuf;
    int hwsize;
    _mpc_cl_comm_size(hardware_comm, &hwsize);

    for(int i = 0; i < hwsize * (size - 1); i++) {
      if(i != 0) {
        sprintf(&(print_data[strlen(print_data)]), ", ");
      }
      sprintf(&(print_data[strlen(print_data)]), "[");

      for(int j = 0; j < tmp_sendcount / 3; j++) {
        if(j != 0) {
          sprintf(&(print_data[strlen(print_data)]), ", ");
        }

        sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
            data[i * tmp_sendcount + j * 3 + 0],
            data[i * tmp_sendcount + j * 3 + 1],
            data[i * tmp_sendcount + j * 3 + 2]);
      }

      sprintf(&(print_data[strlen(print_data)]), "]");
    }
    mpc_common_nodebug ("%d GATHER DATA: %s", rank, print_data);
  }
#endif

  //TODO: could be smaller than 'size'
  int displs[size];
  int counts[size];

  displs[0] = 0;
  counts[0] = (size - 1) * recvcount;
  for(int i = 1; i < hardware_comm_size; i++) {
    displs[i] = displs[i - 1] + counts[i - 1];
    counts[i] = (size - 1) * recvcount;
  }


  void *gatherv_buf = NULL;

  int total_keep_data_count = 0;

  int i;
  // loop from deepest topological level to highest
  for(i = deepest_level - 1; i >= 0; i--) {
    gatherv_buf = tmpbuf;

    int hardware_comm_rank = 0;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &hardware_comm_rank);

    // STEP 4: data removal
    // We remove data from the previous gather
    if(hardware_comm_rank == 0) {

      {
        int size_master = 0;
        MPI_Comm master_comm = MPI_COMM_NULL;

        // get the current level size
        if(i == deepest_level - 1) {
          master_comm = info->hardware_info_ptr->hwcomm[i + 1];
        } else {
          master_comm = info->hardware_info_ptr->rootcomm[i + 1];
        }
        _mpc_cl_comm_size(master_comm, &size_master);
		// Mute clang-tidy warnings about potential overflows when reading displs[j]
		assert(size_master <= size);

        // Here the data is organized as follow:
        // send data block 1-1, keep data block 1, send data block 1-2, ... , send data block n-1, keep data block n, send data block n-2
        // With n the number of rank bellow the current topological level
        // After this step, we want the following 2 buffer:
        // send data: send data block 1-1, send data block 1-2, ... , send data block n-1, send data block n-2
        // keep data: keep data block 1, keep data block n

        int j = 0;
        int k = 0;
        size_t current_displ = 0;
        // We loop through each group of send data & keep data block
        for(j = 0; j < size_master; j++) {
          int packet_count = size - info->hardware_info_ptr->childs_data_count[i + 1][j];
          // We loop through every rank on the same branch, under the current topological level
          for(k = 0; k < info->hardware_info_ptr->childs_data_count[i + 1][j]; k++) {
            int keep_data_count = info->hardware_info_ptr->send_data_count[i] - info->hardware_info_ptr->childs_data_count[i + 1][j];

            void *keep_data_start = tmpbuf + displs[j] * recvext + ((MPI_Aint)k * packet_count + info->hardware_info_ptr->topo_rank) * recvcount * recvext;
            void *keep_data_end = keep_data_start + (MPI_Aint)keep_data_count * recvcount * recvext;

            int move_data_count = 0;
            if(j == size_master - 1 && k == info->hardware_info_ptr->childs_data_count[i + 1][j] - 1) {
              move_data_count = packet_count - keep_data_count - info->hardware_info_ptr->topo_rank;
            } else {
              move_data_count = packet_count - keep_data_count;
            }


          mpc_common_nodebug ("j:%d, k:%d, packet_count:%d, keep_data_count: %d, move_data_count: %d, topo rank:%d, current_displ: %d, displ: %d",
              j, k, packet_count, keep_data_count, move_data_count, info->hardware_info_ptr->topo_rank, current_displ, displs[j]);

            //part to keep
            ___collectives_copy_type(keep_data_start, keep_data_count * recvcount, recvtype,
                keep_data_buf + (MPI_Aint)total_keep_data_count * recvcount * recvext, keep_data_count * recvcount, recvtype,
                comm, coll_type, schedule, info);


            // part to send
            ___collectives_move_type(keep_data_end, move_data_count * recvcount, recvtype,
                keep_data_start - current_displ, move_data_count * recvcount, recvtype,
                comm, coll_type, schedule, info);

            current_displ += (MPI_Aint)keep_data_count * recvcount * recvext;
            total_keep_data_count += keep_data_count;
          }
        }





#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
        if(MPC_COLL_TYPE_COUNT != coll_type) {
          char print_data[131072];
          print_data[0] = '\0';
          int *data = (int*) keep_data_buf;
          int hwsize;
          _mpc_cl_comm_size(hardware_comm, &hwsize);


          for(int j = 0; j < total_keep_data_count * recvcount / 3; j++) {
            if(j != 0) {
              sprintf(&(print_data[strlen(print_data)]), ", ");
            }

            sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
                data[j * 3 + 0],
                data[j * 3 + 1],
                data[j * 3 + 2]);
          }

          mpc_common_nodebug ("%d GATHERV KEEP DATA: [%s]", rank, print_data);


          data = (int*) tmpbuf;
          print_data[0] = '\0';

          for(int j = 0; j < info->hardware_info_ptr->send_data_count[i]*(size - info->hardware_info_ptr->send_data_count[i]) * recvcount / 3; j++) {
            if(j != 0) {
              sprintf(&(print_data[strlen(print_data)]), ", ");
            }

            sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
                data[j * 3 + 0],
                data[j * 3 + 1],
                data[j * 3 + 2]);
          }

          mpc_common_nodebug ("%d GATHERV SEND DATA: [%s]", rank, print_data);
        }
#endif
      }

      // STEP 3:topological gather, gatherv phase
      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int size_master = 0;
      int rank_master = 0;
      _mpc_cl_comm_size(master_comm, &size_master);
      _mpc_cl_comm_rank(master_comm, &rank_master);


      if(rank_master == 0) {
        displs[0] = 0;
        counts[0] = info->hardware_info_ptr->childs_data_count[i][0] * (size - info->hardware_info_ptr->childs_data_count[i][0]) * recvcount;

        int j = 0;
        for(j = 1; j < size_master; j++) {
          displs[j] = displs[j-1] + counts[j-1];
          counts[j] = info->hardware_info_ptr->childs_data_count[i][j] * (size - info->hardware_info_ptr->childs_data_count[i][j]) * recvcount;
        }

        gatherv_buf = MPI_IN_PLACE;
      }

      res = _mpc_mpi_config()->coll_algorithm_intracomm.gatherv(gatherv_buf, info->hardware_info_ptr->send_data_count[i] * (size - info->hardware_info_ptr->send_data_count[i]) * recvcount, recvtype,
          tmpbuf, counts, displs, recvtype,
          0, master_comm, coll_type, schedule, info);
	  MPI_HANDLE_ERROR(res, comm, "Alltoall topo: Intracomm gatherv failed");


      ___collectives_barrier_type(coll_type, schedule, info);

      if(rank_master != 0) {
        break;
      }
#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
        if(coll_type == MPC_COLL_TYPE_BLOCKING) {
          char print_data[131072];
          print_data[0] = '\0';
          int *data = (int*) tmpbuf;

          int c = 0;

          int j;
          for(j = 0; j < size_master; j++) {
            int k;
            for(k = 0; k < counts[j] / 3; k++, c++) {
              if(c != 0) {
                sprintf(&(print_data[strlen(print_data)]), ", ");
              }

              sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
                  data[c * 3 + 0],
                  data[c * 3 + 1],
                  data[c * 3 + 2]);
            }
          }

          mpc_common_nodebug ("%d GATHERV DATA: [%s]", rank, print_data);
        }
#endif

    }
  }
/*
  STEP 6: Highest level data reorder
  In the exemple:
  rank 1 data: [13, 10, 14, 12, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 21, 23, 20, 24, 51, 53, 50, 54]
  We use 3 nested loop to go through this array:
    current master comm size (j):
      [13, 10, 14, 12, 15 | 31, 32, 35, 01, 02, 05, 41, 42, 45 | 21, 23, 20, 24, 51, 53, 50, 54]
    number of ranks under each branch of the current topological level (k):
      [13, 10, 14, 12, 15 | 31, 32, 35 || 01, 02, 05 || 41, 42, 45 | 21, 23, 20, 24 || 51, 53, 50, 54]
    current master comm size minus 1 (we dont send data to ourselves) (l):
      [13, 10, 14 ||| 12, 15 | 31 ||| 32, 35 || 01 ||| 02, 05 || 41 ||| 42, 45 | 21 ||| 23, 20, 24 || 51 ||| 53, 50, 54]

  These nested loop allow to minimse the number of copy to be done:
   __________  ______  __  ______  __  ______  __  ______  __  __________  __  __________
  [13, 10, 14, 12, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 21, 23, 20, 24, 51, 53, 50, 54]
  here 12 copies instead of 22 if done element by element
  We can clearly see for each data block, the source rank stays the same, destination ranks stay in the same topological branch

  To correctly reorder the data, we must now figure out how to get the source & destination pointer and the data size.

  We can represent where each pack should go in order to get the basic feeling:
             __________  ______  __  ______  __  ______  __  ______  __  __________  __  __________
            [13, 10, 14, 12, 15, 31, 32, 35, 01, 02, 05, 41, 42, 45, 21, 23, 20, 24, 51, 53, 50, 54]
             |           |       |   |       |   |       |   |       |   |           |   |
  0 (+0)  :  |           |       0   |       1   |       2   |       3   |           4   |
  1 (+5)  :  0           |           |           |           |           1               2
  2 (+14) :              0           1           2           3
  On the left we have the destination rank in the master communicator
    in the exemple rank 0 (master comm) is rank 1 (main comm), 1 is 3 & 2 is 2
  On each line are the packet order
  I added the displacement near the destination rank
    it represent the number of data packet written before in the destination buffer
  It's more visible on the result:
    [31, 01, 41, 21, 51, 13, 10, 14, 23, 20, 24, 53, 50, 54, 12, 15, 32, 35, 02, 05, 42, 45]
     <----------------> (+5)
     <----------------------------------------------------> (+14)

  Each data block size is easy to get:
    We send data to rank 3: how many rank bellow 3 in the topology at the current level ?
    Here it's 3
  Data source pointer is also easy, we accumulate the data block size we already written, it's our start
  Data destination pointer is less trivial
    We start by using the displacement mentionned above (which is pre-computed)
    The only other information we need is how many previous rank already wrote data for this destination.
    Another way to see this information is the topological rank

     ________
    /   |    \
    |  _|_   |
    | / | \ / \
    0 1 2 3 4 5
  The tree is the same as before but it uses the topological rank instead of the rank on the main communicator
  Here rank 2 (topo rank 4, main comm rank 2) should be the 4rth to write its data blocks
  One last thing, rank 3 (main comm) doesn't have any data block for rank in the same branch (3, 0, 4)
  To account for that we need to substract the size of the branch of the rank the destination rank
    IF the source topological rank is greater than the destination topological rank

  In the exemple, the data block [23, 20, 24] should be the i-th written with:
    i = topo_rank(source) - (topo_rank(source) > topo_rank(dest))?branch_size(dest):0
    i = topo_rank(2) - (topo_rank(2) > topo_rank(3))?branch_size(3):0
    i = 4 - (4 > 1) 3 : 0
    i = 1 (same index than shown before)
*/

  if(highest_level == 0) {
    MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[0];

    int size_master, rank_master;
    _mpc_cl_comm_size(master_comm, &size_master);
    _mpc_cl_comm_rank(master_comm, &rank_master);

    int j;
    int current_count = 0;
    int topo_rank = 0;

    // Precompute displacement
    displs[0] = 0;
    for(j = 1; j < size_master; j++) {
      displs[j] = displs[j-1] + info->hardware_info_ptr->childs_data_count[0][j-1] * (size - info->hardware_info_ptr->childs_data_count[0][j-1]);
    }

    for(j = 0; j < size_master; j++) {
      int packets_count = size_master - 1;
      int k;
      for(k = 0; k < info->hardware_info_ptr->childs_data_count[0][j]; k++) {
        int l;
        for(l = 0; l < packets_count; l++) {
          // the inverse of the check for topo_rank(source) > topo_rank(dest)
          int offset = (l >= j)?1:0;
          // data block  size = number of rank on the same branch as the destination rank
          int packet_count = info->hardware_info_ptr->childs_data_count[0][l + offset];

          void *packet_start = tmpbuf + current_count * recvcount * recvext;
          int tmp = (topo_rank + k - !offset * packet_count) * packet_count;
          void *packet_dest = tmpbuf_other + (tmp + displs[l + offset]) * recvcount * recvext;

          mpc_common_nodebug ("j:%d, k:%d, l:%d, offset:%d, packet_count:%d, topo rank:%d, displ:%d, dest-displ:%d, dest: %d", j, k, l, offset, info->hardware_info_ptr->childs_data_count[0][l + offset], topo_rank, displs[l + offset], tmp, tmp + displs[l + offset]);

          ___collectives_copy_type(packet_start, packet_count * recvcount, recvtype,
              packet_dest, packet_count * recvcount, recvtype,
              comm, coll_type, schedule, info);

          current_count += packet_count;
        }
      }
      topo_rank += info->hardware_info_ptr->childs_data_count[0][j];
    }

    // For simplicity (maybe), we want to keep tmpbuf as the primary temporary buffer
    // As data are in tmp_buf other, we swap the two pointer
    pointer_swap(tmpbuf, tmpbuf_other, swap);


#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
    if(coll_type == MPC_COLL_TYPE_BLOCKING) {
      char print_data[131072];
      print_data[0] = '\0';
      int *data = (int*) tmpbuf;

      int c = 0;

      int j;
      for(j = 0; j < size_master; j++) {
        int packets_count = info->hardware_info_ptr->childs_data_count[0][j] * (size - info->hardware_info_ptr->childs_data_count[0][j]);
        int k;
        for(k = 0; k < packets_count * recvcount / 3; k++, c++) {
          if(c != 0) {
            sprintf(&(print_data[strlen(print_data)]), ", ");
          }

          sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
              data[c * 3 + 0],
              data[c * 3 + 1],
              data[c * 3 + 2]);
        }
      }




      mpc_common_nodebug ("%d REORGANIZED DATA: [%s]", rank, print_data);
    }
#endif
  }

  for(i = 0; i < deepest_level; i++) {

    // STEP 7: topological scatter, scatterv phase
    int rank_split;
    _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[i + 1], &rank_split);

    if(!rank_split) {
      MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[i];

      int size_master, rank_master;
      _mpc_cl_comm_size(master_comm, &size_master);
      _mpc_cl_comm_rank(master_comm, &rank_master);

      if(rank_master == 0) {
        displs[0] = 0;
        counts[0] = info->hardware_info_ptr->childs_data_count[i][0] * (size - info->hardware_info_ptr->childs_data_count[i][0]) * recvcount;
        int j;
        for(j = 1; j < size_master; j++) {
          displs[j] = displs[j-1] + counts[j-1];
          counts[j] = info->hardware_info_ptr->childs_data_count[i][j] * (size - info->hardware_info_ptr->childs_data_count[i][j]) * recvcount;
        }
      }

      res = _mpc_mpi_config()->coll_algorithm_intracomm.scatterv(tmpbuf, counts, displs, recvtype,
          tmpbuf_other, info->hardware_info_ptr->send_data_count[i] * (size - info->hardware_info_ptr->send_data_count[i]) * recvcount, recvtype,
          0, master_comm, coll_type, schedule, info);
	  MPI_HANDLE_ERROR(res, comm, "Alltoall topo: Intracomm scatterv failed");

      ___collectives_barrier_type(coll_type, schedule, info);


#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
      if(MPC_COLL_TYPE_COUNT != coll_type) {
        char print_data[131072];
        print_data[0] = '\0';
        int *data = (int*) tmpbuf_other;

        int j;
        for(j = 0; j < (size - info->hardware_info_ptr->send_data_count[i]) * info->hardware_info_ptr->send_data_count[i] * recvcount / 3; j++) {
          if(j != 0) {
            sprintf(&(print_data[strlen(print_data)]), ", ");
          }

          sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
              data[j * 3 + 0],
              data[j * 3 + 1],
              data[j * 3 + 2]);
        }

        mpc_common_nodebug ("%d SCATTERV DATA: [%s]", rank, print_data);
      }
#endif

/*
      STEP 8: data reorder
      reorder data + insert data
      Similar to the highest level reorder, with data insertion
      In the exemple:
      rank 3 data: [13, 10, 14, 23, 20, 24, 53, 50, 54], stored data: [30, 34, 03, 04, 43, 40]
      We use 2 nested for to loop through the data:
        global size minus number of rank under the current topological level (j):
          [13, 10, 14 | 23, 20, 24 | 53, 50, 54]
        size of the current master comm:
          [13 || 10 || 14 | 23 || 20 || 24 | 53 || 50 || 54]

      These nested loop allow to minimse the number of copy to be done but the exemple tree isn't deep enough to see that:
       __  __  __  __  __  __  __  __  __
      [13, 10, 14, 23, 20, 24, 53, 50, 54]

      We can represent where each pack should go in order to get the basic feeling, data insertion is the next step but we still need to make room for it
                __  __  __  __  __  __  __  __  __    __  __  __  __  __  __
               [13, 10, 14, 23, 20, 24, 53, 50, 54], [30, 34, 03, 04, 43, 40]
                |   |   |   |   |   |   |   |   |     |   |   |   |   |   |
      0 (+0) :  0   |   |   3   |   |   4   |   |     |   |   1   |   2   |
      1 (+5) :      0   |       3   |       4   |     1   |       |       2
      2 (+10):          0           3           4         1       2
      On the left we have the destination rank in the master communicator
        in the exemple rank 0 (master comm) is rank 3 (main comm), 1 is 0 & 2 is 4
      Displacement is also represented, see previous explanation

      Each data block size is easy to get:
        We send data to rank 3: how many rank bellow 3 in the topology at the current level ?
        Here it's 1
      Data source pointer is also easy, we accumulate the data block size we already written, it's our start
      Data destination pointer is less trivial
        We start by using the displacement mentionned above (which is pre-computed)
        The only other information we need is how many previous rank already wrote data for this destination.
        Another way to see this information is the topological rank

         ________
        /   |    \
        |  _|_   |
        | / | \ / \
        0 1 2 3 4 5
      The tree is the same as before but it uses the topological rank instead of the rank on the main communicator
      Here rank 2 (topo rank 4) should be the 4rth to write its data blocks
      One last thing, rank 3 doesn't have any data block for rank in the same branch (only itself in the exemple)
      to account for that we need to substract the size of the branch of the rank the destination rank
        IF the source topological rank is greater than the destination topological rank

      In the exemple, the data block [20] should be the i-th written with:
        i = topo_rank(source) - (topo_rank(source) > topo_rank(dest))?branch_size(dest):0
        i = topo_rank(2) - (topo_rank(2) > topo_rank(0))?branch_size(0):0
        i = 4 - (4 > 2) 1 : 0
        i = 3 (same index than shown before)
*/
      int j;
      int size_hwcomm, size_next_master;
      _mpc_cl_comm_size(info->hardware_info_ptr->hwcomm[i + 1], &size_hwcomm);

      // We need to handle the last swap differently, as the last communicator of the tree is not a master comm
      if(i+1 == deepest_level) {
        size_next_master = size_hwcomm;
      } else {
        _mpc_cl_comm_size(info->hardware_info_ptr->rootcomm[i + 1], &size_next_master);
      }

      // preconputed displacement
      displs[0] = 0;
      for(j = 1; j < size_next_master; j++) {
        displs[j] = displs[j-1] + info->hardware_info_ptr->childs_data_count[i + 1][j-1] * (size - info->hardware_info_ptr->childs_data_count[i + 1][j-1]);
      }

      // NOTE: debug purpose only, set the whole destination buffer to 0 to be able to clearly see which data has been added after the reorder phase before insertion phase
      //memset(tmpbuf, 0, (displs[size_next_master-1] + info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1] * (size - info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1])) * recvcount * recvext);

      // Reorder for data received from scatter
      int current_count = 0;
      // for every rank other than the ones on the same branch in the topological tree (under the current level)
      for(j = 0; j < size - info->hardware_info_ptr->send_data_count[i]; j++) {
        int k, packets_count = size_next_master;
        // for every rank on the same branch in the topological tree (under the current level)
        for(k = 0; k < packets_count; k++) {
          int offset = (j >= info->hardware_info_ptr->topo_rank)?1:0;
          int packet_count = info->hardware_info_ptr->childs_data_count[i + 1][k];

          void *packet_start = tmpbuf_other + current_count * recvcount * recvext;
          int tmp = j * packet_count + offset * (info->hardware_info_ptr->send_data_count[i] - packet_count) * packet_count;
          void *packet_dest = tmpbuf + (tmp + displs[k]) * recvcount * recvext;

          mpc_common_nodebug ("j:%d, k:%d, offset:%d, packet_count:%d, displ:%d, send_count:%d, src offset: %d, dst offset: %d", j, k, offset, info->hardware_info_ptr->childs_data_count[i+1][k], displs[k], info->hardware_info_ptr->send_data_count[i], current_count, tmp + displs[k]);

          ___collectives_copy_type(packet_start, packet_count * recvcount, recvtype,
              packet_dest, packet_count * recvcount, recvtype,
              comm, coll_type, schedule, info);

          current_count += packet_count;
        }
      }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
      if(coll_type == MPC_COLL_TYPE_BLOCKING) {
        char print_data[131072];
        print_data[0] = '\0';
        int *data = (int*) tmpbuf;

        int j;
        for(j = 0; j < (displs[size_next_master-1] + info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1] * (size - info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1])) * recvcount / 3; j++) {
          if(j != 0) {
            sprintf(&(print_data[strlen(print_data)]), ", ");
          }

          sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
              data[j * 3 + 0],
              data[j * 3 + 1],
              data[j * 3 + 2]);
        }

        mpc_common_nodebug ("%d SCATTERV REORDERED DATA: [%s]", rank, print_data);
      }
#endif


      // STEP 8: data insertion
      // TODO: a reorder of this data could be done sooner to ease this step
      // insertion & reorder from data kept during gather phase
      int topo_rank = info->hardware_info_ptr->send_data_count[i] - 1;
      current_count = 0;
      for(j = size_next_master-1; j >= 0; j--) {
        int k;
        for(k = info->hardware_info_ptr->childs_data_count[i + 1][j] - 1; k >= 0 ; k--) {
          int l;
          for(l = size_next_master - 2; l >= 0; l--) {
            int offset = (l >= j)?1:0;
            int packet_count = info->hardware_info_ptr->childs_data_count[i + 1][l + offset];

            void *packet_start = keep_data_buf + (total_keep_data_count - current_count - packet_count) * recvcount * recvext;

            int tmp = (info->hardware_info_ptr->topo_rank + topo_rank - !offset * packet_count) * packet_count;
            void *packet_dest = tmpbuf + (tmp + displs[l + offset]) * recvcount * recvext;

            mpc_common_nodebug ("j:%d, k:%d, l:%d, offset:%d, packet_count:%d, displ:%d, topo_rank:%d, topo_rank2:%d, current_count:%d, src offset: %d, dst offset: %d", j, k, l, offset, packet_count, displs[l + offset], info->hardware_info_ptr->topo_rank, topo_rank, current_count, total_keep_data_count - current_count - packet_count, tmp + displs[l + offset]);

            ___collectives_copy_type(packet_start, packet_count * recvcount, recvtype,
                packet_dest, packet_count * recvcount, recvtype,
                comm, coll_type, schedule, info);

            current_count += packet_count;
          }
          topo_rank--;
        }
      }

      total_keep_data_count -= current_count;

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
      if(coll_type == MPC_COLL_TYPE_BLOCKING) {
        char print_data[131072];
        print_data[0] = '\0';
        int *data = (int*) tmpbuf;

        int j;
        for(j = 0; j < (displs[size_next_master-1] + info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1] * (size - info->hardware_info_ptr->childs_data_count[i + 1][size_next_master-1])) * recvcount / 3; j++) {
          if(j != 0) {
            sprintf(&(print_data[strlen(print_data)]), ", ");
          }

          sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
              data[j * 3 + 0],
              data[j * 3 + 1],
              data[j * 3 + 2]);
        }

        mpc_common_nodebug ("%d SCATTERV INSERTED DATA:  [%s]", rank, print_data);
      }
#endif

    }
  }

  // STEP 7: Scatter
  /* last level topology binomial scatter */
  void *scatter_buf = tmpbuf;

  if(hardware_comm_rank == 0) {
    scatter_buf = MPI_IN_PLACE;
  }

  // TODO maybe stop using IN_PLACE ?
  res = _mpc_mpi_config()->coll_algorithm_intracomm.scatter(tmpbuf, (size - 1) * recvcount, recvtype, scatter_buf, (size - 1) * recvcount, recvtype, 0, hardware_comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);


#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    char print_data[131072];
    print_data[0] = '\0';
    int *data = (int*) tmpbuf;

    int j;
    for(j = 0; j < (size - 1) * recvcount / 3; j++) {
      if(j != 0) {
        sprintf(&(print_data[strlen(print_data)]), ", ");
      }

      sprintf(&(print_data[strlen(print_data)]), "%d-%d_%d",
          data[j * 3 + 0],
          data[j * 3 + 1],
          data[j * 3 + 2]);
    }

    mpc_common_nodebug ("%d SCATTER DATA:  [%s]", rank, print_data);
  }
#endif


  // STEP 10: data insertion & data swap
  // mostly the same than the first one.
  // with the exemple tree:
  // rank 3 data: [13, 03, 43, 23, 53] => [03, 13, 23, XX, 43, 53]
  //   + insertion: [03, 13, 23, 33, 43, 53]
  // We use the precomputed swap & reverse swap array
  // As each rank doesn't have it's own data (XX, 33 with rank 3) in the buffer we need to skip it in the swap array
  start = 0;

  for(end = start + 1; end < size - 1; end++) {
    int offset = (start >= info->hardware_info_ptr->reverse_swap_array[rank]);
    if(info->hardware_info_ptr->swap_array[end + offset] != info->hardware_info_ptr->swap_array[end - 1 + offset] + 1 || end == info->hardware_info_ptr->reverse_swap_array[rank]) {
      ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext, tmp_sendcount * (end - start), tmp_sendtype,
          recvbuf + info->hardware_info_ptr->swap_array[start + offset] * recvcount * recvext, recvcount * (end - start), recvtype,
          comm, coll_type, schedule, info);

      start = end;
    }
  }

  ___collectives_copy_type(tmpbuf + start * tmp_sendcount * sendext, tmp_sendcount * (end - start), tmp_sendtype,
      recvbuf + info->hardware_info_ptr->swap_array[start + (start >= info->hardware_info_ptr->reverse_swap_array[rank])] * recvcount * recvext, recvcount * (end - start), recvtype,
      comm, coll_type, schedule, info);

  // Each rank insert it's own data
  if(sendbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(sendbuf + rank * recvcount * recvext, recvcount, recvtype,
        recvbuf + rank * recvcount * recvext, recvcount, recvtype,
        comm, coll_type, schedule, info);
  }



  info->flag = initial_flag;

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf_alloc);
  }

  return res;
}


/***********
  ALLTOALLV
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Alltoallv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLTOALLV %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

        MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
        //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
	tmp->is_nbc = 1;
	tmp->nbc_handle.is_persistent = 0;

	res = ___collectives_ialltoallv (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(tmp->nbc_handle));
	MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Alltoallv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.alltoallv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ialltoallv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Alltoallv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size = 0;
    int rank = 0;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    _mpc_cl_comm_rank(comm, &rank);
    mpi_check_type(sendtype, comm);
    mpi_check_type(recvtype, comm);
    int i = 0;
    for(i = 0; i < size; i++)
    {
      mpi_check_count(recvcounts[i], comm);
      mpi_check_count(sendcounts[i], comm);
    }
    if(recvcounts[rank] != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }
    if(sendcounts[rank] != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }

  }
  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALLV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_alltoallv_init (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Alltoallv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.alltoallv);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Alltoallv: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Alltoallv
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {

  MPI_Request req = MPI_REQUEST_NULL;
  MPI_Status status;

  PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
  MPI_Wait(&req, &status);

  return status.MPI_ERROR;
}


/**
  \brief Swith between the different Alltoallv algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLTOALLV | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_ALLTOALLV_CLUSTER,
    NBC_ALLTOALLV_BRUCK,
    NBC_ALLTOALLV_PAIRWISE
  } alg;

  int size = 0;
  _mpc_cl_comm_size(comm, &size);

  alg = NBC_ALLTOALLV_PAIRWISE;

  int res = 0;

  switch(alg) {
    case NBC_ALLTOALLV_CLUSTER:
      res = ___collectives_alltoallv_cluster(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_BRUCK:
      res = ___collectives_alltoallv_bruck(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_PAIRWISE:
      res = ___collectives_alltoallv_pairwise(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Alltoallv using the cluster algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  void *tmpbuf = NULL;

  int rsize = 0;
  int recvcount = 0;

  int i;
  for(i = 0; i < size; i++) {
    int size = (rdispls[i] + recvcounts[i]) * recvext;
    rsize = (size > rsize) ? (size) : (rsize) ;

    recvcount += recvcounts[i];
  }

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(recvcount * recvext);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += rsize;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += rsize;
      }
      return MPI_SUCCESS;
  }

  if(sendbuf == MPI_IN_PLACE) {
    // Copy data to avoid erasing it by receiving other's data
    ___collectives_copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    // For each rank, send and recv the data needed
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      ___collectives_send_type(tmpbuf  + rdispls[i] * recvext, recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + rdispls[i] * recvext, recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    // For each rank, send and recv the data needed
    int i;
    for( i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      ___collectives_send_type(sendbuf + sdispls[i] * sendext, sendcounts[i], sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + rdispls[i] * recvext, recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
    // Copy our own data in the recvbuf
    ___collectives_copy_type(sendbuf + sdispls[rank] * sendext, sendcounts[rank], sendtype, recvbuf + rdispls[rank] * recvext, recvcounts[rank], recvtype, comm, coll_type, schedule, info);

  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallv using the bruck algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallv_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallv using the pairwise algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the displacement relative to sendbuf from which to take the sent data for process i in terms of number of ELEMENTS
  \param sendtype Type of the data elements in sendbuf
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i in terms of number of ELEMENTS
  \param recvtype Type of the data elements in recvbuf
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallv_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  // /!\ this function is derived from `___collectives_alltoall_pairwise` from branch `alltoall-topo`

  // Example:
  //
  // P0 [00,00,00,02,02,01,03,03]      P0 [00,00,00,10,30]
  // P1 [12,11,11,13,13,10]            P1 [01,11,11,21,31]
  // P2 [21,22]                    =>  P2 [02,02,12,22,32]
  // P3 [33,32,31,30]                  P3 [03,03,13,13,33]
  //
  // Thus the counts/disps will be:
  //
  // P0:
  //   sendcounts = [3,1,2,2]
  //   recvcounts = [3,1,0,1]
  //   sdispls    = [0,5,3,6]
  //   rdispls    = [0,3,0,4]
  //
  // P1:
  //   sendcounts = [1,2,1,2]
  //   recvcounts = [1,2,1,1]
  //   sdispls    = [5,1,0,3]
  //   rdispls    = [0,1,3,4]
  //
  // P2:
  //   sendcounts = [0,1,1,0]
  //   recvcounts = [2,1,1,1]
  //   sdispls    = [0,0,1,0]
  //   rdispls    = [0,2,3,4]
  //
  // P3:
  //   sendcounts = [1,1,1,1]
  //   recvcounts = [2,2,0,1]
  //   sdispls    = [3,2,1,0]
  //   rdispls    = [0,2,4,0]
  //
  // We also observe that the rdispls are the cumulative sum of the recvcounts

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  // Total number of elements received by the current MPI process, different for each. DIFFERENT FROM NORMAL ALLTOALL RECVCOUNT!!
  // Used to determine buffer allocation length
  int recvcount = 0;
  for(int i = 0; i < size; i++) {
    recvcount += recvcounts[i];
  }

  // Buffer is of size recvcount and stores elements of size smaller or equal to recvext
  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(recvcount * recvext);
      break;

    // tmpbuf_pos increment not multiplied by size because recvcount is a sum that already takes size into account
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += recvcount * recvext;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += recvcount * recvext;
      }
      return MPI_SUCCESS;
  }

  // We multiply by the type extend in comm call if needed (counts, displacements) for pointer arithmetics
  int distance;

  if(sendbuf == MPI_IN_PLACE) {
    // All send-related args are ignored

    // Copy data to avoid erasing it by receiving other's data
    // Same remarks on recvcount as tmpbuf_pos increment
    ___collectives_copy_type(recvbuf, recvcount, recvtype, tmpbuf, recvcount, recvtype, comm, coll_type, schedule, info);

    for(distance = 1; distance < size; distance++) {
      int src  = (rank - distance + size) % size;
      int dest = (rank + distance)        % size;

      // Half the processes do send/recv, and the other half do recv/send to avoid deadlocks
      if ((rank / distance) % 2 == 0) {
        // We don't forget to jump displs[proc] ELEMENTS in the buffer
        ___collectives_send_type(tmpbuf  + rdispls[dest] * recvext, recvcounts[dest], recvtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(recvbuf + rdispls[src] * recvext,  recvcounts[src],  recvtype, src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      } else {
        ___collectives_recv_type(recvbuf + rdispls[src] * recvext,  recvcounts[src],  recvtype, src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(tmpbuf  + rdispls[dest] * recvext, recvcounts[dest], recvtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      }
    }

    // No need to copy our own data from sendbuf to recvbuf
    // MPI_IN_PLACE tells us they're the same buffer so the data is already where we need it to be

  } else {
    for(distance = 1; distance < size; distance++) {
      int src  = (rank - distance + size) % size;
      int dest = (rank + distance)        % size;

      if ((rank / distance) % 2 == 0) {
        ___collectives_send_type(sendbuf + sdispls[dest] * sendext, sendcounts[dest], sendtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(recvbuf + rdispls[src] * recvext,  recvcounts[src],  recvtype, src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      } else {
        ___collectives_recv_type(recvbuf + rdispls[src] * recvext,  recvcounts[src],  recvtype, src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(sendbuf + sdispls[dest] * sendext, sendcounts[dest], sendtype, dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      }
    }

    // Copy our own data in the recvbuf, corresponds to distance = 0 (src = dest = rank)
    // Works in alltoallv the same way it does in the alltoall algorithm, since it only actually does stuff if sendcounts[rank] != 0
    ___collectives_copy_type(sendbuf + sdispls[rank] * sendext, sendcounts[rank], sendtype, recvbuf + rdispls[rank] * recvext, recvcounts[rank], recvtype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}




/***********
  ALLTOALLW
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Alltoallw
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IALLTOALLW %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

        MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
        //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
	tmp->is_nbc = 1;
	tmp->nbc_handle.is_persistent = 0;

	res = ___collectives_ialltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(tmp->nbc_handle));
	MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Alltoallw
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.alltoallw);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ialltoallw: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Alltoallw
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    int size;
    int rank;
    mpi_check_comm(comm);
    _mpc_cl_comm_size(comm, &size);
    _mpc_cl_comm_rank(comm, &rank);
    int i;
    for(i = 0; i < size; i++)
    {
      mpi_check_count(recvcounts[i], comm);
      mpi_check_count(sendcounts[i], comm);
      mpi_check_type(sendtypes[i], comm);
      mpi_check_type(recvtypes[i], comm);
    }
    if(recvcounts[rank] != 0)
    {
      mpi_check_buf(recvbuf, comm);
    }
    if(sendcounts[rank] != 0)
    {
      mpi_check_buf(sendbuf, comm);
    }

  }

  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALLW_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_alltoallw_init (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Alltoallw
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.alltoallw);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Alltoallw: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Alltoallw
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm) {

  MPI_Request req;
  MPI_Status status;

  PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
  MPI_Wait(&req, &status);

  return status.MPI_ERROR;
}


/**
  \brief Swith between the different Alltoallw algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size = -1;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sALLTOALLW | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_ALLTOALLW_CLUSTER,
    NBC_ALLTOALLW_BRUCK,
    NBC_ALLTOALLW_PAIRWISE
  } alg;

  int size = 0;
  _mpc_cl_comm_size(comm, &size);

  alg = NBC_ALLTOALLW_CLUSTER;

  int res = 0;

  switch(alg) {
    case NBC_ALLTOALLW_CLUSTER:
      res = ___collectives_alltoallw_cluster(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_BRUCK:
      res = ___collectives_alltoallw_bruck(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_PAIRWISE:
      res = ___collectives_alltoallw_pairwise(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Alltoallw using the cluster algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  void *tmpbuf = NULL;

  size_t rsize = 0;
  int totalext = 0;

  int i = 0;
  for(i = 0; i < size; i++)
  {
    PMPI_Type_extent(recvtypes[i], &ext);
    size_t size = rdispls[i] + recvcounts[i] * ext;
    rsize = (size > rsize) ? (size) : (rsize);

    totalext += recvcounts[i] * ext;
  }

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(totalext);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += rsize;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += rsize;
      }
      return MPI_SUCCESS;
  }

  if(sendbuf == MPI_IN_PLACE) {
    // Copy data to avoid erasing it by receiving other's data
    ___collectives_copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    // For each rank, send and recv the data needed
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      ___collectives_send_type(tmpbuf  + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    // For each rank, send and recv the data needed
    int i;
    for(i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      ___collectives_send_type(sendbuf + sdispls[i], sendcounts[i], sendtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      ___collectives_recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
    // Copy our own data in the recvbuf
    ___collectives_copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtypes[rank], recvbuf + rdispls[rank], recvcounts[rank], recvtypes[rank], comm, coll_type, schedule, info);

  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallw using the bruck algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement relative to sendbuf from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement relative to recvbuf at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallw_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ const MPI_Datatype *sendtypes, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ const MPI_Datatype *recvtypes, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallw using the pairwise algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param sendcounts Array (of length group size) containing the number of elements send to each process
  \param sdispls Array (of length group size) specifying at entry i the the displacement in bytes (relative to sendbuf) from which to take the sent data for process i
  \param sendtypes Array (of length group size) specifying at entry i the type of the data elements to send to process i
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param recvcounts Array (of length group size) containing the number of elements received from each process
  \param rdispls Array (of length group size) specifying at entry i the the displacement in bytes (relative to recvbuf) at which to place the received data from process i
  \param recvtypes Array (of length group size) specifying at entry i the type of the data elements to receive from process i
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_alltoallw_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  // WARNING /!\ Unlike in alltoall and alltoallv, displacements should here be in BYTES, not in NB OF ELEMENTS, so nothing will change with the associated MR

  int rank, size;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  // Since it's alltoallW, we need an array of extents
  MPI_Aint* sendexts = (MPI_Aint*)sctk_malloc(size * sizeof(MPI_Aint));
  MPI_Aint* recvexts = (MPI_Aint*)sctk_malloc(size * sizeof(MPI_Aint));;


  for(int i = 0; i < size; i++) {
    PMPI_Type_extent(sendtypes[i], &(sendexts[i]));
    PMPI_Type_extent(recvtypes[i], &(recvexts[i]));
  }

  // We somehow need to account for the different extents of all the different datatypes
  // The solution found here is to just get the total amount of bytes.
  // No "totalcount" is stored, because the various datatypes and their various extents make it meaningless in alltoallw
  int totalext = 0;
  for(int i = 0; i < size; i++) {
    totalext += recvcounts[i] * recvexts[i];
  }

  // Buffer is of size totalext bytes
  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(totalext);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += totalext;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += totalext;
      }
      return MPI_SUCCESS;
  }

  int distance = 0;
  if(sendbuf == MPI_IN_PLACE) {
    // All send-related args are ignored

    // Copy data to avoid erasing it by receiving other's data. As we can only use the total extent in bytes of the data, and since MPI_CHAR is 1 byte, we copy totalext MPI_CHARs to cover all the data regardless of all datatypes
    ___collectives_copy_type(recvbuf, totalext, MPI_CHAR, tmpbuf, totalext, MPI_CHAR, comm, coll_type, schedule, info);

    for(distance = 1; distance < size; distance++) {
      int src  = (rank - distance + size) % size;
      int dest = (rank + distance)        % size;

      if ((rank / distance) % 2 == 0) {
        ___collectives_send_type(tmpbuf  + rdispls[dest], recvcounts[dest], recvtypes[dest], dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(recvbuf + rdispls[src],  recvcounts[src],  recvtypes[src],  src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      } else {
        ___collectives_recv_type(recvbuf + rdispls[src],  recvcounts[src],  recvtypes[src],  src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(tmpbuf  + rdispls[dest], recvcounts[dest], recvtypes[dest], dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      }
    }

    // No need to copy our own data from sendbuf to recvbuf
    // MPI_IN_PLACE tells us they're the same buffer so the data is already where we need it to be

  } else {
    for(distance = 1; distance < size; distance++) {
      int src  = (rank - distance + size) % size;
      int dest = (rank + distance)        % size;

      if ((rank / distance) % 2 == 0) {
        ___collectives_send_type(sendbuf + sdispls[dest], sendcounts[dest], sendtypes[dest], dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_recv_type(recvbuf + rdispls[src],  recvcounts[src],  recvtypes[src],  src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      } else {
        ___collectives_recv_type(recvbuf + rdispls[src],  recvcounts[src],  recvtypes[src],  src,  MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
        ___collectives_send_type(sendbuf + sdispls[dest], sendcounts[dest], sendtypes[dest], dest, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      }
    }

    ___collectives_copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtypes[rank], recvbuf + rdispls[rank], recvcounts[rank], recvtypes[rank], comm, coll_type, schedule, info);

  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return MPI_SUCCESS;
}




/***********
  SCAN
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Scan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering ISCAN %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize = 0;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = ___collectives_iscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Scan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.scan);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iscan: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Scan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm(comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);

    if(count != 0)
    {
      mpi_check_buf(recvbuf, comm);
      mpi_check_buf(sendbuf, comm);
    }
  }

  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCAN_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_scan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;

  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Scan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.scan);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Scan: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.scan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.scan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Scan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.scan);
  return _mpc_mpi_config()->coll_algorithm_intracomm.scan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Scan algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sSCAN | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_SCAN_LINEAR,
    NBC_SCAN_ALLGATHER
  } alg;

  int size = 0;
  _mpc_cl_comm_size(comm, &size);

  alg = NBC_SCAN_LINEAR;

  int res;

  switch(alg) {
    case NBC_SCAN_LINEAR:
      res = ___collectives_scan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_SCAN_ALLGATHER:
      res = ___collectives_scan_allgather(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Scan using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  int res = MPI_SUCCESS;

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      tmpbuf = sctk_malloc(ext * count);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += ext * count;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += ext * count;
      break;
  }

  // Copy data to recvbuf
  if(sendbuf != MPI_IN_PLACE) {
    ___collectives_copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  // recv & reduce data from rank-1
  if(rank > 0) {
    ___collectives_recv_type(tmpbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    ___collectives_barrier_type(coll_type, schedule, info);
    ___collectives_op_type(NULL, tmpbuf, recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  // send data to rank+1
  if(rank + 1 < size) {
    ___collectives_send_type(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
  }


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return res;
}

/**
  \brief Execute or schedule a Scan using the allgather algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext = 0;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      tmpbuf = sctk_malloc(ext * count * size);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += ext * count * size;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += ext * count * size;
      break;
  }

  const void *buf = sendbuf;
  if(buf == MPI_IN_PLACE) {
    buf = recvbuf;
  }


  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);
  int i = 0;
  for( i = 1; i <= rank; i++) {
    ___collectives_op_type(NULL, tmpbuf + ((MPI_Aint)i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  ___collectives_copy_type(tmpbuf + (MPI_Aint)rank * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}




/***********
  EXSCAN
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Exscan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request) {

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IEXSCAN %d", comm);
  SCTK__MPI_INIT_REQUEST (request);

  int csize = 0;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Exscan (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = ___collectives_iexscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Exscan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.exscan);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Iexscan: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.exscan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.exscan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Exscan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm(comm);
    mpi_check_type(datatype, comm);
    mpi_check_count(count, comm);

    if(count != 0)
    {
      mpi_check_buf(recvbuf, comm);
      mpi_check_buf(sendbuf, comm);
    }
  }

  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_EXSCAN_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_exscan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;

  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Exscan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res = 0;
  NBC_Schedule *schedule = NULL;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.exscan);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Exscan: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.exscan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.exscan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Exscan
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.exscan);
  return _mpc_mpi_config()->coll_algorithm_intracomm.exscan(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Exscan algorithms
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sEXSCAN | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
  }
#endif

  if(count == 0) {
    return MPI_SUCCESS;
  }

  enum {
    NBC_EXSCAN_LINEAR,
    NBC_EXSCAN_ALLGATHER
  } alg;

  int size = 0;
  _mpc_cl_comm_size(comm, &size);

  alg = NBC_EXSCAN_LINEAR;

  int res;

  switch(alg) {
    case NBC_EXSCAN_LINEAR:
      res = ___collectives_exscan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_EXSCAN_ALLGATHER:
      res = ___collectives_exscan_allgather(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

  return res;
}

/**
  \brief Execute or schedule a Exscan using the linear algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank = 0;
  int size = 0;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  int res = MPI_SUCCESS;

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      tmpbuf = sctk_malloc(ext * count);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += ext * count;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += ext * count;
      break;
  }

  const void *initial_buf = sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    initial_buf = recvbuf;
  }

  if(rank > 0) {
    // Copy data in tmpbuf
    ___collectives_copy_type(initial_buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);

    // recv data from rank-1
    ___collectives_recv_type(recvbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);

    ___collectives_barrier_type(coll_type, schedule, info);

    // reduce data
    if(rank > 1 && rank < size - 1) {
      ___collectives_op_type(NULL, recvbuf, tmpbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    }
  }

  // send data to rank+1
  if(rank + 1 < size) {
    if(rank == 0) {
      ___collectives_send_type(initial_buf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    } else {
      ___collectives_send_type(tmpbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }

  return res;
}

/**
  \brief Execute or schedule a Exscan using the allgather algorithm
    Or count the number of operations and rounds for the schedule
  \param sendbuf Adress of the pointer to the buffer used to send data
  \param recvbuf Adress of the pointer to the buffer used to receive data
  \param count Number of elements in sendbuf
  \param datatype Type of the data elements in sendbuf
  \param op Reduction operation
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      tmpbuf = sctk_malloc(ext * count * size);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += ext * count * size;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += ext * count * size;
      break;
  }

  const void *buf = sendbuf;
  if(buf == MPI_IN_PLACE) {
    buf = recvbuf;
  }


  _mpc_mpi_config()->coll_algorithm_intracomm.allgather(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  ___collectives_barrier_type(coll_type, schedule, info);

  if(rank > 0) {
    int i;
    for(i = 1; i < rank; i++) {
      ___collectives_op_type(NULL, tmpbuf + (i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
    }

    ___collectives_copy_type(tmpbuf + (rank - 1) * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }
  return MPI_SUCCESS;
}




/***********
  BARRIER
  ***********/


/**
  \brief Initialize NBC structures used in call of non-blocking Barrier
  \param comm Target communicator
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Ibarrier (MPI_Comm comm, MPI_Request *request) {

  if( _mpc_mpi_config()->nbc.use_egreq_barrier )
  {
    return MPI_Ixbarrier( comm, request );
  }

  mpc_common_nodebug ("Entering IBARRIER %d", comm);
  int res = MPI_ERR_INTERN;
  SCTK__MPI_INIT_REQUEST (request);
  int csize;
  MPI_Comm_size(comm, &csize);

  if(csize == 1)
  {
    res = PMPI_Barrier(comm);
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(tmp);
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    mpc_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp = *request = mpc_lowcomm_request_alloc();
    //tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = ___collectives_ibarrier (comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Barrier
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_ibarrier (MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_NONBLOCKING, _mpc_mpi_config()->coll_opts.topo.barrier);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Ibarrier: NBC_Init_handle failed");

  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.barrier(comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.barrier(comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (MPI_SUCCESS != res)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (MPI_SUCCESS != res)
  {
    printf("Error in NBC_Start() (%i)\n", res);
    return res;
  }

  return MPI_SUCCESS;
}


/**
  \brief Initialize NBC structures used in call of persistent Barrier
  \param comm Target communicator
  \param info MPI_Info
  \param request Pointer to the MPI_Request
  \return error code
  */
int PMPI_Barrier_init (MPI_Comm comm, MPI_Info info, MPI_Request *request) {

  {
    mpi_check_comm(comm);
  }

  MPI_internal_request_t *req = *request = mpc_lowcomm_request_alloc();
  mpc_lowcomm_request_t *mpc_req = _mpc_cl_get_lowcomm_request(req);
  //SCTK__MPI_INIT_REQUEST (request);
  //req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  mpc_req->request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_BARRIER_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  ___collectives_barrier_init (comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;

  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Barrier
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int ___collectives_barrier_init (MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_PERSISTENT, _mpc_mpi_config()->coll_opts.topo.barrier);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  MPI_HANDLE_ERROR(res, comm, "Barrier: NBC_Init_handle failed");

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  _mpc_mpi_config()->coll_algorithm_intracomm.barrier(comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  ___collectives_sched_alloc_init(handle, schedule, &info);

  _mpc_mpi_config()->coll_algorithm_intracomm.barrier(comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = ___collectives_sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in ___collectives_sched_commit() (%i)\n", res);
    return res;
  }

  handle->schedule = schedule;

  return MPI_SUCCESS;
}


/**
  \brief Blocking Exscan
  \param comm Target communicator
  \return error code
  */
int _mpc_mpi_collectives_barrier (MPI_Comm comm) {

  Sched_info info;
  ___collectives_sched_info_init(&info, MPC_COLL_TYPE_BLOCKING, _mpc_mpi_config()->coll_opts.topo.barrier);
  return _mpc_mpi_config()->coll_algorithm_intracomm.barrier(comm, MPC_COLL_TYPE_BLOCKING, NULL, &info);
}


/**
  \brief Swith between the different Barrier algorithms
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_barrier_switch (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  {
    int rank = -1, size;
    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &size);
    int global_rank = -1;
    _mpc_cl_comm_rank(MPI_COMM_WORLD, &global_rank);
    mpc_common_debug("%sBARRIER | %d / %d (%d)", __debug_indentation, rank, size, global_rank);
    strncat(__debug_indentation, "\t", DBG_INDENT_LEN - 1);
  }
#endif

  enum {
    NBC_BARRIER_REDUCE_BROADCAST
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);

  alg = NBC_BARRIER_REDUCE_BROADCAST;

  int res;

  switch(alg) {
    case NBC_BARRIER_REDUCE_BROADCAST:
      res = ___collectives_barrier_reduce_broadcast (comm, coll_type, schedule, info);
      break;
  }

#ifdef MPC_COLL_EXTRA_DEBUG_ENABLED
  __debug_indentation[strlen(__debug_indentation) - 1] = '\0';
#endif

  return res;
}

/**
  \brief Execute or schedule a Barrier using the reduce_broadcast algorithm
    Or count the number of operations and rounds for the schedule
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
int ___collectives_barrier_reduce_broadcast (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  static char bar = 0;

  int size, rank;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  unsigned int maxr = ___collectives_ceiled_log2(size);
  int peer = 0;

  unsigned int rmb = ___collectives_rmb_index(rank);
  if(rank == 0) {
    rmb = maxr;
  }

  // Reduce | Broadcast

  if(rank != 0) {
    peer = rank ^ (1 << rmb);
    ___collectives_recv_type(&bar, 1, MPI_CHAR, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

    if(rmb != 0) {
      ___collectives_barrier_type(coll_type, schedule, info);
    }
  }
  int i = 0;
  for(i = (int)rmb - 1; i >= 0; i--) {
    peer = rank ^ (1 << i);
    if(peer >= size) {
      continue;
    }

    ___collectives_send_type(& bar, 1, MPI_CHAR, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }


  ___collectives_barrier_type(coll_type, schedule, info);


  for( i = 0; i < (int)maxr; i++) {
    if(rank & (1 << i)) {
      peer = rank ^ (1 << i);
      ___collectives_barrier_type(coll_type, schedule, info);
      ___collectives_send_type(&bar, 1, MPI_CHAR, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);

      break;
    }

    if((rank | (1 << i)) < size) {
      peer = rank | (1 << i);
      ___collectives_recv_type(&bar, 1, MPI_CHAR, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  }

  return MPI_SUCCESS;
}
