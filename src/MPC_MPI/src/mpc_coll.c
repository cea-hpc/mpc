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

#include <mpc_mpi_internal.h>
#include <mpi_conf.h>
#include <mpc_coll_weak.h>

#include "egreq_nbc.h"


#define SCHED_SIZE (sizeof(int))
#define BARRIER_SCHED_SIZE (sizeof(char))
#define COMM_SCHED_SIZE (sizeof(NBC_Fn_type) + sizeof(NBC_Args))
#define ROUND_SCHED_SIZE (SCHED_SIZE + BARRIER_SCHED_SIZE)
#define MAX_HARDWARE_LEVEL 8

#define MAX_TOPO_SPLIT 4

//  TODO:
//    error hanfling
//    config handling
//    add reduce_scatter(_block)_Allgather(v) algorithm, may be faster than reduce then scatter

#define dbg 0


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

  int is_hardware_algo;       /**< flag set to 1 if using hardware algorithm else 0. */
  mpc_hardware_split_info_t *hardware_info_ptr; /**< ptr to structure for hardware communicator info */
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
static inline int __rmb_index(int a) {
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
static inline int __fill_rmb(int a, int index) {
  return a | ((1 << index) - 1);
}

/**
  \brief Compute the floored log2 of an integer
  \param a Input integer
  \return 
  */
static inline unsigned int __floored_log2(unsigned int a) {
  
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
static inline unsigned int __ceiled_log2(unsigned int a) {

  unsigned int n = __floored_log2(a);
  return (a != (1 << n))?(n+1):(n);
}

/**
  \brief Initialise the Sched_info struct with default values
  \param info The adress of the Sched_info struct to initialise
  */
static inline void __sched_info_init(Sched_info *info) {
  info->pos = 2 * sizeof(int);

  info->round_start_pos = sizeof(int);
  info->round_comm_count = 0;

  info->round_count = 1;
  info->comm_count = 0;
  info->alloc_size = 0;

  info->tmpbuf_size = 0;
  info->tmpbuf_pos = 0;

  info->is_hardware_algo = 0;
  info->hardware_info_ptr = NULL;
}

/**
  \brief Handle the allocation of the schedule and the temporary buffer
  \param handle Handle containing the temporary buffer to allocate
  \param schedule Adress of the schedule to allocate
  \param info Information about the function to schedule
  \return Error code
  */
static inline int __sched_alloc_init(NBC_Handle *handle, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int __Sched_send(const void *buffer, int count, MPI_Datatype datatype, int dest, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int __Sched_recv(void *buffer, int count, MPI_Datatype datatype, int source, MPI_Comm comm, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int __Sched_barrier(NBC_Schedule *schedule, Sched_info *info) {
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
static inline int __Sched_commit(NBC_Schedule *schedule, Sched_info *info){

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
static inline int __Sched_op(void *res_buf, void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int __Sched_copy(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule, Sched_info *info) {

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
static inline int __Send_type(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

#if dbg == 1
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    fprintf(stderr, "SEND | %d -> %d (%d) : %p\n", rank, dest, count, buffer);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_send(buffer, count, datatype, dest, tag, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = __Sched_send(buffer, count, datatype, dest, comm, schedule, info);
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
static inline int __Recv_type(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;
 
#if dbg == 1
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    int rank = -1;
    _mpc_cl_comm_rank(comm, &rank);
    fprintf(stderr, "RECV | %d <- %d (%d) : %p\n", rank, source, count, buffer);
  }
#endif

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      res = _mpc_cl_recv(buffer, count, datatype, source, tag, comm, MPI_STATUS_IGNORE);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = __Sched_recv(buffer, count, datatype, source, comm, schedule, info);
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
static inline int __Barrier_type(MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      res = __Sched_barrier(schedule, info);
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
static inline int __Op_type( __UNUSED__ void *res_buf, const void* left_op_buf, void* right_op_buf, int count, MPI_Datatype datatype, MPI_Op op, sctk_Op mpc_op, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
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
      res = __Sched_op(right_op_buf, left_op_buf, right_op_buf, count, datatype, op, schedule, info);
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
static inline int __Copy_type(const void *src, int srccount, MPI_Datatype srctype, void *tgt, int tgtcount, MPI_Datatype tgttype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule *schedule, Sched_info *info) {
  int res = MPI_SUCCESS;

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      NBC_Copy(src, srccount, srctype, tgt, tgtcount, tgttype, comm);
      break;
    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      __Sched_copy(src, srccount, srctype, tgt, tgtcount, tgttype, schedule, info);
      break;
    case MPC_COLL_TYPE_COUNT:
      info->comm_count += 1;
      break;
  }
  return res;
}



// TODO a changer pour eviter les splits qui ne change pas la taille du comm
void mpc_topology_split_persistent_init(mpc_topology_split_info_t *info, MPI_Comm comm, int root, int level)
{
  int res = MPI_ERR_INTERN;
  MPI_Comm* hwcomm = info->hwcomm;
  int level_num = 0;
  int rank_comm;
  hwcomm[0] = comm;
  _mpc_cl_comm_rank(comm, &rank_comm);
  int max_num_levels = 32;
  int vrank = -1;

  int current_size;
  int previous_size;

  _mpc_cl_comm_size(comm, &previous_size);

  /* set root to 0 te be color as master for every of its topological levels */
  RANK2VRANK(rank_comm, vrank, root);

  /* create topological communicators */
  while((hwcomm[level_num] != MPI_COMM_NULL) && level_num < level)
  {
    if(level_num > 0) {
      _mpc_cl_comm_size(hwcomm[level_num], &current_size);

      //printf("CURRENT %d | PREV %d\n", current_size, previous_size);

      if(previous_size == current_size) {
        // bah c est pas normal de faire ca
        break;
      }
      previous_size = current_size;
    }

    res = PMPI_Comm_split_type(hwcomm[level_num], MPI_COMM_TYPE_HW_UNGUIDED, vrank, MPI_INFO_NULL, &hwcomm[level_num+1]);

    if(hwcomm[level_num+1] != MPI_COMM_NULL) {
      printf("topo split\n");
    }


    /* affichage */
    //if(hwcomm[level_num+1] != MPI_COMM_NULL)
    //{
    //    int size;
    //    MPI_Comm_rank(hwcomm[level_num + 1],&rank_comm);
    //    /* set root to 0 te be color as master for every of its topological levels */
    //    RANK2VRANK(rank_comm, vrank, root)
    //    if(!rank_comm)
    //    {
    //        MPI_Comm_size(hwcomm[level_num + 1],&size);
    //        fprintf(stderr, "hwxomm master rank %d size %d\n", rank_comm, size);
    //    }
    //}

    level_num++;
  }
  info->local_level_max = level_num;
  //SCTK_MPI_CHECK_RETURN_VAL (res, comm);
}

void mpc_topology_split_create_master_comm(mpc_topology_split_info_t *info, int root, int level)
{
  int res = MPI_ERR_INTERN;
  int size_comm = 0;
  int rank_comm = -1;
  int rank = -1;
  int level_num = 0;
  int rank_global = -1;
  MPI_Comm* hwcomm = info->hwcomm;
  int max_num_levels = 32;
  MPI_Comm* rootcomm = info->rootcomm;
  _mpc_cl_comm_rank(hwcomm[0],&rank_global);

  int vrank = -1;
  /* use vrank to set root at rank 0 in new master comm */
  RANK2VRANK(rank_global, vrank, root);

  //int size_next_comm = -1;
  //_mpc_cl_comm_size(hwcomm[level_num + 1], &size_next_comm);

  /* create root communicator with ranks 0 for each hwcomm communicator */
  while((hwcomm[level_num + 1] != MPI_COMM_NULL) /*&& (size_next_comm > 1)*/ && (level_num < level))
  {
    //_mpc_cl_comm_rank(hwcomm[level_num + 1], &rank_comm);
    //_mpc_cl_comm_size(hwcomm[level_num + 1], &size_comm);

    int color = (rank_comm == 0)?(0):(1);

    //fprintf(stderr, "%d, %d, %d, %p\n",hwcomm[level_num ], color, vrank, &rootcomm[level_num ]);
    PMPI_Comm_split(hwcomm[level_num], color, vrank, &rootcomm[level_num + 1]);
    //_mpc_cl_comm_size(hwcomm[level_num + 2], &size_next_comm);
    level_num ++;
  }
  //SCTK_MPI_CHECK_RETURN_VAL (res, hwcomm[0]);
}

static inline int __Topo_comm_init(NBC_Handle* handle, MPI_Comm comm, int root, int level, Sched_info *info) {
  //int max_num_levels = 8;
  //int level_topo = 1;

  handle->info_hwcomm = (mpc_topology_split_info_t*) sctk_malloc(sizeof(mpc_topology_split_info_t));
  handle->info_hwcomm->hwcomm = (MPI_Comm *)sctk_malloc((level+1)*sizeof(MPI_Comm));
  handle->info_hwcomm->rootcomm = (MPI_Comm *)sctk_malloc((level+1)*sizeof(MPI_Comm));
  handle->info_hwcomm->hwcomm_rank = (int *)sctk_malloc((level+1)*sizeof(int));
  
  mpc_topology_split_persistent_init(handle->info_hwcomm, comm, root, level);
  mpc_topology_split_create_master_comm(handle->info_hwcomm, root, level);

  for (int k = 1; k < handle->info_hwcomm->local_level_max; k++)
  {
    _mpc_cl_comm_rank(handle->info_hwcomm->hwcomm[k], &handle->info_hwcomm->hwcomm_rank[k]);
  }

  info->info_hwcomm = handle->info_hwcomm;
}





/***************************
  switch function prototype
  ***************************/

static inline int __Bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Barrier_switch (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);




/***********
  BROADCAST
  ***********/

int PMPI_Ibcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
static inline int __Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
static inline int __Bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Bcast_scatter_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Bcast_full_binomial_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Bcast_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);

/**
  \brief Initialize hardware communicators used for persistent hardware algorithms 
  \param comm communicator of the collective 
  \param vrank virtual rank for the algorithm
  \param max_level Max hardware topological level to split communicators 
  \param info Adress on the information structure about the schedule
  \return error code
  */
static inline int __create_hardware_comm_unguided(MPI_Comm comm, int vrank, int max_level, Sched_info *info)
{
    int res = MPI_ERR_INTERN;
    int level_num = 0;

    MPI_Comm* hwcomm = info->hardware_info_ptr->hwcomm;

    hwcomm[level_num] = comm;

    /* create topology communicators */
    while((hwcomm[level_num] != MPI_COMM_NULL) && (level_num < max_level))
    {
        res = PMPI_Comm_split_type(hwcomm[level_num],
                MPI_COMM_TYPE_HW_UNGUIDED,
                vrank,
                MPI_INFO_NULL,
                &hwcomm[level_num+1]);

        if(hwcomm[level_num+1] != MPI_COMM_NULL)
        {
            _mpc_cl_comm_rank(hwcomm[level_num + 1],&vrank);
        }

        level_num++;
    }

    level_num--;

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
static inline int __create_master_hardware_comm_unguided(int vrank, int level, Sched_info *info)
{
	int res = MPI_ERR_INTERN;
    int rank_comm = -1;
    int level_num = 0;

	MPI_Comm* hwcomm = info->hardware_info_ptr->hwcomm;
	MPI_Comm* rootcomm = info->hardware_info_ptr->rootcomm;

    /* create master communicator with ranks 0 for each hwcomm communicator */
    while((hwcomm[level_num + 1] != MPI_COMM_NULL) && (level_num < level))
    {
        _mpc_cl_comm_rank(hwcomm[level_num + 1],&rank_comm);

        int color = -1;

        if(rank_comm == 0)
        {
            color = 0;
        }
        else
        {
            color = 1;
        }

        PMPI_Comm_split(hwcomm[level_num ], color, vrank, &rootcomm[level_num ]);
        level_num ++;
    }
    return res;
}


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
    res = __Ibcast(buffer, count, datatype, root, comm, &(tmp->nbc_handle));
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
static inline int __Ibcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle *handle ) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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

  __Bcast_init(buffer, count, datatype, root, comm, &(req->nbc_handle));
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
static inline int __Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, NBC_Handle* handle){
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  res = __Topo_comm_init(handle, comm, root, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  /* if hardware algorithm has been scheduled */
  if(info.is_hardware_algo)
  {
      /* to free hardware structure later from handle */
      handle->hardware_info = info.hardware_info_ptr;
  }
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  return __Bcast_switch(buffer, count, datatype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __Bcast_switch(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_BCAST_LINEAR,
    NBC_BCAST_BINOMIAL,
    NBC_BCAST_SCATTER_ALLGATHER,
    NBC_BCAST_HARDWARE_FULL_BINOMIAL,
    NBC_BCAST_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_BCAST_BINOMIAL;
  } else {
    alg = NBC_BCAST_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_BCAST_LINEAR:
      res = __Bcast_linear(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_BINOMIAL:
      res = __Bcast_binomial(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_SCATTER_ALLGATHER:
      res = __Bcast_scatter_allgather(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_HARDWARE_FULL_BINOMIAL:
      res = __Bcast_full_binomial_topo(buffer, count, datatype, root, comm, coll_type, schedule, info);
      break;
    case NBC_BCAST_TOPO:
      res = __Bcast_topo(buffer, count, datatype, root, comm, coll_type, schedule, info);
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
static inline int __Bcast_linear(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  // if rank != root, wait for data from root
  if(rank != root) {
    res = __Recv_type(buffer, count, datatype, root, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    //if rank == root, send data to each rank
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __Send_type(buffer, count, datatype, i, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
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
static inline int __Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size, rank;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  // Get max number of steps
  int maxr, vrank, peer;
  maxr = __ceiled_log2(size);
  // Get virtual rank for processes by swappping rank 0 & root
  RANK2VRANK(rank, vrank, root);
  // Get the index of the right-most bit set to 1 in rank
  // The max number of sends made is equal to rmb
  int rmb = __rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  // Wait for data
  if(rank != root) {
    VRANK2RANK(peer, vrank ^ (1 << rmb), root);

    __Recv_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

    if(rmb != 0) {
      __Barrier_type(coll_type, schedule, info);
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
  for(int i = rmb - 1; i >= 0; i--) {
    VRANK2RANK(peer, vrank | (1 << i), root);
    if(peer >= size) {
      continue;
    }

    __Send_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }
  
  return MPI_SUCCESS;
}

/*TODO remove when __Bcast_full_binomial_topo works with __Bcast_binomial instead of this */
static inline int __bcast_sched_full_hardware_binomial(int root, NBC_Schedule *schedule, void *buffer, int count, MPI_Datatype datatype, MPI_Comm comm, MPC_COLL_TYPE coll_type, Sched_info *info)
{

    int rank, p;

    _mpc_cl_comm_rank(comm, &rank);
    _mpc_cl_comm_size(comm, &p);

    int maxr, vrank, peer, r, res;

    maxr = (int)ceil((log(p)/LOG2));

    RANK2VRANK(rank, vrank, root);

    if (vrank != 0)
    {
        r = 0;
        for(r =0; r<maxr; r++)
        {
            if((vrank & (1 << r)) && (r < maxr))
            {
                VRANK2RANK(peer, vrank - (1 << r), root);

                /* recv buffer from a master of root topologicaal level (rank 0) */
                /* master of previous level is always master of its subsequent topological root split level */
                res = __Recv_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

                break;
            }
        }
        __Barrier_type(coll_type, schedule, info);
    }

    int cpt = 0;
    r = 0;
    while(!(vrank & (1 << r)) && (r < maxr))
    {
        if ((vrank + (1 << r) < p))
        {
            cpt++;
            VRANK2RANK(peer, vrank + (1 << r), root);

            /* recv buffer from a master of root topologicaal level (rank 0) */
            /* master of previous level is always master of its subsequent topological root split level */
            res = __Send_type(buffer, count, datatype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
        }
        r++;
    }

    return res;
}

/**
  \brief Execute or schedule a broadcast using the hardware full binomial algorithm
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
static inline int __Bcast_full_binomial_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
          {
              /* At each topological level, masters do binomial algorithm */

              int deepest_level;

              /* set root to 0 te be color as master for every of its topological levels */
              int vrank;
              RANK2VRANK(rank, vrank, root);

              /* choose max topological level on which to do hardware split */
              int max_level;
              max_level = 3;
              //max_level = MAX_HARDWARE_LEVEL;
              /*TODO choose level wisely */

              /* specify hardware algorithm to pass hardware structure to the handle to free later */
              info->is_hardware_algo = 1;

              info->hardware_info_ptr = (mpc_hardware_split_info_t *)sctk_malloc(sizeof(mpc_hardware_split_info_t));
              info->hardware_info_ptr->hwcomm = (MPI_Comm *)sctk_malloc(MAX_HARDWARE_LEVEL*sizeof(MPI_Comm));
              info->hardware_info_ptr->rootcomm = (MPI_Comm *)sctk_malloc(MAX_HARDWARE_LEVEL*sizeof(MPI_Comm));

              /* Create hardware communicators using unguided topological split and max split level */
              __create_hardware_comm_unguided(comm, vrank, max_level, info);
              deepest_level = info->hardware_info_ptr->deepest_hardware_level;
              __create_master_hardware_comm_unguided(vrank, deepest_level, info);
          }
  }

  int deepest_level = info->hardware_info_ptr->deepest_hardware_level;

  for (int k = 0; k < deepest_level; k++)
  {
      int rank_split;

      _mpc_cl_comm_rank(info->hardware_info_ptr->hwcomm[k + 1], &rank_split);

      if((!rank_split)) /* if master */
      {
          /* schedule binomial bcast for masters at each master levels */
          MPI_Comm master_comm = info->hardware_info_ptr->rootcomm[k];

          //res = __Bcast_binomial(buffer, count, datatype, 0, master_comm, coll_type, schedule, info);
          /*TODO replace by __Bcast_binomial when possible */
          res = __bcast_sched_full_hardware_binomial(0, schedule, buffer, count, datatype, master_comm, coll_type, info);
      }
  }

  /* last level topology binomial bcast */

  MPI_Comm hardware_comm = info->hardware_info_ptr->hwcomm[deepest_level];

  //res = __Bcast_binomial(buffer, count, datatype, 0, hardware_comm, coll_type, schedule, info);
  /*TODO replace by __Bcast_binomial when possible */
  res = __bcast_sched_full_hardware_binomial(0, schedule, buffer, count, datatype, hardware_comm, coll_type, info);


  return res;
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
static inline int __Bcast_scatter_allgather(__UNUSED__ void *buffer, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}

static inline int __Bcast_topo(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size, res;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  enum {
    NBC_BCAST_LINEAR,
    NBC_BCAST_BINOMIAL,
    NBC_BCAST_SCATTER_ALLGATHER
  } alg;

  alg = NBC_BCAST_LINEAR;

  for(int i = 1; i < info->info_hwcomm->local_level_max; i++) {
    if(info->info_hwcomm->hwcomm_rank[i] == 0) {
      switch(alg) {
        case NBC_BCAST_LINEAR:
          res = __Bcast_linear(buffer, count, datatype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
        case NBC_BCAST_BINOMIAL:
          res = __Bcast_binomial(buffer, count, datatype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
        case NBC_BCAST_SCATTER_ALLGATHER:
          res = __Bcast_scatter_allgather(buffer, count, datatype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
      }
      __Barrier_type(MPC_COLL_TYPE_COUNT, schedule, info);
    }

    switch(alg) {
      case NBC_BCAST_LINEAR:
        res = __Bcast_linear(buffer, count, datatype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
      case NBC_BCAST_BINOMIAL:
        res = __Bcast_binomial(buffer, count, datatype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
      case NBC_BCAST_SCATTER_ALLGATHER:
        res = __Bcast_scatter_allgather(buffer, count, datatype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
    }
    __Barrier_type(MPC_COLL_TYPE_COUNT, schedule, info);
  }
}



/***********
  REDUCE
  ***********/

int PMPI_Ireduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
static inline int __Ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
static inline int __Reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_reduce_scatter_allgather(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, &(tmp->nbc_handle));
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
static inline int __Ireduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Reduce_init (sendbuf, recvbuf, count, datatype, op, root, comm, &(req->nbc_handle));
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
static inline int __Reduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IREDUCE_TAG);
  res = __Topo_comm_init(handle, comm, root, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_reduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  return __Reduce_switch(sendbuf, recvbuf, count, datatype, op, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Reduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  enum {
    NBC_REDUCE_LINEAR,
    NBC_REDUCE_BINOMIAL,
    NBC_REDUCE_REDUCE_SCATTER_ALLGATHER,
    NBC_REDUCE_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_REDUCE_BINOMIAL;
  } else {
    alg = NBC_REDUCE_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_REDUCE_LINEAR:
      res = __Reduce_linear(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_BINOMIAL:
      res = __Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_REDUCE_SCATTER_ALLGATHER:
      res = __Reduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_TOPO:
      res = __Reduce_topo(sendbuf, recvbuf, count, datatype, op, root, comm, coll_type, schedule, info);
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
static inline int __Reduce_linear(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
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

  // Gather data from all ranks
  if(sendbuf != MPI_IN_PLACE) {
    __Gather_switch(sendbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  } else {
    __Gather_switch(recvbuf, count, datatype, tmpbuf, count, datatype, root, comm, coll_type, schedule, info);
  }

  // Reduce received data
  if(rank == root) {
    __Barrier_type(coll_type, schedule, info);

    for(int i = 1; i < size; i++) {
      __Op_type(NULL, (char*)tmpbuf + (i-1)*ext*count, (char*)tmpbuf + i*ext*count, count, datatype, op, mpc_op, coll_type, schedule, info);
    }
    
    __Copy_type((char*)tmpbuf + (size-1)*ext*count, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
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
static inline int __Reduce_binomial(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
 
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
  
  maxr = __ceiled_log2(size);

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
    VRANK2RANK(peer, vrank ^ (1 << i), vroot);
    
    if(peer < rank) {
      // send reduce data to rank-2^i
      __Send_type(tmp_sendbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      break;

    } else if(peer < size) {
      // wait data from rank+2^i
      __Recv_type(tmp_recvbuf, count, datatype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      __Barrier_type(coll_type, schedule, info);
      // reduce data
      __Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

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
      __Send_type(tmp_sendbuf, count, datatype, root, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    } else if(rank == root) {
      __Recv_type(recvbuf, count, datatype, 0, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  } else if (rank == root) {
    // Root already have the result, just copy it in revcbuf
    __Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
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
static inline int __Reduce_reduce_scatter_allgather(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {
 
  not_implemented();

  return MPI_SUCCESS;
}

static inline int __Reduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size, res;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  enum {
    NBC_REDUCE_LINEAR,
    NBC_REDUCE_BINOMIAL
  } alg;

  // au choix, soit on fait un switch dans une boucle, ou on utilise un pointeur de fonction

  for(int i = info->info_hwcomm->local_level_max - 1; i > 0; i--) {
    alg = NBC_REDUCE_LINEAR;

    switch(alg) {
      case NBC_REDUCE_LINEAR:
        res = __Reduce_linear(sendbuf, recvbuf, count, datatype, op, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
      case NBC_REDUCE_BINOMIAL:
        res = __Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
    }

    __Barrier_type(coll_type, schedule, info);

    if(info->info_hwcomm->hwcomm_rank[i] == 0) {
      alg = NBC_REDUCE_LINEAR;

      switch(alg) {
        case NBC_REDUCE_LINEAR:
          res = __Reduce_linear(sendbuf, recvbuf, count, datatype, op, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
        case NBC_REDUCE_BINOMIAL:
          res = __Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
      }
    
      __Barrier_type(coll_type, schedule, info);
    }

  }
}



/***********
  ALLREDUCE
  ***********/

int PMPI_Iallreduce (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
static inline int __Iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __Allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_vector_halving_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_ring(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allreduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Iallreduce (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
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
static inline int __Iallreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Allreduce_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
static inline int __Allreduce_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  res = __Topo_comm_init(handle, comm, 0, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_allreduce(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __Allreduce_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Allreduce_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLREDUCE_REDUCE_BROADCAST,
    NBC_ALLREDUCE_DISTANCE_DOUBLING,
    NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING, 
    NBC_ALLREDUCE_BINARY_BLOCK, 
    NBC_ALLREDUCE_RING,
    NBC_ALLREDUCE_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_ALLREDUCE_BINARY_BLOCK;
  } else {
    alg = NBC_ALLREDUCE_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_ALLREDUCE_REDUCE_BROADCAST:
      res = __Allreduce_reduce_broadcast(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_DISTANCE_DOUBLING:
      res = __Allreduce_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_VECTOR_HALVING_DISTANCE_DOUBLING:
      res = __Allreduce_vector_halving_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_BINARY_BLOCK:
      res = __Allreduce_binary_block(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_RING:
      res = __Allreduce_ring(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_ALLREDUCE_TOPO:
      res = __Allreduce_topo(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
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
static inline int __Allreduce_reduce_broadcast(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  __Reduce_switch(sendbuf, recvbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  __Barrier_type(coll_type, schedule, info);
  __Bcast_switch(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);

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
static inline int __Allreduce_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
  maxr = __floored_log2(size);
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
    __Copy_type(sendbuf, count, datatype, tmp_sendbuf, count, datatype, comm, coll_type, schedule, info);
    first_access = 0;
  }

  // Reduce the number of rank participating ro the allreduce to the nearest lower power of 2:
  // all processes with odd rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) send their data to rank-1
  // all processes with even rank and rank < 2 * (size - 2^(floor(log(size)/LOG2))) receive and reduce data from rank+1
  if(rank < group) {
    peer = rank ^ 1;

    if(rank & 1) {
      __Send_type(sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      __Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Barrier_type(coll_type, schedule, info);
      __Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

      if(first_access) {
        tmp_sendbuf = tmpbuf + ext * count;
        first_access = 0;
      }

      pointer_swap(tmp_sendbuf, tmp_recvbuf, swap);
    }
  }

  if(rank >= group || !(rank & 1)) {
    // at each step, processes exchange and reduce their data with (rank XOR 2^i)
    for(int i = 0; i < maxr; i++) {
      
      peer = vrank ^ (1 << i);
      if(peer < group / 2) {
        peer = peer * 2;
      } else {
        peer = peer + group / 2;
      }

      // Prevent deadlock
      if(peer > rank) {
        __Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      } else {
        __Recv_type(tmp_recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }
      __Barrier_type(coll_type, schedule, info);

      // To respect commutativity, we must choose the right order for the Reduction operation
      // if we received from a lower rank: tmp_recvbuf OP tmp_sendbuf
      // if we received from a higher rank: tmp_sendbuf OP tmp_recvbuf
      if(vrank & (1 << i)) {
        __Op_type(NULL, tmp_recvbuf, tmp_sendbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      } else {
        __Op_type(NULL, tmp_sendbuf, tmp_recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);

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
      __Recv_type(recvbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    } else {
      __Send_type(tmp_sendbuf, count, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
    }
  } else {
    __Copy_type(tmp_sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
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
static inline int __Allreduce_vector_halving_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Allreduce_binary_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int rank, size;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(datatype, &ext);

  void* tmpbuf = NULL;

  sctk_Op mpc_op;
  sctk_op_t *mpi_op;

  int vrank/*, peer*/, maxr;
  maxr = __ceiled_log2(size);
  vrank = rank;

  int block = maxr;
  int previous_block = -1;
  int next_block = -1;

  while(block) {
    if(size & (1 << block)) {
      if(vrank >> block == 0) {
        for(int j = block - 1; j >= 0; j--) {
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

  int start = 0, end = count, mid;

  //fprintf(stderr, "RANK %d | VRANK %d | SIZE %d | BLOCK %d | BLOCK SIZE %d | 1rst RANK OF BLOCK %d | PREV BLOCK %d | PREV BLOCK SIZE %d | 1rst RANK OF PREV BLOCK %d | NEXT BLOCK %d | NEXT BLOCK SIZE %d | 1rst RANK OF NEXT BLOCK %d\n", rank, vrank, size, block, block_size, first_rank_of_block, previous_block, previous_block_size, first_rank_of_previous_block, next_block, next_block_size, first_rank_of_next_block);

  switch(coll_type) {
    case MPC_COLL_TYPE_BLOCKING:
      tmpbuf = sctk_malloc(count * ext + 4 * block_size * sizeof(int));
      mpi_op = sctk_convert_to_mpc_op(op);
      mpc_op = mpi_op->op;
      break;

    case MPC_COLL_TYPE_NONBLOCKING:
    case MPC_COLL_TYPE_PERSISTENT:
      tmpbuf = info->tmpbuf + info->tmpbuf_pos;
      info->tmpbuf_pos += count * ext + 4 * block_size * sizeof(int);
      break;

    case MPC_COLL_TYPE_COUNT:
      info->tmpbuf_size += count * ext + 4 * block_size * sizeof(int);
      break;
  }


  int *group_starts      = (int*)(((void*)tmpbuf) + count * ext + 0 * block_size * sizeof(int));
  int *group_ends        = (int*)(((void*)tmpbuf) + count * ext + 1 * block_size * sizeof(int));
  int *prev_group_starts = (int*)(((void*)tmpbuf) + count * ext + 2 * block_size * sizeof(int));
  int *prev_group_ends   = (int*)(((void*)tmpbuf) + count * ext + 3 * block_size * sizeof(int));
  void *swap = NULL;


  if(coll_type != MPC_COLL_TYPE_COUNT) {
    // init group starts/ends
    for(int j = 0; j < block_size; j++) {
      group_starts[j] = 0;
      group_ends[j] = count;
    }
  }

  
  if(sendbuf != MPI_IN_PLACE) {
    __Copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }


  int send_bufsize;
  int recv_bufsize;

  void *tmp_sendbuf;
  void *tmp_recvbuf;
  void *tmp_opbuf;


  void *resbuf = recvbuf;
  void *recv_resbuf = tmpbuf;

  int peer, peer_vrank;



  //REDUCE_SCATTER
  //distance doubling and vector halving
  
  for(int i = 0; i < block; i++) {
    peer_vrank = vrank ^ (1 << i);
    peer = first_rank_of_block + peer_vrank;

    mid = (start + end + 1) >> 1;

    if(peer_vrank > vrank) { // send second part of buffer

      tmp_sendbuf = ((void*) resbuf) + mid * ext;
      send_bufsize = end - mid;

      tmp_recvbuf = ((void*) recv_resbuf) + start * ext;
      tmp_opbuf = ((void*) resbuf) + start * ext;
      recv_bufsize = mid - start;

      if(send_bufsize) {
        __Send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }

      if(recv_bufsize) {
        __Recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __Barrier_type(coll_type, schedule, info);
        __Op_type(NULL, tmp_opbuf, tmp_recvbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);

        pointer_swap(resbuf, recv_resbuf, swap);
      }

      end = mid;
    } else { // send first part of buffer
      
      tmp_sendbuf = ((void*) resbuf) + start * ext;
      send_bufsize = mid - start;

      tmp_recvbuf = ((void*) recv_resbuf) + mid * ext;
      tmp_opbuf = ((void*) resbuf) + mid * ext;
      recv_bufsize = end - mid;

      if(recv_bufsize) {
        __Recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
        __Barrier_type(coll_type, schedule, info);
        __Op_type(NULL, tmp_recvbuf, tmp_opbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);
      }

      if(send_bufsize) {
        __Send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      }

      start = mid;
    }

    if(coll_type != MPC_COLL_TYPE_COUNT) {
      // update group starts/ends
      pointer_swap(prev_group_starts, group_starts, swap);
      pointer_swap(prev_group_ends, group_ends, swap);

      for(int j = 0; j < block_size; j++) {

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
  int peer_start, peer_end, peer_mid;

  if(next_block != -1) {

    tmp_recvbuf = ((void*) recv_resbuf) + start * ext;
    tmp_opbuf = ((void*) resbuf) + start * ext;
    recv_bufsize = end - start;
    //next_block_peer = vrank / (1 << (block - next_block)) + first_rank_of_next_block;
    //next_block_peer = vrank % (1 << (block - next_block)) + first_rank_of_next_block;
    next_block_peer = vrank % next_block_size + first_rank_of_next_block;


    __Recv_type(tmp_recvbuf, recv_bufsize, datatype, next_block_peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    __Barrier_type(coll_type, schedule, info);
    __Op_type(NULL, tmp_opbuf, tmp_recvbuf, recv_bufsize, datatype, op, mpc_op, coll_type, schedule, info);

    pointer_swap(resbuf, recv_resbuf, swap);
  }


  if(resbuf != recvbuf) {
    __Copy_type(resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }


  if(previous_block != -1) {

    for(int j = 0; j < target_count; j++) {
      peer_vrank = vrank + j * block_size;
      peer = peer_vrank + first_rank_of_previous_block;
      peer_start = 0;
      peer_end = count;

      for(int i = 0; i < previous_block; i++) {
        int peer_peer_vrank = peer_vrank ^ (1 << i);

        peer_mid = (peer_start + peer_end + 1) >> 1;

        if(peer_peer_vrank > peer_vrank) {
          peer_end = peer_mid;
        } else {
          peer_start = peer_mid;
        }
      }

      tmp_sendbuf = ((void*) recvbuf) + peer_start * ext;
      send_bufsize = peer_end - peer_start;

      __Send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    }

    for(int j = 0; j < target_count; j++) {
      peer_vrank = vrank + j * block_size;
      peer = peer_vrank + first_rank_of_previous_block;
      peer_start = 0;
      peer_end = count;

      for(int i = 0; i < previous_block; i++) {
        int peer_peer_vrank = peer_vrank ^ (1 << i);

        peer_mid = (peer_start + peer_end + 1) >> 1;

        if(peer_peer_vrank > peer_vrank) {
          peer_end = peer_mid;
        } else {
          peer_start = peer_mid;
        }
      }

      tmp_sendbuf = ((void*) recvbuf) + peer_start * ext;
      send_bufsize = peer_end - peer_start;

      __Recv_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
    }

    __Barrier_type(coll_type, schedule, info);
  }


  if(next_block != -1) {

    tmp_sendbuf = ((void*) recvbuf) + start * ext;
    send_bufsize = end - start;

    __Send_type(tmp_sendbuf, send_bufsize, datatype, next_block_peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
  }


  //ALLGATHER
  //distance halving and vector doubling

  for(int i = block-1; i >= 0; i--) {
    int peer_vrank = vrank ^ (1 << i);
    int peer = first_rank_of_block + peer_vrank;

    tmp_sendbuf = ((void*) recvbuf) + start * ext;
    send_bufsize = end - start;

    if(coll_type != MPC_COLL_TYPE_COUNT) {
      recv_bufsize = group_ends[peer_vrank] - group_starts[peer_vrank];
    }

    if(peer_vrank > vrank) { // receive second part of buffer
      tmp_recvbuf = ((void*) recvbuf) + end * ext;
      
      __Send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Barrier_type(coll_type, schedule, info);

      end += recv_bufsize;
    } else { // receive first part of buffer
      tmp_recvbuf = ((void*) recvbuf) + (start - recv_bufsize) * ext;
      
      __Recv_type(tmp_recvbuf, recv_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Send_type(tmp_sendbuf, send_bufsize, datatype, peer, MPC_ALLREDUCE_TAG, comm, coll_type, schedule, info);
      __Barrier_type(coll_type, schedule, info);

      start -= recv_bufsize;
    }


    if(coll_type != MPC_COLL_TYPE_COUNT) {
      // update group starts/ends
      pointer_swap(prev_group_starts, group_starts, swap);
      pointer_swap(prev_group_ends, group_ends, swap);

      for(int j = 0; j < block_size; j++) {

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
static inline int __Allreduce_ring(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}

static inline int __Allreduce_topo(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  __Reduce_topo(sendbuf, recvbuf, count, datatype, op, 0, comm, coll_type, schedule, info);
  __Barrier_type(coll_type, schedule, info);
  __Bcast_topo(recvbuf, count, datatype, 0, comm, coll_type, schedule, info);
}



/***********
  SCATTER
  ***********/

int PMPI_Iscatter (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
static inline int __Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __Scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scatter_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
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
static inline int __Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Scatter_init(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
static inline int __Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  res = __Topo_comm_init(handle, comm, root, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __Scatter_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __Scatter_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_SCATTER_LINEAR,
    NBC_SCATTER_BINOMIAL,
    NBC_SCATTER_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_SCATTER_BINOMIAL;
  } else {
    alg = NBC_SCATTER_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_SCATTER_LINEAR:
      res = __Scatter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTER_BINOMIAL:
      res = __Scatter_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTER_TOPO:
      res = __Scatter_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
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
static inline int __Scatter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    res = __Recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, send data to each process
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __Send_type(sendbuf + i * ext * sendcount, sendcount, sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(recvbuf != MPI_IN_PLACE) {
      __Copy_type(sendbuf + rank * ext * sendcount, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __Scatter_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
  PMPI_Type_extent(sendtype, &sendext);
  PMPI_Type_extent(tmp_recvtype, &recvext);

  int maxr, vrank, peer, peer_vrank;
  maxr = __ceiled_log2(size);
  RANK2VRANK(rank, vrank, root);
  
  int rmb = __rmb_index(vrank);
  if(rank == root) {
    rmb = maxr;
  }

  int range = __fill_rmb(vrank, rmb);

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
    __Copy_type(sendbuf, sendcount * size, sendtype, tmpbuf, tmp_recvcount * size, tmp_recvtype, comm, coll_type, schedule, info);

    // As we use virtual rank where rank root and rank 0 are swapped, we need to swap their data as well
    if(root != 0) {
      __Copy_type(sendbuf, sendcount, sendtype, tmpbuf + root * tmp_recvcount * recvext, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
      __Copy_type(sendbuf + root * sendcount * sendext, sendcount, sendtype, tmpbuf, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
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

    __Recv_type(tmpbuf, count, tmp_recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
    __Barrier_type(coll_type, schedule, info);
  }

  // Distribute data to rank+2^(rmb-1); ...; rank+2^0
  // See __Bcast_binomial
  for(int i = rmb - 1; i >= 0; i--) {
    peer_vrank = vrank | (1 << i);

    VRANK2RANK(peer, peer_vrank, root);
    if(peer >= size) {
      continue;
    }

    // In case the total number of processes isn't a power of 2 we need to compute the correct size of data to send
    // The range of a process is the higher rank to which this process may send data, regardless of the number of processes
    peer_range = __fill_rmb(peer_vrank, i);
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

    __Send_type(tmpbuf + (1 << i) * tmp_recvcount * recvext, count, tmp_recvtype, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }

  if(recvbuf != MPI_IN_PLACE) {
    __Copy_type(tmpbuf, tmp_recvcount, recvtype, recvbuf, tmp_recvcount, tmp_recvtype, comm, coll_type, schedule, info);
  }




  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
  }

  return MPI_SUCCESS;
}

static inline int __Scatter_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size, res;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  enum {
    NBC_SCATTER_LINEAR,
    NBC_SCATTER_BINOMIAL
  } alg;

  for(int i = 1; i < info->info_hwcomm->local_level_max; i++) {
    if(info->info_hwcomm->hwcomm_rank[i] == 0) {
      switch(alg) {
        case NBC_SCATTER_LINEAR:
          res = __Scatter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
          break;
        case NBC_SCATTER_BINOMIAL:
          res = __Scatter_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
          break;
      }
      __Barrier_type(MPC_COLL_TYPE_COUNT, schedule, info);
    }

    switch(alg) {
      case NBC_SCATTER_LINEAR:
        res = __Scatter_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
        break;
      case NBC_SCATTER_BINOMIAL:
        res = __Scatter_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
        break;
    }
    __Barrier_type(MPC_COLL_TYPE_COUNT, schedule, info);
  }
}



/***********
  SCATTERV
  ***********/

int PMPI_Iscatterv (const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request);
static inline int __Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __Scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scatterv_binomial(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Iscatterv (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
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
static inline int __Iscatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Scatterv_init (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
static inline int __Scatterv_init(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __Scatterv_switch(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL); 
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
static inline int __Scatterv_switch(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_SCATTERV_LINEAR,
    NBC_SCATTERV_BINOMIAL
  } alg;

  alg = NBC_SCATTERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_SCATTERV_LINEAR:
      res = __Scatterv_linear(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_SCATTERV_BINOMIAL:
      res = __Scatterv_binomial(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
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
static inline int __Scatterv_linear(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
        info->comm_count += size;
      } else {
        info->comm_count += 1;
      }

      return MPI_SUCCESS;
  }


  // if rank != root, receive data from root
  if(rank != root) {
    res = __Recv_type(recvbuf, recvcount, recvtype, root, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, send data to each process
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __Send_type(sendbuf + displs[i], sendcounts[i], sendtype, i, MPC_SCATTER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(recvbuf != MPI_IN_PLACE) {
      __Copy_type(sendbuf + displs[rank], sendcounts[rank], sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __Scatterv_binomial(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  GATHER
  ***********/

int PMPI_Igather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
static inline int __Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __Gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Gather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(tmp->nbc_handle));
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
static inline int __Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Gather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, &(req->nbc_handle));
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
static inline int __Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  res = __Topo_comm_init(handle, comm, root, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Gather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_GATHER_LINEAR,
    NBC_GATHER_BINOMIAL,
    NBC_GATHER_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_GATHER_BINOMIAL;
  } else {
    alg = NBC_GATHER_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_GATHER_LINEAR:
      res = __Gather_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHER_BINOMIAL:
      res = __Gather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHER_TOPO:
      res = __Gather_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, coll_type, schedule, info);
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
static inline int __Gather_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  // if rank != root, send data to root
  if(rank != root) {
    res = __Send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, receive data from each process
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __Recv_type(recvbuf + i * ext * recvcount, recvcount, recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __Copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * ext * recvcount, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __Gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
  PMPI_Type_extent(recvtype, &recvext);

  int maxr, vrank, peer, peer_vrank;
  maxr = __ceiled_log2(size);
  RANK2VRANK(rank, vrank, root);
  
  int rmb = __rmb_index(vrank);
  int range = __fill_rmb(vrank, rmb);

  int peer_range, count;

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
      info->tmpbuf_size += size * sendext * tmp_sendcount;
      break;
  }

  __Copy_type(tmp_sendbuf, tmp_sendcount, tmp_sendtype, tmpbuf, tmp_sendcount, tmp_sendtype, comm, coll_type, schedule, info);

  // Receive data from rank+2^(rmb-1); ...; rank+2^0
  // See __Reduce_binomial
  for(int i = 0; i < maxr; i++) {
    peer_vrank = vrank ^ (1 << i);
    VRANK2RANK(peer, peer_vrank, root);

    if(vrank & (1 << i)) {
      
      if(range < size) {
        count = (1 << i) * tmp_sendcount;
      } else {
        count = (size - vrank) * tmp_sendcount;
      }

      __Barrier_type(coll_type, schedule, info);
      __Send_type(tmpbuf, count, tmp_sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      
      break;
    } else if(peer < size) {
      
      // In case the total number of processes isn't a power of 2 we need to compute the correct size of data to recv
      // The range of a process is the higher rank from which this process may recv data, regardless of the number of processes
      peer_range = __fill_rmb(peer_vrank, i);
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

      __Recv_type(tmpbuf + (1 << i) * tmp_sendcount * sendext, count, tmp_sendtype, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  }

  if(rank == root) {
    __Barrier_type(coll_type, schedule, info);
    
    __Copy_type(tmpbuf, tmp_sendcount * size, tmp_sendtype, recvbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);
    
    // As we use virtual rank where rank root and rank 0 are swapped, we need to swap their data as well
    if(root != 0) {
      __Copy_type(tmpbuf, tmp_sendcount, tmp_sendtype, recvbuf + root * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
      __Copy_type(tmpbuf + root * tmp_sendcount * sendext, tmp_sendcount, tmp_sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }
  }

  if(coll_type == MPC_COLL_TYPE_BLOCKING) {
    free(tmpbuf);
  }

  return MPI_SUCCESS;
}

static inline int __Gather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int rank, size, res;
  MPI_Aint ext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  enum {
    NBC_GATHER_LINEAR,
    NBC_GATHER_BINOMIAL
  } alg;

  // au choix, soit on fait un switch dans une boucle, ou on utilise un pointeur de fonction

  for(int i = info->info_hwcomm->local_level_max - 1; i > 0; i--) {
    alg = NBC_GATHER_LINEAR;

    switch(alg) {
      case NBC_GATHER_LINEAR:
        res = __Gather_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
      case NBC_GATHER_BINOMIAL:
        res = __Gather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->hwcomm[i], coll_type, schedule, info);
        break;
    }

    __Barrier_type(coll_type, schedule, info);

    if(info->info_hwcomm->hwcomm_rank[i] == 0) {
      alg = NBC_GATHER_LINEAR;

      switch(alg) {
        case NBC_GATHER_LINEAR:
          res = __Gather_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
        case NBC_GATHER_BINOMIAL:
          res = __Gather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, info->info_hwcomm->rootcomm[i], coll_type, schedule, info);
          break;
      }
    
      __Barrier_type(coll_type, schedule, info);
    }

  }
}



/***********
  GATHERV
  ***********/

int PMPI_Igatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
static inline int __Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
static inline int __Gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Gatherv_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(tmp->nbc_handle));
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
static inline int __Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Gatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, &(req->nbc_handle));
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
static inline int __Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  return __Gatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Gatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_GATHERV_LINEAR,
    NBC_GATHERV_BINOMIAL
  } alg;

  alg = NBC_GATHERV_LINEAR;

  int res;

  switch(alg) {
    case NBC_GATHERV_LINEAR:
      res = __Gatherv_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
      break;
    case NBC_GATHERV_BINOMIAL:
      res = __Gatherv_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, coll_type, schedule, info);
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
static inline int __Gatherv_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  // if rank != root, send data to root
  if(rank != root) {
    res = __Send_type(sendbuf, sendcount, sendtype, root, MPC_GATHER_TAG, comm, coll_type, schedule, info);
    if(res != MPI_SUCCESS) {
      return res;
    }
  } else {
    // if rank == root, receive data from each process
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      res = __Recv_type(recvbuf + displs[i], recvcounts[i], recvtype, i, MPC_GATHER_TAG, comm, coll_type, schedule, info);
      if(res != MPI_SUCCESS) {
        return res;
      }
    }

    if(sendbuf != MPI_IN_PLACE) {
      __Copy_type(sendbuf, sendcount, sendtype, recvbuf + displs[rank], recvcounts[rank], recvtype, comm, coll_type, schedule, info);
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
static inline int __Gatherv_binomial(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ int root, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER BLOCK
  ***********/

int PMPI_Ireduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
static inline int __Ireduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __Reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_block_distance_doubling(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_block_distance_halving(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Ireduce_scatter_block (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
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
static inline int __Ireduce_scatter_block (const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Reduce_scatter_block_init (sendbuf,  recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
static inline int __Reduce_scatter_block_init(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_reduce_scatter_block(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __Reduce_scatter_block_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Reduce_scatter_block_switch(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
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
      res = __Reduce_scatter_block_reduce_scatter(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_DOUBLING:
      res = __Reduce_scatter_block_distance_doubling(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_DISTANCE_HALVING:
      res = __Reduce_scatter_block_distance_halving(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_BLOCK_PAIRWISE:
      res = __Reduce_scatter_block_pairwise(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
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
static inline int __Reduce_scatter_block_reduce_scatter(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    __Reduce_switch(sendbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info); 
  } else {
    __Reduce_switch(recvbuf, tmpbuf, count * size, datatype, op, 0, comm, coll_type, schedule, info); 
  }

  __Barrier_type(coll_type, schedule, info);

  __Scatter_switch(tmpbuf, count, datatype, recvbuf, count, datatype, 0, comm, coll_type, schedule, info);


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
static inline int __Reduce_scatter_block_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Reduce_scatter_block_distance_halving(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ int count, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Reduce_scatter_block_pairwise(const void *sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  __Copy_type(initial_buf, count, datatype, tmp_left_resbuf, count, datatype, comm, coll_type, schedule, info);

  for(int i = 1; i < size; i++) {
    // At each step receive from rank-i, & send to rank+i,
    // This order is choosed to avoid swap buffer:
    // tmp_recvbuf OP tmp_resbuf will put the result in tmp_resbuf
    send_peer = (rank + i) % size;
    recv_peer = (rank - i + size) % size;
    
    __Send_type(initial_buf + send_peer * ext * count, count, datatype, send_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);
    __Recv_type(tmp_recvbuf, count, datatype, recv_peer, MPC_REDUCE_SCATTER_BLOCK_TAG, comm, coll_type, schedule, info);

    __Barrier_type(coll_type, schedule, info);
   
    // We reduce data from rank 0;...;rank in tmp_left_resbuf
    if(recv_peer < rank) { 
      __Op_type(NULL, tmp_recvbuf, tmp_left_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    } else {
    // We reduce data from rank rank+1;...;size-1 in tmp_right_resbuf
      // No reduce operation needed if we received from rank size-1
      if(first_access) {
        pointer_swap(tmp_recvbuf, tmp_right_resbuf, swap);
        first_access = 0;
      } else {
        __Op_type(NULL, tmp_recvbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
      }
    }
  }

  if(first_access) {
    __Copy_type(tmp_left_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  } else {
    __Op_type(NULL, tmp_left_resbuf, tmp_right_resbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    __Copy_type(tmp_right_resbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }
  
  return MPI_SUCCESS;
}




/***********
  REDUCE SCATTER
  ***********/

int PMPI_Ireduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
static inline int __Ireduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __Reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_distance_doubling(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_distance_halving(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Reduce_scatter_pairwise(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res = __Ireduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm, &(tmp->nbc_handle));
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
static inline int __Ireduce_scatter (const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Reduce_scatter_init (sendbuf,  recvbuf, recvcounts, datatype, op, comm, &(req->nbc_handle));
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
static inline int __Reduce_scatter_init(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLREDUCE_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_reduce_scatter(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __Reduce_scatter_switch(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Reduce_scatter_switch(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
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
      res = __Reduce_scatter_reduce_scatterv(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_DOUBLING:
      res = __Reduce_scatter_distance_doubling(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_DISTANCE_HALVING:
      res = __Reduce_scatter_distance_halving(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_REDUCE_SCATTER_PAIRWISE:
      res = __Reduce_scatter_pairwise(sendbuf, recvbuf, recvcounts, datatype, op, comm, coll_type, schedule, info);
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
static inline int __Reduce_scatter_reduce_scatterv(const void *sendbuf, void* recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    for(int i = 1; i < size; i++) {
      displs[i] = displs[i-1] + recvcounts[i-1] * ext;
    }
  }

  const void *tmp_sendbuf = sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf = recvbuf;
  }

  __Reduce_switch(tmp_sendbuf, tmpbuf, count, datatype, op, 0, comm, coll_type, schedule, info); 
  __Barrier_type(coll_type, schedule, info);
  __Scatterv_switch(tmpbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm, coll_type, schedule, info);

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
static inline int __Reduce_scatter_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Reduce_scatter_distance_halving(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Reduce_scatter_pairwise(__UNUSED__ const void *sendbuf, __UNUSED__ void* recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ MPI_Datatype datatype, __UNUSED__ MPI_Op op, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();
  
  return MPI_SUCCESS;
}




/***********
  ALLGATHER
  ***********/

int PMPI_Iallgather (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
static inline int __Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __Allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res =  __Iallgather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
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
static inline int __Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Allgather_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
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
static inline int __Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  res = __Topo_comm_init(handle, comm, 0, MAX_TOPO_SPLIT, &info);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  return __Allgather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Allgather_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  enum {
    NBC_ALLGATHER_GATHER_BROADCAST,
    NBC_ALLGATHER_DISTANCE_DOUBLING,
    NBC_ALLGATHER_BRUCK,
    NBC_ALLGATHER_RING,
    NBC_ALLGATHER_TOPO
  } alg;

  if(coll_type != MPC_COLL_TYPE_PERSISTENT) {
    alg = NBC_ALLGATHER_DISTANCE_DOUBLING;
  } else {
    alg = NBC_ALLGATHER_TOPO;
  }

  int res;

  switch(alg) {
    case NBC_ALLGATHER_GATHER_BROADCAST:
      res = __Allgather_gather_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_DISTANCE_DOUBLING:
      res = __Allgather_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_BRUCK:
      res = __Allgather_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_RING:
      res = __Allgather_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHER_TOPO:
      res = __Allgather_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __Allgather_gather_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int size;
  _mpc_cl_comm_size(comm, &size);

 __Gather_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm, coll_type, schedule, info);
 __Barrier_type(coll_type, schedule, info);
 __Bcast_switch(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);

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
static inline int __Allgather_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  const void *initial_buf = sendbuf;
  int tmp_sendcount = sendcount;
  MPI_Datatype tmp_sendtype = sendtype;
  if(sendbuf == MPI_IN_PLACE) {
    initial_buf = recvbuf;
    tmp_sendcount = recvcount;
    tmp_sendtype = recvtype;
  }


  int rank, size;
  MPI_Aint sendext, recvext;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);
  PMPI_Type_extent(tmp_sendtype, &sendext);
  PMPI_Type_extent(recvtype, &recvext);

  int vrank, vsize, peer, maxr, count = 0, peer_count = 0;
  maxr = __floored_log2(size);
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

  // Initialyse the array containing the amount of data in possession of each processes
  if(coll_type != MPC_COLL_TYPE_COUNT) {
    for(int i = 0; i < vsize; i++) {
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
      __Send_type(initial_buf, tmp_sendcount, tmp_sendtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __Recv_type(recvbuf + peer * recvcount * recvext, recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __Barrier_type(coll_type, schedule, info);
    }
  }
  
  if(rank > group || !(rank & 1)) {
  
    if(sendbuf != MPI_IN_PLACE) {
      __Copy_type(sendbuf , sendcount, sendtype, tmpbuf, recvcount, recvtype, comm, coll_type, schedule, info);
    }

    // at each step, processes exchange their data with (rank XOR 2^i)
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

      // To avoid deadlocks
      if(peer < rank) {
        __Send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        __Recv_type(tmpbuf - peer_count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);

        tmpbuf -= peer_count * recvcount * recvext;
      } else {
        __Recv_type(tmpbuf + count * recvcount * recvext, peer_count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
        __Send_type(tmpbuf, count * recvcount, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      }

      __Barrier_type(coll_type, schedule, info);

      // Update of the array containing the amount of data in possession of each processes
      // Could be replace by a formula to find the amound of data we need to receive from a specific process,
      // but i didnt find it
      if(coll_type != MPC_COLL_TYPE_COUNT) {
        pointer_swap(countbuf, prev_countbuf, swap);
        for(int j = 0; j < vsize; j++) {
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
      __Recv_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __Send_type(recvbuf, recvcount * size, recvtype, peer, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
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
static inline int __Allgather_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Allgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    __Copy_type(sendbuf, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);
  }

  // At each step, send to rank+1, recv from rank-1
  for(int i = 0; i < size - 1; i++) {
    // To prevent deadlocks even ranks send then recv, odd ranks recv then send
    // When i=0 we send our own data
    // At each step we send the data received at the previous step
    if(rank & 1) {
      __Recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __Send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    } else {
      __Send_type(recvbuf + ((rank - i + size) % size) * recvcount * recvext, recvcount, recvtype, (rank + 1) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + ((rank - i - 1 + size) % size) * recvcount * recvext, recvcount, recvtype, (rank - 1 + size) % size, MPC_ALLGATHER_TAG, comm, coll_type, schedule, info);
    }
    
    __Barrier_type(coll_type, schedule, info);
  }

  return MPI_SUCCESS;
}

static inline int __Allgather_topo(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  int size;
  _mpc_cl_comm_size(comm, &size);

  __Gather_topo(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm, coll_type, schedule, info);
  __Barrier_type(coll_type, schedule, info);
  __Bcast_topo(recvbuf, recvcount * size, recvtype, 0, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}



/***********
  ALLGATHERV
  ***********/

int PMPI_Iallgatherv (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
static inline int __Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __Allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgatherv_distance_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Allgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res =  __Iallgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(tmp->nbc_handle));
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
static inline int __Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IALLGATHER_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Allgatherv_init (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, &(req->nbc_handle));
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
static inline int __Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm) {
  return __Allgatherv_switch(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Allgatherv_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Allgatherv_gatherv_broadcast(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_DISTANCE_DOUBLING:
      res = __Allgatherv_distance_doubling(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_BRUCK:
      res = __Allgatherv_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLGATHERV_RING:
      res = __Allgatherv_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, coll_type, schedule, info);
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
static inline int __Allgatherv_gatherv_broadcast(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

  int count = 0, buf_start = 0;
  int size, rank;
  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  for(int i = 0; i < size; i++) {
    count += recvcounts[i];
  }

  const void *tmp_sendbuf = sendbuf;
  int tmp_sendcount = sendcount;
  MPI_Datatype tmp_sendtype = sendtype;
  if(sendbuf == MPI_IN_PLACE && rank != 0) {
    tmp_sendbuf = recvbuf + displs[rank];
    tmp_sendcount = recvcounts[rank];
    tmp_sendtype = recvtype;
  }

 __Gatherv_switch(tmp_sendbuf, tmp_sendcount, tmp_sendtype, recvbuf, recvcounts, displs, recvtype, 0, comm, coll_type, schedule, info);
 __Barrier_type(coll_type, schedule, info);
 __Bcast_switch(recvbuf, count, recvtype, 0, comm, coll_type, schedule, info);

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
static inline int __Allgatherv_distance_doubling(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Allgatherv_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Allgatherv_ring(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *displs, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALL
  ***********/

int PMPI_Ialltoall (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
static inline int __Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __Alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoall_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoall_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    res = __Ialltoall (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(tmp->nbc_handle));
  }
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
static inline int __Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Alltoall_init (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, &(req->nbc_handle));
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
static inline int __Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  //return __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);


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
static inline int __Alltoall_switch(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Alltoall_cluster(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_BRUCK:
      res = __Alltoall_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALL_PAIRWISE:
      res = __Alltoall_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, coll_type, schedule, info);
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
static inline int __Alltoall_cluster(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    // Copy data to avoid erasing it by receiving other's data
    __Copy_type(recvbuf, recvcount * size, recvtype, tmpbuf, recvcount * size, recvtype, comm, coll_type, schedule, info);

    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }
      __Send_type(tmpbuf  + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __Send_type(sendbuf + i * sendcount * sendext, sendcount, sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + i * recvcount * recvext, recvcount, recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
    // Copy our own data in the recvbuf
    __Copy_type(sendbuf + rank * sendcount * sendext, sendcount, sendtype, recvbuf + rank * recvcount * recvext, recvcount, recvtype, comm, coll_type, schedule, info);

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
static inline int __Alltoall_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Alltoall_pairwise(__UNUSED__ const void *sendbuf, __UNUSED__ int sendcount, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ int recvcount, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALLV
  ***********/

int PMPI_Ialltoallv (const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
static inline int __Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
static inline int __Alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallv_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallv_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res = __Ialltoallv (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(tmp->nbc_handle));
  }
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
static inline int __Ialltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Alltoallv_init (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, &(req->nbc_handle));
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
static inline int __Alltoallv_init(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Alltoallv_switch(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {
  //return __Alltoall_switch(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);

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
static inline int __Alltoallv_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Alltoallv_cluster(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_BRUCK:
      res = __Alltoallv_bruck(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLV_PAIRWISE:
      res = __Alltoallv_pairwise(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, coll_type, schedule, info);
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
static inline int __Alltoallv_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    // Copy data to avoid erasing it by receiving other's data
    __Copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __Send_type(tmpbuf  + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __Send_type(sendbuf + sdispls[i], sendcounts[i], sendtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtype, i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
    // Copy our own data in the recvbuf
    __Copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtype, recvbuf + rdispls[rank], recvcounts[rank], recvtype, comm, coll_type, schedule, info);

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
static inline int __Alltoallv_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Alltoallv_pairwise(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ MPI_Datatype sendtype, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ MPI_Datatype recvtype, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  ALLTOALLW
  ***********/

int PMPI_Ialltoallw (const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request);
static inline int __Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm);
static inline int __Alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallw_bruck(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Alltoallw_pairwise(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res = __Ialltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(tmp->nbc_handle));
  }
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
static inline int __Ialltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Alltoallw_init (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, &(req->nbc_handle));
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
static inline int __Alltoallw_init(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, NBC_Handle* handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Alltoallw_switch(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm) {

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
static inline int __Alltoallw_switch(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Alltoallw_cluster(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_BRUCK:
      res = __Alltoallw_bruck(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
      break;
    case NBC_ALLTOALLW_PAIRWISE:
      res = __Alltoallw_pairwise(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, coll_type, schedule, info);
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
static inline int __Alltoallw_cluster(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
    // Copy data to avoid erasing it by receiving other's data
    __Copy_type(recvbuf, rsize, MPI_BYTE, tmpbuf, rsize, MPI_BYTE, comm, coll_type, schedule, info);

    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __Send_type(tmpbuf  + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }

  } else {
    // For each rank, send and recv the data needed
    for(int i = 0; i < size; i++) {
      if(i == rank) {
        continue;
      }

      __Send_type(sendbuf + sdispls[i], sendcounts[i], sendtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
      __Recv_type(recvbuf + rdispls[i], recvcounts[i], recvtypes[i], i, MPC_ALLTOALL_TAG, comm, coll_type, schedule, info);
    }
    // Copy our own data in the recvbuf
    __Copy_type(sendbuf + sdispls[rank], sendcounts[rank], sendtypes[rank], recvbuf + rdispls[rank], recvcounts[rank], recvtypes[rank], comm, coll_type, schedule, info);

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
static inline int __Alltoallw_bruck(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ const MPI_Datatype *sendtypes, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ const MPI_Datatype *recvtypes, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

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
static inline int __Alltoallw_pairwise(__UNUSED__ const void *sendbuf, __UNUSED__ const int *sendcounts, __UNUSED__ const int *sdispls, __UNUSED__ const MPI_Datatype *sendtypes, __UNUSED__ void *recvbuf, __UNUSED__ const int *recvcounts, __UNUSED__ const int *rdispls, __UNUSED__ const MPI_Datatype *recvtypes, __UNUSED__ MPI_Comm comm, __UNUSED__ MPC_COLL_TYPE coll_type, __UNUSED__ NBC_Schedule * schedule, __UNUSED__ Sched_info *info) {

  not_implemented();

  return MPI_SUCCESS;
}




/***********
  SCAN
  ***********/

int PMPI_Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
static inline int __Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __Scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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
    
    res = __Iscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
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
static inline int __Iscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Scan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
static inline int __Scan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_scan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __Scan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Scan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Scan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_SCAN_ALLGATHER:
      res = __Scan_allgather(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
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
static inline int __Scan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
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

  // Copy data to recvbuf
  if(sendbuf != MPI_IN_PLACE) {
    __Copy_type(sendbuf, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }

  // recv & reduce data from rank-1
  if(rank > 0) {
    __Recv_type(tmpbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    __Barrier_type(coll_type, schedule, info);
    __Op_type(NULL, tmpbuf, recvbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  // send data to rank+1
  if(rank + 1 < size) {
    __Send_type(recvbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
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
static inline int __Scan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
 
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


  __Allgather_switch(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  __Barrier_type(coll_type, schedule, info);

  for(int i = 1; i <= rank; i++) {
    __Op_type(NULL, tmpbuf + (i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
  }

  __Copy_type(tmpbuf + rank * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);

  return MPI_SUCCESS;
}




/***********
  EXSCAN
  ***********/

int PMPI_Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
static inline int __Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle);
int PMPI_Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
static inline int __Exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);
static inline int __Exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res = __Iexscan (sendbuf, recvbuf, count, datatype, op, comm, &(tmp->nbc_handle));
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
static inline int __Iexscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle *handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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
  __Exscan_init (sendbuf, recvbuf, count, datatype, op, comm, &(req->nbc_handle));
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
static inline int __Exscan_init (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
int mpc_mpi_exscan (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  return __Exscan_switch(sendbuf, recvbuf, count, datatype, op, comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
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
static inline int __Exscan_switch (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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
      res = __Exscan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
      break;
    case NBC_EXSCAN_ALLGATHER:
      res = __Exscan_linear(sendbuf, recvbuf, count, datatype, op, comm, coll_type, schedule, info);
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
static inline int __Exscan_linear (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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

  const void *initial_buf = sendbuf;
  if(sendbuf == MPI_IN_PLACE) {
    initial_buf = recvbuf;
  }

  if(rank > 0) {
    // Copy data in tmpbuf
    __Copy_type(initial_buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  
    // recv data from rank-1
    __Recv_type(recvbuf, count, datatype, rank - 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);

    __Barrier_type(coll_type, schedule, info);

    // reduce data
    if(rank > 1 && rank < size - 1) {
      __Op_type(NULL, recvbuf, tmpbuf, count, datatype, op, mpc_op, coll_type, schedule, info);
    }
  }

  // send data to rank+1
  if(rank + 1 < size) {
    if(rank == 0) {
      __Send_type(initial_buf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
    } else {
      __Send_type(tmpbuf, count, datatype, rank + 1, MPC_SCAN_TAG, comm, coll_type, schedule, info);
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
static inline int __Exscan_allgather (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {

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


  __Allgather_switch(buf, count, datatype, tmpbuf, count, datatype, comm, coll_type, schedule, info);
  __Barrier_type(coll_type, schedule, info);

  if(rank > 0) {
    for(int i = 1; i < rank; i++) {
      __Op_type(NULL, tmpbuf + (i-1) * count * ext, tmpbuf + i * count * rank, count, datatype, op, mpc_op, coll_type, schedule, info);
    }

    __Copy_type(tmpbuf + (rank - 1) * count * ext, count, datatype, recvbuf, count, datatype, comm, coll_type, schedule, info);
  }
  return MPI_SUCCESS;
}




/***********
  BARRIER
  ***********/

int PMPI_Ibarrier (MPI_Comm comm, MPI_Request *request);
static inline int __Ibarrier (MPI_Comm comm, NBC_Handle *handle);
int PMPI_Barrier_init (MPI_Comm comm, MPI_Info info, MPI_Request *request);
static inline int __Barrier_init (MPI_Comm comm, NBC_Handle* handle);
int mpc_mpi_barrier (MPI_Comm comm);
static inline int __Barrier_reduce_broadcast (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info);


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

    res = __Ibarrier (comm, &(tmp->nbc_handle));
  }

  MPI_HANDLE_RETURN_VAL (res, comm);
}

/**
  \brief Initialize NBC structures used in call of non-blocking Barrier
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int __Ibarrier (MPI_Comm comm, NBC_Handle *handle) {
  
  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);
  handle->tmpbuf = NULL;
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Barrier_switch(comm, MPC_COLL_TYPE_COUNT, NULL, &info);

  __sched_alloc_init(handle, schedule, &info);

  __Barrier_switch(comm, MPC_COLL_TYPE_NONBLOCKING, schedule, &info);
  
  res = __Sched_commit(schedule, &info);
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

  MPI_internal_request_t *req;
  SCTK__MPI_INIT_REQUEST (request);
  req = __sctk_new_mpc_request_internal (request,__sctk_internal_get_MPC_requests());
  req->freeable = 0;
  req->is_active = 0;
  req->is_nbc = 1;
  req->is_persistent = 1;
  req->req.request_type = REQUEST_GENERALIZED;

  req->persistant.op = MPC_MPI_PERSISTENT_BARRIER_INIT;
  req->persistant.info = info;

  /* Init metadata for nbc */
  __Barrier_init (comm, &(req->nbc_handle));
  req->nbc_handle.is_persistent = 1;
  
  return MPI_SUCCESS;
}

/**
  \brief Initialize NBC structures used in call of persistent Barrier
  \param comm Target communicator
  \param handle Pointer to the NBC_Handle
  \return error code
  */
static inline int __Barrier_init (MPI_Comm comm, NBC_Handle* handle) {

  int res;
  NBC_Schedule *schedule;
  Sched_info info;
  __sched_info_init(&info);

  res = NBC_Init_handle(handle, comm, MPC_IBCAST_TAG);

  handle->tmpbuf = NULL;

  /* alloc schedule */
  schedule = (NBC_Schedule *)sctk_malloc(sizeof(NBC_Schedule));

  __Barrier_switch(comm, MPC_COLL_TYPE_COUNT, NULL, &info);
  
  __sched_alloc_init(handle, schedule, &info);
  
  __Barrier_switch(comm, MPC_COLL_TYPE_PERSISTENT, schedule, &info);

  res = __Sched_commit(schedule, &info);
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
  \param comm Target communicator
  \return error code
  */
int mpc_mpi_barrier (MPI_Comm comm) {
  return __Barrier_switch(comm, MPC_COLL_TYPE_BLOCKING, NULL, NULL);
}


/**
  \brief Swith between the different Barrier algorithms
  \param comm Target communicator
  \param coll_type Type of the communication
  \param schedule Adress of the schedule
  \param info Adress on the information structure about the schedule
  \return error code
  */
static inline int __Barrier_switch (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
  enum {
    NBC_BARRIER_REDUCE_BROADCAST
  } alg;

  int size;
  _mpc_cl_comm_size(comm, &size);
  
  alg = NBC_BARRIER_REDUCE_BROADCAST;

  int res;

  switch(alg) {
    case NBC_BARRIER_REDUCE_BROADCAST:
      res = __Barrier_reduce_broadcast (comm, coll_type, schedule, info);
      break;
  }

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
static inline int __Barrier_reduce_broadcast (MPI_Comm comm, MPC_COLL_TYPE coll_type, NBC_Schedule * schedule, Sched_info *info) {
  
  static char bar = 0;

  int size, rank;

  _mpc_cl_comm_size(comm, &size);
  _mpc_cl_comm_rank(comm, &rank);

  int maxr, peer;
  maxr = __ceiled_log2(size);
  int rmb = __rmb_index(rank);
  if(rank == 0) {
    rmb = maxr;
  }

  // Reduce | Broadcast

  if(rank != 0) { 
    peer = rank ^ (1 << rmb);
    __Recv_type(&bar, 1, MPI_CHAR, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);

    if(rmb != 0) {
      __Barrier_type(coll_type, schedule, info);
    }
  }

  for(int i = rmb - 1; i >= 0; i--) {
    peer = rank ^ (1 << i);
    if(peer >= size) {
      continue;
    }

    __Send_type(& bar, 1, MPI_CHAR, peer, MPC_BROADCAST_TAG, comm, coll_type, schedule, info);
  }
  

  __Barrier_type(coll_type, schedule, info);


  for(int i = 0; i < maxr; i++) {
    if(rank & (1 << i)) {
      peer = rank ^ (1 << i);
      __Barrier_type(coll_type, schedule, info);
      __Send_type(&bar, 1, MPI_CHAR, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
      
      break;
    } else if((rank | (1 << i)) < size) {
      peer = rank | (1 << i);
      __Recv_type(&bar, 1, MPI_CHAR, peer, MPC_REDUCE_TAG, comm, coll_type, schedule, info);
    }
  }
  
  return MPI_SUCCESS;
}








