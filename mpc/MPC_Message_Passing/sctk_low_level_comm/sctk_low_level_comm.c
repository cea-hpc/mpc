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
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_launch.h"
#include "string.h"
#include "sctk_rpc.h"

/*Drivers*/
#include "sctk_none.h"
/* #include "sctk_mpi.h" */
#include "sctk_tcp.h"
#include "sctk_infiniband.h"
#include "sctk_shm.h"
#include "sctk_mpcrun_client.h"
#include "sctk_hybrid_comm.h"

void (*sctk_net_adm_poll) (void *) = NULL;
void (*sctk_net_ptp_poll) (void *) = NULL;
int sctk_mpcrun_client_port = 0;
char *sctk_mpcrun_client_host = NULL;

static void (*sctk_net_register_ptr) (void *addr, size_t size) = NULL;
static void (*sctk_net_unregister_ptr) (void *addr, size_t size) = NULL;

/*****/
 /*RPC*/
/*****/
void (*sctk_net_sctk_specific_adm_rpc_remote_ptr) (void * arg) = NULL;
void
sctk_net_sctk_specific_adm_rpc_remote (void * arg)
{
  if(sctk_net_sctk_specific_adm_rpc_remote_ptr){
    sctk_net_sctk_specific_adm_rpc_remote_ptr(arg);
  }
}


#warning "Optimize for scalability"
/*send_task_location*/
  typedef struct
{
  int task;
  int new_proc;
} send_task_location_t;

static void
sctk_net_send_task_location_remote (send_task_location_t * arg)
{
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_register_distant_thread (arg->task, arg->new_proc);
}

void
sctk_net_send_task_location (int task, int process)
{
  int i;
  send_task_location_t msg;
  sctk_nodebug ("%s", SCTK_FUNCTION);
  msg.task = task;
  msg.new_proc = process;
  for (i = 0; i < sctk_process_number; i++)
    {
      if (i != sctk_process_rank)
	{
	  sctk_perform_rpc ((sctk_rpc_t) sctk_net_send_task_location_remote,
			    i, &msg, sizeof (send_task_location_t));
	}
    }
}

#warning "Optimize for scalability"
/*send_task_end*/
typedef struct
{
  int task;
  int new_proc;
} send_task_end_t;

static void
sctk_net_send_task_end_remote (send_task_location_t * arg)
{
  assume (arg != NULL);
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);
}

void
sctk_net_send_task_end (int task, int process)
{
#if 0
  int i;
  send_task_location_t msg;

  sctk_nodebug ("%s", SCTK_FUNCTION);
  msg.task = task;
  msg.new_proc = process;
  for (i = 0; i < sctk_process_number; i++)
    {
      if (i != sctk_process_rank)
	{
	  sctk_perform_rpc ((sctk_rpc_t) sctk_net_send_task_end_remote, i,
			    &msg, sizeof (send_task_location_t));
	}
    }
#endif

  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);
}

static void
sctk_net_update_communicator_remote (sctk_update_communicator_t * arg)
{
  sctk_nodebug ("%s vp %d task_id %d com %d process %d", SCTK_FUNCTION,
		arg->vp, arg->task_id, arg->com, arg->src);
  sctk_init_collective_communicator (arg->vp, arg->task_id, arg->com,
				     arg->src);
}

void
sctk_net_update_communicator (int task, sctk_communicator_t comm, int vp)
{
  sctk_update_communicator_t msg;
  int i;
  if(comm != 0){
  sctk_nodebug ("%s vp %d task_id %d com %d process %d", SCTK_FUNCTION,
		vp, task, comm, sctk_process_rank);
  //TODO
  assume (task >= 0);
  msg.com = comm;
  msg.vp = vp;
  msg.task_id = task;
  msg.src = sctk_process_rank;
  for (i = 0; i < sctk_process_number; i++)
    {
      if (i != sctk_process_rank)
	{
	  sctk_perform_rpc ((sctk_rpc_t) sctk_net_update_communicator_remote,
			    i, &msg, sizeof (sctk_update_communicator_t));
	}
    }
  }
}

#warning "Optimize for scalability"
/*get_free_communicator*/
typedef struct
{
  sctk_communicator_t origin_communicator;
  int proc;
} sctk_get_free_communicator_t;

typedef struct
{
  sctk_communicator_t origin_communicator;
  int val;
} sctk_get_free_communicator_wait_t;

static void
sctk_net_get_free_communicator_remote (sctk_get_free_communicator_t * arg)
{
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_get_free_communicator_on_root (arg->origin_communicator);
}

static void
sctk_get_free_communicator_wait (sctk_get_free_communicator_wait_t * arg)
{
  sctk_internal_communicator_t *tmp = NULL;
  sctk_nodebug ("%s", SCTK_FUNCTION);
  tmp = sctk_get_communicator (arg->origin_communicator);
  if (tmp->new_communicator != (sctk_communicator_t)-1)
    {
      arg->val = 1;
    }
}

void
sctk_net_get_free_communicator (const sctk_communicator_t origin_communicator)
{
  sctk_get_free_communicator_t msg;
  sctk_get_free_communicator_wait_t wait_s;

  sctk_nodebug ("%s", SCTK_FUNCTION);
  wait_s.origin_communicator = origin_communicator;
  wait_s.val = 0;

  msg.origin_communicator = origin_communicator;
  msg.proc = sctk_process_rank;

  sctk_perform_rpc ((sctk_rpc_t) sctk_net_get_free_communicator_remote, 0,
		    &msg, sizeof (sctk_get_free_communicator_t));

  sctk_thread_wait_for_value_and_poll
    ((int *) &(wait_s.val), 1,
     (void (*)(void *)) sctk_get_free_communicator_wait, (void *) &wait_s);
}

#warning "Optimize for scalability"
/*update_new_communicator*/
typedef struct sctk_update_new_communicator_s
{
  sctk_communicator_t origin_communicator;
  int nb_task_involved;
  int process;
  void *addr;
  volatile int done;
  struct sctk_update_new_communicator_s *msg;
  int src;
  int check;
} sctk_update_new_communicator_t;
static void
sctk_net_update_new_communicator_remote (sctk_update_new_communicator_t * msg)
{
  int *task_list;

  sctk_internal_communicator_t* inter_com;

  sctk_nodebug ("%s", SCTK_FUNCTION);

  sctk_nodebug ("nb_involved %d", msg->nb_task_involved);
  task_list = sctk_malloc (msg->nb_task_involved * sizeof (int));
  memset (task_list, 0, msg->nb_task_involved * sizeof (int));

  sctk_nodebug ("USE %p", msg->addr);

  if (sctk_net_register_ptr && task_list)
    sctk_net_register_ptr (task_list, msg->nb_task_involved * sizeof (int));

  sctk_nodebug(" TEST : %d, %d", ((int*) msg->addr)[0], ((int*) msg->addr)[1]);

  sctk_perform_rpc_retrive (task_list, msg->addr,
      msg->nb_task_involved * sizeof (int), msg->src,
      (int *) &(msg->msg->done));

  if (msg->nb_task_involved != 0)
  {
    sctk_nodebug
      ("sctk_tools_recv_update_new_communicator %d task comm %d rank 0 %d rank 1 %d",
       msg->nb_task_involved, msg->origin_communicator, task_list[0],
       task_list[1]);
    inter_com = sctk_update_new_communicator (msg->origin_communicator,
        msg->nb_task_involved, task_list);

    sctk_net_hybrid_init_new_com(inter_com, msg->nb_task_involved, task_list);
  }
  else
  {
    sctk_net_hybrid_free_com(msg->origin_communicator);

    sctk_update_free_communicator (msg->origin_communicator);
  }
  /* hook for SHM module */

  if (sctk_net_unregister_ptr && task_list)
    sctk_net_unregister_ptr (task_list, msg->nb_task_involved * sizeof (int));
  sctk_free (task_list);
}

void
sctk_net_update_new_communicator (const sctk_communicator_t
    origin_communicator,
    const int nb_task_involved,
    const int *task_list, const int process)
{
  int i;
  sctk_update_new_communicator_t msg;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_nodebug ("%s", SCTK_FUNCTION);

  sctk_thread_mutex_lock (&lock);
  msg.origin_communicator = origin_communicator;
  msg.nb_task_involved = nb_task_involved;
  msg.process = process;
  msg.addr = (void *) task_list;
  msg.done = 0;
  msg.msg = &msg;
  msg.src = sctk_process_rank;
  msg.check = 0;

  sctk_nodebug ("nb_involved %d", msg.nb_task_involved);

  sctk_nodebug ("PROVIDE %p", msg.addr);
  if (sctk_net_register_ptr && msg.addr)
    sctk_net_register_ptr (msg.addr, msg.nb_task_involved * sizeof (int));
  if (sctk_net_register_ptr)
    sctk_net_register_ptr ((void *) &(msg.done), sizeof (int));

  for (i = 0; i < nb_task_involved; i++)
  {
    sctk_nodebug("TASK_LIST task[%d] %d to process %d", i, task_list[i], process);
    msg.check += task_list[i];
  }

  sctk_nodebug ("PROVIDE %p on %d to %d %p", task_list, sctk_process_rank,
      process, &msg.done);
  /*       { */
  /* 	int w; */
  /* 	for( w = 0; w < 2; w++){ */
  /* 	  sctk_nodebug("UPDATE SEND %d = %d",w,((int*)task_list)[w]); */
  /* 	} */
  /*       } */

  sctk_perform_rpc ((sctk_rpc_t) sctk_net_update_new_communicator_remote,
      process, &msg, sizeof (sctk_update_new_communicator_t));

  sctk_thread_wait_for_value (((int *) &(msg.done)), 1);
  sctk_thread_mutex_unlock (&lock);

  if (sctk_net_unregister_ptr && msg.addr)
    sctk_net_unregister_ptr (msg.addr, nb_task_involved * sizeof (int));
  if (sctk_net_unregister_ptr)
    sctk_net_unregister_ptr ((void *) &(msg.done), sizeof (int));
}

#warning "Optimize for scalability"
/*set_free_communicator*/
typedef struct
{
  sctk_communicator_t origin_communicator;
  sctk_communicator_t communicator;
} sctk_set_free_communicator_t;

  static void
sctk_net_set_free_communicator_remote (sctk_set_free_communicator_t * arg)
{
  sctk_internal_communicator_t *tmp = NULL;
  sctk_nodebug ("%s", SCTK_FUNCTION);
  tmp = sctk_get_communicator (arg->origin_communicator);
  tmp->new_communicator = arg->communicator;
}

void
sctk_net_set_free_communicator (const sctk_communicator_t
    origin_communicator,
    const sctk_communicator_t communicator)
{
  int i;
  sctk_set_free_communicator_t msg;

  sctk_nodebug ("%s", SCTK_FUNCTION);
  msg.origin_communicator = origin_communicator;
  msg.communicator = communicator;

  for (i = 0; i < sctk_process_number; i++)
  {
    if (i != sctk_process_rank)
    {
      sctk_perform_rpc ((sctk_rpc_t)
          sctk_net_set_free_communicator_remote, i, &msg,
          sizeof (sctk_set_free_communicator_t));
    }
  }
}

/*migration*/
static int sctk_val_net_migration_available = 0;
  void
sctk_set_net_migration_available (int val)
{
  sctk_val_net_migration_available = val;
}

  int
sctk_is_net_migration_available ()
{
  return sctk_val_net_migration_available;
}

typedef struct
{
  int task;
  void *ptr;
  int new_proc;
} sctk_migration_t;

  static void
sctk_net_migration_remote (sctk_migration_t * arg)
{
  void *self_p = NULL;
  int vp;
  char name[SCTK_MAX_FILENAME_SIZE];
  sctk_nodebug ("%s", SCTK_FUNCTION);


  self_p = arg->ptr;
  vp = 0;

  sctk_nodebug ("Recover task %d thread %p", arg->task, self_p);

  sprintf (name, "%s/mig_task_%p", sctk_store_dir, self_p);
  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks++;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);
  sctk_thread_restore (self_p, name, vp);
}

  void
sctk_net_migration (const int rank, const int process)
{
  sctk_migration_t msg;

  assume (sctk_is_net_migration_available () != 0);

  sctk_nodebug ("%s", SCTK_FUNCTION);
  msg.task = rank;
  msg.new_proc = process;
  msg.ptr = sctk_thread_self ();
  sctk_nodebug ("Send Restart %d to %d", msg.task, msg.new_proc);

  sctk_perform_rpc ((sctk_rpc_t) sctk_net_migration_remote, process, &msg,
      sizeof (sctk_migration_t));

  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);
}

/*rpc_collective_op*/
typedef struct sctk_rpc_collective_op_s
{
  size_t elem_size;
  size_t nb_elem;
  void *data_in;
  void *data_out;
  void (*func) (void *, void *, size_t, sctk_datatype_t);
  sctk_communicator_t com_id;
  int vp;
  int task_id;
  sctk_datatype_t data_type;
  int process;
  void *tmp_data_in;
  void *tmp_data_out;
  int host_process;

  void *addr_in;
  void *addr_out;
  volatile int done_in;
  volatile int done_out;
  volatile int done_end;
  struct sctk_rpc_collective_op_s *msg;
  int src;
  size_t size;
} sctk_rpc_collective_op_t;

  static void *
sctk_rpc_collective_op_remote_thread (sctk_rpc_collective_op_t * msg)
{
  size_t size;
  sctk_nodebug ("sctk_rpc_collective_op_remote_thread");

  sctk_nodebug ("Collective operation %p %p %p %lu %lu",
      msg->tmp_data_in, msg->tmp_data_out, msg->func,
      msg->elem_size, msg->nb_elem);
  sctk_perform_collective_communication_rpc (msg->elem_size, msg->nb_elem,
      msg->tmp_data_in,
      msg->tmp_data_out,
      (void (*)
       (const void *, void *, size_t,
        sctk_datatype_t)) msg->func,
      msg->com_id, msg->vp,
      msg->task_id, msg->data_type);

  sctk_nodebug ("Collective operation done %p %p %lu", msg->data_out,
      msg->tmp_data_out, msg->elem_size * msg->nb_elem);

  size = msg->elem_size * msg->nb_elem;
  if (msg->data_out == NULL)
  {
    size = 0;
  }

  sctk_perform_rpc_send (msg->data_out, msg->tmp_data_out,
      size, msg->src, (int *) &(msg->msg->done_end));

  sctk_free (msg->tmp_data_in);
  sctk_free (msg->tmp_data_out);
  sctk_free (msg);

  return NULL;
}

  static void
sctk_rpc_collective_op_remote (sctk_rpc_collective_op_t * msg_t)
{
  void *data_in;
  void *data_out;
  sctk_rpc_collective_op_t *msg;
  sctk_thread_t pid;
  size_t size;
  sctk_nodebug ("%s", SCTK_FUNCTION);

  msg = sctk_malloc (sizeof (sctk_rpc_collective_op_t));
  memcpy (msg, msg_t, sizeof (sctk_rpc_collective_op_t));

  size = msg->elem_size * msg->nb_elem;

  sctk_nodebug ("\t\t\tBuffer size %lu", size);

  msg->tmp_data_in = NULL;
  msg->tmp_data_out = NULL;
  if (msg->data_in)
  {
    data_in = sctk_malloc (size);
    memset (data_in, 0, size);
    if (sctk_net_register_ptr && data_in)
      sctk_net_register_ptr (data_in, size);

    msg->tmp_data_in = data_in;
    sctk_perform_rpc_retrive (data_in, msg->addr_in, size,
        msg->src, (int *) &(msg->msg->done_in));
    if (sctk_net_unregister_ptr && data_in)
      sctk_net_unregister_ptr (data_in, size);
  }
  if (msg->data_out)
  {
    data_out = sctk_malloc (size);
    memset (data_out, 0, size);
    if (sctk_net_register_ptr && data_out)
      sctk_net_register_ptr (data_out, size);

    msg->tmp_data_out = data_out;
    sctk_perform_rpc_retrive (data_out, msg->addr_out, size,
        msg->src, (int *) &(msg->msg->done_out));
    if (sctk_net_unregister_ptr && data_out)
      sctk_net_unregister_ptr (data_out, size);
  }
  sctk_nodebug ("Buffer size all retrived %lu", size);

  sctk_nodebug ("PREPARE Collective operation %p %p %p %lu %lu",
      msg->tmp_data_in, msg->tmp_data_out, msg->func,
      msg->elem_size, msg->nb_elem);
  sctk_user_thread_create (&pid, NULL,
      (void *(*)(void *))
      sctk_rpc_collective_op_remote_thread,
      (void *) msg);
  sctk_nodebug ("sctk_rpc_collective_op_remote_thread done");
  sctk_thread_detach (pid);
  sctk_nodebug ("sctk_rpc_collective_op_remote_thread detached");
}

  void
sctk_rpc_collective_op (const size_t elem_size,
    const size_t nb_elem,
    const void *data_in,
    void *const data_out,
    void (*func) (const void *, void *,
      size_t, sctk_datatype_t),
    const sctk_communicator_t com_id,
    const int vp, const int task_id,
    const sctk_datatype_t data_type, int process)
{
  sctk_rpc_collective_op_t msg;
  sctk_nodebug ("%s", SCTK_FUNCTION);

  sctk_nodebug ("RPC COLL OP %p %p %p %lu %lu", data_in, data_out, func,
      elem_size, nb_elem);

  msg.elem_size = elem_size;
  msg.nb_elem = nb_elem;

  msg.data_in = (void *) data_in;
  if (sctk_net_register_ptr && data_in)
    sctk_net_register_ptr ((void *) data_in, elem_size * nb_elem);

  msg.data_out = (void *) data_out;
  if (sctk_net_register_ptr && data_out)
    sctk_net_register_ptr ((void *) data_out, elem_size * nb_elem);

  msg.func = (void (*)(void *, void *, size_t, sctk_datatype_t)) func;
  msg.com_id = com_id;
  msg.vp = vp;
  msg.task_id = task_id;
  msg.data_type = data_type;
  msg.process = process;
  msg.host_process = sctk_process_rank;

  msg.addr_in = (void *) data_in;
  msg.addr_out = (void *) data_out;
  msg.done_in = 0;
  msg.done_out = 0;
  msg.done_end = 0;
  msg.msg = &msg;
  msg.src = sctk_process_rank;


  sctk_perform_rpc ((sctk_rpc_t) sctk_rpc_collective_op_remote, process,
      &msg, sizeof (sctk_rpc_collective_op_t));

  sctk_thread_wait_for_value (((int *) &(msg.done_end)), 1);
  if (sctk_net_unregister_ptr && data_in)
    sctk_net_unregister_ptr ((void *) data_in, elem_size * nb_elem);
  if (sctk_net_unregister_ptr && data_out)
    sctk_net_unregister_ptr (data_out, elem_size * nb_elem);
}

/*COLLECTIVES*/
static void (*sctk_net_collective_op_ptr) (sctk_collective_communications_t
    * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void (*func) (const void *,
      void *, size_t,
      sctk_datatype_t),
    const sctk_datatype_t data_type)
= NULL;
  void
  sctk_collective_init_func (void (*collective_func)
      (sctk_collective_communications_t * com,
       sctk_virtual_processor_t * my_vp,
       const size_t elem_size, const size_t nb_elem,
       void (*func) (const void *, void *, size_t,
         sctk_datatype_t),
       const sctk_datatype_t data_type))
{
  sctk_nodebug ("\t\t%s", SCTK_FUNCTION);
  sctk_net_collective_op_ptr = collective_func;
}

  void
sctk_net_collective_op (sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void (*func) (const void *, void *, size_t,
      sctk_datatype_t),
    const sctk_datatype_t data_type)
{
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_net_collective_op_ptr (com, my_vp, elem_size, nb_elem, func,
      data_type);
  sctk_nodebug ("%s DONE", SCTK_FUNCTION);
}

/*PT 2 PT*/
static void (*sctk_net_send_ptp_message_ptr) (sctk_thread_ptp_message_t * msg,
    int dest_process) = NULL;
  void
sctk_net_send_ptp_message (sctk_thread_ptp_message_t * msg, int dest_process)
{
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_net_send_ptp_message_ptr (msg, dest_process);
  sctk_nodebug ("%s DONE", SCTK_FUNCTION);
}
static void
(*sctk_net_copy_message_func_driver) (sctk_thread_ptp_message_t *
    restrict dest,
    sctk_thread_ptp_message_t *
    restrict src) = NULL;
static void (*sctk_net_free_func_driver) (sctk_thread_ptp_message_t * item) =
NULL;
  void
sctk_net_copy_message_func (sctk_thread_ptp_message_t * restrict dest,
    sctk_thread_ptp_message_t * restrict src)
{
  sctk_net_copy_message_func_driver (dest, src);
}

void
sctk_send_init_func (void (*send_func) (sctk_thread_ptp_message_t * msg,
      int dest_process),
    void
    (*copy_func) (sctk_thread_ptp_message_t * restrict dest,
      sctk_thread_ptp_message_t * restrict src),
    void (*free_func) (sctk_thread_ptp_message_t * item))
{
  sctk_nodebug ("%s", SCTK_FUNCTION);
  sctk_net_send_ptp_message_ptr = send_func;
  sctk_net_copy_message_func_driver = copy_func;
  sctk_net_free_func_driver = free_func;
}

  void
sctk_net_free_func (sctk_thread_ptp_message_t * item)
{
  sctk_nodebug("Free here");
  sctk_net_free_func_driver (item);
}


/* geters */
void* sctk_net_get_send_ptp_message_ptr()
{
  return sctk_net_send_ptp_message_ptr;
}

void* sctk_net_get_copy_message_func_driver()
{
  return sctk_net_copy_message_func_driver;
}

void* sctk_net_get_free_func_driver()
{
  return  sctk_net_free_func_driver;
}


void* sctk_net_get_collective_op_ptr()
{
  return sctk_net_collective_op_ptr;
}


/* DSM */
static void (*sctk_net_get_pages_driver) (void *addr, size_t size,
    int process) = NULL;
  void
sctk_net_get_pages_init (void (*func) (void *addr, size_t size, int process))
{
  sctk_net_get_pages_driver = func;
}

  void
sctk_net_get_pages (void *addr, size_t size, int process)
{
  sctk_nodebug ("Retrive %p-%p", addr, (char *) addr + size);
  if (sctk_net_register_ptr)
    sctk_net_register_ptr (addr, size);

  sctk_net_get_pages_driver (addr, size, process);

  if (sctk_net_unregister_ptr)
    sctk_net_unregister_ptr (addr, size);
}

  void
sctk_register_ptr (void (*func) (void *addr, size_t size),
    void (*unfunc) (void *addr, size_t size))
{
  sctk_net_register_ptr = func;
  sctk_net_unregister_ptr = unfunc;
}

/*INIT*/
#define TRY_DRIVER(dr_name)				\
  if(strcmp(name,SCTK_STRING(dr_name)) == 0){		\
    sctk_net_preinit_driver_##dr_name();		\
    sctk_set_net_val(sctk_net_init_driver_##dr_name);	\
    return ;						\
  } (void)(0)
#define TRY_GENDRIVER(dr_name,gen_name)			\
  if(strcmp(name,SCTK_STRING(gen_name)) == 0){		\
    sctk_net_preinit_driver_##dr_name();		\
    sctk_set_net_val(sctk_net_init_driver_##dr_name);	\
    return ;						\
  } (void)(0)
  void
sctk_net_init_driver (char *name)
{
  sctk_nodebug ("Use network driver |%s|", name);
  strcpy(sctk_module_name, name);

  sctk_register_function ((sctk_rpc_t) sctk_net_send_task_location_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_send_task_end_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_update_communicator_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_get_free_communicator_remote);
  sctk_register_function ((sctk_rpc_t)
      sctk_net_update_new_communicator_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_set_free_communicator_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_migration_remote);
  sctk_register_function ((sctk_rpc_t) sctk_rpc_collective_op_remote);
  sctk_register_function ((sctk_rpc_t) sctk_net_sctk_specific_adm_rpc_remote);


  sctk_set_max_rpc_size_comm (sizeof (send_task_location_t));
  sctk_set_max_rpc_size_comm (sizeof (send_task_end_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_update_communicator_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_get_free_communicator_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_update_new_communicator_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_set_free_communicator_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_migration_t));
  sctk_set_max_rpc_size_comm (sizeof (sctk_rpc_collective_op_t));


  /*-----------------------------------------------------------
   *                /!\ /!\ /!\ CARE /!\ /!\ /!\
   *
   * The implementation of MPC modules has changed in order to
   * introduce the notion of "hybrid modules".
   *
   * Currently, only IPoIB and TCP work. Other modules need
   * to be patched.
   *
   * Contact me for any questions : sdidelot@exascale-computing.eu
   *
   *----------------------------------------------------------*/


  /*Generic drivers */
  TRY_GENDRIVER (none, none);
  /*   TRY_GENDRIVER (mpi, mpi); */
  //  TRY_GENDRIVER (ipoib, ipoib);
  //  TRY_GENDRIVER (tcp, tcp);
  //  TRY_GENDRIVER (infiniband, infiniband);
  //  TRY_GENDRIVER (infiniband, ib);

  /* SHM drivers */
  //  TRY_GENDRIVER (shm_tcp, shm_tcp);
  //  TRY_GENDRIVER (shm_ipoib, shm_ipoib);
  //  TRY_GENDRIVER (hybrid, hybrid);

  if(strcmp(name,"tcp") == 0){
    sctk_net_preinit_driver_hybrid();
    return;
  }
  if(strcmp(name,"ipoib") == 0){
    sctk_net_preinit_driver_hybrid();
    return;
  }
    if(strcmp(name,"ib") == 0){
    sctk_net_preinit_driver_hybrid();
    return;
  }

  sctk_error ("Network mode |%s| not available", name);
  sctk_abort ();
}
