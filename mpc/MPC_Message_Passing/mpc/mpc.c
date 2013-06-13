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
#define MPC_MAIN_FILE
#include "mpc.h"
#undef main

#include "sctk.h"
#include "mpc_reduction.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_tls.h"
#include "sctk_route.h"
#include "mpcthread.h"
#include <mpcmp.h>
#include <sys/time.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk_collective_communications.h>
#include "sctk_stdint.h"

 /*#define MPC_LOG_DEBUG*/
#ifdef MPC_LOG_DEBUG
#include <stdarg.h>
#endif

#include "mpc_common.h"
#include <mpc_internal_thread.h>
#ifdef MPC_OpenMP
#include"mpcomp_internal.h"
#endif

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#include "mpc_weak.h"
#endif

#ifdef MPC_Message_Passing

#include "sctk_low_level_comm.h"

#if MPC_USE_INFINIBAND
#include "sctk_multirail_ib.h"
#endif

#endif


 /************************************************************************/
 /*GLOBAL VARIABLES                                                      */
 /************************************************************************/
static sctk_thread_key_t sctk_task_specific;
static sctk_thread_key_t sctk_check_point_key;
static sctk_thread_key_t sctk_func_key;
static int mpc_disable_buffering = 0;

static inline void *
sctk_thread_getspecific_mpc (sctk_thread_key_t key)
{
  return sctk_thread_getspecific (key);
}

static inline void
sctk_thread_setspecific_mpc (sctk_thread_key_t key, void *tmp)
{
  sctk_thread_setspecific (key, tmp);
}


const MPC_Group_t mpc_group_empty = { 0, NULL };
const MPC_Group_t mpc_group_null = { -1, NULL };
MPC_Request mpc_request_null;

__thread struct sctk_thread_specific_s *sctk_message_passing;

inline mpc_per_communicator_t* sctk_thread_getspecific_mpc_per_comm(sctk_task_specific_t* task_specific,sctk_communicator_t comm){
  mpc_per_communicator_t *per_communicator;

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh,task_specific->per_communicator,&comm,sizeof(sctk_communicator_t),per_communicator);
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  return per_communicator;
}

static inline void sctk_thread_addspecific_mpc_per_comm(sctk_task_specific_t* task_specific,mpc_per_communicator_t* mpc_per_comm,sctk_communicator_t comm){
  mpc_per_communicator_t *per_communicator;
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh,task_specific->per_communicator,&comm,sizeof(sctk_communicator_t),per_communicator);
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

  HASH_ADD(hh,task_specific->per_communicator,key,sizeof(sctk_communicator_t),mpc_per_comm);

   /*{ 
     mpc_per_communicator_t* pair; 
     mpc_per_communicator_t* tmp; 
     HASH_ITER(hh,task_specific->per_communicator,pair,tmp){ 
       sctk_nodebug("Key after INS %d",pair->key); 
     } 
   }*/
   
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  return ;
}

static inline void sctk_thread_removespecific_mpc_per_comm(sctk_task_specific_t* task_specific,sctk_communicator_t comm){
  mpc_per_communicator_t*per_communicator;
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  
  /* { */
  /*   mpc_per_communicator_t* pair; */
  /*   mpc_per_communicator_t* tmp; */
  /*   HASH_ITER(hh,task_specific->per_communicator,pair,tmp){ */
  /*     sctk_nodebug("Before DEL Key %d",pair->key); */
  /*   } */
  /* } */

  HASH_FIND(hh,task_specific->per_communicator,&comm,sizeof(sctk_communicator_t),per_communicator);

  assume(per_communicator != NULL);
  assume(per_communicator->key == comm);

  HASH_DELETE(hh,task_specific->per_communicator,per_communicator);

  /* { */
  /*   mpc_per_communicator_t* pair; */
  /*   mpc_per_communicator_t* tmp; */
  /*   HASH_ITER(hh,task_specific->per_communicator,pair,tmp){ */
  /*     sctk_nodebug("After DEL Key %d",pair->key); */
  /*   } */
  /* } */

  sctk_nodebug("Remove %d %p (%d)",comm,per_communicator,task_specific->task_id);
  sctk_free(per_communicator);
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
}

static inline mpc_per_communicator_t* sctk_thread_createspecific_mpc_per_comm(){
  mpc_per_communicator_t* tmp;
  sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;

  tmp = sctk_malloc(sizeof(mpc_per_communicator_t));

  tmp->err_handler = (MPC_Handler_function *)PMPC_Default_error;
  tmp->err_handler_lock = lock;
  tmp->mpc_mpi_per_communicator = NULL;
  tmp->mpc_mpi_per_communicator_copy = NULL;
  tmp->mpc_mpi_per_communicator_copy_dup = NULL;
  
  sctk_nodebug("Allocate new per comm %p",tmp);

  return tmp;
}

static inline void sctk_thread_createnewspecific_mpc_per_comm(sctk_task_specific_t* task_specific,sctk_communicator_t new_comm,sctk_communicator_t old_comm){
	mpc_per_communicator_t* per_communicator;
	mpc_per_communicator_t* per_communicator_new;
	int i;
	assume(new_comm != old_comm);

	sctk_spinlock_lock(&(task_specific->per_communicator_lock));
	HASH_FIND(hh,task_specific->per_communicator,&old_comm,sizeof(sctk_communicator_t),per_communicator);
	assume(per_communicator != NULL);

	per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
	memcpy(per_communicator_new,per_communicator,sizeof(mpc_per_communicator_t));

	if(per_communicator->mpc_mpi_per_communicator_copy != NULL){
	per_communicator->mpc_mpi_per_communicator_copy(&(per_communicator_new->mpc_mpi_per_communicator),per_communicator->mpc_mpi_per_communicator);
	}

	sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
	assert(per_communicator_new->mpc_mpi_per_communicator);
	sctk_thread_addspecific_mpc_per_comm(task_specific,per_communicator_new,new_comm);
}

static inline void sctk_thread_createspecific_mpc_per_comm_from_existing(sctk_task_specific_t* task_specific,sctk_communicator_t new_comm,sctk_communicator_t old_comm){
  mpc_per_communicator_t* per_communicator;
  mpc_per_communicator_t* per_communicator_new;

  assume(new_comm != old_comm);

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh,task_specific->per_communicator,&old_comm,sizeof(sctk_communicator_t),per_communicator);
  assume(per_communicator != NULL);
  
  per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
  memcpy(per_communicator_new,per_communicator,sizeof(mpc_per_communicator_t));

  if(per_communicator->mpc_mpi_per_communicator_copy != NULL){
    per_communicator->mpc_mpi_per_communicator_copy(&(per_communicator_new->mpc_mpi_per_communicator),per_communicator->mpc_mpi_per_communicator);
  }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

  sctk_thread_addspecific_mpc_per_comm(task_specific,per_communicator_new,new_comm);
}

static inline void sctk_thread_createspecific_mpc_per_comm_from_existing_dup(sctk_task_specific_t* task_specific,sctk_communicator_t new_comm,sctk_communicator_t old_comm){
  mpc_per_communicator_t* per_communicator;
  mpc_per_communicator_t* per_communicator_new;

  assume(new_comm != old_comm);

  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_FIND(hh,task_specific->per_communicator,&old_comm,sizeof(sctk_communicator_t),per_communicator);
  assume(per_communicator != NULL);
  
  per_communicator_new = sctk_thread_createspecific_mpc_per_comm();
  memcpy(per_communicator_new,per_communicator,sizeof(mpc_per_communicator_t));

  if(per_communicator->mpc_mpi_per_communicator_copy != NULL){
    per_communicator->mpc_mpi_per_communicator_copy_dup(&(per_communicator_new->mpc_mpi_per_communicator),per_communicator->mpc_mpi_per_communicator);
  }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));

  sctk_thread_addspecific_mpc_per_comm(task_specific,per_communicator_new,new_comm);
}

/************************************************************************/
/*ACCESSORS                                                             */
/************************************************************************/

static inline void sctk_mpc_verify_request_compatibility()
{
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
  /*     assume ((int) mpc_req.completion_flag == (int) sctk_req.completion_flag); */
}

static inline void sctk_mpc_commit_status_from_request(MPC_Request * request, MPC_Status * status){
  if (status != MPC_STATUS_IGNORE)
    {
      status->MPC_SOURCE = request->header.source;
      status->MPC_TAG = request->header.message_tag;
      status->MPC_ERROR = MPC_SUCCESS;
      status->count = request->header.msg_size;
      if (request->completion_flag == SCTK_MESSAGE_CANCELED)
	{
	  status->cancelled = 1;
	}
      else
	{
	  status->cancelled = 0;
	}
    }
}

static inline
sctk_communicator_t sctk_mpc_get_communicator_from_request(MPC_Request * request){
  return ((sctk_request_t *) request)->header.communicator;
}

static inline
void sctk_mpc_perform_messages(MPC_Request * request){
  struct sctk_perform_messages_s _wait;

  sctk_perform_messages_wait_init(&_wait, request);
  sctk_perform_messages_wait_init_request_type(&_wait);

  sctk_perform_messages(&_wait);
}

static inline int sctk_mpc_completion_flag(MPC_Request * request){
  return request->completion_flag;
}

static inline void sctk_mpc_init_request_null(){
  mpc_request_null.is_null = 1;
}

static inline void sctk_mpc_init_request (MPC_Request * request, MPC_Comm comm, int src, int request_type){
  if (request != NULL)
    {
      request->header.source = MPC_PROC_NULL;
      request->header.destination = MPC_PROC_NULL;
      request->header.message_tag = MPC_ANY_TAG;
      request->header.communicator = comm;
      request->header.msg_size = 0;
      request->completion_flag = SCTK_MESSAGE_DONE;
      request->request_type = request_type;
      request->is_null = 0;
      request->msg = NULL;
    }
}

static inline void sctk_mpc_register_message_in_request(MPC_Request * request,
							sctk_thread_ptp_message_t *msg){
  request->msg = msg;
}

static inline sctk_thread_ptp_message_t* sctk_mpc_get_message_in_request(MPC_Request * request){
  return request->msg;
}

static inline void sctk_mpc_init_message_size(MPC_Request * request){
  request->msg->body.header.msg_size = 0;
}

static inline void sctk_mpc_add_to_message_size(MPC_Request * request, size_t size){
  request->msg->body.header.msg_size += size;
}

static inline size_t sctk_mpc_get_message_size(MPC_Request * request){
  return request->msg->body.header.msg_size;
}

static inline int sctk_mpc_get_message_source(MPC_Request * request){
  return request->header.source;
}

static inline int sctk_mpc_message_is_null(MPC_Request * request){
  return request->is_null;
}

static inline void sctk_mpc_message_set_is_null(MPC_Request * request,int val){
  request->is_null = val;
}

static inline int sctk_mpc_message_get_is_null(MPC_Request * request){
  return request->is_null;
}

static inline void sctk_mpc_set_header_in_message(sctk_thread_ptp_message_t *
						  msg, const int message_tag,
						  const sctk_communicator_t
						  communicator,
						  const int source,
						  const int destination,
						  MPC_Request * request,
						  const size_t count,
						  specific_message_tag_t specific_message_tag){
  sctk_set_header_in_message(msg,message_tag,communicator,source,destination,request,count,
			     specific_message_tag);
}

static inline void sctk_mpc_wait_message (MPC_Request * request){
  if(sctk_mpc_message_is_null(request) == 0){
    sctk_wait_message(request);
  }
}

static inline void sctk_mpc_wait_all (const int task, const sctk_communicator_t com){
  sctk_wait_all(task,com);
}

static inline void sctk_mpc_cancel_message (MPC_Request * msg){
  sctk_cancel_message(msg);
}

/************************************************************************/
/*FUNCTIONS                                                             */
/************************************************************************/

void __MPC_init_thread_specific(){
#if defined(SCTK_USE_TLS)
  if(sctk_message_passing == NULL){
    int i;
    sctk_thread_specific_t* tmp;

    tmp = sctk_malloc(sizeof(sctk_thread_specific_t));
    memset(tmp,0,sizeof(sctk_thread_specific_t));
    for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++)
      {
	sctk_mpc_init_request(&(tmp->buffer.buffer[i].request),
			      MPC_COMM_NULL,MPC_PROC_NULL, REQUEST_NULL);
	sctk_mpc_init_request(&(tmp->buffer_async.buffer_async[i].request),
			      MPC_COMM_NULL,MPC_PROC_NULL, REQUEST_NULL);
      }
    tmp->buffer.buffer_rank = 0;
    tmp->buffer_async.buffer_async_rank = 0;
    sctk_message_passing = tmp;
  }
#else
  not_implemented();
#endif

}
void __MPC_delete_thread_specific(){
#if defined(SCTK_USE_TLS)
  if(sctk_message_passing != NULL){
    int i;
    sctk_thread_specific_t* tmp;
    tmp = sctk_message_passing;

    sctk_spinlock_lock (&(tmp->buffer.lock));
    for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++)
      {
	sctk_mpc_wait_message (&(tmp->buffer.buffer[i].request));
      }
    sctk_spinlock_unlock (&(tmp->buffer.lock));
    sctk_spinlock_lock (&(tmp->buffer_async.lock));
    for (i = 0; i < MAX_MPC_BUFFERED_MSG; i++)
      {
	sctk_mpc_wait_message (&
			       (tmp->buffer_async.buffer_async[i].
				request));
      }
    sctk_spinlock_unlock (&(tmp->buffer_async.lock));
    sctk_message_passing = NULL;
  }
#else
  not_implemented();
#endif
}

static inline void
__MPC_init_task_specific_t (sctk_task_specific_t * tmp)
{
  unsigned long i;
  mpc_per_communicator_t* per_comm_tmp;

  tmp->task_id = sctk_get_task_rank ();

  for (i = 0; i < sctk_user_data_types_max; i++)
    {
      tmp->user_types.user_types[i] = 0;
    }

  for (i = 0; i < sctk_user_data_types_max; i++)
    {
      tmp->user_types_struct.user_types_struct[i] = 0;
    }

  per_comm_tmp = sctk_thread_createspecific_mpc_per_comm();
  sctk_thread_addspecific_mpc_per_comm(tmp,per_comm_tmp,MPC_COMM_WORLD);
  per_comm_tmp = sctk_thread_createspecific_mpc_per_comm();
  sctk_thread_addspecific_mpc_per_comm(tmp,per_comm_tmp,MPC_COMM_SELF);

/*   tmp->keys = NULL; */
/*   tmp->requests = NULL; */
/*   tmp->buffers = NULL; */
/*   tmp->errors = NULL; */
/*   tmp->comm_type = NULL; */
/*   tmp->op = NULL; */
/*   tmp->groups = NULL; */

  __MPC_init_thread_specific();

  tmp->init_done = 0;
  tmp->finalize_done = 0;

  tmp->my_ptp_internal = sctk_get_internal_ptp(tmp->task_id);

}

static void
__MPC_init_task_specific ()
{
  sctk_task_specific_t *tmp;

  tmp =
    (sctk_task_specific_t *) sctk_thread_getspecific_mpc (sctk_task_specific);
  sctk_assert (tmp == NULL);

  tmp = (sctk_task_specific_t *) sctk_malloc (sizeof (sctk_task_specific_t));
  memset (tmp, 0, sizeof (sctk_task_specific_t));
  __MPC_init_task_specific_t (tmp);


  sctk_thread_setspecific_mpc (sctk_task_specific, tmp);
}

static void
__MPC_delete_task_specific ()
{
  sctk_task_specific_t *tmp;

  tmp =
    (sctk_task_specific_t *) sctk_thread_getspecific_mpc (sctk_task_specific);

  sctk_thread_setspecific_mpc (sctk_task_specific, NULL);

  sctk_free (tmp);
}

void
__MPC_reinit_task_specific (struct sctk_task_specific_s *tmp)
{
  sctk_thread_setspecific_mpc (sctk_task_specific, tmp);
}

struct sctk_task_specific_s *
__MPC_get_task_specific ()
{
  struct sctk_task_specific_s *tmp;
  tmp = (struct sctk_task_specific_s *)
    sctk_thread_getspecific_mpc (sctk_task_specific);
  return tmp;
}

static inline int
__MPC_ERROR_REPORT__ (MPC_Comm comm, int error, char *message, char *file,
		      int line)
{
  MPC_Comm comm_id;
  int error_id;
  MPC_Handler_function *func;
  sctk_task_specific_t *task_specific;
  mpc_per_communicator_t* tmp;
  task_specific = __MPC_get_task_specific ();
  comm_id = comm;
  tmp = sctk_thread_getspecific_mpc_per_comm(task_specific,comm_id);

  if (tmp == NULL){
    comm_id = MPC_COMM_WORLD;
    tmp = sctk_thread_getspecific_mpc_per_comm(task_specific,comm_id);
  }

  error_id = error;
  sctk_spinlock_lock (&(tmp->err_handler_lock));
  func = tmp->err_handler;
  sctk_spinlock_unlock (&(tmp->err_handler_lock));
  func (&comm_id, &error_id, message, file, line);
  return error;
}

#define MPC_ERROR_REPORT(comm, error,message) return __MPC_ERROR_REPORT__(comm, error,message,__FILE__, __LINE__)

#define MPC_ERROR_SUCESS() return MPC_SUCCESS;

#undef MPC_CREATE_INTERN_FUNC
#define MPC_CREATE_INTERN_FUNC(name)			\
  const MPC_Op MPC_##name = {MPC_##name##_func,NULL}

MPC_CREATE_INTERN_FUNC (SUM);
MPC_CREATE_INTERN_FUNC (MAX);
MPC_CREATE_INTERN_FUNC (MIN);
MPC_CREATE_INTERN_FUNC (PROD);
MPC_CREATE_INTERN_FUNC (LAND);
MPC_CREATE_INTERN_FUNC (BAND);
MPC_CREATE_INTERN_FUNC (LOR);
MPC_CREATE_INTERN_FUNC (BOR);
MPC_CREATE_INTERN_FUNC (LXOR);
MPC_CREATE_INTERN_FUNC (BXOR);
MPC_CREATE_INTERN_FUNC (MINLOC);
MPC_CREATE_INTERN_FUNC (MAXLOC);

#define mpc_check_type(datatype,comm)		\
  if (datatype >= sctk_user_data_types_max)	\
    MPC_ERROR_REPORT (comm, MPC_ERR_TYPE, "");

TODO("To optimize")
/* #define mpc_check_comm(com,comm)		\ */
/*   if(sctk_thread_getspecific_mpc_per_comm(__MPC_get_task_specific (),com) == NULL)	\ */
/*     MPC_ERROR_REPORT(comm,MPC_ERR_COMM,"") */
#define mpc_check_comm(com,comm)		\
  if(com < 0)					\
    MPC_ERROR_REPORT(comm,MPC_ERR_COMM,"")

#define mpc_check_buf(buf,comm)			\
  if((buf == NULL) && (buf != MPC_BOTTOM))	\
    MPC_ERROR_REPORT(comm,MPC_ERR_BUFFER,"")

#define mpc_check_buf_null(buf,comm)		\
  if((buf == NULL))				\
    MPC_ERROR_REPORT(comm,MPC_ERR_BUFFER,"")

#define mpc_check_count(count,comm)		\
  if(count < 0)					\
    MPC_ERROR_REPORT(comm,MPC_ERR_COUNT,"")

static inline int
__mpc_check_task__ (int task, int max_rank)
{
  return (((task < 0) && (task != -4) && (task != -2)) || (task >= max_rank))&& (task != MPC_ANY_SOURCE);
}

#define mpc_check_task(task,comm,max_rank)				\
  if(__mpc_check_task__(task,max_rank)) MPC_ERROR_REPORT(comm,MPC_ERR_RANK,"")

static inline int
__mpc_check_task_msg__ (int task, int max_rank)
{
  int res;
  
  res = (((task < 0) || (task >= max_rank)) && (task != MPC_ANY_SOURCE));
  return res;
}

#define mpc_check_task_msg_size(task,comm,msg,max_rank)			\
  if(((task < 0) || (task >= max_rank)) && (task != MPC_ANY_SOURCE))	\
    MPC_ERROR_REPORT(comm,MPC_ERR_RANK,msg)

#define mpc_check_task_msg(task,comm,msg,max_rank)	\
	if(__mpc_check_task_msg__(task,max_rank))		\
		MPC_ERROR_REPORT(comm,MPC_ERR_RANK,msg)		\

#define mpc_check_tag(tag,comm)						\
  if(!(tag >= MPC_ALLTOALLV_TAG))					\
    MPC_ERROR_REPORT(comm,MPC_ERR_TAG,"")

#define mpc_check_msg_inter(src,dest,tag,comm,comm_size,comm_remote_size)		\
	mpc_check_task_msg(src,comm," source",comm_size);		\
    mpc_check_task_msg(dest,comm," destination",comm_remote_size);	\
    mpc_check_tag(tag,comm)

#define mpc_check_msg(src,dest,tag,comm,comm_size)		\
  mpc_check_task_msg(src,comm," source",comm_size);		\
  mpc_check_task_msg(dest,comm," destination",comm_size);	\
  mpc_check_tag(tag,comm)

#define mpc_check_msg_size(src,dest,tag,comm,s)		\
  mpc_check_task_msg_size(src,comm," source",s);	\
  mpc_check_task_msg_size(dest,comm," destination",s);	\
  mpc_check_tag(tag,comm)
  
#define mpc_check_msg_size_inter(src,dest,tag,comm,s,rs)		\
  mpc_check_task_msg_size(src,comm," source",s);	\
  mpc_check_task_msg_size(dest,comm," destination",rs);	\
  mpc_check_tag(tag,comm)

static size_t mpc_common_types[sctk_user_data_types];


sctk_thread_key_t
sctk_get_check_point_key ()
{
  return sctk_check_point_key;
}

void
sctk_mpc_init_keys ()
{
  sctk_thread_key_create (&sctk_func_key, NULL);
  sctk_thread_key_create (&sctk_check_point_key, NULL);

  sctk_thread_key_create (&sctk_task_specific, NULL);
}

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

int
PMPC_Get_version (int *version, int *subversion)
{
  *version = SCTK_VERSION_MAJOR;
  *subversion = SCTK_VERSION_MINOR;
  MPC_ERROR_SUCESS ();
}

int
PMPC_Get_multithreading (char *name, int size)
{
  memcpy (name, sctk_multithreading_mode, size - 2);
  name[size - 1] = '\0';
  MPC_ERROR_SUCESS ();
}

int
PMPC_Get_networking (char *name, int size)
{
  memcpy (name, sctk_network_mode, size - 2);
  name[size - 1] = '\0';
  MPC_ERROR_SUCESS ();
}

static inline int *
__MPC_Comm_task_list (MPC_Comm comm, int size)
{
	int i;
	int *task_list = sctk_malloc(size*sizeof(int));
	for(i = 0 ; i < size ; i++)
	{
		task_list[i] = sctk_get_comm_world_rank (comm, i);
	}
	return task_list;
}

static inline int *
__MPC_Comm_task_list_remote (MPC_Comm comm, int size)
{
	int i;
	int *task_list_remote = sctk_malloc(size*sizeof(int));
	for(i = 0 ; i < size ; i++)
	{
		task_list_remote[i] = sctk_get_remote_comm_world_rank (comm, i);
	}
	return task_list_remote;
}

static inline int
__MPC_Comm_rank (MPC_Comm comm, int *rank,
		 sctk_task_specific_t * task_specific)
{
  *rank = sctk_get_rank (comm, task_specific->task_id);
  return 0;
}

static inline int
__MPC_Comm_size (MPC_Comm comm, int *size)
{
  *size = sctk_get_nb_task_total (comm);
  if(*size == -1)
	return MPC_ERR_COMM;
	
  return 0;
}

static inline int
__MPC_Comm_remote_size (MPC_Comm comm, int *size)
{
  *size = sctk_get_nb_task_remote (comm);
  if(*size == -1)
	return MPC_ERR_COMM;
  return 0;
}

static inline void
__MPC_Comm_rank_size (MPC_Comm comm, int *rank, int *size,
		      sctk_task_specific_t * task_specific)
{
  sctk_get_rank_size_total (comm, rank, size, task_specific->task_id);
}

static void MPC_Set_buffering(int val){
  mpc_disable_buffering = !(val);
}

void MPC_Hard_Check(){
  int rank;
  int size;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific ();
  __MPC_Comm_rank_size (MPC_COMM_WORLD, &rank, &size, task_specific);
  if(rank == 0){
    sctk_warning("Enable MPC_HARD_CHECKING");
  }
  MPC_Set_buffering(0);
}

#if defined(MPC_LOG_DEBUG)
static int mpc_log_debug_status = 0;
static inline void
mpc_log_debug (MPC_Comm comm, const char *fmt, ...)
{
  if (mpc_log_debug_status == 1)
    {
      va_list ap;
      char buff[4096];
      int rank;
      FILE *stream;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();

      stream = stderr;

      __MPC_Comm_rank (comm, &rank, task_specific);
      sprintf (buff, "Task %4d/%4d: %s\n", rank, (int) comm, fmt);

      va_start (ap, fmt);
      vfprintf (stream, buff, ap);
      va_end (ap);
    }
}

#endif

int __MPC_Barrier (MPC_Comm comm)
{
	mpc_check_comm (comm, comm);
	sctk_nodebug("Barrier comm %d", comm);
	if(sctk_is_inter_comm(comm))
	{
		int root = 0, buf = 0, rank;
		sctk_task_specific_t *task_specific;
		task_specific = __MPC_get_task_specific ();
		__MPC_Comm_rank (comm, &rank, task_specific);
		sctk_barrier (sctk_get_local_comm_id(comm));
		
		if(sctk_is_in_local_group(comm))
		{
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			PMPC_Bcast(&buf, 1, MPC_INT, root, comm);
			
			root = 0;
			PMPC_Bcast(&buf, 1, MPC_INT, root, comm);
		}
		else
		{
			root = 0;
			PMPC_Bcast(&buf, 1, MPC_INT, root, comm);
			
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			PMPC_Bcast(&buf, 1, MPC_INT, root, comm);
		}
	}
	else
		sctk_barrier ((sctk_communicator_t) comm);
	
	MPC_ERROR_SUCESS ();
}

/*Data types*/
static inline int
sctk_is_derived_type (MPC_Datatype data_in)
{
  if ((data_in >= sctk_user_data_types + sctk_user_data_types_max) &&
      (data_in < sctk_user_data_types + 2 * sctk_user_data_types_max))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}


int
PMPC_Type_free (MPC_Datatype * datatype_p)
{
	sctk_task_specific_t *task_specific;
	MPC_Datatype datatype;
	SCTK_PROFIL_START (MPC_Type_free);
	task_specific = __MPC_get_task_specific ();
	datatype = *datatype_p;
      
	if ((datatype == MPC_DATATYPE_NULL) || (datatype == MPC_PACKED))
	{
		SCTK_PROFIL_END (MPC_Type_free);
		MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_TYPE, "");
	}

	if (datatype < sctk_user_data_types)
	{
		SCTK_PROFIL_END (MPC_Type_free);
		return MPC_SUCCESS;
		//~ MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_TYPE, "");
	}
	else if (datatype - sctk_user_data_types < sctk_user_data_types_max)
	{
		sctk_datatype_t *other_user_types;
		sctk_spinlock_lock (&(task_specific->other_user_types.lock));
		other_user_types = task_specific->other_user_types.other_user_types;
		sctk_assert (other_user_types != NULL);
		other_user_types[datatype - sctk_user_data_types] = 0;
		sctk_spinlock_unlock (&(task_specific->other_user_types.lock));
	}
	else
	{
		sctk_nodebug("FREE datatype N° %d", datatype);
		sctk_derived_type_t **user_types;
		sctk_derived_type_t *t;
		
		sctk_assert (datatype - sctk_user_data_types - sctk_user_data_types_max < sctk_user_data_types_max);
		sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
			user_types = task_specific->user_types_struct.user_types_struct;
			sctk_assert (user_types != NULL);
			sctk_assert (user_types[datatype - sctk_user_data_types - sctk_user_data_types_max] != NULL);
			t = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max];
			t->ref_count--;
			if (t->ref_count == 0)
			{
				sctk_free (t->begins);
				sctk_free (t->ends);
				sctk_free (t);
				user_types[datatype - sctk_user_data_types - sctk_user_data_types_max] = NULL;
			}
		sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
	}

	*datatype_p = (MPC_Datatype) (-1);
	SCTK_PROFIL_END (MPC_Type_free);
	MPC_ERROR_SUCESS ();
}

static inline size_t
__MPC_Get_datatype_size (MPC_Datatype datatype,
			 sctk_task_specific_t * task_specific)
{
  if (datatype == MPC_DATATYPE_NULL)
    MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_TYPE, "");

  if (datatype == MPC_UB)
    {
      return 0;
    }
  if (datatype == MPC_LB)
    {
      return 0;
    }
  if (datatype == MPC_PACKED)
    {
      return 1;
    }
  if (datatype < sctk_user_data_types)
    {
      return mpc_common_types[datatype];
    }
  else if (datatype - sctk_user_data_types < sctk_user_data_types_max)
    {
      sctk_other_datatype_t *other_user_types;
      size_t res;
      sctk_spinlock_lock (&(task_specific->other_user_types.lock));
      other_user_types = task_specific->other_user_types.other_user_types;
      sctk_assert (other_user_types != NULL);
      res = other_user_types[datatype - sctk_user_data_types].size;
      sctk_spinlock_unlock (&(task_specific->other_user_types.lock));
      return res;
    }
  else
    {
      size_t res;
      sctk_derived_type_t **user_types;
      sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
		  sctk_nodebug ("%lu relative datatype max %d", datatype - sctk_user_data_types - sctk_user_data_types_max, sctk_user_data_types_max);
		  sctk_assert (datatype - sctk_user_data_types - sctk_user_data_types_max < sctk_user_data_types_max);
		  
		  user_types = task_specific->user_types_struct.user_types_struct;
		  
		  sctk_assert (user_types != NULL);
		  sctk_assert (user_types[datatype - sctk_user_data_types - sctk_user_data_types_max] != NULL);
		  
		  res = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->size;
		  sctk_nodebug("Datatype_size = %d", res);
      sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
      return res;
    }
}
int
PMPC_Type_size (MPC_Datatype datatype, size_t * size)
{
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Type_size);
  task_specific = __MPC_get_task_specific ();
  *size = __MPC_Get_datatype_size (datatype, task_specific);
  SCTK_PROFIL_END (MPC_Type_size);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Sizeof_datatype (MPC_Datatype * datatype, size_t size, size_t count, MPC_Datatype *data_in)
{
  int i;
  sctk_other_datatype_t *other_user_types;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Sizeof_datatype);
  *datatype = MPC_DATATYPE_NULL;
  task_specific = __MPC_get_task_specific ();
  sctk_spinlock_lock (&(task_specific->other_user_types.lock));
  other_user_types = task_specific->other_user_types.other_user_types;
  for (i = 0; i < sctk_user_data_types_max; i++)
    {
      if (other_user_types[i].id == 0)
	{
	  *datatype = (sctk_user_data_types + i);
	  other_user_types[i].id = i;
	  other_user_types[i].size = size;
	  other_user_types[i].count = count;
	  other_user_types[i].datatype = data_in;
	  sctk_spinlock_unlock (&(task_specific->other_user_types.lock));
	  SCTK_PROFIL_END (MPC_Sizeof_datatype);
	  MPC_ERROR_SUCESS ();
	}
    }
  sctk_warning ("Not enough datatypes allowed");
  sctk_spinlock_unlock (&(task_specific->other_user_types.lock));
  return -1;
}


int
PMPC_Derived_use (MPC_Datatype datatype)
{
  sctk_task_specific_t *task_specific;
  if ((datatype >= sctk_user_data_types + sctk_user_data_types_max) &&
      (datatype < sctk_user_data_types + 2 * sctk_user_data_types_max))
    {
      sctk_derived_type_t **user_types;
      task_specific = __MPC_get_task_specific ();
      sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
      user_types = task_specific->user_types_struct.user_types_struct;
      user_types[datatype - sctk_user_data_types -
		 sctk_user_data_types_max]->ref_count++;
      sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Derived_datatype (MPC_Datatype * datatype,
					   mpc_pack_absolute_indexes_t * begins,
					   mpc_pack_absolute_indexes_t * ends, unsigned long count,
					   mpc_pack_absolute_indexes_t lb, int is_lb,
					   mpc_pack_absolute_indexes_t ub, int is_ub)
{
	int i;
	sctk_derived_type_t **user_types;
	sctk_task_specific_t *task_specific;
	SCTK_PROFIL_START (MPC_Derived_datatype);
	*datatype = MPC_DATATYPE_NULL;
	task_specific = __MPC_get_task_specific ();
	sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
	user_types = task_specific->user_types_struct.user_types_struct;
	for (i = 0; i < sctk_user_data_types_max; i++)
    {
		if (user_types[i] == NULL)
		{
			unsigned long j;
			sctk_derived_type_t *t;
			*datatype = (sctk_user_data_types + sctk_user_data_types_max + i);
			sctk_nodebug("datatype = %d + %d + %d = %d", sctk_user_data_types, sctk_user_data_types_max, i, *datatype);
			t = (sctk_derived_type_t *) sctk_malloc (sizeof (sctk_derived_type_t));
			t->begins = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
			t->ends = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
			
			memcpy (t->begins, begins, count * sizeof (mpc_pack_absolute_indexes_t));
			memcpy (t->ends, ends, count * sizeof (mpc_pack_absolute_indexes_t));
			
			user_types[i] = t;
			t->size = 0;
			t->nb_elements = count;
			sctk_nodebug ("Create type %d with count %lu", sctk_user_data_types + sctk_user_data_types_max + i, count);
			t->count = count;
			t->ref_count = 1;
			for (j = 0; j < count; j++)
			{
				t->size += t->ends[j] - t->begins[j] + 1;
			}
			t->ub = ub;
			t->lb = lb;
			t->is_ub = is_ub;
			t->is_lb = is_lb;
			sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
			SCTK_PROFIL_END (MPC_Derived_datatype);
			MPC_ERROR_SUCESS ();
		}
    }
	sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
	sctk_warning ("Not enough datatypes allowed");
	return -1;
}


int
PMPC_Copy_in_buffer (void *inbuffer, void *outbuffer, int count,
		     MPC_Datatype datatype)
{
  if (sctk_is_derived_type (datatype))
    {
      sctk_derived_type_t **user_types;
      sctk_derived_type_t *t;
      int j;
      char *tmp;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();
      sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
      tmp = (char *) outbuffer;
      sctk_assert (datatype - sctk_user_data_types -
		   sctk_user_data_types_max < sctk_user_data_types_max);
      user_types = task_specific->user_types_struct.user_types_struct;
      sctk_assert (user_types != NULL);
      sctk_assert (user_types
		   [datatype - sctk_user_data_types -
		    sctk_user_data_types_max] != NULL);
      t = user_types[datatype - sctk_user_data_types -
		     sctk_user_data_types_max];
      for (j = 0; j < count; j++)
	{
	  size_t size;
	  size = t->ends[j] - t->begins[j] + 1;
	  memcpy (tmp, ((char *) inbuffer) + t->begins[j], size);
	  tmp += size;
	}
      sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
    }
  else
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_INTERN, "");
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Copy_from_buffer (void *inbuffer, void *outbuffer, int count,
		       MPC_Datatype datatype)
{
  if (sctk_is_derived_type (datatype))
    {
      sctk_derived_type_t **user_types;
      sctk_derived_type_t *t;
      int j;
      char *tmp;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();
      sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
      tmp = (char *) inbuffer;
      sctk_assert (datatype - sctk_user_data_types -
		   sctk_user_data_types_max < sctk_user_data_types_max);
      user_types = task_specific->user_types_struct.user_types_struct;
      sctk_assert (user_types != NULL);
      sctk_assert (user_types
		   [datatype - sctk_user_data_types -
		    sctk_user_data_types_max] != NULL);
      t = user_types[datatype - sctk_user_data_types -
		     sctk_user_data_types_max];
      for (j = 0; j < count; j++)
	{
	  size_t size;
	  size = t->ends[j] - t->begins[j] + 1;
	  memcpy (((char *) outbuffer) + t->begins[j], tmp, size);
	  tmp += size;
	}
      sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
    }
  else
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_INTERN, "");
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Is_derived_datatype (MPC_Datatype datatype, int *res,
			  mpc_pack_absolute_indexes_t ** begins,
			  mpc_pack_absolute_indexes_t ** ends,
			  unsigned long *count,
			  mpc_pack_absolute_indexes_t * lb, int *is_lb,
			  mpc_pack_absolute_indexes_t * ub, int *is_ub)
{
  sctk_task_specific_t *task_specific;

  *res = 0;
  *ends = NULL;
  *begins = NULL;
  *count = 1;

  if ((datatype >= sctk_user_data_types + sctk_user_data_types_max) &&
      (datatype < sctk_user_data_types + 2 * sctk_user_data_types_max))
    {
      sctk_derived_type_t **user_types;
      task_specific = __MPC_get_task_specific ();
      sctk_spinlock_lock (&(task_specific->user_types_struct.lock));
      user_types = task_specific->user_types_struct.user_types_struct;

sctk_nodebug("datatype(%d) - sctk_user_data_types(%d) - sctk_user_data_types_max(%d) ==  %d-%d-%d == %d", 
datatype, sctk_user_data_types, sctk_user_data_types_max, datatype, sctk_user_data_types, sctk_user_data_types_max, datatype - sctk_user_data_types - sctk_user_data_types_max);
      *res = 1;
      *ends = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->ends;
      *begins = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->begins;
      *count = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->count;

      *lb = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->lb;
      *ub = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->ub;
      *is_lb = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->is_lb;
      *is_ub = user_types[datatype - sctk_user_data_types - sctk_user_data_types_max]->is_ub;
      sctk_spinlock_unlock (&(task_specific->user_types_struct.lock));
    }
  MPC_ERROR_SUCESS ();
}

/*Initialisation*/
int
PMPC_Init (int *argc, char ***argv)
{
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Init);
#ifdef MPC_LOG_DEBUG
  char *mpc_log_debug_env;
  mpc_log_debug_env = getenv ("MPC_LOG_DEBUG");
  if (mpc_log_debug_env != NULL)
    {
      if (strcmp ("1", mpc_log_debug_env) == 0)
	{
	  mpc_log_debug_status = 1;
	}
    }
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Init");
#endif
  task_specific = __MPC_get_task_specific ();
  /* If the task calls MPI_Init() a second time */
  if (task_specific->finalize_done == 1) {
    return MPC_ERR_OTHER;
  }

  PMPC_Barrier (MPC_COMM_WORLD);
  task_specific->init_done = 1;
  SCTK_PROFIL_END (MPC_Init);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Init_thread (int *argc, char ***argv, int required, int *provided)
{
  const int res = PMPC_Init (argc, argv);
  if (res == MPC_SUCCESS) {
    *provided = required;
  }

  return res;
}

int
PMPC_Initialized (int *flag)
{
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Initialized);
  task_specific = __MPC_get_task_specific ();
  *flag = task_specific->init_done;
  SCTK_PROFIL_END (MPC_Initialized);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Finalize (void)
{
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Finalize);
  sctk_nodebug("PMPC_Finalize");

  __MPC_Barrier (MPC_COMM_WORLD);

  task_specific = __MPC_get_task_specific ();
  task_specific->init_done = 0;
  task_specific->finalize_done = 1;

#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Finalize");
#endif
  fflush (stderr);
  fflush (stdout);
  SCTK_PROFIL_END (MPC_Finalize);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Abort (MPC_Comm comm, int errorcode)
{
  SCTK_PROFIL_START (MPC_Abort);
  mpc_check_comm (comm, comm);
  sctk_error ("MPC_Abort with error %d", errorcode);
  fflush (stderr);
  fflush (stdout);
  sctk_abort ();
  SCTK_PROFIL_END (MPC_Abort);
  MPC_ERROR_SUCESS ();
}

void
PMPC_Default_error (MPC_Comm * comm, int *error, char *msg, char *file,
		    int line)
{
  char str[1024];
  int i;
  PMPC_Error_string (*error, str, &i);
  if (i != 0)
    sctk_error ("%s file %s line %d %s", str, file, line, msg);
  else
    sctk_error ("Unknown error");
  PMPC_Abort (*comm, *error);
}

void
PMPC_Return_error (MPC_Comm * comm, int *error, ...)
{
  char str[1024];
  int i;
  PMPC_Error_string (*error, str, &i);
  if (i != 0)
    sctk_error ("Warning: %s on comm %d", str, (int) *comm);
}

static void
MPC_Checkpoint_restart_init ()
{
  if (sctk_check_point_restart_mode)
    {
      int rank;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();
      __MPC_Barrier (MPC_COMM_WORLD);
      __MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);
      if (rank == 0)
	{
	  FILE *file;
	  int size;
	  int nb_processes;
	  char filename[SCTK_MAX_FILENAME_SIZE];
	  __MPC_Comm_size (MPC_COMM_WORLD, &size);
	  PMPC_Process_number (&nb_processes);
	  if (sctk_restart_mode == 0)
	    {
	      /*Normal start */
	      sprintf (filename, "%s/Job_description", sctk_store_dir);
	      file = fopen (filename, "w+");
	      assume (file != NULL);
	      fprintf (file, "Job with %d tasks on %d processes\n", size,
		       nb_processes);
	      sctk_launch_contribution (file);
	      fclose (file);
	    }
	  else
	    {
	      sctk_warning ("Restart the job");
	      not_reachable ();
	    }
	}
      __MPC_Barrier (MPC_COMM_WORLD);
      {
	FILE *file;
	char name[SCTK_MAX_FILENAME_SIZE];
	sctk_thread_t self;
	void *self_p = NULL;
	int vp;

	self = sctk_thread_self ();
	memcpy (&self_p, &self, sizeof (long));
	PMPC_Processor_rank (&vp);
	sprintf (name, "%s/Task_%d", sctk_store_dir, rank);

	file = fopen (name, "w+");
	assume (file != NULL);
	fprintf (file, "Restart 0\n");
	fprintf (file, "Process %d\n", sctk_process_rank);
	fprintf (file, "Thread %p\n", self_p);
	fprintf (file, "Virtual processor %d\n", vp);
	fclose (file);
      }
      {
	char name[1024];
	PMPC_Wait_pending_all_comm ();
	sprintf (name, "task_%p_%lu", sctk_thread_self (), (unsigned long) 0);

	sctk_thread_dump (name);
      }
      __MPC_Barrier (MPC_COMM_WORLD);
      sctk_restart_mode = 0;
    }
}
static void
MPC_Checkpoint_restart_end ()
{
  if (sctk_check_point_restart_mode)
    {
      char name[SCTK_MAX_FILENAME_SIZE];
      int rank;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();
      __MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);
      sctk_thread_dump_clean ();
      sprintf (name, "%s/Task_%d", sctk_store_dir, rank);
      remove (name);
    }
}

int
PMPC_Restarted (int *flag)
{
  void *f;
  f = sctk_thread_getspecific_mpc (sctk_check_point_key);
  if (f == ((void *) 1))
    *flag = 1;
  else
    *flag = 0;
  MPC_ERROR_SUCESS ();
}

int
PMPC_Checkpoint ()
{
  if (sctk_check_point_restart_mode)
    {
      PMPC_Checkpoint_timed (0, MPC_COMM_WORLD);
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Checkpoint_timed (unsigned int sec, MPC_Comm comm)
{
  if (sctk_check_point_restart_mode)
    {
      static volatile unsigned long perform_check = 1;
      static volatile unsigned long long last_time = 0;
      unsigned long perform;
      unsigned long tmp_perform;
      int rank;
      char name[1024];
      static unsigned long step = 1;
      int restarted;
      sctk_task_specific_t *task_specific;
      task_specific = __MPC_get_task_specific ();

      sctk_thread_setspecific_mpc (sctk_check_point_key, ((void *) 0));

      mpc_check_comm (comm, comm);
      __MPC_Comm_rank (comm, &rank, task_specific);
      perform = perform_check;

      if (perform != 0)
	{
	  int total_rank;

	  __MPC_Comm_rank (MPC_COMM_WORLD, &total_rank, task_specific);
	  assume (rank == total_rank);

	  sprintf (name, "task_%p_%lu", sctk_thread_self (), perform);

	  PMPC_Wait_pending_all_comm ();

	  if (rank == 0)
	    {
	      fprintf (stderr, "Checkpoint %lu in file %s\n", perform, name);
	    }
	  sctk_thread_dump (name);
	  step = perform + 1;
	  if (perform >= 2)
	    {
	      sprintf (name, "%s/task_%p_%lu", sctk_store_dir,
		       sctk_thread_self (), perform - 2);
	      remove (name);
	    }
	  last_time = sctk_timer;
	  PMPC_Restarted (&restarted);
	  if (restarted == 1)
	    {
	      sctk_ptp_per_task_init (rank);
	      __MPC_Barrier (MPC_COMM_WORLD);

#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
INFO("Si on redemarre , recreation des commnicateurs")
#endif
	    }
	}

      if (rank == 0)
	{
	  unsigned long long tmp_time = 0;
	  tmp_time = sctk_timer;
	  if ((tmp_time - last_time) * sctk_time_interval >= sec * 1000)
	    {
	      FILE *last;
	      tmp_perform = step;

	      sprintf (name, "%s/last_point", sctk_store_dir);
	      last = fopen (name, "w");
	      assume (last != NULL);
	      fprintf (last, "%lu\n", step);
	      fclose (last);

	      step++;
	      last_time = tmp_time;
	    }
	  else
	    {
	      tmp_perform = 0;
	    }
	}

      PMPC_Bcast (&tmp_perform, 1, MPC_LONG, 0, comm);

      perform_check = tmp_perform;
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Migrate ()
{
  if (sctk_check_point_restart_mode)
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
}

int
PMPC_Restart (int rank)
{
  if (sctk_check_point_restart_mode)
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
}

int
PMPC_Get_activity (int nb_item, MPC_Activity_t * tab, double *process_act)
{
  int i;
  int nb_proc;
  double proc_act = 0;
  nb_proc = sctk_get_processor_number ();
  sctk_nodebug ("Activity %d/%d:", nb_item, nb_proc);
  for (i = 0; (i < nb_item) && (i < nb_proc); i++)
    {
      double tmp;
      tab[i].virtual_cpuid = i;
      tmp = sctk_thread_get_activity (i);
      tab[i].usage = tmp;
      proc_act += tmp;
      sctk_nodebug ("\t- cpu %d activity %f\n", tab[i].virtual_cpuid,
		    tab[i].usage);
    }
  for (; i < nb_item; i++)
    {
      tab[i].virtual_cpuid = -1;
      tab[i].usage = -1;
    }
  proc_act = proc_act / ((double) nb_proc);
  *process_act = proc_act;
  MPC_ERROR_SUCESS ();
}

int
PMPC_Move_to (int process, int cpuid)
{
  int proc;
  proc = sctk_get_processor_rank ();
  sctk_nodebug ("move to %d %d(old %d)", process, cpuid, proc);
  if (process == sctk_process_rank)
    {
      if (proc != cpuid)
	{
	  sctk_thread_proc_migration (cpuid);
	}
    }
  else
    {
      if (sctk_is_net_migration_available () && sctk_migration_mode)
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
	}
      else
	{
	  sctk_warning ("Inter process migration Disabled");
	}
    }
  sctk_nodebug ("move to %d %d done", process, cpuid);
  MPC_ERROR_SUCESS ();
}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#pragma weak MPC_Task_hook
void  MPC_Task_hook(int rank)
{
  /*This function is used to intercept MPC tasks' creation when profiling*/
}
#endif

#define mpc_init(name,t) mpc_common_types[name] = sizeof(t) ; sctk_assert(name >=0 ); sctk_assert(name < sctk_user_data_types)

void
__MPC_init_types ()
{
  mpc_init (MPC_CHAR, char);
  mpc_init (MPC_LOGICAL, int);
  mpc_init (MPC_BYTE, unsigned char);
  mpc_init (MPC_SHORT, short);
  mpc_init (MPC_INT, int);
  mpc_init (MPC_LONG, long);
  mpc_init (MPC_FLOAT, float);
  mpc_init (MPC_DOUBLE, double);
  mpc_init (MPC_UNSIGNED_CHAR, unsigned char);
  mpc_init (MPC_UNSIGNED_SHORT, unsigned short);
  mpc_init (MPC_UNSIGNED, unsigned int);
  mpc_init (MPC_UNSIGNED_LONG, unsigned long);
  mpc_init (MPC_LONG_DOUBLE, long double);
  mpc_init (MPC_LONG_LONG_INT, long long);
  mpc_init (MPC_INTEGER1, sctk_int8_t);
  mpc_init (MPC_INTEGER2, sctk_int16_t);
  mpc_init (MPC_INTEGER4, sctk_int32_t);
  mpc_init (MPC_INTEGER8, sctk_int64_t);
  mpc_init (MPC_REAL4, float);
  mpc_init (MPC_REAL8, double);
  mpc_init (MPC_REAL16, long double);
  mpc_common_types[MPC_PACKED] = 0;
  mpc_init (MPC_FLOAT_INT, mpc_float_int);
  mpc_init (MPC_LONG_INT, mpc_long_int);
  mpc_init (MPC_DOUBLE_INT, mpc_double_int);
  mpc_init (MPC_SHORT_INT, mpc_short_int);
  mpc_init (MPC_2INT, mpc_int_int);
  mpc_init (MPC_2FLOAT, mpc_float_float);
  mpc_init (MPC_COMPLEX, mpc_float_float);
  mpc_init (MPC_2DOUBLE_PRECISION, mpc_double_double);
  mpc_init (MPC_DOUBLE_COMPLEX, mpc_double_double);
}

#ifdef MPC_OpenMP
#endif

int
/* main (int argc, char **argv) */
sctk_user_main (int argc, char **argv)
{
  int result;

  sctk_mpc_init_request_null();

  sctk_size_checking_eq (MPC_COMM_WORLD, SCTK_COMM_WORLD,
			 "MPC_COMM_WORLD", "SCTK_COMM_WORLD", __FILE__,
			 __LINE__);
  sctk_size_checking_eq (MPC_COMM_SELF, SCTK_COMM_SELF,
			 "MPC_COMM_SELF", "SCTK_COMM_SELF", __FILE__,
			 __LINE__);
  sctk_check_equal_types (MPC_Datatype, sctk_datatype_t);
  sctk_check_equal_types (sctk_communicator_t, MPC_Comm);
  sctk_check_equal_types (sctk_pack_indexes_t, mpc_pack_indexes_t);
  sctk_check_equal_types (sctk_pack_absolute_indexes_t,
			  mpc_pack_absolute_indexes_t);
  sctk_check_equal_types (sctk_count_t, mpc_msg_count);
  sctk_check_equal_types (sctk_thread_key_t, mpc_thread_key_t);

  sctk_mpc_verify_request_compatibility();

  __MPC_init_task_specific ();

  __MPC_Barrier (MPC_COMM_WORLD);
  if(getenv("MPC_HARD_CHECKING") != NULL){
    MPC_Hard_Check();
  }
  __MPC_Barrier (MPC_COMM_WORLD);


#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
  MPC_Task_hook(sctk_get_task_rank());
#endif

  MPC_Checkpoint_restart_init ();

#ifdef MPC_OpenMP
  __mpcomp_init() ;
#endif
  __MPC_Barrier (MPC_COMM_WORLD);
  
  result = mpc_user_main (argc, argv);
  
  __MPC_Barrier (MPC_COMM_WORLD);

#ifdef MPC_Profiler
	sctk_internal_profiler_render();
#endif

  MPC_Checkpoint_restart_end ();

  sctk_nodebug ("Wait for pending messages");

  __MPC_get_task_specific ();

  __MPC_delete_thread_specific();

  sctk_nodebug ("All message done");

  sctk_nodebug ("LAST BARRIER");
  __MPC_Barrier (MPC_COMM_WORLD);
  sctk_nodebug ("END BARRIER");

  __MPC_delete_task_specific ();

  return result;
}


/*Topology informations*/
int
PMPC_Comm_rank (MPC_Comm comm, int *rank)
{
  SCTK_PROFIL_START (MPC_Comm_rank);
  sctk_task_specific_t *task_specific;
  sctk_nodebug ("Get rank of %d", comm);
  mpc_check_comm (comm, comm);
  task_specific = __MPC_get_task_specific ();
  __MPC_Comm_rank (comm, rank, task_specific);
  sctk_nodebug ("Get rank of %d done", comm);
  SCTK_PROFIL_END (MPC_Comm_rank);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_size (MPC_Comm comm, int *size)
{
  SCTK_PROFIL_START (MPC_Comm_size);
  mpc_check_comm (comm, comm);
  __MPC_Comm_size (comm, size);
  SCTK_PROFIL_END (MPC_Comm_size);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_remote_size (MPC_Comm comm, int *size)
{
  SCTK_PROFIL_START (MPC_Comm_remote_size);
  mpc_check_comm (comm, comm);
  __MPC_Comm_remote_size (comm, size);
  SCTK_PROFIL_END (MPC_Comm_remote_size);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Node_rank (int *rank)
{
  SCTK_PROFIL_START (MPC_Node_rank);
  *rank = sctk_get_node_rank ();
  SCTK_PROFIL_END (MPC_Node_rank);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Node_number (int *number)
{
  SCTK_PROFIL_START (MPC_Node_number);
  *number = sctk_get_node_number ();
  SCTK_PROFIL_END (MPC_Node_number);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Processor_rank (int *rank)
{
  SCTK_PROFIL_START (MPC_Processor_rank);
  *rank = sctk_get_processor_rank ();
  SCTK_PROFIL_END (MPC_Processor_rank);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Processor_number (int *number)
{
  SCTK_PROFIL_START (MPC_Processor_number);
  *number = sctk_get_processor_number ();
  SCTK_PROFIL_END (MPC_Processor_number);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Process_rank (int *rank)
{
  SCTK_PROFIL_START (MPC_Process_rank);
  *rank = sctk_get_process_rank ();
  SCTK_PROFIL_END (MPC_Process_rank);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Process_number (int *number)
{
  SCTK_PROFIL_START (MPC_Process_number);
  *number = sctk_get_process_number ();
  SCTK_PROFIL_END (MPC_Process_number);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Local_process_rank (int *rank)
{
  SCTK_PROFIL_START (MPC_Local_process_rank);
  *rank = sctk_get_local_process_rank();
  SCTK_PROFIL_END (MPC_Local_process_rank);
  MPC_ERROR_SUCESS();
}

int
PMPC_Local_process_number (int *number)
{
  SCTK_PROFIL_START (MPC_Local_process_number);
  *number = sctk_get_local_process_number();
  SCTK_PROFIL_END (MPC_Local_process_number);
  MPC_ERROR_SUCESS();
}

int
PMPC_Task_rank (int *rank)
{
  SCTK_PROFIL_START (MPC_Task_rank);
  sctk_task_specific_t *task_specific;
  mpc_check_comm (MPC_COMM_WORLD, MPC_COMM_WORLD);
  task_specific = __MPC_get_task_specific ();
  __MPC_Comm_rank (MPC_COMM_WORLD, rank, task_specific);
  SCTK_PROFIL_END (MPC_Task_rank);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Task_number (int *number)
{
  SCTK_PROFIL_START (MPC_Task_number);
  mpc_check_comm (MPC_COMM_WORLD, MPC_COMM_WORLD);
  __MPC_Comm_size (MPC_COMM_WORLD, number);
  SCTK_PROFIL_END (MPC_Task_number);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Local_task_rank (int *rank)
{
  *rank = sctk_get_local_task_rank();
  MPC_ERROR_SUCESS();
}

int
PMPC_Local_task_number (int *number)
{
  *number = sctk_get_local_task_number();
  MPC_ERROR_SUCESS();
}

/*Collective operations*/

int
PMPC_Barrier (MPC_Comm comm)
{
  int tmp;
  SCTK_PROFIL_START (MPC_Barrier);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Barrier");
#endif

  sctk_nodebug ("begin barrier");
  tmp = __MPC_Barrier (comm);
  sctk_nodebug ("end barrier");

  SCTK_PROFIL_END (MPC_Barrier);
  return tmp;
}

static inline int
__MPC_Bcast (void *buffer, mpc_msg_count count, MPC_Datatype datatype,
	     int root, MPC_Comm comm, sctk_task_specific_t * task_specific)
{
	int size, rank;
	MPC_Status status;
	mpc_check_comm (comm, comm);
	__MPC_Comm_size (comm, &size);
	mpc_check_task (root, comm, size);
	mpc_check_type (datatype, comm);
	mpc_check_count (count, comm);
	
	if(sctk_is_inter_comm(comm))
	{
		if (root == MPC_PROC_NULL)
		{
			MPC_ERROR_SUCESS ();
		}
		else if (root == MPC_ROOT)
		{
			PMPC_Send(buffer, count, datatype, 0, 51, comm); 
		}
		else
		{
			__MPC_Comm_rank(comm, &rank, task_specific);
			
			if (rank == 0)
			{
				PMPC_Recv(buffer, count, datatype, root, 51, comm, &status);
			}
			__MPC_Bcast(buffer, count, datatype, 0, sctk_get_local_comm_id(comm), task_specific);
		}
	}
	else
	{
		sctk_nodebug ("sctk_broadcast root %d size %d", root, count * __MPC_Get_datatype_size (datatype, task_specific));
		sctk_broadcast (buffer, count * __MPC_Get_datatype_size (datatype, task_specific), root, comm);
	}
	MPC_ERROR_SUCESS ();
}

int
PMPC_Bcast (void *buffer, mpc_msg_count count, MPC_Datatype datatype,
	    int root, MPC_Comm comm)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Bcast);
  task_specific = __MPC_get_task_specific ();

#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Bcast ptr=%p count=%lu, type=%d, root=%d",
		 buffer, count, datatype, root);
#endif
  res = __MPC_Bcast (buffer, count, datatype, root, comm, task_specific);

  sctk_nodebug ("Bcast res %d", res);

  SCTK_PROFIL_END (MPC_Bcast);
  return res;
}


static void
MPC_Op_tmp (void *in, void *inout, size_t size, MPC_Datatype t)
{
  int n_size;
  MPC_Datatype n_t;
  MPC_User_function *func;
  n_size = (int) size;
  n_t = t;
  func = (MPC_User_function *) sctk_thread_getspecific_mpc (sctk_func_key);
  sctk_assert (func != NULL);
  sctk_nodebug ("User reduce %p", func);
  func (in, inout, &n_size, &n_t);
  sctk_nodebug ("User reduce %p done", func);
}

#define ADD_FUNC_HANDLER(func,t,op) case t: op = (MPC_Op_f)func##_##t;break
#define COMPAT_DATA_TYPE(op,func)                       \
  if(op == func){                                       \
    switch(datatype){                                   \
      ADD_FUNC_HANDLER(func,MPC_CHAR,op);               \
      ADD_FUNC_HANDLER(func,MPC_BYTE,op);               \
      ADD_FUNC_HANDLER(func,MPC_SHORT,op);              \
      ADD_FUNC_HANDLER(func,MPC_INT,op);                \
      ADD_FUNC_HANDLER(func,MPC_LONG,op);               \
      ADD_FUNC_HANDLER(func,MPC_FLOAT,op);              \
      ADD_FUNC_HANDLER(func,MPC_DOUBLE,op);             \
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_CHAR,op);      \
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_SHORT,op);     \
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED,op);           \
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_LONG,op);      \
      ADD_FUNC_HANDLER(func,MPC_LONG_DOUBLE,op);        \
      ADD_FUNC_HANDLER(func,MPC_LONG_LONG_INT,op);      \
      ADD_FUNC_HANDLER(func,MPC_COMPLEX,op);		\
      ADD_FUNC_HANDLER(func,MPC_DOUBLE_COMPLEX,op);	\
    default:not_reachable();				\
    }							\
  }

#define COMPAT_DATA_TYPE2(op,func)			\
  if(op == func){					\
    switch(datatype){					\
      ADD_FUNC_HANDLER(func,MPC_CHAR,op);		\
      ADD_FUNC_HANDLER(func,MPC_BYTE,op);		\
      ADD_FUNC_HANDLER(func,MPC_SHORT,op);		\
      ADD_FUNC_HANDLER(func,MPC_INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_LONG,op);		\
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_CHAR,op);	\
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_SHORT,op);	\
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED,op);		\
      ADD_FUNC_HANDLER(func,MPC_UNSIGNED_LONG,op);	\
      ADD_FUNC_HANDLER(func,MPC_LONG_LONG_INT,op);	\
      ADD_FUNC_HANDLER(func,MPC_LOGICAL,op);		\
    default:not_reachable();				\
    }							\
  }


#define COMPAT_DATA_TYPE3(op,func)			\
  if(op == func){					\
    switch(datatype){					\
      ADD_FUNC_HANDLER(func,MPC_FLOAT_INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_LONG_INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_DOUBLE_INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_SHORT_INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_2INT,op);		\
      ADD_FUNC_HANDLER(func,MPC_2FLOAT,op);		\
      ADD_FUNC_HANDLER(func,MPC_2DOUBLE_PRECISION,op);	\
    default:not_reachable();				\
    }							\
  }

MPC_Op_f
sctk_get_common_function (MPC_Datatype datatype, MPC_Op op)
{
  MPC_Op_f func;

  func = op.func;

  /*Internals function */
  COMPAT_DATA_TYPE (func, MPC_SUM_func)
  else
    COMPAT_DATA_TYPE (func, MPC_MAX_func)
    else
      COMPAT_DATA_TYPE (func, MPC_MIN_func)
      else
	COMPAT_DATA_TYPE (func, MPC_PROD_func)
	else
	  COMPAT_DATA_TYPE2 (func, MPC_BAND_func)
	  else
	    COMPAT_DATA_TYPE2 (func, MPC_LAND_func)
	    else
	      COMPAT_DATA_TYPE2 (func, MPC_BXOR_func)
	      else
		COMPAT_DATA_TYPE2 (func, MPC_LXOR_func)
		else
		  COMPAT_DATA_TYPE2 (func, MPC_BOR_func)
		  else
		    COMPAT_DATA_TYPE2 (func, MPC_LOR_func)
		    else
		      COMPAT_DATA_TYPE3 (func, MPC_MAXLOC_func)
		      else
			COMPAT_DATA_TYPE3 (func, MPC_MINLOC_func) sctk_nodebug ("Internal reduce");

  return func;
}

static inline int
__MPC_Allreduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		 MPC_Datatype datatype, MPC_Op op, MPC_Comm comm,
		 sctk_task_specific_t * task_specific)
{
	MPC_Op_f func;
	int root, rank;
	mpc_check_comm (comm, comm);
	mpc_check_count (count, comm);
	mpc_check_type (datatype, comm);
	__MPC_Comm_rank(comm, &rank, task_specific);

	sctk_nodebug ("Allreduce on %d with type %d", comm, datatype);
	if ((op.u_func == NULL) && (datatype < sctk_user_data_types))
    {
		func = op.func;

		/*Internals function */
		COMPAT_DATA_TYPE (func, MPC_SUM_func)
		else
		COMPAT_DATA_TYPE (func, MPC_MAX_func)
		else
		COMPAT_DATA_TYPE (func, MPC_MIN_func)
		else
	    COMPAT_DATA_TYPE (func, MPC_PROD_func)
	    else
	    COMPAT_DATA_TYPE2 (func, MPC_BAND_func)
		else
		COMPAT_DATA_TYPE2 (func, MPC_LAND_func)
		else
		COMPAT_DATA_TYPE2 (func, MPC_BXOR_func)
		else
		COMPAT_DATA_TYPE2 (func, MPC_LXOR_func)
		else
		COMPAT_DATA_TYPE2 (func, MPC_BOR_func)
		else
		COMPAT_DATA_TYPE2 (func, MPC_LOR_func)
		else
		COMPAT_DATA_TYPE3 (func, MPC_MAXLOC_func)
		else
		COMPAT_DATA_TYPE3 (func, MPC_MINLOC_func) 
		sctk_nodebug ("Internal reduce");
	}
	else
    {
		assume (op.u_func != NULL);
		/*User define function */
		sctk_thread_setspecific_mpc (sctk_func_key, (void *) op.u_func);
		func = (MPC_Op_f) MPC_Op_tmp;
		sctk_nodebug ("User reduce");
    }
	
	/* inter comm */
	if(sctk_is_inter_comm(comm))
	{
		/* local group */
		if(sctk_is_in_local_group(comm))
		{
			/* reduce receiving from remote group*/
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
			
			/* reduce sending to remote group */
			root = 0;
			PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
		}
		/* remote group */
		else
		{
			/* reduce sending to local group */
			root = 0;
			PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
			
			/* reduce receiving from local group */
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			PMPC_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
		}
		/* broadcast intracomm */
		__MPC_Bcast(recvbuf, count, datatype, 0, sctk_get_local_comm_id(comm), task_specific);
	}
	else
	{
		sctk_all_reduce(sendbuf, 
						recvbuf, 
						__MPC_Get_datatype_size (datatype, task_specific),
						count,
						func,
						(sctk_communicator_t) comm, 
						(sctk_datatype_t) datatype);
	}
	MPC_ERROR_SUCESS ();
}

int
PMPC_Allreduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
		MPC_Datatype datatype, MPC_Op op, MPC_Comm comm)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Allreduce);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm,
		 "MPC_Allreduce send_ptr=%p recv_ptr=%p count=%lu, type=%d",
		 sendbuf, recvbuf, count, datatype);
#endif
  res =
    __MPC_Allreduce (sendbuf, recvbuf, count, datatype, op, comm,
		     task_specific);
  SCTK_PROFIL_END (MPC_Allreduce);
  return res;
}

#define MPC_Reduce_tmp_recvbuf_size 4096
int
PMPC_Reduce (void *sendbuf, void *recvbuf, mpc_msg_count count,
	     MPC_Datatype datatype, MPC_Op op, int root, MPC_Comm comm)
{
  unsigned long size;
  char tmp_recvbuf[MPC_Reduce_tmp_recvbuf_size];
  sctk_task_specific_t *task_specific;
  MPC_Status status;
  int com_size;
  int com_rank;
  void *tmp_buf;
  SCTK_PROFIL_START (MPC_Reduce);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);
  __MPC_Comm_rank_size (comm, &com_rank, &com_size, task_specific);
  mpc_check_task (root, comm, com_size);
  mpc_check_count (count, comm);
  mpc_check_type (datatype, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm,
		 "MPC_Reduce send_ptr=%p recv_ptr=%p count=%lu, type=%d",
		 sendbuf, recvbuf, count, datatype);
#endif

	/* inter comm */
	if(sctk_is_inter_comm(comm))
	{
		/* do nothing */
		if (root == MPC_PROC_NULL) 
			MPC_ERROR_SUCESS ();
		
		/* root receive from rank 0 on remote group */
		if (root == MPC_ROOT) 
		{
			PMPC_Recv(recvbuf, count, datatype, 0, 27, comm, &status);
		}
		else
		{
			if (com_rank == 0) 
			{
				/* allocate temp buffer */
				size = count * __MPC_Get_datatype_size (datatype, task_specific);
				tmp_buf = (void *) sctk_malloc(size);
			}
			/* reduce on remote group to rank 0*/
			PMPC_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, sctk_get_local_comm_id(comm));
			
			if (com_rank == 0)
			{
				/* send to root on local group */
				PMPC_Send(tmp_buf, count, datatype, root, 27, comm);
			}
		}
	}
	else
	{
	  if (com_rank != root)
		{
		  size = count * __MPC_Get_datatype_size (datatype, task_specific);
		  if (size < MPC_Reduce_tmp_recvbuf_size)
		{
		  __MPC_Allreduce (sendbuf, tmp_recvbuf, count, datatype, op,
				   comm, task_specific);
		}
		  else
		{
		  recvbuf = sctk_malloc (size);
		  __MPC_Allreduce (sendbuf, recvbuf, count, datatype, op, comm,
				   task_specific);
		  sctk_free (recvbuf);
		}
		}
	  else
		{
		  __MPC_Allreduce (sendbuf, recvbuf, count, datatype, op, comm,
				   task_specific);
		}
	}
  SCTK_PROFIL_END (MPC_Reduce);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Op_create (MPC_User_function * function, int commute, MPC_Op * op)
{
  MPC_Op init = MPC_OP_INIT;
  assume (commute == 1);
  *op = init;
  op->u_func = function;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Op_create");
#endif
  MPC_ERROR_SUCESS ();
}

int
PMPC_Op_free (MPC_Op * op)
{
  MPC_Op init = MPC_OP_INIT;
  *op = init;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Op_free");
#endif
  MPC_ERROR_SUCESS ();
}

/*P-t-P Communications*/
static inline int
__MPC_Isend (void *buf, mpc_msg_count count, MPC_Datatype datatype,
	     int dest, int tag, MPC_Comm comm, MPC_Request * request,
	     sctk_task_specific_t * task_specific)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int com_size;
  int buffer_rank;
  size_t msg_size;
  int comm_remote_size;
  mpc_buffered_msg_t *restrict tmp_buf;
  char tmp;

  mpc_check_comm (comm, comm);
  __MPC_Comm_rank_size (comm, &src, &com_size, task_specific);
 
  mpc_check_count (count, comm);
  if (count == 0)
    {
      buf = &tmp;
    }
  mpc_check_buf_null (buf, comm);
  mpc_check_type (datatype, comm);
  if (tag != MPC_ANY_TAG)
    {
      mpc_check_tag (tag, comm);
    }

  if (dest == MPC_PROC_NULL)
    {
      sctk_mpc_init_request(request,comm,src, REQUEST_SEND);
      MPC_ERROR_SUCESS ();
    }
  
	//~ if(sctk_is_inter_comm(comm))
	//~ {
		//~ __MPC_Comm_remote_size(comm, &comm_remote_size);
		//~ 
		//~ mpc_check_task_msg(src,comm," source",com_size);		
		//~ mpc_check_task_msg(dest,comm," destination",comm_remote_size);	
		//~ mpc_check_tag(tag,comm);  		
	//~ }
	//~ else
	//~ {
		//~ mpc_check_msg (src, dest, tag, comm, com_size);
	//~ }
  sctk_mpc_init_request(request,comm,src, REQUEST_SEND);
  msg = sctk_create_header (src,sctk_message_contiguous);
  d_size = __MPC_Get_datatype_size (datatype, task_specific);
  msg_size = count * d_size;

  if ((msg_size > MAX_MPC_BUFFERED_SIZE) || (sctk_is_net_message (dest)) || mpc_disable_buffering)
  {
    sctk_add_adress_in_message (msg, buf, msg_size);
    sctk_mpc_set_header_in_message (msg, tag, comm, src, dest,
        request, msg_size,pt2pt_specific_message_tag);
  }
  else
  {
    sctk_thread_specific_t* thread_specific;
    thread_specific = sctk_message_passing;
    sctk_spinlock_lock (&(thread_specific->buffer_async.lock));
    buffer_rank = thread_specific->buffer_async.buffer_async_rank;
    tmp_buf = &(thread_specific->buffer_async.buffer_async[buffer_rank]);
    thread_specific->buffer_async.buffer_async_rank =
      (buffer_rank + 1) % MAX_MPC_BUFFERED_MSG;

      if (tmp_buf->completion_flag == SCTK_MESSAGE_DONE)
      {
        /* We set the buffer as busy */
        tmp_buf->completion_flag = SCTK_MESSAGE_PENDING;
        sctk_add_adress_in_message (msg, tmp_buf->buf, msg_size);
        sctk_mpc_set_header_in_message (msg, tag, comm, src, dest,
            request, msg_size,pt2pt_specific_message_tag);

        sctk_spinlock_unlock (&(thread_specific->buffer_async.lock));

        msg->tail.buffer_async = tmp_buf;
        memcpy (tmp_buf->buf, buf, msg_size);
      }
      else
      {
        sctk_spinlock_unlock (&(thread_specific->buffer_async.lock));
        sctk_add_adress_in_message (msg, buf, msg_size);
        sctk_mpc_set_header_in_message (msg, tag, comm, src, dest,
            request, msg_size,pt2pt_specific_message_tag);
      }
  }

  sctk_nodebug ("Message from %d to %d", src, dest);
  sctk_nodebug ("isend : snd2, my rank = %d", src);
  sctk_send_message (msg);
  MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Issend (void *buf, mpc_msg_count count, MPC_Datatype datatype,
	      int dest, int tag, MPC_Comm comm, MPC_Request * request,
	      sctk_task_specific_t * task_specific)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int com_size;
  size_t msg_size;
  char tmp;

  mpc_check_comm (comm, comm);
  __MPC_Comm_rank_size (comm, &src, &com_size, task_specific);

  mpc_check_count (count, comm);
  if (count == 0)
    {
      buf = &tmp;
    }
  mpc_check_buf (buf, comm);
  mpc_check_type (datatype, comm);


  if (dest == MPC_PROC_NULL)
    {
      sctk_mpc_init_request(request,comm,src, REQUEST_SEND);
      MPC_ERROR_SUCESS ();
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
  sctk_mpc_init_request(request,comm,src, REQUEST_SEND);
  msg = sctk_create_header (src,sctk_message_contiguous);
  d_size = __MPC_Get_datatype_size (datatype, task_specific);
  msg_size = count * d_size;

  sctk_add_adress_in_message (msg, buf, msg_size);
  sctk_mpc_set_header_in_message (msg, tag, comm, src, dest,
				  request, msg_size,pt2pt_specific_message_tag);

  sctk_nodebug ("Message from %d to %d", src, dest);
  sctk_nodebug ("issend : snd2, my rank = %d", src);
  sctk_send_message (msg);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Isend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
	    int tag, MPC_Comm comm, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Isend);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm,
		 "MPC_Isend ptr=%p count=%lu type=%d dest=%d tag=%d req=%p",
		 buf, count, datatype, dest, tag, request);
#endif
  res =
    __MPC_Isend (buf, count, datatype, dest, tag, comm, request,
		 task_specific);
  SCTK_PROFIL_END (MPC_Isend);
  return res;
}

int
PMPC_Ibsend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
	     int tag, MPC_Comm comm, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START (MPC_Ibsend);
  task_specific = __MPC_get_task_specific ();
  res =
    __MPC_Isend (buf, count, datatype, dest, tag, comm, request,
		 task_specific);
  SCTK_PROFIL_END (MPC_Ibsend);
  return res;
}

int
PMPC_Issend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
	     int tag, MPC_Comm comm, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START (MPC_Issend);
  task_specific = __MPC_get_task_specific ();
  res =
    __MPC_Issend (buf, count, datatype, dest, tag, comm, request,
		  task_specific);
  SCTK_PROFIL_END (MPC_Issend);
  return res;
}

int
PMPC_Irsend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
	     int tag, MPC_Comm comm, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START (MPC_Irsend);
  task_specific = __MPC_get_task_specific ();
  res =
    __MPC_Isend (buf, count, datatype, dest, tag, comm, request,
		 task_specific);
  SCTK_PROFIL_END (MPC_Irsend);
  return res;
}

static inline int
__MPC_Irecv (void *buf, mpc_msg_count count, MPC_Datatype datatype,
	     int source, int tag, MPC_Comm comm, MPC_Request * request,
	     sctk_task_specific_t * task_specific)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  size_t d_size;
  int comm_size;
  char tmp;

  mpc_check_comm (comm, comm);
  mpc_check_count (count, comm);
  if (count == 0)
    {
      buf = &tmp;
    }
  mpc_check_buf_null (buf, comm);
  mpc_check_type (datatype, comm);

  __MPC_Comm_rank_size (comm, &src, &comm_size, task_specific);
 
  if (source == MPC_PROC_NULL)
    {
      sctk_mpc_init_request(request,comm,src, REQUEST_RECV);
      MPC_ERROR_SUCESS ();
    }
	sctk_mpc_init_request(request,comm,src, REQUEST_RECV);
  /*   mpc_check_msg (source, src, tag, comm, comm_size); */
  mpc_check_task_msg (src, comm, " destination", comm_size);
  if (source != MPC_ANY_SOURCE)
    {
		//~ if(sctk_is_inter_comm(comm))
		//~ {
			//~ int remote_size;
			//~ __MPC_Comm_remote_size (comm, &remote_size);
			//~ mpc_check_task_msg (source, comm, " source", remote_size);
		//~ }
		//~ else
			//~ mpc_check_task_msg (source, comm, " source", comm_size);
    }
  if (tag != MPC_ANY_TAG)
    {
      mpc_check_tag (tag, comm);
    }

  msg = sctk_create_header (src,sctk_message_contiguous);
  d_size = __MPC_Get_datatype_size (datatype, task_specific);
  sctk_add_adress_in_message (msg, buf, count * d_size);
  sctk_mpc_set_header_in_message (msg, tag, comm, source, src,
				  request, count * d_size,pt2pt_specific_message_tag);
  sctk_nodebug ("ircv : rcv, my rank = %d", src);
  sctk_recv_message (msg,task_specific->my_ptp_internal, 1);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Irecv (void *buf, mpc_msg_count count, MPC_Datatype datatype,
	    int source, int tag, MPC_Comm comm, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;

  SCTK_PROFIL_START (MPC_Irecv);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm,
		 "MPC_Irecv ptr=%p count=%lu type=%d source=%d tag=%d req=%p",
		 buf, count, datatype, source, tag, request);
#endif
  res =
    __MPC_Irecv (buf, count, datatype, source, tag, comm, request,
		 task_specific);
  SCTK_PROFIL_END (MPC_Irecv);
  return res;
}
static inline int
__MPC_Wait (MPC_Request * request, MPC_Status * status)
{
  if (sctk_mpc_get_message_source(request) == MPC_PROC_NULL)
    {
      sctk_mpc_message_set_is_null(request,1);
    }
  if (sctk_mpc_message_is_null(request) != 1)
    {
      sctk_mpc_wait_message (request);
      sctk_mpc_message_set_is_null(request,1);
    }
  sctk_mpc_commit_status_from_request(request,status);
  MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Test (MPC_Request * request, int *flag, MPC_Status * status)
{
	mpc_check_comm (sctk_mpc_get_communicator_from_request(request), MPC_COMM_WORLD);
	*flag = 0;
	//~ request->completion_flag == 0 && request->is_null == 0
	if ((sctk_mpc_completion_flag(request) == SCTK_MESSAGE_PENDING) && (!sctk_mpc_message_is_null(request)))
	{
		sctk_mpc_perform_messages(request);
		sctk_thread_yield ();
	}
	
	//~ request->completion_flag != 0
	if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING)
	{
		*flag = 1;
		sctk_mpc_commit_status_from_request(request,status);
	}
	
	//~ request->is_null > 0
	if(sctk_mpc_message_is_null(request)) 
	{
		*flag = 1;
		sctk_mpc_commit_status_from_request(request,status);
	}

	MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Test_check (MPC_Request * request, int *flag, MPC_Status * status)
{
  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING)
    {
      *flag = 1;
      sctk_mpc_commit_status_from_request(request,status);
      MPC_ERROR_SUCESS ();
    }

  mpc_check_comm (sctk_mpc_get_communicator_from_request(request), MPC_COMM_WORLD);
  *flag = 0;

  sctk_mpc_perform_messages(request);

  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING)
    {
      *flag = 1;
      sctk_mpc_commit_status_from_request(request,status);
    }
  MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Test_check_light (MPC_Request * request)
{
  sctk_mpc_perform_messages (request);

  return sctk_mpc_completion_flag(request);
}

static inline int
__MPC_Test_no_check (MPC_Request * request, int *flag, MPC_Status * status)
{
  *flag = 0;
  if (sctk_mpc_completion_flag(request) != SCTK_MESSAGE_PENDING)
    {
      *flag = 1;
      sctk_mpc_commit_status_from_request(request,status);
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Wait (MPC_Request * request, MPC_Status * status)
{
  int res;
  SCTK_PROFIL_START (MPC_Wait);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Wait req=%p", request);
#endif
  res = __MPC_Wait (request, status);
  SCTK_PROFIL_END (MPC_Wait);
  return res;
}

struct wfv_waitall_s{
  int ret;
  mpc_msg_count count;
  MPC_Request *array_of_requests;
  MPC_Status *array_of_statuses;
};

static inline void wfv_waitall (void *arg) {
  struct wfv_waitall_s *args = (struct wfv_waitall_s*) arg;
  int flag = 1;
  int i;

  for (i = 0; i < args->count; i++)
  {
    flag = flag & __MPC_Test_check_light (&(args->array_of_requests[i]));
  }

  /* All requests received */
  if (flag) {
    for (i = 0; i < args->count; i++)
    {
      MPC_Status *status;
      MPC_Request *request;

      request = &(args->array_of_requests[i]);
      if(args->array_of_statuses != NULL){
        status = &(args->array_of_statuses[i]);

        sctk_mpc_commit_status_from_request(request,status);
        sctk_nodebug ("source %d\n", status->MPC_SOURCE);
        sctk_mpc_message_set_is_null(request ,1);
      }
    }
    args->ret = 1;
  }
}

static inline int
__MPC_Waitall (mpc_msg_count count,
	       MPC_Request array_of_requests[],
	       MPC_Status array_of_statuses[])
{
  int i;
  int flag = 1;
  double start, end;
  int show = 1;

  sctk_nodebug ("waitall count %d\n", count);
  start = MPC_Wtime();

  for (i = 0; i < count; i++) {
    int tmp_flag = 0;
    MPC_Status *status;
    MPC_Request *request;
    if(array_of_statuses != NULL){
      status = &(array_of_statuses[i]);
    } else {
      status = MPC_STATUS_IGNORE;
    }
    request = &(array_of_requests[i]);
    __MPC_Test_check (request, &tmp_flag,
        status);

    if (tmp_flag && status != MPC_STATUS_IGNORE) {
      sctk_mpc_message_set_is_null(request,1);
    }

    flag = flag & tmp_flag;
  }
  if (flag == 1) MPC_ERROR_SUCESS();

  /* XXX: Waitall has been ported for using wait_for_value_and_poll
   * because it provides better results than thread_yield.
   * It performs well at least on Infiniband on NAS  */
  struct wfv_waitall_s wfv;
  wfv.ret = 0;
  wfv.array_of_requests = array_of_requests;
  wfv.array_of_statuses = array_of_statuses;
  wfv.count = count;
  sctk_thread_wait_for_value_and_poll
    ((int *) &(wfv.ret), 1 ,
     (void(*)(void*))wfv_waitall,(void*)&wfv);

  MPC_ERROR_SUCESS ();
}

int
PMPC_Waitall (mpc_msg_count count,
	      MPC_Request array_of_requests[], MPC_Status array_of_statuses[])
{
  int res;

  SCTK_PROFIL_START (MPC_Waitall);

#ifdef MPC_LOG_DEBUG
  int i;
  for (i = 0; i < count; i++)
    {
      mpc_log_debug (MPC_COMM_WORLD, "MPC_Waitall position=%d req=%p", i,
		     array_of_requests[i]);
    }
#endif
  res = __MPC_Waitall (count, array_of_requests, array_of_statuses);
  SCTK_PROFIL_END (MPC_Waitall);
  return res;
}

int
PMPC_Waitsome (mpc_msg_count incount,
	       MPC_Request array_of_requests[],
	       mpc_msg_count * outcount,
	       mpc_msg_count array_of_indices[],
	       MPC_Status array_of_statuses[])
{
  int i;
  int done = 0;
  SCTK_PROFIL_START (MPC_Waitsome);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Waitsome");
#endif
  while (done == 0)
    {
      for (i = 0; i < incount; i++)
	{
	  if (sctk_mpc_message_get_is_null(&(array_of_requests[i])) != 1)
	    {
	      int tmp_flag = 0;
	      __MPC_Test_check (&(array_of_requests[i]), &tmp_flag,
				&(array_of_statuses[done]));
	      if (tmp_flag)
		{
		  sctk_mpc_message_set_is_null(&(array_of_requests[i]),1);
		  array_of_indices[done] = i;
		  done++;
		}
	    }
	}
      if (done == 0)
	{
TODO("wait_for_value_and_poll should be used here")
	  sctk_thread_yield ();
	}
    }
  *outcount = done;
  SCTK_PROFIL_END (MPC_Waitsome);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Waitany (mpc_msg_count count,
	      MPC_Request array_of_requests[], mpc_msg_count * index,
	      MPC_Status * status)
{
  int i;
  SCTK_PROFIL_START (MPC_Waitany);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Waitany");
#endif
  *index = MPC_UNDEFINED;
  while (1)
    {
      for (i = 0; i < count; i++)
	{
	  if (sctk_mpc_message_get_is_null(&(array_of_requests[i])) != 1)
	    {
	      int tmp_flag = 0;
	      __MPC_Test_check (&(array_of_requests[i]), &tmp_flag, status);
	      if (tmp_flag)
		{
		  __MPC_Wait (&(array_of_requests[i]), status);
		  *index = count;
		  SCTK_PROFIL_END (MPC_Waitany);
		  MPC_ERROR_SUCESS ();
		}
	    }
	}
TODO("wait_for_value_and_poll should be used here")
      sctk_thread_yield ();
    }
}

static inline int
__MPC_Wait_pending (MPC_Comm comm)
{
  int src;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);

  __MPC_Comm_rank (comm, &src, task_specific);

  sctk_mpc_wait_all (src, comm);

  MPC_ERROR_SUCESS ();
}

int
PMPC_Wait_pending (MPC_Comm comm)
{
  int res;

  SCTK_PROFIL_START (MPC_Wait_pending);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Wait_pending");
#endif
  res = __MPC_Wait_pending (comm);
  SCTK_PROFIL_END (MPC_Wait_pending);
  return res;
}

int
PMPC_Wait_pending_all_comm ()
{
  int j;
  sctk_task_specific_t *task_specific;
  mpc_per_communicator_t*per_communicator;
  mpc_per_communicator_t*per_communicator_tmp;
  task_specific = __MPC_get_task_specific ();

  SCTK_PROFIL_START (MPC_Wait_pending_all_comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Wait_pending_all_comm");
#endif
  sctk_spinlock_lock(&(task_specific->per_communicator_lock));
  HASH_ITER(hh,task_specific->per_communicator,per_communicator,per_communicator_tmp)
    {
      j = per_communicator->key;
      if (sctk_is_valid_comm (j))
	__MPC_Wait_pending (j);
    }
  sctk_spinlock_unlock(&(task_specific->per_communicator_lock));
  SCTK_PROFIL_END (MPC_Wait_pending_all_comm);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Test (MPC_Request * request, int *flag, MPC_Status * status)
{
  int res;

  SCTK_PROFIL_START (MPC_Test);
  res = __MPC_Test (request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Test req=%p flag=%d", request, *flag);
#endif
  SCTK_PROFIL_END (MPC_Test);
  return res;
}

int
PMPC_Test_no_check (MPC_Request * request, int *flag, MPC_Status * status)
{
  int res;
  SCTK_PROFIL_START (MPC_Test_no_check);
  res = __MPC_Test_no_check (request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Test_no_check req=%p flag=%d",
		 request, *flag);
#endif
  SCTK_PROFIL_END (MPC_Test_no_check);
  return res;
}

int
PMPC_Test_check (MPC_Request * request, int *flag, MPC_Status * status)
{
  int res;
  SCTK_PROFIL_START (MPC_Test_check);
  res = __MPC_Test_check (request, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Test_check req=%p flag=%d", request,
		 *flag);
#endif
  SCTK_PROFIL_END (MPC_Test_check);
  return res;
}

static int
__MPC_Ssend (void *buf, mpc_msg_count count, MPC_Datatype datatype,
	     int dest, int tag, MPC_Comm comm)
{
  MPC_Request request;
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  size_t msg_size;
  sctk_task_specific_t *task_specific;
  char tmp;
  task_specific = __MPC_get_task_specific ();

  __MPC_Comm_rank_size (comm, &src, &size, task_specific);

  mpc_check_comm (comm, comm);
  mpc_check_count (count, comm);
  if (count == 0)
    {
      buf = &tmp;
    }
  mpc_check_buf (buf, comm);
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
  mpc_log_debug (comm,
		 "MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf,
		 count, datatype, dest, tag);
#endif

  if (dest == MPC_PROC_NULL)
    {
      MPC_ERROR_SUCESS ();
    }


  msg_size = count * __MPC_Get_datatype_size (datatype, task_specific);

  msg = sctk_create_header (src,sctk_message_contiguous);
  sctk_add_adress_in_message (msg, buf,msg_size);
  sctk_mpc_init_request(&request,comm,src, REQUEST_SEND);

  sctk_mpc_set_header_in_message (msg, tag, comm, src, dest, &request, msg_size,pt2pt_specific_message_tag);

  sctk_nodebug ("count = %d, datatype = %d", msg->body.header.msg_size, datatype);
  sctk_send_message (msg);
  sctk_mpc_wait_message (&request);
  MPC_ERROR_SUCESS ();
}

static void sctk_no_free_header(void* tmp){

}

static int
__MPC_Send (void *restrict buf, mpc_msg_count count, MPC_Datatype datatype,
        int dest, int tag, MPC_Comm comm)
  {
    MPC_Request request;
    sctk_thread_ptp_message_t *msg;
    int src;
    int size;
    size_t msg_size;
    sctk_task_specific_t *task_specific;
    int buffer_rank;
    mpc_buffered_msg_t *restrict tmp_buf;
    char tmp;
    sctk_thread_ptp_message_t header;

    task_specific = __MPC_get_task_specific ();

    __MPC_Comm_rank_size (comm, &src, &size, task_specific);
   
    mpc_check_comm (comm, comm);
    mpc_check_count (count, comm);
    if (count == 0)
    {
      buf = &tmp;
    }
    mpc_check_buf (buf, comm);
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
    mpc_log_debug (comm,
        "MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf,
        count, datatype, dest, tag);
#endif
    sctk_nodebug ("MPC_Send ptr=%p count=%lu type=%d dest=%d tag=%d ", buf,
        count, datatype, dest, tag);

    if (dest == MPC_PROC_NULL)
    {
      MPC_ERROR_SUCESS ();
    }


    msg_size = count * __MPC_Get_datatype_size (datatype, task_specific);
    sctk_nodebug ("Message size %lu", msg_size);


    if ((msg_size > MAX_MPC_BUFFERED_SIZE) || (sctk_is_net_message (dest)) || mpc_disable_buffering)
    {
      msg = &header;
      sctk_init_header(msg,src,sctk_message_contiguous,sctk_no_free_header,sctk_message_copy);
      sctk_mpc_init_request(&request,comm,src, REQUEST_SEND);
      sctk_add_adress_in_message(msg,buf,msg_size);
      sctk_mpc_set_header_in_message (msg, tag, comm, src, dest,
          &request, msg_size,pt2pt_specific_message_tag);
      sctk_send_message (msg);
      sctk_nodebug("send request.is_null %d",request.is_null);
      sctk_mpc_wait_message (&request);
    }
    else
    {
      sctk_thread_specific_t* thread_specific;
      thread_specific = sctk_message_passing;
      sctk_spinlock_lock (&(thread_specific->buffer.lock));
      buffer_rank = thread_specific->buffer.buffer_rank;
      tmp_buf = &(thread_specific->buffer.buffer[buffer_rank]);
      thread_specific->buffer.buffer_rank =
        (buffer_rank + 1) % MAX_MPC_BUFFERED_MSG;
      TODO("To optimize")
        if (sctk_mpc_completion_flag(&(tmp_buf->request)) != SCTK_MESSAGE_DONE)
        {
          msg = &header;
          sctk_init_header(msg,src,sctk_message_contiguous,sctk_no_free_header,sctk_message_copy);
          sctk_spinlock_unlock (&(thread_specific->buffer.lock));
          sctk_mpc_init_request(&request,comm,src, REQUEST_SEND);
          sctk_add_adress_in_message(msg,buf,msg_size);
          sctk_mpc_set_header_in_message (msg, tag, comm, src, dest, &request, msg_size,pt2pt_specific_message_tag);
          sctk_send_message (msg);
          sctk_nodebug("send request.is_null %d",request.is_null);
          sctk_mpc_wait_message (&request);
        }
        else
        {
          msg = &(tmp_buf->header);
          sctk_init_header(msg,src,sctk_message_contiguous,sctk_no_free_header,sctk_message_copy);
          sctk_mpc_init_request(&(tmp_buf->request),comm,src, REQUEST_SEND);
          sctk_nodebug ("Copied message |%s| -> |%s| %d", buf, tmp_buf->buf, msg_size);
          sctk_add_adress_in_message(msg,tmp_buf->buf,msg_size);
          sctk_mpc_set_header_in_message (msg, tag, comm, src, dest, &(tmp_buf->request), msg_size,pt2pt_specific_message_tag);
          sctk_spinlock_unlock (&(thread_specific->buffer.lock));
		  
          msg->tail.buffer_async = tmp_buf;
          memcpy (tmp_buf->buf, buf, msg_size);
          sctk_send_message (msg);
        }
    }

    MPC_ERROR_SUCESS ();
  }

  int
  PMPC_Ssend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
        int tag, MPC_Comm comm)
  {
    int res;
    SCTK_PROFIL_START (MPC_Ssend);
    mpc_check_type (datatype, comm);
    res = __MPC_Ssend (buf, count, datatype, dest, tag, comm);
    SCTK_PROFIL_END (MPC_Ssend);
    return res;
  }

  int
  PMPC_Bsend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
        int tag, MPC_Comm comm)
  {
    int res;
    SCTK_PROFIL_START (MPC_Bsend);
    mpc_check_type (datatype, comm);
    res = __MPC_Send (buf, count, datatype, dest, tag, comm);
    SCTK_PROFIL_END (MPC_Bsend);
    return res;
  }

  int
  PMPC_Send (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
      int tag, MPC_Comm comm)
  {
    int res;
    SCTK_PROFIL_START (MPC_Send);
    mpc_check_type (datatype, comm);
    res = __MPC_Send (buf, count, datatype, dest, tag, comm);
    SCTK_PROFIL_END (MPC_Send);
    sctk_nodebug("exit send comm %d", comm);
    return res;
  }

  int
  PMPC_Rsend (void *buf, mpc_msg_count count, MPC_Datatype datatype, int dest,
        int tag, MPC_Comm comm)
  {
    int res;
    SCTK_PROFIL_START (MPC_Rsend);
    mpc_check_type (datatype, comm);
    res = __MPC_Send (buf, count, datatype, dest, tag, comm);
    SCTK_PROFIL_END (MPC_Rsend);
    return res;
  }

  static int
  __MPC_Probe (int source, int tag, MPC_Comm comm, MPC_Status * status,
        sctk_task_specific_t * task_specific);

  int
  PMPC_Recv (void *buf, mpc_msg_count count, MPC_Datatype datatype, int source,
      int tag, MPC_Comm comm, MPC_Status * status)
  {
    MPC_Request request;
    sctk_thread_ptp_message_t *msg;
    int src;
    int size;
    size_t msg_size;
    sctk_task_specific_t *task_specific;
    char tmp;
    SCTK_PROFIL_START (MPC_Recv);
    task_specific = __MPC_get_task_specific ();

    mpc_check_comm (comm, comm);
    mpc_check_count (count, comm);
    if (count == 0)
      {
        buf = &tmp;
      }
    mpc_check_buf (buf, comm);
    mpc_check_type (datatype, comm);

#ifdef MPC_LOG_DEBUG
    mpc_log_debug (comm,
      "MPC_Recv ptr=%p count=%lu type=%d source=%d tag=%d",
      buf, count, datatype, source, tag);
#endif
    sctk_nodebug ("MPC_Recv ptr=%p count=%lu type=%d source=%d tag=%d",
      buf, count, datatype, source, tag);

    if (source == MPC_PROC_NULL)
      {
        if (status != MPC_STATUS_IGNORE)
    {
      status->MPC_SOURCE = MPC_PROC_NULL;
      status->MPC_TAG = MPC_ANY_TAG;
      status->MPC_ERROR = MPC_SUCCESS;
      status->count = 0;
    }
        SCTK_PROFIL_END (MPC_Recv);
        MPC_ERROR_SUCESS ();
      }

    __MPC_Comm_rank_size (comm, &src, &size, task_specific);
    
    //~ if(sctk_is_inter_comm(comm))
	//~ {
		//~ int remote_size;
		//~ PMPC_Comm_remote_size(comm, &remote_size);
		//~ mpc_check_msg_size_inter (source, src, tag, comm, size, remote_size);
	//~ }
	//~ else
	//~ {
		//~ mpc_check_msg_size (source, src, tag, comm, size);
	//~ }
    

    msg_size = count * __MPC_Get_datatype_size (datatype, task_specific);

    sctk_mpc_init_request(&request,comm,src, REQUEST_RECV);
    msg = sctk_create_header (src,sctk_message_contiguous);

    sctk_add_adress_in_message (msg, buf,msg_size);

    sctk_mpc_set_header_in_message (msg, tag, comm, source, src, &request,
            msg_size,pt2pt_specific_message_tag);

    sctk_recv_message (msg,task_specific->my_ptp_internal, 1);
    sctk_nodebug("recv request.is_null %d",request.is_null);
    sctk_mpc_wait_message (&request);

    sctk_nodebug ("count = %d", msg_size);
    sctk_nodebug ("req count = %d", request.header.msg_size);

    sctk_mpc_commit_status_from_request(&request,status);
    SCTK_PROFIL_END (MPC_Recv);
    sctk_nodebug("exit recv comm %d", comm);
    MPC_ERROR_SUCESS ();
  }

  int
  PMPC_Sendrecv (void *sendbuf, mpc_msg_count sendcount, MPC_Datatype sendtype,
          int dest, int sendtag,
          void *recvbuf, mpc_msg_count recvcount, MPC_Datatype recvtype,
          int source, int recvtag, MPC_Comm comm, MPC_Status * status)
  {
    MPC_Request sendreq;
    MPC_Request recvreq;
    sctk_task_specific_t *task_specific;
    char tmp;
    SCTK_PROFIL_START (MPC_Sendrecv);
    task_specific = __MPC_get_task_specific ();

    sendreq = MPC_REQUEST_NULL;
    recvreq = MPC_REQUEST_NULL;

    mpc_check_count (sendcount, comm);
    if (sendcount == 0)
      {
        sendbuf = &tmp;
      }
    mpc_check_count (recvcount, comm);
    if (recvcount == 0)
      {
        recvbuf = &tmp;
      }

    mpc_check_buf (sendbuf, comm);
    mpc_check_buf (recvbuf, comm);
    mpc_check_type (sendtype, comm);
    mpc_check_type (recvtype, comm);
    mpc_check_comm (comm, comm);
    mpc_check_tag (recvtag, comm);
    mpc_check_tag (sendtag, comm);
#ifdef MPC_LOG_DEBUG
    mpc_log_debug (comm, "Sendrecv");
#endif
    __MPC_Irecv (recvbuf, recvcount, recvtype, source, recvtag, comm,
          &recvreq, task_specific);
    __MPC_Isend (sendbuf, sendcount, sendtype, dest, sendtag, comm,
	       &sendreq, task_specific);
  __MPC_Wait (&sendreq, status);
  __MPC_Wait (&recvreq, status);
  SCTK_PROFIL_END (MPC_Sendrecv);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Test_cancelled (MPC_Status * status, int *flag)
{
  *flag = (status->cancelled == 1);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Cancel (MPC_Request * request)
{
  sctk_mpc_cancel_message (request);
  MPC_ERROR_SUCESS ();
}

#define MPC_MAX_CONCURENT 100
static inline int
__MPC_Gatherv (void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
	       void *recvbuf, mpc_msg_count * recvcnts,
	       mpc_msg_count * displs, MPC_Datatype recvtype, int root,
	       MPC_Comm comm, sctk_task_specific_t * task_specific)
{
	int i;
	int j;
	int rank;
	int size;
	MPC_Request request;
	MPC_Request recvrequest[MPC_MAX_CONCURENT];

	mpc_check_comm (comm, comm);
	__MPC_Comm_rank_size (comm, &rank, &size, task_specific);
	mpc_check_task (root, comm, size);
	mpc_check_buf (sendbuf, comm);
	mpc_check_count (sendcnt, comm);
	mpc_check_type (sendtype, comm);
	
	if(sctk_is_inter_comm(comm))
	{
		if(root == MPC_PROC_NULL)
		{
			MPC_ERROR_SUCESS ();
		}
		else if(root == MPC_ROOT)
		{
			size_t recv_type_size;
			mpc_check_buf (recvbuf, comm);
			mpc_check_type (recvtype, comm);

			recv_type_size = __MPC_Get_datatype_size (recvtype, task_specific);
			i = 0;
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Irecv (((char *) recvbuf) + (displs[i] * recv_type_size), recvcnts[i], 
					recvtype, i, MPC_GATHERV_TAG, comm, &(recvrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(recvrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}
		else
		{
			__MPC_Isend (sendbuf, sendcnt, sendtype, root, MPC_GATHERV_TAG, comm, &request, task_specific);
			__MPC_Wait (&(request), MPC_STATUS_IGNORE);
		}  
	}
	else
	{	
		__MPC_Isend (sendbuf, sendcnt, sendtype, root, MPC_GATHERV_TAG, comm, &request, task_specific);

		if (rank == root)
		{
			size_t recv_type_size;
			mpc_check_buf (recvbuf, comm);
			mpc_check_type (recvtype, comm);

			recv_type_size = __MPC_Get_datatype_size (recvtype, task_specific);
			i = 0;
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Irecv (((char *) recvbuf) + (displs[i] * recv_type_size), recvcnts[i], 
					recvtype, i, MPC_GATHERV_TAG, comm, &(recvrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(recvrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}

		__MPC_Wait (&(request), MPC_STATUS_IGNORE);
	}
	MPC_ERROR_SUCESS ();
}

int
PMPC_Gatherv (void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
	      void *recvbuf, mpc_msg_count * recvcnts,
	      mpc_msg_count * displs, MPC_Datatype recvtype, int root,
	      MPC_Comm comm)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Gatherv);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Gatherv");
#endif
  res =
    __MPC_Gatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
		   displs, recvtype, root, comm, task_specific);
  SCTK_PROFIL_END (MPC_Gatherv);
  return res;
}

int
PMPC_Allgatherv (void *sendbuf, mpc_msg_count sendcount,
		 MPC_Datatype sendtype, void *recvbuf,
		 mpc_msg_count * recvcounts, mpc_msg_count * displs,
		 MPC_Datatype recvtype, MPC_Comm comm)
{
  int size;
  int rank;
  int root = 0;
  MPC_Comm local_comm;
  sctk_task_specific_t *task_specific;
  size_t dsize;
  SCTK_PROFIL_START (MPC_Allgatherv);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);
  mpc_check_buf (sendbuf, comm);
  mpc_check_buf (recvbuf, comm);
  mpc_check_count (sendcount, comm);
  mpc_check_type (sendtype, comm);
  mpc_check_type (recvtype, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Allgatherv");
#endif
	
	__MPC_Comm_rank_size (comm, &rank, &size, task_specific);
	if(sctk_is_inter_comm(comm))
	{
		if(sctk_is_in_local_group(comm))
		{
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			__MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, task_specific);
			
			root = 0;
			__MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, task_specific);
		}
		else
		{
			root = 0;
			__MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, task_specific);
        
			root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
			__MPC_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, task_specific);
       
		}
		
		size--;
		root = 0;
		local_comm = sctk_get_local_comm_id(comm);
		dsize = __MPC_Get_datatype_size (recvtype, task_specific);
		for (; size >= 0; size--)
		{
			__MPC_Bcast (((char *) recvbuf) + (displs[size] * dsize), recvcounts[size], recvtype, root, local_comm, task_specific);
		}
	}
	else
	{
		__MPC_Gatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, task_specific);
		size--;
		dsize = __MPC_Get_datatype_size (recvtype, task_specific);
		for (; size >= 0; size--)
		{
			__MPC_Bcast (((char *) recvbuf) + (displs[size] * dsize), recvcounts[size], recvtype, root, comm, task_specific);
		}
	}
	SCTK_PROFIL_END (MPC_Allgatherv);
	MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Gather (void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
	      void *recvbuf, mpc_msg_count recvcount, MPC_Datatype recvtype,
	      int root, MPC_Comm comm, sctk_task_specific_t * task_specific)
{
	int i;
	int j;
	int rank;
	int size;
	MPC_Request request;
	size_t dsize;
	MPC_Request recvrequest[MPC_MAX_CONCURENT];

	mpc_check_comm (comm, comm);
	__MPC_Comm_rank_size (comm, &rank, &size, task_specific);
	mpc_check_buf (sendbuf, comm);
	mpc_check_count (sendcnt, comm);
	mpc_check_count (recvcount, comm);
	mpc_check_type (sendtype, comm);
	mpc_check_type (recvtype, comm);
	mpc_check_buf (recvbuf, comm);
	mpc_check_task (root, comm, size);
	
  if(sctk_is_inter_comm(comm))
  {
	if(root == MPC_PROC_NULL)
	{
		MPC_ERROR_SUCESS ();
	}
	else if(root == MPC_ROOT)
	{
		i = 0;
		dsize = __MPC_Get_datatype_size (recvtype, task_specific);
		while (i < size)
		{
			for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
			{
				__MPC_Irecv (((char *) recvbuf) + (i * recvcount * dsize), recvcount, recvtype, i, MPC_GATHER_TAG, comm, &(recvrequest[j]), task_specific);
				i++;
				j++;
			}
			j--;
			for (; j >= 0; j--)
			{
				__MPC_Wait (&(recvrequest[j]), MPC_STATUS_IGNORE);
			}
		}
	}
	else
	{
		__MPC_Isend (sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm, &request, task_specific);
		__MPC_Wait (&(request), MPC_STATUS_IGNORE);
	}  
  }
  else
  {
	  __MPC_Isend (sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm, &request, task_specific);

	  __MPC_Comm_rank_size (comm, &rank, &size, task_specific);
	  
	  if (rank == root)
		{
		  i = 0;
		  dsize = __MPC_Get_datatype_size (recvtype, task_specific);
		  while (i < size)
		{
		  for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
			{
			  __MPC_Irecv (((char *) recvbuf) + (i * recvcount * dsize), recvcount, recvtype, i, MPC_GATHER_TAG, comm, &(recvrequest[j]), task_specific);
			  i++;
			  j++;
			}
		  j--;
		  for (; j >= 0; j--)
			{
			  __MPC_Wait (&(recvrequest[j]), MPC_STATUS_IGNORE);
			}
		}
		}
	  __MPC_Wait (&(request), MPC_STATUS_IGNORE);
	}
  MPC_ERROR_SUCESS ();
}

int
PMPC_Gather (void *sendbuf, mpc_msg_count sendcnt, MPC_Datatype sendtype,
	     void *recvbuf, mpc_msg_count recvcount, MPC_Datatype recvtype,
	     int root, MPC_Comm comm)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Gather);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Gather send %p,%lu,%d recv %p,%lu,%d root=%d",
		 sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype,
		 root);
#endif
  res = __MPC_Gather (sendbuf, sendcnt, sendtype, recvbuf, recvcount,
		      recvtype, root, comm, task_specific);
  SCTK_PROFIL_END (MPC_Gather);
  return res;
}

static inline int
__MPC_Allgather (void *sendbuf, mpc_msg_count sendcount,
		 MPC_Datatype sendtype, void *recvbuf,
		 mpc_msg_count recvcount, MPC_Datatype recvtype,
		 MPC_Comm comm, sctk_task_specific_t * task_specific)
{
  int size, rank, remote_size;
  int root = 0;
  void *tmp_buf;
  mpc_check_comm (comm, comm);
  mpc_check_buf (sendbuf, comm);
  mpc_check_buf (recvbuf, comm);
  mpc_check_count (recvcount, comm);
  mpc_check_type (sendtype, comm);
  mpc_check_type (recvtype, comm);
  mpc_check_count (sendcount, comm);
  __MPC_Comm_rank_size(comm, &rank, &size, task_specific);
  remote_size = sctk_get_nb_task_remote(comm);
  
	if(sctk_is_inter_comm(comm))
	{
		if(rank == 0 && sendcount > 0)
		{
			tmp_buf = (void *)sctk_malloc(sendcount*size*sizeof(void *));
		}
		
		__MPC_Gather (sendbuf, sendcount, sendtype, tmp_buf, sendcount, sendtype, 0, sctk_get_local_comm_id(comm), task_specific);

		if(sctk_is_in_local_group(comm))
		{
			if(sendcount != 0)
			{
				root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
				sctk_nodebug("bcast size %d to the left",size*sendcount); 
				__MPC_Bcast(tmp_buf, size * sendcount, sendtype, root, comm, task_specific);
			}
			
			if(recvcount != 0)
			{
				root = 0;
				sctk_nodebug("bcast size %d from the left", remote_size * recvcount);
				__MPC_Bcast(recvbuf, remote_size * recvcount, recvtype, root, comm, task_specific);
			}
		}
		else
		{
			if(recvcount != 0)
			{
				root = 0;
				sctk_nodebug("bcast size %d from the right", remote_size * recvcount);
				__MPC_Bcast(recvbuf, remote_size * recvcount, recvtype, root, comm, task_specific);
			}
			
			if(sendcount != 0)
			{
				sctk_nodebug("bcast size %d to the right",size*sendcount); 
				root = (rank == 0) ? MPC_ROOT : MPC_PROC_NULL;
				__MPC_Bcast(tmp_buf, size * sendcount, sendtype, root, comm, task_specific);
			}
		}
	}
	else
	{
		__MPC_Gather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, task_specific);
		__MPC_Bcast (recvbuf, size * recvcount, recvtype, root, comm, task_specific);
	}
	MPC_ERROR_SUCESS ();
}

int
PMPC_Allgather (void *sendbuf, mpc_msg_count sendcount,
		MPC_Datatype sendtype, void *recvbuf,
		mpc_msg_count recvcount, MPC_Datatype recvtype, MPC_Comm comm)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Allgather);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Allgather send %p,%lu,%d recv %p,%lu,%d",
		 sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
#endif
  res = __MPC_Allgather (sendbuf, sendcount, sendtype, recvbuf, recvcount,
			 recvtype, comm, task_specific);
  SCTK_PROFIL_END (MPC_Allgather);
  return res;
}

int
PMPC_Scatterv (void *sendbuf,
	       mpc_msg_count * sendcnts,
	       mpc_msg_count * displs,
	       MPC_Datatype sendtype,
	       void *recvbuf,
	       mpc_msg_count recvcnt, MPC_Datatype recvtype, int root,
	       MPC_Comm comm)
{
  int i;
  int j;
  int rank;
  int size;
  MPC_Request request;
  MPC_Request sendrequest[MPC_MAX_CONCURENT];
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Scatterv);
  task_specific = __MPC_get_task_specific ();

  __MPC_Comm_rank_size (comm, &rank, &size, task_specific);

  mpc_check_comm (comm, comm);
  mpc_check_task (root, comm, size);

  if (rank == root)
    {
      mpc_check_buf (sendbuf, comm);
      mpc_check_type (sendtype, comm);
    }

  mpc_check_type (recvtype, comm);
  mpc_check_buf (recvbuf, comm);
  mpc_check_count (recvcnt, comm);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Scatterv");
#endif

	if(sctk_is_inter_comm(comm))
	{
		if(root == MPC_PROC_NULL)
		{
			MPC_ERROR_SUCESS ();
		}
		else if(root == MPC_ROOT)
		{
			size_t send_type_size;
			send_type_size = __MPC_Get_datatype_size (sendtype, task_specific);
			i = 0;
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Isend (((char *) sendbuf) + (displs[i] * send_type_size), sendcnts[i], sendtype, i, MPC_SCATTERV_TAG, comm, &(sendrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(sendrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}
		else
		{
			__MPC_Irecv (recvbuf, recvcnt, recvtype, root, MPC_SCATTERV_TAG, comm, &request, task_specific);
			__MPC_Wait (&(request), MPC_STATUS_IGNORE);
		}  
	}
	else
	{
		__MPC_Irecv (recvbuf, recvcnt, recvtype, root, MPC_SCATTERV_TAG, comm, &request, task_specific);


		if (rank == root)
		{
			size_t send_type_size;
			send_type_size = __MPC_Get_datatype_size (sendtype, task_specific);
			i = 0;
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Isend (((char *) sendbuf) + (displs[i] * send_type_size), sendcnts[i], sendtype, i, MPC_SCATTERV_TAG, comm, &(sendrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(sendrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}

		__MPC_Wait (&(request), MPC_STATUS_IGNORE);
	}
	SCTK_PROFIL_END (MPC_Scatterv);
	MPC_ERROR_SUCESS ();
}

int
PMPC_Scatter (void *sendbuf,
	      mpc_msg_count sendcnt,
	      MPC_Datatype sendtype,
	      void *recvbuf,
	      mpc_msg_count recvcnt, MPC_Datatype recvtype, int root,
	      MPC_Comm comm)
{
  int i;
  int j;
  int rank;
  int size;
  MPC_Request request;
  size_t dsize;
  MPC_Request sendrequest[MPC_MAX_CONCURENT];
  sctk_task_specific_t *task_specific;
  
  //~ if(sendcnt != recvcnt)
	//~ MPC_ERROR_REPORT(comm, MPC_ERR_COUNT, "sendcount and recvcount must be the same");
  
  SCTK_PROFIL_START (MPC_Scatter);
  task_specific = __MPC_get_task_specific ();

  mpc_check_comm (comm, comm);
  __MPC_Comm_rank_size (comm, &rank, &size, task_specific);
  mpc_check_buf (sendbuf, comm);
  mpc_check_buf (recvbuf, comm);
  mpc_check_type (sendtype, comm);
  mpc_check_type (recvtype, comm);
  mpc_check_count (sendcnt, comm);
  mpc_check_count (recvcnt, comm);
  mpc_check_task (root, comm, size);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Scatter");
#endif

	if(sctk_is_inter_comm(comm))
	{
		if(root == MPC_PROC_NULL)
		{
			MPC_ERROR_SUCESS ();
		}
		else if(root == MPC_ROOT)
		{
			i = 0;
			dsize = __MPC_Get_datatype_size (recvtype, task_specific);
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Isend (((char *) sendbuf) + (i * sendcnt *dsize), sendcnt, sendtype, i, MPC_SCATTER_TAG, comm, &(sendrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(sendrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}
		else
		{
			__MPC_Irecv (recvbuf, recvcnt, recvtype, root, MPC_SCATTER_TAG, comm, &request, task_specific);
			__MPC_Wait (&(request), MPC_STATUS_IGNORE);
		}  
	}
	else
	{
		__MPC_Irecv (recvbuf, recvcnt, recvtype, root, MPC_SCATTER_TAG, comm, &request, task_specific);
		__MPC_Comm_rank_size (comm, &rank, &size, task_specific);

		if (rank == root)
		{
			i = 0;
			dsize = __MPC_Get_datatype_size (sendtype, task_specific);
			sctk_nodebug("sendcnt = %d, dsize = %d", sendcnt, dsize);
			while (i < size)
			{
				for (j = 0; (i < size) && (j < MPC_MAX_CONCURENT);)
				{
					__MPC_Isend (((char *) sendbuf) + (i * sendcnt *dsize), sendcnt, sendtype, i, MPC_SCATTER_TAG, comm, &(sendrequest[j]), task_specific);
					i++;
					j++;
				}
				j--;
				for (; j >= 0; j--)
				{
					__MPC_Wait (&(sendrequest[j]), MPC_STATUS_IGNORE);
				}
			}
		}

		__MPC_Wait (&(request), MPC_STATUS_IGNORE);
	}
	SCTK_PROFIL_END (MPC_Scatter);
	MPC_ERROR_SUCESS ();
}

int
PMPC_Alltoall (void *sendbuf, mpc_msg_count sendcount, MPC_Datatype sendtype,
	       void *recvbuf, mpc_msg_count recvcnt, MPC_Datatype recvtype,
	       MPC_Comm comm)
{
  int i;
  int size;
  int rank;
  int bblock = 4;
  MPC_Request requests[2*bblock*sizeof(MPC_Request)];
  int ss, ii;
  int dst;
  size_t d_send;
  size_t d_recv;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Alltoall);
  task_specific = __MPC_get_task_specific ();

  mpc_check_buf (sendbuf, comm);
  mpc_check_type (sendtype, comm);
  mpc_check_type (recvtype, comm);
  mpc_check_buf (recvbuf, comm);
  mpc_check_count (recvcnt, comm);
  mpc_check_count (sendcount, comm);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Alltoall");
#endif

	if(sctk_is_inter_comm(comm))
	{
		int local_size, remote_size, max_size, i;
		MPC_Status status;
		size_t sendtype_extent, recvtype_extent;
		int src, dst, rank;
		char *sendaddr, *recvaddr;
		
		__MPC_Comm_size (comm, &local_size);
		__MPC_Comm_rank (comm, &rank, task_specific);
		__MPC_Comm_remote_size(comm, &remote_size);
		
		PMPC_Type_size(sendtype, &sendtype_extent);
		PMPC_Type_size(recvtype, &recvtype_extent);
		
		max_size = (local_size < remote_size) ? remote_size : local_size;
		for (i=0; i<max_size; i++) 
		{
			src = (rank - i + max_size) % max_size;
			dst = (rank + i) % max_size;
			
			if (src >= remote_size) 
			{
				src = MPC_PROC_NULL;
				recvcnt = 0;
				recvaddr = NULL;
			}
			else 
			{
				recvaddr = (char *)recvbuf + src*recvcnt*recvtype_extent;
			}
			
			if (dst >= remote_size) 
			{
				dst = MPC_PROC_NULL;
				sendcount = 0;
				sendaddr = NULL;
			}
			else 
			{
				sendaddr = (char *)sendbuf + dst*sendcount*sendtype_extent;
			}
				
			PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, 47, recvaddr, recvcnt, recvtype, src, 47, comm, &status);
		}
	}
	else
	{
		__MPC_Comm_size (comm, &size);
		__MPC_Comm_rank (comm, &rank, task_specific);
		d_send = __MPC_Get_datatype_size (sendtype, task_specific);
		d_recv = __MPC_Get_datatype_size (recvtype, task_specific);

		for (ii=0; ii<size;ii+=bblock) 
		{
			ss = size-ii < bblock ? size-ii : bblock;
			for (i=0; i<ss; ++i) 
			{
				dst = (rank+i+ii) % size;
				__MPC_Irecv (((char *) recvbuf) + (dst * recvcnt * d_recv), recvcnt, recvtype, dst, MPC_ALLTOALL_TAG, comm, &requests[i], task_specific);
			}
			for (i=0; i<ss; ++i)
			{
				dst = (rank-i-ii+size) % size;
				__MPC_Isend (((char *) sendbuf) + (dst * sendcount * d_send), sendcount, sendtype, dst, MPC_ALLTOALL_TAG, comm, &requests[i+ss], task_specific);
			}
			__MPC_Waitall(2*ss,requests,MPC_STATUS_IGNORE);
		}
	}
	SCTK_PROFIL_END (MPC_Alltoall);
	MPC_ERROR_SUCESS ();
}

int
PMPC_Alltoallv (void *sendbuf,
		mpc_msg_count * sendcnts,
		mpc_msg_count * sdispls,
		MPC_Datatype sendtype,
		void *recvbuf,
		mpc_msg_count * recvcnts,
		mpc_msg_count * rdispls, MPC_Datatype recvtype, MPC_Comm comm)
{
  int i;
  int size;
  int rank;
  int bblock = 4;
  MPC_Request requests[2*bblock*sizeof(MPC_Request)];
  int ss, ii;
  int dst;
  size_t d_send;
  size_t d_recv;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Alltoallv);
  task_specific = __MPC_get_task_specific ();

  __MPC_Comm_size (comm, &size);
  __MPC_Comm_rank (comm, &rank, task_specific);
  mpc_check_buf (sendbuf, comm);
  mpc_check_type (sendtype, comm);
  mpc_check_type (recvtype, comm);
  mpc_check_buf (recvbuf, comm);

  sctk_nodebug("[%d] Start all to all, size %d", rank, size);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Alltoallv");
#endif
	if(sctk_is_inter_comm(comm))
	{
		int local_size, remote_size, max_size, i;
		MPC_Status status;
		size_t sendtype_extent, recvtype_extent;
		int src, dst, rank, sendcount, recvcount;
		char *sendaddr, *recvaddr;
		
		__MPC_Comm_size (comm, &local_size);
		__MPC_Comm_rank (comm, &rank, task_specific);
		__MPC_Comm_remote_size(comm, &remote_size);
		
		PMPC_Type_size(sendtype, &sendtype_extent);
		PMPC_Type_size(recvtype, &recvtype_extent);
		
		max_size = (local_size < remote_size) ? remote_size : local_size;
		for (i=0; i<max_size; i++) 
		{
			src = (rank - i + max_size) % max_size;
			dst = (rank + i) % max_size;
			if (src >= remote_size) 
			{
				src = MPC_PROC_NULL;
				recvaddr = NULL;
				recvcount = 0;
			}
			else 
			{
				recvaddr = (char *)recvbuf + rdispls[src]*recvtype_extent;
				recvcount = recvcnts[src];
			}
			if (dst >= remote_size) 
			{
				dst = MPC_PROC_NULL;
				sendaddr = NULL;
				sendcount = 0;
			}
			else 
			{
				sendaddr = (char *)sendbuf + sdispls[dst]*sendtype_extent;
				sendcount = sendcnts[dst];
			}

			PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, 48, recvaddr, recvcount, recvtype, src, 48, comm, &status);
		}
	}
	else
	{
		d_send = __MPC_Get_datatype_size (sendtype, task_specific);
		d_recv = __MPC_Get_datatype_size (recvtype, task_specific);
		for (ii=0; ii<size;ii+=bblock) 
		{
			ss = size-ii < bblock ? size-ii : bblock;
			for (i=0; i<ss; ++i) 
			{
				dst = (rank+i+ii) % size;
				sctk_nodebug("[%d] Send to %d", rank, dst);
				__MPC_Irecv (((char *) recvbuf) + (rdispls[dst] * d_recv), recvcnts[dst], recvtype, dst, MPC_ALLTOALLV_TAG, comm, &requests[i], task_specific);
			}
			for (i=0; i<ss; ++i) {
				dst = (rank-i-ii+size) % size;
				sctk_nodebug("[%d] Recv from %d", rank, dst);
				__MPC_Isend (((char *) sendbuf) + (sdispls[dst] * d_send), sendcnts[dst], sendtype, dst, MPC_ALLTOALLV_TAG, comm, &requests[i+ss], task_specific);
			}
			__MPC_Waitall(2*ss,requests,MPC_STATUS_IGNORE);
		}
	}

	sctk_nodebug("[%d] end all to all", rank);
	SCTK_PROFIL_END (MPC_Alltoallv);
	MPC_ERROR_SUCESS ();
}

int
PMPC_Alltoallw (const void *sendbuf, const int sendcounts[],
                  const int sdispls[], const MPC_Datatype sendtypes[],
                  void *recvbuf, const int recvcounts[], const int rdispls[],
                  const MPC_Datatype recvtypes[], MPC_Comm comm)
{
	sctk_task_specific_t *task_specific;
	SCTK_PROFIL_START (MPC_Alltoallv);
	task_specific = __MPC_get_task_specific ();

	mpc_check_buf (sendbuf, comm);
	mpc_check_buf (recvbuf, comm);

	sctk_nodebug("[%d] Start all to all, size %d", rank, size);
#ifdef MPC_LOG_DEBUG
	mpc_log_debug (comm, "MPC_Alltoallv");
#endif
	if(sctk_is_inter_comm(comm))
	{
		int local_size, remote_size, max_size, i;
		MPC_Status status;
		MPC_Datatype sendtype, recvtype;
		int src, dst, rank, sendcount, recvcount;
		char *sendaddr, *recvaddr;
		
		__MPC_Comm_size (comm, &local_size);
		__MPC_Comm_rank (comm, &rank, task_specific);
		__MPC_Comm_remote_size(comm, &remote_size);
		
		max_size = (local_size < remote_size) ? remote_size : local_size;
		for (i=0; i<max_size; i++) 
		{
			src = (rank - i + max_size) % max_size;
			dst = (rank + i) % max_size;
			if (src >= remote_size) 
			{
				src = MPC_PROC_NULL;
				recvaddr = NULL;
				recvcount = 0;
				recvtype = MPC_DATATYPE_NULL;
			}
			else 
			{
				recvaddr = (char *)recvbuf + rdispls[src];
				recvcount = recvcounts[src];
				recvtype = recvtypes[src];
			}
			if (dst >= remote_size) 
			{
				dst = MPC_PROC_NULL;
				sendaddr = NULL;
				sendcount = 0;
				sendtype = MPC_DATATYPE_NULL;
			}
			else 
			{
				sendaddr = (char *)sendbuf+sdispls[dst];
				sendcount = sendcounts[dst];
				sendtype = sendtypes[dst];
			}

			PMPC_Sendrecv(sendaddr, sendcount, sendtype, dst, 49, recvaddr, recvcount, recvtype, src, 49, comm, &status);
		}
	}
	else
	{
		int i, ii, ss, dst;
		int type_size, size; 
		int rank, bblock = 4;
		MPC_Status status;
		MPC_Status *starray;
		MPC_Request *reqarray;
		int outstanding_requests;
		__MPC_Comm_size (comm, &size);
		__MPC_Comm_rank (comm, &rank, task_specific);
		
		starray  = (MPC_Status*)  sctk_malloc(2*bblock*sizeof(MPC_Status));
        reqarray = (MPC_Request*) sctk_malloc(2*bblock*sizeof(MPC_Request));
        
		for (ii=0; ii<size; ii+=bblock) 
		{
            outstanding_requests = 0;
            ss = size-ii < bblock ? size-ii : bblock;

            for ( i=0; i<ss; i++ ) 
            { 
                dst = (rank+i+ii) % size;
                if (recvcounts[dst]) 
                {
                    type_size = __MPC_Get_datatype_size(recvtypes[dst], task_specific);
                    if (type_size) 
                    {
                        PMPC_Irecv((char *)recvbuf+rdispls[dst], recvcounts[dst], recvtypes[dst], dst, 49, comm, &reqarray[outstanding_requests]);
                        outstanding_requests++;
                    }
                }
            }

            for ( i=0; i<ss; i++ ) 
            { 
                dst = (rank-i-ii+size) % size;
                if (sendcounts[dst]) 
                {
                    type_size = __MPC_Get_datatype_size(recvtypes[dst], task_specific);
                    if (type_size) 
                    {
                        PMPC_Isend((char *)sendbuf+sdispls[dst], sendcounts[dst], sendtypes[dst], dst, 49, comm, &reqarray[outstanding_requests]);
                        outstanding_requests++;
                    }
                }
            }

            __MPC_Waitall(outstanding_requests, reqarray, starray);
        }
	}

	sctk_nodebug("[%d] end all to all", rank);
	SCTK_PROFIL_END (MPC_Alltoallv);
	MPC_ERROR_SUCESS ();
}

static inline int
MPC_Iprobe_inter (const int source, const int destination,
		  const int tag, const MPC_Comm comm,
		  int *flag, MPC_Status * status)
{
  sctk_thread_message_header_t msg;
  MPC_Status status_init = MPC_STATUS_INIT;

  mpc_check_comm (comm, comm);
  *status = status_init;
  *flag = 0;

  sctk_assert (status != MPC_STATUS_IGNORE);
  if ((source == MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG))
    {
      sctk_probe_any_source_any_tag (destination, comm, flag, &msg);
      status->MPC_SOURCE = msg.source;
      status->MPC_TAG = msg.message_tag;
      status->count = (mpc_msg_count) msg.msg_size;
      MPC_ERROR_SUCESS ();
    }
  if ((source != MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG))
    {
      msg.message_tag = tag;
      sctk_probe_source_tag (destination, source, comm, flag, &msg);
      status->MPC_SOURCE = msg.source;
      status->MPC_TAG = msg.message_tag;
      status->count = (mpc_msg_count) msg.msg_size;
      MPC_ERROR_SUCESS ();
    }
  if ((source != MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG))
    {
      sctk_probe_source_any_tag (destination, source, comm, flag, &msg);
      status->MPC_SOURCE = msg.source;
      status->MPC_TAG = msg.message_tag;
      status->count = (mpc_msg_count) msg.msg_size;
      MPC_ERROR_SUCESS ();
    }
  if ((source == MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG))
    {
      msg.message_tag = tag;
      sctk_probe_any_source_tag (destination, comm, flag, &msg);
      status->MPC_SOURCE = msg.source;
      status->MPC_TAG = msg.message_tag;
      status->count = (mpc_msg_count) msg.msg_size;
      MPC_ERROR_SUCESS ();
    }

  fprintf (stderr, "source = %d tag = %d\n", source, tag);
  not_reachable ();
  MPC_ERROR_SUCESS ();
}

int
PMPC_Iprobe (int source, int tag, MPC_Comm comm, int *flag,
	     MPC_Status * status)
{
  int destination;
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Iprobe);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);
  __MPC_Comm_rank (comm, &destination, task_specific);
  res = MPC_Iprobe_inter (source, destination, tag, comm, flag, status);
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Iprobe source=%d tag=%d flag=%d", source, tag,
		 *flag);
#endif
  SCTK_PROFIL_END (MPC_Iprobe);
  return res;
}

typedef struct
{
  int flag;
  int source;
  int destination;
  int tag;
  MPC_Comm comm;
  MPC_Status *status;
} MPC_probe_struct_t;

static void
MPC_Probe_poll (MPC_probe_struct_t * arg)
{
  MPC_Iprobe_inter (arg->source, arg->destination, arg->tag, arg->comm,
		    &(arg->flag), arg->status);
}

static int
__MPC_Probe (int source, int tag, MPC_Comm comm, MPC_Status * status,
	     sctk_task_specific_t * task_specific)
{
  MPC_probe_struct_t probe_struct;
  __MPC_Comm_rank (comm, &probe_struct.destination, task_specific);

#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Probe source=%d tag=%d", source, tag);
#endif
  probe_struct.source = source;
  probe_struct.tag = tag;
  probe_struct.comm = comm;
  probe_struct.status = status;

  MPC_Iprobe_inter (source, probe_struct.destination, tag, comm,
		    &probe_struct.flag, status);

  if (probe_struct.flag != 1)
    {
      sctk_thread_wait_for_value_and_poll (&probe_struct.flag, 1,
					   (void (*)(void *))
					   MPC_Probe_poll, &probe_struct);
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Probe (int source, int tag, MPC_Comm comm, MPC_Status * status)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Probe);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);
  res = __MPC_Probe (source, tag, comm, status, task_specific);
  SCTK_PROFIL_END (MPC_Probe);
  return res;
}

int
PMPC_Get_count (MPC_Status * status, MPC_Datatype datatype,
		mpc_msg_count * count)
{
  SCTK_PROFIL_START (MPC_Get_count);
  size_t size;
  mpc_msg_count res;
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific ();
  sctk_assert (status != MPC_STATUS_IGNORE);
  sctk_nodebug ("%d %d", datatype, MPC_PACKED);
  size = __MPC_Get_datatype_size (datatype, task_specific);
  res = status->count / (mpc_msg_count) size;
  sctk_nodebug ("%lu / %lu = %lu", status->count, size, res);
  if (status->count % (mpc_msg_count) size != 0)
    {
      res = MPC_UNDEFINED;
    }
  *count = res;
  SCTK_PROFIL_END (MPC_Get_count);
  MPC_ERROR_SUCESS ();
}

#define HOSTNAME_SIZE 4096
int
PMPC_Get_processor_name (char *name, int *resultlen)
{
  SCTK_PROFIL_START (MPC_Get_processor_name);
  gethostname (name, HOSTNAME_SIZE);
  *resultlen = strlen (name);
  SCTK_PROFIL_END (MPC_Get_processor_name);
  MPC_ERROR_SUCESS ();
}

/*Groups*/
static inline int
__MPC_Comm_group (MPC_Comm comm, MPC_Group * group)
{
    int size;
    int i;
    __MPC_Comm_size (comm, &size);

    sctk_nodebug ("MPC_Comm_group");

    *group = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));

    (*group)->task_nb = size;
    (*group)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));

    for (i = 0; i < size; i++)
    {
		(*group)->task_list_in_global_ranks[i] = sctk_get_comm_world_rank(comm,i);
    }
	MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_group (MPC_Comm comm, MPC_Group * group)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Comm_group group=%p", group);
#endif
  return __MPC_Comm_group (comm, group);
}

/*Remote Groups*/
static inline int
__MPC_Comm_remote_group (MPC_Comm comm, MPC_Group * group)
{
	int size;
    int i;
    __MPC_Comm_remote_size (comm, &size);
    sctk_nodebug ("MPC_Comm_group");

    *group = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));

    (*group)->task_nb = size;
    (*group)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));

	for (i = 0; i < size; i++)
    {
		(*group)->task_list_in_global_ranks[i] = sctk_get_remote_comm_world_rank(comm, i);
    }
	MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_remote_group (MPC_Comm comm, MPC_Group * group)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Comm_remote_group group=%p", group);
#endif
  return __MPC_Comm_remote_group (comm, group);
}


static inline int
__MPC_Group_free (MPC_Group * group)
{
  if (*group != MPC_GROUP_NULL)
    {
      sctk_free ((*group)->task_list_in_global_ranks);
      sctk_free (*group);
      *group = MPC_GROUP_NULL;
    }
  MPC_ERROR_SUCESS ();
}

int
PMPC_Convert_to_intercomm (MPC_Comm comm, MPC_Group group)
{
  not_implemented ();
  MPC_ERROR_SUCESS ();
}

int
PMPC_Group_free (MPC_Group * group)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Group_free group=%p", group);
#endif
  return __MPC_Group_free (group);
}

int
PMPC_Group_incl (MPC_Group group, int n, int *ranks, MPC_Group * newgroup)
{
  int i;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Group_incl");
#endif

	(*newgroup) = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
	assume((*newgroup) != NULL);

	(*newgroup)->task_nb = n;
	(*newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (n * sizeof (int));
	assume(((*newgroup)->task_list_in_global_ranks) != NULL);
	
	for (i = 0; i < n; i++)
    {
		(*newgroup)->task_list_in_global_ranks[i] = group->task_list_in_global_ranks[ranks[i]];
		sctk_nodebug ("%d", group->task_list_in_global_ranks[ranks[i]]);
    }

  MPC_ERROR_SUCESS ();
}

int
PMPC_Group_difference (MPC_Group group1, MPC_Group group2,
		       MPC_Group * newgroup)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Group_difference");
#endif
	int size;
	int i, j, k;
	int not_in;
	
	for(i=0; i<group1->task_nb; i++)
		sctk_nodebug("group1[%d] = %d", i, group1->task_list_in_global_ranks[i]);
	for(i=0; i<group2->task_nb; i++)
		sctk_nodebug("group2[%d] = %d", i, group2->task_list_in_global_ranks[i]);
	/* get the size of newgroup */
	size = 0;
	for (i = 0; i < group1->task_nb; i++)
    {
		not_in = 1;
		for (j = 0; j < group2->task_nb; j++)
		{
			if(group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
				not_in = 0;
		}
		if(not_in)
			size++;
	}
	
	/* allocation */
	*newgroup = (MPC_Group) sctk_malloc (sizeof (MPC_Group_t));
	
	(*newgroup)->task_nb = size;
	if(size == 0)
	{
		MPC_ERROR_SUCESS ();
	}
	
	(*newgroup)->task_nb = size;
	(*newgroup)->task_list_in_global_ranks = (int *) sctk_malloc (size * sizeof (int));
	
	/* fill the newgroup */
	k=0;
	for (i = 0; i < group1->task_nb; i++)
    {
		not_in = 1;
		for (j = 0; j < group2->task_nb; j++)
		{
			if(group1->task_list_in_global_ranks[i] == group2->task_list_in_global_ranks[j])
				not_in = 0;
		}
		if(not_in)
		{
			(*newgroup)->task_list_in_global_ranks[k] = group1->task_list_in_global_ranks[i];
			k++;
		}
	}
	
	MPC_ERROR_SUCESS ();
}

/*Communicators*/
static inline int
__MPC_Comm_create_from_intercomm (MPC_Comm comm, MPC_Group group, MPC_Comm * comm_out,
		   int is_inter_comm)
{
	int rank;
	int i;
	int present = 0;
	sctk_task_specific_t *task_specific;
	task_specific = __MPC_get_task_specific ();
	mpc_check_comm (comm, comm);
	__MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);
	sctk_assert (comm != MPC_COMM_NULL);
	
	__MPC_Barrier (comm);

	for (i = 0; i < group->task_nb; i++)
    {
		if (group->task_list_in_global_ranks[i] == rank)
		{
			sctk_nodebug("%d present", rank);
			present = 1;
			break;
		}
    }
	
	for(i = 0 ; i < group->task_nb ; i++)
	{
		sctk_nodebug("task_list[%d] = %d", i, group->task_list_in_global_ranks[i]);
	}
	
	if(sctk_is_in_local_group(comm))
		(*comm_out) = sctk_create_communicator_from_intercomm (comm, group->task_nb, group->task_list_in_global_ranks, 1);
	else
		(*comm_out) = sctk_create_communicator_from_intercomm (comm, group->task_nb, group->task_list_in_global_ranks, 0);
    sctk_nodebug("\tnew comm from intercomm %d", *comm_out);
	__MPC_Barrier (comm);
	
	if (present == 1)
    {
		sctk_nodebug("from intercomm barrier comm %d", *comm_out);
		__MPC_Barrier (*comm_out);
		sctk_nodebug("sortie barrier");
		sctk_thread_createnewspecific_mpc_per_comm(task_specific,*comm_out,comm);
    }
    sctk_nodebug("comm %d created from intercomm %d", *comm_out, comm);
	MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_create_from_intercomm (MPC_Comm comm, MPC_Group group, MPC_Comm * comm_out)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Comm_create_from_intercomm comm_out=%p", comm_out);
#endif
  return __MPC_Comm_create_from_intercomm (comm, group, comm_out, 0);
}

static inline int
__MPC_Comm_create (MPC_Comm comm, MPC_Group group, MPC_Comm * comm_out,
		   int is_inter_comm)
{
	int rank;
	int i;
	int present = 0;
	sctk_task_specific_t *task_specific;
	task_specific = __MPC_get_task_specific ();
	mpc_check_comm (comm, comm);
	__MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);
	sctk_assert (comm != MPC_COMM_NULL);
	
	__MPC_Barrier (comm);

	for (i = 0; i < group->task_nb; i++)
    {
		if (group->task_list_in_global_ranks[i] == rank)
		{
			present = 1;
			break;
		}
    }
		
	(*comm_out) = sctk_create_communicator (comm, group->task_nb, group->task_list_in_global_ranks, is_inter_comm);
    sctk_nodebug("\tnew comm -> %d", *comm_out);
	__MPC_Barrier (comm);
	
	if (present == 1)
    {
		sctk_nodebug("rank %d : barrier comm %d", rank, *comm_out);
		__MPC_Barrier (*comm_out);
		sctk_thread_createnewspecific_mpc_per_comm(task_specific,*comm_out,comm);
    }
	MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_create (MPC_Comm comm, MPC_Group group, MPC_Comm * comm_out)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Comm_create comm_out=%p", comm_out);
#endif
  return __MPC_Comm_create (comm, group, comm_out, 0);
}

static inline int
__MPC_Intercomm_create (MPC_Comm local_comm, int local_leader,
		       MPC_Comm peer_comm, int remote_leader, int tag,
		       MPC_Comm * newintercomm)
{
	int rank, grank;
	int i;
	int present = 0;
	sctk_task_specific_t *task_specific;
	task_specific = __MPC_get_task_specific ();
	mpc_check_comm (local_comm, local_comm);
	mpc_check_comm (peer_comm, peer_comm);
	int *task_list;
	int *remote_task_list;
	int size;
	int other_leader;
	int first = 0;
	MPC_Status status;
	sctk_nodebug("Intercomm create orig comm %d", local_comm);
	__MPC_Comm_rank (MPC_COMM_WORLD, &rank, task_specific);
	__MPC_Comm_rank (local_comm, &grank, task_specific);
	__MPC_Comm_size(local_comm, &size);
	sctk_assert (local_comm != MPC_COMM_NULL);
	
	task_list = __MPC_Comm_task_list(local_comm, size);
	
	if(grank == local_leader)
	{
		sctk_nodebug("rank %d, grank %d : sendrecv local_leader %d, remote leader %d", rank, grank, local_leader, remote_leader);
		PMPC_Sendrecv(&remote_leader, 1, MPC_INT, remote_leader, tag, &other_leader, 1, MPC_INT, remote_leader, tag, peer_comm, &status);
	}
	
	sctk_nodebug("broadcast rank %d, grank %d, comm %d", rank, grank, local_comm);
	PMPC_Bcast(&other_leader, 1, MPC_INT, local_leader, local_comm);
	sctk_nodebug("broadcast done");
	if(other_leader < remote_leader)
		first = 1;
	else
		first = 0;
	
	(*newintercomm) = sctk_create_intercommunicator (local_comm, local_leader, peer_comm, remote_leader, tag, first);
	sctk_nodebug("new intercomm %d", *newintercomm);
	
	__MPC_Barrier (local_comm);

	for (i = 0; i < size; i++)
    {
		if (task_list[i] == rank)
		{
			present = 1;
			break;
		}
    }
	if(present)
	{
		sctk_nodebug("create specific mpc per comm");
		sctk_thread_createspecific_mpc_per_comm_from_existing(task_specific,*newintercomm, local_comm);
		sctk_nodebug("barrier on local_comm");
		__MPC_Barrier (local_comm);
		sctk_nodebug("done");
	}
	
	MPC_ERROR_SUCESS ();
}

int
PMPC_Intercomm_create (MPC_Comm local_comm, int local_leader,
		       MPC_Comm peer_comm, int remote_leader, int tag,
		       MPC_Comm * newintercomm)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Intercomm_create comm_out=%p", comm_out);
#endif
  return __MPC_Intercomm_create (local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

int
PMPC_Comm_create_list (MPC_Comm comm, int *list, int nb_elem,
		       MPC_Comm * comm_out)
{
  MPC_Group_t group;
  int i;

  group.task_nb = nb_elem;
  group.task_list_in_global_ranks = sctk_malloc(nb_elem*sizeof(int));

  for(i = 0; i < nb_elem; i++){
    group.task_list_in_global_ranks[i] = sctk_get_comm_world_rank(comm,list[i]);
  }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Comm_create_list nb_elem=%d comm_out=%p",
		 nb_elem, comm_out);
#endif
  __MPC_Comm_create (comm, &group, comm_out, 0);

  sctk_free(group.task_list_in_global_ranks);

  MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Comm_free (MPC_Comm * comm)
{
  sctk_task_specific_t *task_specific;
  task_specific = __MPC_get_task_specific ();
  if (*comm == MPC_COMM_WORLD)
  {
      MPC_ERROR_SUCESS ();
    }
  MPC_Comm old_comm = *comm;
  *comm = MPC_COMM_NULL;
  INFO("Comm free disabled")
  MPC_ERROR_SUCESS();
  sctk_nodebug ("Comm free %d", old_comm);
  mpc_check_comm (old_comm, MPC_COMM_WORLD);
  sctk_assert (old_comm != MPC_COMM_NULL);
  __MPC_Barrier (old_comm);
  sctk_nodebug ("Delete Comm %d", old_comm);
  sctk_delete_communicator (old_comm);
  sctk_nodebug ("Comm free done %d",  old_comm);
  sctk_thread_removespecific_mpc_per_comm(task_specific, old_comm);

  *comm = MPC_COMM_NULL;
  sctk_nodebug("COMM FREE DONE");
  MPC_ERROR_SUCESS ();
}

int
PMPC_Comm_free (MPC_Comm * comm)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (*comm, "MPC_Comm_free");
#endif
  return __MPC_Comm_free (comm);
}

int
PMPC_Comm_dup (MPC_Comm comm, MPC_Comm * comm_out)
{
	sctk_nodebug("duplicate comm %d", comm);
	sctk_task_specific_t *task_specific;
	MPC_Group group;
	int rank;
	mpc_check_comm (comm, comm);
#ifdef MPC_LOG_DEBUG
	mpc_log_debug (comm, "MPC_Comm_dup out_comm=%p", comm_out);
#endif

	task_specific = __MPC_get_task_specific ();
	__MPC_Comm_rank (comm, &rank, task_specific);
	*comm_out = sctk_duplicate_communicator (comm, sctk_is_inter_comm (comm), rank);
	if(sctk_is_inter_comm (comm))
		sctk_nodebug("intercomm %d duplicate --> newintercomm %d", comm, *comm_out);
	else
		sctk_nodebug("comm %d duplicate --> newcomm %d", comm, *comm_out);

	sctk_thread_createspecific_mpc_per_comm_from_existing_dup(task_specific, *comm_out, comm);
	MPC_ERROR_SUCESS ();
}


typedef struct
{
  int color;
  int key;
  int rank;
} mpc_comm_split_t;

int
PMPC_Comm_split (MPC_Comm comm, int color, int key, MPC_Comm * comm_out)
{
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
	task_specific = __MPC_get_task_specific ();

	mpc_check_comm (comm, comm);

#ifdef MPC_LOG_DEBUG
	mpc_log_debug (comm, "MPC_Comm_split color=%d key=%d out_comm=%p",
	color, key, comm_out);
#endif

	__MPC_Comm_size (comm, &size);
	__MPC_Comm_rank (comm, &rank, task_specific);
	sctk_nodebug("rank %d : PMPC_Comm_split with comm %d, color %d and key %d", rank, comm, color, key);
	tab = (mpc_comm_split_t *) sctk_malloc (size * sizeof (mpc_comm_split_t));
	color_tab = (int *) sctk_malloc (size * sizeof (int));

	tab_this.rank = rank;
	tab_this.color = color;
	tab_this.key = key;
	sctk_nodebug("Allgather...");
	__MPC_Allgather (&tab_this, sizeof (mpc_comm_split_t), MPC_CHAR, tab, sizeof (mpc_comm_split_t), MPC_CHAR, comm, task_specific);
	sctk_nodebug("done");
	/*Sort the new tab */
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size - 1; j++)
		{
			if (tab[j].color > tab[j + 1].color)
			{
				tab_tmp = tab[j + 1];
				tab[j + 1] = tab[j];
				tab[j] = tab_tmp;
			}
			else
			{
				if ((tab[j].color == tab[j + 1].color) && (tab[j].key > tab[j + 1].key))
				{
					tab_tmp = tab[j + 1];
					tab[j + 1] = tab[j];
					tab[j] = tab_tmp;
				}
			}
		}
	}

	color_number = 0;
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < color_number; j++)
		{
			if (color_tab[j] == tab[i].color)
			{
				break;
			}
		}
		if (j == color_number)
		{
			color_tab[j] = tab[i].color;
			color_number++;
		}
		sctk_nodebug ("rank %d color %d", i, tab[i].color, tab[i].rank);
	}

	sctk_nodebug ("%d colors", color_number);

	/*We need on comm_create per color */
	for (k = 0; k < color_number; k++)
	{
		tmp_color = color_tab[k];
		if (tmp_color != MPC_UNDEFINED)
		{
			group_size = 0;
			for (i = 0; i < size; i++)
			{
				if (tab[i].color == tmp_color)
				{
					group_size++;
				}
			}

			group.task_nb = group_size;
			group.task_list_in_global_ranks = (int *) sctk_malloc (group_size * sizeof (int));

			j = 0;
			for (i = 0; i < size; i++)
			{
				if (tab[i].color == tmp_color)
				{
					group.task_list_in_global_ranks[j] = sctk_get_comm_world_rank(comm,tab[i].rank);
					sctk_nodebug ("Thread %d color (%d) size %d on %d rank %d",
					tmp_color, k, j, group_size, tab[i].rank);
					j++;
				}
			}

			__MPC_Comm_create (comm, &group, comm_out, 0);
			if (*comm_out != MPC_COMM_NULL)
			{
				comm_res = *comm_out;
			}

			sctk_free (group.task_list_in_global_ranks);
			sctk_nodebug ("Split color %d done", tmp_color);
		}
	}

	sctk_free (color_tab);
	sctk_free (tab);
	sctk_nodebug ("Split done");
	*comm_out = comm_res;
	MPC_ERROR_SUCESS ();
}

/*Error_handler*/
int
PMPC_Errhandler_create (MPC_Handler_function * function,
			MPC_Errhandler * errhandler)
{
  *errhandler = function;
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Errhandler_create func=%p handler=%p", function,
		 errhandler);
#endif
  MPC_ERROR_SUCESS ();
}

int
PMPC_Errhandler_set (MPC_Comm comm, MPC_Errhandler errhandler)
{
  sctk_task_specific_t *task_specific;
  mpc_per_communicator_t* tmp;
  task_specific = __MPC_get_task_specific ();
  tmp = sctk_thread_getspecific_mpc_per_comm(task_specific,comm);

  sctk_spinlock_lock (&(tmp->err_handler_lock));
  tmp->err_handler = errhandler;
  sctk_spinlock_unlock (&(tmp->err_handler_lock));
  MPC_ERROR_SUCESS ();
}

int
PMPC_Errhandler_get (MPC_Comm comm, MPC_Errhandler * errhandler)
{
  sctk_task_specific_t *task_specific;
  mpc_per_communicator_t* tmp;
  task_specific = __MPC_get_task_specific ();
  tmp = sctk_thread_getspecific_mpc_per_comm(task_specific,comm);

  sctk_spinlock_lock (&(tmp->err_handler_lock));
  *errhandler = tmp->err_handler;
  sctk_spinlock_unlock (&(tmp->err_handler_lock));
  MPC_ERROR_SUCESS ();
}

int
PMPC_Errhandler_free (MPC_Errhandler * errhandler)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Errhandler_free handler=%p",
		 errhandler);
#endif
  *errhandler = (MPC_Errhandler) MPC_ERRHANDLER_NULL;
  MPC_ERROR_SUCESS ();
}

#define MPC_Error_string_convert(code,msg)	\
  case code: sprintf(str,msg); break

int
PMPC_Error_string (int code, char *str, int *len)
{
  size_t lngt;
  str[0] = '\0';
  switch (code)
    {
      MPC_Error_string_convert (MPC_ERR_BUFFER, "Invalid buffer pointer");
      MPC_Error_string_convert (MPC_ERR_COUNT, "Invalid count argument");
      MPC_Error_string_convert (MPC_ERR_TYPE, "Invalid datatype argument");
      MPC_Error_string_convert (MPC_ERR_TAG, "Invalid tag argument");
      MPC_Error_string_convert (MPC_ERR_COMM, "Invalid communicator");
      MPC_Error_string_convert (MPC_ERR_RANK, "Invalid rank");
      MPC_Error_string_convert (MPC_ERR_ROOT, "Invalid root");
      MPC_Error_string_convert (MPC_ERR_TRUNCATE,
				"Message truncated on receive");
      MPC_Error_string_convert (MPC_ERR_GROUP, "Invalid group");
      MPC_Error_string_convert (MPC_ERR_OP, "Invalid operation");
      MPC_Error_string_convert (MPC_ERR_REQUEST,
				"Invalid mpc_request handle");
      MPC_Error_string_convert (MPC_ERR_TOPOLOGY, "Invalid topology");
      MPC_Error_string_convert (MPC_ERR_DIMS, "Invalid dimension argument");
      MPC_Error_string_convert (MPC_ERR_ARG, "Invalid argument");
      MPC_Error_string_convert (MPC_ERR_OTHER,
				"Other error; use Error_string");
      MPC_Error_string_convert (MPC_ERR_UNKNOWN, "Unknown error");
      MPC_Error_string_convert (MPC_ERR_INTERN, "Internal error code");
      MPC_Error_string_convert (MPC_ERR_IN_STATUS,
				"Look in status for error value");
      MPC_Error_string_convert (MPC_ERR_PENDING, "Pending request");
      MPC_Error_string_convert (MPC_NOT_IMPLEMENTED, "Not implemented");
    default:
      sctk_warning ("%d error code unknown", code);
    }
  lngt = strlen (str);
  *len = (int) lngt;
  MPC_ERROR_SUCESS ();
}

int
PMPC_Error_class (int errorcode, int *errorclass)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Error_class errorcode=%d errorclass=%p", errorcode,
		 errorclass);
#endif
  *errorclass = errorcode;
  MPC_ERROR_SUCESS ();
}

/*Timing*/
double
PMPC_Wtime ()
{
  SCTK_PROFIL_START (MPC_Wtime);
  double res;
  struct timeval tp;

  gettimeofday (&tp, NULL);
  res = (double) tp.tv_usec + (double) tp.tv_sec * (double) 1000000;
  res = res / (double) 1000000;

  sctk_nodebug ("Wtime = %f", res);
  SCTK_PROFIL_END (MPC_Wtime);
  return res;
}

double
PMPC_Wtick ()
{
  /* sctk_warning ("Always return 0"); */
  return 10e-6;
}

/*Requests*/
int
PMPC_Request_free (MPC_Request * request)
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Request_free req=%p", request);
#endif

  /* Firstly wait the message before freeing */
  sctk_mpc_wait_message(request);
  *request = mpc_request_null;
  MPC_ERROR_SUCESS ();
}


int
PMPC_Proceed ()
{
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Proceed");
#endif

  sctk_thread_yield ();
  MPC_ERROR_SUCESS ();
}


/*Packs*/
int
PMPC_Open_pack (MPC_Request * request)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Open_pack);
  task_specific = __MPC_get_task_specific ();
  if (request == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Open_pack req=%p", request);
#endif

  __MPC_Comm_rank (MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header (src,sctk_message_pack_undefined);

  sctk_mpc_register_message_in_request(request,msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END (MPC_Open_pack);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Default_pack (mpc_msg_count count,
		   mpc_pack_indexes_t * begins, mpc_pack_indexes_t * ends,
		   MPC_Request * request)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Default_pack);
  task_specific = __MPC_get_task_specific ();
  if (request == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Default_pack count=%lu req=%p",
		 count, request);
#endif
  /*   sctk_mpc_init_pack_std(request,count,begins,ends); */

  __MPC_Comm_rank (MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header (src,sctk_message_pack);
  msg->tail.default_pack.std.count = count;
  msg->tail.default_pack.std.begins = begins;
  msg->tail.default_pack.std.ends = ends;

  sctk_mpc_register_message_in_request(request,msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END (MPC_Default_pack);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Default_pack_absolute (mpc_msg_count count,
			    mpc_pack_absolute_indexes_t * begins,
			    mpc_pack_absolute_indexes_t * ends,
			    MPC_Request * request)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Default_pack);
  task_specific = __MPC_get_task_specific ();
  if (request == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD, "MPC_Default_pack count=%lu req=%p",
		 count, request);
#endif
  /*   sctk_mpc_init_pack_absolute(request,count,begins,ends); */

  __MPC_Comm_rank (MPC_COMM_WORLD, &src, task_specific);

  msg = sctk_create_header (src,sctk_message_pack_absolute);
  msg->tail.default_pack.absolute.count = count;
  msg->tail.default_pack.absolute.begins = begins;
  msg->tail.default_pack.absolute.ends = ends;

  sctk_mpc_register_message_in_request(request,msg);
  sctk_mpc_init_message_size(request);
  SCTK_PROFIL_END (MPC_Default_pack);
  MPC_ERROR_SUCESS ();
}


static inline int
__MPC_Add_pack (void *buf, mpc_msg_count count,
		mpc_pack_indexes_t * begins, mpc_pack_indexes_t * ends,
		MPC_Datatype datatype, MPC_Request * request,
		sctk_task_specific_t * task_specific)
{
  sctk_thread_ptp_message_t *msg;
  int i;
  size_t data_size;
  size_t total = 0;

  if (request == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
  if (begins == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }
  if (ends == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }
  if (count == 0)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }

  data_size = __MPC_Get_datatype_size (datatype, task_specific);

  msg = sctk_mpc_get_message_in_request(request);

  sctk_add_pack_in_message (msg, buf, count, data_size, begins, ends);

  sctk_mpc_register_message_in_request(request,msg);

  /*Compute message size */
  for (i = 0; i < count; i++)
    {
      total += ends[i] - begins[i] + 1;
    }
  sctk_mpc_add_to_message_size(request,total * data_size);

  MPC_ERROR_SUCESS ();
}

static inline int
__MPC_Add_pack_absolute (void *buf, mpc_msg_count count,
			 mpc_pack_absolute_indexes_t * begins,
			 mpc_pack_absolute_indexes_t * ends,
			 MPC_Datatype datatype, MPC_Request * request,
			 sctk_task_specific_t * task_specific)
{
  sctk_thread_ptp_message_t *msg;
  int i;
  size_t data_size;
  size_t total = 0;

  data_size = __MPC_Get_datatype_size (datatype, task_specific);

  sctk_nodebug ("TYPE numer %d size %lu, count = %d", datatype, data_size, count);
  
  if (request == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
  if (begins == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }
  if (ends == NULL)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }
  if (count == 0)
    {
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_ARG, "");
    }

  msg = sctk_mpc_get_message_in_request(request);

  sctk_add_pack_in_message_absolute (msg, buf, count, data_size,
				       begins, ends);

  sctk_mpc_register_message_in_request(request,msg);

  /*Compute message size */
  for (i = 0; i < count; i++)
    {
      total += ends[i] - begins[i] + 1;
    }
    
  sctk_mpc_add_to_message_size(request,total * data_size);

  MPC_ERROR_SUCESS ();
}

int
PMPC_Add_pack_absolute (void *buf, mpc_msg_count count,
			mpc_pack_absolute_indexes_t * begins,
			mpc_pack_absolute_indexes_t * ends,
			MPC_Datatype datatype, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Add_pack_absolute);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Add_pack ptr=%p type=%d nb_item=%lu req=%p", buf,
		 datatype, count, request);
#endif

  /*   assume (buf == NULL); */

  res =
    __MPC_Add_pack_absolute (buf, count, begins, ends, datatype,
			     request, task_specific);
  SCTK_PROFIL_END (MPC_Add_pack_absolute);
  return res;
}

int
PMPC_Add_pack (void *buf, mpc_msg_count count,
	       mpc_pack_indexes_t * begins, mpc_pack_indexes_t * ends,
	       MPC_Datatype datatype, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Add_pack);
  task_specific = __MPC_get_task_specific ();
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Add_pack ptr=%p type=%d nb_item=%lu req=%p", buf,
		 datatype, count, request);
#endif
  res =
    __MPC_Add_pack (buf, count, begins, ends, datatype, request,
		    task_specific);
  SCTK_PROFIL_END (MPC_Add_pack);
  return res;
}

int
PMPC_Add_pack_default (void *buf, MPC_Datatype datatype, MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Add_pack_default);
  task_specific = __MPC_get_task_specific ();
  if (request == NULL)
    {
      SCTK_PROFIL_END (MPC_Add_pack_default);
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Add_pack_default ptr=%p type=%d req=%p", buf,
		 datatype, request);
#endif

  res = __MPC_Add_pack (buf, request->msg->tail.default_pack.std.count,
			request->msg->tail.default_pack.std.begins,
			request->msg->tail.default_pack.std.ends, datatype, request,
			task_specific);

  SCTK_PROFIL_END (MPC_Add_pack_default);
  return res;
}

int
PMPC_Add_pack_default_absolute (void *buf, MPC_Datatype datatype,
				MPC_Request * request)
{
  int res;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Add_pack_default);
  task_specific = __MPC_get_task_specific ();
  if (request == NULL)
    {
      SCTK_PROFIL_END (MPC_Add_pack_default);
      MPC_ERROR_REPORT (MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (MPC_COMM_WORLD,
		 "MPC_Add_pack_default ptr=%p type=%d req=%p", buf,
		 datatype, request);
#endif

  res = __MPC_Add_pack_absolute (buf, request->msg->tail.default_pack.absolute.count,
				 request->msg->tail.default_pack.absolute.begins,
				 request->msg->tail.default_pack.absolute.ends,
				 datatype, request, task_specific);

  SCTK_PROFIL_END (MPC_Add_pack_default);
  return res;
}

int
PMPC_Isend_pack (int dest, int tag, MPC_Comm comm, MPC_Request * request)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Isend_pack);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);

  __MPC_Comm_rank_size (comm, &src, &size, task_specific);

  if (dest == MPC_PROC_NULL)
    {
      sctk_mpc_init_request(request,comm,src, REQUEST_SEND);
      MPC_ERROR_SUCESS ();
    }
  if (request == NULL)
    {
      MPC_ERROR_REPORT (comm, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Isend_pack dest=%d tag=%d req=%p", dest, tag,
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
  request->request_type = REQUEST_SEND;
  sctk_mpc_set_header_in_message (msg,
				  tag,
				  comm,
				  src,
				  dest,
				  request,
				  sctk_mpc_get_message_size(request),pt2pt_specific_message_tag);
  sctk_send_message (msg);
  SCTK_PROFIL_END (MPC_Isend_pack);
  MPC_ERROR_SUCESS ();
}

int
PMPC_Irecv_pack (int source, int tag, MPC_Comm comm, MPC_Request * request)
{
  sctk_thread_ptp_message_t *msg;
  int src;
  int size;
  sctk_task_specific_t *task_specific;
  SCTK_PROFIL_START (MPC_Irecv_pack);
  task_specific = __MPC_get_task_specific ();
  mpc_check_comm (comm, comm);

  __MPC_Comm_rank_size (comm, &src, &size, task_specific);

  if (source == MPC_PROC_NULL)
    {
      sctk_mpc_init_request(request,comm,src, REQUEST_RECV);
      MPC_ERROR_SUCESS ();
    }

  if (request == NULL)
    {
      MPC_ERROR_REPORT (comm, MPC_ERR_REQUEST, "");
    }
#ifdef MPC_LOG_DEBUG
  mpc_log_debug (comm, "MPC_Irecv_pack source=%d tag=%d req=%p", source,
		 tag, request);
#endif
	
  msg = sctk_mpc_get_message_in_request(request);

  if (source != MPC_ANY_SOURCE)
    {
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
  request->request_type = REQUEST_RECV;
  sctk_mpc_set_header_in_message (msg,
				  tag,
				  comm,
				  source,
				  src,
				  request,
				  sctk_mpc_get_message_size(request),pt2pt_specific_message_tag);

  sctk_recv_message (msg,task_specific->my_ptp_internal, 0);
  SCTK_PROFIL_END (MPC_Irecv_pack);
  MPC_ERROR_SUCESS ();
}


int
PMPC_Main (int argc, char **argv)
{
  return sctk_launch_main (argc, argv);
}

int
PMPC_User_Main (int argc, char **argv)
{
  return mpc_user_main (argc, argv);
}

/********************************************************************/
/*    Network functions. These functions are not a part of the      */
/*    MPI standard.                                                 */
/********************************************************************/

/* For getting stats about the network */
void MPC_Network_stats(struct MPC_Network_stats_s *stats) {
#ifdef MPC_Message_Passing
#ifdef MPC_USE_INFINIBAND
  sctk_network_stats_ib(stats);
#endif
#endif
}

/* Send a message to a process using the signalization network */
void MPC_Send_signalization_network(int dest_process, int tag, void *buff, size_t size) {
#ifdef MPC_Message_Passing
    sctk_route_messages_send(sctk_process_rank,dest_process,user_specific_message_tag,
        tag, buff, size);
#endif
}

/* Recv a message from a process using the signalization network */
void MPC_Recv_signalization_network(int src_process, int tag, void *buff, size_t size) {
#ifdef MPC_Message_Passing
  sctk_route_messages_recv(src_process,sctk_process_rank,user_specific_message_tag,
        tag, buff, size);
#endif
}

