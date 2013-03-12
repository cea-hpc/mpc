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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#define MAX_MPC_BUFFERED_MSG 16
#define MAX_MPC_BUFFERED_SIZE (128 * sizeof(long))
#include <uthash.h>

typedef struct mpc_buffered_msg_s
{
  sctk_thread_ptp_message_t header;
  /* Completion flag to use if the user do not provide a valid request */
  int completion_flag;
  /* MPC_Request if the message is buffered  */
  MPC_Request request;
  long buf[(MAX_MPC_BUFFERED_SIZE / sizeof (long)) + 1];
} mpc_buffered_msg_t;

typedef struct
{
  size_t size;
  mpc_pack_absolute_indexes_t *begins;
  mpc_pack_absolute_indexes_t *ends;
  unsigned long count;
  unsigned long ref_count;
  mpc_pack_absolute_indexes_t lb;
  mpc_pack_absolute_indexes_t ub;
  int is_lb;
  int is_ub;
} sctk_derived_type_t;

#define MPC_DECL_TYPE_PROTECTED(data_type, name_type) typedef struct {data_type; sctk_spinlock_t lock;} name_type##_t
#define MPC_USE_TYPE(name_type) name_type##_t name_type

MPC_DECL_TYPE_PROTECTED (sctk_datatype_t user_types[sctk_user_data_types_max],
			 user_types);
MPC_DECL_TYPE_PROTECTED (sctk_derived_type_t *
			 user_types_struct[sctk_user_data_types_max],
			 user_types_struct);

MPC_DECL_TYPE_PROTECTED (mpc_buffered_msg_t buffer[MAX_MPC_BUFFERED_MSG];
			 volatile int buffer_rank, buffer);
MPC_DECL_TYPE_PROTECTED (mpc_buffered_msg_t
			 buffer_async[MAX_MPC_BUFFERED_MSG];
			 volatile int buffer_async_rank, buffer_async);

struct mpc_mpi_per_communicator_s;

typedef struct {
  sctk_communicator_t key;

  sctk_spinlock_t err_handler_lock;
  MPC_Handler_function*  err_handler;

  struct mpc_mpi_per_communicator_s* mpc_mpi_per_communicator;
  void (*mpc_mpi_per_communicator_copy)(struct mpc_mpi_per_communicator_s**,struct mpc_mpi_per_communicator_s*);
  void (*mpc_mpi_per_communicator_copy_dup)(struct mpc_mpi_per_communicator_s**,struct mpc_mpi_per_communicator_s*);

  UT_hash_handle hh;
}mpc_per_communicator_t;

struct sctk_task_specific_s
{
  int task_id;

  MPC_USE_TYPE (user_types);
  MPC_USE_TYPE (user_types_struct);

  mpc_per_communicator_t*per_communicator;
  sctk_spinlock_t per_communicator_lock;

  struct mpc_mpi_data_s* mpc_mpi_data;

  struct sctk_internal_ptp_s* my_ptp_internal;

  int init_done;  /* =1 if the task has called MPI_Init() */
  int finalize_done; /* =1 if the task has already called MPI_Finalize()  */
};

mpc_per_communicator_t* sctk_thread_getspecific_mpc_per_comm(struct sctk_task_specific_s* task_specific,sctk_communicator_t comm);
struct sctk_task_specific_s *__MPC_get_task_specific ();

typedef struct sctk_task_specific_s sctk_task_specific_t;


struct sctk_thread_specific_s
{
  MPC_USE_TYPE (buffer);
  MPC_USE_TYPE (buffer_async);
};
typedef struct sctk_thread_specific_s sctk_thread_specific_t;

