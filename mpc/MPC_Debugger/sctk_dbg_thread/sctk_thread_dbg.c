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
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* ######################################################################## */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
void sctk_thread_print_stack_out (void);

#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread_dbg.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_spinlock.h"
#include "sctk_ethread.h"
#include "sctk_context.h"

#include <thread_db.h>

#if defined(SCTK_USE_THREAD_DEBUG)
static sctk_spinlock_t sctk_thread_dbg_lock = 0;
static volatile sctk_thread_data_t *mpc_thread_list = NULL ;
static volatile unsigned long sctk_thread_number = 0;
static volatile unsigned long sctk_thread_number_alive = 0;
static volatile int sctk_thread_debug_enable = 0;

int sctk_thread_count = 0 ;

static char *
sctk_thread_id (sctk_thread_data_t * item)
{
  static char tmp[4096];
  sprintf (tmp, "%06lu task %04d user thread %04d (LWP %02d) (TID %p) (DATA %p)",
	   item->thread_number, item->task_id, item->user_thread,
	   item->virtual_processor, item->tid, item);
  return (char *) tmp;
}

/** ***************************************************************************** **/

void
sctk_thread_add (sctk_thread_data_t * item, void *tid)
{
  sctk_spinlock_lock (&sctk_thread_dbg_lock);
  item->thread_number = sctk_thread_number;
  item->tid = tid ;
  item->status = sctk_thread_running_status;
  
  /** *** */
  sctk_init_thread_debug (item);
  /** *** */
  
  if (mpc_thread_list == NULL)
    {
      item->prev = item;
      item->next = item;
      mpc_thread_list = item;
      sctk_nodebug("(list@%p was empty)", &mpc_thread_list);
    }
  else
    {
      item->next = mpc_thread_list;
      item->prev = mpc_thread_list->prev;
      item->next->prev = item;
      item->prev->next = item;
    }

  sctk_thread_number++;
  sctk_thread_number_alive++;
  sctk_spinlock_unlock (&sctk_thread_dbg_lock);
  sctk_nodebug_printf ("[New MPCThread %d %p (LWP %d)]\n",
		     item->thread_number, item->tid, item->virtual_processor);
}

void
sctk_thread_remove (sctk_thread_data_t * item)
{
  item->status = sctk_thread_undef_status;
  sctk_spinlock_lock (&sctk_thread_dbg_lock);
  if (item->next == item)
    {
      mpc_thread_list = NULL;
    }
  else
    {
      if (item == mpc_thread_list)
	{
	  mpc_thread_list = item->next;
	}
      item->next->prev = item->prev;
      item->prev->next = item->next;
    }
  sctk_thread_number_alive--;
  sctk_spinlock_unlock (&sctk_thread_dbg_lock);
  
  /** *** */
  sctk_remove_thread (item->tid) ;
  /** *** */
  
  sctk_nodebug_printf ("[End MPCThread %d (LWP %d)]\n",
		     item->thread_number, item->virtual_processor); 
}

void
sctk_thread_enable_debug ()
{
  sctk_thread_debug_enable = 1;
  sctk_nodebug_printf ("Enable MPC thread debug support\n");
}

void
sctk_thread_disable_debug ()
{
  sctk_nodebug_printf ("Disable MPC thread debug support\n");
  sctk_thread_debug_enable = 0;
}

static inline char *
sctk_get_status (sctk_thread_status_t s)
{
  char *status;
  switch (s)
    {
    case sctk_thread_running_status:
      status = "Running  ";
      break;
    case sctk_thread_blocked_status:
      status = "Blocked  ";
      break;
    case sctk_thread_sleep_status:
      status = "Sleep    ";
      break;
    case sctk_thread_undef_status:
      status = "Undefined";
      break;
    default:
      status = "Unknown  ";
    }
  return status;
}

void
sctk_thread_list ()
{
  sctk_thread_data_t *tmp;
  size_t i;
  sctk_thread_debug_enable = 1;
  tmp = (sctk_thread_data_t *) mpc_thread_list;
  sctk_nodebug_printf ("%d MPC Threads\n", sctk_thread_number_alive);
  for (i = 0; i < sctk_thread_number_alive; i++)
    {
      char *th_id;
      char *th_status;
      tmp = (sctk_thread_data_t *) tmp->next;

      th_id = sctk_thread_id (tmp);
      th_status = sctk_get_status (tmp->status);

      sctk_nodebug_printf ("[MPCThread %s %s]\n", th_id, th_status);
    }
}

void
sctk_thread_list_task (int j)
{
  sctk_thread_data_t *tmp;
  size_t i;
  sctk_thread_debug_enable = 1;
  tmp = (sctk_thread_data_t *) mpc_thread_list;
  sctk_nodebug_printf ("MPC Threads for task %d\n", j);
  for (i = 0; i < sctk_thread_number_alive; i++)
    {
      tmp = (sctk_thread_data_t *) tmp->next;
      if (tmp->task_id == j)
	{
	  char *th_id;
	  char *th_status;
	  th_id = sctk_thread_id (tmp);
	  th_status = sctk_get_status (tmp->status);
	  sctk_nodebug_printf ("[MPCThread %s %s]\n", th_id, th_status);
	}
    }
}

void
sctk_thread_current ()
{
  sctk_thread_data_t *tmp;
  tmp = (sctk_thread_data_t *) sctk_thread_data_get ();
  if (tmp != NULL)
    {
      if (tmp->task_id >= 0)
	{
	  sctk_nodebug_printf
	    ("[Current thread is %s (Thread %p)]\n",
	     sctk_thread_id (tmp), tmp->tid);
	}
      else
	{
	  sctk_nodebug_printf ("[Current thread a MPC internal thread]\n");
	}
    }
  else
    {
      sctk_nodebug_printf ("[Current thread a idle task]\n");
    }
}


void
sctk_thread_goto (int j)
{
  sctk_thread_data_t *tmp = NULL;
  sctk_thread_data_t *step = NULL;
  size_t i;

  step = (sctk_thread_data_t *) mpc_thread_list;

  for (i = 0; i < sctk_thread_number_alive; i++)
    {
      step = (sctk_thread_data_t *) step->next;
      if (step->thread_number == (size_t)j)
	tmp = step;
    }

  if (tmp != NULL)
    {
      if (tmp->status == sctk_thread_running_status)
	{
	  sctk_nodebug_printf
	    ("Unable to switch to a thread in running mode,\nuse mpc_thread_cur localise this thread on LWPs\n");
	}
      else
	{
	  sctk_nodebug_printf
	    ("The thread %d is in %s state \nuse mpc_print_stack %d to view current state \nof thread %d\n",
	     j, sctk_get_status (tmp->status), j, j);
	}
    }
  else
    {
      sctk_nodebug_printf ("Thread ID %d not known.\n", j);
    }
}

static volatile sctk_mctx_t old_ctx;
void
sctk_thread_print_stack_in (int j)
{
  size_t i;
  sctk_thread_data_t *tmp = NULL;
  sctk_thread_data_t *step = NULL;
  static volatile sctk_mctx_t new_ctx;
  sctk_thread_data_t *tmp_old;
  static volatile int done = 1;

  if (done == 1)
    {
      done = 0;
      tmp_old = (sctk_thread_data_t *) sctk_thread_data_get ();

      step = (sctk_thread_data_t *) mpc_thread_list;
      for (i = 0; i < sctk_thread_number_alive; i++)
	{
	  step = (sctk_thread_data_t *) step->next;
	  if (step->thread_number == (size_t)j)
	    tmp = step;
	}

      if (tmp != NULL)
	{
	  if (tmp->status == sctk_thread_running_status)
	    {
	      sctk_thread_goto (j);
	    }
	  else
	    {
	      if ((tmp->status != sctk_thread_undef_status)
		  && (sctk_ethread_check_state () == 1))
		{
		  char *th_id;
		  char *th_status;
		  th_id = sctk_thread_id (tmp);
		  th_status = sctk_get_status (tmp->status);
		  sctk_nodebug_printf
		    ("[Switching to thread %s (Thread %p)] %s\n",
		     th_id, tmp->tid, th_status);
		  new_ctx = (((sctk_ethread_per_thread_t *) tmp->tid)->ctx);
		  tmp->force_stop = 1;
		  sctk_swapcontext ((sctk_mctx_t *) & (old_ctx),
				    (sctk_mctx_t *) & (new_ctx));
                  sctk_nodebug_printf ("##################### comeback from switch\n") ;
		}
	      else
		{
		  sctk_nodebug_printf ("Thread ID %d not available.\n", j);
		}
	    }
	}
      else
	{
	  sctk_nodebug_printf ("Thread ID %d not known.\n", j);
	}

      if (tmp_old != NULL)
	{
	  if (tmp_old->task_id >= 0)
	    {
	      char *th_id;
	      char *th_status;
	      th_id = sctk_thread_id (tmp_old);
	      th_status = sctk_thread_id (tmp_old);
	      sctk_nodebug_printf
		("[Switching to thread %s (Thread %p)] %s\n",
		 th_id, tmp_old->tid, th_status);
	    }
	  else
	    {
	      sctk_nodebug_printf ("[Switch to an MPC internal thread]\n");
	    }
	}
      else
	{
	  sctk_nodebug_printf ("[Switch to an idle task]\n");
	}
      done = 1;
    }
  else
    {
      sctk_nodebug_printf ("Unable to switch, last switch is not finished\n");
    }
}

void
sctk_thread_print_stack_task (int j)
{
  size_t i;
  sctk_thread_data_t *step = NULL;

  step = (sctk_thread_data_t *) mpc_thread_list;
  for (i = 0; i < sctk_thread_number_alive; i++)
    {
      step = (sctk_thread_data_t *) step->next;
      if ((step->thread_number == (size_t)j) && (step->user_thread == 0))
	{
	  sctk_thread_print_stack_in (i);
	  return;
	}
    }
  sctk_nodebug_printf ("Task ID %d not known.\n", j);
}

void
sctk_thread_print_stack_out ()
{
  /*  static sctk_mctx_t nold_ctx;
     sctk_swapcontext (&(nold_ctx),(sctk_mctx_t*)&(old_ctx)); */
  sctk_setcontext ((sctk_mctx_t *) & (old_ctx));
  abort ();
}

/** ***************************************************************************** **/
/** *********************   LIB_THREAD_DB   ************************************* **/
/** ***************************************************************************** **/
static int sctk_use_rtdb = 0 ;
static sctk_spinlock_t sctk_rtdb_lock = 0;

/* Translate a sctk status to a gdb status*/
td_thr_state_e sctk_get_gdb_status (sctk_thread_status_t status) {
  switch (status) {
  case sctk_thread_running_status : return TD_THR_ACTIVE   ;
  case sctk_thread_sleep_status   : return TD_THR_RUN;
  case sctk_thread_blocked_status : return TD_THR_RUN; //return TD_THR_SLEEP ;
  case sctk_thread_undef_status   : return TD_THR_ZOMBIE ;
  case sctk_thread_check_status   : return TD_THR_ACTIVE ;
  default : return TD_THR_UNKNOWN ;
  }
}

int sctk_init_thread_debug (sctk_thread_data_t *item) {
  sctk_ethread_per_thread_t *tid;
  volatile tdb_thread_debug_t *thread ;

  if (!sctk_use_rtdb) return 0 ;

  tid = (sctk_ethread_per_thread_t *) item->tid ;
  
  sctk_nodebug("enabled ? %d", sctk_use_rtdb) ;
  rtdb_add_thread (tid, &thread) ;
  rtdb_set_thread_startfunc (thread, item->__start_routine) ;
  rtdb_set_thread_stkbase (thread, tid->stack);
  rtdb_set_thread_stksize (thread, tid->stack_size) ;
  rtdb_set_thread_type (thread, TD_THR_USER) ;
  sctk_nodebug("context");
#if defined(TDB_i686_ARCH_TDB) || defined(TDB_x86_64_ARCH_TDB)
#if SCTK_MCTX_MTH(mcsc)
  rtdb_set_thread_context (thread, tid->ctx.uc.uc_mcontext.gregs) ;
#elif SCTK_MCTX_MTH(sjlj)
  rtdb_set_thread_context (thread, tid->ctx.jb);
#endif
#else
  #warning "Architecture not supported"
#endif
  tid->debug_p = (tdb_thread_debug_t *)thread;

  sctk_nodebug("refresh");
  
  /*initialiaze variable parts of the data*/
  rtdb_update_thread_this_lid (thread) ;
  
  return 0 ;
}

int sctk_remove_thread (void *tid) {

  if (!sctk_use_rtdb) return 0 ;
  
  rtdb_remove_thread_tid(tid);

  return 0 ;
}

/** ***************************************************************************** **/
/* currently never called */
int sctk_refresh_thread_debug (sctk_ethread_per_thread_t *tid, sctk_thread_status_t s) {
  lwpid_t lid ;

  if (!sctk_use_rtdb) return 0 ;
  if (s == sctk_thread_undef_status) return 0 ;
  
  /* the kernel threads are not managed bu rtdb */
  if (mpc_thread_list == NULL && rtdb_get_thread(tid) == NULL) {
    sctk_nodebug("(NO THREAD/Thread %p NOT) IN THE LIST", tid);
    return 1 ;
  }
 
#if defined (TDB_Linux_SYS)
  lid = syscall(SYS_gettid);
#else
  lid = _lwp_self() ;
#endif
  
  rtdb_update_thread_lid(tid->debug_p,lid);
/*   rtdb_update_thread_lid_tid (tid, lid) ; */
  sctk_nodebug("status : %d, gdb : %d", s, sctk_get_gdb_status(s));
  rtdb_update_thread_state_tid (tid, sctk_get_gdb_status (s));
  return 0 ;
}

/** ***************************************************************************** **/
/** ***************************************************************************** **/
/** ***************************************************************************** **/

int sctk_enable_lib_thread_db (void) {
  sctk_dbg_init () ;
  rtdb_enable_lib_thread_db () ;
  sctk_use_rtdb = 1 ;
  
#if defined(SCTK_i686_ARCH_SCTK)
#if SCTK_MCTX_MTH(mcsc)
  rtdb_set_eip_offset (14) ;
  rtdb_set_esp_offset (7) ;
  rtdb_set_ebp_offset (6) ;
  rtdb_set_edi_offset (4) ;
  rtdb_set_ebx_offset (8) ;
    
#elif SCTK_MCTX_MTH(sjlj)
  rtdb_set_eip_offset (5) ;
  rtdb_set_esp_offset (4) ;
  rtdb_set_ebp_offset (3) ;
  rtdb_set_edi_offset (2) ;
  rtdb_set_ebx_offset (0) ;
#endif
#elif defined(SCTK_x86_64_ARCH_SCTK)
  rtdb_set_rbx_offset (0) ;
  rtdb_set_rbp_offset (1) ;
  rtdb_set_r12_offset (2) ;
  rtdb_set_r13_offset (3) ;
  rtdb_set_r14_offset (4) ;
  rtdb_set_r15_offset (5) ;
  rtdb_set_rsp_offset (6) ;
  rtdb_set_pc_offset (7) ;  
#else
#warning "Architecture not supported"
#endif

  rtdb_set_lock ((void *)&sctk_rtdb_lock,
		 (int (*)(void *))sctk_spinlock_lock,
		 (int (*)(void *))sctk_spinlock_unlock,
		 NULL) ;

  /* initialization of the main thread*/
  sctk_init_idle_thread_dbg (&sctk_ethread_main_thread, NULL);
  return 0 ;
}

/** ***************************************************************************** **/

int sctk_init_idle_thread_dbg (void *tid, void *start_fct) {
  sctk_ethread_per_thread_t *ttid;
  volatile tdb_thread_debug_t *thread ;
  if (!sctk_use_rtdb) return 0 ;
  ttid = (sctk_ethread_per_thread_t *) tid ;
  
  sctk_nodebug ("-----------> IDLE TASK : %p",tid) ;

  rtdb_add_thread (ttid, &thread) ;
  rtdb_set_thread_stkbase (thread, ttid->stack);
  rtdb_set_thread_stksize (thread, ttid->stack_size) ;
  rtdb_set_thread_type (thread, TD_THR_SYSTEM) ;
  rtdb_log("start fct : %p", start_fct) ;
  rtdb_set_thread_startfunc (thread, start_fct) ;
  
#if defined(TDB_i686_ARCH_TDB) || defined(TDB_x86_64_ARCH_TDB)
#if SCTK_MCTX_MTH(mcsc)
  rtdb_set_thread_context (thread, ttid->ctx.uc.uc_mcontext.gregs) ;
#elif SCTK_MCTX_MTH(sjlj)
  rtdb_set_thread_context (thread, ttid->ctx.jb);
#endif
#else
  #warning "Architecture not supported"
#endif

  ttid->debug_p = (tdb_thread_debug_t *)thread;
  sctk_nodebug("refresh idle");
  /*initialiaze variable parts of the data*/
  rtdb_update_thread_this_lid (thread) ;

  sctk_nodebug("creation of the idle task");
  rtdb_report_creation_event (thread) ;

  return 0 ;
}

int sctk_free_idle_thread_dbg (void *tid) {

  if (!sctk_use_rtdb) return 0 ;
  
  rtdb_report_death_event_tid (tid) ;

  return 0 ;
}

int sctk_report_creation (void *tid) {
  if (!sctk_use_rtdb) return 0 ;
  sctk_nodebug("creation of a task");
  rtdb_report_creation_event_tid (tid);

  return 0 ;
}


int sctk_report_death (void *tid) {
  if (!sctk_use_rtdb) return 0 ;

  rtdb_report_death_event_tid (tid);

  return 0 ;
}

#endif
