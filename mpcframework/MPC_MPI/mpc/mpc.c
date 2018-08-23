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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */
#define MPC_MAIN_FILE
#include "mpc.h"
#undef main

#include "mpc_reduction.h"
#include "mpcthread.h"
#include "sctk.h"
#include "sctk_accessor.h"
#include "sctk_atomics.h"
#include "sctk_collective_communications.h"
#include "sctk_communicator.h"
#include "sctk_debug.h"
#include "sctk_handle.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_route.h"
#include "sctk_runtime_config.h"
#include "sctk_stdint.h"
#include "sctk_topology.h"
#include <mpcmp.h>
#include <sctk_ethread_internal.h>
#include <sys/time.h>
#include "sctk_handle.h"
/*#define MPC_LOG_DEBUG*/
#ifdef MPC_LOG_DEBUG
#include <stdarg.h>
#endif

#include "mpc_common.h"
#include <mpc_internal_thread.h>
#ifdef MPC_OpenMP
#include "mpcomp_core.h"
#endif

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_weak.h"
#endif

#ifdef MPC_Message_Passing
#include "sctk_low_level_comm.h"
#endif

#ifdef MPC_Fault_Tolerance
#include "sctk_ft_iface.h"
#endif

MPC_Op_f sctk_get_common_function(MPC_Datatype datatype, MPC_Op op);

/************************************************************************/
/*MPC STATUS                                                            */
/************************************************************************/
int MPC_Config_Status_MPC_HAVE_OPTION_HLS();
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW();
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED();

int MPC_Config_Status(MPC_Config_Status_t option) {
  int res = 0;

  if (option == MPC_HAVE_OPTION_HLS) {
    res = MPC_Config_Status_MPC_HAVE_OPTION_HLS();
  }
  if (option == MPC_HAVE_OPTION_ETLS_COW) {
    res = MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW();
  }
  if (option == MPC_HAVE_OPTION_ETLS_OPTIMIZED) {
    res = MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED();
  }

  return res;
}

void MPC_Config_Status_Print(FILE *fd) {
  int i;
  fprintf(fd, "MPC Status:\n");
  for (i = 0; i < MPC_HAVE_OPTION_END; i++) {
    fprintf(fd, "\t- %s %d\n", MPC_Config_Status_Name(i), MPC_Config_Status(i));
  }
}

char *MPC_Config_Status_Name(MPC_Config_Status_t option) {
  if (option == MPC_HAVE_OPTION_HLS) {
    return "MPC_HAVE_OPTION_HLS";
  }
  if (option == MPC_HAVE_OPTION_ETLS_COW) {
    return "MPC_HAVE_OPTION_ETLS_COW";
  }
  if (option == MPC_HAVE_OPTION_ETLS_OPTIMIZED) {
    return "MPC_HAVE_OPTION_ETLS_OPTIMIZED";
  }
  return "Unknown Option";
}
/************************************************************************/
/*GLOBAL VARIABLES                                                      */
/************************************************************************/
sctk_thread_key_t sctk_task_specific;
static sctk_thread_key_t sctk_check_point_key;
static sctk_thread_key_t sctk_func_key;
static int mpc_disable_buffering = 0;

const MPC_Group_t mpc_group_empty = {0, NULL};
const MPC_Group_t mpc_group_null = {-1, NULL};
MPC_Request mpc_request_null;

/** \brief Intitializes thread context keys
 * This function is called in sctk_thread.c:2212
 * by sctk_start_func
 */
void sctk_mpc_init_keys() {
  sctk_thread_key_create(&sctk_func_key, NULL);
  sctk_thread_key_create(&sctk_check_point_key, NULL);
  sctk_thread_key_create(&sctk_task_specific, NULL);
}

static inline void *sctk_thread_getspecific_mpc(sctk_thread_key_t key) {
  return sctk_thread_getspecific(key);
}

static inline void sctk_thread_setspecific_mpc(sctk_thread_key_t key,
                                               void *tmp) {
  sctk_thread_setspecific(key, tmp);
}

inline mpc_per_communicator_t *
sctk_thread_getspecific_mpc_per_comm(sctk_task_specific_t *task_specific,
                                     sctk_communicator_t comm) {
  mpc_per_communicator_t *per_communicator;

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh, task_specific->per_communicator, &comm,
            sizeof(sctk_communicator_t), per_communicator);
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  return per_communicator;
}

static inline void
sctk_thread_addspecific_mpc_per_comm(sctk_task_specific_t *task_specific,
                                     mpc_per_communicator_t *mpc_per_comm,
                                     sctk_communicator_t comm) {
  mpc_per_communicator_t *per_communicator;
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh, task_specific->per_communicator, &comm,
            sizeof(sctk_communicator_t), per_communicator);
  assume(per_communicator == NULL);
  mpc_per_comm->key = comm;
  //~ sctk_nodebug("Add %d %p (%d)",comm,mpc_per_comm,task_specific->task_id);

  /*{
    mpc_per_communicator_t* pair;
    mpc_per_communicator_t* tmp;
    HASH_ITER(hh,task_specific->per_communicator,pair,tmp){
      sctk_nodebug("Key before INS %d",pair->key);
    }
  }*/

  HASH_ADD(hh, task_specific->per_communicator, key,
           sizeof(sctk_communicator_t), mpc_per_comm);

  /*{
    mpc_per_communicator_t* pair;
    mpc_per_communicator_t* tmp;
    HASH_ITER(hh,task_specific->per_communicator,pair,tmp){
      sctk_nodebug("Key after INS %d",pair->key);
    }
  }*/

  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  return;
}

static inline void
sctk_thread_removespecific_mpc_per_comm(sctk_task_specific_t *task_specific,
                                        sctk_communicator_t comm) {
  mpc_per_communicator_t *per_communicator;
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));

  /* { */
  /*   mpc_per_communicator_t* pair; */
  /*   mpc_per_communicator_t* tmp; */
  /*   HASH_ITER(hh,task_specific->per_communicator,pair,tmp){ */
  /*     sctk_nodebug("Before DEL Key %d",pair->key); */
  /*   } */
  /* } */

  HASH_FIND(hh, task_specific->per_communicator, &comm,
            sizeof(sctk_communicator_t), per_communicator);

  assume(per_communicator != NULL);
  assume(per_communicator->key == comm);

  HASH_DELETE(hh, task_specific->per_communicator, per_communicator);

  /* { */
  /*   mpc_per_communicator_t* pair; */
  /*   mpc_per_communicator_t* tmp; */
  /*   HASH_ITER(hh,task_specific->per_communicator,pair,tmp){ */
  /*     sctk_nodebug("After DEL Key %d",pair->key); */
  /*   } */
  /* } */

  sctk_nodebug("Remove %d %p (%d)", comm, per_communicator,
               task_specific->task_id);
  sctk_free(per_communicator);
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
}

static inline mpc_per_communicator_t *
sctk_thread_createspecific_mpc_per_comm() {
  mpc_per_communicator_t *tmp;
  sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;

  tmp = sctk_malloc(sizeof(mpc_per_communicator_t));

  tmp->err_handler = (MPC_Handler_function *)PMPC_Default_error;
  tmp->err_handler_lock = lock;
  tmp->mpc_mpi_per_communicator = NULL;
  tmp->mpc_mpi_per_communicator_copy = NULL;
  tmp->mpc_mpi_per_communicator_copy_dup = NULL;

  sctk_nodebug("Allocate new per comm %p", tmp);
  return tmp;
}

static inline void
sctk_thread_createnewspecific_mpc_per_comm(sctk_task_specific_t *task_specific,
                                           sctk_communicator_t new_comm,
                                           sctk_communicator_t old_comm) {
  mpc_per_communicator_t *per_communicator;
  mpc_per_communicator_t *per_communicator_new;
  int i;
  assume(new_comm != old_comm);

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh, task_specific->per_communicator, &old_comm,
            sizeof(sctk_communicator_t), per_communicator);
  assume(per_communicator != NULL);

  per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
  memcpy(per_communicator_new, per_communicator,
         sizeof(mpc_per_communicator_t));

  if (per_communicator->mpc_mpi_per_communicator_copy != NULL) {
    per_communicator->mpc_mpi_per_communicator_copy(
        &(per_communicator_new->mpc_mpi_per_communicator),
        per_communicator->mpc_mpi_per_communicator);
  }

  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  assert(per_communicator_new->mpc_mpi_per_communicator);
  sctk_thread_addspecific_mpc_per_comm(task_specific, per_communicator_new,
                                       new_comm);
}

static inline void sctk_thread_createspecific_mpc_per_comm_from_existing(
    sctk_task_specific_t *task_specific, sctk_communicator_t new_comm,
    sctk_communicator_t old_comm) {
  mpc_per_communicator_t *per_communicator;
  mpc_per_communicator_t *per_communicator_new;

  assume(new_comm != old_comm);

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh, task_specific->per_communicator, &old_comm,
            sizeof(sctk_communicator_t), per_communicator);
  assume(per_communicator != NULL);

  per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
  memcpy(per_communicator_new, per_communicator,
         sizeof(mpc_per_communicator_t));

  if (per_communicator->mpc_mpi_per_communicator_copy != NULL) {
    per_communicator->mpc_mpi_per_communicator_copy(
        &(per_communicator_new->mpc_mpi_per_communicator),
        per_communicator->mpc_mpi_per_communicator);
  }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

  sctk_thread_addspecific_mpc_per_comm(task_specific, per_communicator_new,
                                       new_comm);
}

static inline void sctk_thread_createspecific_mpc_per_comm_from_existing_dup(
    sctk_task_specific_t *task_specific, sctk_communicator_t new_comm,
    sctk_communicator_t old_comm) {
  mpc_per_communicator_t *per_communicator;
  mpc_per_communicator_t *per_communicator_new;

  assume(new_comm != old_comm);

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh, task_specific->per_communicator, &old_comm,
            sizeof(sctk_communicator_t), per_communicator);
  assume(per_communicator != NULL);

  per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
  memcpy(per_communicator_new, per_communicator,
         sizeof(mpc_per_communicator_t));

  if (per_communicator->mpc_mpi_per_communicator_copy != NULL) {
    per_communicator->mpc_mpi_per_communicator_copy_dup(
        &(per_communicator_new->mpc_mpi_per_communicator),
        per_communicator->mpc_mpi_per_communicator);
  }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

  sctk_thread_addspecific_mpc_per_comm(task_specific, per_communicator_new,
                                       new_comm);
}

/************************************************************************/
/*ACCESSORS                                                             */
/************************************************************************/

static inline void sctk_mpc_verify_request_compatibility() {
  /*     MPC_Request mpc_req; */
  /*     sctk_request_t sctk_req; */

  /*Verify type compatibility*/
  /*     sctk_check_equal_types (MPC_Request, sctk_request_t); */
  /*     memset (&sctk_req, 0, sizeof (sctk_request_t)); */
  /*     memset (&mpc_req, 0, sizeof (MPC_Request)); */
  /*     mpc_req.msg = (void *) sctk_user_main; */
  /*     mpc_req.completion_flag = -1; */
  /*     memcpy (&sctk_req, &mpc_req, sizeof (sctk_request_t)); */
  /*     assume ((void *) mpc_req.msg == (void *) sctk_req.msg); */
  /*     assume ((int) mpc_req.completion_flag == (int)
   * sctk_req.completion_flag); */
}

static inline void sctk_mpc_commit_status_from_request(MPC_Request *request,
                                                       MPC_Status *status) {
  if (request->request_type == REQUEST_GENERALIZED) {
    MPC_Status static_status;

    /* You must provide a valid status to the querry function */
    if (status == MPC_STATUS_IGNORE)
      status = &static_status;

	memset( status, 0 , sizeof(MPC_Status));
    /* Fill in the status info */
    (request->query_fn)(request->extra_state, status);
    /* Free the request */
    (request->free_fn)(request->extra_state);
  } else if (status != MPC_STATUS_IGNORE) {
    status->MPC_SOURCE = request->header.source_task;
    status->MPC_TAG = request->header.message_tag;
    status->MPC_ERROR = MPC_SUCCESS;

    if (request->truncated) {
      status->MPC_ERROR = MPC_ERR_TRUNCATE;
    }

    status->size = request->header.msg_size;

    if (request->completion_flag == SCTK_MESSAGE_CANCELED) {
      status->cancelled = 1;
    } else {
      status->cancelled = 0;
    }
  }
}

static inline sctk_communicator_t
sctk_mpc_get_communicator_from_request(MPC_Request *request) {
  return ((sctk_request_t *)request)->header.communicator;
}


void __MPC_poll_progress();

static inline void sctk_mpc_perform_messages(MPC_Request *request) {
  struct sctk_perform_messages_s _wait;
  if (request->request_type == REQUEST_GENERALIZED) {
    /* Try to poll the request */
    __MPC_poll_progress();
    /* We are done here */
    return;
  }

  sctk_perform_messages_wait_init(&_wait, request, 0);
  sctk_perform_messages_wait_init_request_type(&_wait);

  sctk_perform_messages(&_wait);
}

static inline int sctk_mpc_completion_flag(MPC_Request *request) {
  return request->completion_flag;
}

static inline void sctk_mpc_init_request_null() {
  mpc_request_null.is_null = 1;
  mpc_request_null.msg = NULL;
  mpc_request_null.request_type = REQUEST_NULL;
  mpc_request_null.completion_flag = SCTK_MESSAGE_DONE;
  mpc_request_null.truncated = 0;
  mpc_request_null.grequest_rank = -1;
  mpc_request_null.query_fn = NULL;
  mpc_request_null.cancel_fn = NULL;
  mpc_request_null.free_fn = NULL;
  mpc_request_null.wait_fn = NULL;
  mpc_request_null.poll_fn = NULL;
  mpc_request_null.extra_state = NULL;
  mpc_request_null.pointer_to_source_request = NULL;
  mpc_request_null.pointer_to_shadow_request = NULL;
}

void sctk_mpc_init_request(MPC_Request *request, MPC_Comm comm, int src,
                           int request_type) {
  sctk_init_request(request, comm, request_type);
}

static inline void
sctk_mpc_register_message_in_request(MPC_Request *request,
                                     sctk_thread_ptp_message_t *msg) {
  request->msg = msg;
}

static inline sctk_thread_ptp_message_t *
sctk_mpc_get_message_in_request(MPC_Request *request) {
  return request->msg;
}

static inline void sctk_mpc_init_message_size(MPC_Request *request) {
  request->SCTK_MSG_SIZE(msg) = 0;
}

static inline void sctk_mpc_add_to_message_size(MPC_Request *request,
                                                size_t size) {
  request->SCTK_MSG_SIZE(msg) += size;
}

static inline size_t sctk_mpc_get_message_size(MPC_Request *request) {
  return request->SCTK_MSG_SIZE(msg);
}

static inline int sctk_mpc_get_message_source(MPC_Request *request) {
  return request->header.source_task;
}

static inline int sctk_mpc_message_is_null(MPC_Request *request) {
  return request->is_null;
}

static inline void sctk_mpc_message_set_is_null(MPC_Request *request, int val) {
  request->is_null = val;
}

static inline int sctk_mpc_message_get_is_null(MPC_Request *request) {
  return request->is_null;
}

static inline void sctk_mpc_set_header_in_message(
    sctk_thread_ptp_message_t *msg, const int message_tag,
    const sctk_communicator_t communicator, const int source,
    const int destination, MPC_Request *request, const size_t count,
    sctk_message_class_t specific_message_tag, MPC_Datatype datatype,
    sctk_request_type_t request_type) {

  sctk_set_header_in_message(msg, message_tag, communicator, source,
                             destination, request, count, specific_message_tag,
                             datatype, request_type);
}

static inline void sctk_mpc_wait_message(MPC_Request *request) {
  SCTK_PROFIL_START(sctk_mpc_wait_message);
  if (sctk_mpc_message_is_null(request) == 0) {
    sctk_wait_message(request);
  }
  SCTK_PROFIL_END(sctk_mpc_wait_message);
}

static inline void sctk_mpc_wait_all(const int task,
                                     const sctk_communicator_t com) {
  sctk_wait_all(task, com);
}

static inline int sctk_mpc_cancel_message(MPC_Request *msg) {
  return sctk_cancel_message(msg);
}

/** \brief This function allows the retrieval of a data-type context
 *  \param datatype Target datatype
 *  \return NULL on error the context otherwise
 */
static inline struct Datatype_context *
sctk_retrieve_datatype_ctx(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  sctk_contiguous_datatype_t *target_contiguous_type;
  sctk_derived_datatype_t *target_derived_type;

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    /* Nothing to do here */
    return NULL;
    break;

  case MPC_DATATYPES_CONTIGUOUS:

    /* Get a pointer to the type of interest */
    target_contiguous_type =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    return &target_contiguous_type->context;
    break;

  case MPC_DATATYPES_DERIVED:

    /* Get a pointer to the type of interest */
    target_derived_type =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    return &target_derived_type->context;
    break;

  case MPC_DATATYPES_UNKNOWN:
    return NULL;
    break;
  }

  return NULL;
}

/************************************************************************/
/*FUNCTIONS                                                             */
/************************************************************************/

/** \brief Return 0 if the process is the process root */
int MPC_Is_process_root() {
  if (sctk_get_local_task_rank() == 0)
    return 1;

  return 0;
}

/** \brief Returns 1 in MPC (has to be overrided in the app by a weask symbol)
 */
int MPC_Is_present() { return 1; }

static void sctk_no_free_header(void *tmp) {}

/************************************************************************/
/* MPC Asynchronous buffers                                             */
/************************************************************************/

void MPC_per_thread_buffer_pool_init(sctk_thread_buffer_pool_t *pool) {
#if defined(SCTK_USE_TLS)

  int i;

  for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++) {
    sctk_mpc_init_request(&(pool->sync.buffer[i].request), MPC_COMM_NULL,
                          MPC_PROC_NULL, REQUEST_NULL);
    sctk_mpc_init_request(&(pool->async.buffer[i].request), MPC_COMM_NULL,
                          MPC_PROC_NULL, REQUEST_NULL);
  }

  pool->sync.buffer_rank = 0;
  pool->async.buffer_rank = 0;

#else
  not_implemented();
#endif
}
void MPC_per_thread_buffer_pool_realease(sctk_thread_buffer_pool_t *pool) {
#if defined(SCTK_USE_TLS)
  int i;

  for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++) {
    sctk_nodebug("WAIT message %d type %d src %d dest %d tag %d size %ld", i,
                 tmp->sync.buffer[i].request.request_type,
                 tmp->sync.buffer[i].request.header.source,
                 tmp->sync.buffer[i].request.header.destination,
                 tmp->sync.buffer[i].request.header.message_tag,
                 tmp->sync.buffer[i].request.header.msg_size);

    sctk_mpc_wait_message(&(pool->sync.buffer[i].request));
  }

  for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++) {
    sctk_nodebug("WAIT message ASYNC %d", i);
    sctk_mpc_wait_message(&(pool->async.buffer[i].request));
  }
#else
  not_implemented();
#endif
}

mpc_buffered_msg_t *MPC_per_thread_buffer_pool_acquire_synchronous( sctk_thread_specific_t * thread_spec ) {
  sctk_thread_buffer_pool_t *pool = &(thread_spec->buffer_pool);

  mpc_buffered_msg_t *ret = NULL;

  int buffer_rank = pool->sync.buffer_rank;
  ret = &(pool->sync.buffer[buffer_rank]);

  return ret;
}

void MPC_per_thread_buffer_pool_step_synchronous(sctk_thread_specific_t * thread_spec) {
  sctk_thread_buffer_pool_t *pool = &(thread_spec->buffer_pool);

  int buffer_rank = pool->sync.buffer_rank;
  pool->sync.buffer_rank = (buffer_rank + 1) % MAX_MPC_BUFFERED_MSG;
}

mpc_buffered_msg_t *MPC_per_thread_buffer_pool_acquire_asynchronous() {
  sctk_thread_buffer_pool_t *pool = &(sctk_get_thread_specific()->buffer_pool);

  mpc_buffered_msg_t *ret = NULL;

  int buffer_rank = pool->async.buffer_rank;
  ret = &(pool->async.buffer[buffer_rank]);

  return ret;
}

void MPC_per_thread_buffer_pool_step_asynchronous() {
  sctk_thread_buffer_pool_t *pool = &(sctk_get_thread_specific()->buffer_pool);

  int buffer_rank = pool->async.buffer_rank;
  pool->async.buffer_rank = (buffer_rank + 1) % MAX_MPC_BUFFERED_MSG;
}

/************************************************************************/
/* MPC per thread context                                               */
/************************************************************************/

__thread struct sctk_thread_specific_s *___sctk_message_passing;

sctk_thread_specific_t *sctk_get_thread_specific() {
  if (!___sctk_message_passing)
    MPC_Init_thread_specific();

  return (sctk_thread_specific_t *)___sctk_message_passing;
}

void sctk_set_thread_specific(sctk_thread_specific_t *pointer) {
  ___sctk_message_passing = (struct sctk_thread_specific_s *)pointer;
}

void MPC_Init_thread_specific() {
  if (___sctk_message_passing)
    return;

  /* Allocate */

  sctk_thread_specific_t *tmp = NULL;
  tmp = sctk_malloc(sizeof(sctk_thread_specific_t));
  assume(tmp != NULL);
  memset(tmp, 0, sizeof(sctk_thread_specific_t));

  /* Initialize */
  MPC_per_thread_buffer_pool_init(&tmp->buffer_pool);

  /* Register thread context */
  sctk_set_thread_specific(tmp);
}

void MPC_Release_thread_specific() {
  if (!___sctk_message_passing)
    return;

  sctk_thread_specific_t *thread_specific = ___sctk_message_passing;

  /* Release */
  MPC_per_thread_buffer_pool_realease(&thread_specific->buffer_pool);

  /* Free */
  sctk_free(thread_specific);
  sctk_set_thread_specific(NULL);
}

/************************************************************************/
/* Task specific                                                        */
/************************************************************************/

#ifdef SCTK_PROCESS_MODE
struct sctk_task_specific_s *___the_process_specific = NULL;
#endif

static int * ___local_to_global_table = NULL;
static struct sctk_task_specific_s **___disguisements = NULL;
sctk_atomics_int ________is_disguised;

int __MPC_init_disguise( struct sctk_task_specific_s * my_specific )
{

    int my_id = sctk_get_local_task_rank();

    if( my_id == 0 )
    {
        sctk_atomics_store_int(&________is_disguised, 0);
        int local_count = sctk_get_local_task_number();
        ___disguisements = sctk_malloc( sizeof(struct sctk_task_specific_s *) * local_count );

        if( ___disguisements == NULL )
        {
            perror("malloc");
            abort();
        }

        ___local_to_global_table = sctk_malloc( sizeof(int) * local_count );

        if( ___local_to_global_table == NULL )
        {
            perror("malloc");
            abort();
        }

    }

    sctk_barrier( SCTK_COMM_WORLD );

    ___disguisements[ my_id ] = my_specific;
    ___local_to_global_table[ my_id ] = sctk_get_task_rank();

    sctk_barrier( SCTK_COMM_WORLD );

    return 0;
}

int MPCX_Disguise( MPC_Comm comm, int target_rank )
{

  sctk_thread_data_t * th = __sctk_thread_data_get(1);
  
  if( th->my_disguisement )
  {
    /* Sorry I'm already wearing a mask */
    return MPC_ERR_ARG;
  }

  /* Retrieve the ctx pointer */
  int cwr = sctk_get_comm_world_rank( (sctk_communicator_t) comm, target_rank );

  int local_count = sctk_get_local_task_number();
  
  int i;


  for (i = 0; i < local_count; ++i) {
      if( ___local_to_global_table[i] == cwr )
      {
        sctk_atomics_incr_int( &________is_disguised );
        th->my_disguisement = ___disguisements[i]->thread_data;
        th->ctx_disguisement = (void *)___disguisements[i];
        return MPC_SUCCESS;
      }
  }
  
  return MPC_ERR_ARG;
}

int MPCX_Undisguise()
{
  sctk_thread_data_t * th = __sctk_thread_data_get(1);

  if( th->my_disguisement == NULL )
      return MPC_ERR_ARG;


  th->my_disguisement = NULL;
  th->ctx_disguisement = NULL;
  sctk_atomics_decr_int( &________is_disguised );

  return MPC_SUCCESS;
}




static struct sctk_progress_engine_pool __mpc_progress_pool;



int __MPC_init_progress(sctk_task_specific_t * tmp )
{

  int my_local_id = sctk_get_local_task_rank();
  if( my_local_id == 0 )
  {
    int local_count = sctk_get_local_task_number();
    sctk_progress_engine_pool_init( &__mpc_progress_pool, local_count );
  }

  sctk_barrier(MPC_COMM_WORLD);

  /* Retrieve the ctx pointer */
  tmp->progress_list = sctk_progress_engine_pool_join( &__mpc_progress_pool );


}

int __MPC_release_progress( sctk_task_specific_t * tmp  )
{
    static sctk_spinlock_t l = 0;
    static sctk_spinlock_t d = 0;
  
    tmp->progress_list = NULL;

    int done = 0;
    sctk_spinlock_lock( &l );
    done = d;
	d=1;
    sctk_spinlock_unlock( &l );

    if( done )
        return MPC_SUCCESS;

    sctk_progress_engine_pool_release( &__mpc_progress_pool );
    return MPC_SUCCESS;
}


void __MPC_poll_progress_id(int id)
{
    return sctk_progress_engine_pool_poll( &__mpc_progress_pool , id  );
}

void __MPC_poll_progress()
{
    struct sctk_task_specific_s * spe = __MPC_get_task_specific();
    return __MPC_poll_progress_id( spe->progress_list->id );
}



/** \brief Initalizes a structure of type \ref sctk_task_specific_t
 */
static inline void __MPC_init_task_specific_t(sctk_task_specific_t *tmp) {
  /* First empty the whole sctk_task_specific_t */
  memset(tmp, 0, sizeof(sctk_task_specific_t));

  tmp->thread_data = sctk_thread_data_get();
  tmp->progress_list = NULL;

  /* Set task id */
  tmp->task_id = sctk_get_task_rank();

  __MPC_init_progress(tmp);

  sctk_barrier((sctk_communicator_t) MPC_COMM_WORLD );

  /* Initialize Data-type array */
  Datatype_Array_init(&tmp->datatype_array);

  /* Set initial per communicator data */
  mpc_per_communicator_t *per_comm_tmp;

  /* COMM_WORLD */
  per_comm_tmp = sctk_thread_createspecific_mpc_per_comm();
  sctk_thread_addspecific_mpc_per_comm(tmp, per_comm_tmp, MPC_COMM_WORLD);

  /* COMM_SELF */
  per_comm_tmp = sctk_thread_createspecific_mpc_per_comm();
  sctk_thread_addspecific_mpc_per_comm(tmp, per_comm_tmp, MPC_COMM_SELF);

  /* Propagate initialization to the thread context */
  MPC_Init_thread_specific();

  /* Set MPI status informations */
  tmp->init_done = 0;
  tmp->finalize_done = 0;
  tmp->thread_level = -1;

  {
    sctk_communicator_t tmp_comm;
    tmp->my_ptp_internal = sctk_malloc(SCTK_PARALLEL_COMM_QUEUES_NUMBER *
                                       sizeof(struct sctk_internal_ptp_s *));
    for (tmp_comm = 0; tmp_comm < SCTK_PARALLEL_COMM_QUEUES_NUMBER;
         tmp_comm++) {
      tmp->my_ptp_internal[tmp_comm] =
          sctk_get_internal_ptp(tmp->task_id, tmp_comm);
    }
  }

  /* Create the MPI_Info factory */
  MPC_Info_factory_init(&tmp->info_fact);

  /* Create the context class handling structure */
  GRequest_context_init(&tmp->grequest_context);

  /* Clear exit handlers */
  tmp->exit_handlers = NULL;

#ifdef SCTK_PROCESS_MODE
  ___the_process_specific = tmp;
#endif
}

/** \brief Relases a structure of type \ref sctk_task_specific_t
 */
static inline void __MPC_release_task_specific_t(sctk_task_specific_t *tmp) {
  /* Release the MPI_Info factory */
  MPC_Info_factory_release(&tmp->info_fact);

  /* Release the context class handling structure */
  GRequest_context_release(&tmp->grequest_context);
}

/** \brief Creation point for MPI task context in an \ref sctk_task_specific_t
 *
 * Called from \ref sctk_user_main this function allocates and initialises
 * an sctk_task_specific_t. It also takes care of storing it in the host
 * thread context.
 */
static void __MPC_setup_task_specific() {
  /* Retrieve the task ctx pointer */
  sctk_task_specific_t *tmp;
  tmp = (sctk_task_specific_t *)sctk_thread_getspecific_mpc(sctk_task_specific);

  /* Make sure that it is not already present */
  sctk_assert(tmp == NULL);

  /* If not allocate a new sctk_task_specific_t */
  tmp = (sctk_task_specific_t *)sctk_malloc(sizeof(sctk_task_specific_t));

  /* And initalize it */
  __MPC_init_task_specific_t(tmp);

  /* Set the sctk_task_specific key in thread CTX */
  sctk_thread_setspecific_mpc(sctk_task_specific, tmp);

  /* Register the task specific in the disguisemement array */
  __MPC_init_disguise(tmp);

  /* Initialize commond data-types */
  sctk_datatype_init();

  /* Initialize composed datatypes */
  init_composed_common_types();
}

/** \brief Define a function to be called when a task leaves
 *  \arg function Function to be callsed when task exits
 *  \return 1 on error 0 otherwise
 */
int __MPC_atexit_task_specific(void (*function)(void)) {
  /* Retrieve the task ctx pointer */
  sctk_task_specific_t *tmp;
  tmp = (sctk_task_specific_t *)sctk_thread_getspecific_mpc(sctk_task_specific);

  if (!tmp)
    return 1;

  struct sctk_task_specific_atexit_s *new_exit =
      sctk_malloc(sizeof(struct sctk_task_specific_atexit_s));

  if (!new_exit) {
    return 1;
  }

  sctk_info("Registering task-level atexit handler (%p)", function);

  new_exit->func = function;
  new_exit->next = tmp->exit_handlers;

  /* Register the entry */
  tmp->exit_handlers = new_exit;

  return 0;
}

/** \brief Call atexit handlers for the task (mimicking the process behavior)
 */
void __MPC_atexit_task_specific_trigger() {
  /* Retrieve the task ctx pointer */
  sctk_task_specific_t *tmp;
  tmp = (sctk_task_specific_t *)sctk_thread_getspecific_mpc(sctk_task_specific);

  if (!tmp)
    return;

  struct sctk_task_specific_atexit_s *current = tmp->exit_handlers;
  struct sctk_task_specific_atexit_s *to_free;

  while (current) {
    to_free = current;

    if (current->func) {
      sctk_info("Calling task-level atexit handler (%p)", current->func);
      (current->func)();
    }

    current = current->next;

    sctk_free(to_free);
  }
}

/** \brief Releases and frees task ctx
 *  Also called from  \ref sctk_user_main this function releases
 *  MPI task ctx and remove them from host thread keys
 */
static void __MPC_delete_task_specific() {
  /* Retrieve the ctx pointer */
  sctk_task_specific_t *tmp;
  tmp = (sctk_task_specific_t *)sctk_thread_getspecific_mpc(sctk_task_specific);

  /* Clear progress */
  __MPC_release_progress( tmp );

  /* Free composed datatypes */
  release_composed_common_types();

  /* Free the type array */
  Datatype_Array_release(&tmp->datatype_array);

  /* Call atexit handlers */
  __MPC_atexit_task_specific_trigger();

  /* Remove the ctx reference in the host thread */
  sctk_thread_setspecific_mpc(sctk_task_specific, NULL);

  /* Release the task ctx */
  __MPC_release_task_specific_t(tmp);

  /* Free the task ctx */
  sctk_free(tmp);
}

/** \brief Set current thread task specific context
 *  \param tmp New value to be set
 */
void __MPC_reinit_task_specific(struct sctk_task_specific_s *tmp) {
  sctk_thread_setspecific_mpc(sctk_task_specific, tmp);
}

/** \brief Retrieves current thread task specific context
 */

struct sctk_task_specific_s *__MPC_get_task_specific() {
#ifdef SCTK_PROCESS_MODE
  return ___the_process_specific;
#endif

  struct sctk_task_specific_s *ret = NULL;
  int maybe_disguised = __MPC_Maybe_disguised();

  if( maybe_disguised )
  {
    sctk_thread_data_t * th = __sctk_thread_data_get(1);
    if( th->ctx_disguisement )
    {
        return th->ctx_disguisement;
    }
 }

  static __thread int last_rank = -2;
  static __thread struct sctk_task_specific_s *last_specific = NULL;

   if ( last_rank == sctk_get_task_rank() ) {
       if( last_specific )
        return last_specific;
   }

  
    ret = (struct sctk_task_specific_s *)sctk_thread_getspecific(
            sctk_task_specific);

  last_specific = ret;
  last_rank = sctk_get_task_rank();
  
  return ret;
}

/** \brief Retrieves a pointer to a contiguous datatype from its datatype ID
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be retrieved
 *
 */
sctk_contiguous_datatype_t *
sctk_task_specific_get_contiguous_datatype(sctk_task_specific_t *task_specific,
                                           MPC_Datatype datatype) {
  sctk_assert(task_specific != NULL);
  /* Return the pointed sctk_contiguous_datatype_t */
  return Datatype_Array_get_contiguous_datatype(&task_specific->datatype_array,
                                                datatype);
}

/** \brief Retrieves a pointer to a contiguous datatype from its datatype ID
 *
 *  \param datatype datatype ID to be retrieved
 *
 */
sctk_contiguous_datatype_t *
sctk_get_contiguous_datatype(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  sctk_assert(task_specific != NULL);
  /* Return the pointed sctk_contiguous_datatype_t */
  return Datatype_Array_get_contiguous_datatype(&task_specific->datatype_array,
                                                datatype);
}

/** \brief Retrieves a pointer to a derived datatype from its datatype ID
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be retrieved
 *
 */
sctk_derived_datatype_t *
sctk_task_specific_get_derived_datatype(sctk_task_specific_t *task_specific,
                                        MPC_Datatype datatype) {
  sctk_assert(task_specific != NULL);
  return Datatype_Array_get_derived_datatype(&task_specific->datatype_array,
                                             datatype);
}

/** \brief Retrieves a pointer to a derived datatype from its datatype ID
 *
 *  \param datatype datatype ID to be retrieved
 *
 */
sctk_derived_datatype_t *sctk_get_derived_datatype(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  sctk_assert(task_specific != NULL);
  return Datatype_Array_get_derived_datatype(&task_specific->datatype_array,
                                             datatype);
}
/** \brief Removed a derived datatype from the datatype array
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be removed from datatype array
 */
void sctk_task_specific_set_derived_datatype(
    sctk_task_specific_t *task_specific, MPC_Datatype datatype,
    sctk_derived_datatype_t *value) {
  sctk_assert(task_specific != NULL);
  Datatype_Array_set_derived_datatype(&task_specific->datatype_array, datatype,
                                      value);
}

/************************************************************************/
/* Error Reporting                                                      */
/************************************************************************/

static inline int __MPC_ERROR_REPORT__(MPC_Comm comm, int error, char *message,
                                       char *file, int line) {
  MPC_Comm comm_id;
  int error_id;

  MPC_Errhandler errh = (MPC_Errhandler)sctk_handle_get_errhandler(
      (sctk_handle)comm, SCTK_HANDLE_COMM);

  MPC_Handler_function *func = sctk_errhandler_resolve(errh);
  error_id = error;
  (func)(&comm_id, &error_id, message, file, line);

  return error;
}

#define MPC_ERROR_REPORT(comm, error, message)                                 \
  return __MPC_ERROR_REPORT__(comm, error, message, __FILE__, __LINE__)

#define MPC_ERROR_SUCESS() return MPC_SUCCESS;

#undef MPC_CREATE_INTERN_FUNC
#define MPC_CREATE_INTERN_FUNC(name)                                           \
  const MPC_Op MPC_##name = {MPC_##name##_func, NULL}

MPC_CREATE_INTERN_FUNC(SUM);
MPC_CREATE_INTERN_FUNC(MAX);
MPC_CREATE_INTERN_FUNC(MIN);
MPC_CREATE_INTERN_FUNC(PROD);
MPC_CREATE_INTERN_FUNC(LAND);
MPC_CREATE_INTERN_FUNC(BAND);
MPC_CREATE_INTERN_FUNC(LOR);
MPC_CREATE_INTERN_FUNC(BOR);
MPC_CREATE_INTERN_FUNC(LXOR);
MPC_CREATE_INTERN_FUNC(BXOR);
MPC_CREATE_INTERN_FUNC(MINLOC);
MPC_CREATE_INTERN_FUNC(MAXLOC);

#define mpc_check_type(datatype, comm)                                         \
  if (sctk_datatype_is_derived(datatype) &&                                    \
      !sctk_datatype_is_struct_datatype(datatype))                             \
    MPC_ERROR_REPORT(comm, MPC_ERR_TYPE, "");

TODO("To optimize")
/* #define mpc_check_comm(com,comm)		\ */
/*   if(sctk_thread_getspecific_mpc_per_comm(__MPC_get_task_specific (),com) ==
 * NULL)	\ */
/*     MPC_ERROR_REPORT(comm,MPC_ERR_COMM,"") */
#define mpc_check_comm(com, comm)                                              \
  if (com < 0)                                                                 \
  MPC_ERROR_REPORT(comm, MPC_ERR_COMM, "")

#define mpc_check_buf(buf, comm)                                               \
  if ((buf == NULL) && (buf != MPC_BOTTOM))                                    \
  MPC_ERROR_REPORT(comm, MPC_ERR_BUFFER, "")

#define mpc_check_buf_null(buf, comm)                                          \
  if ((buf == NULL))                                                           \
  MPC_ERROR_REPORT(comm, MPC_ERR_BUFFER, "")

#define mpc_check_count(count, comm)                                           \
  if (count < 0)                                                               \
  MPC_ERROR_REPORT(comm, MPC_ERR_COUNT, "")

static inline int __mpc_check_task__(int task, int max_rank) {
  return (((task < 0) && (task != -4) && (task != -2)) || (task >= max_rank)) &&
         (task != MPC_ANY_SOURCE);
}

#define mpc_check_task(task, comm, max_rank)                                   \
  if (__mpc_check_task__(task, max_rank))                                      \
  MPC_ERROR_REPORT(comm, MPC_ERR_RANK, "")

static inline int __mpc_check_task_msg__(int task, int max_rank) {
  int res;

  res = (((task < 0) || (task >= max_rank)) && (task != MPC_ANY_SOURCE));
  return res;
}

#define mpc_check_task_msg_size(task, comm, msg, max_rank)                     \
  if (((task < 0) || (task >= max_rank)) && (task != MPC_ANY_SOURCE))          \
  MPC_ERROR_REPORT(comm, MPC_ERR_RANK, msg)

#define mpc_check_task_msg(task, comm, msg, max_rank)                          \
  if (__mpc_check_task_msg__(task, max_rank))                                  \
  MPC_ERROR_REPORT(comm, MPC_ERR_RANK, msg)

#define mpc_check_tag(tag, comm)                                               \
  if (!(tag > MPC_LAST_TAG))                                                   \
  MPC_ERROR_REPORT(comm, MPC_ERR_TAG, "")

#define mpc_check_msg_inter(src, dest, tag, comm, comm_size, comm_remote_size) \
  mpc_check_task_msg(src, comm, " source", comm_size);                         \
  mpc_check_task_msg(dest, comm, " destination", comm_remote_size);            \
  mpc_check_tag(tag, comm)

#define mpc_check_msg(src, dest, tag, comm, comm_size)                         \
  mpc_check_task_msg(src, comm, " source", comm_size);                         \
  mpc_check_task_msg(dest, comm, " destination", comm_size);                   \
  mpc_check_tag(tag, comm)

#define mpc_check_msg_size(src, dest, tag, comm, s)                            \
  mpc_check_task_msg_size(src, comm, " source", s);                            \
  mpc_check_task_msg_size(dest, comm, " destination", s);                      \
  mpc_check_tag(tag, comm)

#define mpc_check_msg_size_inter(src, dest, tag, comm, s, rs)                  \
  mpc_check_task_msg_size(src, comm, " source", s);                            \
  mpc_check_task_msg_size(dest, comm, " destination", rs);                     \
  mpc_check_tag(tag, comm)

sctk_thread_key_t sctk_get_check_point_key() { return sctk_check_point_key; }

/* int */
/* PMPC_Get_op (void **op) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *op = task_specific->op; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_op (void *op) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->op = op; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_comm_type (void **comm_type) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *comm_type = task_specific->comm_type; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_comm_type (void *comm_type) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->comm_type = comm_type; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_keys (void **keys) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *keys = task_specific->keys; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_keys (void *keys) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->keys = keys; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_requests (void **requests) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *requests = task_specific->requests; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_requests (void *requests) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->requests = requests; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_groups (void **groups) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *groups = task_specific->groups; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_groups (void *groups) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->groups = groups; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_errors (void **errors) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *errors = task_specific->errors; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_errors (void *errors) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->errors = errors; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Get_buffers (void **buffers) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   *buffers = task_specific->buffers; */
/*   MPC_ERROR_SUCESS (); */
/* } */

/* int */
/* PMPC_Set_buffers (void *buffers) */
/* { */
/*   sctk_task_specific_t *task_specific; */
/*   task_specific = __MPC_get_task_specific (); */
/*   task_specific->buffers = buffers; */
/*   MPC_ERROR_SUCESS (); */
/* } */

int PMPC_Get_multithreading(char *name, int size) {
  memcpy(name, sctk_multithreading_mode, size - 2);
  name[size - 1] = '\0';
  MPC_ERROR_SUCESS();
}

int PMPC_Get_networking(char *name, int size) {
  memcpy(name, sctk_network_mode, size - 2);
  name[size - 1] = '\0';
  MPC_ERROR_SUCESS();
}

static inline int *__MPC_Comm_task_list(MPC_Comm comm, int size) {
  int i;
  int *task_list = sctk_malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    task_list[i] = sctk_get_comm_world_rank(comm, i);
  }
  return task_list;
}

static inline int *__MPC_Comm_task_list_remote(MPC_Comm comm, int size) {
  int i;
  int *task_list_remote = sctk_malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    task_list_remote[i] = sctk_get_remote_comm_world_rank(comm, i);
  }
  return task_list_remote;
}

static inline int __MPC_Comm_rank(MPC_Comm comm, int *rank,
                                  sctk_task_specific_t *task_specific) {
  *rank = sctk_get_rank(comm, task_specific->task_id);
  return 0;
}

static inline int __MPC_Comm_size(MPC_Comm comm, int *size) {
  *size = sctk_get_nb_task_total(comm);
  if (*size == -1)
    return MPC_ERR_COMM;

  return 0;
}

static inline int __MPC_Comm_remote_size(MPC_Comm comm, int *size) {
  *size = sctk_get_nb_task_remote(comm);
  if (*size == -1)
    return MPC_ERR_COMM;
  return 0;
}

static inline void __MPC_Comm_rank_size(MPC_Comm comm, int *rank, int *size,
                                        sctk_task_specific_t *task_specific) {
  sctk_get_rank_size_total(comm, rank, size, task_specific->task_id);
}

static void MPC_Set_buffering(int val) { mpc_disable_buffering = !(val); }

void MPC_Hard_Check() {
  int rank;
  int size;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  __MPC_Comm_rank_size(MPC_COMM_WORLD, &rank, &size, task_specific);
  if (rank == 0) {
    sctk_warning("Enable MPC_HARD_CHECKING");
  }
  MPC_Set_buffering(0);
}

#if defined(MPC_LOG_DEBUG)
static inline void mpc_log_debug(MPC_Comm comm, const char *fmt, ...) {
  if (sctk_runtime_config_get()->modules.message_passing.log_debug) {
    va_list ap;
    char buff[4096];
    int rank;
    FILE *stream;
    sctk_task_specific_t *task_specific;
    task_specific = __MPC_get_task_specific();

    stream = stderr;

    __MPC_Comm_rank(comm, &rank, task_specific);
    sprintf(buff, "Task %4d/%4d: %s\n", rank, (int)comm, fmt);

    va_start(ap, fmt);
    vfprintf(stream, buff, ap);
    va_end(ap);
  }
}

#endif

int __MPC_Barrier(MPC_Comm comm) {
  int size;
  PMPC_Comm_size(comm, &size);
  mpc_check_comm(comm, comm);

  if (sctk_is_inter_comm(comm)) {
    int root = 0, buf = 0, rank;
    sctk_task_specific_t *task_specific;

    task_specific = __MPC_get_task_specific();
    __MPC_Comm_rank(comm, &rank, task_specific);

	/* Sync Local */
    if (size > 1)
      __MPC_Barrier(sctk_get_local_comm_id(comm));

	/* Sync A-B */
	if( rank == 0 )
	{
		MPC_Sendrecv( &buf, 1, MPC_INT, 0, MPC_BARRIER_TAG, &buf, 1, MPC_INT, 0, MPC_BARRIER_TAG, comm, MPC_STATUS_IGNORE);	
	}

	/* Sync Local */
	if (size > 1)
      __MPC_Barrier(sctk_get_local_comm_id(comm));

  } else {

    if (size > 1)
      sctk_barrier((sctk_communicator_t)comm);
  }
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

int ___grequest_disguise_poll( void * arg )
{
    int ret = 0;
    //sctk_error("POLL %p", arg);

    MPC_Request * req = (MPC_Request *)arg;

    if( !req->poll_fn )
    {
        ret = 0;
        goto POLL_DONE_G;
    }

    if( req->completion_flag == SCTK_MESSAGE_DONE )
    {
        ret = 1;
        goto POLL_DONE_G;
    }

    int my_rank = sctk_get_task_rank();
    
    int disguised = 0;
    if( my_rank != req->grequest_rank )
    {
        disguised = 1;
        if( MPCX_Disguise( MPC_COMM_WORLD, req->grequest_rank ) != MPC_SUCCESS )
        {
            ret = 0;
            goto POLL_DONE_G;
        }
    }

    MPC_Status tmp_status;
    (req->poll_fn)(req->extra_state, &tmp_status);   

    ret = (req->completion_flag == SCTK_MESSAGE_DONE);


POLL_DONE_G:
    if( disguised )
    {
       MPCX_Undisguise();
    }

    return ret;
}


void sctk_generic_request_poll( MPC_Request * req )
{
    if(req->completion_flag == SCTK_MESSAGE_DONE)
        return;
    
    sctk_progress_work_unit_poll( req->progress_unit );
}



/** \brief This function handles request starting for every generalized request
 * types
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime (CAN BE NULL)
 *  \param wait_fn Wait function called when the runtime waits several requests
 * of the same type (CAN BE NULL)
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int PMPCX_Grequest_start_generic(MPC_Grequest_query_function *query_fn,
                                 MPC_Grequest_free_function *free_fn,
                                 MPC_Grequest_cancel_function *cancel_fn,
                                 MPCX_Grequest_poll_fn *poll_fn,
                                 MPCX_Grequest_wait_fn *wait_fn,
                                 void *extra_state, MPC_Request *request) {
  if (request == NULL)
    MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG,
                     "Bad request passed to MPC_Grequest_start");

  /* Initialized as a NULL request */
  memcpy(request, &mpc_request_null, sizeof(MPC_Request));

  /* Change type */
  request->request_type = REQUEST_GENERALIZED;
  /* Set not null as we want to be waited */
  request->is_null = 0;

  request->grequest_rank = sctk_get_task_rank();

  /* Fill in generalized request CTX */
  request->pointer_to_source_request = (void *)request;
  request->query_fn = query_fn;
  request->free_fn = free_fn;
  request->cancel_fn = cancel_fn;
  request->poll_fn = poll_fn;
  request->wait_fn = wait_fn;
  request->extra_state = extra_state;

  /* We set the request as pending */
  request->completion_flag = SCTK_MESSAGE_PENDING;

  /* We now push the request inside the progress list */
  struct sctk_task_specific_s *ctx = __MPC_get_task_specific();
  request->progress_unit = NULL;
  request->progress_unit = (void*)sctk_progress_list_add(ctx->progress_list, ___grequest_disguise_poll, (void *)request );

  MPC_ERROR_SUCESS()
}

/** \brief Starts a generalized request with a polling function
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int PMPCX_Grequest_start(MPC_Grequest_query_function *query_fn,
                         MPC_Grequest_free_function *free_fn,
                         MPC_Grequest_cancel_function *cancel_fn,
                         MPCX_Grequest_poll_fn *poll_fn, void *extra_state,
                         MPC_Request *request) {
  return PMPCX_Grequest_start_generic(query_fn, free_fn, cancel_fn, poll_fn,
                                      NULL, extra_state, request);

  //sctk_error("START %p", request);
}
/************************************************************************/
/* Extended Generalized Requests Request Classes                        */
/************************************************************************/

/** \brief This creates a request class which can be referred to later on
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime (CAN BE NULL)
 *  \param wait_fn Wait function called when the runtime waits several requests
 * of the same type (CAN BE NULL)
 *  \param new_class The identifier of the class we are creating (output)
 */
int PMPCX_GRequest_class_create(MPC_Grequest_query_function *query_fn,
                                MPC_Grequest_cancel_function *cancel_fn,
                                MPC_Grequest_free_function *free_fn,
                                MPCX_Grequest_poll_fn *poll_fn,
                                MPCX_Grequest_wait_fn *wait_fn,
                                MPCX_Request_class *new_class) {
  /* Retrieve task context */
  struct sctk_task_specific_s *task_specific = __MPC_get_task_specific();
  assume(task_specific != NULL);
  struct GRequest_context *class_ctx = &task_specific->grequest_context;

  /* Register the class */
  return GRequest_context_add_class(class_ctx, query_fn, cancel_fn, free_fn,
                                    poll_fn, wait_fn, new_class);
}

/** \brief Create a request linked to an \ref MPCX_Request_class type
 *
 *  \param target_class Identifier of the class of work we launch as created by
 * \ref PMPIX_GRequest_class_create
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int PMPCX_Grequest_class_allocate(MPCX_Request_class target_class,
                                  void *extra_state, MPC_Request *request) {
  /* Retrieve task context */
  struct sctk_task_specific_s *task_specific = __MPC_get_task_specific();
  assume(task_specific != NULL);
  struct GRequest_context *class_ctx = &task_specific->grequest_context;

  /* Retrieve task description */
  MPCX_GRequest_class_t *class_desc =
      GRequest_context_get_class(class_ctx, target_class);

  /* Not found */
  if (!class_desc)
    MPC_ERROR_REPORT(
        MPC_COMM_SELF, MPC_ERR_ARG,
        "Bad MPCX_Request_class passed to PMPIX_Grequest_class_allocate");

  return PMPCX_Grequest_start_generic(
      class_desc->query_fn, class_desc->free_fn, class_desc->cancel_fn,
      class_desc->poll_fn, class_desc->wait_fn, extra_state, request);
}

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/

/** \brief This function starts a generic request
*
* \param query_fn Query function called to fill the status object
* \param free_fn Free function which frees extra arg
* \param cancel_fn Function called when the request is canceled
* \param extra_state Extra context passed to every handlers
* \param request New request to be created
*
* \warning Generalized Requests are not progressed by MPI the user are
*          in charge of providing the progress mechanism.
*
* Once the operation completes, the user has to call \ref PMPC_Grequest_complete
*
*/
int PMPC_Grequest_start(MPC_Grequest_query_function *query_fn,
                        MPC_Grequest_free_function *free_fn,
                        MPC_Grequest_cancel_function *cancel_fn,
                        void *extra_state, MPC_Request *request) {
  return PMPCX_Grequest_start_generic(query_fn, free_fn, cancel_fn, NULL, NULL,
                                      extra_state, request);
}

/** \brief Flag a Generalized Request as finished
* \param request Request we want to finish
*/
int PMPC_Grequest_complete(MPC_Request request) {

    MPC_Request * src_req = (MPC_Request *)request.pointer_to_source_request; 

    struct sctk_progress_work_unit * pwu = ((struct sctk_progress_work_unit *)src_req->progress_unit);

    pwu->done = 1;
    
    /* We have to do this as request complete takes
   * a copy of the request ... but we want
   * to modify the original request which is being polled ... */
  src_req->completion_flag =
      SCTK_MESSAGE_DONE;



  //sctk_error("COMP %p",  request.pointer_to_source_request);
  MPC_ERROR_SUCESS()
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/* Utilities functions for this section are located in mpc_datatypes.[ch] */

/** \brief This fuction releases a datatype
 *
 *  This function call the right release function relatively to the
 *  datatype kind (contiguous or derived) and then set the freed
 *  datatype to MPC_DATATYPE_NULL
 *
 */
int PMPC_Type_free(MPC_Datatype *datatype_p) {
  /* Dereference the datatype pointer for convenience */
  MPC_Datatype datatype = *datatype_p;

  SCTK_PROFIL_START(MPC_Type_free);

  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  if (datatype == MPC_DATATYPE_NULL || datatype == MPC_LB ||
      datatype == MPC_UB) {
    return MPC_ERR_TYPE;
  }

  /* Is the datatype NULL or PACKED ? */
  if (datatype == MPC_PACKED) {
    /* ERROR */
    SCTK_PROFIL_END(MPC_Type_free);
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
                     "You tried to free an MPI_PACKED datatype");
  }

  /* Choose what to do in function of the datatype kind */
  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    /* You are not supposed to free predefined datatypes */
    SCTK_PROFIL_END(MPC_Type_free);
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
                     "You are not allowed to free a COMMON datatype");
    break;

  case MPC_DATATYPES_CONTIGUOUS:
    /* Free a contiguous datatype */

    /* Lock the contiguous type array */
    sctk_datatype_lock(task_specific);
    /* Retrieve a pointer to the type to be freed */
    sctk_contiguous_datatype_t *continuous_type_target =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    /* Unlock type array */
    sctk_datatype_unlock(task_specific);
    /* Free it */
    sctk_contiguous_datatype_release(continuous_type_target);

    break;

  case MPC_DATATYPES_DERIVED:
    /* Free a derived datatype */

    /* Lock the derived type array */
    sctk_datatype_lock(task_specific);
    /* Retrieve a pointer to the type to be freed */
    sctk_derived_datatype_t *derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    /* Unlock the derived type array */
    sctk_datatype_unlock(task_specific);

    /* Check if it is really allocated */
    if (!derived_type_target) {
      /* ERROR */
      MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                       "Tried to release an uninitialized datatype");
    }

    sctk_nodebug("Free derived [%d] contructor %d", datatype,
                 derived_type_target->context.combiner);

    /* Free it  (not that the container is also freed in this function */
    if (sctk_derived_datatype_release(derived_type_target)) {
      /* Set the pointer to the freed datatype to NULL in the derived datatype
       * array */
      sctk_task_specific_set_derived_datatype(task_specific, datatype, NULL);
    }

    break;

  case MPC_DATATYPES_UNKNOWN:
    /* If we are here the type provided must have been erroneous */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                     "An unknown datatype was provided");
    break;
  }

  /* Set the source datatype as freed */
  *datatype_p = MPC_DATATYPE_NULL;

  SCTK_PROFIL_END(MPC_Type_free);
  MPC_ERROR_SUCESS();
}

/** \brief Set a name to an MPC_Datatype
 *  \param datatype Datatype to be named
 *  \param name Name to be set
 */
int PMPC_Type_set_name(MPC_Datatype datatype, char *name) {
  if (sctk_datype_set_name(datatype, name)) {
    MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_INTERN, "Could not set name");
  }

  MPC_ERROR_SUCESS();
}

/** \brief Duplicate a  datatype
 *  \param old_type Type to be duplicated
 *  \param new_type Copy of old_type
 */
int PMPC_Type_dup(MPC_Datatype old_type, MPC_Datatype *new_type) {
  sctk_derived_datatype_t *derived_type_target;
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Set type context */
  struct Datatype_External_context dtctx;
  sctk_datatype_external_context_clear(&dtctx);
  dtctx.combiner = MPC_COMBINER_DUP;
  dtctx.oldtype = old_type;

  switch (sctk_datatype_kind(old_type)) {
  case MPC_DATATYPES_COMMON:
  /* We dup common datatypes in contiguous ones in order
   * to have a context where to put the DUP combiner */
  case MPC_DATATYPES_CONTIGUOUS:
    /* Create a type consisting in one entry of the contiguous block
       here as the combiner is set to be a dup it wont reuse the previous
       contiguous that we are dupping from unless it has already been duped */
    __INTERNAL__PMPC_Type_hcontiguous(new_type, 1, &old_type, &dtctx);
    break;
  case MPC_DATATYPES_DERIVED:
    /* Here we just build a copy of the derived data-type */
    derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, old_type);
    PMPC_Derived_datatype(
        new_type, derived_type_target->begins, derived_type_target->ends,
        derived_type_target->datatypes, derived_type_target->count,
        derived_type_target->lb, derived_type_target->is_lb,
        derived_type_target->ub, derived_type_target->is_ub, &dtctx);
    break;
  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG, "Bad data-type");
    break;
  }

  MPC_ERROR_SUCESS();
}

/** \brief Set a name to an MPC_Datatype
 *  \param datatype Datatype to get the name of
 *  \param name Datatype name (OUT)
 *  \param resultlen Maximum length of the target buffer (OUT)
 */
int PMPC_Type_get_name(MPC_Datatype datatype, char *name, int *resultlen) {
  char *retname = sctk_datype_get_name(datatype);

  if (datatype == MPC_UB) {
    retname = "MPI_UB";
  } else if (datatype == MPC_LB) {
    retname = "MPI_LB";
  }

  if (!retname) {
    /* Return an empty string */
    name[0] = '\0';
    *resultlen = 0;
  } else {
    sprintf(name, "%s", retname);
    *resultlen = strlen(name);
  }

  MPC_ERROR_SUCESS();
}

/** \brief This call is used to fill the envelope of an MPI type
 *
 *  \param ctx Source context
 *  \param num_integers Number of input integers [OUT]
 *  \param num_addresses Number of input addresses [OUT]
 *  \param num_datatypes Number of input datatypes [OUT]
 *  \param combiner Combiner used to build the datatype [OUT]
 *
 *
 */
int PMPC_Type_get_envelope(MPC_Datatype datatype, int *num_integers,
                           int *num_addresses, int *num_datatypes,
                           int *combiner) {
  *num_integers = 0;
  *num_addresses = 0;
  *num_datatypes = 0;

  /* Handle the common data-type case */
  if (sctk_datatype_is_common(datatype) ||
      sctk_datatype_is_boundary(datatype)) {
    *combiner = MPC_COMBINER_NAMED;
    *num_integers = 0;
    *num_addresses = 0;
    *num_datatypes = 0;
    MPC_ERROR_SUCESS();
  }

  struct Datatype_context *dctx = sctk_retrieve_datatype_ctx(datatype);

  if (!dctx) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "This datatype is unknown");
  }

  sctk_datatype_fill_envelope(dctx, num_integers, num_addresses, num_datatypes,
                              combiner);

  MPC_ERROR_SUCESS();
}

int PMPC_Type_get_contents(MPC_Datatype datatype, int max_integers,
                           int max_addresses, int max_datatypes,
                           int array_of_integers[],
                           MPC_Aint array_of_addresses[],
                           MPC_Datatype array_of_datatypes[]) {
  /* First make sure we were not called on a common type */
  if (sctk_datatype_is_common(datatype)) {
    MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG,
                     "Cannot call MPI_Type_get_contents on a defaut datatype");
  }

  /* Retrieve the context */
  struct Datatype_context *dctx = sctk_retrieve_datatype_ctx(datatype);

  assume(dctx != NULL);

  /* Then make sure the user actually called MPI_Get_envelope
  * with the same datatype or that we have enough room */
  int n_int = 0;
  int n_addr = 0;
  int n_type = 0;
  int dummy_combiner;

  sctk_datatype_fill_envelope(dctx, &n_int, &n_addr, &n_type, &dummy_combiner);

  if ((max_integers < n_int) || (max_addresses < n_addr) ||
      (max_datatypes < n_type)) {
    /* Not enough room */
    MPC_ERROR_REPORT(
        MPC_COMM_SELF, MPC_ERR_ARG,
        "Cannot call MPI_Type_get_contents as it would overflow target arrays");
  }

  /* Now we just copy back the content from the context */
  if (array_of_integers)
    memcpy(array_of_integers, dctx->array_of_integers, n_int * sizeof(int));

  if (array_of_addresses)
    memcpy(array_of_addresses, dctx->array_of_addresses,
           n_addr * sizeof(MPC_Aint));

  /* Flag the datatypes as duplicated */
  int i;
  for (i = 0; i < n_type; i++)
    PMPC_Type_use(dctx->array_of_types[i]);

  if (array_of_datatypes)
    memcpy(array_of_datatypes, dctx->array_of_types,
           n_type * sizeof(MPC_Datatype));

  MPC_ERROR_SUCESS();
}

/** \brief Retrieves datatype size
 *
 *  Returns the right size field in function of datype kind
 *
 *  \param datatype datatype which size is requested
 *  \param task_specific a pointer to task CTX
 *
 *  \return the size of the datatype or aborts
 */
size_t __MPC_Get_datatype_size(MPC_Datatype datatype,
                               sctk_task_specific_t *task_specific) {
  /* Special cases */
  if (datatype == MPC_DATATYPE_NULL) {
    /* Here we return 0 for data-type null
     * in order to pass the struct-zero-count test */
    return 0;
  } else if (datatype == MPC_UB) {
    return 0;
  }else if (datatype == MPC_LB) {
    return 0;
  }else if (datatype == MPC_PACKED) {
    return 1;
  }

  size_t ret;
  sctk_contiguous_datatype_t *contiguous_type_target;
  sctk_derived_datatype_t *derived_type_target;

  /* Compute the size in function of the datatype */
  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    /* Common datatype sizes */

    return sctk_common_datatype_get_size(datatype);
    break;

  case MPC_DATATYPES_CONTIGUOUS:
    /* Contiguous datatype size */

    /* Get a pointer to the type of interest */
    contiguous_type_target =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    /* Extract its size field */
    ret = contiguous_type_target->size;

    /* Return */
    return ret;
    break;

  case MPC_DATATYPES_DERIVED:
    /* Derived datatype size */

    /* Get a pointer to the object of interest */
    derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);

    /* Is it allocated ? */
    if (!derived_type_target) {
      /* ERROR */
      sctk_fatal("Tried to retrieve an uninitialized datatype %d", datatype);
    }

    if (derived_type_target->is_a_padded_struct) {

      /* Here we return UB as the size (padded struct) */
      ret = derived_type_target->ub;

    } else {
      /* Extract the size field */
      ret = derived_type_target->size;
    }

    /* Return */
    return ret;
    break;

  case MPC_DATATYPES_UNKNOWN:
    /* No such datatype */
    /* ERROR */
    sctk_fatal("An unknown datatype was provided");
    break;
  }

  /* We shall never arrive here ! */
  sctk_fatal("This error shall never be reached");
}

int PMPC_Type_get_true_extent(MPC_Datatype datatype, MPC_Aint *true_lb,
                              MPC_Aint *true_extent) {
  sctk_derived_datatype_t *derived_type_target;
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  mpc_pack_absolute_indexes_t tmp_true_lb;
  mpc_pack_absolute_indexes_t tmp_true_ub;

  /* Special cases */
  if (datatype == MPC_DATATYPE_NULL) {
    /* Here we return 0 for data-type null
     * in order to pass the struct-zero-count test */
    *true_lb = 0;
    *true_extent = 0;
    MPC_ERROR_SUCESS();
  }

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
  case MPC_DATATYPES_CONTIGUOUS:
    tmp_true_lb = 0;
    tmp_true_ub = __MPC_Get_datatype_size(datatype, task_specific);
    break;
  case MPC_DATATYPES_DERIVED:
    derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    sctk_derived_datatype_true_extent(derived_type_target, &tmp_true_lb,
                                      &tmp_true_ub);
    break;
  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG, "Bad data-type");
    break;
  }

  *true_lb = tmp_true_lb;
  *true_extent = (tmp_true_ub - tmp_true_lb) + 1;

  MPC_ERROR_SUCESS();
}

/** \brief Checks if a datatype has already been released
 *  \param datatype target datatype
 *  \param flag 1 if the type is allocated [OUT]
 */
int PMPC_Type_is_allocated(MPC_Datatype datatype, int *flag) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  sctk_derived_datatype_t *derived_type_target;
  sctk_contiguous_datatype_t *contiguous_type_target;

  *flag = 0;

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    *flag = 1;
    break;
  case MPC_DATATYPES_CONTIGUOUS:
    contiguous_type_target =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    *flag = SCTK_CONTIGUOUS_DATATYPE_IS_IN_USE(contiguous_type_target);
    break;
  case MPC_DATATYPES_DERIVED:
    derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    *flag = (derived_type_target != NULL);
    break;
  case MPC_DATATYPES_UNKNOWN:
    *flag = 0;
    break;
  }

  MPC_ERROR_SUCESS();
}

/** \brief Set a Struct datatype as a padded one to return the extent instead of
 * the size
 *  \param datatype to be flagged as padded
 */
int PMPC_Type_flag_padded(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  sctk_derived_datatype_t *derived_type_target = NULL;

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
  case MPC_DATATYPES_CONTIGUOUS:
  case MPC_DATATYPES_UNKNOWN:
    sctk_fatal("Only Common datatypes can be flagged");
    break;
  case MPC_DATATYPES_DERIVED:
    derived_type_target =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);

    if (derived_type_target) {
      derived_type_target->is_a_padded_struct = 1;
    } else {
      return MPC_ERR_ARG;
    }

    break;
  }

  MPC_ERROR_SUCESS();
}

/** \brief Compute the size of a \ref MPC_Datatype
 *  \param datatype target datatype
 *  \param size where to write the size of datatype
 */
int PMPC_Type_size(MPC_Datatype datatype, size_t *size) {
  SCTK_PROFIL_START(MPC_Type_size);

  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  *size = __MPC_Get_datatype_size(datatype, task_specific);

  SCTK_PROFIL_END(MPC_Type_size);
  MPC_ERROR_SUCESS();
}

static inline int
sctk_datatype_check_contiguous(sctk_task_specific_t *task_specific,
                               struct Datatype_External_context *ctx,
                               MPC_Datatype *datatype) {
  int i;
  /* see if such a datatype is not already allocated */
  for (i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++) {
    /* For each contiguous type slot */
    sctk_contiguous_datatype_t *current_type =
        sctk_task_specific_get_contiguous_datatype(
            task_specific, MPC_TYPE_MAP_FROM_CONTIGUOUS(i));

    /* We are only interested in allocated types */
    if (SCTK_CONTIGUOUS_DATATYPE_IS_FREE(current_type))
      continue;

    /* If types match */
    if (Datatype_context_match(ctx, &current_type->context)) {
      /* Add a reference to this data-type and we are done */
      PMPC_Type_use(MPC_TYPE_MAP_FROM_CONTIGUOUS(i));

      *datatype = MPC_TYPE_MAP_FROM_CONTIGUOUS(i);

      return 1;
    }
  }

  return 0;
}

static inline int
sctk_datatype_check_derived(sctk_task_specific_t *task_specific,
                            struct Datatype_External_context *ctx,
                            MPC_Datatype *datatype) {
  int i;

  /* try to find a data-type with the same footprint in derived  */
  for (i = MPC_STRUCT_DATATYPE_COUNT; i < SCTK_USER_DATA_TYPES_MAX; i++) {
    /* For each datatype */
    sctk_derived_datatype_t *current_user_type =
        sctk_task_specific_get_derived_datatype(task_specific,
                                                MPC_TYPE_MAP_FROM_DERIVED(i));

    /* Is the slot NOT free ? */
    if (current_user_type != NULL) {

      /* If types match */
      if (Datatype_context_match(ctx, &current_user_type->context)) {
        /* Add a reference to this data-type and we are done */
        PMPC_Type_use(MPC_TYPE_MAP_FROM_DERIVED(i));

        *datatype = MPC_TYPE_MAP_FROM_DERIVED(i);

        return 1;
      }
    }
  }

  return 0;
}

/** \brief This function is the generic initializer for
 * sctk_contiguous_datatype_t
 *  Creates a contiguous datatypes of count data_in while checking for unicity
 *
 *  \param datatype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *  \param ctx Context of the new data-type in order to allow unicity check
 *
 */
int __INTERNAL__PMPC_Type_hcontiguous(MPC_Datatype *datatype, size_t count,
                                      MPC_Datatype *data_in,
                                      struct Datatype_External_context *ctx) {
  SCTK_PROFIL_START(MPC_Type_hcontiguous);

  /* Retrieve task specific context */
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  /* Set the datatype to NULL in case we do not find a slot */
  *datatype = MPC_DATATYPE_NULL;

  /* Lock contiguous types array */
  sctk_datatype_lock(task_specific);

  /* Retrieve size per element */
  size_t size;
  PMPC_Type_size(*data_in, &size);

  int i;

  if (sctk_datatype_check_contiguous(task_specific, ctx, datatype)) {
    sctk_datatype_unlock(task_specific);
    MPC_ERROR_SUCESS();
  }

  /* We did not find a previously defined type with the same footprint
   * Now lets try to find a free slot in the array */
  for (i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++) {
    /* For each contiguous type slot */
    sctk_contiguous_datatype_t *current_type =
        sctk_task_specific_get_contiguous_datatype(
            task_specific, MPC_TYPE_MAP_FROM_CONTIGUOUS(i));

    /* Are you free ? */
    if (SCTK_CONTIGUOUS_DATATYPE_IS_FREE(current_type)) {
      /* Yes !*/
      /* Here we create an id falling in the continuous datatype range */
      int new_id = MPC_TYPE_MAP_FROM_CONTIGUOUS(i);

      /* Set the news datatype to the continuous ID we have just allocated */
      *datatype = new_id;

      /* Initialize the datatype */
      sctk_contiguous_datatype_init(current_type, new_id, size, count,
                                    *data_in);

      /* Set context */
      MPC_Datatype_set_context(*datatype, ctx);

      /* Increment target datatype refcounter here we do it once as there is
       * only a single datatype */
      // PMPC_Type_use( *data_in );

      /* Unlock the array */
      sctk_datatype_unlock(task_specific);

      SCTK_PROFIL_END(MPC_Type_hcontiguous);

      /* We are done return */
      MPC_ERROR_SUCESS();
    }
  }

  /* If we are here we did not find any slot so we abort you might think of
   * increasing
   * SCTK_USER_DATA_TYPES_MAX if you app needs more than 265 datatypes =) */
  sctk_fatal("Not enough datatypes allowed : you requested to many contiguous "
             "types (forgot to free ?)");

  return -1;
}

/** \brief This function is the generic initializer for
 * sctk_contiguous_datatype_t
 *  Creates a contiguous datatypes of count data_in
 *
 *  \param datatype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *
 */
int PMPC_Type_hcontiguous(MPC_Datatype *datatype, size_t count,
                          MPC_Datatype *data_in) {
  /* HERE WE SET A DEFAULT CONTEXT */
  struct Datatype_External_context dtctx;
  sctk_datatype_external_context_clear(&dtctx);
  dtctx.combiner = MPC_COMBINER_CONTIGUOUS;
  dtctx.count = count;
  dtctx.oldtype = *data_in;

  return __INTERNAL__PMPC_Type_hcontiguous(datatype, count, data_in, &dtctx);
}

int PMPC_Type_commit(MPC_Datatype *datatype) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  sctk_derived_datatype_t *target_derived_type;

  if (*datatype == MPC_DATATYPE_NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                     "You are trying to commit a NULL data-type");
  }

  switch (sctk_datatype_kind(*datatype)) {
  case MPC_DATATYPES_COMMON:
  case MPC_DATATYPES_CONTIGUOUS:
    /* Nothing to do here */
    MPC_ERROR_SUCESS();
    break;

  case MPC_DATATYPES_DERIVED:

    /* Get a pointer to the type of interest */
    target_derived_type =
        sctk_task_specific_get_derived_datatype(task_specific, *datatype);

    /* OPTIMIZE */
    sctk_derived_datatype_optimize(target_derived_type);

    MPC_ERROR_SUCESS();
    break;

  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "This datatype is unknown");
    break;
  }

  MPC_ERROR_SUCESS();
}

int MPCX_Type_debug(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  sctk_derived_datatype_t *target_derived_type;
  sctk_contiguous_datatype_t *contiguous_type;

  if (datatype == MPC_DATATYPE_NULL) {
    sctk_error("=============ERROR==================");
    sctk_error("MPC_DATATYPE_NULL");
    sctk_error("====================================");
    MPC_ERROR_SUCESS();
  }

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    sctk_common_datatype_display(datatype);
    break;
  case MPC_DATATYPES_CONTIGUOUS:
    contiguous_type =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    sctk_contiguous_datatype_display(contiguous_type);
    break;

  case MPC_DATATYPES_DERIVED:
    target_derived_type =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    sctk_derived_datatype_display(target_derived_type);
    break;

  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "This datatype is unknown");
    break;
  }

  MPC_ERROR_SUCESS();
}

/** \brief This function increases the refcounter for a datatype
 *
 *  \param datatype Target datatype
 *
 *  \warning We take no lock here thus the datatype lock shall be held
 *  when manipulating such objects
 */
int PMPC_Type_use(MPC_Datatype datatype) {
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  sctk_contiguous_datatype_t *target_contiguous_type;
  sctk_derived_datatype_t *target_derived_type;

  if (sctk_datatype_is_boundary(datatype))
    MPC_ERROR_SUCESS();

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    /* Nothing to do here */
    MPC_ERROR_SUCESS();
    break;

  case MPC_DATATYPES_CONTIGUOUS:

    /* Get a pointer to the type of interest */
    target_contiguous_type =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    /* Increment the refcounter */
    target_contiguous_type->ref_count++;
    sctk_nodebug("Type %d Refcount %d", datatype,
                 target_contiguous_type->ref_count);

    break;

  case MPC_DATATYPES_DERIVED:

    /* Get a pointer to the type of interest */
    target_derived_type =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    /* Increment the refcounter */
    target_derived_type->ref_count++;
    sctk_nodebug("Type %d Refcount %d", datatype,
                 target_derived_type->ref_count);

    break;

  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "This datatype is unknown");
    break;
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Derived_datatype_on_slot(int id, mpc_pack_absolute_indexes_t *begins,
                                  mpc_pack_absolute_indexes_t *ends,
                                  MPC_Datatype *types, unsigned long count,
                                  mpc_pack_absolute_indexes_t lb, int is_lb,
                                  mpc_pack_absolute_indexes_t ub, int is_ub) {
  SCTK_PROFIL_START(MPC_Derived_datatype);

  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Here we allocate the new derived datatype */
  sctk_derived_datatype_t *new_type =
      (sctk_derived_datatype_t *)sctk_malloc(sizeof(sctk_derived_datatype_t));

  if (!new_type) {
    sctk_fatal("Failled to allocate a new derived type");
  }

  /* And we call the datatype initializer */
  sctk_derived_datatype_init(new_type, id, count, begins, ends, types, lb,
                             is_lb, ub, is_ub);

  /* Now we register the datatype pointer in the derived datatype array */
  sctk_task_specific_set_derived_datatype(task_specific, id, new_type);

  /* We unlock the derived datatype array */
  sctk_datatype_unlock(task_specific);

  SCTK_PROFIL_END(MPC_Derived_datatype);
  MPC_ERROR_SUCESS();
}

/** \brief Derived datatype constructor
 *
 * This function allocates a derived datatype slot and fills it
 * with a datatype according to the parameters.
 *
 * \param datatype Datatype to build
 * \param begins list of starting offsets in the datatype
 * \param ends list of end offsets in the datatype
 * \param count number of offsets to store
 * \param lb offset of type lower bound
 * \param is_lb tells if the type has a lowerbound
 * \param ub offset for type upper bound
 * \param is_b tells if the type has an upper bound
 *
 */
int PMPC_Derived_datatype(MPC_Datatype *datatype,
                          mpc_pack_absolute_indexes_t *begins,
                          mpc_pack_absolute_indexes_t *ends,
                          MPC_Datatype *types, unsigned long count,
                          mpc_pack_absolute_indexes_t lb, int is_lb,
                          mpc_pack_absolute_indexes_t ub, int is_ub,
                          struct Datatype_External_context *ectx) {
  SCTK_PROFIL_START(MPC_Derived_datatype);

  /* First the the target datatype to NULL if we do not manage to
   * allocate a new slot */
  *datatype = MPC_DATATYPE_NULL;

  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /*Lock derived datatype array */
  sctk_datatype_lock(task_specific);

  int i;

  /* Here we check against contiguous as in some case
   * a derived datatype can be stored directly as a
   * contiguous one */
  if (sctk_datatype_check_contiguous(task_specific, ectx, datatype)) {
    sctk_datatype_unlock(task_specific);
    MPC_ERROR_SUCESS();
  }

  /* We did not find it lets now look at contiguous */
  if (sctk_datatype_check_derived(task_specific, ectx, datatype)) {
    sctk_datatype_unlock(task_specific);
    MPC_ERROR_SUCESS();
  }

  /* Here we jump the first MPC_STRUCT_DATATYPE_COUNT items
   * as they are reserved for common datatypes which are actually
   * derived ones */
  for (i = MPC_STRUCT_DATATYPE_COUNT; i < SCTK_USER_DATA_TYPES_MAX; i++) {
    /* For each datatype */
    sctk_derived_datatype_t *current_user_type =
        sctk_task_specific_get_derived_datatype(task_specific,
                                                MPC_TYPE_MAP_FROM_DERIVED(i));

    /* Is the slot free ? */
    if (current_user_type == NULL) {
      /* Here we compute an ID falling in the derived datatype range
      * this range it the converted back to a local id using
      * MPC_TYPE_MAP_TO_DERIVED( datatype) */
      int new_id = MPC_TYPE_MAP_FROM_DERIVED(i);

      /* Set the new ID in the target datatype */
      *datatype = new_id;

      int ret = PMPC_Derived_datatype_on_slot(new_id, begins, ends, types,
                                              count, lb, is_lb, ub, is_ub);
      MPC_Datatype_set_context(new_id, ectx);

      return ret;
    }
  }

  /* If we are here we did not find any slot so we abort you might think of
   * increasing
   * SCTK_USER_DATA_TYPES_MAX if you app needs more than 265 datatypes =) */
  sctk_fatal("Not enough datatypes allowed : you requested too many derived "
             "types (forgot to free ?)");
}

/** \brief this function is used to convert a datatype to a derived datatype
 *
 *  \param in_datatype Input datatype
 *  \param out_datatype Output datatype expressed as a derived datatype
 *
 */
int PMPC_Type_convert_to_derived(MPC_Datatype in_datatype,
                                 MPC_Datatype *out_datatype) {
  /* Is it already a derived datatype ? */
  if (sctk_datatype_is_derived(in_datatype)) {
    /* Yes nothing to do here just copy it to the new type */
    *out_datatype = in_datatype;
    MPC_ERROR_SUCESS();
  } else {
    /* Its either a derived or common datatype */

    /* Retrieve task specific context */
    sctk_task_specific_t *task_specific;
    task_specific = __MPC_get_task_specific();

    /* First allocate desriptive arrays for derived datatype */
    mpc_pack_absolute_indexes_t *begins_out =
        sctk_malloc(sizeof(mpc_pack_absolute_indexes_t));
    mpc_pack_absolute_indexes_t *ends_out =
        sctk_malloc(sizeof(mpc_pack_absolute_indexes_t));
    sctk_datatype_t *datatypes_out = sctk_malloc(sizeof(sctk_datatype_t));

    /* We have a single contiguous block here all fo the same type starting at
     * offset 0 */
    begins_out[0] = 0;

    /* Compute the datatype block end with contiguous size */
    size_t type_size;
    PMPC_Type_size(in_datatype, &type_size);
    ends_out[0] = type_size - 1;

    /* Retrieve previous datatype if the type is contiguous */
    datatypes_out[0] = in_datatype;

    /* FILL ctx */
    struct Datatype_External_context dtctx;
    sctk_datatype_external_context_clear(&dtctx);
    dtctx.combiner = MPC_COMBINER_CONTIGUOUS;
    dtctx.count = 1;
    dtctx.oldtype = in_datatype;

    /* Lets now initialize the new derived datatype */
    PMPC_Derived_datatype(out_datatype, begins_out, ends_out, datatypes_out, 1,
                          0, 0, 0, 0, &dtctx);

    /* Free temporary buffers */
    sctk_free(begins_out);
    sctk_free(ends_out);
    sctk_free(datatypes_out);
  }

  MPC_ERROR_SUCESS();
}

/** \brief Pack a derived datatype in a contiguous buffer
 *  \param inbuffer The input buffer
 *  \param outbuffer The output buffer (has to be allocated to size !)
 *  \param datatype Datatype to be copied
 */
int PMPC_Copy_in_buffer(void *inbuffer, void *outbuffer,
                        MPC_Datatype datatype) {
  /* Only meaningful if its a derived datatype */
  if (sctk_datatype_is_derived(datatype)) {
    int j;
    char *tmp;

    /* Retrieve task ctx */
    sctk_task_specific_t *task_specific = __MPC_get_task_specific();

    tmp = (char *)outbuffer;

    /* Get a pointer to the target type */
    sctk_derived_datatype_t *t =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);

    if (!t) {
      MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                       "Failed to retrieve this derived datatype");
    }

    /* Copy ach block one by one in the pack */
    for (j = 0; j < t->opt_count; j++) {
      size_t size;
      /* Sizeof block */
      size = t->opt_ends[j] - t->opt_begins[j] + 1;
      memcpy(tmp, ((char *)inbuffer) + t->begins[j], size);
      /* Increment offset in packed block */
      tmp += size;
    }

  } else {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN, "");
  }
  MPC_ERROR_SUCESS();
}

/** \brief Unpack a derived datatype from contiguous buffer
 *  \param inbuffer The input buffer
 *  \param outbuffer The output buffer (has to be allocated to size !)
 *  \param datatype Datatype to be unpacked
 */
int PMPC_Copy_from_buffer(void *inbuffer, void *outbuffer,
                          MPC_Datatype datatype) {
  /* Only meaningful if its a derived datatype */
  if (sctk_datatype_is_derived(datatype)) {
    int j;
    char *tmp;

    /* Retrieve task ctx */
    sctk_task_specific_t *task_specific = __MPC_get_task_specific();

    tmp = (char *)inbuffer;

    /* Get a pointer to the target type */
    sctk_derived_datatype_t *t =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);

    if (!t) {
      MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                       "Failed to retrieve this derived datatype");
    }

    /* Unpack each block at the correct offset */
    for (j = 0; j < t->opt_count; j++) {
      size_t size;
      /* Sizeof block */
      size = t->opt_ends[j] - t->opt_begins[j] + 1;
      /* Copy at offset */
      memcpy(((char *)outbuffer) + t->begins[j], tmp, size);
      /* Move in the contiguous block */
      tmp += size;
    }

  } else {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN, "");
  }

  MPC_ERROR_SUCESS();
}

/** \brief Extract derived datatype informations
 *  On non derived datatypes res is set to 0
 *  and datatype contains garbage
 *
 *  \param datatype Datatype to analyze
 *  \param res set to 0 if not a derived datatype, 1 if derived
 *  \param output_datatype A derived datatype filled with all the informations
 *
 */
int MPC_Is_derived_datatype(MPC_Datatype datatype, int *res,
                            sctk_derived_datatype_t *output_datatype) {
  /*
   * Initialize output argument
   * assuming it is not a derived datatype
   */
  *res = 0;
  memset(output_datatype, 0, sizeof(sctk_derived_datatype_t));
  output_datatype->count = 1;

  /* Check whether the datatype ID falls in the derived range ID */
  if (sctk_datatype_is_derived(datatype)) {
    /* Retrieve task specific context */
    sctk_task_specific_t *task_specific;
    task_specific = __MPC_get_task_specific();

    sctk_nodebug("datatype(%d) - SCTK_COMMON_DATA_TYPE_COUNT(%d) - "
                 "SCTK_USER_DATA_TYPES_MAX(%d) ==  %d-%d-%d == %d",
                 datatype, SCTK_COMMON_DATA_TYPE_COUNT,
                 SCTK_USER_DATA_TYPES_MAX, datatype,
                 SCTK_COMMON_DATA_TYPE_COUNT, SCTK_USER_DATA_TYPES_MAX,
                 MPC_TYPE_MAP_TO_DERIVED(datatype));

    /* Retrieve the datatype from the array */
    sctk_derived_datatype_t *target_type =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);

    if (!target_type) {
      MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
                       "Failed to retrieve this derived datatype");
    }

    /* We have found a derived datatype */
    *res = 1;

    /* Copy its content to out arguments */
    memcpy(output_datatype, target_type, sizeof(sctk_derived_datatype_t));
  }

  MPC_ERROR_SUCESS();
}

int MPC_Datatype_set_context(MPC_Datatype datatype,
                             struct Datatype_External_context *dctx) {

  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  sctk_contiguous_datatype_t *target_contiguous_type;
  sctk_derived_datatype_t *target_derived_type;

  switch (sctk_datatype_kind(datatype)) {
  case MPC_DATATYPES_COMMON:
    /* Nothing to do here */
    MPC_ERROR_SUCESS();
    break;

  case MPC_DATATYPES_CONTIGUOUS:
    /* Get a pointer to the type of interest */
    target_contiguous_type =
        sctk_task_specific_get_contiguous_datatype(task_specific, datatype);
    /* Save the context */
    sctk_datatype_context_set(&target_contiguous_type->context, dctx);
    break;

  case MPC_DATATYPES_DERIVED:
    /* Get a pointer to the type of interest */
    target_derived_type =
        sctk_task_specific_get_derived_datatype(task_specific, datatype);
    /* Save the context */
    sctk_datatype_context_set(&target_derived_type->context, dctx);
    break;

  case MPC_DATATYPES_UNKNOWN:
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "This datatype is unknown");
    break;
  }

  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Datatype  Attribute Handling                                         */
/************************************************************************/

/* KEYVALS */

int PMPC_Type_create_keyval(MPC_Type_copy_attr_function *copy,
                            MPC_Type_delete_attr_function *delete,
                            int *type_keyval, void *extra_state) {
  return sctk_type_create_keyval(copy, delete, type_keyval, extra_state);
}

int PMPC_Type_free_keyval(int *type_keyval) {
  return sctk_type_free_keyval(type_keyval);
}

/* ATTRS */

int PMPC_Type_get_attr(MPC_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  struct Datatype_Array *da = &task_specific->datatype_array;

  if (!da) {
    return MPC_ERR_INTERN;
  }

  return sctk_type_get_attr(da, datatype, type_keyval, attribute_val, flag);
}

int PMPC_Type_set_attr(MPC_Datatype datatype, int type_keyval,
                       void *attribute_val) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  struct Datatype_Array *da = &task_specific->datatype_array;

  if (!da) {
    return MPC_ERR_INTERN;
  }

  return sctk_type_set_attr(da, datatype, type_keyval, attribute_val);
}

int PMPC_Type_delete_attr(MPC_Datatype datatype, int type_keyval) {
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();
  struct Datatype_Array *da = &task_specific->datatype_array;

  if (!da) {
    return MPC_ERR_INTERN;
  }

  return sctk_type_delete_attr(da, datatype, type_keyval);
}

/************************************************************************/
/* MPC Init and Finalize                                                */
/************************************************************************/

int PMPC_Init(int *argc, char ***argv) {
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Init);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Init");
#endif
  task_specific = __MPC_get_task_specific();
  /* If the task calls MPI_Init() a second time */
  if (task_specific->finalize_done == 1) {
    return MPC_ERR_OTHER;
  }

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);
  task_specific->init_done = 1;
  task_specific->thread_level = MPC_THREAD_MULTIPLE;
  SCTK_PROFIL_END(MPC_Init);
  MPC_ERROR_SUCESS();
}

int PMPC_Init_thread(int *argc, char ***argv, int required, int *provided) {
  const int res = PMPC_Init(argc, argv);
  if (res == MPC_SUCCESS) {
    sctk_task_specific_t *task_specific;
    task_specific = __MPC_get_task_specific();
    task_specific->thread_level = required;
    *provided = required;
  }

  return res;
}

int PMPC_Query_thread(int *provided) {
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  if (task_specific->thread_level == -1) {
    return MPC_ERR_OTHER;
  }

  *provided = task_specific->thread_level;
  MPC_ERROR_SUCESS();
}

int PMPC_Initialized(int *flag) {
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Initialized);
  task_specific = __MPC_get_task_specific();
  *flag = task_specific->init_done;
  SCTK_PROFIL_END(MPC_Initialized);
  MPC_ERROR_SUCESS();
}

int PMPC_Finalize(void) {
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Finalize);
  sctk_nodebug("PMPC_Finalize");

  __MPC_Barrier(MPC_COMM_WORLD);

  task_specific = __MPC_get_task_specific();
  task_specific->init_done = 0;
  task_specific->finalize_done = 1;

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Finalize");
#endif
  fflush(stderr);
  fflush(stdout);
  SCTK_PROFIL_END(MPC_Finalize);
  MPC_ERROR_SUCESS();
}

static int MPC_EXIT_ON_ABORT_VAL = 0;
void MPC_EXIT_ON_ABORT() { /*   MPC_EXIT_ON_ABORT_VAL = 1; */
}

int PMPC_Abort(MPC_Comm comm, int errorcode) {
  SCTK_PROFIL_START(MPC_Abort);
  mpc_check_comm(comm, comm);
  sctk_error("MPC_Abort with error %d", errorcode);
  fflush(stderr);
  fflush(stdout);
  if (MPC_EXIT_ON_ABORT_VAL == 0) {
    sctk_abort();
  } else {
    sctk_error("Skip abort and exit with 0");
    exit(0);
  }
  SCTK_PROFIL_END(MPC_Abort);
  MPC_ERROR_SUCESS();
}

void PMPC_Default_error(MPC_Comm *comm, int *error, char *msg, char *file,
                        int line) {
  char str[1024];
  int i;
  PMPC_Error_string(*error, str, &i);
  if (i != 0)
    sctk_error("%s file %s line %d %s", str, file, line, msg);
  else
    sctk_error("Unknown error");
  PMPC_Abort(*comm, *error);
}

void PMPC_Return_error(MPC_Comm *comm, int *error, ...) {
  char str[1024];
  int i;
  PMPC_Error_string(*error, str, &i);
  if (i != 0)
    sctk_error("Warning: %s on comm %d", str, (int)*comm);
}

void PMPC_Abort_error(MPC_Comm *comm, int *error, char *message, char *file,
                      int line) {
  char str[1024];
  int i;
  PMPC_Error_string(*error, str, &i);

  sctk_error("===================================================");
  if (i != 0)
    sctk_error("Error: %s on comm %d", str, (int)*comm, message);

  sctk_error("This occured at %s:%d", file, line);

  sctk_error("MPC Encountered an Error will now abort");
  sctk_error("===================================================");
  sctk_abort();
}

int PMPC_Restarted(int *flag) {
  void *f;
  f = sctk_thread_getspecific_mpc(sctk_check_point_key);
  if (f == ((void *)1))
    *flag = 1;
  else
    *flag = 0;
  MPC_ERROR_SUCESS();
}

#ifdef MPC_Fault_Tolerance
static volatile MPC_Checkpoint_state global_state = MPC_STATE_ERROR;
#else
static volatile MPC_Checkpoint_state global_state = '\0';
#endif

/**
 * Trigger a checkpoint for the whole application.
 *
 * We are aware this functino implements somehow a barrier through three "expensive" atomics
 * But keep in mind that creating a checkpoint for the whole application is far more expensive
 * than that.
 *
 * This function sets "state" depending on the application state:
 *   - If we reach this function, it is a simple checkpoint -> MPC_STATE_CHECKPOINT
 *   - If we restart from a snapshot, it is a restart -> MPC_STATE_RESTART
 *
 * \param[out] state The state after the procedure.
 */
int PMPC_Checkpoint(MPC_Checkpoint_state* state) {
#ifdef MPC_Fault_Tolerance
	if (sctk_checkpoint_mode)
	{
		int total_nbprocs = sctk_get_process_number();
		int local_nbtasks = sctk_get_local_task_number();
		int local_tasknum = sctk_get_local_task_rank();
		int task_rank = sctk_get_task_rank();
		int pmi_rank = -1;
		static sctk_atomics_int init_once = OPA_INT_T_INITIALIZER(0);
		static sctk_atomics_int gen_acquire = OPA_INT_T_INITIALIZER(0);
		static sctk_atomics_int gen_release = OPA_INT_T_INITIALIZER(0);
		static sctk_atomics_int gen_current = OPA_INT_T_INITIALIZER(0);
		static int * task_generations;

		/* init once the genration array */
		if(sctk_atomics_cas_int(&init_once, 0, 1) == 0)
		{
			task_generations = (int*)malloc(sizeof(int) * local_nbtasks);
			memset(task_generations, 0, sizeof(int) * local_nbtasks);
			sctk_atomics_store_int(&init_once, 2);
		}
		else
		{
			while(sctk_atomics_load_int(&init_once) != 2)
				sctk_thread_yield();
		}

		/* ensure there won't be any overlapping betwen different MPC_Checkpoint() calls */
		while(sctk_atomics_load_int(&gen_current) < task_generations[local_tasknum])
			sctk_thread_yield();

		/* if I'm the last task to process: do the work */
		if(sctk_atomics_fetch_and_incr_int(&gen_acquire) == local_nbtasks -1)
		{
                        /* save the old checkpoint/restart counters */
			sctk_ft_checkpoint_init();

			/* synchronize all processes */
			sctk_ft_disable();
			if(total_nbprocs > 1)
			{
				sctk_pmi_get_process_rank(&pmi_rank);
				sctk_pmi_barrier();
			}
			else
			{
				pmi_rank = 0;
			}

                        /* close the network */
                        sctk_ft_checkpoint_prepare();
                        sctk_ft_enable();

			/* Only one process triggers the checkpoint (=notify the coordinator) */
			if(pmi_rank == 0)
			{
                                sctk_ft_checkpoint(); /* we can ignore the return value here, by construction */
			}
		
			global_state = sctk_ft_checkpoint_wait();

			sctk_ft_checkpoint_finalize();

			/* set gen_release to 0, prepare the end of current generation */ 
			sctk_atomics_store_int(&gen_release, 0);
			/* set gen_aquire to 0: unlock waiting tasks */
			sctk_atomics_store_int(&gen_acquire, 0);

		}
		else
		{
			/* waiting tasks */
			while(sctk_atomics_load_int(&gen_acquire) != 0)
				sctk_thread_yield();
		}
	
		/* update the state for all tasks of this process */
		if(state)
			*state = global_state;

		/* re-init the network at task level if necessary */
		sctk_net_init_task_level(task_rank, sctk_get_cpu());
		sctk_terminaison_barrier(task_rank);

		/* If I'm the last task to reach here, increment the global generation counter */ 
		if(sctk_atomics_fetch_and_incr_int(&gen_release) == local_nbtasks -1)
		{
			sctk_atomics_incr_int(&gen_current);
		}

		/* the current task finished the work for the current generation */
		task_generations[local_tasknum]++;
	}
	else
	{
		if(state)
			*state = MPC_STATE_IGNORE;
	}
#endif
	MPC_ERROR_SUCESS();
}

int PMPC_Migrate() {
#if 0
  if (sctk_checkpoint_mode)
    {
      FILE *file;
      char name[SCTK_MAX_FILENAME_SIZE];
      sctk_thread_t self;
      void *self_p = NULL;
      int rank;
      int vp;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();
      __MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);

      self = sctk_thread_self ();
      memcpy (&self_p, &self, sizeof (long));

      PMPC_Wait_pending_all_comm ();

      sctk_unregister_thread (rank);

      sctk_nodebug ("Will Migrate");
      sctk_thread_migrate ();
      sctk_nodebug ("Migrated");

      PMPC_Processor_rank (&vp);

      sprintf (name, "%s/Task_%d", sctk_store_dir, rank);
      file = fopen (name, "w+");
      assume (file != NULL);
      fprintf (file, "Restart 0\n");
      fprintf (file, "Process %d\n", sctk_process_rank);
      fprintf (file, "Thread %p\n", self_p);
      fprintf (file, "Virtual processor %d\n", vp);
      fclose (file);

      sctk_register_thread (rank);
    }
  MPC_ERROR_SUCESS ();
#else
  not_implemented();
#endif
  return 0;
}

int PMPC_Restart(int rank) {
#if 0
  if (sctk_checkpoint_mode)
    {
      FILE *file;
      char name[SCTK_MAX_FILENAME_SIZE];
      void *self_p = NULL;
      int tmp;
      int vp;
      int res;
      sprintf (name, "%s/Task_%d", sctk_store_dir, rank);

      file = fopen (name, "r");
      assume (file != NULL);
      res = fscanf (file, "Restart %d\n", &tmp);
      assume (res == 1);
      res = fscanf (file, "Process %d\n", &tmp);
      assume (res == 1);
      res = fscanf (file, "Thread %p\n", &self_p);
      assume (res == 1);
      res = fscanf (file, "Virtual processor %d\n", &vp);
      assume (res == 1);
      fclose (file);

      sctk_nodebug ("Recover task %d thread %p", rank, self_p);

      sprintf (name, "mig_task_%p", self_p);
      sctk_thread_restore (self_p, name, vp);
    }
  MPC_ERROR_SUCESS ();
#else
  not_implemented();
#endif
  return 0;
}

int PMPC_Get_activity(int nb_item, MPC_Activity_t *tab, double *process_act) {
  int i;
  int nb_proc;
  double proc_act = 0;
  nb_proc = sctk_get_processor_number();
  sctk_nodebug("Activity %d/%d:", nb_item, nb_proc);
  for (i = 0; (i < nb_item) && (i < nb_proc); i++) {
    double tmp;
    tab[i].virtual_cpuid = i;
    tmp = sctk_thread_get_activity(i);
    tab[i].usage = tmp;
    proc_act += tmp;
    sctk_nodebug("\t- cpu %d activity %f\n", tab[i].virtual_cpuid,
                 tab[i].usage);
  }
  for (; i < nb_item; i++) {
    tab[i].virtual_cpuid = -1;
    tab[i].usage = -1;
  }
  proc_act = proc_act / ((double)nb_proc);
  *process_act = proc_act;
  MPC_ERROR_SUCESS();
}

int PMPC_Move_to(int process, int cpuid) {
  int proc;
  proc = sctk_get_processor_rank();
  sctk_nodebug("move to %d %d(old %d)", process, cpuid, proc);
  if (process == sctk_process_rank) {
    if (proc != cpuid) {
      sctk_thread_proc_migration(cpuid);
    }
  } else {
    if (sctk_is_net_migration_available() && sctk_migration_mode) {
#if 0
	  FILE *file;
	  char name[SCTK_MAX_FILENAME_SIZE];
	  sctk_thread_t self;
	  void *self_p = NULL;
	  int rank;
	  int vp;
	  sctk_task_specific_t *task_specific;
	  task_specific = __MPC_get_task_specific ();
	  __MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);

	  self = sctk_thread_self ();
	  self_p = self;

	  vp = cpuid;

	  sprintf (name, "%s/Task_%d", sctk_store_dir, rank);
	  file = fopen (name, "w+");
	  assume (file != NULL);
	  fprintf (file, "Restart 0\n");
	  fprintf (file, "Process %d\n", sctk_process_rank);
	  fprintf (file, "Thread %p\n", self_p);
	  fprintf (file, "Virtual processor %d\n", vp);
	  fclose (file);

	  memcpy (&self_p, &self, sizeof (long));

	  PMPC_Wait_pending_all_comm ();

	  sctk_unregister_thread (rank);

	  sctk_net_migration (rank, process);

	  PMPC_Processor_rank (&vp);

	  sprintf (name, "%s/Task_%d", sctk_store_dir, rank);
	  file = fopen (name, "w+");
	  assume (file != NULL);
	  fprintf (file, "Restart 0\n");
	  fprintf (file, "Process %d\n", sctk_process_rank);
	  fprintf (file, "Thread %p\n", self_p);
	  fprintf (file, "Virtual processor %d\n", vp);
	  fclose (file);

	  sctk_register_thread (rank);
	  proc = sctk_get_processor_rank ();
	  if (proc != cpuid)
	    {
	      sctk_thread_proc_migration (cpuid);
	    }
#else
      not_implemented();
#endif
    } else {
      sctk_warning("Inter process migration Disabled");
    }
  }
  sctk_nodebug("move to %d %d done", process, cpuid);
  MPC_ERROR_SUCESS();
}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#pragma weak MPC_Task_hook
void MPC_Task_hook(int rank) {
  /*This function is used to intercept MPC tasks' creation when profiling*/
}
#endif

#ifdef MPC_OpenMP
#endif

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
#include <stdio.h>
extern char **environ;
#endif

/** \brief Function used to create a temporary run directory
 */
static inline void sctk_move_to_temp_dir_if_requested_from_env() {
  char *do_move_to_temp = getenv("MPC_MOVE_TO_TEMP");

  if (do_move_to_temp == NULL) {
    /* Nothing to do */
    return;
  }

  /* Retrieve task rank */
  int rank;
  PMPC_Comm_rank(MPC_COMM_WORLD, &rank);

  char currentdir[800];
  char tmpdir[1000];

  /* If root rank create the temp dir */
  if (rank == 0) {
    // First enter a sandbox DIR
    getcwd(currentdir, 800);
    snprintf(tmpdir, 1000, "%s/XXXXXX", currentdir);
    mkdtemp(tmpdir);
    sctk_warning("Creating temp directory %s", tmpdir);
  }

  /* Broadcast the path to all tasks */
  PMPC_Bcast((void *)tmpdir, 1000, MPC_CHAR, 0, MPC_COMM_WORLD);

  /* Only the root of each process does the chdir */
  if (sctk_get_local_task_rank() == 0) {
    chdir(tmpdir);
  }
}

/* main (int argc, char **argv) */
int sctk_user_main(int argc, char **argv) {
  int result;

  sctk_mpc_init_request_null();

  sctk_size_checking_eq(MPC_COMM_WORLD, SCTK_COMM_WORLD, "MPC_COMM_WORLD",
                        "SCTK_COMM_WORLD", __FILE__, __LINE__);
  sctk_size_checking_eq(MPC_COMM_SELF, SCTK_COMM_SELF, "MPC_COMM_SELF",
                        "SCTK_COMM_SELF", __FILE__, __LINE__);

  sctk_check_equal_types(MPC_Datatype, sctk_datatype_t);
  sctk_check_equal_types(sctk_communicator_t, MPC_Comm);
  sctk_check_equal_types(sctk_pack_indexes_t, mpc_pack_indexes_t);
  sctk_check_equal_types(sctk_pack_absolute_indexes_t,
                         mpc_pack_absolute_indexes_t);
  sctk_check_equal_types(sctk_count_t, mpc_msg_count);
  sctk_check_equal_types(sctk_thread_key_t, mpc_thread_key_t);

  sctk_mpc_verify_request_compatibility();

  __MPC_setup_task_specific();

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);

  if (sctk_runtime_config_get()->modules.mpc.hard_checking) {
    MPC_Hard_Check();
  }

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
  MPC_Task_hook(sctk_get_task_rank());
#endif

#ifdef MPC_OpenMP
//__mpcomp_init() ;
#endif

  sctk_move_to_temp_dir_if_requested_from_env();

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);

#ifdef HAVE_ENVIRON_VAR
  result = mpc_user_main(argc, argv, environ);
#else
  result = mpc_user_main(argc, argv);
#endif

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);

#ifdef MPC_Profiler
  sctk_internal_profiler_render();
#endif

#ifdef MPC_OpenMP
//__mpcomp_exit() ;
#endif

  sctk_nodebug("Wait for pending messages");

  MPC_Release_thread_specific();

  sctk_nodebug("All message done");

  sctk_barrier((sctk_communicator_t)MPC_COMM_WORLD);

  __MPC_delete_task_specific();

  return result;
}

/************************************************************************/
/* Topology Informations                                                */
/************************************************************************/
int PMPC_Comm_rank(MPC_Comm comm, int *rank) {
  SCTK_PROFIL_START(MPC_Comm_rank);


  sctk_task_specific_t *task_specific;
  sctk_nodebug("Get rank of %d", comm);
  mpc_check_comm(comm, comm);
  task_specific = __MPC_get_task_specific();
  __MPC_Comm_rank(comm, rank, task_specific);
  sctk_nodebug("Get rank of %d done", comm);

  SCTK_PROFIL_END(MPC_Comm_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_size(MPC_Comm comm, int *size) {
  SCTK_PROFIL_START(MPC_Comm_size);
  mpc_check_comm(comm, comm);
  __MPC_Comm_size(comm, size);
  SCTK_PROFIL_END(MPC_Comm_size);
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_remote_size(MPC_Comm comm, int *size) {
  // SCTK_PROFIL_START (MPC_Comm_remote_size);
  mpc_check_comm(comm, comm);
  __MPC_Comm_remote_size(comm, size);
  // SCTK_PROFIL_END (MPC_Comm_remote_size);
  MPC_ERROR_SUCESS();
}

int PMPC_Node_rank(int *rank) {
  SCTK_PROFIL_START(MPC_Node_rank);
  *rank = sctk_get_node_rank();
  SCTK_PROFIL_END(MPC_Node_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Node_number(int *number) {
  SCTK_PROFIL_START(MPC_Node_number);
  *number = sctk_get_node_number();
  SCTK_PROFIL_END(MPC_Node_number);
  MPC_ERROR_SUCESS();
}

int PMPC_Processor_rank(int *rank) {
  SCTK_PROFIL_START(MPC_Processor_rank);
  *rank = sctk_get_processor_rank();
  SCTK_PROFIL_END(MPC_Processor_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Processor_number(int *number) {
  SCTK_PROFIL_START(MPC_Processor_number);
  *number = sctk_get_processor_number();
  SCTK_PROFIL_END(MPC_Processor_number);
  MPC_ERROR_SUCESS();
}

int PMPC_Process_rank(int *rank) {
  SCTK_PROFIL_START(MPC_Process_rank);
  *rank = sctk_get_process_rank();
  SCTK_PROFIL_END(MPC_Process_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Process_number(int *number) {
  SCTK_PROFIL_START(MPC_Process_number);
  *number = sctk_get_process_number();
  SCTK_PROFIL_END(MPC_Process_number);
  MPC_ERROR_SUCESS();
}

int PMPC_Local_process_rank(int *rank) {
  SCTK_PROFIL_START(MPC_Local_process_rank);
  *rank = sctk_get_local_process_rank();
  SCTK_PROFIL_END(MPC_Local_process_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Local_process_number(int *number) {
  SCTK_PROFIL_START(MPC_Local_process_number);
  *number = sctk_get_local_process_number();
  SCTK_PROFIL_END(MPC_Local_process_number);
  MPC_ERROR_SUCESS();
}

int PMPC_Task_rank(int *rank) {
  SCTK_PROFIL_START(MPC_Task_rank);
  sctk_task_specific_t *task_specific;
  mpc_check_comm(MPC_COMM_WORLD, MPC_COMM_WORLD);
  task_specific = __MPC_get_task_specific();
  __MPC_Comm_rank(MPC_COMM_WORLD, rank, task_specific);
  SCTK_PROFIL_END(MPC_Task_rank);
  MPC_ERROR_SUCESS();
}

int PMPC_Task_number(int *number) {
  SCTK_PROFIL_START(MPC_Task_number);
  mpc_check_comm(MPC_COMM_WORLD, MPC_COMM_WORLD);
  __MPC_Comm_size(MPC_COMM_WORLD, number);
  SCTK_PROFIL_END(MPC_Task_number);
  MPC_ERROR_SUCESS();
}

int PMPC_Local_task_rank(int *rank) {
  *rank = sctk_get_local_task_rank();
  MPC_ERROR_SUCESS();
}

int PMPC_Local_task_number(int *number) {
  *number = sctk_get_local_task_number();
  MPC_ERROR_SUCESS();
}

#define HOSTNAME_SIZE 4096
int PMPC_Get_processor_name(char *name, int *resultlen) {
  SCTK_PROFIL_START(MPC_Get_processor_name);
  gethostname(name, HOSTNAME_SIZE);
  *resultlen = strlen(name);
  SCTK_PROFIL_END(MPC_Get_processor_name);
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Point to point communications                                        */
/************************************************************************/

static inline int __MPC_Isend(void *buf, mpc_msg_count count,
                              MPC_Datatype datatype, int dest, int tag,
                              MPC_Comm comm, MPC_Request *request,
                              sctk_task_specific_t *task_specific) {
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int com_size;
  int buffer_rank;
  size_t msg_size;
  int comm_remote_size;
  char tmp;

  mpc_check_comm(comm, comm);

  if (dest == MPC_PROC_NULL) {
    sctk_mpc_init_request(request, comm, src, REQUEST_SEND);
    MPC_ERROR_SUCESS();
  }

  __MPC_Comm_rank_size(comm, &src, &com_size, task_specific);

  mpc_check_count(count, comm);

  if (count == 0) {
    buf = &tmp;
  }

  mpc_check_buf_null(buf, comm);
  mpc_check_type(datatype, comm);

  if (tag != MPC_ANY_TAG) {
    mpc_check_tag(tag, comm);
  }

  d_size = __MPC_Get_datatype_size(datatype, task_specific);
  msg_size = count * d_size;

  if ((msg_size > MAX_MPC_BUFFERED_SIZE) || (sctk_is_net_message(dest)) ||
      mpc_disable_buffering) {
  FALLBACK_TO_UNBUFERED_ISEND:
    msg = sctk_create_header(src, SCTK_MESSAGE_CONTIGUOUS);
    sctk_add_adress_in_message(msg, buf, msg_size);
    sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, request, msg_size,
                                   SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);
  } else {
    mpc_buffered_msg_t *tmp_buf =
        MPC_per_thread_buffer_pool_acquire_asynchronous();

    if (tmp_buf->completion_flag == SCTK_MESSAGE_DONE) {
      /* We set the buffer as busy */
      tmp_buf->completion_flag = SCTK_MESSAGE_PENDING;

      /* Use the header from the slot */
      msg = &tmp_buf->header;

      /* Initialize the header */
      sctk_init_header(msg, SCTK_MESSAGE_CONTIGUOUS, sctk_no_free_header,
                       sctk_message_copy);

      /* We move asynchronous buffer pool head ahead */
      MPC_per_thread_buffer_pool_step_asynchronous();

      /* Copy message content in the buffer */
      memcpy(tmp_buf->buf, buf, msg_size);

      sctk_add_adress_in_message(msg, tmp_buf->buf, msg_size);
      sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, request,
                                     msg_size, SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);

      /* Register the async buffer to release the wait immediately */
      msg->tail.buffer_async = tmp_buf;
    } else {
      /* Here we are starving: the oldest async buffer is still
       * in use therefore no buffering */
      goto FALLBACK_TO_UNBUFERED_ISEND;
    }
  }

  sctk_nodebug("Message from %d to %d", src, dest);
  sctk_nodebug("isend : snd2, my rank = %d", src);
  sctk_send_message(msg);
  MPC_ERROR_SUCESS();
}

static inline int __MPC_Issend(void *buf, mpc_msg_count count,
                               MPC_Datatype datatype, int dest, int tag,
                               MPC_Comm comm, MPC_Request *request,
                               sctk_task_specific_t *task_specific) {
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int com_size;
  size_t msg_size;
  char tmp;

  mpc_check_comm(comm, comm);
  __MPC_Comm_rank_size(comm, &src, &com_size, task_specific);

  mpc_check_count(count, comm);
  if (count == 0) {
    buf = &tmp;
  }
  mpc_check_buf(buf, comm);
  mpc_check_type(datatype, comm);

  if (dest == MPC_PROC_NULL) {
    sctk_mpc_init_request(request, comm, src, REQUEST_SEND);
    MPC_ERROR_SUCESS();
  }

  //~ if(sctk_is_inter_comm(comm))
  //~ {
  //~ int remote_size;
  //~ PMPC_Comm_remote_size(comm, &remote_size);
  //~ mpc_check_msg_inter(src, dest, tag, comm, com_size, remote_size);
  //~ }
  //~ else
  //~ {
  //~ mpc_check_msg (src, dest, tag, comm, com_size);
  //~ }
  msg = sctk_create_header(src, SCTK_MESSAGE_CONTIGUOUS);
  d_size = __MPC_Get_datatype_size(datatype, task_specific);
  msg_size = count * d_size;

  sctk_add_adress_in_message(msg, buf, msg_size);
  sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, request, msg_size,
                                 SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);

  sctk_nodebug("Message from %d to %d", src, dest);
  sctk_nodebug("issend : snd2, my rank = %d", src);
  sctk_send_message(msg);
  MPC_ERROR_SUCESS();
}

int PMPC_Isend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
               int tag, MPC_Comm comm, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Isend);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm,
                "MPC_Isend ptr=%p count=%lu type=%d dest=%d tag=%d req=%p", buf,
                count, datatype, dest, tag, request);
#endif
  res = __MPC_Isend(buf, count, datatype, dest, tag, comm, request,
                    task_specific);
  SCTK_PROFIL_END(MPC_Isend);
  return res;
}

int PMPC_Ibsend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
                int tag, MPC_Comm comm, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START(MPC_Ibsend);
  task_specific = __MPC_get_task_specific();
  res = __MPC_Isend(buf, count, datatype, dest, tag, comm, request,
                    task_specific);
  SCTK_PROFIL_END(MPC_Ibsend);
  return res;
}

int PMPC_Issend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
                int tag, MPC_Comm comm, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START(MPC_Issend);
  task_specific = __MPC_get_task_specific();
  res = __MPC_Issend(buf, count, datatype, dest, tag, comm, request,
                     task_specific);
  SCTK_PROFIL_END(MPC_Issend);
  return res;
}

int PMPC_Irsend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
                int tag, MPC_Comm comm, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START(MPC_Irsend);
  task_specific = __MPC_get_task_specific();
  res = __MPC_Isend(buf, count, datatype, dest, tag, comm, request,
                    task_specific);
  SCTK_PROFIL_END(MPC_Irsend);
  return res;
}

static inline int __MPC_Irecv(void *buf, mpc_msg_count count,
                              MPC_Datatype datatype, int source, int tag,
                              MPC_Comm comm, MPC_Request *request,
                              sctk_task_specific_t *task_specific) {
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int comm_size;
  char tmp;

  mpc_check_comm(comm, comm);
  if (source == MPC_PROC_NULL) {
    sctk_mpc_init_request(request, comm, src, REQUEST_RECV);
    MPC_ERROR_SUCESS();
  }

  mpc_check_count(count, comm);
  if (count == 0) {
    buf = &tmp;
  }
  mpc_check_buf_null(buf, comm);
  mpc_check_type(datatype, comm);

  __MPC_Comm_rank_size(comm, &src, &comm_size, task_specific);

  /*   mpc_check_msg (source, src, tag, comm, comm_size); */
  mpc_check_task_msg(src, comm, " destination", comm_size);
  if (source != MPC_ANY_SOURCE) {
    //~ if(sctk_is_inter_comm(comm))
    //~ {
    //~ int remote_size;
    //~ __MPC_Comm_remote_size (comm, &remote_size);
    //~ mpc_check_task_msg (source, comm, " source", remote_size);
    //~ }
    //~ else
    //~ mpc_check_task_msg (source, comm, " source", comm_size);
  }
  if (tag != MPC_ANY_TAG) {
    mpc_check_tag(tag, comm);
  }

  msg = sctk_create_header(src, SCTK_MESSAGE_CONTIGUOUS);
  d_size = __MPC_Get_datatype_size(datatype, task_specific);
  sctk_add_adress_in_message(msg, buf, count * d_size);
  sctk_mpc_set_header_in_message(msg, tag, comm, source, src, request,
                                 count * d_size, SCTK_P2P_MESSAGE, datatype, REQUEST_RECV);
  sctk_nodebug("ircv : rcv, my rank = %d", src);
  sctk_recv_message(
      msg,
      task_specific->my_ptp_internal[comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER],
      0);
  MPC_ERROR_SUCESS();
}

int PMPC_Irecv(void *buf, mpc_msg_count count, MPC_Datatype datatype,
               int source, int tag, MPC_Comm comm, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START(MPC_Irecv);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm,
                "MPC_Irecv ptr=%p count=%lu type=%d source=%d tag=%d req=%p",
                buf, count, datatype, source, tag, request);
#endif
  res = __MPC_Irecv(buf, count, datatype, source, tag, comm, request,
                    task_specific);
  SCTK_PROFIL_END(MPC_Irecv);
  return res;
}
static inline int __MPC_Wait(MPC_Request *request, MPC_Status *status) {
  if (sctk_mpc_get_message_source(request) == MPC_PROC_NULL) {
    sctk_mpc_message_set_is_null(request, 1);
  }

  if (sctk_mpc_message_is_null(request) != 1) {
    sctk_mpc_wait_message(request);
    sctk_mpc_message_set_is_null(request, 1);
  }

  sctk_mpc_commit_status_from_request(request, status);
  MPC_ERROR_SUCESS();
}

static inline int __MPC_Test(MPC_Request *request, int *flag,
                             MPC_Status *status) {
  mpc_check_comm(sctk_mpc_get_communicator_from_request(request),
                 MPC_COMM_WORLD);
  *flag = 0;
  //~ request->completion_flag == 0 && request->is_null == 0
  if ((sctk_mpc_completion_flag(request) == SCTK_MESSAGE_PENDING) &&
      (!sctk_mpc_message_is_null(request))) {
    sctk_mpc_perform_messages(request);
  }

  //~ request->completion_flag != 0
  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING) {
    *flag = 1;
    sctk_mpc_commit_status_from_request(request, status);
  } else {
    /* No Match Yield */
    sctk_thread_yield();
  }

  //~ request->is_null > 0
  if (sctk_mpc_message_is_null(request)) {
    *flag = 1;
    sctk_mpc_commit_status_from_request(request, status);
  }

  if ((status != MPC_STATUS_IGNORE) && (*flag == 0)) {
    status->MPC_ERROR = MPC_ERR_PENDING;
  }

  MPC_ERROR_SUCESS();
}

static inline int __MPC_Test_check(MPC_Request *request, int *flag,
                                   MPC_Status *status) {
  if (sctk_mpc_message_is_null(request)) {
    *flag = 1;
    MPC_ERROR_SUCESS();
  }

  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING) {
    *flag = 1;
    sctk_mpc_commit_status_from_request(request, status);
    MPC_ERROR_SUCESS();
  }

  mpc_check_comm(sctk_mpc_get_communicator_from_request(request),
                 MPC_COMM_WORLD);
  *flag = 0;

  sctk_mpc_perform_messages(request);

  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING) {
    *flag = 1;
    sctk_mpc_commit_status_from_request(request, status);
  } else {
    /* No Match Yield */
    sctk_thread_yield();
  }

  if ((status != MPC_STATUS_IGNORE) && (*flag == 0)) {
    status->MPC_ERROR = MPC_ERR_PENDING;
  }

  MPC_ERROR_SUCESS();
}

static inline int __MPC_Test_check_light(MPC_Request *request) {
  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING) {
    return 1;
  }

  sctk_mpc_perform_messages(request);

  return sctk_mpc_completion_flag(request);
}

static inline int __MPC_Test_no_check(MPC_Request *request, int *flag,
                                      MPC_Status *status) {
  *flag = 0;
  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING) {
    *flag = 1;
    sctk_mpc_commit_status_from_request(request, status);
  }

  if ((status != MPC_STATUS_IGNORE) && (*flag == 0)) {
    status->MPC_ERROR = MPC_ERR_PENDING;
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Wait(MPC_Request *request, MPC_Status *status) {

  int res;
  SCTK_PROFIL_START(MPC_Wait);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Wait req=%p", request);
#endif

  res = __MPC_Wait(request, status);

  SCTK_PROFIL_END(MPC_Wait);
  return res;
}

struct wfv_waitall_s {
  int ret;
  mpc_msg_count count;
  MPC_Request **array_of_requests;
  MPC_Status *array_of_statuses;
};

static inline void wfv_waitall(void *arg) {
  struct wfv_waitall_s *args = (struct wfv_waitall_s *)arg;
  int flag = 1;
  int i;

  for (i = 0; i < args->count; i++) {
    flag = flag & __MPC_Test_check_light(args->array_of_requests[i]);
  }

  /* All requests received */
  if (flag) {
    for (i = 0; i < args->count; i++) {
      MPC_Status *status;
      MPC_Request *request;

      request = args->array_of_requests[i];

      if (args->array_of_statuses != NULL) {
        status = &(args->array_of_statuses[i]);
        sctk_mpc_commit_status_from_request(request, status);
        sctk_nodebug("source %d\n", status->MPC_SOURCE);
        sctk_mpc_message_set_is_null(request, 1);
      }
    }

    args->ret = 1;
  }
}

/** \brief This function is used to perform a batch wait over ExGrequest classes
 * It relies on the wait_fn provided at \ref MPCX_GRequest_class_create
 * but in order to use it we must make sure that all the requests are of
 * the same type, to do so we check if they have the same wait_fn.
 *
 * We try not to be intrusive as we are in the waitall critical path
 * therefore we are progressivelly testing for our conditions
 */
static inline int __MPC_Waitall_Grequest(mpc_msg_count count,
                                         MPC_Request *parray_of_requests[],
                                         MPC_Status array_of_statuses[]) {
  int i;
  int all_of_the_same_class = 0;
  MPCX_Grequest_wait_fn *ref_wait = NULL;

  /* Do we have at least two requests ? */
  if (2 < count) {
    /* Are we looking at generalized requests ? */
    if (parray_of_requests[0]->request_type == REQUEST_GENERALIZED) {
      ref_wait = parray_of_requests[0]->wait_fn;

      /* Are we playing with extended Grequest classes ? */
      if (ref_wait) {
        /* Does the first two match ? */
        if (ref_wait == parray_of_requests[1]->wait_fn) {
          /* Consider they all match now check the rest */
          all_of_the_same_class = 1;

          for (i = 0; i < count; i++) {
            /* Can we find a different one ? */
            if (parray_of_requests[i]->wait_fn != ref_wait) {
              all_of_the_same_class = 0;
              break;
            }
          }
        }
      }
    }
  }

  /* Yes we can perform the batch wait */
  if (all_of_the_same_class) {
    MPC_Status tmp_status;
    /* Prepare the state array */
    void **array_of_states = sctk_malloc(sizeof(void *) * count);
    assume(array_of_states != NULL);
    for (i = 0; i < count; i++)
      array_of_states[i] = parray_of_requests[i]->extra_state;

    /* Call the batch wait function */
    /* Here the timeout parameter is not obvious as for example
     * ROMIO relies on Wtime which is not constrained by the norm
     * is is just monotonous. Whe should have a scaling function
     * which depends on the time source to be fair */
    (ref_wait)(count, array_of_states, 1e9, &tmp_status);

    sctk_free(array_of_states);

    /* Update the statuses */
    if (array_of_statuses != MPC_STATUSES_IGNORE) {
      /* here we do a for loop as we only checked that the wait function
       * was identical we are not sure that the XGrequests classes werent
       * different ones */
      for (i = 0; i < count; i++) {
        (parray_of_requests[i]->query_fn)(parray_of_requests[i]->extra_state,
                                          &array_of_statuses[i]);
      }
    }

    /* Free the requests */
    for (i = 0; i < count; i++)
      (parray_of_requests[i]->free_fn)(parray_of_requests[i]->extra_state);

    return 1;
  }

  return 0;
}

/** \brief Internal MPI_Waitall implementation relying on pointer to requests
 *
 *  This function is needed in order to call the MPC interface from
 *  the MPI one without having to rely on a set of MPI_Tests as before
 *  now the progress from MPI_Waitall functions can also be polled
 *
 *  \warning This function is not MPI_Waitall (see the MPC_Request *
 * parray_of_requests[] argument)
 *
 */

int __MPC_Waitallp(mpc_msg_count count, MPC_Request *parray_of_requests[],
                   MPC_Status array_of_statuses[]) {
  int i;
  int flag = 0;
  double start, end;
  int show = 1;

  sctk_nodebug("waitall count %d\n", count);
  start = MPC_Wtime();


  /* We have to detect generalized requests as we are not able
   * to progress them in the pooling function as we have no
   * MPI context therefore we force a non-pooled progress
   * when we have generalized requests*/
  int contains_generalized = 0;

  if (__MPC_Waitall_Grequest(count, parray_of_requests, array_of_statuses)) {
    MPC_ERROR_SUCESS()
  }

  for (i = 0; i < count; i++) {
    if (parray_of_requests[i]->request_type == REQUEST_GENERALIZED) {
      contains_generalized |= 1;
      break; 
	}
  }
	  /* Here we force the local polling because of generalized requests
                This will happen only if classical and generalized requests are
                mixed or if the wait_fn is not the same for all requests */
  while( contains_generalized && (flag == 0)) 
  {
    flag = 1;

    for (i = 0; i < count; i++) {
      int tmp_flag = 0;
      MPC_Status *status;
      MPC_Request *request;

      if (array_of_statuses != NULL) {
        status = &(array_of_statuses[i  % count]);
      } else {
        status = MPC_STATUS_IGNORE;
      }

      request = parray_of_requests[i % count];
      
      if( request == &MPC_REQUEST_NULL )
          continue;
     
      __MPC_Test_check(request, &tmp_flag, status);

      /* We set this flag in order to prevent the status
       * from being updated repetitivelly in __MPC_Test_check */
      if (tmp_flag) {
        sctk_mpc_message_set_is_null(request, 1);
      }

      flag = flag & tmp_flag;
    }

    if (flag == 1)
      MPC_ERROR_SUCESS();

    sctk_thread_yield();
  }

  /* XXX: Waitall has been ported for using wait_for_value_and_poll
  * because it provides better results than thread_yield.
  * It performs well at least on Infiniband on NAS  */
  struct wfv_waitall_s wfv;
  wfv.ret = 0;
  wfv.array_of_requests = parray_of_requests;
  wfv.array_of_statuses = array_of_statuses;
  wfv.count = count;
  sctk_inter_thread_perform_idle((int *)&(wfv.ret), 1,
                                 (void (*)(void *))wfv_waitall, (void *)&wfv);

  MPC_ERROR_SUCESS();
}

#define PMPC_WAIT_ALL_STATIC_TRSH 32

int PMPC_Waitall(mpc_msg_count count, MPC_Request array_of_requests[],
                 MPC_Status array_of_statuses[]) {
  int res;
  int i;

  SCTK_PROFIL_START(MPC_Waitall);

#ifdef MPC_LOG_DEBUG
  int i;
  for (i = 0; i < count; i++) {
    mpc_log_debug(MPC_COMM_WORLD, "MPC_Waitall position=%d req=%p", i,
                  array_of_requests[i]);
  }
#endif

  /* Here we are preparing the array of pointer to request
   * in order to call the __MPC_Waitallp function
   * it might be an extra cost but it allows the use of
   * the __MPC_Waitallp function from MPI_Waitall */
  MPC_Request **parray_of_requests;
  MPC_Request *static_parray_of_requests[PMPC_WAIT_ALL_STATIC_TRSH];

  if (count < PMPC_WAIT_ALL_STATIC_TRSH) {
    parray_of_requests = static_parray_of_requests;
  } else {
    parray_of_requests = sctk_malloc(sizeof(MPC_Request *) * count);
    assume(parray_of_requests != NULL);
  }

  /* Fill in the array of requests */
  for (i = 0; i < count; i++)
    parray_of_requests[i] = &(array_of_requests[i]);

  /* Call the pointer based waitall */
  res = __MPC_Waitallp(count, parray_of_requests, array_of_statuses);

  /* Do we need to free the temporary request pointer array */
  if (PMPC_WAIT_ALL_STATIC_TRSH <= count) {
    sctk_free(parray_of_requests);
  }

  SCTK_PROFIL_END(MPC_Waitall);
  return res;
}

int PMPC_Waitsome(mpc_msg_count incount, MPC_Request array_of_requests[],
                  mpc_msg_count *outcount, mpc_msg_count array_of_indices[],
                  MPC_Status array_of_statuses[]) {
  int i;
  int done = 0;
  SCTK_PROFIL_START(MPC_Waitsome);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Waitsome");
#endif
  while (done == 0) {
    for (i = 0; i < incount; i++) {
      if (sctk_mpc_message_get_is_null(&(array_of_requests[i])) != 1) {
        int tmp_flag = 0;
        __MPC_Test_check(&(array_of_requests[i]), &tmp_flag,
                         &(array_of_statuses[done]));
        if (tmp_flag) {
          sctk_mpc_message_set_is_null(&(array_of_requests[i]), 1);
          array_of_indices[done] = i;
          done++;
        }
      }
    }
    if (done == 0) {
      TODO("wait_for_value_and_poll should be used here")
      sctk_thread_yield();
    }
  }
  *outcount = done;
  SCTK_PROFIL_END(MPC_Waitsome);
  MPC_ERROR_SUCESS();
}

int PMPC_Waitany(mpc_msg_count count, MPC_Request array_of_requests[],
                 mpc_msg_count *index, MPC_Status *status) {
  int i;
  SCTK_PROFIL_START(MPC_Waitany);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Waitany");
#endif
  *index = MPC_UNDEFINED;
  while (1) {
    for (i = 0; i < count; i++) {
      if (sctk_mpc_message_get_is_null(&(array_of_requests[i])) != 1) {
        int tmp_flag = 0;
        __MPC_Test_check(&(array_of_requests[i]), &tmp_flag, status);
        if (tmp_flag) {
          __MPC_Wait(&(array_of_requests[i]), status);
          *index = count;
          SCTK_PROFIL_END(MPC_Waitany);
          MPC_ERROR_SUCESS();
        }
      }
    }
    TODO("wait_for_value_and_poll should be used here")
    sctk_thread_yield();
  }
}

static inline int __MPC_Wait_pending(MPC_Comm comm) {
  int src;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);

  __MPC_Comm_rank(comm, &src, task_specific);

  sctk_mpc_wait_all(src, comm);

  MPC_ERROR_SUCESS();
}

int PMPC_Wait_pending(MPC_Comm comm) {
  int res;

  SCTK_PROFIL_START(MPC_Wait_pending);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Wait_pending");
#endif
  res = __MPC_Wait_pending(comm);
  SCTK_PROFIL_END(MPC_Wait_pending);
  return res;
}

int PMPC_Wait_pending_all_comm() {
  int j;
  sctk_task_specific_t *task_specific;
  mpc_per_communicator_t *per_communicator;
  mpc_per_communicator_t *per_communicator_tmp;
  task_specific = __MPC_get_task_specific();

  SCTK_PROFIL_START(MPC_Wait_pending_all_comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Wait_pending_all_comm");
#endif
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_ITER(hh, task_specific->per_communicator, per_communicator,
            per_communicator_tmp) {
    j = per_communicator->key;
    if (sctk_is_valid_comm(j))
      __MPC_Wait_pending(j);
  }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  SCTK_PROFIL_END(MPC_Wait_pending_all_comm);
  MPC_ERROR_SUCESS();
}

int PMPC_Test(MPC_Request *request, int *flag, MPC_Status *status) {
  int res;

  SCTK_PROFIL_START(MPC_Test);
  res = __MPC_Test(request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Test req=%p flag=%d", request, *flag);
#endif
  SCTK_PROFIL_END(MPC_Test);
  return res;
}

int PMPC_Test_no_check(MPC_Request *request, int *flag, MPC_Status *status) {
  int res;
  SCTK_PROFIL_START(MPC_Test_no_check);
  res = __MPC_Test_no_check(request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Test_no_check req=%p flag=%d", request,
                *flag);
#endif
  SCTK_PROFIL_END(MPC_Test_no_check);
  return res;
}

int PMPC_Test_check(MPC_Request *request, int *flag, MPC_Status *status) {
  int res;
  SCTK_PROFIL_START(MPC_Test_check);
  res = __MPC_Test_check(request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Test_check req=%p flag=%d", request,
                *flag);
#endif
  SCTK_PROFIL_END(MPC_Test_check);
  return res;
}

static int __MPC_Ssend(void *buf, mpc_msg_count count, MPC_Datatype datatype,
                       int dest, int tag, MPC_Comm comm) {
  MPC_Request request;
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  size_t msg_size;
  sctk_task_specific_t *task_specific;
  char tmp;
  task_specific = __MPC_get_task_specific();

  __MPC_Comm_rank_size(comm, &src, &size, task_specific);

  mpc_check_comm(comm, comm);
  mpc_check_count(count, comm);
  if (count == 0) {
    buf = &tmp;
  }
  mpc_check_buf(buf, comm);
//~ if(sctk_is_inter_comm(comm))
//~ {
//~ int remote_size;
//~ PMPC_Comm_remote_size(comm, &remote_size);
//~ mpc_check_msg_size_inter (src, dest, tag, comm, size, remote_size);
//~ }
//~ else
//~ {
//~ mpc_check_msg_size (src, dest, tag, comm, size);
//~ }

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf,
                count, datatype, dest, tag);
#endif

  if (dest == MPC_PROC_NULL) {
    MPC_ERROR_SUCESS();
  }

  msg_size = count * __MPC_Get_datatype_size(datatype, task_specific);

  msg = sctk_create_header(src, SCTK_MESSAGE_CONTIGUOUS);
  sctk_add_adress_in_message(msg, buf, msg_size);

  sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, &request, msg_size,
                                 SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);

  sctk_nodebug("count = %d, datatype = %d", SCTK_MSG_SIZE(msg), datatype);
  sctk_send_message(msg);
  sctk_mpc_wait_message(&request);
  MPC_ERROR_SUCESS();
}

static int __MPC_Send(void *restrict buf, mpc_msg_count count,
                      MPC_Datatype datatype, int dest, int tag, MPC_Comm comm) {
  MPC_Request request;

  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  size_t msg_size;
  sctk_task_specific_t *task_specific;
  int buffer_rank;
  char tmp;
  sctk_thread_ptp_message_t header;

  task_specific = __MPC_get_task_specific();

  __MPC_Comm_rank_size(comm, &src, &size, task_specific);

  mpc_check_comm(comm, comm);
  mpc_check_count(count, comm);

  if (count == 0) {
    buf = &tmp;
  }

  mpc_check_buf(buf, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf,
                count, datatype, dest, tag);
#endif
  sctk_nodebug("MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf, count,
               datatype, dest, tag);

  if (dest == MPC_PROC_NULL) {
    MPC_ERROR_SUCESS();
  }

  msg_size = count * __MPC_Get_datatype_size(datatype, task_specific);

  if ((msg_size > MAX_MPC_BUFFERED_SIZE) || (sctk_is_net_message(dest)) ||
      mpc_disable_buffering) {
  FALLBACK_TO_BLOCKING_SEND:
    msg = &header;

    sctk_init_header(msg, SCTK_MESSAGE_CONTIGUOUS, sctk_no_free_header,
                     sctk_message_copy);
    sctk_add_adress_in_message(msg, buf, msg_size);
    sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, &request,
                                   msg_size, SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);

    /* Send */
    sctk_send_message(msg);

    sctk_nodebug("send request.is_null %d", request.is_null);
    /* Wait */
    sctk_mpc_wait_message(&request);
  } else {

    sctk_thread_specific_t * thread_spec = sctk_get_thread_specific();
    mpc_buffered_msg_t *tmp_buf =
        MPC_per_thread_buffer_pool_acquire_synchronous(thread_spec);

    if (sctk_mpc_completion_flag(&(tmp_buf->request)) != SCTK_MESSAGE_DONE) {
      goto FALLBACK_TO_BLOCKING_SEND;
    } else {
      /* Move the buffer head */
      MPC_per_thread_buffer_pool_step_synchronous(thread_spec);

      /* Use static header */
      msg = &(tmp_buf->header);
      /* Init the header */
      sctk_init_header(msg, SCTK_MESSAGE_CONTIGUOUS, sctk_no_free_header,
                       sctk_message_copy);

      sctk_nodebug("Copied message |%s| -> |%s| %d", buf, tmp_buf->buf,
                   msg_size);

      sctk_add_adress_in_message(msg, tmp_buf->buf, msg_size);
      sctk_mpc_set_header_in_message(msg, tag, comm, src, dest,
                                     &(tmp_buf->request), msg_size,
                                     SCTK_P2P_MESSAGE, datatype, REQUEST_SEND);

      /* Copy message in buffer */
      memcpy(tmp_buf->buf, buf, msg_size);

      /* Register async buffer */
      msg->tail.buffer_async = tmp_buf;

      /* Send but do not wait */
      sctk_send_message(msg);
    }
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Ssend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
               int tag, MPC_Comm comm) {
  int res;
  SCTK_PROFIL_START(MPC_Ssend);
  mpc_check_type(datatype, comm);
  res = __MPC_Ssend(buf, count, datatype, dest, tag, comm);
  SCTK_PROFIL_END(MPC_Ssend);
  return res;
}

int PMPC_Bsend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
               int tag, MPC_Comm comm) {
  int res;
  SCTK_PROFIL_START(MPC_Bsend);
  mpc_check_type(datatype, comm);
  res = __MPC_Send(buf, count, datatype, dest, tag, comm);
  SCTK_PROFIL_END(MPC_Bsend);
  return res;
}

int PMPC_Send(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
              int tag, MPC_Comm comm) {
  int res;
  SCTK_PROFIL_START(MPC_Send);
  mpc_check_type(datatype, comm);
  res = __MPC_Send(buf, count, datatype, dest, tag, comm);
  SCTK_PROFIL_END(MPC_Send);
  sctk_nodebug("exit send comm %d", comm);
  return res;
}

int PMPC_Rsend(void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
               int tag, MPC_Comm comm) {
  int res;
  SCTK_PROFIL_START(MPC_Rsend);
  mpc_check_type(datatype, comm);
  res = __MPC_Send(buf, count, datatype, dest, tag, comm);
  SCTK_PROFIL_END(MPC_Rsend);
  return res;
}

static int __MPC_Probe(int source, int tag, MPC_Comm comm, MPC_Status *status,
                       sctk_task_specific_t *task_specific);

int PMPC_Recv(void *buf, mpc_msg_count count, MPC_Datatype datatype, int source,
              int tag, MPC_Comm comm, MPC_Status *status) {
  MPC_Request request;

  sctk_thread_ptp_message_t *msg;
  int src;
  size_t msg_size;
  sctk_task_specific_t *task_specific;
  char tmp;
  SCTK_PROFIL_START(MPC_Recv);
  task_specific = __MPC_get_task_specific();

  mpc_check_comm(comm, comm);
  mpc_check_count(count, comm);

  if (count == 0) {
    buf = &tmp;
  }

  mpc_check_buf(buf, comm);

  mpc_check_type(datatype, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Recv ptr=%p count=%lu type=%d source=%d tag=%d", buf,
                count, datatype, source, tag);
#endif

  sctk_nodebug("MPC_Recv ptr=%p count=%lu type=%d source=%d tag=%d", buf, count,
               datatype, source, tag);

  if (source == MPC_PROC_NULL) {
    if (status != MPC_STATUS_IGNORE) {
      status->MPC_SOURCE = MPC_PROC_NULL;
      status->MPC_TAG = MPC_ANY_TAG;
      status->MPC_ERROR = MPC_SUCCESS;
      status->size = 0;
    }
    SCTK_PROFIL_END(MPC_Recv);
    MPC_ERROR_SUCESS();
  }

  __MPC_Comm_rank_size(comm, &src, &size, task_specific);

  //~ if(sctk_is_inter_comm(comm))
  //~ {
  //~ int remote_size;
  //~ PMPC_Comm_remote_size(comm, &remote_size);
  //~ mpc_check_msg_size_inter (source, src, tag, comm, size, remote_size);
  // remote_size);
  //~ }
  //~ else
  //~ {
  //~ mpc_check_msg_size (source, src, tag, comm, size);
  //~ }

  SCTK_PROFIL_START(MPC_Recv_init_message);
  msg_size = count * __MPC_Get_datatype_size(datatype, task_specific);

  msg = sctk_create_header(src, SCTK_MESSAGE_CONTIGUOUS);

  sctk_add_adress_in_message(msg, buf, msg_size);

  sctk_mpc_set_header_in_message(msg, tag, comm, source, src, &request,
                                 msg_size, SCTK_P2P_MESSAGE, datatype, REQUEST_RECV);

  sctk_recv_message(
      msg,
      task_specific->my_ptp_internal[comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER],
      1);
  sctk_nodebug("recv request.is_null %d", request.is_null);
  SCTK_PROFIL_END(MPC_Recv_init_message);

  sctk_mpc_wait_message(&request);

  if (request.status_error != MPC_SUCCESS) {
    if (status != NULL) {
      status->MPC_ERROR = request.status_error;
    }

    return request.status_error;
  }

  sctk_nodebug("count = %d", msg_size);
  sctk_nodebug("req count = %d", request.header.msg_size);

  sctk_mpc_commit_status_from_request(&request, status);
  SCTK_PROFIL_END(MPC_Recv);
  sctk_nodebug("exit recv comm %d", comm);
  MPC_ERROR_SUCESS();
}

int PMPC_Sendrecv(void *sendbuf, mpc_msg_count sendcount, MPC_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, mpc_msg_count recvcount,
                  MPC_Datatype recvtype, int source, int recvtag, MPC_Comm comm,
                  MPC_Status *status) {
  MPC_Request sendreq;
  MPC_Request recvreq;
  sctk_task_specific_t *task_specific;
  char tmp;
  SCTK_PROFIL_START(MPC_Sendrecv);
  task_specific = __MPC_get_task_specific();

  sendreq = MPC_REQUEST_NULL;
  recvreq = MPC_REQUEST_NULL;

  mpc_check_count(sendcount, comm);
  if (sendcount == 0) {
    sendbuf = &tmp;
  }
  mpc_check_count(recvcount, comm);
  if (recvcount == 0) {
    recvbuf = &tmp;
  }

  mpc_check_buf(sendbuf, comm);
  mpc_check_buf(recvbuf, comm);
  mpc_check_type(sendtype, comm);
  mpc_check_type(recvtype, comm);
  mpc_check_comm(comm, comm);
  mpc_check_tag(recvtag, comm);
  mpc_check_tag(sendtag, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "Sendrecv");
#endif
  __MPC_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &recvreq,
              task_specific);
  __MPC_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &sendreq,
              task_specific);
  __MPC_Wait(&sendreq, status);
  __MPC_Wait(&recvreq, status);
  SCTK_PROFIL_END(MPC_Sendrecv);
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/

int PMPC_Status_set_elements_x(MPC_Status *status, MPC_Datatype datatype,
                               MPC_Count count) {
  if (status == MPC_STATUS_IGNORE)
    MPC_ERROR_SUCESS();

  size_t elem_size = 0;
  PMPC_Type_size(datatype, &elem_size);
  status->size = elem_size * count;

  MPC_ERROR_SUCESS();
}

int PMPC_Status_set_elements(MPC_Status *status, MPC_Datatype datatype,
                             int count) {
  return MPC_Status_set_elements_x(status, datatype, count);
}

int PMPC_Status_set_cancelled(MPC_Status *status, int cancelled) {
  status->cancelled = cancelled;
  MPC_ERROR_SUCESS();
}

int PMPC_Request_get_status(MPC_Request request, int *flag,
                            MPC_Status *status) {
  sctk_mpc_commit_status_from_request(&request, status);

  *flag = (request.completion_flag == SCTK_MESSAGE_DONE);

  MPC_ERROR_SUCESS()
}

int PMPC_Test_cancelled(MPC_Status *status, int *flag) {
  *flag = (status->cancelled == 1);
  MPC_ERROR_SUCESS();
}

int PMPC_Cancel(MPC_Request *request) {
  int ret = sctk_mpc_cancel_message(request);
  return ret;
}

int MPC_Iprobe_inter(const int source, const int destination, const int tag,
                     const MPC_Comm comm, int *flag, MPC_Status *status) {
  sctk_thread_message_header_t msg;
  memset(&msg, 0, sizeof(sctk_thread_message_header_t));

  MPC_Status status_init = MPC_STATUS_INIT;

  mpc_check_comm(comm, comm);

  int has_status = 1;

  if ((status == MPC_STATUS_IGNORE) || (!status)) {
    has_status = 0;
  } else {
    *status = status_init;
  }

  *flag = 0;

  /*handler for MPC_PROC_NULL*/
  if (source == MPC_PROC_NULL) {
    *flag = 1;

    if (has_status) {
      status->MPC_SOURCE = MPC_PROC_NULL;
      status->MPC_TAG = MPC_ANY_TAG;
      status->size = 0;
      status->MPC_ERROR = MPC_SUCCESS;
    }

    MPC_ERROR_SUCESS();
  }

  /* Value to check that the case was handled by
   * one of the if in this function */
  int __did_process = 0;

  if ((source == MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG)) {
    sctk_probe_any_source_any_tag(destination, comm, flag, &msg);
    __did_process = 1;
  } else if ((source != MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG)) {
    msg.message_tag = tag;
    sctk_probe_source_tag(destination, source, comm, flag, &msg);
    __did_process = 1;
  } else if ((source != MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG)) {
    sctk_probe_source_any_tag(destination, source, comm, flag, &msg);
    __did_process = 1;
  } else if ((source == MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG)) {
    msg.message_tag = tag;
    sctk_probe_any_source_tag(destination, comm, flag, &msg);
    __did_process = 1;
  }

  if (*flag) {
    if (has_status) {
      status->MPC_SOURCE = sctk_get_rank(comm, msg.source_task);
      status->MPC_TAG = msg.message_tag;
      status->size = (mpc_msg_count)msg.msg_size;
      status->MPC_ERROR = MPC_ERR_PENDING;
    }
    MPC_ERROR_SUCESS();
  }

  if (!__did_process) {
    fprintf(stderr, "source = %d tag = %d\n", source, tag);
    not_reachable();
    MPC_ERROR_SUCESS();
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Iprobe(int source, int tag, MPC_Comm comm, int *flag,
                MPC_Status *status) {
  int destination;
  int res;

  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Iprobe);

  task_specific = __MPC_get_task_specific();

  mpc_check_comm(comm, comm);
  __MPC_Comm_rank(comm, &destination, task_specific);

  /* Translate ranks */

  if (source != MPC_ANY_SOURCE) {
    if (sctk_is_inter_comm(comm)) {
      source = sctk_get_remote_comm_world_rank(comm, source);
    } else {
      source = sctk_get_comm_world_rank(comm, source);
    }
  } else {
    source = MPC_ANY_SOURCE;
  }

  destination = sctk_get_comm_world_rank(comm, destination);

  res = MPC_Iprobe_inter(source, destination, tag, comm, flag, status);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Iprobe source=%d tag=%d flag=%d", source, tag,
                *flag);
#endif

  SCTK_PROFIL_END(MPC_Iprobe);
  return res;
}

typedef struct {
  int flag;
  int source;
  int destination;
  int tag;
  MPC_Comm comm;
  MPC_Status *status;
} MPC_probe_struct_t;

static void MPC_Probe_poll(MPC_probe_struct_t *arg) {
  MPC_Iprobe_inter(arg->source, arg->destination, arg->tag, arg->comm,
                   &(arg->flag), arg->status);
}

static int __MPC_Probe(int source, int tag, MPC_Comm comm, MPC_Status *status,
                       sctk_task_specific_t *task_specific) {
  MPC_probe_struct_t probe_struct;
  int comm_rank = -1;

  __MPC_Comm_rank(comm, &comm_rank, task_specific);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Probe source=%d tag=%d", source, tag);
#endif
  if (source != SCTK_ANY_SOURCE) {
    if (sctk_is_inter_comm(comm)) {
      probe_struct.source = sctk_get_remote_comm_world_rank(comm, source);
    } else {
      probe_struct.source = sctk_get_comm_world_rank(comm, source);
    }
  } else {
    probe_struct.source = SCTK_ANY_SOURCE;
  }

  probe_struct.destination = sctk_get_comm_world_rank(comm, comm_rank);

  probe_struct.tag = tag;
  probe_struct.comm = comm;
  probe_struct.status = status;

  MPC_Iprobe_inter(probe_struct.source, probe_struct.destination, tag, comm,
                   &probe_struct.flag, status);

  if (probe_struct.flag != 1) {
    sctk_inter_thread_perform_idle(
        &probe_struct.flag, 1, (void (*)(void *))MPC_Probe_poll, &probe_struct);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Probe(int source, int tag, MPC_Comm comm, MPC_Status *status) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Probe);
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);
  res = __MPC_Probe(source, tag, comm, status, task_specific);
  SCTK_PROFIL_END(MPC_Probe);
  return res;
}

int PMPC_Get_count(MPC_Status *status, MPC_Datatype datatype,
                   mpc_msg_count *count) {
  int res = MPC_SUCCESS;
  unsigned long size;
  size_t data_size;

  if (status == MPC_STATUS_IGNORE) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_IN_STATUS, "Invalid status");
  }

  res = PMPC_Type_size(datatype, &data_size);

  if (res != MPC_SUCCESS) {
    return res;
  }

  if (data_size != 0) {
    size = status->size;
    sctk_nodebug("Get_count : count %d, data_type %d (size %d)", size, datatype,
                 data_size);

    if (size % data_size == 0) {
      size = size / data_size;
      *count = size;
    } else {
      *count = MPC_UNDEFINED;
    }
  } else {
    *count = 0;
  }

  return res;
}

/************************************************************************/
/* Collective operations                                                */
/************************************************************************/

int PMPC_Barrier(MPC_Comm comm) {
  int tmp;
  SCTK_PROFIL_START(MPC_Barrier);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Barrier");
#endif

  sctk_nodebug("begin barrier");
  tmp = __MPC_Barrier(comm);
  sctk_nodebug("end barrier");

  SCTK_PROFIL_END(MPC_Barrier);
  return tmp;
}

static inline int __MPC_Bcast(void *buffer, mpc_msg_count count,
                              MPC_Datatype datatype, int root, MPC_Comm comm,
                              sctk_task_specific_t *task_specific) {
  int size, rank;
  MPC_Status status;
  mpc_check_comm(comm, comm);
  __MPC_Comm_size(comm, &size);
  mpc_check_task(root, comm, size);
  mpc_check_type(datatype, comm);
  mpc_check_count(count, comm);

  if (sctk_is_inter_comm(comm)) {
    if (root == MPC_PROC_NULL) {
      MPC_ERROR_SUCESS();
    } else if (root == MPC_ROOT) {
      PMPC_Send(buffer, count, datatype, 0, MPC_BROADCAST_TAG, comm);
    } else {
      __MPC_Comm_rank(comm, &rank, task_specific);

      if (rank == 0) {
        PMPC_Recv(buffer, count, datatype, root, MPC_BROADCAST_TAG, comm,
                  &status);
      }
      __MPC_Bcast(buffer, count, datatype, 0, sctk_get_local_comm_id(comm),
                  task_specific);
    }
  } else {
    if ((size <= 1) || (count == 0)) {
      MPC_ERROR_SUCESS();
    } else
      sctk_broadcast(buffer,
                     count * __MPC_Get_datatype_size(datatype, task_specific),
                     root, comm);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Bcast(void *buffer, mpc_msg_count count, MPC_Datatype datatype,
               int root, MPC_Comm comm) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Bcast);
  task_specific = __MPC_get_task_specific();

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Bcast ptr=%p count=%lu, type=%d, root=%d", buffer,
                count, datatype, root);
#endif
  res = __MPC_Bcast(buffer, count, datatype, root, comm, task_specific);

  sctk_nodebug("Bcast comm %d res %d", comm, res);

  SCTK_PROFIL_END(MPC_Bcast);
  return res;
}

static void MPC_Op_tmp(void *in, void *inout, size_t size, MPC_Datatype t) {
  int n_size;
  MPC_Datatype n_t;
  MPC_User_function *func;
  n_size = (int)size;
  n_t = t;
  func = (MPC_User_function *)sctk_thread_getspecific_mpc(sctk_func_key);
  sctk_assert(func != NULL);
  sctk_nodebug("User reduce %p", func);
  func(in, inout, &n_size, &n_t);
  sctk_nodebug("User reduce %p done", func);
}

static inline int __MPC_Allreduce(void *sendbuf, void *recvbuf,
                                  mpc_msg_count count, MPC_Datatype datatype,
                                  MPC_Op op, MPC_Comm comm,
                                  sctk_task_specific_t *task_specific) {
  MPC_Op_f func;
  int root, rank;
  mpc_check_comm(comm, comm);
  mpc_check_count(count, comm);
  mpc_check_type(datatype, comm);
  __MPC_Comm_rank(comm, &rank, task_specific);

  sctk_nodebug("Allreduce on %d with type %d", comm, datatype);
  if ((op.u_func == NULL) && (sctk_datatype_is_common(datatype) ||
                              sctk_datatype_is_struct_datatype(datatype))) {
    func = sctk_get_common_function(datatype, op);
  } else {
    assume(op.u_func != NULL);
    /*User define function */
    sctk_thread_setspecific_mpc(sctk_func_key, (void *)op.u_func);
    func = (MPC_Op_f)MPC_Op_tmp;
    sctk_nodebug("User reduce");
  }

  /* inter comm */
  if (sctk_is_inter_comm(comm)) {
    /* local group */
    if (sctk_is_in_local_group(comm)) {
      /* reduce receiving from remote group*/
      root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
      PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

      /* reduce sending to remote group */
      root = 0;
      PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    }
    /* remote group */
    else {
      /* reduce sending to local group */
      root = 0;
      PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

      /* reduce receiving from local group */
      root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
      PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    }

    /* broadcast intracomm */
    __MPC_Bcast(recvbuf, count, datatype, 0, sctk_get_local_comm_id(comm),
                task_specific);
  } else {
    sctk_all_reduce(sendbuf, recvbuf,
                    __MPC_Get_datatype_size(datatype, task_specific), count,
                    func, (sctk_communicator_t)comm, (sctk_datatype_t)datatype);
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Allreduce(void *sendbuf, void *recvbuf, mpc_msg_count count,
                   MPC_Datatype datatype, MPC_Op op, MPC_Comm comm) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Allreduce);
  task_specific = __MPC_get_task_specific();
  if (sendbuf == MPC_IN_PLACE) {
    sendbuf = recvbuf;
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm,
                "MPC_Allreduce send_ptr=%p recv_ptr=%p count=%lu, type=%d",
                sendbuf, recvbuf, count, datatype);
#endif
  res = __MPC_Allreduce(sendbuf, recvbuf, count, datatype, op, comm,
                        task_specific);
  SCTK_PROFIL_END(MPC_Allreduce);
  return res;
}

#define MPC_Reduce_tmp_recvbuf_size 4096
int PMPC_Reduce(void *sendbuf, void *recvbuf, mpc_msg_count count,
                MPC_Datatype datatype, MPC_Op op, int root, MPC_Comm comm) {
  unsigned long size;
  char tmp_recvbuf[MPC_Reduce_tmp_recvbuf_size];
  sctk_task_specific_t *task_specific;
  MPC_Status status;
  int com_size;
  int com_rank;
  void *tmp_buf;
  SCTK_PROFIL_START(MPC_Reduce);
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);
  __MPC_Comm_rank_size(comm, &com_rank, &com_size, task_specific);
  mpc_check_task(root, comm, com_size);
  mpc_check_count(count, comm);
  mpc_check_type(datatype, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Reduce send_ptr=%p recv_ptr=%p count=%lu, type=%d",
                sendbuf, recvbuf, count, datatype);
#endif

  /* inter comm */
  if (sctk_is_inter_comm(comm)) {
    /* do nothing */
    if (root == MPC_PROC_NULL)
      MPC_ERROR_SUCESS();

    /* root receive from rank 0 on remote group */
    if (root == MPC_ROOT) {
      PMPC_Recv(recvbuf, count, datatype, 0, 27, comm, &status);
    } else {
      if (com_rank == 0) {
        /* allocate temp buffer */
        size = count * __MPC_Get_datatype_size(datatype, task_specific);
        tmp_buf = (void *)sctk_malloc(size);
      }
      /* reduce on remote group to rank 0*/
      PMPC_Reduce(sendbuf, tmp_buf, count, datatype, op, 0,
                  sctk_get_local_comm_id(comm));

      if (com_rank == 0) {
        /* send to root on local group */
        PMPC_Send(tmp_buf, count, datatype, root, 27, comm);
      }
    }
  } else {
    if (com_rank != root) {
      size = count * __MPC_Get_datatype_size(datatype, task_specific);
      if (size < MPC_Reduce_tmp_recvbuf_size) {
        __MPC_Allreduce(sendbuf, tmp_recvbuf, count, datatype, op, comm,
                        task_specific);
      } else {
        recvbuf = sctk_malloc(size);
        __MPC_Allreduce(sendbuf, recvbuf, count, datatype, op, comm,
                        task_specific);
        sctk_free(recvbuf);
      }
    } else {
      __MPC_Allreduce(sendbuf, recvbuf, count, datatype, op, comm,
                      task_specific);
    }
  }
  SCTK_PROFIL_END(MPC_Reduce);
  MPC_ERROR_SUCESS();
}

int PMPC_Op_create(MPC_User_function *function, int commute, MPC_Op *op) {
  MPC_Op init = MPC_OP_INIT;
  assume(commute);
  *op = init;
  op->u_func = function;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Op_create");
#endif
  MPC_ERROR_SUCESS();
}

int PMPC_Op_free(MPC_Op *op) {
  MPC_Op init = MPC_OP_INIT;
  *op = init;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Op_free");
#endif
  MPC_ERROR_SUCESS();
}

#define MPC_MAX_CONCURENT 100
static inline int __MPC_Gatherv(void *sendbuf, mpc_msg_count sendcnt,
                                MPC_Datatype sendtype, void *recvbuf,
                                mpc_msg_count *recvcnts, mpc_msg_count *displs,
                                MPC_Datatype recvtype, int root, MPC_Comm comm,
                                sctk_task_specific_t *task_specific) {
  int i;
  int j;
  int rank;
  int size, rsize;
  MPC_Request request;
  MPC_Request recvrequest[MPC_MAX_CONCURENT];

  mpc_check_comm(comm, comm);
  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  __MPC_Comm_remote_size(comm, &rsize);
  mpc_check_task(root, comm, size);
  mpc_check_buf(sendbuf, comm);
  mpc_check_count(sendcnt, comm);
  mpc_check_type(sendtype, comm);

  if (sctk_is_inter_comm(comm)) {
    if (root == MPC_PROC_NULL) {
      MPC_ERROR_SUCESS();
    } else if (root == MPC_ROOT) {
      size_t recv_type_size;
      mpc_check_buf(recvbuf, comm);
      mpc_check_type(recvtype, comm);

      recv_type_size = __MPC_Get_datatype_size(recvtype, task_specific);
      i = 0;
      while (i < rsize) {
        for (j = 0; (i < rsize) && (j < MPC_MAX_CONCURENT);) {
          __MPC_Irecv(((char *)recvbuf) + (displs[i] * recv_type_size),
                      recvcnts[i], recvtype, i, MPC_GATHERV_TAG, comm,
                      &(recvrequest[j]), task_specific);
          i++;
          j++;
        }
        j--;
        for (; j >= 0; j--) {
          __MPC_Wait(&(recvrequest[j]), MPC_STATUS_IGNORE);
        }
      }
    } else {
      __MPC_Isend(sendbuf, sendcnt, sendtype, root, MPC_GATHERV_TAG, comm,
                  &request, task_specific);
      __MPC_Wait(&(request), MPC_STATUS_IGNORE);
    }
  } else {
    if ((sendbuf == MPC_IN_PLACE) && (rank == root)) {
      request = mpc_request_null;
    } else {
      __MPC_Isend(sendbuf, sendcnt, sendtype, root, MPC_GATHERV_TAG, comm,
                  &request, task_specific);
    }

    if (rank == root) {
      size_t recv_type_size;
      mpc_check_buf(recvbuf, comm);
      mpc_check_type(recvtype, comm);

      recv_type_size = __MPC_Get_datatype_size(recvtype, task_specific);
      i = 0;
      while (i < size) {
        for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);) {
          if ((sendbuf == MPC_IN_PLACE) && (i == root)) {
            recvrequest[j] = mpc_request_null;
          } else {
            __MPC_Irecv(((char *)recvbuf) + (displs[i] * recv_type_size),
                        recvcnts[i], recvtype, i, MPC_GATHERV_TAG, comm,
                        &(recvrequest[j]), task_specific);
          }
          i++;
          j++;
        }
        j--;
        PMPC_Waitall(j + 1, recvrequest, MPC_STATUSES_IGNORE);
        /* 				for (; j >= 0; j--) */
        /* 				{ */
        /* 					__MPC_Wait (&(recvrequest[j]),
         * MPC_STATUS_IGNORE); */
        /* 				} */
      }
    }

    __MPC_Wait(&(request), MPC_STATUS_IGNORE);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Gatherv(void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
                 void *recvbuf, mpc_msg_count *recvcnts, mpc_msg_count *displs,
                 MPC_Datatype recvtype, int root, MPC_Comm comm) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Gatherv);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Gatherv");
#endif
  res = __MPC_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs,
                      recvtype, root, comm, task_specific);
  SCTK_PROFIL_END(MPC_Gatherv);
  return res;
}

int PMPC_Allgatherv(void *sendbuf, mpc_msg_count sendcount,
                    MPC_Datatype sendtype, void *recvbuf,
                    mpc_msg_count *recvcounts, mpc_msg_count *displs,
                    MPC_Datatype recvtype, MPC_Comm comm) {
  int size, rsize;
  int rank;
  int root = 0;
  MPC_Comm local_comm;
  sctk_task_specific_t *task_specific;
  size_t dsize;
  SCTK_PROFIL_START(MPC_Allgatherv);
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);
  mpc_check_buf(sendbuf, comm);
  mpc_check_buf(recvbuf, comm);
  if (sendbuf != MPC_IN_PLACE) {
    mpc_check_count(sendcount, comm);
    mpc_check_type(sendtype, comm);
  }
  mpc_check_type(recvtype, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Allgatherv");
#endif

  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  __MPC_Comm_remote_size(comm, &rsize);
  if (sctk_is_inter_comm(comm)) {
    if (sctk_is_in_local_group(comm)) {
      root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
      __MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                    recvtype, root, comm, task_specific);

      root = 0;
      __MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                    recvtype, root, comm, task_specific);
    } else {
      root = 0;
      __MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                    recvtype, root, comm, task_specific);

      root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
      __MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                    recvtype, root, comm, task_specific);
    }

    rsize--;
    root = 0;
    local_comm = sctk_get_local_comm_id(comm);
    dsize = __MPC_Get_datatype_size(recvtype, task_specific);
    for (; rsize >= 0; rsize--) {
      __MPC_Bcast(((char *)recvbuf) + (displs[rsize] * dsize),
                  recvcounts[rsize], recvtype, root, local_comm, task_specific);
    }
  } else {
    if (sendbuf == MPC_IN_PLACE) {
      int i = 0;
      size_t extent;
      PMPC_Type_size(recvtype, &extent);
      sendtype = recvtype;
      sendbuf = (char *)recvbuf;
      for (i = 0; i < rank; ++i) {
        sendbuf += (recvcounts[i] * extent);
      }
      sendcount = recvcounts[rank];
    }
    __MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                  recvtype, root, comm, task_specific);
    size--;
    dsize = __MPC_Get_datatype_size(recvtype, task_specific);
    for (; size >= 0; size--) {
      __MPC_Bcast(((char *)recvbuf) + (displs[size] * dsize), recvcounts[size],
                  recvtype, root, comm, task_specific);
    }
  }
  SCTK_PROFIL_END(MPC_Allgatherv);
  MPC_ERROR_SUCESS();
}

static inline int __MPC_Gather(void *sendbuf, mpc_msg_count sendcnt,
                               MPC_Datatype sendtype, void *recvbuf,
                               mpc_msg_count recvcount, MPC_Datatype recvtype,
                               int root, MPC_Comm comm,
                               sctk_task_specific_t *task_specific) {
  int i;
  int j;
  int rank;
  int size;
  MPC_Request request;
  size_t dsize;
  MPC_Request *recvrequest =
      sctk_malloc(sizeof(MPC_Request) * MPC_MAX_CONCURENT);

  assume(recvrequest != NULL);

  mpc_check_comm(comm, comm);
  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);

  if (rank != root) {
    mpc_check_buf(sendbuf, comm);
    mpc_check_count(sendcnt, comm);
    mpc_check_type(sendtype, comm);
  } else {
    mpc_check_count(recvcount, comm);
    mpc_check_type(recvtype, comm);
    mpc_check_buf(recvbuf, comm);
  }
  mpc_check_task(root, comm, size);

  if (sctk_is_inter_comm(comm)) {
    if (root == MPC_PROC_NULL) {
      MPC_ERROR_SUCESS();
    } else if (root == MPC_ROOT) {
      i = 0;
      dsize = __MPC_Get_datatype_size(recvtype, task_specific);
      while (i < size) {
        for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);) {
          __MPC_Irecv(((char *)recvbuf) + (i * recvcount * dsize), recvcount,
                      recvtype, i, MPC_GATHER_TAG, comm, &(recvrequest[j]),
                      task_specific);
          i++;
          j++;
        }
        j--;
        for (; j >= 0; j--) {
          __MPC_Wait(&(recvrequest[j]), MPC_STATUS_IGNORE);
        }
      }
    } else {
      __MPC_Isend(sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm,
                  &request, task_specific);
      __MPC_Wait(&(request), MPC_STATUS_IGNORE);
    }
  } else {

    __MPC_Comm_rank_size(comm, &rank, &size, task_specific);

    if ((sendbuf == MPC_IN_PLACE) && (rank == root)) {
      request = mpc_request_null;
    } else {
      __MPC_Isend(sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm,
                  &request, task_specific);
    }

    if (rank == root) {
      i = 0;
      dsize = __MPC_Get_datatype_size(recvtype, task_specific);
      while (i < size) {
        for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);) {
          if ((sendbuf == MPC_IN_PLACE) && (i == root)) {
            recvrequest[j] = mpc_request_null;
          } else {
            __MPC_Irecv(((char *)recvbuf) + (i * recvcount * dsize), recvcount,
                        recvtype, i, MPC_GATHER_TAG, comm, &(recvrequest[j]),
                        task_specific);
          }
          i++;
          j++;
        }
        j--;
        PMPC_Waitall(j + 1, recvrequest, MPC_STATUSES_IGNORE);
        /* 		  for (; j >= 0; j--) */
        /* 			{ */
        /* 			  __MPC_Wait (&(recvrequest[j]),
         * MPC_STATUS_IGNORE); */
        /* 			} */
      }
    }
    __MPC_Wait(&(request), MPC_STATUS_IGNORE);
  }

  sctk_free(recvrequest);

  MPC_ERROR_SUCESS();
}

int PMPC_Gather(void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
                void *recvbuf, mpc_msg_count recvcount, MPC_Datatype recvtype,
                int root, MPC_Comm comm) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Gather);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Gather send %p,%lu,%d recv %p,%lu,%d root=%d",
                sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root);
#endif
  res = __MPC_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype,
                     root, comm, task_specific);
  SCTK_PROFIL_END(MPC_Gather);
  return res;
}

static inline int __MPC_Allgather(void *sendbuf, mpc_msg_count sendcount,
                                  MPC_Datatype sendtype, void *recvbuf,
                                  mpc_msg_count recvcount,
                                  MPC_Datatype recvtype, MPC_Comm comm,
                                  sctk_task_specific_t *task_specific) {
  int size, rank, remote_size;
  int root = 0;
  void *tmp_buf;
  mpc_check_comm(comm, comm);
  mpc_check_buf(sendbuf, comm);
  mpc_check_buf(recvbuf, comm);
  mpc_check_count(recvcount, comm);
  mpc_check_type(sendtype, comm);
  mpc_check_type(recvtype, comm);
  if (sendbuf != MPC_IN_PLACE)
    mpc_check_count(sendcount, comm);
  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  remote_size = sctk_get_nb_task_remote(comm);

  if (sctk_is_inter_comm(comm)) {
    if (rank == 0 && sendcount > 0) {
      tmp_buf = (void *)sctk_malloc(sendcount * size * sizeof(void *));
    }

    __MPC_Gather(sendbuf, sendcount, sendtype, tmp_buf, sendcount, sendtype, 0,
                 sctk_get_local_comm_id(comm), task_specific);

    if (sctk_is_in_local_group(comm)) {
      if (sendcount != 0) {
        root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
        sctk_nodebug("bcast size %d to the left", size * sendcount);
        __MPC_Bcast(tmp_buf, size * sendcount, sendtype, root, comm,
                    task_specific);
      }

      if (recvcount != 0) {
        root = 0;
        sctk_nodebug("bcast size %d from the left", remote_size * recvcount);
        __MPC_Bcast(recvbuf, remote_size * recvcount, recvtype, root, comm,
                    task_specific);
      }
    } else {
      if (recvcount != 0) {
        root = 0;
        sctk_nodebug("bcast size %d from the right", remote_size * recvcount);
        __MPC_Bcast(recvbuf, remote_size * recvcount, recvtype, root, comm,
                    task_specific);
      }

      if (sendcount != 0) {
        sctk_nodebug("bcast size %d to the right", size * sendcount);
        root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
        __MPC_Bcast(tmp_buf, size * sendcount, sendtype, root, comm,
                    task_specific);
      }
    }
  } else {
    if (MPC_IN_PLACE == sendbuf) {
      size_t extent;
      PMPC_Type_size(recvtype, &extent);
      sendbuf = ((char *)recvbuf) + (rank * extent * recvcount);
      sendtype = recvtype;
      sendcount = recvcount;
    }
    __MPC_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                 root, comm, task_specific);
    __MPC_Bcast(recvbuf, size * recvcount, recvtype, root, comm, task_specific);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Allgather(void *sendbuf, mpc_msg_count sendcount,
                   MPC_Datatype sendtype, void *recvbuf,
                   mpc_msg_count recvcount, MPC_Datatype recvtype,
                   MPC_Comm comm) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Allgather);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Allgather send %p,%lu,%d recv %p,%lu,%d", sendbuf,
                sendcount, sendtype, recvbuf, recvcount, recvtype);
#endif
  res = __MPC_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                        recvtype, comm, task_specific);
  SCTK_PROFIL_END(MPC_Allgather);
  return res;
}

int PMPC_Scatterv(void *sendbuf, mpc_msg_count *sendcnts, mpc_msg_count *displs,
                  MPC_Datatype sendtype, void *recvbuf, mpc_msg_count recvcnt,
                  MPC_Datatype recvtype, int root, MPC_Comm comm) {
  int i;
  int j;
  int rank;
  int size, rsize;
  MPC_Request request;
  MPC_Request sendrequest[MPC_MAX_CONCURENT];
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Scatterv);
  task_specific = __MPC_get_task_specific();

  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  __MPC_Comm_remote_size(comm, &rsize);

  mpc_check_comm(comm, comm);
  mpc_check_task(root, comm, size);

  if (rank == root) {
    mpc_check_buf(sendbuf, comm);
    mpc_check_type(sendtype, comm);
  }

  mpc_check_type(recvtype, comm);
  mpc_check_buf(recvbuf, comm);
  mpc_check_count(recvcnt, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Scatterv");
#endif

  if (sctk_is_inter_comm(comm)) {
    if (root == MPC_PROC_NULL) {
      MPC_ERROR_SUCESS();
    } else if (root == MPC_ROOT) {
      size_t send_type_size;
      send_type_size = __MPC_Get_datatype_size(sendtype, task_specific);
      i = 0;
      while (i < rsize) {
        for (j = 0; (i < rsize) && (j < MPC_MAX_CONCURENT);) {
          __MPC_Isend(((char *)sendbuf) + (displs[i] * send_type_size),
                      sendcnts[i], sendtype, i, MPC_SCATTERV_TAG, comm,
                      &(sendrequest[j]), task_specific);
          i++;
          j++;
        }
        j--;
        for (; j >= 0; j--) {
          __MPC_Wait(&(sendrequest[j]), MPC_STATUS_IGNORE);
        }
      }
    } else {
      __MPC_Irecv(recvbuf, recvcnt, recvtype, root, MPC_SCATTERV_TAG, comm,
                  &request, task_specific);
      __MPC_Wait(&(request), MPC_STATUS_IGNORE);
    }
  } else {
    if ((recvbuf == MPC_IN_PLACE) && (rank == root)) {
      request = mpc_request_null;
    } else {
      __MPC_Irecv(recvbuf, recvcnt, recvtype, root, MPC_SCATTERV_TAG, comm,
                  &request, task_specific);
    }

    if (rank == root) {
      size_t send_type_size;
      send_type_size = __MPC_Get_datatype_size(sendtype, task_specific);
      i = 0;
      while (i < size) {
        for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);) {
          if ((recvbuf == MPC_IN_PLACE) && (i == root)) {
            sendrequest[j] = mpc_request_null;
          } else {
            __MPC_Isend(((char *)sendbuf) + (displs[i] * send_type_size),
                        sendcnts[i], sendtype, i, MPC_SCATTERV_TAG, comm,
                        &(sendrequest[j]), task_specific);
          }
          i++;
          j++;
        }
        j--;
        PMPC_Waitall(j + 1, sendrequest, MPC_STATUSES_IGNORE);
        /*
                                        for (; j >= 0; j--)
                                        {
                                                __MPC_Wait (&(sendrequest[j]),
           MPC_STATUS_IGNORE);
                                        }
        */
      }
    }

    __MPC_Wait(&(request), MPC_STATUS_IGNORE);
  }
  SCTK_PROFIL_END(MPC_Scatterv);
  MPC_ERROR_SUCESS();
}

int PMPC_Scatter(void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
                 void *recvbuf, mpc_msg_count recvcnt, MPC_Datatype recvtype,
                 int root, MPC_Comm comm) {
  int i;
  int j;
  int rank, world_rank;
  int size, rsize;
  MPC_Request request;
  size_t dsize;
  MPC_Request sendrequest[MPC_MAX_CONCURENT];
  sctk_task_specific_t *task_specific;

  //~ if(sendcnt != recvcnt)
  //~ MPC_ERROR_REPORT(comm, MPC_ERR_COUNT, "sendcount and recvcount must be the
  // same");

  SCTK_PROFIL_START(MPC_Scatter);
  task_specific = __MPC_get_task_specific();

  mpc_check_comm(comm, comm);
  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  __MPC_Comm_rank(MPC_COMM_WORLD, &world_rank, task_specific);
  __MPC_Comm_remote_size(comm, &rsize);
  mpc_check_buf(sendbuf, comm);
  mpc_check_buf(recvbuf, comm);
  mpc_check_type(sendtype, comm);
  mpc_check_type(recvtype, comm);
  mpc_check_count(sendcnt, comm);
  mpc_check_count(recvcnt, comm);
  mpc_check_task(root, comm, size);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Scatter");
#endif

  if (sctk_is_inter_comm(comm)) {
    if (root == MPC_PROC_NULL) {
      MPC_ERROR_SUCESS();
    } else if (root == MPC_ROOT) {
      i = 0;
      dsize = __MPC_Get_datatype_size(recvtype, task_specific);
      while (i < rsize) {
        for (j = 0; (i < rsize) && (j < MPC_MAX_CONCURENT);) {
          __MPC_Isend(((char *)sendbuf) + (i * sendcnt * dsize), sendcnt,
                      sendtype, i, MPC_SCATTER_TAG, comm, &(sendrequest[j]),
                      task_specific);
          i++;
          j++;
        }
        j--;
        for (; j >= 0; j--) {
          __MPC_Wait(&(sendrequest[j]), MPC_STATUS_IGNORE);
        }
      }
    }

    else {
      __MPC_Irecv(recvbuf, recvcnt, recvtype, root, MPC_SCATTER_TAG, comm,
                  &request, task_specific);
      __MPC_Wait(&(request), MPC_STATUS_IGNORE);
    }
  } else {
    __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
    if ((recvbuf == MPC_IN_PLACE) && (rank == root)) {
      request = mpc_request_null;
    } else {
      __MPC_Irecv(recvbuf, recvcnt, recvtype, root, MPC_SCATTER_TAG, comm,
                  &request, task_specific);
    }

    if (rank == root) {
      i = 0;
      dsize = __MPC_Get_datatype_size(sendtype, task_specific);
      while (i < size) {
        for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);) {
          if ((recvbuf == MPC_IN_PLACE) && (i == root)) {
            sendrequest[j] = mpc_request_null;
          } else {
            __MPC_Isend(((char *)sendbuf) + (i * sendcnt * dsize), sendcnt,
                        sendtype, i, MPC_SCATTER_TAG, comm, &(sendrequest[j]),
                        task_specific);
          }
          i++;
          j++;
        }
        j--;
        PMPC_Waitall(j + 1, sendrequest, MPC_STATUSES_IGNORE);
        /*
                                        for (; j >= 0; j--)
                                        {
                                                __MPC_Wait (&(sendrequest[j]),
           MPC_STATUS_IGNORE);
                                        }
        */
      }
    }

    __MPC_Wait(&(request), MPC_STATUS_IGNORE);
  }
  SCTK_PROFIL_END(MPC_Scatter);
  MPC_ERROR_SUCESS();
}

int PMPC_Alltoall(void *sendbuf, mpc_msg_count sendcount, MPC_Datatype sendtype,
                  void *recvbuf, mpc_msg_count recvcnt, MPC_Datatype recvtype,
                  MPC_Comm comm) {
  int i;
  int size;
  int rank;
  int bblock = 4;
  MPC_Request requests[2 * bblock * sizeof(MPC_Request)];
  int ss, ii;
  int dst;
  size_t d_send;
  size_t d_recv;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Alltoall);
  task_specific = __MPC_get_task_specific();

  mpc_check_buf(sendbuf, comm);
  mpc_check_type(sendtype, comm);
  mpc_check_type(recvtype, comm);
  mpc_check_buf(recvbuf, comm);
  mpc_check_count(recvcnt, comm);
  mpc_check_count(sendcount, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Alltoall");
#endif

  if (sctk_is_inter_comm(comm)) {
    int local_size, remote_size, max_size, i;
    MPC_Status status;
    size_t sendtype_extent, recvtype_extent;
    int src, dst, rank;
    char *sendaddr, *recvaddr;

    __MPC_Comm_size(comm, &local_size);
    __MPC_Comm_rank(comm, &rank, task_specific);
    __MPC_Comm_remote_size(comm, &remote_size);

    PMPC_Type_size(sendtype, &sendtype_extent);
    PMPC_Type_size(recvtype, &recvtype_extent);

    max_size = (local_size < remote_size) ? remote_size : local_size;
    for (i = 0; i < max_size; i++) {
      src = (rank - i + max_size) % max_size;
      dst = (rank + i) % max_size;

      if (src >= remote_size) {
        src = MPC_PROC_NULL;
        recvaddr = NULL;
      } else {
        recvaddr = (char *)recvbuf + src * recvcnt * recvtype_extent;
      }

      if (dst >= remote_size) {
        dst = MPC_PROC_NULL;
        sendaddr = NULL;
      } else {
        sendaddr = (char *)sendbuf + dst * sendcount * sendtype_extent;
      }
      PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, MPC_ALLTOALL_TAG,
                    recvaddr, recvcnt, recvtype, src, MPC_ALLTOALL_TAG, comm,
                    &status);
    }
  } else {
    __MPC_Comm_size(comm, &size);
    __MPC_Comm_rank(comm, &rank, task_specific);
    d_send = __MPC_Get_datatype_size(sendtype, task_specific);
    d_recv = __MPC_Get_datatype_size(recvtype, task_specific);

    for (ii = 0; ii < size; ii += bblock) {
      ss = size - ii < bblock ? size - ii : bblock;
      for (i = 0; i < ss; ++i) {
        dst = (rank + i + ii) % size;
        __MPC_Irecv(((char *)recvbuf) + (dst * recvcnt * d_recv), recvcnt,
                    recvtype, dst, MPC_ALLTOALL_TAG, comm, &requests[i],
                    task_specific);
      }
      for (i = 0; i < ss; ++i) {
        dst = (rank - i - ii + size) % size;
        __MPC_Isend(((char *)sendbuf) + (dst * sendcount * d_send), sendcount,
                    sendtype, dst, MPC_ALLTOALL_TAG, comm, &requests[i + ss],
                    task_specific);
      }
      PMPC_Waitall(2 * ss, requests, MPC_STATUSES_IGNORE);
    }
  }
  SCTK_PROFIL_END(MPC_Alltoall);
  MPC_ERROR_SUCESS();
}

int PMPC_Alltoallv(void *sendbuf, mpc_msg_count *sendcnts,
                   mpc_msg_count *sdispls, MPC_Datatype sendtype, void *recvbuf,
                   mpc_msg_count *recvcnts, mpc_msg_count *rdispls,
                   MPC_Datatype recvtype, MPC_Comm comm) {
  int i;
  int size;
  int rank;
  int bblock = 4;
  MPC_Request requests[2 * bblock * sizeof(MPC_Request)];
  int ss, ii;
  int dst;
  size_t d_send;
  size_t d_recv;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Alltoallv);
  task_specific = __MPC_get_task_specific();

  __MPC_Comm_size(comm, &size);
  __MPC_Comm_rank(comm, &rank, task_specific);
  mpc_check_buf(sendbuf, comm);
  mpc_check_type(sendtype, comm);
  mpc_check_type(recvtype, comm);
  mpc_check_buf(recvbuf, comm);

  sctk_nodebug("[%d] Start all to all, size %d", rank, size);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Alltoallv");
#endif
  if (sctk_is_inter_comm(comm)) {
    int local_size, remote_size, max_size, i;
    MPC_Status status;
    size_t sendtype_extent, recvtype_extent;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;

    __MPC_Comm_size(comm, &local_size);
    __MPC_Comm_rank(comm, &rank, task_specific);
    __MPC_Comm_remote_size(comm, &remote_size);

    PMPC_Type_size(sendtype, &sendtype_extent);
    PMPC_Type_size(recvtype, &recvtype_extent);

    max_size = (local_size < remote_size) ? remote_size : local_size;
    for (i = 0; i < max_size; i++) {
      src = (rank - i + max_size) % max_size;
      dst = (rank + i) % max_size;
      if (src >= remote_size) {
        src = MPC_PROC_NULL;
        recvaddr = NULL;
        recvcount = 0;
      } else {
        recvaddr = (char *)recvbuf + rdispls[src] * recvtype_extent;
        recvcount = recvcnts[src];
      }
      if (dst >= remote_size) {
        dst = MPC_PROC_NULL;
        sendaddr = NULL;
        sendcount = 0;
      } else {
        sendaddr = (char *)sendbuf + sdispls[dst] * sendtype_extent;
        sendcount = sendcnts[dst];
      }

      PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, 48, recvaddr, recvcount,
                    recvtype, src, 48, comm, &status);
    }
  } else {
    d_send = __MPC_Get_datatype_size(sendtype, task_specific);
    d_recv = __MPC_Get_datatype_size(recvtype, task_specific);
    for (ii = 0; ii < size; ii += bblock) {
      ss = size - ii < bblock ? size - ii : bblock;
      for (i = 0; i < ss; ++i) {
        dst = (rank + i + ii) % size;
        sctk_nodebug("[%d] Send to %d", rank, dst);
        __MPC_Irecv(((char *)recvbuf) + (rdispls[dst] * d_recv), recvcnts[dst],
                    recvtype, dst, MPC_ALLTOALLV_TAG, comm, &requests[i],
                    task_specific);
      }
      for (i = 0; i < ss; ++i) {
        dst = (rank - i - ii + size) % size;
        sctk_nodebug("[%d] Recv from %d", rank, dst);
        __MPC_Isend(((char *)sendbuf) + (sdispls[dst] * d_send), sendcnts[dst],
                    sendtype, dst, MPC_ALLTOALLV_TAG, comm, &requests[i + ss],
                    task_specific);
      }
      PMPC_Waitall(2 * ss, requests, MPC_STATUS_IGNORE);
    }
  }

  sctk_nodebug("[%d] end all to all", rank);
  SCTK_PROFIL_END(MPC_Alltoallv);
  MPC_ERROR_SUCESS();
}

int PMPC_Alltoallw(const void *sendbuf, const int sendcounts[],
                   const int sdispls[], const MPC_Datatype sendtypes[],
                   void *recvbuf, const int recvcounts[], const int rdispls[],
                   const MPC_Datatype recvtypes[], MPC_Comm comm) {
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Alltoallv);
  task_specific = __MPC_get_task_specific();

  mpc_check_buf(sendbuf, comm);
  mpc_check_buf(recvbuf, comm);

  sctk_nodebug("[%d] Start all to all, size %d", rank, size);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Alltoallv");
#endif
  if (sctk_is_inter_comm(comm)) {
    int local_size, remote_size, max_size, i;
    MPC_Status status;
    MPC_Datatype sendtype, recvtype;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;

    __MPC_Comm_size(comm, &local_size);
    __MPC_Comm_rank(comm, &rank, task_specific);
    __MPC_Comm_remote_size(comm, &remote_size);

    max_size = (local_size < remote_size) ? remote_size : local_size;
    for (i = 0; i < max_size; i++) {
      src = (rank - i + max_size) % max_size;
      dst = (rank + i) % max_size;
      if (src >= remote_size) {
        src = MPC_PROC_NULL;
        recvaddr = NULL;
        recvcount = 0;
        recvtype = MPC_DATATYPE_NULL;
      } else {
        recvaddr = (char *)recvbuf + rdispls[src];
        recvcount = recvcounts[src];
        recvtype = recvtypes[src];
      }
      if (dst >= remote_size) {
        dst = MPC_PROC_NULL;
        sendaddr = NULL;
        sendcount = 0;
        sendtype = MPC_DATATYPE_NULL;
      } else {
        sendaddr = (char *)sendbuf + sdispls[dst];
        sendcount = sendcounts[dst];
        sendtype = sendtypes[dst];
      }

      PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, MPC_ALLTOALLW_TAG,
                    recvaddr, recvcount, recvtype, src, MPC_ALLTOALLW_TAG, comm,
                    &status);
    }
  } else {
    int i, ii, ss, dst;
    int type_size, size;
    int rank, bblock = 4;
    MPC_Status status;
    MPC_Status *starray;
    MPC_Request *reqarray;
    int outstanding_requests;
    __MPC_Comm_size(comm, &size);
    __MPC_Comm_rank(comm, &rank, task_specific);

    starray = (MPC_Status *)sctk_malloc(2 * bblock * sizeof(MPC_Status));
    reqarray = (MPC_Request *)sctk_malloc(2 * bblock * sizeof(MPC_Request));

    for (ii = 0; ii < size; ii += bblock) {
      outstanding_requests = 0;
      ss = size - ii < bblock ? size - ii : bblock;

      for (i = 0; i < ss; i++) {
        dst = (rank + i + ii) % size;
        if (recvcounts[dst]) {
          type_size = __MPC_Get_datatype_size(recvtypes[dst], task_specific);
          if (type_size) {
            PMPC_Irecv((char *)recvbuf + rdispls[dst], recvcounts[dst],
                       recvtypes[dst], dst, MPC_ALLTOALLW_TAG, comm,
                       &reqarray[outstanding_requests]);
            outstanding_requests++;
          }
        }
      }

      for (i = 0; i < ss; i++) {
        dst = (rank - i - ii + size) % size;
        if (sendcounts[dst]) {
          type_size = __MPC_Get_datatype_size(sendtypes[dst], task_specific);
          if (type_size) {
            PMPC_Isend((char *)sendbuf + sdispls[dst], sendcounts[dst],
                       sendtypes[dst], dst, MPC_ALLTOALLW_TAG, comm,
                       &reqarray[outstanding_requests]);
            outstanding_requests++;
          }
        }
      }

      PMPC_Waitall(outstanding_requests, reqarray, starray);
    }
  }

  sctk_nodebug("[%d] end all to all", rank);
  SCTK_PROFIL_END(MPC_Alltoallv);
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Groups and communicators                                             */
/************************************************************************/

static inline int __MPC_Comm_group(MPC_Comm comm, MPC_Group *group) {
  int size;
  int i;
  __MPC_Comm_size(comm, &size);

  sctk_nodebug("MPC_Comm_group");

  *group = (MPC_Group)sctk_malloc(sizeof(MPC_Group_t));

  (*group)->task_nb = size;
  (*group)->task_list_in_global_ranks = (int *)sctk_malloc(size * sizeof(int));

  for (i = 0; i < size; i++) {
    (*group)->task_list_in_global_ranks[i] = sctk_get_comm_world_rank(comm, i);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_group(MPC_Comm comm, MPC_Group *group) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_group group=%p", group);
#endif
  return __MPC_Comm_group(comm, group);
}

static inline int __MPC_Comm_remote_group(MPC_Comm comm, MPC_Group *group) {
  int size;
  int i;
  __MPC_Comm_remote_size(comm, &size);
  sctk_nodebug("MPC_Comm_group");

  *group = (MPC_Group)sctk_malloc(sizeof(MPC_Group_t));

  (*group)->task_nb = size;
  (*group)->task_list_in_global_ranks = (int *)sctk_malloc(size * sizeof(int));

  for (i = 0; i < size; i++) {
    (*group)->task_list_in_global_ranks[i] =
        sctk_get_remote_comm_world_rank(comm, i);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_remote_group(MPC_Comm comm, MPC_Group *group) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_remote_group group=%p", group);
#endif
  return __MPC_Comm_remote_group(comm, group);
}

static inline int __MPC_Group_free(MPC_Group *group) {
  if (*group != MPC_GROUP_NULL) {
    sctk_free((*group)->task_list_in_global_ranks);
    sctk_free(*group);
    *group = MPC_GROUP_NULL;
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Convert_to_intercomm(MPC_Comm comm, MPC_Group group) {
  not_implemented();
  MPC_ERROR_SUCESS();
}

int PMPC_Group_free(MPC_Group *group) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Group_free group=%p", group);
#endif
  return __MPC_Group_free(group);
}

int PMPC_Group_incl(MPC_Group group, int n, int *ranks, MPC_Group *newgroup) {
  int i;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Group_incl");
#endif

  (*newgroup) = (MPC_Group)sctk_malloc(sizeof(MPC_Group_t));
  assume((*newgroup) != NULL);

  (*newgroup)->task_nb = n;
  (*newgroup)->task_list_in_global_ranks = (int *)sctk_malloc(n * sizeof(int));
  assume(((*newgroup)->task_list_in_global_ranks) != NULL);

  for (i = 0; i < n; i++) {
    (*newgroup)->task_list_in_global_ranks[i] =
        group->task_list_in_global_ranks[ranks[i]];
    sctk_nodebug("%d", group->task_list_in_global_ranks[ranks[i]]);
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Group_difference(MPC_Group group1, MPC_Group group2,
                          MPC_Group *newgroup) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Group_difference");
#endif
  int size;
  int i, j, k;
  int not_in;

  for (i = 0; i < group1->task_nb; i++)
    sctk_nodebug("group1[%d] = %d", i, group1->task_list_in_global_ranks[i]);
  for (i = 0; i < group2->task_nb; i++)
    sctk_nodebug("group2[%d] = %d", i, group2->task_list_in_global_ranks[i]);
  /* get the size of newgroup */
  size = 0;
  for (i = 0; i < group1->task_nb; i++) {
    not_in = 1;
    for (j = 0; j < group2->task_nb; j++) {
      if (group1->task_list_in_global_ranks[i] ==
          group2->task_list_in_global_ranks[j])
        not_in = 0;
    }
    if (not_in)
      size++;
  }

  /* allocation */
  *newgroup = (MPC_Group)sctk_malloc(sizeof(MPC_Group_t));

  (*newgroup)->task_nb = size;
  if (size == 0) {
    MPC_ERROR_SUCESS();
  }

  (*newgroup)->task_nb = size;
  (*newgroup)->task_list_in_global_ranks =
      (int *)sctk_malloc(size * sizeof(int));

  /* fill the newgroup */
  k = 0;
  for (i = 0; i < group1->task_nb; i++) {
    not_in = 1;
    for (j = 0; j < group2->task_nb; j++) {
      if (group1->task_list_in_global_ranks[i] ==
          group2->task_list_in_global_ranks[j])
        not_in = 0;
    }
    if (not_in) {
      (*newgroup)->task_list_in_global_ranks[k] =
          group1->task_list_in_global_ranks[i];
      k++;
    }
  }

  MPC_ERROR_SUCESS();
}

static inline int __MPC_Comm_create_from_intercomm(MPC_Comm comm,
                                                   MPC_Group group,
                                                   MPC_Comm *comm_out,
                                                   int is_inter_comm) {
  int rank;
  int i;
  int present = 0;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);
  __MPC_Comm_rank(MPC_COMM_WORLD, &rank, task_specific);
  sctk_assert(comm != MPC_COMM_NULL);

  __MPC_Barrier(comm);

  for (i = 0; i < group->task_nb; i++) {
    if (group->task_list_in_global_ranks[i] == rank) {
      sctk_nodebug("%d present", rank);
      present = 1;
      break;
    }
  }

  for (i = 0; i < group->task_nb; i++) {
    sctk_nodebug("task_list[%d] = %d", i, group->task_list_in_global_ranks[i]);
  }

  if (sctk_is_in_local_group(comm))
    (*comm_out) = sctk_create_communicator_from_intercomm(
        comm, group->task_nb, group->task_list_in_global_ranks, 1);
  else
    (*comm_out) = sctk_create_communicator_from_intercomm(
        comm, group->task_nb, group->task_list_in_global_ranks, 0);
  sctk_nodebug("\tnew comm from intercomm %d", *comm_out);
  __MPC_Barrier(comm);

  if (present == 1) {
    sctk_nodebug("from intercomm barrier comm %d", *comm_out);
    __MPC_Barrier(*comm_out);
    sctk_nodebug("sortie barrier");
    sctk_thread_createnewspecific_mpc_per_comm(task_specific, *comm_out, comm);
  }
  sctk_nodebug("comm %d created from intercomm %d", *comm_out, comm);
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_create_from_intercomm(MPC_Comm comm, MPC_Group group,
                                    MPC_Comm *comm_out) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_create_from_intercomm comm_out=%p", comm_out);
#endif
  return __MPC_Comm_create_from_intercomm(comm, group, comm_out, 0);
}

static inline int __MPC_Comm_create(MPC_Comm comm, MPC_Group group,
                                    MPC_Comm *comm_out, int is_inter_comm) {
  int nb;
  int grank, rank;
  int i;
  int present = 0;
  MPC_Status status;
  MPC_Comm intra_comm;
  int remote_leader;
  int rleader;
  int local_leader = 0;
  int tag = 729;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);
  __MPC_Comm_rank(MPC_COMM_WORLD, &rank, task_specific);
  __MPC_Comm_rank(comm, &grank, task_specific);
  sctk_assert(comm != MPC_COMM_NULL);
  __MPC_Barrier(comm);

  for (i = 0; i < group->task_nb; i++) {
    if (group->task_list_in_global_ranks[i] == rank) {
      present = 1;
      break;
    }
  }

  if (is_inter_comm) {
    rleader = sctk_get_remote_leader(comm);
    /* get remote_leader */
    if (grank == local_leader) {
      int tmp = group->task_list_in_global_ranks[local_leader];
      PMPC_Sendrecv(&tmp, 1, MPC_INT, rleader, tag, &remote_leader, 1, MPC_INT,
                    rleader, tag, sctk_get_peer_comm(comm), &status);
    }
    PMPC_Bcast(&remote_leader, 1, MPC_INT, local_leader,
               sctk_get_local_comm_id(comm));

    /* create local_comm */
    __MPC_Comm_create(sctk_get_local_comm_id(comm), group, &intra_comm, 0);

    __MPC_Barrier(comm);

    if (intra_comm != MPC_COMM_NULL) {
      /* create intercomm */
      (*comm_out) = sctk_create_intercommunicator_from_intercommunicator(
          comm, group->task_nb, group->task_list_in_global_ranks, remote_leader,
          intra_comm);
      sctk_nodebug("\trank %d: new intercomm -> %d", rank, *comm_out);
    } else
      (*comm_out) = MPC_COMM_NULL;
  } else {
    (*comm_out) = sctk_create_communicator(comm, group->task_nb,
                                           group->task_list_in_global_ranks);
    sctk_nodebug("\trank %d: new intracomm -> %d", rank, *comm_out);
  }

  __MPC_Barrier(comm);

  if (present == 1) {
    __MPC_Barrier(*comm_out);
    sctk_thread_createnewspecific_mpc_per_comm(task_specific, *comm_out, comm);
  }
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_create(MPC_Comm comm, MPC_Group group, MPC_Comm *comm_out) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_create comm_out=%p", comm_out);
#endif
  return __MPC_Comm_create(comm, group, comm_out, sctk_is_inter_comm(comm));
}

static inline int __MPC_Intercomm_create(MPC_Comm local_comm, int local_leader,
                                         MPC_Comm peer_comm, int remote_leader,
                                         int tag, MPC_Comm *newintercomm) {
  int rank, grank;
  int i;
  int present = 0;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(local_comm, local_comm);
  mpc_check_comm(peer_comm, peer_comm);
  int *task_list;
  int *remote_task_list;
  int size;
  int other_leader;
  int first = 0;
  MPC_Status status;
  __MPC_Comm_rank(MPC_COMM_WORLD, &rank, task_specific);
  __MPC_Comm_rank(local_comm, &grank, task_specific);
  __MPC_Comm_size(local_comm, &size);
  sctk_assert(local_comm != MPC_COMM_NULL);

  task_list = __MPC_Comm_task_list(local_comm, size);

  if (grank == local_leader) {
    PMPC_Sendrecv(&remote_leader, 1, MPC_INT, remote_leader, tag, &other_leader,
                  1, MPC_INT, remote_leader, tag, peer_comm, &status);
    if (other_leader < remote_leader)
      first = 1;
    else
      first = 0;
  }

  PMPC_Bcast(&first, 1, MPC_INT, local_leader, local_comm);

  (*newintercomm) = sctk_create_intercommunicator(
      local_comm, local_leader, peer_comm, remote_leader, tag, first);
  sctk_nodebug("new intercomm %d", *newintercomm);

  __MPC_Barrier(local_comm);

  for (i = 0; i < size; i++) {
    if (task_list[i] == rank) {
      present = 1;
      break;
    }
  }
  if (present) {
    sctk_thread_createspecific_mpc_per_comm_from_existing(
        task_specific, *newintercomm, local_comm);
    __MPC_Barrier(local_comm);
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Intercomm_create(MPC_Comm local_comm, int local_leader,
                          MPC_Comm peer_comm, int remote_leader, int tag,
                          MPC_Comm *newintercomm) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Intercomm_create comm_out=%p", comm_out);
#endif
  return __MPC_Intercomm_create(local_comm, local_leader, peer_comm,
                                remote_leader, tag, newintercomm);
}

int PMPC_Comm_create_list(MPC_Comm comm, int *list, int nb_elem,
                          MPC_Comm *comm_out) {
  MPC_Group_t group;
  int i;

  group.task_nb = nb_elem;
  group.task_list_in_global_ranks = sctk_malloc(nb_elem * sizeof(int));

  for (i = 0; i < nb_elem; i++) {
    group.task_list_in_global_ranks[i] =
        sctk_get_comm_world_rank(comm, list[i]);
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_create_list nb_elem=%d comm_out=%p", nb_elem,
                comm_out);
#endif
  __MPC_Comm_create(comm, &group, comm_out, 0);

  sctk_free(group.task_list_in_global_ranks);

  MPC_ERROR_SUCESS();
}

static inline int __MPC_Comm_free(MPC_Comm *comm) {
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  if (*comm == MPC_COMM_WORLD) {
    MPC_ERROR_SUCESS();
  }

  MPC_Comm old_comm = *comm;
  *comm = MPC_COMM_NULL;
  INFO("Comm free disabled")
  MPC_ERROR_SUCESS();
  sctk_nodebug("Comm free %d", old_comm);
  mpc_check_comm(old_comm, MPC_COMM_WORLD);
  sctk_assert(old_comm != MPC_COMM_NULL);
  __MPC_Barrier(old_comm);
  sctk_nodebug("Delete Comm %d", old_comm);
  sctk_delete_communicator(old_comm);
  sctk_nodebug("Comm free done %d", old_comm);
  sctk_thread_removespecific_mpc_per_comm(task_specific, old_comm);

  *comm = MPC_COMM_NULL;
  sctk_nodebug("COMM FREE DONE");
  MPC_ERROR_SUCESS();
}

int PMPC_Comm_free(MPC_Comm *comm) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(*comm, "MPC_Comm_free");
#endif
  return __MPC_Comm_free(comm);
}

int PMPC_Comm_dup(MPC_Comm comm, MPC_Comm *comm_out) {
  sctk_nodebug("duplicate comm %d", comm);
  sctk_task_specific_t *task_specific;
  MPC_Group group;
  int rank;
  mpc_check_comm(comm, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Comm_dup out_comm=%p", comm_out);
#endif

  task_specific = __MPC_get_task_specific();
  __MPC_Comm_rank(comm, &rank, task_specific);
  *comm_out = sctk_duplicate_communicator(comm, sctk_is_inter_comm(comm), rank);
  if (sctk_is_inter_comm(comm))
    sctk_nodebug("intercomm %d duplicate --> newintercomm %d", comm, *comm_out);
  else
    sctk_nodebug("comm %d duplicate --> newcomm %d", comm, *comm_out);

  sctk_thread_createspecific_mpc_per_comm_from_existing_dup(task_specific,
                                                            *comm_out, comm);
  MPC_ERROR_SUCESS();
}

typedef struct {
  int color;
  int key;
  int rank;
} mpc_comm_split_t;

int PMPC_Comm_split(MPC_Comm comm, int color, int key, MPC_Comm *comm_out) {
  mpc_comm_split_t *tab;
  mpc_comm_split_t tab_this;
  mpc_comm_split_t tab_tmp;
  int size;
  int group_size;
  int i, j, k;
  int tmp_color;
  int color_number;
  int *color_tab;
  int rank;
  MPC_Group_t group;

  MPC_Comm comm_res = MPC_COMM_NULL;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific();

  mpc_check_comm(comm, comm);

  sctk_nodebug("MPC_Comm_split color=%d key=%d out_comm=%p", color, key,
               comm_out);

  __MPC_Comm_size(comm, &size);
  __MPC_Comm_rank(comm, &rank, task_specific);
  sctk_nodebug("rank %d : PMPC_Comm_split with comm %d, color %d and key %d",
               rank, comm, color, key);
  tab = (mpc_comm_split_t *)sctk_malloc(size * sizeof(mpc_comm_split_t));
  color_tab = (int *)sctk_malloc(size * sizeof(int));

  tab_this.rank = rank;

  tab_this.color = color;
  tab_this.key = key;
  sctk_nodebug("Allgather...");
  __MPC_Allgather(&tab_this, sizeof(mpc_comm_split_t), MPC_CHAR, tab,
                  sizeof(mpc_comm_split_t), MPC_CHAR, comm, task_specific);

  sctk_nodebug("done");
  /*Sort the new tab */
  for (i = 0; i < size; i++) {
    for (j = 0; j < size - 1; j++) {
      if (tab[j].color > tab[j + 1].color) {
        tab_tmp = tab[j + 1];
        tab[j + 1] = tab[j];
        tab[j] = tab_tmp;
      } else {
        if ((tab[j].color == tab[j + 1].color) &&
            (tab[j].key > tab[j + 1].key)) {
          tab_tmp = tab[j + 1];
          tab[j + 1] = tab[j];
          tab[j] = tab_tmp;
        }
      }
    }
  }

  color_number = 0;
  for (i = 0; i < size; i++) {
    for (j = 0; j < color_number; j++) {
      if (color_tab[j] == tab[i].color) {
        break;
      }
    }
    if (j == color_number) {
      color_tab[j] = tab[i].color;
      color_number++;
    }
    sctk_nodebug("rank %d color %d", i, tab[i].color, tab[i].rank);
  }

  sctk_nodebug("%d colors", color_number);

  /*We need on comm_create per color */
  for (k = 0; k < color_number; k++) {
    tmp_color = color_tab[k];
    if (tmp_color != MPC_UNDEFINED) {
      group_size = 0;
      for (i = 0; i < size; i++) {
        if (tab[i].color == tmp_color) {
          group_size++;
        }
      }

      group.task_nb = group_size;
      group.task_list_in_global_ranks =
          (int *)sctk_malloc(group_size * sizeof(int));

      j = 0;
      for (i = 0; i < size; i++) {
        if (tab[i].color == tmp_color) {
          group.task_list_in_global_ranks[j] =
              sctk_get_comm_world_rank(comm, tab[i].rank);
          sctk_nodebug("Thread %d color (%d) size %d on %d rank %d", tmp_color,
                       k, j, group_size, tab[i].rank);
          j++;
        }
      }

      __MPC_Comm_create(comm, &group, comm_out, sctk_is_inter_comm(comm));
      if (*comm_out != MPC_COMM_NULL) {
        comm_res = *comm_out;
        sctk_nodebug("Split done -> new %s %d",
                     (sctk_is_inter_comm(comm)) ? "intercomm" : "intracom",
                     comm_res);
      }

      sctk_free(group.task_list_in_global_ranks);
      sctk_nodebug("Split color %d done", tmp_color);
    }
  }

  sctk_free(color_tab);
  sctk_free(tab);
  sctk_nodebug("Split done");
  *comm_out = comm_res;
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/*  Error handling                                                      */
/************************************************************************/

int PMPC_Errhandler_create(MPC_Handler_function *function,
                           MPC_Errhandler *errhandler) {
  sctk_errhandler_register((sctk_generic_handler)function,
                           (sctk_errhandler_t *)errhandler);
  MPC_ERROR_SUCESS();
}

int PMPC_Errhandler_set(MPC_Comm comm, MPC_Errhandler errhandler) {
  int ret = sctk_handle_set_errhandler((sctk_handle)comm, SCTK_HANDLE_COMM,
                                       (sctk_errhandler_t)errhandler);
  MPC_ERROR_SUCESS();
}

int PMPC_Errhandler_get(MPC_Comm comm, MPC_Errhandler *errhandler) {
  *errhandler = (MPC_Errhandler)sctk_handle_get_errhandler((sctk_handle)comm,
                                                           SCTK_HANDLE_COMM);
  MPC_ERROR_SUCESS();
}

int PMPC_Errhandler_free(MPC_Errhandler *errhandler) {
  sctk_errhandler_free(*errhandler);
  *errhandler = (MPC_Errhandler)MPC_ERRHANDLER_NULL;
  MPC_ERROR_SUCESS();
}

#define MPC_Error_string_convert(code, msg)                                    \
  case code:                                                                   \
    sprintf(str, msg);                                                         \
    break

int PMPC_Error_string(int code, char *str, int *len) {
  size_t lngt;
  str[0] = '\0';
  switch (code) {
    MPC_Error_string_convert(MPC_ERR_BUFFER, "Invalid buffer pointer");
    MPC_Error_string_convert(MPC_ERR_COUNT, "Invalid count argument");
    MPC_Error_string_convert(MPC_ERR_TYPE, "Invalid datatype argument");
    MPC_Error_string_convert(MPC_ERR_TAG, "Invalid tag argument");
    MPC_Error_string_convert(MPC_ERR_COMM, "Invalid communicator");
    MPC_Error_string_convert(MPC_ERR_RANK, "Invalid rank");
    MPC_Error_string_convert(MPC_ERR_ROOT, "Invalid root");
    MPC_Error_string_convert(MPC_ERR_TRUNCATE, "Message truncated on receive");
    MPC_Error_string_convert(MPC_ERR_GROUP, "Invalid group");
    MPC_Error_string_convert(MPC_ERR_OP, "Invalid operation");
    MPC_Error_string_convert(MPC_ERR_REQUEST, "Invalid mpc_request handle");
    MPC_Error_string_convert(MPC_ERR_TOPOLOGY, "Invalid topology");
    MPC_Error_string_convert(MPC_ERR_DIMS, "Invalid dimension argument");
    MPC_Error_string_convert(MPC_ERR_ARG, "Invalid argument");
    MPC_Error_string_convert(MPC_ERR_OTHER, "Other error; use Error_string");
    MPC_Error_string_convert(MPC_ERR_UNKNOWN, "Unknown error");
    MPC_Error_string_convert(MPC_ERR_INTERN, "Internal error code");
    MPC_Error_string_convert(MPC_ERR_IN_STATUS,
                             "Look in status for error value");
    MPC_Error_string_convert(MPC_ERR_PENDING, "Pending request");
    MPC_Error_string_convert(MPC_NOT_IMPLEMENTED, "Not implemented");

    MPC_Error_string_convert(MPC_ERR_INFO, "Invalid Status argument");
    MPC_Error_string_convert(MPC_ERR_INFO_KEY,
                             "Provided info key is too large");
    MPC_Error_string_convert(MPC_ERR_INFO_VALUE,
                             "Provided info value is too large");
    MPC_Error_string_convert(MPC_ERR_INFO_NOKEY,
                             "Could not locate a value with this key");

  default:
    sctk_warning("%d error code unknown", code);
  }
  lngt = strlen(str);
  *len = (int)lngt;
  MPC_ERROR_SUCESS();
}

int PMPC_Error_class(int errorcode, int *errorclass) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Error_class errorcode=%d errorclass=%p",
                errorcode, errorclass);
#endif
  *errorclass = errorcode;
  MPC_ERROR_SUCESS();
}

static volatile int error_init_done = 0;

int __MPC_Error_init() {
  if (error_init_done == 0) {
    error_init_done = 1;

    sctk_errhandler_register_on_slot((sctk_generic_handler)PMPC_Default_error,
                                     MPC_ERRHANDLER_NULL);
    sctk_errhandler_register_on_slot((sctk_generic_handler)PMPC_Return_error,
                                     MPC_ERRORS_RETURN);
    sctk_errhandler_register_on_slot((sctk_generic_handler)PMPC_Abort_error,
                                     MPC_ERRORS_ARE_FATAL);

    PMPC_Errhandler_set(MPC_COMM_WORLD, MPC_ERRORS_ARE_FATAL);
    PMPC_Errhandler_set(MPC_COMM_SELF, MPC_ERRORS_ARE_FATAL);
  }

  MPC_ERROR_SUCESS();
}

/*Timing*/
double PMPC_Wtime() {
  double res;
  SCTK_PROFIL_START(MPC_Wtime);
#if SCTK_WTIME_USE_GETTIMEOFDAY
  struct timeval tp;

  gettimeofday(&tp, NULL);
  res = (double)tp.tv_usec + (double)tp.tv_sec * (double)1000000;
  res = res / (double)1000000;

#else
  res = sctk_atomics_get_timestamp_tsc();
#endif
  sctk_nodebug("Wtime = %f", res);
  SCTK_PROFIL_END(MPC_Wtime);
  return res;
}

double PMPC_Wtick() {
  /* sctk_warning ("Always return 0"); */
  return 10e-6;
}

/*Requests*/
int PMPC_Request_free(MPC_Request *request) {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Request_free req=%p", request);
#endif

  int ret = MPC_SUCCESS;

  sctk_nodebug("wait for message");
  /* Firstly wait the message before freeing */
  sctk_mpc_wait_message(request);

  if (request->request_type == REQUEST_GENERALIZED) {
    ret = (request->free_fn)(request->extra_state);
  }

  *request = mpc_request_null;
  return ret;
}

int PMPC_Proceed() {
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Proceed");
#endif

  sctk_thread_yield();
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Pack managment                                                       */
/************************************************************************/

int PMPC_Open_pack(MPC_Request *request) {
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Open_pack);
  task_specific = __MPC_get_task_specific();
  if (request == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Open_pack req=%p", request);
#endif

  __MPC_Comm_rank(MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header(src, SCTK_MESSAGE_PACK_UNDEFINED);

  sctk_mpc_register_message_in_request(request, msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END(MPC_Open_pack);
  MPC_ERROR_SUCESS();
}

int PMPC_Default_pack(mpc_msg_count count, mpc_pack_indexes_t *begins,
                      mpc_pack_indexes_t *ends, MPC_Request *request) {
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Default_pack);
  task_specific = __MPC_get_task_specific();
  if (request == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Default_pack count=%lu req=%p", count,
                request);
#endif
  /*   sctk_mpc_init_pack_std(request,count,begins,ends); */

  __MPC_Comm_rank(MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header(src, SCTK_MESSAGE_PACK);
  msg->tail.default_pack.std.count = count;
  msg->tail.default_pack.std.begins = begins;
  msg->tail.default_pack.std.ends = ends;

  sctk_mpc_register_message_in_request(request, msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END(MPC_Default_pack);
  MPC_ERROR_SUCESS();
}

int PMPC_Default_pack_absolute(mpc_msg_count count,
                               mpc_pack_absolute_indexes_t *begins,
                               mpc_pack_absolute_indexes_t *ends,
                               MPC_Request *request) {
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Default_pack);
  task_specific = __MPC_get_task_specific();
  if (request == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Default_pack count=%lu req=%p", count,
                request);
#endif
  /*   sctk_mpc_init_pack_absolute(request,count,begins,ends); */

  __MPC_Comm_rank(MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header(src, SCTK_MESSAGE_PACK_ABSOLUTE);
  msg->tail.default_pack.absolute.count = count;
  msg->tail.default_pack.absolute.begins = begins;
  msg->tail.default_pack.absolute.ends = ends;

  sctk_mpc_register_message_in_request(request, msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END(MPC_Default_pack);
  MPC_ERROR_SUCESS();
}

static inline int __MPC_Add_pack(void *buf, mpc_msg_count count,
                                 mpc_pack_indexes_t *begins,
                                 mpc_pack_indexes_t *ends,
                                 MPC_Datatype datatype, MPC_Request *request,
                                 sctk_task_specific_t *task_specific) {
  sctk_thread_ptp_message_t *msg;
  int i;
  size_t data_size;
  size_t total = 0;

  if (request == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
  if (begins == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
  }
  if (ends == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
  }
  if (count == 0) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
  }

  data_size = __MPC_Get_datatype_size(datatype, task_specific);

  msg = sctk_mpc_get_message_in_request(request);

  sctk_add_pack_in_message(msg, buf, count, data_size, begins, ends);

  sctk_mpc_register_message_in_request(request, msg);

  /*Compute message size */
  for (i = 0; i < count; i++) {
    total += ends[i] - begins[i] + 1;
  }
  sctk_mpc_add_to_message_size(request, total * data_size);

  MPC_ERROR_SUCESS();
}

static inline int __MPC_Add_pack_absolute(void *buf, mpc_msg_count count,
                                          mpc_pack_absolute_indexes_t *begins,
                                          mpc_pack_absolute_indexes_t *ends,
                                          MPC_Datatype datatype,
                                          MPC_Request *request,
                                          sctk_task_specific_t *task_specific) {
  sctk_thread_ptp_message_t *msg;
  int i;
  size_t data_size;
  size_t total = 0;

  data_size = __MPC_Get_datatype_size(datatype, task_specific);

  sctk_nodebug("TYPE numer %d size %lu, count = %d", datatype, data_size,
               count);

  if (request == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
  if (begins == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
  }
  if (ends == NULL) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
  }

  msg = sctk_mpc_get_message_in_request(request);

  sctk_add_pack_in_message_absolute(msg, buf, count, data_size, begins, ends);

  sctk_mpc_register_message_in_request(request, msg);

  /*Compute message size */
  for (i = 0; i < count; i++) {
    total += ends[i] - begins[i] + 1;
  }

  sctk_mpc_add_to_message_size(request, total * data_size);

  MPC_ERROR_SUCESS();
}

int PMPC_Add_pack_absolute(void *buf, mpc_msg_count count,
                           mpc_pack_absolute_indexes_t *begins,
                           mpc_pack_absolute_indexes_t *ends,
                           MPC_Datatype datatype, MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Add_pack_absolute);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD,
                "MPC_Add_pack ptr=%p type=%d nb_item=%lu req=%p", buf, datatype,
                count, request);
#endif

  /*   assume (buf == NULL); */

  res = __MPC_Add_pack_absolute(buf, count, begins, ends, datatype, request,
                                task_specific);
  SCTK_PROFIL_END(MPC_Add_pack_absolute);
  return res;
}

int PMPC_Add_pack(void *buf, mpc_msg_count count, mpc_pack_indexes_t *begins,
                  mpc_pack_indexes_t *ends, MPC_Datatype datatype,
                  MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Add_pack);
  task_specific = __MPC_get_task_specific();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD,
                "MPC_Add_pack ptr=%p type=%d nb_item=%lu req=%p", buf, datatype,
                count, request);
#endif
  res = __MPC_Add_pack(buf, count, begins, ends, datatype, request,
                       task_specific);
  SCTK_PROFIL_END(MPC_Add_pack);
  return res;
}

int PMPC_Add_pack_default(void *buf, MPC_Datatype datatype,
                          MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Add_pack_default);
  task_specific = __MPC_get_task_specific();
  if (request == NULL) {
    SCTK_PROFIL_END(MPC_Add_pack_default);
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Add_pack_default ptr=%p type=%d req=%p",
                buf, datatype, request);
#endif

  res = __MPC_Add_pack(buf, request->msg->tail.default_pack.std.count,
                       request->msg->tail.default_pack.std.begins,
                       request->msg->tail.default_pack.std.ends, datatype,
                       request, task_specific);

  SCTK_PROFIL_END(MPC_Add_pack_default);
  return res;
}

int PMPC_Add_pack_default_absolute(void *buf, MPC_Datatype datatype,
                                   MPC_Request *request) {
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Add_pack_default);
  task_specific = __MPC_get_task_specific();
  if (request == NULL) {
    SCTK_PROFIL_END(MPC_Add_pack_default);
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(MPC_COMM_WORLD, "MPC_Add_pack_default ptr=%p type=%d req=%p",
                buf, datatype, request);
#endif

  res = __MPC_Add_pack_absolute(buf,
                                request->msg->tail.default_pack.absolute.count,
                                request->msg->tail.default_pack.absolute.begins,
                                request->msg->tail.default_pack.absolute.ends,
                                datatype, request, task_specific);

  SCTK_PROFIL_END(MPC_Add_pack_default);
  return res;
}

int PMPC_Isend_pack(int dest, int tag, MPC_Comm comm, MPC_Request *request) {
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Isend_pack);
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);

  __MPC_Comm_rank_size(comm, &src, &size, task_specific);

  if (dest == MPC_PROC_NULL) {
    sctk_mpc_init_request(request, comm, src, REQUEST_SEND);
    MPC_ERROR_SUCESS();
  }
  if (request == NULL) {
    MPC_ERROR_REPORT(comm, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Isend_pack dest=%d tag=%d req=%p", dest, tag,
                request);
#endif
  msg = sctk_mpc_get_message_in_request(request);

  //~ if(sctk_is_inter_comm(comm))
  //~ {
  //~ int remote_size;
  //~ PMPC_Comm_remote_size(comm, &remote_size);
  //~ mpc_check_msg_inter(src, dest, tag, comm, size, remote_size);
  //~ }
  //~ else
  //~ {
  //~ mpc_check_msg (src, dest, tag, comm, size);
  //~ }

  sctk_mpc_set_header_in_message(msg, tag, comm, src, dest, request,
                                 sctk_mpc_get_message_size(request),
                                 SCTK_P2P_MESSAGE, MPC_PACKED, REQUEST_SEND);
  sctk_send_message(msg);
  SCTK_PROFIL_END(MPC_Isend_pack);
  MPC_ERROR_SUCESS();
}

int PMPC_Irecv_pack(int source, int tag, MPC_Comm comm, MPC_Request *request) {
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START(MPC_Irecv_pack);
  task_specific = __MPC_get_task_specific();
  mpc_check_comm(comm, comm);

  __MPC_Comm_rank_size(comm, &src, &size, task_specific);

  if (source == MPC_PROC_NULL) {
    sctk_mpc_init_request(request, comm, src, REQUEST_RECV);
    MPC_ERROR_SUCESS();
  }

  if (request == NULL) {
    MPC_ERROR_REPORT(comm, MPC_ERR_REQUEST, "");
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug(comm, "MPC_Irecv_pack source=%d tag=%d req=%p", source, tag,
                request);
#endif

  msg = sctk_mpc_get_message_in_request(request);

  if (source != MPC_ANY_SOURCE) {
    //~ if(sctk_is_inter_comm(comm))
    //~ {
    //~ int remote_size;
    //~ PMPC_Comm_remote_size(comm, &remote_size);
    //~ mpc_check_msg_inter(src, source, tag, comm, size, remote_size);
    //~ }
    //~ else
    //~ {
    //~ mpc_check_msg (src, source, tag, comm, size);
    //~ }
  }

  sctk_mpc_set_header_in_message(msg, tag, comm, source, src, request,
                                 sctk_mpc_get_message_size(request),
                                 SCTK_P2P_MESSAGE, MPC_PACKED, REQUEST_RECV);

  sctk_recv_message(
      msg,
      task_specific->my_ptp_internal[comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER],
      0);
  SCTK_PROFIL_END(MPC_Irecv_pack);
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* MPC main                                                             */
/************************************************************************/

int PMPC_Main(int argc, char **argv) { return sctk_launch_main(argc, argv); }

int PMPC_User_Main(int argc, char **argv) {
#ifndef HAVE_ENVIRON_VAR
  return mpc_user_main(argc, argv);
#else
  return mpc_user_main(argc, argv, environ);
#endif
}

/************************************************************************/
/* MPI_Info management                                                  */
/************************************************************************/

/* Utilities functions for this section are located in mpc_info.[ch] */

int PMPC_Info_create(MPC_Info *info) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* First set to NULL */
  *info = MPC_INFO_NULL;

  /* Create a new entry */
  int new_id = MPC_Info_factory_create(&task_specific->info_fact);

  /* We did not get a new entry */
  if (new_id < 0) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "Failled to allocate new MPI_Info");
  }

  /* Then set the new ID */
  *info = new_id;

  /* All clear */
  MPC_ERROR_SUCESS();
}

int PMPC_Info_free(MPC_Info *info) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Try to delete from factory */
  int ret = MPC_Info_factory_delete(&task_specific->info_fact, (int)*info);

  if (ret) {
    /* Failed to delete no such info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to delete MPI_Info");
  } else {
    /* Delete was successful */
    *info = MPC_INFO_NULL;
    MPC_ERROR_SUCESS();
  }
}

int PMPC_Info_set(MPC_Info info, const char *key, const char *value) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO, "Failled to get MPI_Info");
  }

  /* Check for lenght boundaries */
  int keylen = strlen(key);
  int valuelen = strlen(value);

  if (MPC_MAX_INFO_KEY <= keylen)
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_KEY, "");

  if (MPC_MAX_INFO_VAL <= valuelen)
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_VALUE, "");

  /* Now set the key */
  MPC_Info_cell_set(cell, (char *)key, (char *)value, 1);

  MPC_ERROR_SUCESS();
}

int PMPC_Info_delete(MPC_Info info, const char *key) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to delete MPI_Info");
  }

  int ret = MPC_Info_cell_delete(cell, (char *)key);

  if (ret) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
                     " Failled to delete key");
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Info_get(MPC_Info info, const char *key, int valuelen, char *value,
                  int *flag) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to delete MPI_Info");
  }

  MPC_Info_cell_get(cell, (char *)key, value, valuelen, flag);

  MPC_ERROR_SUCESS();
}

int PMPC_Info_get_nkeys(MPC_Info info, int *nkeys) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  *nkeys = 0;

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to get MPI_Info key count");
  }

  *nkeys = MPC_Info_key_count(cell->keys);

  MPC_ERROR_SUCESS();
}

int PMPC_Info_get_nthkey(MPC_Info info, int n, char *key) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to get MPI_Info key count");
  }

  /* Thry to get the nth entry */
  struct MPC_Info_key *keyEntry = MPC_Info_key_get_nth(cell->keys, n);

  /* Not found */
  if (!keyEntry) {
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
                     "Failled to retrieve an MPI_Info's key");
  } else {
    /* Found just snprintf the key */
    snprintf(key, MPC_MAX_INFO_KEY, "%s", keyEntry->key);
  }

  MPC_ERROR_SUCESS();
}

int PMPC_Info_dup(MPC_Info info, MPC_Info *newinfo) {
  /* First create a new entry */
  int ret = PMPC_Info_create(newinfo);

  if (ret != MPC_SUCCESS)
    return ret;

  /* Prepare to copy keys one by one */
  int num_keys = 0;
  char keybuf[MPC_MAX_INFO_KEY];
  /* This is the buffer for the value */
  char *valbuff = sctk_malloc(MPC_MAX_INFO_VAL * sizeof(char));

  if (!valbuff) {
    perror("sctk_alloc");
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
                     "Failled to allocate temporary value buffer");
  }

  int i;

  PMPC_Info_get_nkeys(info, &num_keys);

  int flag = 0;

  /* Copy keys */
  for (i = 0; i < num_keys; i++) {
    /* Get key */
    PMPC_Info_get_nthkey(info, i, keybuf);
    /* Get value */
    PMPC_Info_get(info, keybuf, MPC_MAX_INFO_VAL, valbuff, &flag);

    if (!flag) {
      /* Shall not happen */
      sctk_free(valbuff);
      MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
                       "Could not retrieve a key which should have been there");
    }

    /* Store value at key in the newly allocated object */
    PMPC_Info_set(*newinfo, keybuf, valbuff);
  }

  sctk_free(valbuff);

  MPC_ERROR_SUCESS();
}

int PMPC_Info_get_valuelen(MPC_Info info, char *key, int *valuelen, int *flag) {
  /* Retrieve task context */
  sctk_task_specific_t *task_specific = __MPC_get_task_specific();

  /* Locate the cell */
  struct MPC_Info_cell *cell =
      MPC_Info_factory_resolve(&task_specific->info_fact, (int)info);

  if (!cell) {
    /* We failed to locate the info */
    MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
                     "Failled to get MPI_Info key count");
  }

  /* Retrieve the cell @ key */
  struct MPC_Info_key *keyEntry = MPC_Info_key_get(cell->keys, (char *)key);

  if (!keyEntry) {
    /* Nothing found, set flag */
    *flag = 0;
  } else {
    /* We found it ! set length with strlen */
    *flag = 1;
    *valuelen = strlen(keyEntry->value);
  }

  MPC_ERROR_SUCESS();
}

/********************************************************************/
/*    Network functions. These functions are not a part of the      */
/*    MPI standard.                                                 */
/********************************************************************/

/* Send a message to a process using the signalization network */
void MPC_Send_signalization_network(int dest_process, int tag, void *buff,
                                    size_t size) {
#ifdef MPC_Message_Passing
  sctk_route_messages_send(sctk_process_rank, dest_process,
                           SCTK_CONTROL_MESSAGE_USER, tag, buff, size);
#endif
}

/* Recv a message from a process using the signalization network */
void MPC_Recv_signalization_network(int src_process, int tag, void *buff,
                                    size_t size) {
#ifdef MPC_Message_Passing
  sctk_route_messages_recv(src_process, sctk_process_rank,
                           SCTK_CONTROL_MESSAGE_USER, tag, buff, size);
#endif
}
