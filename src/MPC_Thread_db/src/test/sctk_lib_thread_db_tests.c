/* ############################# MPC License ############################## */
/* # Thu Apr 24 18:48:27 CEST 2008                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* #                                                                      # */
/* # This file is part of the MPC Library.                                # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_lib_thread_db_tests.h"

#ifdef ENABLE_TEST

struct test_td_data test_data ;

td_err_e test_td_init (void) {
#if defined ENABLE_INIT_TEST
  static int inited = 0 ;
  if (inited == 1) return TD_CONTINUE ;

  test_data.ta = NULL ;

  test_data.inside_tester = 1 ;
  test_td_return_codes () ;
  test_data.inside_tester = 0 ;
  inited = 1 ;
#endif
  return TD_OK ;
}

/* Generate new thread debug library handle for process PH.  */
td_err_e test_td_ta_new (struct ps_prochandle *ph, td_thragent_t **ta) {
  int prog_err, ps_err ;

  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  ps_err = ps_try_stop(ph) ;
  if (ps_err != PS_OK) {
    return TD_DBERR ;
  }

  test_data.inside_tester = 1 ;
  prog_err = td_ta_new (ph, ta) ;
  test_data.inside_tester = 0 ;

  if (prog_err == TD_OK) {
    test_data.ta = *ta ;


    assert(*ta != NULL) ;
    assert((*ta)->ph == ph) ;
  }

  ps_err = ps_continue(ph) ;
  if (ps_err != PS_OK) {
    return TD_DBERR ;
  }

  return prog_err ;
}

/** ***************************************************************************** **/

/* Free resources allocated for TA.  */
td_err_e test_td_ta_delete (td_thragent_t *ta) {
  int prog_err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  prog_err = td_ta_delete (ta) ;
  test_data.inside_tester = 0 ;

  if (prog_err == TD_OK) {
    /*ensure that ta space has been freed (how ?)*/
  }
  return prog_err ;
}

/** ***************************************************************************** **/

/* Call for each thread in a process associated with TA the callback function
   CALLBACK.  */
td_err_e test_td_ta_thr_iter (const td_thragent_t *ta,
                              td_thr_iter_f *callback, void *cbdata_p,
                              td_thr_state_e state, int ti_pri,
                              sigset_t *ti_sigmask_p,
                              unsigned int ti_user_flags)
{
  int err, prog_err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  if (ta != NULL) {
    err = ps_try_stop(ta->ph) ;
    if (err != PS_OK)
       return TD_DBERR ;
  }

  test_data.inside_tester = 1 ;
  prog_err = td_ta_thr_iter (ta, callback, cbdata_p, state, ti_pri, ti_sigmask_p, ti_user_flags) ;

  test_td_consistency () ;

  if (ta != NULL) {
    err = ps_continue(ta->ph) ;
    if (err != PS_OK) {
       return TD_DBERR ;
    }
  }

  test_data.inside_tester = 0 ;

  return prog_err ;
}

/** ***************************************************************************** **/

int test_td_return_codes (void) {
  td_thragent_t ta ;
  struct ps_prochandle *ph_p = (void *) -1 ;
  ta.ph = NULL ;
  tdb_log("testing return codes ...") ;

  /** constructors*/
  assert (TD_BADPH == td_ta_new (NULL, NULL));
  assert (TD_ERR == td_ta_new (ph_p, NULL));
  assert (TD_BADTA == td_ta_delete(NULL)) ;
  assert (TD_BADPH == td_ta_delete(&ta)) ;
  assert (TD_BADTA == td_ta_get_ph(NULL, NULL)) ;
  assert (TD_BADPH == td_ta_get_ph(&ta, NULL)) ;

  /** events*/
  assert (TD_BADTA == td_ta_event_addr (NULL, 0, NULL)) ;
  assert (TD_BADPH == td_ta_event_addr (&ta, 0, NULL)) ;

  assert (TD_BADTA == td_ta_clear_event (NULL, NULL)) ;
  assert (TD_BADPH == td_ta_clear_event (&ta, NULL)) ;

  assert (TD_BADTA == td_ta_event_getmsg (NULL, NULL)) ;
  assert (TD_BADPH == td_ta_event_getmsg (&ta, NULL)) ;

  assert (TD_BADTH == td_thr_event_getmsg (NULL, NULL)) ;

  assert (TD_BADTH == td_thr_event_enable (NULL, 0)) ;
  /*TD_DB_ERR*/

  /** accessors*/
  assert (TD_BADTH == td_thr_get_info (NULL, NULL)) ;

  assert (TD_BADTA == td_ta_get_nthreads (NULL, NULL)) ;
  assert (TD_BADPH == td_ta_get_nthreads (&ta, NULL)) ;
  ta.ph = (void *) -1 ;
  assert (TD_ERR == td_ta_get_nthreads (&ta, NULL)) ;
  ta.ph = NULL ;

  /** mapping*/
  assert (TD_BADTA == td_ta_map_id2thr (NULL, 0, NULL)) ;
  assert (TD_BADPH == td_ta_map_id2thr (&ta, 0, NULL)) ;
  ta.ph = (void *) -1 ;
  assert (TD_BADTH == td_ta_map_id2thr (&ta, 0, NULL)) ;
  ta.ph = NULL ;

  assert (TD_BADTA == td_ta_map_lwp2thr (NULL, 0, NULL)) ;
  assert (TD_BADPH == td_ta_map_lwp2thr (&ta, 0, NULL)) ;

  /** iterators*/
  assert (TD_BADTA == td_ta_thr_iter (NULL, NULL, NULL, 0, 0, NULL, 0));
  assert (TD_BADPH == td_ta_thr_iter (&ta, NULL, NULL, 0, 0, NULL, 0)) ;

  /** registers*/
  prgregset_t gregs ;

  assert(td_thr_getfpregs(NULL, NULL)  == TD_BADTH) ;
  assert( td_thr_getgregs(NULL, gregs) == TD_BADTH) ;
#if defined (SCTK_sparc_ARCH_SCTK)
  assert( td_thr_getxregs(NULL, NULL) == TD_BADTH) ;
  assert( td_thr_getxregsize(NULL, NULL) == TD_BADTH) ;
  assert( td_thr_setxregs(NULL, NULL) == TD_BADTH) ;
#else
  assert( td_thr_getxregs(NULL, NULL) == TD_NOXREGS) ;
  assert( td_thr_getxregsize(NULL, NULL) == TD_NOXREGS) ;
  assert( td_thr_setxregs(NULL, NULL) == TD_NOXREGS) ;
#endif
  assert( td_thr_setfpregs(NULL, NULL) == TD_BADTH) ;
  assert( td_thr_setgregs(NULL, NULL) == TD_BADTH) ;

  return 0 ;
}

/** ***************************************************************************** **/
int test_td_consistency_callback (const td_thrhandle_t *th_p, void *cbdata_p) {
  int *iter = (int*) cbdata_p ;
  *iter =  *iter + 1 ;

  {
    td_thrinfo_t original ;
    td_thrinfo_t current ;
    td_thr_events_t events_mask ;

    tdb_log("test_td_consistency_callback@%p", th_p->th_unique);
#if defined(TDB___linux__) && 0
    if (th_p->th_unique == IDLE_THREAD) {
      tdb_log("skip the idle thread");
      return TD_OK ;
    }
#endif
    if (TD_OK == td_thr_get_info(th_p, &original)) {
      int i ;

      /** test set_event*/
      tdb_log("*****************************");
      tdb_log("***  td_thr_set_event  ******");
      tdb_log("*****************************");
      /*set all the events*/
      td_event_fillset(&events_mask) ;
      td_thr_set_event(th_p, &events_mask) ;
      if (TD_OK != td_thr_get_info(th_p, &current)) {
        tdb_log("Impossible to retreive thread info");
        assert(0);
      }
      /*ensure that all the events are enabled*/
      for (i = TD_MIN_EVENT_NUM; i <= TD_MAX_EVENT_NUM; i++) {
        int value = td_eventismember(&current.ti_events, i) ;
        tdb_log("test event #%d %d", i, value);
        if (!td_eventismember(&current.ti_events, i)) {
          tdb_log("td_eventismember failed") ;
          assert(0);
        }
      }

      /** test clear_event*/
      tdb_log("*****************************");
      tdb_log("***  td_thr_clear_event *****");
      tdb_log("*****************************");
      /*clear all the events*/
      td_event_fillset(&events_mask) ;
      td_thr_clear_event(th_p, &events_mask) ;
      if (TD_OK != td_thr_get_info(th_p, &current)) {
        tdb_log("Impossible to retreive thread info");
        assert(0);
      }
      /*ensure that all the events are disabled*/
      for (i = TD_MIN_EVENT_NUM; i <= TD_MAX_EVENT_NUM; i++) {
        int value = td_eventismember(&current.ti_events, i) ;
        tdb_log("test event #%d %d", i, value);
        if (td_eventismember(&current.ti_events, i)) {
          tdb_log("td_eventismember failed") ;
          assert(0);
        }
      }

      tdb_log("*****************************");
      tdb_log("***  td_thr_event_enable ****");
      tdb_log("*****************************");

      /** restore original state*/
      /*empty the event_set*/
      td_event_fillset(&events_mask) ;
      td_thr_clear_event(th_p, &events_mask) ;
      /*restore original*/
      td_thr_set_event(th_p, &original.ti_events) ;

      if (TD_OK == td_thr_get_info(th_p, &current)) {
        /** ensure that the thread has the same state as at the beginning*/
        test_td_check_thread_eq (&original, &current);
      } else {
        tdb_log("Threads are not identical");
        assert(0) ;
      }
    } else {
      tdb_log("td_thr_get_info failed");
      assert(0) ;
    }
  }
  {
#if 0
    td_thrhandle_t thrhandle ;
    td_thrinfo_t original ;

    tdb_log("*****************************");
    tdb_log("*****  td_ta_map_id2thr *****");
    tdb_log("*****************************");

    if (TD_OK != td_thr_get_info(th_p, &original)) {
      tdb_log("Impossible to retreive thread info");
      assert(0);
    }
    if (TD_OK != td_ta_map_id2thr(test_data.ta, original.ti_tid, &thrhandle)) {
      tdb_log("Impossible to map_id2thr");
      assert(0);
    }
    assert(th_p->th_unique == thrhandle.th_unique) ;

    tdb_log("*****************************");
    tdb_log("****  td_ta_map_lwp2thr *****");
    tdb_log("*****************************");

    if (original.ti_state == TD_THR_ACTIVE) {
      if (!TD_OK == td_ta_map_lwp2thr(test_data.ta, original.ti_lid, &thrhandle)) {
        tdb_log("Impossible to map_lwp2thr");
        assert(0);
      }

      assert(th_p->th_unique == thrhandle.th_unique) ;

    } else {
      tdb_log("Thread not running, cannot map_lwp2thr");
    }
#endif
  }

  return TD_OK ;
}

int test_td_err_callback (const td_thrhandle_t *th_p, void *cbdata_p) {
  return TD_ERR ;
}

int test_td_consistency (void) {

  tdb_log("test_td_consistency");
  {
    int nthread ;

    tdb_log("*****************************");
    tdb_log("****  td_ta_thr_iter ******** test_td_err_callback");
    tdb_log("*****************************");

    if ((TD_OK == td_ta_get_nthreads (test_data.ta, &nthread)) && nthread >= 1) {
      assert(TD_DBERR == td_ta_thr_iter (test_data.ta, test_td_err_callback, NULL, TD_THR_ANY_STATE, 0, NULL, 0)) ;
    }
  }
  {
    int nthread ;
    int iter = 0 ;
    tdb_log("*****************************");
    tdb_log("****  td_ta_thr_iter ******** nthread == iter");
    tdb_log("*****************************");

    if ((TD_OK == td_ta_get_nthreads (test_data.ta, &nthread)) &&
        (TD_OK == td_ta_thr_iter (test_data.ta, test_td_consistency_callback, &iter, TD_THR_ANY_STATE, 0, NULL, 0)))
    {
      tdb_log("nthread : %d", nthread);
      tdb_log("iterations : %d", iter);
      assert(nthread == iter);
    }
  }
  {
    tdb_log("*****************************");
    tdb_log("****  td_ta_set_event *******");
    tdb_log("*****************************");

    tdb_log("*****************************");
    tdb_log("***  td_ta_clear_event ******");
    tdb_log("*****************************");
  }
  tdb_log("test_td_consistency returns");

  return 0 ;
}

/** ***************************************************************************** **/

int test_td_check_infaddr_eq(psaddr_t addr, int value) {
  int err, read ;
  err = ps_pdread(test_data.ta->ph, addr, &read, sizeof(int)) ;
  if (err != PS_OK) return -1 ;
  else if (read != value) return 0 ;
  else return 1 ;
}

int test_td_check_thread_eq(const td_thrinfo_t *info1, const td_thrinfo_t *info2) {
  int i ;
  int eq ;

  eq = (info1->ti_ta_p == info2->ti_ta_p) ;
  assert(eq);

#if defined (TDB___linux__) && 0
  if (info1->ti_tid == (thread_t) IDLE_THREAD) {
    if (info2->ti_tid != (thread_t) IDLE_THREAD) {
      assert(0);
      return 0 ;
    }
    eq *= (info1->ti_lid == info2->ti_lid && info2->ti_lid == test_data.ta->idle_thread.lwp) ;
    assert(eq);
    eq *= (info1->ti_state == info2->ti_state && info2->ti_state == TD_THR_ACTIVE) ;
    assert(eq);

    return eq ;
  }
#endif
  eq *= (info1->ti_lid == info2->ti_lid) ;
  assert(eq);
  eq *= (info1->ti_tid == info2->ti_tid) ;
  assert(eq);
  eq *= (info1->ti_state == info2->ti_state) ;
  assert(eq);
  eq *= (info1->ti_startfunc == info2->ti_startfunc) ;
  assert(eq);
#if defined (TDB_SunOS_SYS)
  eq *= (info1->ti_type == info2->ti_type) ;
  assert(eq);
#endif
  /*ensure that all the events are disabled*/
  for (i = TD_MIN_EVENT_NUM; i <= TD_MAX_EVENT_NUM; i++) {
    eq *= (td_eventismember(&info2->ti_events, i) == td_eventismember(&info2->ti_events, i)) ;
    tdb_log("td_eventismember #%d : %d", i, eq);
    assert(eq);
  }
  assert(eq);

  return eq ;
}

#endif /*enable tests*/
