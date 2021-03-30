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

#include <stdarg.h>
#include <mpc_mpi_internal.h>
#include <mpc_coll_weak.h>

#include "egreq_nbc.h"

#define LOG2 0.69314718055994530941


#define SCHED_SIZE (sizeof(int))
#define BARRIER_SCHED_SIZE (sizeof(char))
#define COMM_SCHED_SIZE (sizeof(NBC_Fn_type) + sizeof(NBC_Args))
#define ROUND_SCHED_SIZE (SCHED_SIZE + BARRIER_SCHED_SIZE)

// TODO : error handling & config handling
#define SCTK_MPI_CHECK_RETURN_VAL(res,comm)do{if(res == MPI_SUCCESS){return res;} else {MPI_ERROR_REPORT(comm,res,"Generic error retrun");}}while(0)



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


#define pointer_swap(a, b, swap) \
{                                \
  swap = (a);                    \
  a = (b);                       \
  b = (swap);                    \
}

#ifndef RANK2VRANK
#define RANK2VRANK(rank, vrank, root) \
{ \
  vrank = (rank); \
  if ((rank) == 0) vrank = (root); \
  if ((rank) == (root)) vrank = 0; \
}
#endif

#ifndef VRANK2RANK
#define VRANK2RANK(rank, vrank, root) \
{ \
  rank = (vrank); \
  if ((vrank) == 0) rank = (root); \
  if ((vrank) == (root)) rank = 0; \
}
#endif

/**
  \brief Find the index of the first bit set to 1 in an integer
  \param a Integer
  \return Index of the first bit set to 1 int the input integer
  */
static inline int rmb_index(int a) {
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
static inline int fill_rmb(int a, int index) {
  return a | ((1 << index) - 1);
}

/**
  \brief Initialise the Sched_info struct with default values
  \param info The adress of the Sched_info struct to initialise
  */
void sched_info_init(Sched_info *info) {
  info->pos = 2 * sizeof(int);

  info->round_start_pos = sizeof(int);
  info->round_comm_count = 0;

  info->round_count = 1;
  info->comm_count = 0;
  info->alloc_size = 0;

  info->tmpbuf_size = 0;
  info->tmpbuf_pos = 0;
}

/**
  \brief Handle the allocation of the schedule and the temporary buffer
  \param handle Handle containing the temporary buffer to allocate
  \param schedule Adress of the schedule to allocate
  \param info Information about the function to schedule
  \return Error code
  */
int sched_alloc_init(NBC_Handle *handle, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int my_NBC_Sched_send(const void *buffer, int count, MPI_Datatype datatype, int dest, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* send_args;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = SEND;

  /* store the passed arguments */
  send_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));

  send_args->buf=buffer;
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
static inline int my_NBC_Sched_recv(void *buffer, int count, MPI_Datatype datatype, int source, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* recv_args;

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
static inline int my_NBC_Sched_barrier(NBC_Schedule *schedule, Sched_info *info) {
  *(char *)((char *)*schedule + info->pos) = 1;

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
static inline int my_NBC_Sched_commit(NBC_Schedule *schedule, Sched_info *info)
{

  *(int *)((char*)*schedule) = info->alloc_size;
  *(int *)((char *)*schedule + info->round_start_pos) = info->round_comm_count;

  /* add the barrier char (0) because this is the last round */
  *(char *)((char *)*schedule + info->pos) = 0;

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
static inline int my_NBC_Sched_op(void *res_buf, void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args* op_args;
  
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
static inline int my_NBC_Sched_copy(void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info) {

  NBC_Args *copy_args;

  /* adjust the function type */
  *(NBC_Fn_type *)((char *)*schedule + info->pos) = COPY;

  /* store the passed arguments */
  copy_args = (NBC_Args *)((char *)*schedule + info->pos + sizeof(NBC_Fn_type));
  copy_args->src = src;
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
static inline int __INTERNAL__Send_type(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_send(buffer, count, datatype, dest, tag, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = my_NBC_Sched_send(buffer, count, datatype, dest, comm, schedule, info);
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
static inline int __INTERNAL__Recv_type(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;
  
  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_recv(buffer, count, datatype, source, tag, comm, MPI_STATUS_IGNORE);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = my_NBC_Sched_recv(buffer, count, datatype, source, comm, schedule, info);
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
static inline int __INTERNAL__Barrier_type(MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = my_NBC_Sched_barrier(schedule, info);
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
static inline int __INTERNAL__Op_type( __UNUSED__ void *res_buf, const void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, sctk_Op mpc_op, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(mpc_op.u_func != NULL)
      {
        mpc_op.u_func(left_op_buf, right_op_buf, &count, &datatype);
      }
      else
      {
        sctk_Op_f func;
        func = sctk_get_common_function(datatype, mpc_op);
        func(left_op_buf, right_op_buf, count, datatype);
      }
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = my_NBC_Sched_op(right_op_buf, left_op_buf, right_op_buf, count, datatype, op, schedule, info);
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
static inline int __INTERNAL__Copy_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      NBC_Copy(src, srccount, srctype, tgt, tgtcount, tgttype, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      my_NBC_Sched_copy(src, srccount, srctype, tgt, tgtcount, tgttype, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}


/***************************
  switch function prototype
  ***************************/

static inline int __INTERNAL__Bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);




/***********
  BROADCAST
  ***********/

int PMPI_Ibcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
static inline int __INTERNAL__Bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Bcast_scatter_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
  
  if( sctk_runtime_config_get()->modules.nbc.use_egreq_bcast )
  {
    return MPI_Ixbcast( buffer, count, datatype, root, comm, request );
  }


  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering IBCAST %d with count %d", comm, count);
  SCTK__MPI_INIT_REQUEST (request);
  int csize;
  MPI_Comm_size(comm, &csize);
  if(csize == 1)
  {
    res = PMPI_Bcast (buffer, count, datatype, root, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Ibcast(buffer, count, datatype, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ibcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle ) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
    int size;
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_BCAST_INIT;
  req->persistant.info = info;

  __INTERNAL__Bcast_init(buffer, count, datatype, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle)
{
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  return __INTERNAL__Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __INTERNAL__Bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_BCAST_LINEAR,
    NBC_BCAST_BINOMIAL,
    NBC_BCAST_SCATTER_ALLGATHER
  } alg;

  alg = NBC_BCAST_BINOMIAL;

  int res;

  switch(alg) {
    case NBC_BCAST_LINEAR:
      res = __INTERNAL__Bcast_linear(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_BINOMIAL:
      res = __INTERNAL__Bcast_binomial(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_SCATTER_ALLGATHER:
      res = __INTERNAL__Bcast_scatter_allgather(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
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


  if(rank != root) {
    res = __INTERNAL__Recv_type(buffer, count, datatype, root, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __INTERNAL__Send_type(buffer, count, datatype, i, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size, rank;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int maxr, vrank, peer;
  maxr = (int)ceil((log(size)/LOG2));
  RANK2VRANK(rank, vrank, root);
  int rmb = rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  if(rank != root) {
    VRANK2RANK(peer, vrank ^ (1 << rmb), root);

    __INTERNAL__Recv_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

    if(rmb != 0) {
      __INTERNAL__Barrier_type(coll_type, schedule, info);
    }
  }

  for(int i = rmb - 1; i >= 0; i--) {
    VRANK2RANK(peer, vrank | (1 << i), root);
    if(peer >= size) {
      continue;
    }

    __INTERNAL__Send_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Bcast_scatter_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}





/***********
  REDUCE
  ***********/

int PMPI_Ireduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
static inline int __INTERNAL__Reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_reduce_scatter_allgather(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
  
  if( sctk_runtime_config_get()->modules.nbc.use_egreq_reduce )
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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Reduce_init (sendbuf, recvbuf, count, datatype, op, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  return __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  enum {
    NBC_REDUCE_LINEAR,
    NBC_REDUCE_BINOMIAL, /* /!\ respecte la commutativité, non l'associativité /!\ */
    NBC_REDUCE_REDUCE_SCATTER_ALLGATHER
  } alg;

  alg = NBC_REDUCE_LINEAR;

  int res;

  switch(alg) {
    case NBC_REDUCE_LINEAR:
      res = __INTERNAL__Reduce_linear(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_BINOMIAL:
      res = __INTERNAL__Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_REDUCE_SCATTER_ALLGATHER:
      res = __INTERNAL__Reduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
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
      if(rank == root) {
        mpi_op = sctk_convert_to_mpc_op(op);
        mpc_op = mpi_op->op;
        tmpbuf = sctk_malloc(ext * count * size);
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(rank == root) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += ext * count * size;
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == root) {
        info->tmpbuf_size += ext * count * size;
      }
      break;
  }

  if(sendbuf != MPI_IN_PLACE) {
    __INTERNAL__Gather_switch(sendbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  } else {
    __INTERNAL__Gather_switch(recvbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  }

  if(rank == root) {
    __INTERNAL__Barrier_type(coll_type, schedule, info);

    for(int i = 1; i < size; i++) {
      __INTERNAL__Op_type(NULL, (char*)tmpbuf + (i-1)*ext*count, (char*)tmpbuf + i*ext*count, count, datatype, op, mpc_op, coll_type, schedule, info);
    }
    
    __INTERNAL__Copy_type((char*)tmpbuf + (size-1)*ext*count, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING && rank == root) {
    free(tmpbuf);
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
static inline int __INTERNAL__Reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
 
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

  int vrank, vroot, peer, maxr;
  
  vroot = root;
  if(!can_commute) {
    vroot = 0;
  }
  
  RANK2VRANK(rank, vrank, vroot);
  
  maxr = (int)ceil((log(size)/LOG2));

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(2 * ext * count);
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
    tmp_sendbuf = sendbuf;
  }

  for(int i = 0; i < maxr; i++) {
    if(vrank & (1 << i)) {
      VRANK2RANK(peer, vrank ^ (1 << i), vroot);
     
      __INTERNAL__Send_type(tmp_sendbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      
      break;
    } else if((vrank | (1 << i)) < size) {
      VRANK2RANK(peer, vrank | (1 << i), vroot);
      
      __INTERNAL__Recv_type(tmp_recvbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Barrier_type(coll_type, schedule, info);
      __INTERNAL__Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

      if(first_access) {
        tmp_sendbuf = tmpbuf + ext * count;
        first_access = 0;
      }

      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    }
  }

  if(!can_commute) {
    if(root != 0) {
      if(rank == 0) {
        __INTERNAL__Send_type(tmp_sendbuf, count, datatype, root, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      } else if(rank == root) {
        __INTERNAL__Recv_type(recvbuf, count, datatype, 0, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      }
    }
  } else if (rank == root){
    __INTERNAL__Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }



  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
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
static inline int __INTERNAL__Reduce_reduce_scatter_allgather(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
 
  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLREDUCE
  ***********/

int PMPI_Iallreduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __INTERNAL__Allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allreduce_vector_halving_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allreduce_ring(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Iallreduce (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLREDUCE_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Allreduce_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
int __INTERNAL__Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __INTERNAL__Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  enum {
    NBC_ALLREDUCE_REDUCE_BROADCAST,
    NBC_ALLREDUCE_DISTANCE_DOUBLING,
    NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING, //need reduce_scatter + allgather
    NBC_ALLREDUCE_BINARY_BLOCK, // need reduce_scatter + allgather
    NBC_ALLREDUCE_RING
  } alg;

  alg = NBC_ALLREDUCE_DISTANCE_DOUBLING;

  int res;

  switch(alg) {
    case NBC_ALLREDUCE_REDUCE_BROADCAST:
      res = __INTERNAL__Allreduce_reduce_broadcast(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_DISTANCE_DOUBLING:
      res = __INTERNAL__Allreduce_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING:
      res = __INTERNAL__Allreduce_vector_halving_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_BINARY_BLOCK:
      res = __INTERNAL__Allreduce_binary_block(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_RING:
      res = __INTERNAL__Allreduce_ring(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  __INTERNAL__Reduce_switch(sendbuf, recvbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  __INTERNAL__Barrier_type(coll_type, schedule, info);
  __INTERNAL__Bcast_switch(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);

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
static inline int __INTERNAL__Allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
  maxr = (int)floor((log(size)/LOG2));
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
    __INTERNAL__Copy_type(sendbuf, count, datatype, tmp_sendbuf, count, datatype, comm, coll_type, schedule, info);
    first_access = 0;
  }



  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      __INTERNAL__Send_type(sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Barrier_type(coll_type, schedule, info);
      __INTERNAL__Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

      if(first_access) {
        tmp_sendbuf = tmpbuf + ext * count;
        first_access = 0;
      }

      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    }
  }

  if(rank >= group || !(rank & 1)) {
    for(int i = 0; i < maxr; i++) {
      
      peer = vrank ^ (1 << i);
      if(peer < group / 2) {
        peer = peer * 2;
      } else {
        peer = peer + group / 2;
      }

      if(peer > rank) {
        __INTERNAL__Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __INTERNAL__Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      } else {
        __INTERNAL__Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __INTERNAL__Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }
      __INTERNAL__Barrier_type(coll_type, schedule, info);

      if(vrank & (1 << i)) {
        __INTERNAL__Op_type(NULL, tmp_recvbuf, tmp_sendbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      } else {
        __INTERNAL__Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

        if(first_access) {
          tmp_sendbuf = tmpbuf + ext * count;
          first_access = 0;
        }
      
        pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
      }
    }
  }

  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      __INTERNAL__Recv_type(recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
    }
  } else {
    __INTERNAL__Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf);
  }
  
  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a reduce using the vector halving distance doubling algorithm
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
static inline int __INTERNAL__Allreduce_vector_halving_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
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
static inline int __INTERNAL__Allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
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
static inline int __INTERNAL__Allreduce_ring(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}




/***********
  SCATTER
  ***********/

int PMPI_Iscatter (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __INTERNAL__Scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
  
  if( sctk_runtime_config_get()->modules.nbc.use_egreq_scatter )
  {
    return MPI_Ixscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
  }

  int res = MPI_ERR_INTERN;
  mpc_common_nodebug ("Entering ISCATTER %d", comm);
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
    res = PMPI_Scatter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
    int size;
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCATTER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Scatter_init(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __INTERNAL__Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __INTERNAL__Scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_SCATTER_LINEAR,
    NBC_SCATTER_BINOMIAL
  } alg;

  alg = NBC_SCATTER_BINOMIAL;

  int res;

  switch(alg) {
    case NBC_SCATTER_LINEAR:
      res = __INTERNAL__Scatter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTER_BINOMIAL:
      res = __INTERNAL__Scatter_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &ext);

  int res = MPI_SUCCESS;

  void *tmp_sendbuf = NULL;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  } else {
    tmp_sendbuf = sendbuf;
  }

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


  if(rank != root) {
    res = __INTERNAL__Recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __INTERNAL__Send_type(tmp_sendbuf + i * ext * sendcount, sendcount, sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __INTERNAL__Copy_type(tmp_sendbuf + rank * ext * sendcount, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  int maxr, vrank, peer, peer_vrank;
  maxr = (int)ceil((log(size)/LOG2));
  RANK2VRANK(rank, vrank, root);
  
  int rmb = rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  int range = fill_rmb(vrank, rmb);

  int peer_range, count;

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * recvext * recvcount);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * recvext * recvcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += size * recvext * recvcount;
      break;
  }

  if(rank == root) {
    __INTERNAL__Copy_type(sendbuf, sendcount * size, sendtype, tmpbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);

    if(root != 0) {
      __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, tmpbuf + root * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
      __INTERNAL__Copy_type(sendbuf + root * sendcount * sendext, sendcount, sendtype, tmpbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  if(vrank != 0) {
    VRANK2RANK(peer, vrank ^ (1 << rmb), root);

    if(range < size) {
      count = (1 << rmb) * recvcount;
    } else {
      count = (size - peer) * recvcount;
    }

    __INTERNAL__Recv_type(tmpbuf, count, recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    __INTERNAL__Barrier_type(coll_type, schedule, info);
  }

  for(int i = rmb - 1; i >= 0; i--) {
    peer_vrank = vrank | (1 << i);

    VRANK2RANK(peer, peer_vrank, root);
    if(peer >= size) {
      continue;
    }

    peer_range = fill_rmb(peer_vrank, i);
    if(peer_range < size) {
      count = (1 << i) * recvcount;
    } else {
      count = (size - peer_vrank) * recvcount;
    }

    __INTERNAL__Send_type(tmpbuf + (1 << i) * recvcount * recvext, count, recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }

  __INTERNAL__Copy_type(tmpbuf, recvcount, recvtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);




  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
  }

  return MPI_SUCCESS;
}




/***********
  SCATTERV
  ***********/

int PMPI_Iscatterv (const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request);
int __INTERNAL__Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __INTERNAL__Scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scatterv_binomial(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Iscatterv (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCATTERV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Scatterv_init (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __INTERNAL__Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __INTERNAL__Scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_SCATTERV_LINEAR,
    NBC_SCATTERV_BINOMIAL
  } alg;

  alg = NBC_SCATTERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_SCATTERV_LINEAR:
      res = __INTERNAL__Scatterv_linear(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTERV_BINOMIAL:
      res = __INTERNAL__Scatterv_binomial(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &ext);

  int res = MPI_SUCCESS;

  void *tmp_sendbuf = NULL;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  } else {
    tmp_sendbuf = sendbuf;
  }

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


  if(rank != root) {
    res = __INTERNAL__Recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __INTERNAL__Send_type(tmp_sendbuf + displs[i], sendcounts[i], sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __INTERNAL__Copy_type(tmp_sendbuf + displs[rank], sendcounts[rank], sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Scatterv_binomial(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}




/***********
  GATHER
  ***********/

int PMPI_Igather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __INTERNAL__Gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
  
  if( sctk_runtime_config_get()->modules.nbc.use_egreq_gather )
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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_GATHER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Gather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_GATHER_LINEAR,
    NBC_GATHER_BINOMIAL
  } alg;

  alg = NBC_GATHER_BINOMIAL;

  int res;

  switch(alg) {
    case NBC_GATHER_LINEAR:
      res = __INTERNAL__Gather_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHER_BINOMIAL:
      res = __INTERNAL__Gather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  if(rank != root) {
    res = __INTERNAL__Send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __INTERNAL__Recv_type(recvbuf + i * ext * recvcount, recvcount, recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * ext * recvcount, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  int maxr, vrank, peer, peer_vrank;
  maxr = (int)ceil((log(size)/LOG2));
  RANK2VRANK(rank, vrank, root);
  
  int rmb = rmb_index(vrank);
  int range = fill_rmb(vrank, rmb);

  int peer_range, count;

  void *tmpbuf = NULL;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(size * sendext * sendcount);
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += size * sendext * sendcount;
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += size * sendext * sendcount;
      break;
  }

  __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, tmpbuf, sendcount, sendtype, comm, coll_type, schedule, info);


  for(int i = 0; i < maxr; i++) {
    peer_vrank = vrank ^ (1 << i);
    VRANK2RANK(peer, peer_vrank, root);

    if(vrank & (1 << i)) {
      
      if(range < size) {
        count = (1 << i) * sendcount;
      } else {
        count = (size - vrank) * sendcount;
      }

      __INTERNAL__Barrier_type(coll_type, schedule, info);
      __INTERNAL__Send_type(tmpbuf, count, sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      
      break;
    } else if(peer < size) {
      
      peer_range = fill_rmb(peer_vrank, i);
      if(peer_range < size) {
        count = (1 << i) * sendcount;
      } else {
        count = (size - peer_vrank) * sendcount;
      }

      __INTERNAL__Recv_type(tmpbuf + (1 << i) * sendcount * sendext, count, sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  }

  if(rank == root) {
    __INTERNAL__Barrier_type(coll_type, schedule, info);
    
    __INTERNAL__Copy_type(tmpbuf, sendcount * size, sendtype, recvbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);
    
    if(root != 0) {
      __INTERNAL__Copy_type(tmpbuf, sendcount, sendtype, recvbuf + root * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
      __INTERNAL__Copy_type(tmpbuf + root * sendcount * sendext, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
  }

  return MPI_SUCCESS;
}




/***********
  GATHERV
  ***********/

int PMPI_Igatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __INTERNAL__Gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Gatherv_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
int PMPI_Igatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) {
  
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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request) {
  
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_GATHERV_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Gatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(req->nbc_handle));
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
int __INTERNAL__Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_GATHERV_LINEAR,
    NBC_GATHERV_BINOMIAL
  } alg;

  alg = NBC_GATHERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_GATHERV_LINEAR:
      res = __INTERNAL__Gatherv_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHERV_BINOMIAL:
      res = __INTERNAL__Gatherv_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  if(rank != root) {
    res = __INTERNAL__Send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __INTERNAL__Recv_type(recvbuf + displs[i], recvcounts[i], recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, recvbuf + displs[rank], recvcounts[rank], recvtype, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Gatherv_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER BLOCK
  ***********/

int PMPI_Ireduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ireduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __INTERNAL__Reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_block_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_block_distance_halving(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Ireduce_scatter_block (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ireduce_scatter_block (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_SCATTER_BLOCK_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Reduce_scatter_block_init (sendbuf,  recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
int __INTERNAL__Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __INTERNAL__Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  enum {
    NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER,
    NBC_REDUCE_SCATTER_BLOCK_DISTANCE_DOUBLING, // difficile a mettre en place
    NBC_REDUCE_SCATTER_BLOCK_DISTANCE_HALVING, // necessite operateur commutatif & associatif
    NBC_REDUCE_SCATTER_BLOCK_PAIRWISE // seulement en non bloquant
  } alg;

  alg = NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER;

  int res;

  switch(alg) {
    case NBC_REDUCE_SCATTER_BLOCK_REDUCE_SCATTER:
      res = __INTERNAL__Reduce_scatter_block_reduce_scatter(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_DOUBLING:
      res = __INTERNAL__Reduce_scatter_block_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_HALVING:
      res = __INTERNAL__Reduce_scatter_block_distance_halving(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_PAIRWISE:
      res = __INTERNAL__Reduce_scatter_block_pairwise(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    __INTERNAL__Reduce_switch(sendbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info); 
  } else {
    __INTERNAL__Reduce_switch(recvbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info); 
  }

  __INTERNAL__Barrier_type(coll_type, schedule, info);

  __INTERNAL__Scatter_switch(tmpbuf, count, datatype, recvbuf, count, datatype, 0, comm, coll_type, schedule, info);


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
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
static inline int __INTERNAL__Reduce_scatter_block_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Reduce_scatter_block_distance_halving(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  if(sendbuf == MPI_IN_PLACE) {
    __INTERNAL__Copy_type(recvbuf, count, datatype, tmp_left_resbuf, count, datatype, comm, coll_type, schedule, info);
  } else {
    __INTERNAL__Copy_type(sendbuf, count, datatype, tmp_left_resbuf, count, datatype, comm, coll_type, schedule, info);
  }


  for(int i = 1; i < size; i++) {
    send_peer = (rank + i) % size;
    recv_peer = (rank - i + size) % size;
    
    __INTERNAL__Send_type(sendbuf + send_peer * ext * count, count, datatype, send_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);
    __INTERNAL__Recv_type(tmp_recvbuf, count, datatype, recv_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);

    __INTERNAL__Barrier_type(coll_type, schedule, info);
    
    if(recv_peer < rank) { 
      __INTERNAL__Op_type(NULL, tmp_recvbuf, tmp_left_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    } else {
      if(first_access) {
        swap = tmp_recvbuf;
        tmp_recvbuf = tmp_right_resbuf;
        tmp_right_resbuf = swap;

        first_access = 0;
      } else {
        __INTERNAL__Op_type(NULL, tmp_recvbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      }
    }
  }

  if(first_access) {
    __INTERNAL__Copy_type(tmp_left_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  } else {
    __INTERNAL__Op_type(NULL, tmp_left_resbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    __INTERNAL__Copy_type(tmp_right_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }
  
  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER
  ***********/

int PMPI_Ireduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ireduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __INTERNAL__Reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_distance_doubling(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_distance_halving(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Reduce_scatter_pairwise(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = __INTERNAL__Ireduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ireduce_scatter (const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_REDUCE_SCATTER_INIT;
  req->persistant.info = info;

  /* Init metadat for nbc */
  __INTERNAL__Reduce_scatter_init (sendbuf,  recvbuf, recvcounts, datatype, op, comm, &(req->nbc_handle));
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
int __INTERNAL__Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __INTERNAL__Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
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
      res = __INTERNAL__Reduce_scatter_reduce_scatterv(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_DOUBLING:
      res = __INTERNAL__Reduce_scatter_distance_doubling(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_HALVING:
      res = __INTERNAL__Reduce_scatter_distance_halving(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_PAIRWISE:
      res = __INTERNAL__Reduce_scatter_pairwise(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void *tmpbuf = NULL;
  int *displs = NULL;

  int count = 0;
  for(int i = 0; i < size; i++) {
    count += recvcounts[i];
  }

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      if(rank == 0) {
        tmpbuf = sctk_malloc(ext * count + size * sizeof(int));
        displs = tmpbuf + ext * count;
      }
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(rank == 0) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        displs = tmpbuf + ext * count;
        info->tmpbuf_pos += ext * count + size * sizeof(int);
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      if(rank == 0) {
        info->tmpbuf_size += ext * count + size * sizeof(int);
      }
      break;
  }

  for(int i = 0; i < size; i++) {
    displs[i] = i * ext;
  }

  if(sendbuf != MPI_IN_PLACE) {
    __INTERNAL__Reduce_switch(sendbuf, tmpbuf, count, datatype, op, 0, comm, coll_type, schedule, info); 
  } else {
    __INTERNAL__Reduce_switch(recvbuf, tmpbuf, count, datatype, op, 0, comm, coll_type, schedule, info); 
  }

  __INTERNAL__Barrier_type(coll_type, schedule, info);

  __INTERNAL__Scatterv_switch(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts, datatype, 0, comm, coll_type, schedule, info);


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
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
static inline int __INTERNAL__Reduce_scatter_distance_doubling(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Reduce_scatter_distance_halving(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Reduce_scatter_pairwise(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}




/***********
  ALLGATHER
  ***********/

int PMPI_Iallgather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __INTERNAL__Allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;

    tmp->nbc_handle.is_persistent = 0;
    res =  __INTERNAL__Iallgather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLGATHER_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Allgather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
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
int __INTERNAL__Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  return __INTERNAL__Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLGATHER_GATHER_BROADCAST,
    NBC_ALLGATHER_DISTANCE_DOUBLING,
    NBC_ALLGATHER_BRUCK,
    NBC_ALLGATHER_RING
  } alg;

  alg = NBC_ALLGATHER_DISTANCE_DOUBLING;

  int res;

  switch(alg) {
    case NBC_ALLGATHER_GATHER_BROADCAST:
      res = __INTERNAL__Allgather_gather_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_DISTANCE_DOUBLING:
      res = __INTERNAL__Allgather_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_BRUCK:
      res = __INTERNAL__Allgather_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_RING:
      res = __INTERNAL__Allgather_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size;
  _mpc_cl_comm_size(comm, &size);

 __INTERNAL__Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm, coll_type, schedule, info);
 __INTERNAL__Barrier_type(coll_type, schedule, info);
 __INTERNAL__Bcast_switch(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);

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
static inline int __INTERNAL__Allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  int res = MPI_SUCCESS;

  int vrank, vsize, peer, maxr, count = 0, peer_count = 0;
  maxr = (int)floor((log(size)/LOG2));
  vsize = (1 << maxr);
  int group = 2 * (size - (1 << maxr));
 
  if(rank < group) {
    vrank = rank / 2;
  } else {
    vrank = rank - group / 2;
  }

  void *tmpbuf = recvbuf + rank * recvcount * recvext;
  void *tmpbuf2 = NULL;
  int *countbuf = NULL, *prev_countbuf = NULL;
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


  if(coll_type != MPC_COLL_TYPE_COUNT) {
    for(int i = 0; i < vsize; i++) {
      if(i < group / 2) {
        countbuf[i] = 2;
      } else {
        countbuf[i] = 1;
      }
    }
  }


  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      __INTERNAL__Send_type(sendbuf, sendcount, sendtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Recv_type(recvbuf + peer * recvcount * recvext, recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Barrier_type(coll_type, schedule, info);
    }
  }
  
  if(rank > group || !(rank & 1)) {
  
    __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, tmpbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    
    for(int i = 0; i < maxr; i++) {
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


      if(peer < rank) {
        __INTERNAL__Send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        __INTERNAL__Recv_type(tmpbuf - peer_count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);

        tmpbuf -= peer_count * recvcount * recvext;
      } else {
        __INTERNAL__Recv_type(tmpbuf + count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        __INTERNAL__Send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      }

      __INTERNAL__Barrier_type(coll_type, schedule, info);

      if(coll_type != MPC_COLL_TYPE_COUNT) {
        pointer_swap(countbuf, prev_countbuf, swap);
        for(int j = 0; j < vsize; j++) {
          peer = j ^ (1 << i);
          countbuf[j] = prev_countbuf[j] + prev_countbuf[peer];
        }
      }
    }
  }



  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      __INTERNAL__Recv_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Send_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    sctk_free(tmpbuf2);
  }

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Allgather using the bruck algorithm
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
static inline int __INTERNAL__Allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

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
static inline int __INTERNAL__Allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  __INTERNAL__Copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);

  for(int i = 0; i < size - 1; i++) {
    if(rank & 1) {
      __INTERNAL__Recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    }
    
    __INTERNAL__Barrier_type(coll_type, schedule, info);
  }

  return MPI_SUCCESS;
}




/***********
  ALLGATHERV
  ***********/

int PMPI_Iallgatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __INTERNAL__Allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgatherv_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Allgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res =  __INTERNAL__Iallgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
    
    for(int i = 0; i < size; i++)
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLGATHERV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Allgatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(req->nbc_handle));
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
int __INTERNAL__Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm) {
  return __INTERNAL__Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __INTERNAL__Allgatherv_gatherv_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_DISTANCE_DOUBLING:
      res = __INTERNAL__Allgatherv_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_BRUCK:
      res = __INTERNAL__Allgatherv_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_RING:
      res = __INTERNAL__Allgatherv_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int count = 0;
  int size;
  _mpc_cl_comm_size(comm, &size);

  for(int i = 0; i < size; i++) {
    count += recvcounts[i];
  }

 __INTERNAL__Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, 0, comm, coll_type, schedule, info);
 __INTERNAL__Barrier_type(coll_type, schedule, info);
 __INTERNAL__Bcast_switch(recvbuf, count, recvtype, 0, comm, coll_type, schedule, info);

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
static inline int __INTERNAL__Allgatherv_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Allgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
static inline int __INTERNAL__Allgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALL
  ***********/

int PMPI_Ialltoall (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __INTERNAL__Alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
  if(csize == 1)
  {
    res = PMPI_Alltoall (sendbuf, sendcount, sendtype, recvbuf,
        recvcount, recvtype, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    res = __INTERNAL__Ialltoall (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALL_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Alltoall_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
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
int __INTERNAL__Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  //return __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);


  MPI_Request req;
  MPI_Status status;

  MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &req);
  MPI_Wait(&req, &status);

  return status.MPI_ERROR;
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
static inline int __INTERNAL__Alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLTOALL_CLUSTER,
    NBC_ALLTOALL_BRUCK,
    NBC_ALLTOALL_PAIRWISE
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_ALLTOALL_CLUSTER;

  int res;

  switch(alg) {
    case NBC_ALLTOALL_CLUSTER:
      res = __INTERNAL__Alltoall_cluster(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_BRUCK:
      res = __INTERNAL__Alltoall_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_PAIRWISE:
      res = __INTERNAL__Alltoall_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
  }

  return res; 
}

/**
  \brief Execute or schedule a Alltoall using the cluster algorithm
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
static inline int __INTERNAL__Alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  void *tmpbuf = NULL;
  
  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      not_implemented();
      return MPI_SUCCESS;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      if(sendbuf == MPI_IN_PLACE) {
        tmpbuf = info->tmpbuf + info->tmpbuf_pos;
        info->tmpbuf_pos += recvext * sendcount * size; 
      }
      break;

    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 2 * (size - 1) + 1;

      if(sendbuf == MPI_IN_PLACE) {
        info->tmpbuf_size += recvext * sendcount * size;
      }
      return MPI_SUCCESS;
  }

  if(sendbuf == MPI_IN_PLACE) {
    __INTERNAL__Copy_type(recvbuf, recvcount * size, recvtype, tmpbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);

    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(tmpbuf  + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(sendbuf + i * sendcount * sendext, sendcount, sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

    __INTERNAL__Copy_type(sendbuf + rank * sendcount * sendext, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);

  }

  return MPI_SUCCESS;
}

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
static inline int __INTERNAL__Alltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoall using the pairwise algorithm
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
static inline int __INTERNAL__Alltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALLV
  ***********/

int PMPI_Ialltoallv (const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __INTERNAL__Alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallv_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallv_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

  if(csize == 1)
  {
    res = PMPI_Alltoallv (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = __INTERNAL__Ialltoallv (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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
  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALLV_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Alltoallv_init (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(req->nbc_handle));
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
int __INTERNAL__Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {
  //return __INTERNAL__Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);

  MPI_Request req;
  MPI_Status status;

  MPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &req);
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
static inline int __INTERNAL__Alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLTOALLV_CLUSTER,
    NBC_ALLTOALLV_BRUCK,
    NBC_ALLTOALLV_PAIRWISE
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_ALLTOALLV_CLUSTER;

  int res;

  switch(alg) {
    case NBC_ALLTOALLV_CLUSTER:
      res = __INTERNAL__Alltoallv_cluster(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_BRUCK:
      res = __INTERNAL__Alltoallv_bruck(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_PAIRWISE:
      res = __INTERNAL__Alltoallv_pairwise(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  void *tmpbuf = NULL;

  int rsize = 0;
  for(int i = 0; i < size; i++) {
    int size = rdispls[i] + recvcounts[i] * recvext;
    rsize = (size > rsize) ? (size) : (rsize) ;
  }
  
  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      not_implemented();
      return MPI_SUCCESS;

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
    __INTERNAL__Copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(tmpbuf  + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(sendbuf + sdispls[i], sendcounts[i], sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

    __INTERNAL__Copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtype, recvbuf + rdispls[rank], recvcounts[rank], recvtype, comm, coll_type, schedule, info);

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
static inline int __INTERNAL__Alltoallv_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallv using the pairwise algorithm
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
static inline int __INTERNAL__Alltoallv_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALLW
  ***********/

int PMPI_Ialltoallw (const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm);
static inline int __INTERNAL__Alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallw_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Alltoallw_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

  if(csize == 1)
  {
    res = PMPI_Alltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = __INTERNAL__Ialltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(tmp->nbc_handle));
  }
  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_ALLTOALLW_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Alltoallw_init (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(req->nbc_handle));
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
int __INTERNAL__Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm) {

  MPI_Request req;
  MPI_Status status;

  MPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &req);
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
static inline int __INTERNAL__Alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLTOALLW_CLUSTER,
    NBC_ALLTOALLW_BRUCK,
    NBC_ALLTOALLW_PAIRWISE
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_ALLTOALLW_CLUSTER;

  int res;

  switch(alg) {
    case NBC_ALLTOALLW_CLUSTER:
      res = __INTERNAL__Alltoallw_cluster(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_BRUCK:
      res = __INTERNAL__Alltoallw_bruck(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_PAIRWISE:
      res = __INTERNAL__Alltoallw_pairwise(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  void *tmpbuf = NULL;

  int rsize = 0;
  for(int i = 0; i < size; i++) {
    PMPI_Type_extent(recvtypes[i], &ext);
    int size = rdispls[i] + recvcounts[i] * ext;
    rsize = (size > rsize) ? (size) : (rsize);
  }
  
  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      not_implemented();
      return MPI_SUCCESS;

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
    __INTERNAL__Copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(tmpbuf  + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __INTERNAL__Send_type(sendbuf + sdispls[i], sendcounts[i], sendtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __INTERNAL__Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

    __INTERNAL__Copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtypes[rank], recvbuf + rdispls[rank], recvcounts[rank], recvtypes[rank], comm, coll_type, schedule, info);

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
static inline int __INTERNAL__Alltoallw_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}

/**
  \brief Execute or schedule a Alltoallw using the pairwise algorithm
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
static inline int __INTERNAL__Alltoallw_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  SCAN
  ***********/

int PMPI_Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __INTERNAL__Scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

  int csize;
  MPI_Comm_size(comm, &csize);
  
  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;
    
    res = __INTERNAL__Iscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_SCAN_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Scan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
int __INTERNAL__Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __INTERNAL__Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_SCAN_LINEAR,
    NBC_SCAN_ALLGATHER
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_SCAN_LINEAR;

  int res;

  switch(alg) {
    case NBC_SCAN_LINEAR:
      res = __INTERNAL__Scan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_SCAN_ALLGATHER:
      res = __INTERNAL__Scan_allgather(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
  }

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
static inline int __INTERNAL__Scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
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

  if(sendbuf != MPI_IN_PLACE) {
    __INTERNAL__Copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  if(rank > 1) {
    __INTERNAL__Recv_type(tmpbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
  } else if(rank == 1) {
    __INTERNAL__Recv_type(recvbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
  }

  __INTERNAL__Barrier_type(coll_type, schedule, info);
  
  if(rank > 1) {
    __INTERNAL__Op_type(NULL, tmpbuf, recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  if(rank + 1 < size) {
    if(rank == 0) {
      if(sendbuf != MPI_IN_PLACE) {
        __INTERNAL__Send_type(sendbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
      } else {
        __INTERNAL__Send_type(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
      }
    } else {
      __INTERNAL__Send_type(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    }
  }


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
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
static inline int __INTERNAL__Scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
 
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


  __INTERNAL__Allgather_switch(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  __INTERNAL__Barrier_type(coll_type, schedule, info);

  for(int i = 1; i <= rank; i++) {
    __INTERNAL__Op_type(NULL, tmpbuf + (i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  __INTERNAL__Copy_type(tmpbuf + rank * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}




/***********
  EXSCAN
  ***********/

int PMPI_Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int __INTERNAL__Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int __INTERNAL__Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int __INTERNAL__Exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __INTERNAL__Exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __INTERNAL__Exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

  int csize;
  MPI_Comm_size(comm, &csize);

  if(recvbuf == sendbuf)
  {
    MPI_ERROR_REPORT(comm,MPI_ERR_ARG,"");
  }

  if(csize == 1)
  {
    res = PMPI_Exscan (sendbuf, recvbuf, count, datatype, op, comm);
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->req.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
  }
  else
  {
    MPI_internal_request_t *tmp;
    tmp = __sctk_new_mpc_request_internal(request, __sctk_internal_get_MPC_requests());
    tmp->is_nbc = 1;
    tmp->nbc_handle.is_persistent = 0;

    res = __INTERNAL__Iexscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
  }

  SCTK_MPI_CHECK_RETURN_VAL (res, comm);
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
int __INTERNAL__Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  sched_alloc_init(handle, schedule, &info);

  __INTERNAL__Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = my_NBC_Sched_commit(schedule, &info);
  if (NBC_OK != res)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
    return res;
  }

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res)
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_EXSCAN_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __INTERNAL__Exscan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
int __INTERNAL__Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __INTERNAL__Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  sched_alloc_init(handle, schedule, &info);
  
  __INTERNAL__Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = my_NBC_Sched_commit(schedule, &info);
  if (res != MPI_SUCCESS)
  {
    printf("Error in NBC_Sched_commit() (%i)\n", res);
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
int __INTERNAL__Exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __INTERNAL__Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __INTERNAL__Exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_EXSCAN_LINEAR,
    NBC_EXSCAN_ALLGATHER
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_EXSCAN_LINEAR;

  int res;

  switch(alg) {
    case NBC_EXSCAN_LINEAR:
      res = __INTERNAL__Exscan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_EXSCAN_ALLGATHER:
      res = __INTERNAL__Exscan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
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
static inline int __INTERNAL__Exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  if(rank > 0) {
    if(sendbuf != MPI_IN_PLACE) {
      __INTERNAL__Copy_type(sendbuf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
    } else {
      __INTERNAL__Copy_type(recvbuf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
    }
  
    __INTERNAL__Recv_type(recvbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
  }

  __INTERNAL__Barrier_type(coll_type, schedule, info);
  
  if(rank > 1 && rank < size - 1) {
    __INTERNAL__Op_type(NULL, recvbuf, tmpbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  if(rank + 1 < size) {
    if(rank == 0) {
      if(sendbuf != MPI_IN_PLACE) {
        __INTERNAL__Send_type(sendbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
      } else {
        __INTERNAL__Send_type(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
      }
    } else {
      __INTERNAL__Send_type(tmpbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    }
  }


  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
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
static inline int __INTERNAL__Exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  __INTERNAL__Allgather_switch(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  __INTERNAL__Barrier_type(coll_type, schedule, info);

  for(int i = 1; i <= rank; i++) {
    __INTERNAL__Op_type(NULL, tmpbuf + (i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  __INTERNAL__Copy_type(tmpbuf + (rank - 1) * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}



