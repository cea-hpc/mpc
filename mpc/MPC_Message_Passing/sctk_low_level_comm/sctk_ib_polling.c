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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include "sctk_route.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "POLLING"
#include "sctk_ib_toolkit.h"

#define HOSTNAME 2048
/*-----------------------------------------------------------
 *  FUNCTION
 *----------------------------------------------------------*/
  char *
sctk_ib_polling_print_status (enum ibv_wc_status status)
{
  switch (status) {
    case IBV_WC_SUCCESS:
      return ("IBV_WC_SUCCESS: success");
      break;
    case IBV_WC_LOC_LEN_ERR:
      return ("IBV_WC_LOC_LEN_ERR: local length error");
      break;
    case IBV_WC_LOC_QP_OP_ERR:
      return ("IBV_WC_LOC_QP_OP_ERR: local QP op error");
      break;
    case IBV_WC_LOC_EEC_OP_ERR:
      return ("IBV_WC_LOC_EEC_OP_ERR: local EEC op error");
      break;
    case IBV_WC_LOC_PROT_ERR:
      return ("IBV_WC_LOC_PROT_ERR: local protection error");
      break;
    case IBV_WC_WR_FLUSH_ERR:
      return ("IBV_WC_WR_FLUSH_ERR: write flush error");
      break;
    case IBV_WC_MW_BIND_ERR:
      return ("IBV_WC_MW_BIND_ERR: MW bind error");
      break;
    case IBV_WC_BAD_RESP_ERR:
      return ("IBV_WC_BAD_RESP_ERR: bad response error");
      break;
    case IBV_WC_LOC_ACCESS_ERR:
      return ("IBV_WC_LOC_ACCESS_ERR: local access error");
      break;
    case IBV_WC_REM_INV_REQ_ERR:
      return ("IBV_WC_REM_INV_REQ_ERR: remote invalid request error");
      break;
    case IBV_WC_REM_ACCESS_ERR:
      return ("IBV_WC_REM_ACCESS_ERR: remote access error");
      break;
    case IBV_WC_REM_OP_ERR:
      return ("IBV_WC_REM_OP_ERR: remote op error");
      break;
    case IBV_WC_RETRY_EXC_ERR:
      return ("IBV_WC_RETRY_EXC_ERR: retry exceded error");
      break;
    case IBV_WC_RNR_RETRY_EXC_ERR:
      return ("IBV_WC_RNR_RETRY_EXC_ERR: RNR retry exceded error");
      break;
    case IBV_WC_LOC_RDD_VIOL_ERR:
      return ("IBV_WC_LOC_RDD_VIOL_ERR: local RDD violation error");
      break;
    case IBV_WC_REM_INV_RD_REQ_ERR:
      return ("IBV_WC_REM_INV_RD_REQ_ERR: remote invalid read request error");
      break;
    case IBV_WC_REM_ABORT_ERR:
      return ("IBV_WC_REM_ABORT_ERR: remote abort error");
      break;
    case IBV_WC_INV_EECN_ERR:
      return ("IBV_WC_INV_EECN_ERR: invalid EECN error");
      break;
    case IBV_WC_INV_EEC_STATE_ERR:
      return ("IBV_WC_INV_EEC_STATE_ERR: invalid EEC state error");
      break;
    case IBV_WC_FATAL_ERR:
      return ("IBV_WC_FATAL_ERR: fatal error");
      break;
    case IBV_WC_RESP_TIMEOUT_ERR:
      return ("IBV_WC_RESP_TIMEOUT_ERR: response timeout error");
      break;
    case IBV_WC_GENERAL_ERR:
      return ("IBV_WC_GENERAL_ERR: general error");
      break;
  }
  return ("ERROR NOT KNOWN");
}


void
sctk_ib_polling_check_wc(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_wc wc) {
  LOAD_CONFIG(rail_ib);
  struct sctk_ibuf_s* ibuf;
  char host[HOSTNAME];
  char ibuf_desc[4096];

  if (wc.status != IBV_WC_SUCCESS) {
    ibuf = (struct sctk_ibuf_s*) wc.wr_id;
    assume(ibuf);
    gethostname(host, HOSTNAME);

    if (config->ibv_quiet_crash){
      sctk_error ("\033[1;31mIB - FATAL ERROR FROM PROCESS %d (%s)",
          sctk_process_rank, host);
    } else {
      sctk_ibuf_print(ibuf, ibuf_desc);
      sctk_error ("\033[1;31m\nIB - FATAL ERROR FROM PROCESS %d (%s)\n"
          "################################\033[0m\n"
          "Work ID is   : %d\n"
          "Status       : %s\n"
          "ERROR Vendor : %d\n"
          "Byte_len     : %d\n"
          "Dest process : %d\n"
          "\033[1;31m######### IBUF DESC ############\033[0m\n"
          "%s\n"
          "\033[1;31m################################\033[0m",
          sctk_process_rank, host,
          wc.wr_id, sctk_ib_polling_print_status(wc.status),
          wc.vendor_err, wc.byte_len,
          ibuf->dest_process, ibuf_desc);
    }
    sctk_abort();
  }
}

int sctk_ib_cq_poll(sctk_rail_info_t* rail,
    struct ibv_cq *cq, const int poll_nb, struct sctk_ib_polling_s *poll,
    int (*ptr_func)(sctk_rail_info_t* rail, struct ibv_wc*, struct sctk_ib_polling_s *poll))
{
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  struct ibv_wc wc;
  int found_nb = 0;
  int i, res = 0;

  for(i=0; i < poll_nb; ++i)
  {
      res = ibv_poll_cq (cq, 1, &wc);
      if (res) {
        sctk_ib_polling_check_wc(rail_ib, wc);
        ptr_func(rail, &wc, poll);
        found_nb++;
      }
  }
  return found_nb;
}


#if 0
static void check_thread_id(int id)
{
  /* check if we received a wrong task id */
  if( (id < 0) ||  (id >= sctk_get_total_tasks_number()))
  {
    sctk_error("Wrong id received: %d", id);
    not_reachable();
  }
  return;
}

struct sctk_net_ibv_allocator_task_s* sctk_ib_allocator_create_thread_entry(int id)
{
  struct sctk_net_ibv_allocator_task_s *t = NULL;
  struct sctk_net_ibv_allocator_task_ptr_s *t_ptr = NULL;
  int vp = sctk_thread_get_vp();

  if (id == -1) return NULL;
  check_thread_id(id);

  sctk_spinlock_write_lock(&tasks_table_lock);
  /* first check if the id has not already been created */
  HASH_FIND_INT(tasks_table, &id, t);
  /* if entry found, we return it */
  if (t) {
    sctk_spinlock_write_unlock(&tasks_table_lock);
    return t;
  }

  /* alloc + init task id */
  t = sctk_malloc_on_node(sizeof(struct sctk_net_ibv_allocator_task_s), sctk_get_node_from_cpu(vp));
  t_ptr = sctk_malloc_on_node(sizeof(struct sctk_net_ibv_allocator_task_ptr_s), sctk_get_node_from_cpu(vp));
  memset(t, 0, sizeof(struct sctk_net_ibv_allocator_task_s));
  t->task_nb = id;
  t->frag_eager_list =
        sctk_calloc(sctk_get_total_tasks_number(), sizeof(struct sctk_list_header*));
  assume(t->frag_eager_list);

  SCTK_LIST_HEAD_INIT(&t->pending_msg);

  t->recv_wc = sctk_malloc_on_node(sizeof(OPA_Queue_info_t), sctk_get_node_from_cpu(vp));
  OPA_Queue_init(t->recv_wc);

  t->remote = sctk_calloc(sizeof(sched_sn_t), sctk_get_total_tasks_number());
  assume(t->remote);

  t->recv_lock = SCTK_SPINLOCK_INITIALIZER;
  t->sched_lock = SCTK_SPINLOCK_INITIALIZER;
  t->lock = SCTK_SPINLOCK_INITIALIZER;
  t->vp = sctk_thread_get_vp();
  t->pending_msg_nb = 0;
  t->node = sctk_get_node_from_cpu(t->vp);

  t_ptr->task = t;

  HASH_ADD_INT(tasks_table, task_nb, t);
  HASH_ADD_INT(tasks_per_vp_table[vp], task_nb, t_ptr);

  sctk_spinlock_write_unlock(&tasks_table_lock);
  sctk_nodebug("Entry %p created for task %d", t, id);

  return t;
}


struct sctk_net_ibv_allocator_task_s* sctk_ib_allocator_get_thread_entry(int id, int creation)
{
  struct sctk_net_ibv_allocator_task_s *t = NULL;

  if (id == -1) return NULL;
  check_thread_id(id);

  sctk_spinlock_read_lock(&tasks_table_lock);
  HASH_FIND_INT(tasks_table, &id, t);
  sctk_spinlock_read_unlock(&tasks_table_lock);

  if(creation && !t)
    t = sctk_ib_allocator_create_thread_entry(id);

  sctk_nodebug("Entry %p got for task %d", t, id);
  assume(t);

  return t;
}

  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;
  /* FIXME: change to HashTables */
  size_t size;

  size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);
  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t) + size);

  sctk_net_ibv_allocator->entry = (sctk_net_ibv_allocator_entry_t*)
    ((char*) sctk_net_ibv_allocator + sizeof(sctk_net_ibv_allocator_t));
  assume(sctk_net_ibv_allocator->entry);

  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  /* set up procs list */
  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_net_ibv_allocator->entry[i].rc_sr_lock = SCTK_SPINLOCK_INITIALIZER;
    sctk_net_ibv_allocator->entry[i].rc_rdma_lock = SCTK_SPINLOCK_INITIALIZER;
  }

  /* task per vp table */
  size = sizeof(struct sctk_net_ibv_allocator_task_ptr_s*) * sctk_get_cpu_number();
  tasks_per_vp_table = sctk_malloc(size);
  memset(tasks_per_vp_table, 0, size);


  return sctk_net_ibv_allocator;
}

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      if (sctk_net_ibv_allocator->entry[rank].rc_sr)
      {
        sctk_error("Process %d already registered to sctk_net_ibv_allocator RC_SR", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_sr = entry;

      sctk_ibv_profiler_inc(IBV_QP_CONNECTED);
      break;

    case IBV_CHAN_RC_RDMA:
      if (sctk_net_ibv_allocator->entry[rank].rc_rdma)
      {
        sctk_error("Process %d already registered to chan RC_RDMA", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_rdma = entry;
      sctk_nodebug("Registering %p", &sctk_net_ibv_allocator->entry[rank]);
      break;

    default: assume(0); break;
  }
}

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      return sctk_net_ibv_allocator->entry[rank].rc_sr;
      break;

    case IBV_CHAN_RC_RDMA:
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;

    default: assume(0); break;
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
static int sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int dest, struct sctk_net_ibv_allocator_task_s* task, polling_return_t *res)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_local, task, res);
  return 0;
}

int inline sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int dest, struct sctk_net_ibv_allocator_task_s* task, polling_return_t *res)
{

  return sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, 0, task, res);
}

/* TODO: for now, we restrict the adaptive polling to
 * Linux Systems. It could be cool to port it to other systems */
#if defined(Linux_SYS)
/* Begin of experimental */

uint64_t cpu_frequency = 0;

/* for storing times */
__thread double poll_period = 0;
__thread double poll_last_found = 0;
/* store counters */
__thread int poll_nb_found = 0;
__thread int poll_nb_not_found = 0;
__thread double poll_time_found = 0;
__thread double poll_time_not_found = 0;
#else
#warning "System not supported for adaptive polling"
#endif

/* End of experimental */

static int ib_is_initialized = 0;

void sctk_net_ibv_set_is_initialized()
{ ib_is_initialized = 1; }

int sctk_net_ibv_is_initialized()
{ return ib_is_initialized; }

void sctk_net_ibv_allocator_ptp_poll_all_init(struct sctk_net_hybrid_thread_args_s* args)
{
  struct sctk_net_ibv_allocator_task_s* task  = args->ib_task;

  args->ib_task = sctk_ib_allocator_get_thread_entry(args->task_id, 1);
  task = args->ib_task;

  task->vp = args->vp;
  task->node = sctk_get_node_from_cpu(args->vp);
  sctk_nodebug("Init on vp %d", task->vp);
}

/* locks for secure_polling */
static sctk_spinlock_t lock_recv = SCTK_SPINLOCK_INITIALIZER;

int __thread new_poll_period = 1;
int __thread current_poll = 0;
uint8_t static inline is_adaptive_polling_activate()
{
  uint8_t activate = 1;

  if (ibv_adaptive_polling) {
    current_poll++;
    if (current_poll >= new_poll_period)
    {
      current_poll = 0;
      activate = 1;
    }
    else activate = 0;
  }

  if (activate == 1)
    nb_activate++;
  else
  {
    nb_not_activate++;
  }
  return activate;
}

static void update_adaptive_polling(int found)
{
  if (ibv_adaptive_polling) {
    if (found)
    {
      ++poll_nb_found;
      new_poll_period >>= 1;
      if (new_poll_period < 1)
        new_poll_period = 1;
    } else {
      int tmp;
      if (poll_nb_found == 0)
        tmp = 1;
      else tmp = (poll_nb_not_found / poll_nb_found);
      new_poll_period <<= 1;
      if (tmp < 1) tmp = 1;
      if (new_poll_period > tmp)
        new_poll_period = tmp;
    }
    ++poll_nb_not_found;
  }
}

/* structure which defines where the polling comes from :
 * idle or normal polling function */
typedef enum{
  POLLING,
  IDLE
} polling_e;

/* * * * * * * * * * *
 *  POLLING FUNCTONS
 * * * * * * * * * * */

#define RETURN  //if(SCTK_POLLING_GET_OWN_RECV(&polling_res)) {goto exit_polling;}
int __thread nb_poll_idle = 0;

static int run_polling(struct sctk_net_ibv_allocator_task_s* task, polling_e polling)
{
  int i = 0, res = 0;
  /* if a matching message has been found (a message
   * with a PSN == ESN for the current task) */
  struct sctk_net_ibv_allocator_task_s *poll_pend_task, *tmp;
  int nb_polled;
  polling_return_t polling_res;

  task->in_polling = 1;
  SCTK_POLLING_INIT(&polling_res);
  SCTK_POLLING_SET_POLLING_TASK(&polling_res, task);

  IBV_TIMER_HEAD(timer_polling);
  IBV_TIMER_HEAD(timer_idle_polling);
  IBV_TIMER_HEAD(timer_sched);
  IBV_TIMER_HEAD(timer_pending);
  IBV_TIMER_HEAD(timer_recv);
  IBV_TIMER_HEAD(timer_send);
  IBV_TIMER_HEAD(timer_steal);

  /* start the timers */
  if (polling == POLLING) {
    IBV_TIMER_START(timer_polling);
  } else if (polling == IDLE) {
    IBV_TIMER_START(timer_idle_polling);
  }

  /* read the pending wc from the current task-list */
  if (ibv_steal > 0)
  {
    IBV_TIMER_START(timer_pending);
    int i = 0;
    if (ibv_steal == 1)
    {
      /* the current task is the only one task which can read a
       * pending wc. If the lock is already taken, we retry */
      do {
        res = sctk_net_ibv_read_pending_wc(rail, rc_sr_local, rc_rdma_local, task, &polling_res);
        if (res) i++;
      } while (res);
    } else {
      /* the current task can be stolen. If it is locked,
       * we don't retry. Maybe another task is steal reading
       * messages from us. Retrying is costly on Tachyon i.e */
      do {
        res = sctk_net_ibv_read_pending_wc(rail, rc_sr_local, rc_rdma_local, task, &polling_res);
        if (res) i++;
      } while (res == 1);
    }

    if (sctk_process_rank == 0 && i)
      sctk_debug("[%d] Polled %d messages, pending %d", task->task_nb, i, task->nb_pending);
    IBV_TIMER_STOP(timer_pending);
    RETURN;
  }

  /* read the pending messages from the scheduling list = messages
   * received with a wrong PSN. */
  IBV_TIMER_START(timer_sched);
  /* We first retreive messages for the curren task */
  i=0;
  if (ibv_steal > 0)
  {
    /* we keep going while messages have a goood PSN */
    do {
      res = sctk_net_ibv_sched_poll_pending_msg(task, &polling_res);
    } while (res);
    /* if normal mode */
  } else if (ibv_steal == 0) {
    HASH_ITER(hh, tasks_table, poll_pend_task, tmp) {
      if (poll_pend_task->task_nb == task->task_nb)
      {
        do {
          res = sctk_net_ibv_sched_poll_pending_msg(poll_pend_task, &polling_res);
        } while (res);
        break;
      } else {
        res = sctk_net_ibv_sched_poll_pending_msg(poll_pend_task, &polling_res);
      }
    }
    RETURN;
  }
  IBV_TIMER_STOP(timer_sched);

  /* read received messages */
  IBV_TIMER_START(timer_recv);
  do {
    SCTK_POLLING_RESET_LAST_RECV(&polling_res);
    nb_polled = sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, ibv_wc_in_number, sctk_net_ibv_rc_sr_recv_cq, IBV_CHAN_RECV, task, &polling_res);
  } while (SCTK_POLLING_GET_LAST_RECV(&polling_res));
  IBV_TIMER_STOP(timer_recv);
  RETURN;

  /* read sent messages */
  IBV_TIMER_START(timer_send);
  nb_polled = sctk_net_ibv_cq_poll(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, IBV_CHAN_SEND, task, &polling_res);
  IBV_TIMER_STOP(timer_send);


  /*-----------------------------------------------------------
   *  STEALING STEP
   *----------------------------------------------------------*/

  /* if no message with a good PSN has been found for
   * the current task */
  if ( SCTK_POLLING_GET_OWN_RECV(&polling_res) == 0)
  {
    if (ibv_steal > 1)
    {
      /* we first try to steal messages from the scheduling
       * pending list */
      i=0;
      IBV_TIMER_START(timer_sched);
      HASH_ITER(hh, tasks_table, poll_pend_task, tmp) {
        do
        {
          res = sctk_net_ibv_sched_poll_pending_msg(poll_pend_task, &polling_res);
        } while(res);
        ++i;
      }
      IBV_TIMER_STOP(timer_sched);
    }

    if (ibv_steal > 1)
    {
      IBV_TIMER_START(timer_steal);
      sctk_net_ibv_steal_wc(rail, rc_sr_local, rc_rdma_local, 2, task, &polling_res);
      IBV_TIMER_STOP(timer_steal);
    }
  }

  /* stop timers */
  if (polling == POLLING) {
    IBV_TIMER_STOP(timer_polling);
  } else if (polling == IDLE) {
    IBV_TIMER_STOP(timer_idle_polling);
  }

exit_polling:
  task->in_polling = 0;

  return SCTK_POLLING_GET_OWN_RECV(&polling_res) + SCTK_POLLING_GET_OTHER_RECV(&polling_res);
}


void sctk_net_ibv_allocator_ptp_poll_all_idle(struct sctk_net_hybrid_thread_args_s* args)
{
  int found = 0;
  uint8_t activate = 1;
  struct sctk_net_ibv_allocator_task_s* task  = args->ib_task;

  task->in_polling = 1;

  nb_poll_idle++;

  /* if thread must be activated */
#if defined(Linux_SYS)
  activate = is_adaptive_polling_activate();
  if (activate)
  {
#endif
    if (ibv_secure_polling) {
      if(sctk_spinlock_trylock(&lock_recv) == 0)
      {
        goto execute_polling;
      }
    } else {
      goto execute_polling;
    }
#if defined(Linux_SYS)
  }
#endif
  goto exit_polling;

  /* POLLING FUNCTION */
execute_polling:
  found = run_polling(task, IDLE);
#if defined(Linux_SYS)
      update_adaptive_polling(found);
#endif

exit_polling:
  if (ibv_secure_polling) {
    sctk_spinlock_unlock(&lock_recv);
  }
}

void sctk_net_ibv_allocator_ptp_poll_all()
{
  int found = 0;
  /* if thread must be activated */
  int task_id;
  int thread_id;
  uint8_t activate = 1;
  struct sctk_net_ibv_allocator_task_s* task;

  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_allocator_get_thread_entry(task_id, 1);
  task->in_polling = 1;

  /* if thread must be activated */
#if defined(Linux_SYS)
  activate = is_adaptive_polling_activate();
  if (activate)
  {
#endif
    if (ibv_secure_polling) {
      if(sctk_spinlock_trylock(&lock_recv) == 0)
      {
        goto execute_polling;
      }
    } else {
      goto execute_polling;
    }
#if defined(Linux_SYS)
  }
#endif
  goto exit_polling;

  /* POLLING FUNCTION */
execute_polling:
  found = run_polling(task, POLLING);
#if defined(Linux_SYS)
      update_adaptive_polling(found);
#endif

exit_polling:
  if (ibv_secure_polling) {
    sctk_spinlock_unlock(&lock_recv);
  }
}

/*-----------------------------------------------------------
 *  PTP SENDING
 *----------------------------------------------------------*/


/**
 *  Function which sends a PTP message to a dest process. The
 *  channel to use is determinated according to the size of
 *  the message to send
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  DBG_S(1);
  size_t size;
  size_t size_with_header;
  sctk_net_ibv_allocator_request_t req;
  int task_id;
  int thread_id;
  struct sctk_net_ibv_allocator_task_s* task;

  sctk_nodebug("size: %lu", msg->header.msg_size);
  nb_ptp_sent++;
  size_ptp_sent += msg->header.msg_size;

  /* determine the message number and the
   * thread ID of the current thread */
  size = msg->header.msg_size;
  size_with_header = size + sizeof(sctk_thread_ptp_message_t);
  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_allocator_get_thread_entry(task_id, 1);

  /* profile if message is contigous or not (if we can
   * use the zerocopy method or not) */
  sctk_ibv_profiler_add(IBV_PTP_SIZE, size_with_header);
  sctk_ibv_profiler_inc(IBV_PTP_NB);

  /* fill the request */
  req.msg = msg;
  req.dest_process = dest_process;
  req.dest_task = ((sctk_thread_ptp_message_t*) msg)->header.destination;
  req.ptp_type = IBV_PTP;
  req.psn = sctk_net_ibv_sched_psn_inc(task, req.dest_task);
  req.task = task;

  if ( size_with_header +
      RC_SR_HEADER_SIZE <= ibv_eager_limit) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);
    sctk_ibv_profiler_add(IBV_EAGER_SIZE, size_with_header);

    /*
     * EAGER MSG
     */
    req.size = size_with_header;
    req.channel = IBV_CHAN_RC_SR;

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);

  } else if ( size <= ibv_frag_eager_limit) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size_with_header);

    /*
     * FRAG MSG: we don't consider the size of the headers
     */
    req.size = size_with_header;
    req.channel = IBV_CHAN_RC_SR_FRAG;

    sctk_net_ibv_comp_rc_sr_send_frag_ptp_message (rc_sr_local, req);
  } else {
#if IBV_ENABLE_PROFILE == 1
    void* entire_msg = NULL;
    entire_msg = sctk_net_if_one_msg_in_buffer(msg);
    if (entire_msg)
    {
      sctk_ibv_profiler_inc(IBV_MSG_DIRECTLY_PINNED);
    }  else {
      sctk_ibv_profiler_inc(IBV_MSG_NOT_DIRECTLY_PINNED);
    }
#endif

    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);
    /*
     * RENDEZVOUS
     */
    req.size = size;
    req.channel = IBV_CHAN_RC_RDMA;

    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 1);
  }

  sctk_net_ibv_allocator->entry[dest_process].nb_ptp_msg_transfered++;

  DBG_E(1);
}

/**
 *  Function which sends a PTP collective message to a dest process. The
 *  channel to use is determinated according to the size of
 *  the message to send
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_coll_message (
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_collective_communications_t * com,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_ptp_type_t type,
    uint32_t psn)
{
  DBG_S(1);
  sctk_net_ibv_allocator_request_t req;
  int task_id;
  int thread_id;
  struct sctk_net_ibv_allocator_task_s* task;

  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_allocator_get_thread_entry(task_id, 1);

  req.ptp_type = type;
  req.size = size;
  req.msg   = msg;
  req.dest_process  = dest_process;
  req.dest_task     = -1;
  req.psn = psn;
  req.task = task;

  sctk_nodebug("Collective type: %d", type);

#ifdef SCTK_USE_ADLER
  req.crc_full_payload = sctk_adler32_buffer(msg, size, 0);
#endif
  /* copy the comm id if this is a message from a bcast or
   * a reduce */
  if (com)
  {
    req.com_id = com->id;
  } else {
    req.com_id = 0;
  }

  sctk_nodebug("Send collective message with size %lu %d", size, type);

  if ( (size + RC_SR_HEADER_SIZE) <= ibv_eager_limit) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);
    /*
     * EAGER MSG
     */
    req.channel = IBV_CHAN_RC_SR;
    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);
  } else if ( (size  + RC_SR_HEADER_SIZE )<= ibv_frag_eager_limit) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size + RC_SR_HEADER_SIZE);

    /*
     * FRAG MSG
     */
    req.channel = IBV_CHAN_RC_SR_FRAG;
    sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message (rc_sr_local, req);
  }
  else
  {
    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);

    /*
     * RENDEZVOUS
     */
    req.channel = IBV_CHAN_RC_RDMA;
    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 0);
  }

  DBG_E(1);
}
#endif

#endif
