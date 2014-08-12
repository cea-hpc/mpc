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

#ifndef MPC_COMMON_H
#define MPC_COMMON_H

#include <uthash.h>
#include "mpc_info.h"

/************************************************************************/
/* Datatypes definition                                                 */
/************************************************************************/

typedef struct
{
	size_t size;
	size_t nb_elements;
	mpc_pack_absolute_indexes_t *begins;
	mpc_pack_absolute_indexes_t *ends;
	unsigned long count;
	unsigned long ref_count;
	mpc_pack_absolute_indexes_t lb;
	mpc_pack_absolute_indexes_t ub;
	int is_lb;
	int is_ub;
} sctk_derived_type_t;

typedef struct 
{
	size_t id_rank;
	size_t size;
	size_t count;
	sctk_datatype_t datatype;
	int used;
} sctk_other_datatype_t;


typedef struct 
{
	sctk_datatype_t user_types[SCTK_USER_DATA_TYPES_MAX];
	sctk_spinlock_t lock;
} user_types_t;

typedef struct 
{
	sctk_other_datatype_t other_user_types[SCTK_USER_DATA_TYPES_MAX];
	sctk_spinlock_t lock;
} other_user_types_t;

typedef struct 
{
	sctk_derived_type_t * user_types_struct[SCTK_USER_DATA_TYPES_MAX];
	sctk_spinlock_t lock;
} user_types_struct_t;

/************************************************************************/
/* Buffers definition                                                   */
/************************************************************************/
 
#define MAX_MPC_BUFFERED_MSG 32
#define MAX_MPC_BUFFERED_SIZE (128 * sizeof(long))

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
	mpc_buffered_msg_t buffer[MAX_MPC_BUFFERED_MSG];
	volatile int buffer_rank;
	sctk_spinlock_t lock;
} buffer_t;

typedef struct 
{
	mpc_buffered_msg_t buffer_async[MAX_MPC_BUFFERED_MSG];
	volatile int buffer_async_rank;
	sctk_spinlock_t lock;
} buffer_async_t;

/************************************************************************/
/* Per communicator context                                             */
/************************************************************************/

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

/************************************************************************/
/* Per task context                                                     */
/************************************************************************/

/**
 *  \brief Describes the context of an MPI task
 *  
 *  This data structure is initialised by \ref __MPC_init_task_specific_t and
 * 	released by \ref __MPC_release_task_specific_t. Initial setup is done
 *  in \ref __MPC_setup_task_specific called in \ref sctk_user_main.
 * 
 */
typedef struct sctk_task_specific_s
{
	/* ID */
	int task_id; /**< MPI comm rank of the task */

	/* Status */
	int init_done;  /**< =1 if the task has called MPI_Init() */
	int finalize_done; /**< =1 if the task has already called MPI_Finalize()  */
	int thread_level;

	/* Types */
	user_types_t user_types;
	other_user_types_t other_user_types;
	user_types_struct_t user_types_struct;

	/* Communicator handling */
	mpc_per_communicator_t*per_communicator;
	sctk_spinlock_t per_communicator_lock;

	/* TODO */
	struct mpc_mpi_data_s* mpc_mpi_data;
	struct sctk_internal_ptp_s* my_ptp_internal;

	/* MPI_Info handling */
	struct MPC_Info_factory info_fact; /**< This structure is used to store the association between MPI_Infos structs and their ID */
} sctk_task_specific_t;

/** \brief Retrieves current thread task specific context
 */
struct sctk_task_specific_s *__MPC_get_task_specific ();

/** \brief Retrieves a given per communicator context from task CTX
 */
mpc_per_communicator_t* sctk_thread_getspecific_mpc_per_comm(struct sctk_task_specific_s* task_specific,sctk_communicator_t comm);


/************************************************************************/
/* Per thread context                                                   */
/************************************************************************/

struct sctk_thread_specific_s
{
  buffer_t buffer;
  buffer_async_t buffer_async;
};
typedef struct sctk_thread_specific_s sctk_thread_specific_t;

#endif /* MPC_COMMON_H */
