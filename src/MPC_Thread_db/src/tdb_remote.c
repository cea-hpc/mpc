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
/* # following URL "http://www.cecill.info".                              # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#if defined(MPC_Thread_db)

#include "tdb_remote.h"

tdb_err_e rtdb_report_event (
  volatile tdb_thread_debug_t *thread, td_event_e event, void (*bp) (void)) ;

void tdb_formated_assert_print (FILE * stream, const int line,
				const char *file, const char *func,
				const char *fmt, ...) ;
volatile register_offsets_t rtdb_reg_offsets ;

volatile int rtdb_thread_number_alive = 0 ;
volatile tdb_lock_t rtdb_lock = {NULL, NULL, NULL, NULL};

tdb_err_e rtdb_add_thread (const void *tid, volatile tdb_thread_debug_t **thread_p) {
  volatile tdb_thread_debug_t *thread ;
  rtdb_log("new tid : %p", tid);

  assert(tid != NULL) ;

  assert(rtdb_lib_state == TDB_LIB_STARTED) ;
  if (rtdb_lib_state != TDB_LIB_STARTED) return TDB_LIB_NOT_STARTED ;

  thread = (tdb_thread_debug_t *) malloc(sizeof(tdb_thread_debug_t)) ;

  tdb_assert(thread != NULL) ;
  if (thread == NULL) return TDB_MALLOC ;

  thread->tid = tid ;
  thread->context = NULL ;
  thread->extls = NULL ;
  thread->last_event = TD_EVENT_NONE ;

  bzero((void *) &thread->info, sizeof(td_thrinfo_t)) ;

  thread->info.ti_state = TD_THR_ACTIVE ;
  thread->info.ti_tid = (thread_t) tid ;
  thread->info.ti_startfunc = rtdb_unknown_start_function ;
  td_event_emptyset(&thread->info.ti_events) ;

  rtdb_update_thread_this_lid (thread);

  tdb_assert(rtdb_lock.lock != NULL) ;
  if (rtdb_lock.lock == NULL) return TDB_LOCK_UNINIT ;

  if (rtdb_lock.lock != RTDB_NOLOCK) {
    if (rtdb_lock.acquire (rtdb_lock.lock) != 0) {
      rtdb_log("Impossible to acquire the lock ...");
      tdb_assert(NULL) ;
      return TDB_DBERR ;
    }
  }

  if (rtdb_thread_list == NULL) {
      thread->prev = thread;
      thread->next = thread;
      rtdb_thread_list = thread;
      rtdb_log("(list@%p was empty)", &rtdb_thread_list);
  } else {
      thread->next = rtdb_thread_list;
      thread->prev = rtdb_thread_list->prev;
      thread->next->prev = thread;
      thread->prev->next = thread;
  }

  rtdb_thread_number_alive++;

  if (rtdb_lock.lock != RTDB_NOLOCK) {
    rtdb_lock.release (rtdb_lock.lock) ;
  }
  rtdb_log("new thread is %p", thread);
  /* give a thread handle to the user, if he wants*/
  if (thread_p != NULL) *thread_p = thread ;

  return TDB_OK ;
}

tdb_err_e rtdb_remove_thread (volatile tdb_thread_debug_t *thread) {

  tdb_assert(thread != NULL) ;
  if (thread == NULL) return TDB_NO_THR ;

  tdb_assert(rtdb_lock.lock != NULL) ;
  if (rtdb_lock.lock == NULL) return TDB_LOCK_UNINIT ;

  if (rtdb_lock.lock != RTDB_NOLOCK) {
    if (rtdb_lock.acquire (rtdb_lock.lock) != 0) {
      tdb_assert(NULL) ;
      return TDB_DBERR ;
    }
  }

  if (thread->next == thread) {
      rtdb_thread_list = NULL;
  } else {
    if (thread == rtdb_thread_list) {
      rtdb_thread_list = thread->next;
    }

    thread->next->prev = thread->prev;
    thread->prev->next = thread->next;
  }

  rtdb_thread_number_alive--;

  if (rtdb_lock.lock != RTDB_NOLOCK) {
    rtdb_lock.release (rtdb_lock.lock) ;
  }

  free ((void *) thread) ;

  return TDB_OK ;
}


volatile tdb_thread_debug_t *rtdb_get_thread (const void *tid) {
  volatile tdb_thread_debug_t *first;
  volatile tdb_thread_debug_t *current ;
  int found = 0 ;


  if (rtdb_lock.lock != RTDB_NOLOCK) {
    assert (rtdb_lock.acquire (rtdb_lock.lock) == 0);
  }

  first = rtdb_thread_list ;

  tdb_assert(tid != NULL);

  if (first == NULL){
    if (rtdb_lock.lock != RTDB_NOLOCK) {
      rtdb_lock.release (rtdb_lock.lock) ;
    }
    return NULL ;
  }

  current = first ;
  do {
    if (current->tid == tid) {
      found = 1 ;
      break ;
    }
    current = current->next ;
  } while (current != first) ;

  if (!found) current = NULL ;

  rtdb_log("rtdb_get_thread %p -> %p", tid, current);

  if (rtdb_lock.lock != RTDB_NOLOCK) {
    rtdb_lock.release (rtdb_lock.lock) ;
  }

  return current ;
}

tdb_err_e rtdb_enable_lib_thread_db (void) {
  tdb_err_e err ;
  rtdb_log("sctk_enable_lib_thread_db : %d (TO_START : %d)", rtdb_lib_state, TDB_LIB_TO_START);

  if (rtdb_lib_state == TDB_LIB_TO_START) {
    rtdb_lib_state = TDB_LIB_STARTED ;
    rtdb_log("sctk_enable_lib_thread_db Started !");

    rtdb_reg_offsets.size = 0 ;

    err = TDB_OK ;
  } else if (rtdb_lib_state == TDB_LIB_STARTED){
    rtdb_log("sctk_enable_lib_thread_db Already started");
    err = TDB_LIB_ALREADY ;
  } else {
    rtdb_log("sctk_enable_lib_thread_db Inhibited");
    err = TDB_LIB_INHIBED ;
  }
  return err ;
}

tdb_err_e rtdb_disable_lib_thread_db (void) {
  tdb_err_e err ;
  if (rtdb_lib_state == TDB_LIB_STARTED) {
    err = TDB_LIB_ALREADY ;
  } else {

    rtdb_lib_state = TDB_LIB_INHIB ;
    err = TDB_OK ;
  }

  return err ;
}


/** ***************************************************************************** **/

tdb_err_e rtdb_report_event (
  volatile tdb_thread_debug_t *thread, td_event_e event, void (*bp) (void)) {

  tdb_assert(rtdb_lib_state == TDB_LIB_STARTED) ;
  if (rtdb_lib_state != TDB_LIB_STARTED) return TDB_LIB_NOT_STARTED ;

  tdb_assert(thread != NULL) ;
  if (thread == NULL) return TDB_NO_THR ;

  /* debugger are confused is the thread hits a breakpoint,
   * but the PC read in the memory points to a swapcontext
   * function */
  assert(thread->info.ti_state == TD_THR_ACTIVE) ;

  /* Check whether this thread has to report events */
  if (!thread->info.ti_traceme) {
    return TDB_OK ;
  }

  /* Check whether this event has to be reported */
  if (!td_eventismember(&rtdb_ta_events, event) &&
      !td_eventismember(&thread->info.ti_events, event))
  {
    return TDB_OK ;
  }
  thread->last_event = event ;

  rtdb_log ("REPORT EVENT #%d\n", event);
  rtdb_log("--");
  /* conduct execution flow to the breakpoint*/
  bp () ;
  rtdb_log("--");
  return TDB_OK ;
}

/** ***************************************************************************** **/

/* call by sctk_thread.c:sctk_thread_create_tmp_start_routine
 * and sctk_thread_create_tmp_start_routine_user */
tdb_err_e rtdb_report_creation_event (volatile tdb_thread_debug_t *thread) {

  rtdb_log("---------------------> REPORT CREATION <--------------------------");
  return rtdb_report_event (thread, TD_CREATE, rtdb_bp_creation);
}

/** ***************************************************************************** **/
/* call by sctk_thread.c:sctk_thread_create_tmp_start_routine */
tdb_err_e rtdb_report_death_event (volatile tdb_thread_debug_t *thread) {

  rtdb_log("--------------------> REPORT DEATH <------------------------------");
  //return rtdb_report_event (thread, TD_DEATH, rtdb_bp_death);
  return TDB_OK ;
}

/** ***************************************************************************** **/

#define SMALL_BUFFER_SIZE 4096
void rtdb_log (const char *fmt, ...) {
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];

  snprintf (buff, SMALL_BUFFER_SIZE,
	    "\033[32m/*\\ %s \033[0;0m\n",
	    fmt);
  va_start (ap, fmt);
//  vfprintf (stderr, buff, ap);
  va_end (ap);
  fflush(stderr);
}

void tdb_formated_assert_print (FILE * stream, const int line,
				const char *file, const char *func,
				const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];

  snprintf (buff, SMALL_BUFFER_SIZE,
	    "Assertion %s fail at line %d file %s\n",
	    fmt, line,
	    file);

  va_start (ap, fmt);
  vfprintf (stream, buff, ap);
  va_end (ap);
  abort ();
}

/** ***************************************************************************** **/
void rtdb_unknown_start_function(void) {}

#endif
