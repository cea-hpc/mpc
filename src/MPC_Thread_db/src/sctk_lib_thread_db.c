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

#include "mpc_keywords.h"
#include <mpc_arch.h>

#if !(defined(MPC_I686_ARCH) || defined(MPC_X86_64_ARCH))
#warning "Architecture not supported (no GDB support)"
#endif

#include "sctk_lib_thread_db.h"

static const char *symbol_list_arr[] = {
  [CREATE_EVENT] = "rtdb_bp_creation",
  [DEATH_EVENT] = "rtdb_bp_death",
  [LIB_STATE] = "rtdb_lib_state",
  [NB_THREAD] = "rtdb_thread_number_alive",
  [THREAD_LIST_HEAD] = "rtdb_thread_list",
  [TA_EVENTS] = "rtdb_ta_events",
  [REG_OFFSETS] = "rtdb_reg_offsets",
  [NUM_MESSAGES] = NULL
};

/** ***************************************************************************** **/
/** **************   INITIALISATION  ******************************************** **/
/** ***************************************************************************** **/

td_err_e td_init (void) {
  td_err_e err ;
  verbosity_level = INIT_VERBOSITY ;

  tdb_log ("td_init");

  tdb_log ("#############");
#if defined(THREAD_DB_COMPLIANT)
  tdb_log ("## THREAD_DB COMPLIANT");
#else
  tdb_log ("## GDB COMPLIANT");
#endif
  tdb_log ("#############");

#if defined (ENABLE_TEST) && 0
  test_td_init () ;
#endif

  err = TD_OK ;
  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
/*
  if(symbol_list_arr[NUM_MESSAGES] == NULL){
    return TD_ERR;
  }
*/
  return err;
}

/** ***************************************************************************** **/

td_err_e td_ta_new (struct ps_prochandle *ph, td_thragent_t **ta) {
  static int count = 0 ;
  static td_thragent_t *ta_p = NULL ;
  td_err_e err; ps_err_e ps_err ;

#if defined (ENABLE_TEST)
  err = test_td_ta_new (ph, ta) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  /* special parameters -> reset static data*/
  if (ph == (void *) -1 && ta == (void *) -1) {
    ta_p = NULL ;
    count = 0 ;
    tdb_log ("td_ta_new re-initialization");
    return TD_OK ;
  }

  tdb_log ("td_ta_new %d", count);

  count++ ;
  if (ph == NULL) {
    return TD_BADPH;
  } else if (ta == NULL) {
    return TD_ERR;
  }

  if (ta_p == NULL) {
    /* instanciate the thread agent*/
    (*ta) = (td_thragent_t *) malloc (sizeof (td_thragent_t));
    if ((*ta) == NULL) {
      return TD_MALLOC;
    }
    tdb_log ("td_ta_new Save process ...");
    (*ta)->ph = ph ;

    ta_p = *ta ;

    /*initialize TA pointers-to-inferior-space to 0 :
     * updated as soon as TA is *really* initialized*/
    ta_p->thread_list_p = NULL ;


    /* actual initialization will occure when the user provides them */

    ta_p->reg_offsets.size = 0 ;

  } else {
    (*ta) = ta_p  ;
  }

  ps_err = ps_try_stop(ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

#if defined(TDB_SunOS_SYS)
  int model ;
  ps_pdmodel(ph, &model);
  tdb_log ("Model : %d", model);
#endif

#if 0
  /* enable debug in MPC space */
  tdb_log("td_ta_new write %d", );
  err = td_write(ph, TDB_LIB_STATE, TDB_LIB_TO_START) ;
  if (err == TD_NOCAPAB) {
    tdb_log ("td_ta_new Initialization not yet possible ...");

    tdb_log ("td_ta_new Return OK anyway");
    /* otherwise dbx is unhappy */
    err = TD_OK ;
    goto exit ;

  } else if(err != TD_OK){
    /* something wrong happend, cannot handle it ... */

    goto exit ;
  }
#endif
  /** */


  tdb_log("td_ta_new Read the address of the thread list");
  ps_err = ps_lookup (ph, THREAD_LIST_HEAD,
		      &(ta_p->thread_list_p)) ;
  if (ps_err != PS_OK) {
    err = TD_NOCAPAB ;
    goto exit ;
  }

  err = TD_OK ;

exit:
  ps_err = ps_continue(ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* Free resources allocated for TA.  */
td_err_e td_ta_delete (td_thragent_t *ta) {
  ps_err_e ps_err;
  td_err_e err ;

#if defined (ENABLE_TEST)
  ps_err = test_td_ta_delete (ta) ;
  if (ps_err != TD_CONTINUE)
    return ps_err ;
#endif

  tdb_log("td_ta_delete");

  if (ta == NULL)
    return TD_BADTA ;
  else if (ta->ph == NULL)
    return TD_BADPH ;

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  /* reinit ta_new static data (for the next run) */
  td_ta_new ((void *)-1, (void *)-1) ;

  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;


  free(ta) ;

  err = TD_OK ;
  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* ensure that libthread_db variables have been initialized */
td_err_e td_is_ta_initialized (const td_thragent_t *ta) {

  if (ta == NULL) return TD_BADTA ;
  else if (ta->ph == NULL) return TD_BADPH ;

  return TD_OK ;
}

/* ensure that variables in MPC space have been initialized */
td_err_e td_is_ta_ready (const td_thragent_t *ta) {
  td_err_e err;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) return err ;

  /* if TA is initalized, but does not have yet its pointers,
   * try to set them */
  if (ta->thread_list_p == NULL) {
    tdb_log("td_is_ta_ready thread_data_list_addr is NULL") ;
    return TD_NOCAPAB ;
  }

  return TD_OK ;
}

/** ***************************************************************************** **/
/** **************  EVENT ACCESSORS ********************************************* **/
/** ***************************************************************************** **/

#define SET 1
#define DEL 0
/** SET or DEL events */
td_err_e update_event_set (struct ps_prochandle *ph,
                           td_thr_events_t *event_addr,
                           td_thr_events_t *new_events,
                           int enable)
{
  int i, err; ps_err_e ps_err;
  td_thr_events_t thread_event ;

  /* read event set from the user process */
  tdb_log ("read event set @ %p", event_addr);
  ps_err = ps_pdread (ph, (psaddr_t) event_addr, &thread_event, sizeof(td_thr_events_t));
  if (ps_err != PS_OK) {
    tdb_log("read failed");
    return TD_ERR ;
  }

  for (i = TD_MIN_EVENT_NUM; i <= TD_MAX_EVENT_NUM; i++) {
    if (td_eventismember(new_events, i)) {
      if (enable == SET) {
        td_event_addset(&thread_event, i) ;
      } else {
        td_event_delset(&thread_event, i) ;
      }
    }
  }
#if defined (ENABLE_TEST)
  /*ensure that all the events asked have been enabled*/
  for (i = TD_MIN_EVENT_NUM; i <= TD_MAX_EVENT_NUM; i++) {
    tdb_log("test event #%d", i);
    if ((td_eventismember(new_events, i) && !td_eventismember(&thread_event, i))) {
      if (enable == SET) {
        tdb_log("td_eventismember failed") ;
        assert(0);
      }
    } else {
      if (enable == DEL) {
        tdb_log("td_eventismember failed") ;
        assert(0);
      }
    }
  }
#endif

  err = TD_OK ;

  /* write event set back to the user process */
  ps_err = ps_pdwrite (ph, (psaddr_t) event_addr, &thread_event, sizeof(td_thr_events_t));
  if (ps_err != PS_OK) {
    err = TD_ERR ;
  }

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* Enable EVENTs in global mask.  */
td_err_e td_ta_set_event (const td_thragent_t *ta,
                          td_thr_events_t *event)
{
  td_err_e err ;
  td_thr_events_t *event_addr ;
  ps_err_e ps_err ;

#if defined (ENABLE_TEST)
  err = test_td_ta_set_event(ta, event);
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("td_ta_set_event");

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) {
    goto exit ;
  }

  /*the event set has not yet been hooked to the ta_debug structure */
  ps_err = ps_lookup(ta->ph, TA_EVENTS, (psaddr_t *)&event_addr) ;
  if (ps_err != PS_OK) {
    err = TD_NOCAPAB ;
    goto exit ;
  }
  tdb_log("td_ta_set_event ta_event located at %p", event_addr);

  err = update_event_set (ta->ph, (td_thr_events_t *) event_addr, event, SET) ;

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* Disable EVENT in global mask.  */
td_err_e td_ta_clear_event (const td_thragent_t *ta,
                            td_thr_events_t *event)
{
  psaddr_t event_addr ;
  td_err_e err ;
  ps_err_e ps_err ;

#if defined (ENABLE_TEST)
  err = test_td_ta_clear_event(ta, event);
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_ta_clear_event");

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) {
    goto exit ;
  }

  ps_err = ps_lookup(ta->ph, TA_EVENTS, (psaddr_t *)&event_addr) ;
  if (ps_err == PS_OK) {
    err = update_event_set (ta->ph, (td_thr_events_t *) event_addr, event, DEL) ;
  } else {
    tdb_log("err ...");
    /* impossible to lookup, maybe the program is not running anymore */
    err = TD_ERR ;
  }

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* ON/OFF reporting events for thread TH.  */
td_err_e td_thr_event_enable (const td_thrhandle_t *th, int on_off)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *thread ;

  tdb_log("td_thr_event_enable");

#if defined (ENABLE_TEST)
  err = test_td_thr_event_enable(th, on_off) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK)  {
    goto exit ;
  }

  thread = (tdb_thread_debug_t *) th->th_unique ;

  tdb_log("td_thr_event_enable %p : %s",thread, on_off ? "on" : "off");

  /* write enable or disable reporting in the user process */
  ps_err = ps_pdwrite (th->th_ta_p->ph,
		       &thread->info.ti_traceme, &on_off, sizeof(int));
  if (ps_err != PS_OK) {
    err = TD_ERR ;
    goto exit ;
  }

  err = TD_OK ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return TD_OK;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Enable EVENT for thread TH.  */
td_err_e td_thr_set_event (const td_thrhandle_t *th,
                           td_thr_events_t *event)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *thread_p ;
  td_thr_events_t *event_p ;

#if defined (ENABLE_TEST)
  err = test_td_thr_set_event(th, event) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_set_event");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) {
    goto exit ;
  }
  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  //event_p = debug_p->event
  event_p = &thread_p->info.ti_events ;

  err = update_event_set (th->th_ta_p->ph, event_p, event, SET) ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Disable EVENT for thread TH.  */
td_err_e td_thr_clear_event (const td_thrhandle_t *th,
                             td_thr_events_t *event)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *thread_p ;
  td_thr_events_t *event_p ;

#if defined (ENABLE_TEST)
  err = test_td_thr_clear_event(th, event) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_clear_event");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) {
    goto exit ;
  }

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  event_p = &thread_p->info.ti_events ;

  err = update_event_set (th->th_ta_p->ph, event_p, event, DEL) ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/
/** **************  EVENT NOTIFICATION & TRANSMISSION  ************************** **/
/** ***************************************************************************** **/

/* Get event address for EVENT.  */
td_err_e td_ta_event_addr (const td_thragent_t *ta,
                           td_event_e event, td_notify_t *ptr)
{
  int idx = -1 ;
  td_err_e err = TD_NOEVENT;
  ps_err_e ps_err ;

#if defined (ENABLE_TEST)
  err = test_td_ta_event_addr(ta, event, ptr) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log ("td_ta_event_addr #%d", event);

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) {
    goto exit ;
  }

  switch(event) {
    case TD_CREATE : idx = CREATE_EVENT ; break ;
    case TD_DEATH :  idx = DEATH_EVENT  ; break ;
    default:
      /* Event cannot be handled.  */
      break ;
  }

  /* Now get the address.  */
  if (idx != -1) {
    psaddr_t addr;

    if (ps_lookup (ta->ph, idx, &addr) == PS_OK) {
      /* Success, we got the address.  */
      ptr->type = NOTIFY_BPT;
      ptr->u.bptaddr = addr;
      tdb_log ("td_ta_event_addr #%d@%p", event, addr);
      err = TD_OK;
    } else {
      err = TD_ERR;
    }
  }

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;

}

/** ***************************************************************************** **/

int td_event_getmsg_callback(const td_thrhandle_t *th, void *cb_data) ;

/* Return information about last event.  */
td_err_e td_ta_event_getmsg (const td_thragent_t *ta,
                             td_event_msg_t *msg)
{
  /* XXX (LinuxThread) I cannot think of another way but using a static variable.*/
  static td_thrhandle_t th;
  td_err_e err; ps_err_e ps_err ;

#if defined (ENABLE_TEST)
  err = test_td_ta_event_getmsg (ta, msg) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_ta_event_getmsg (static th@%p), msg %p", &th, msg);

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) {
    goto exit ;
  } else if (msg == NULL) {
    err = TD_ERR;
    goto exit ;
  }


  /* static part of the thread_agent */
  th.th_ta_p = (td_thragent_t *) ta ;
  th.th_unique = NULL ;

  /*provide the address of the static handle to the callback*/
  msg->th_p = &th ;

  /*iterate over the threads until one has a message*/
  tdb_log("td_ta_event_getmsg iterate");
  err = td_ta_thr_iter(ta, td_event_getmsg_callback, msg,
		       TD_THR_ANY_STATE, TD_THR_LOWEST_PRIORITY,
		       TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS) ;
  tdb_log("td_ta_event_getmsg iteration returned %s",  tdb_err_str(err));

  tdb_log("msg = %p", msg);
  if (err == TD_OK) {
    err = TD_NOMSG ;
  } else if (msg->event != TD_EVENT_NONE) {
    assert(msg->th_p == &th);
    err = TD_OK ;
  } else {
    err = TD_ERR ;
  }

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;
  tdb_log("td_ta_event_getmsg return: (%s",  tdb_err_str(err));

  return err ;
}

/*
 * return TD_OK if no message is found,
 * returns TD_ERR (to stop the iteration) if a message exists on this thread
 *
 * A special care must be taken regarding the thread handle :
 * the one pointed by msg->th_p is statically allocated in ta_event_getmsg,
 * whereas th is local at ta_iter.
 * thr_event_getmsg copies his th to msg->th_p. So we first need to save
 * msg->th_p, call thr_event_getmsg, then RESTORE its original value, to ensure
 * that msg->th_p pointes to the static address
 *
 * */
int td_event_getmsg_callback(const td_thrhandle_t *th, void *cb_data) {
  td_event_msg_t *msg = (td_event_msg_t *) cb_data ;
  td_err_e err ;
  /*save the static handle provided by ta_getmsg*/
  /*(th is local at thr_iter)*/
  td_thrhandle_t *th_good = (td_thrhandle_t *) msg->th_p ;

  /* get the msg of the current thread*/
  tdb_log("td_event_getmsg_callback callback for thread %p with msg %p", th->th_unique, msg);
  err = td_thr_event_getmsg (th, msg) ;

  /* stop once a non-none msg is found or an error occured*/
  if (err == TD_OK && msg->event == TD_EVENT_NONE) {
    err = TD_OK ;
  } else {
    err = TD_ERR ;
  }
  /*copy information back to the static handle*/
  th_good->th_unique = th->th_unique ;
  msg->th_p = th_good ;

  tdb_log("td_event_getmsg_callback return %d", err);
  return err ;
}



/** *****************   NOT USED IN GDB   *************************************** **/
/** but used in MPC by td_ta_event_getmsg */
/** ****/

/* Get event message for thread TH.  */
td_err_e td_thr_event_getmsg (const td_thrhandle_t *th,
                              td_event_msg_t *msg)
{
  td_err_e err; ps_err_e ps_err ;
  td_event_e event ;
  tdb_thread_debug_t *thread_p = NULL;

#if defined (ENABLE_TEST)
  err = test_td_thr_event_getmsg (th, msg) ;
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("td_thr_event_getmsg %p, msg : %p", thread_p, msg);

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  if (msg == NULL) {
    err = TD_ERR;
    goto exit ;
  }

  err = td_thr_validate (th) ;
  if (err != TD_OK) {
    goto exit ;
  }

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  tdb_log("td_thr_event_getmsg@%p", thread_p);

  msg->th_p = (td_thrhandle_t *)th;

  /*event = th->th_unique->debug->last_event */

  /* event = debug_p->last_event */
  ps_err = ps_pdread(th->th_ta_p->ph,
		     (psaddr_t) &thread_p->last_event,
		     &event, sizeof(int)) ;
  if (ps_err != PS_OK) {
    err = TD_ERR ;
    goto exit ;
  }
  tdb_log("td_thr_event_getmsg event read : %d (TD_CREATE %d, TD_DEATH %d,TD_EVENT_NONE %d)",
      event, TD_CREATE, TD_DEATH, TD_EVENT_NONE);
  tdb_log("msg : %p", msg);
  msg->event = event ;

  /* empty the last_event buffer */

  /* debug_p->last_event = TD_EVENT_NONE*/
  event = TD_EVENT_NONE ;
  tdb_log("td_thr_event_getmsg write TD_EVENT_NONE");
  ps_err = ps_pdwrite(th->th_ta_p->ph, (psaddr_t) &thread_p->last_event, &event, sizeof(int)) ;
  if (ps_err != PS_OK) {
    err = TD_ERR ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;
  tdb_log("td_thr_event_getmsg return") ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}



/** ***************************************************************************** **/
/** **********************   ITERATORS  ***************************************** **/
/** ***************************************************************************** **/

/* Call for each thread in a process associated with TA the callback function
   CALLBACK.  */
td_err_e td_ta_thr_iter (const td_thragent_t *ta,
                         td_thr_iter_f *callback, void *cbdata_p,
                         td_thr_state_e state, __UNUSED__ int ti_pri,
                         __UNUSED__ sigset_t *ti_sigmask_p,
                         __UNUSED__ unsigned int ti_user_flags)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *first ;
  tdb_thread_debug_t *current ;
  td_thrhandle_t handle ;
  td_thrinfo_t info ;
  int is_system;

#if defined (ENABLE_TEST)
  err = test_td_ta_thr_iter(ta, callback, cbdata_p, state,
			    ti_pri, ti_sigmask_p, ti_user_flags) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_ta_thr_iter");

  err = td_is_ta_ready (ta) ;
  if (err != TD_OK) {
    tdb_log("td_ta_thr_iter the TA is not ready");
    if (err == TD_NOCAPAB)
      err = TD_OK ;
    return err ;
  }

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  handle.th_ta_p = (td_thragent_t *) ta ;

  /* read the head of the list */
  tdb_log("td_ta_thr_iter Read the head of the thread list");
  /*current = *ta->thread_list_p */
  ps_err = ps_pdread (ta->ph, ta->thread_list_p,
		      &first, sizeof(tdb_thread_debug_t *));
  if (ps_err != PS_OK) {
    err = TD_ERR ;
    goto exit ;
  }
  tdb_log("td_ta_thr_iter the head of the list is @%p, its value is %p", ta->thread_list_p, first);
  /* return if no thread has been create*/
  if (first == NULL) {
    tdb_log("td_ta_thr_iter no thread");
    err = TD_OK ;
    goto exit ;
  } else {
    int nb ;
    td_ta_get_nthreads (ta, &nb) ;

    tdb_log("td_ta_thr_iter there are some threads (%d)", nb);
  }


  current = first ;
  do {
    tdb_log("td_ta_thr_iter current@%p", current);
    /*retrieve the whole current element (we have to read its state) */
    /*data = *current*/
    ps_err = ps_pdread (ta->ph, (psaddr_t) &current->info, &info, sizeof(td_thrinfo_t));
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    assert(current != NULL);

    is_system = 0;
    if(1){
      if(info.ti_type == TD_THR_SYSTEM){
	is_system = 1;
      } else {
	is_system = 0;
      }
    }

    if (((state == TD_THR_ANY_STATE || info.ti_state == state)) && (is_system == 0)) {
      handle.th_unique = (psaddr_t) current ;

      tdb_log("td_ta_thr_iter callback for %p", current);
      if (callback (&handle, cbdata_p) != 0) {
	tdb_log("td_ta_thr_iter callback returned error, -> TD_DBERR %d", TD_DBERR) ;
	err = TD_DBERR ;
	goto exit ;
      }

      tdb_log("td_ta_thr_iter end of callback");
    }

    /* continue with the next element */
    ps_err = ps_pdread (ta->ph, (psaddr_t) &current->next, &current,
			sizeof(tdb_thread_debug_t *));
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }
  } while (current != first) ;

  tdb_log("td_ta_thr_iter return");
  err = TD_OK ;

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/
/** **************   INFORMATION   ********************************************** **/
/** ***************************************************************************** **/

/* Return information about thread TH.  */
td_err_e td_thr_get_info (const td_thrhandle_t *th,
                          td_thrinfo_t *infop)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *thread_p ;
  prgregset_t prgregset ;

#if defined (ENABLE_TEST)
  err =  test_td_thr_get_info(th, infop);
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_get_info");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_ERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) {
    goto exit ;
  }

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  tdb_log("td_thr_get_info@%p", thread_p);

  /* read the structures from user space*/
  /* data = *th->th_unique */

  ps_err = ps_pdread (th->th_ta_p->ph, &thread_p->info,
		      infop, sizeof(td_thrinfo_t));
  if (ps_err != PS_OK) {
    err = TD_ERR ;
    goto exit ;
  }
  assert(infop->ti_lid != 0) ;

  infop->ti_ta_p = th->th_ta_p ;

  tdb_log("td_thr_get_info ti_type %d (TD_THR_USER %d) (TD_THR_SYSTEM %d)",
      infop->ti_type, TD_THR_USER, TD_THR_SYSTEM);

  tdb_log("td_thr_get_info ti_tid : %p (%ld)", infop->ti_tid);
  err = td_thr_getgregs (th, prgregset) ;
  if (err != TD_OK && err != TD_PARTIALREG) {
    err = TD_ERR ;
    goto exit ;
  }

  infop->ti_pc = prgregset[TDB_PC] ;
  infop->ti_sp = prgregset[TDB_SP] ;

  tdb_log("td_thr_get_info ti_sp : %p ", infop->ti_sp);
  tdb_log("td_thr_get_info ti_pc : %p ", infop->ti_pc);
  tdb_log("td_thr_get_info ti_state : %p ", infop->ti_state);

  err = TD_OK ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;


  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/
/* return 0 until the current thread is the one we looked for */
int td_thr_validate_callback(const td_thrhandle_t *th, void *data) {
  td_thrhandle_t *looked_for = (td_thrhandle_t *) data ;
  return (looked_for->th_unique == th->th_unique) ;
}

/* Validate that TH is a thread handle.  */
td_err_e td_thr_validate (const td_thrhandle_t *th)
{
  td_err_e err; ps_err_e ps_err ;
  tdb_thread_debug_t *thread_p ;
  td_thrhandle_t th_to_lookfor ;

  if (th == NULL) {
    tdb_log("td_thr_validate TD_BADTH (%d)", TD_BADTH);
    return TD_BADTH ;
  }

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  if (thread_p == NULL) {
    tdb_log("td_thr_validate TD_NOTHR (%d)", TD_NOTHR);
    return TD_NOTHR ;
  }

  tdb_log("td_thr_validate") ;

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = !TD_OK ;
#if 1
  th_to_lookfor.th_unique = thread_p ;
  th_to_lookfor.th_ta_p = th->th_ta_p ;
  /* iterate over the threads until we find the one we're looking for*/
  err = td_ta_thr_iter(th->th_ta_p, td_thr_validate_callback, &th_to_lookfor,
		       TD_THR_ANY_STATE, TD_THR_LOWEST_PRIORITY,
		       TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS) ;
#endif
  /* if iteration went right, the thread was not in the list*/
  if (err == TD_OK) {
    err = TD_NOTHR ;
    goto exit;
  }

  err = TD_OK ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Return process handle passed in `td_ta_new' for process associated with TA.  */
td_err_e td_ta_get_ph (const td_thragent_t *ta,
                       struct ps_prochandle **ph)
{
  td_err_e err ;

#if defined (ENABLE_TEST)
  err =  test_td_ta_get_ph(ta, ph);
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("td_ta_get_ph");

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) return err ;

  (*ph) = ta->ph ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return TD_OK ;
}

/** ***************************************************************************** **/
/** **************   MAPPING    ************************************************* **/
/** ***************************************************************************** **/


/** ***************************************************************************** **/
struct td_map_data_s {
  void *value ;
  td_thrhandle_t* th ;
  size_t size ;
  size_t offset ;
  int first ;
} ;

/**
 * returns TD_OK is th is not the thread we looked for, or it is not running
 * returns TD_ERR if it is, which stops the iteration
 *
 * th has to be a valid thread handle
 * */
int td_map_callback(const td_thrhandle_t *th, void *cb_data) {
  struct td_map_data_s *data = (struct td_map_data_s *) cb_data ;
  tdb_thread_debug_t *current = (tdb_thread_debug_t *) th->th_unique ;
  void *read ;
  td_err_e err; ps_err_e ps_err ;

  read = malloc(data->size) ;

  ps_err = ps_pdread (th->th_ta_p->ph, (psaddr_t) current+data->offset, &read, data->size);
  if (ps_err != PS_OK) return TD_ERR ;

  err = TD_OK ;
  tdb_log("%s thread %p, value %p == read %p ?", __FUNCTION__, current,data->value, read) ;
  if (memcmp(&read, data->value, data->size) == 0) {
    tdb_log("%s yes !", __FUNCTION__) ;
    /* save this thread in any case*/
    data->th->th_unique = (psaddr_t) current ;

    /* stop only if we look for the first, or the thread is running*/
    if (data->first) err = TD_ERR ;
    else {
      int status ;

      ps_err = ps_pdread (th->th_ta_p->ph, (psaddr_t) &current->info.ti_state, &status, sizeof(int));
      if (ps_err != PS_OK) return TD_ERR ;

      if (status != TD_THR_ACTIVE) {
	tdb_log("%s %d matches but is not running",__FUNCTION__, current) ;
	err = TD_OK ;
      } else {
	tdb_log("%s %p --> %p",__FUNCTION__, current, read) ;
	err = TD_ERR ;
      }
    }
  }

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

td_err_e td_ta_map (const td_thragent_t *ta,
		    void *value, size_t offset,
		    size_t size,
		    int first,
		    td_thrhandle_t *th)
{
  struct td_map_data_s data ;
  td_err_e err; ps_err_e ps_err ;

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) goto exit ;

  th->th_ta_p = (td_thragent_t *) ta ;
  th->th_unique = NULL ;

  data.th = th ;
  data.value = value ;
  data.offset = offset ;
  data.size = size ;
  data.first = first ;

  /* iterate over the threads until we find one bound to lid */
  td_ta_thr_iter(ta, td_map_callback, &data, TD_THR_ANY_STATE, TD_THR_LOWEST_PRIORITY, TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS) ;

  /*if a thread has been found */
  if (th->th_unique != NULL) {
    err = td_thr_validate (th) ;
    if (err != TD_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    err = TD_OK ;
    goto exit ;
  }

  tdb_log("%s no matching thread -> TD_NOTHR", __FUNCTION__) ;
  err = TD_NOTHR ;

exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK) {
    return TD_DBERR ;
  }

  return err ;
}

/* Map process ID LWPID to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e td_ta_map_lwp2thr (const td_thragent_t *ta, lwpid_t lwpid,
                                   td_thrhandle_t *th)
{
  tdb_thread_debug_t *thread_p ;
  size_t offset ;
  td_err_e err ;
#if defined (ENABLE_TEST)
  err = test_td_ta_map_lwp2thr (ta, lwpid, th) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  if (th == NULL) return TD_BADTH ;
  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  tdb_log("td_ta_map_lwp2thr %d (%p)", lwpid, lwpid) ;
  offset = (void *) &thread_p->info.ti_lid - (void *) thread_p ;

  err = td_ta_map (ta, &lwpid, offset, sizeof(lwpid_t), 0, th) ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}


/* Map thread library handle PT to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e td_ta_map_id2thr (const td_thragent_t *ta, pthread_t pt, td_thrhandle_t *th)
{
  tdb_thread_debug_t *thread_p ;
  size_t offset ;
  td_err_e err;
#if defined (ENABLE_TEST)
  err = test_td_ta_map_id2thr (ta, pt, th) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  if (th == NULL) return TD_BADTH ;
  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  offset = (void *) &thread_p->info.ti_tid - (void *) thread_p ;

  tdb_log("td_ta_map_id2thr %p", (void *) pt) ;

  th->th_unique = NULL ;
  err = td_ta_map (ta, &pt, offset, sizeof(pthread_t), 1, th) ;

  tdb_log("td_ta_map_id2thr %p -> %p", (void *) pt, th->th_unique) ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/
/** *****************       REGISTERS    **************************************** **/
/** ***************************************************************************** **/

int td_check_reg_offsets (const td_thragent_t *ta) {
  int ok = 1 ;
  ps_err_e ps_err;

  /* do we need to read the register offsets ?*/
  if (ta->reg_offsets.size == 0) {
    psaddr_t reg_off_p ;

    tdb_log("td_thr_getgregs Register offsets not read yet");

    ps_err = ps_lookup(ta->ph, REG_OFFSETS ,&reg_off_p);
    if (ps_err != PS_OK) return 0 ;

    ps_err = ps_pdread(ta->ph, reg_off_p, (psaddr_t) &ta->reg_offsets,
		       sizeof(register_offsets_t));
    if (ps_err != PS_OK) return 0 ;

    if (ta->reg_offsets.size == 0) {
      ok = 0 ;
    }
  }

  return ok ;
}

td_err_e td_thr_getRunningState (const td_thrhandle_t *th, lwpid_t *lid_p, td_thr_state_e *state) {
  ps_err_e ps_err ;
  lwpid_t lid ;
  tdb_thread_debug_t *thread_p = (tdb_thread_debug_t *) th->th_unique ;
  td_thr_state_e status ;

  if (state == NULL) return TD_ERR ;

  /* read the current status */
  ps_err = ps_pdread (th->th_ta_p->ph,
		   (psaddr_t)  &thread_p->info.ti_state,
		   &status, sizeof(td_thr_state_e));
  if (ps_err != PS_OK) {
    return TD_ERR ;
  }

  *state = status ;
  if (status == TD_THR_ACTIVE) {
    /* get the LID of the td_thrinfo_t structure */
    ps_err = ps_pdread (th->th_ta_p->ph, (psaddr_t) &thread_p->info.ti_lid,
      &lid, sizeof(lwpid_t));
    if (ps_err != PS_OK) {
      return TD_ERR ;
    }

    tdb_log("td_thr_getsetRunningRegs %p is running on %d", thread_p, lid);
    *lid_p = lid ;
  } else {
    tdb_log("td_thr_getsetRunningRegs %p is not running (%d)", thread_p, status);
  }

  return TD_OK ;
}

/* Retrieve general register contents of process running thread TH.  */
td_err_e td_thr_getgregs (const td_thrhandle_t *th,
                          prgregset_t gregs)
{
  tdb_thread_debug_t *thread_p ;
  td_err_e err; ps_err_e ps_err ;
  td_thr_state_e state ;
  lwpid_t lid ;

#if defined (ENABLE_TEST)
  err = test_td_thr_getgregs (th, gregs) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_getgregs") ;
  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  err = td_thr_getRunningState (th, &lid, &state) ;
  if (err != TD_OK) goto exit ;

  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lgetregs (th->th_ta_p->ph, lid, gregs) ;

    if (ps_err != PS_OK) {
      err = TD_DBERR ;
      goto exit ;
    }
    else
      err = TD_OK ;

  } else {
    psaddr_t registers_p ;
    static psaddr_t *registers ; /* never freed, but not important*/
    if (!td_check_reg_offsets(th->th_ta_p)) {
      tdb_log("td_thr_getgregs Register offsets not yet initialized, not normal ...");

      bzero (gregs, sizeof(prgregset_t));
      err = TD_PARTIALREG ;
      goto exit ;
    }
    assert(th->th_ta_p->reg_offsets.size != 0) ;

    if (registers == NULL) {
      registers = malloc (sizeof(psaddr_t) * th->th_ta_p->reg_offsets.size) ;
      if (registers == NULL) {
	err  = TD_MALLOC ;
	goto exit ;
      }
    }

    /* read the register pointer */
    ps_err = ps_pdread(th->th_ta_p->ph, &thread_p->context, &registers_p, sizeof(psaddr_t)) ;
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    tdb_log("td_thr_getgregs Read register set at %p", registers_p);


    if (registers_p == NULL) {
      tdb_log("td_thr_getgregs Register pointer not yet initialized, \
thread in initialization ...");
      bzero (gregs, sizeof(prgregset_t));
      err = TD_PARTIALREG ;
      goto exit ;
    }
    assert(registers_p != NULL);
    ps_err = ps_pdread (th->th_ta_p->ph, registers_p, registers,
			sizeof(psaddr_t)*th->th_ta_p->reg_offsets.size);
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    td_utils_context_to_prgregset (th->th_ta_p, registers, gregs) ;

    err = TD_PARTIALREG ;
  }

  tdb_log("td_thr_getgregs SP (%d) [%p]", TDB_SP, gregs[TDB_SP]);
  tdb_log("td_thr_getgregs PC (%d) [%p]", TDB_PC,  gregs[TDB_PC]);

  tdb_log("td_thr_getgregs returns %s", tdb_err_str(err)) ;

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}


/** ***************************************************************************** **/


/* Set general register contents of process running thread TH.  */
td_err_e td_thr_setgregs (const td_thrhandle_t *th,
                          CONST_REGSET prgregset_t gregs)
{
  td_err_e err; ps_err_e ps_err ;
  lwpid_t lid ;
  td_thr_state_e state ;

#if defined (ENABLE_TEST)
  err = test_td_thr_setgregs (th, gregs) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_setgregs");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid, &state) ;
  if (err != TD_OK) goto exit ;

  if(state == TD_THR_ACTIVE) {
    tdb_log("td_thr_setgregs write the registers on the LWP");
    ps_err = ps_lsetregs (th->th_ta_p->ph, lid, gregs) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;

    goto exit ;
  } else {
    tdb_log("td_thr_setgregs thread is not running ... %d", state);
    err =  TD_OK ;
    goto exit ;
  }
#if 0
  /** MUST BE REWRITTEN, IF NECESSARY **/
  else if (err == TD_THR_NOT_RUNNING) {
    tdb_thread_debug_t *thread_p = (tdb_thread_debug_t *) th->th_unique ;
    psaddr_t registers_p ;
    static psaddr_t *registers = NULL ;;
    size_t size ;

    if (!td_check_reg_offsets(th->th_ta_p)) {
      tdb_log("td_thr_getgregs Register not yet initialized, not normal ...");
      err = TD_PARTIALREG ;
      goto exit ;
    }

    size = th->th_ta_p->reg_offsets.size ;

    assert(size != 0) ;

    if (registers == NULL) {
      /* never freed, but not important*/
      registers = malloc (sizeof(psaddr_t)*size) ;
      if (registers == NULL) {
	err = TD_MALLOC ;
	goto exit ;
      }
    }

    ps_err = ps_pdread (th->th_ta_p->ph, (psaddr_t) &thread_p->context, &registers_p,
			sizeof(psaddr_t));
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    ps_err = ps_pdread (th->th_ta_p->ph, registers_p, registers,
		     sizeof(psaddr_t)*size);
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    td_utils_prgregset_to_context (th->th_ta_p, gregs, registers) ;

    ps_err = ps_pdwrite (th->th_ta_p->ph, registers_p, registers,
		      sizeof(psaddr_t)*size);
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }

    err = TD_PARTIALREG ;
  }
#endif

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* Retrieve floating-point register contents of process running thread TH.  */
td_err_e td_thr_getfpregs (const td_thrhandle_t *th,
                           prfpregset_t *regset)
{
  td_err_e err; ps_err_e ps_err ;
  lwpid_t lid ;
  td_thr_state_e state ;

#if defined (ENABLE_TEST)
  err = test_td_thr_getfpregs (th, regset) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_getfpregs");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid, &state) ;
  if (err != TD_OK) goto exit ;

  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lgetfpregs (th->th_ta_p->ph, lid, regset) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;
  } else {
    err = TD_OK ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/

/* Set floating-point register contents of process running thread TH.  */
td_err_e td_thr_setfpregs (const td_thrhandle_t *th,
                           const prfpregset_t *fpregs)
{
  td_err_e err; ps_err_e ps_err ;
  lwpid_t lid ;
  td_thr_state_e state ;

#if defined (ENABLE_TEST)
  err = test_td_thr_setfpregs (th, fpregs) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_setfpregs");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid, &state) ;
  if (err != TD_OK) goto exit ;

  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lsetfpregs (th->th_ta_p->ph, lid, fpregs) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;
  } else {
    err =  TD_OK ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/


/* Retrieve extended register contents of process running thread TH.  */
td_err_e td_thr_getxregs (__UNUSED__ const td_thrhandle_t *th, __UNUSED__ void *xregs)
{
  td_err_e err = TD_DBERR;
#if defined (SCTK_sparc_ARCH_SCTK)
  #error not implemented
  td_thr_state_e state ;
#if defined (ENABLE_TEST)
  err = test_td_thr_getxregs (th, xregs) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_getxregs");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid, &state) ;
  if (err != TD_OK) goto exit ;
  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lgetxregs (ta->ph, lid, xregs) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;

    goto exit ;
  } else {
    err = TD_NOXREGS ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  return err ;
#else
#if defined (ENABLE_TEST)
  err = test_td_thr_getxregs (th, xregs) ;
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return TD_NOXREGS ;
#endif
}


/** ***************************************************************************** **/

/* Get size of extended register set of process running thread TH.  */
td_err_e td_thr_getxregsize (__UNUSED__ const td_thrhandle_t *th, __UNUSED__ int *sizep)
{
#if defined (SCTK_sparc_ARCH_SCTK)
    #error  not implemented
  td_err_e err = TD_DBERR;
#if defined (ENABLE_TEST)
  err = test_td_thr_getxregsize (th, sizep) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_getxregsize");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid) ;
  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lgetxregsize (ta->ph, lid, sizep) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;

  } else {
    err = TD_NOXREGS ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
#else
  td_err_e err = TD_DBERR;

#if defined (ENABLE_TEST)
  err = test_td_thr_getxregsize (th, sizep) ;
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return TD_NOXREGS ;
#endif
}

/** ***************************************************************************** **/

/* Set extended register contents of process running thread TH.  */
td_err_e td_thr_setxregs (__UNUSED__ const td_thrhandle_t *th,
                          __UNUSED__ const void *addr)
{
  td_err_e err = TD_DBERR;
#if defined (SCTK_sparc_ARCH_SCTK)
    #error  not implemented
  lwpid_t lid ;
#if defined (ENABLE_TEST)
  err = test_td_thr_setxregs (th, addr) ;
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_thr_setxregs");

  ps_err = ps_try_stop(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  err = td_thr_validate (th) ;
  if (err != TD_OK) goto exit ;

  err = td_thr_getRunningState (th, &lid) ;
  if(state == TD_THR_ACTIVE) {
    ps_err = ps_lsetxregs (ta->ph, lid, addr) ;
    if (ps_err != PS_OK)
      err = TD_DBERR ;
    else
      err = TD_OK ;

    goto exit ;
  } else {
    err = TD_NOXREGS ;
  }

exit:
  ps_err = ps_continue(th->th_ta_p->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
#else

#if defined (ENABLE_TEST)
  err = test_td_thr_setxregs (th, addr) ;
  if (err != TD_CONTINUE)
    return err ;
#endif
  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return TD_NOXREGS ;
#endif
}


/** ***************************************************************************** **/
/** *****************   NOT USED IN GDB   *************************************** **/
/*but used in tests*/

/* Get number of currently running threads in process associated with TA.  */
td_err_e td_ta_get_nthreads (const td_thragent_t *ta, int *np)
{
  psaddr_t addr ;
  td_err_e err; ps_err_e ps_err ;
  unsigned long nb_thread ;

#if defined (ENABLE_TEST)
  err = test_td_ta_get_nthreads (ta, np);
  if (err != TD_CONTINUE)
    return err ;
#endif

  tdb_log("td_ta_get_nthreads");

  err = td_is_ta_initialized (ta) ;
  if (err != TD_OK) return err ;
  else if (np == NULL)
    return TD_ERR;

  ps_err = ps_try_stop(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  *np = 0 ;
  ps_err = ps_lookup (ta->ph, NB_THREAD, &addr) ;
  if (ps_err != PS_OK) {
    tdb_log("td_ta_get_nthreads rtdb_thread_number_alive not loaded yet -> 0 thread");
    *np = 0 ;

  } else {
    ps_err = ps_pdread(ta->ph, addr, &nb_thread, sizeof(unsigned long)) ;
    if (ps_err != PS_OK) {
      err = TD_ERR ;
      goto exit ;
    }
    *np = (int) nb_thread ;
    tdb_log("td_ta_get_nthreads tdb_thread_number_alive : %d thread(s)", nb_thread);
  }
  err = TD_OK ;
exit:
  ps_err = ps_continue(ta->ph) ;
  if (ps_err != PS_OK)
    return TD_DBERR ;

  tdb_log("%s returns: %s", __FUNCTION__, tdb_err_str(err)) ;
  return err ;
}

/** ***************************************************************************** **/
/** **************  REMOTE PROCESS MANAGEMENT   ********************************* **/
/** ***************************************************************************** **/

td_err_e td_write(struct ps_prochandle *ph, int idx, int val) {
  const char ** symb ;
  psaddr_t addr ;
  td_err_e err; ps_err_e ps_err;

  tdb_log("lookup") ;
  ps_err = ps_lookup (ph, idx, &addr) ;
  if (ps_err != PS_OK) {
    tdb_log ("td_write Lookup not yet possible to read");
    err = TD_NOCAPAB ;
    goto exit ;
  }
  tdb_log("----");
  symb = td_symbol_list() ;

  if (idx == LIB_STATE && val == TDB_LIB_TO_START) {
    /* ensure that the library has not already been enabled */
    int action ;

    tdb_log ("td_write Initialization of libthread_db");

    ps_err = ps_pdread(ph, addr, &action, sizeof(int)) ;
    if (ps_err != PS_OK) {
      tdb_log ("td_write Read not yet possible to read");
      err = TD_NOCAPAB ;
      goto exit ;
    }
    if (action != TDB_LIB_INHIB) {
      tdb_log ("td_write Already done");
      err = TD_OK ;
      goto exit ;
    }

    ps_err = ps_pdwrite(ph, addr, &idx, sizeof(int)) ;
    if (ps_err == PS_OK) {
      tdb_log ("td_write Initialized");
      err = TD_OK ;
      goto exit ;
    } else {
      tdb_log ("td_write Write not yet possible to write");
      err = TD_NOCAPAB ;
      goto exit ;
    }
  }

  tdb_log("td_write #%d -> %s@%p", val, symb[idx], addr);
  ps_err = ps_pdwrite(ph, addr, &idx, sizeof(int)) ;
  if (ps_err != PS_OK) {
    err = TD_ERR ;
    goto exit ;
  }
  err = TD_OK ;
exit:
  return err ;
}

/** ***************************************************************************** **/
/** **************  REGISTERS TRANSLATION  ************************************** **/
/** ***************************************************************************** **/

#if defined(MPC_I686_ARCH)
void td_utils_context_to_prgregset(const td_thragent_t *ta,
				   psaddr_t *context, prgregset_t prgregset) {
  prgregset[EBX] = (long) context[ta->reg_offsets.off_ebx] ;         /* EBX */
  prgregset[EDI] = (long) context[ta->reg_offsets.off_edi] ;         /* EDI */
  prgregset[EBP] = (long) context[ta->reg_offsets.off_ebp] ;         /* EBP */
  prgregset[EIP]  = (long) context[ta->reg_offsets.off_eip] ;         /* EIP */
  prgregset[UESP] = (long) context[ta->reg_offsets.off_esp] ;         /* ESP */
}

void td_utils_prgregset_to_context(const td_thragent_t *ta,
				   prgregset_t prgregset, psaddr_t *context) {
  context[ta->reg_offsets.off_ebx] = (psaddr_t) prgregset[EBX] ;         /* EBX */
  context[ta->reg_offsets.off_esp] = (psaddr_t) prgregset[UESP] ;         /* ESP */
  context[ta->reg_offsets.off_ebp] = (psaddr_t) prgregset[EBP] ;         /* EBP */
  context[ta->reg_offsets.off_edi] = (psaddr_t) prgregset[EDI] ;         /* EDI */
  context[ta->reg_offsets.off_eip] = (psaddr_t) prgregset[EIP]  ;         /* EIP */
}

#elif defined(MPC_X86_64_ARCH)
void td_utils_context_to_prgregset(const td_thragent_t *ta,
				   psaddr_t *context, prgregset_t prgregset) {
  prgregset[RBX] = (long) context[ta->reg_offsets.off_rbx] ;
  prgregset[RBP] = (long) context[ta->reg_offsets.off_rbp] ;
  prgregset[R12] = (long) context[ta->reg_offsets.off_r12] ;
  prgregset[R13] = (long) context[ta->reg_offsets.off_r13] ;
  prgregset[R14] = (long) context[ta->reg_offsets.off_r14] ;
  prgregset[R15] = (long) context[ta->reg_offsets.off_r15] ;
  prgregset[RSP] = (long) context[ta->reg_offsets.off_rsp] ;
  prgregset[RIP] = (long) context[ta->reg_offsets.off_pc] ;
}

void td_utils_prgregset_to_context(const td_thragent_t *ta,
				   prgregset_t prgregset, psaddr_t *context) {
  context[ta->reg_offsets.off_rbx] = (psaddr_t) prgregset[RBX];
  context[ta->reg_offsets.off_rbp] = (psaddr_t) prgregset[RBP];
  context[ta->reg_offsets.off_r12] = (psaddr_t) prgregset[R12];
  context[ta->reg_offsets.off_r13] = (psaddr_t) prgregset[R13];
  context[ta->reg_offsets.off_r14] = (psaddr_t) prgregset[R14];
  context[ta->reg_offsets.off_r15] = (psaddr_t) prgregset[R15];
  context[ta->reg_offsets.off_rsp] = (psaddr_t) prgregset[RSP];
 context[ta->reg_offsets.off_pc] = (psaddr_t) prgregset[RIP];
}
#else
void td_utils_context_to_prgregset(const td_thragent_t *ta,
				   psaddr_t *context, prgregset_t prgregset) {
}

void td_utils_prgregset_to_context(const td_thragent_t *ta,
				   prgregset_t prgregset, psaddr_t *context) {
}
#error "Architecture not supported"
#endif

/** ***************************************************************************** **/
/** **************  SYMBOL LOOKUP    ******************************************** **/
/** ***************************************************************************** **/

ps_err_e ps_lookup (struct ps_prochandle *ph, int idx, psaddr_t *sym_addr)
{
  /* static */ char *lib_path = LIB_RTDB_SO ;
  int ps_err;

  assert (ph != NULL);
  assert (sym_addr != NULL);


  *sym_addr = NULL ;
  assert (idx >= 0 && idx < NUM_MESSAGES);

  tdb_log("ps_pglobal_lookup %s", symbol_list_arr[idx]);

  ps_err = ps_pglobal_lookup (ph, lib_path, symbol_list_arr[idx], sym_addr) ;
  if (ps_err != PS_OK && lib_path != NULL) {
    tdb_log("ps_lookup Impossible to read in %s, try NULL...", lib_path);
    lib_path = NULL ;
    ps_err = ps_pglobal_lookup (ph, lib_path, symbol_list_arr[idx], sym_addr) ;
  }

  if (ps_err != PS_OK) {
    tdb_log("ps_pglobal_lookup %d (PS_NO_SYM %d PS_ERR %d)", ps_err, PS_NOSYM, PS_ERR);
  } else {
    tdb_log("ps_pglobal_lookup %p", *sym_addr);
  }

  return ps_err;

}

/** ***************************************************************************** **/

const char **td_symbol_list (void) {
  return symbol_list_arr ;
}

/** ***************************************************************************** **/
/** **************  STOP & CONTINUE PS    *************************************** **/
/** ***************************************************************************** **/

static struct {
  int lock ;
  int count ;
} td_stopped ;

ps_err_e ps_try_stop (struct ps_prochandle *ph) {
  int ps_err = PS_OK ;

  td_stopped.count++ ;

  if (!td_stopped.lock) {
    ps_err = ps_pstop (ph) ;
    tdb_log("-----------------------> STOP");
    td_stopped.lock = 1 ;
#if defined (ENABLE_TEST) || 1
    assert(td_stopped.count == 1);
#endif
  }
  else {
    tdb_log("STOP %d", td_stopped.count) ;
  }

  return ps_err ;
}

/** ***************************************************************************** **/

ps_err_e ps_continue (struct ps_prochandle *ph) {
  int ps_err = PS_OK ;

#if defined (ENABLE_TEST) || 1
    assert(td_stopped.lock);
#endif

  if (td_stopped.count == 1) {
    ps_err = ps_pcontinue (ph) ;

    tdb_log("-----------------------> CONTINUE");
    td_stopped.lock = 0 ;
  }
  else {
    tdb_log("CONT %d", td_stopped.count);
  }

  td_stopped.count-- ;

  return ps_err ;
}

/** ***************************************************************************** **/
/** ***************************************************************************** **/


/** ***************************************************************************** **/
/** **************  HOME-MADE PRINTFs     *************************************** **/
/** ***************************************************************************** **/

#define SMALL_BUFFER_SIZE 4096
void tdb_formated_assert_print (FILE * stream, const int line,
                                   const char *file, __UNUSED__ const char *func,
                                   const char *fmt, ...) {
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
static inline void to_output(__UNUSED__ FILE *stream, const char *str) {
#if defined (DBG_OUTPUT)
  ps_plog(str);
#elif defined (NO_OUTPUT)
    //void
#else
  fprintf(stderr, str);
  fflush(stderr);
#endif
}

#define MAX_INDENT 10
void tdb_log (const char *fmt, ...) {
  char buff[SMALL_BUFFER_SIZE];
  char str[SMALL_BUFFER_SIZE];
  char tab[MAX_INDENT] ;
  int indent ;

  va_list ap;

  if (td_stopped.count >= MAX_INDENT)
    indent = MAX_INDENT - 1 ;
  else
    indent = td_stopped.count ;

  if (verbosity_level <= td_stopped.count) return ;

  if (indent != 0) {

    memset(tab, '\t', indent) ;
    tab[indent] = '\0' ;
  } else {
    tab[0] = '\0' ;
  }
  snprintf (buff, SMALL_BUFFER_SIZE,
	    "\033[31m/!\\%s%s \033[0;0m\n",
	    tab, fmt);

  va_start (ap, fmt);
  vsprintf(str, buff, ap) ;
  to_output (stderr, str);
  va_end (ap);
}

int verbosity_level ;
void td_set_tdb_verbosity (int n) {
  verbosity_level = n ;
  tdb_log ("New verbosity : %d", n) ;
}

int td_get_tdb_verbosity (void) {
  return verbosity_level ;
}

/* call by DBX to enable logs */
#if defined (_THREAD_DB_H)
td_err_e td_log (void) {td_set_tdb_verbosity (10) ;return TD_OK ;}
#else
void td_log (void) {td_set_tdb_verbosity (10) ;}
#endif


/** ***************************************************************************** **/
/** **************   TLS********************************************************* **/
/** ***************************************************************************** **/
#include <link.h>

#define USE_TLS 1
td_err_e
td_thr_tls_get_addr (const td_thrhandle_t *th __attribute__ ((unused)),
		     void *map_address __attribute__ ((unused)),
		     size_t offset __attribute__ ((unused)),
		     void **address __attribute__ ((unused)))
{
#if USE_TLS
  /* Read the module ID from the link_map.  */
  size_t modid = 0;

  /*
    HACK do not determine module id
   */
#if 0
  if (ps_pdread (th->th_ta_p->ph,
		 &((struct link_map *) map_address)->l_tls_modid,
		 &modid, sizeof modid) != PS_OK)
    return TD_ERR;	/* XXX Other error value?  */
#endif

  td_err_e result = td_thr_tlsbase (th, modid, address);
  if (result == TD_OK)
    *address += offset;
  return result;
#else
  return TD_ERR;
#endif
}


td_err_e td_thr_tlsbase (const td_thrhandle_t *th,
			 unsigned long int __modid,
			 psaddr_t *__base){
  ps_err_e ps_err ;
  tdb_thread_debug_t *thread_p ;
  void** tls;
  void* tls_level;
  char** tls_modules;
  *__base = NULL;

  thread_p = (tdb_thread_debug_t *) th->th_unique ;

  ps_err = ps_pdread(th->th_ta_p->ph, &thread_p->extls, &tls, sizeof(tls)) ;
  if (ps_err != PS_OK) {
    return TD_ERR;
  }


  /*
    HACK to have only task scope
   */
  ps_err = ps_pdread(th->th_ta_p->ph, &(tls[1]), &tls_level, sizeof(tls_level)) ;
  if (ps_err != PS_OK) {
    return TD_ERR;
  }

  ps_err = ps_pdread(th->th_ta_p->ph, tls_level+sizeof(size_t), &tls_modules, sizeof(tls_level)) ;
  if (ps_err != PS_OK) {
    return TD_ERR;
  }

  ps_err = ps_pdread(th->th_ta_p->ph, &(tls_modules[__modid]) , __base, sizeof(tls_level)) ;
  if (ps_err != PS_OK) {
    return TD_ERR;
  }

  fprintf(stderr,"ERROR in td_thr_tlsbase %p %d tls %p %p\n",th,__modid,tls, tls_level);
  return TD_OK;
}
