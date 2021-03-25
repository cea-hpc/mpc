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
/* #                                                                      # */
/* ######################################################################## */



#ifndef __COLL_H__
#define __COLL_H__

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
  int pos;                    /**< Current offset from the starting adress of the schedule. */

  int round_start_pos;        /**< Offset from the starting adress of the schedule to the starting adress of the current round. */
  int round_comm_count;       /**< Number of operations in the current round. */

  int round_count;            /**< Number of rounds in the schedule. */
  int comm_count;             /**< Number of operations in the schedule. */
  int alloc_size;             /**< Allocation size for the schedule. */

  void *tmpbuf;               /**< Adress of the temporary buffer for the schedule. */
  int tmpbuf_size;            /**< Size of the temporary buffer for the schedule. */
  int tmpbuf_pos;             /**< Offset from the staring adress of the temporary buffer to the current position. */
} Sched_info;



int __INTERNAL__Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
int __INTERNAL__Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);



#endif
