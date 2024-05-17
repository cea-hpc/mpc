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

/** ***************************************************************************** **/

/* Return process handle passed in `td_ta_new' for process associated with
   TA.  */
td_err_e test_td_ta_get_ph (const td_thragent_t *ta,
                            struct ps_prochandle **ph)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_get_ph (ta, ph) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Map thread library handle PT to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e test_td_ta_map_id2thr (const td_thragent_t *ta, pthread_t pt,
                                td_thrhandle_t *th)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_map_id2thr (ta, pt, th) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Map process ID LWPID to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e test_td_ta_map_lwp2thr (const td_thragent_t *ta, lwpid_t lwpid,
                                 td_thrhandle_t *th)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_map_lwp2thr (ta, lwpid, th) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Call for each defined thread local data entry the callback function KI.  */
td_err_e test_td_ta_tsd_iter (const td_thragent_t *ta, td_key_iter_f *ki,
                              void *p)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_tsd_iter (ta, ki, p) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Enable EVENT in global mask.  */
td_err_e test_td_ta_set_event (const td_thragent_t *ta,
                               td_thr_events_t *event)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_set_event (ta, event) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Disable EVENT in global mask.  */
td_err_e test_td_ta_clear_event (const td_thragent_t *ta,
                                 td_thr_events_t *event)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_clear_event (ta, event) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Get event address for EVENT.  */
td_err_e test_td_ta_event_addr (const td_thragent_t *ta,
                                td_event_e event, td_notify_t *ptr)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_event_addr (ta, event, ptr) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/


/* Return information about last event.  */
td_err_e test_td_ta_event_getmsg (const td_thragent_t *ta,
                                  td_event_msg_t *msg)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_event_getmsg (ta, msg) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/** no hook yet*/
/* Validate that TH is a thread handle.  */
td_err_e test_td_thr_validate (const td_thrhandle_t *th)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_validate (th) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Return information about thread TH.  */
td_err_e test_td_thr_get_info (const td_thrhandle_t *th,
                               td_thrinfo_t *infop)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_get_info (th, infop) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Retrieve floating-point register contents of process running thread TH.  */
td_err_e test_td_thr_getfpregs (const td_thrhandle_t *th,
                                prfpregset_t *regset)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_getfpregs (th, regset) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Retrieve general register contents of process running thread TH.  */
td_err_e test_td_thr_getgregs (const td_thrhandle_t *th,
                               prgregset_t gregs)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_getgregs  (th, gregs) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Retrieve extended register contents of process running thread TH.  */
td_err_e test_td_thr_getxregs (const td_thrhandle_t *th, void *xregs)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_getxregs (th, xregs) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Get size of extended register set of process running thread TH.  */
td_err_e test_td_thr_getxregsize (const td_thrhandle_t *th, int *sizep)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_getxregsize (th, sizep) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Set floating-point register contents of process running thread TH.  */
td_err_e test_td_thr_setfpregs (const td_thrhandle_t *th,
                                const prfpregset_t *fpregs)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_setfpregs (th, fpregs) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Set general register contents of process running thread TH.  */
td_err_e test_td_thr_setgregs (const td_thrhandle_t *th,
                               CONST_REGSET prgregset_t gregs)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_setgregs (th, gregs) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Set extended register contents of process running thread TH.  */
td_err_e test_td_thr_setxregs (const td_thrhandle_t *th,
                               const void *addr)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_setxregs (th, addr) ;
  test_data.inside_tester = 0 ;

  return err ;
}


/** ***************************************************************************** **/

/* Enable reporting for EVENT for thread TH.  */
td_err_e test_td_thr_event_enable (const td_thrhandle_t *th, int event) {
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_event_enable (th, event) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Enable EVENT for thread TH.  */
td_err_e test_td_thr_set_event (const td_thrhandle_t *th,
                                td_thr_events_t *event)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_set_event (th, event) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Disable EVENT for thread TH.  */
td_err_e test_td_thr_clear_event (const td_thrhandle_t *th,
                                  td_thr_events_t *event)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_clear_event (th, event) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/

/* Get event message for thread TH.  */
td_err_e test_td_thr_event_getmsg (const td_thrhandle_t *th,
                                   td_event_msg_t *msg)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_event_getmsg (th, msg) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/** ***************************************************************************** **/
/** ***************************************************************************** **/
/** ***************************************************************************** **/


/* Set priority of thread TH.  */
td_err_e test_td_thr_setprio (const td_thrhandle_t *th, int prio){
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_setprio (th, prio) ;
  test_data.inside_tester = 0 ;

  return err ;
}


/* Set pending signals for thread TH.  */
#if defined (TDB___linux__)
td_err_e test_td_thr_setsigpending (const td_thrhandle_t *th,
                               unsigned char n, const sigset_t *ss)
#else
td_err_e test_td_thr_setsigpending (const td_thrhandle_t *th,
                               unsigned char n, const sigset_t ss)
#endif
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_setsigpending (th, n, ss) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/* Set signal mask for thread TH.  */
#if defined (TDB___linux__)
td_err_e test_td_thr_sigsetmask (const td_thrhandle_t *th,
                                   const sigset_t *ss)
#else
td_err_e test_td_thr_sigsetmask (const td_thrhandle_t *th,
                                   const sigset_t ss)
#endif
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_sigsetmask  (th, ss) ;
  test_data.inside_tester = 0 ;

  return err ;
}


/* Return thread local data associated with key TK in thread TH.  */
td_err_e test_td_thr_tsd (const td_thrhandle_t *th,
                          const thread_key_t tk, void **data)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_tsd (th, tk, data) ;
  test_data.inside_tester = 0 ;

  return err ;
}


/* Suspend execution of thread TH.  */
td_err_e test_td_thr_dbsuspend (const td_thrhandle_t *th)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_dbsuspend (th) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/* Resume execution of thread TH.  */
td_err_e test_td_thr_dbresume (const td_thrhandle_t *th)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_dbresume (th) ;
  test_data.inside_tester = 0 ;

  return err ;
}


/* Enable collecting statistics for process associated with TA.  */
td_err_e test_td_ta_enable_stats (const td_thragent_t *ta, int enable) {
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_enable_stats (ta, enable) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/* Reset statistics.  */
td_err_e test_td_ta_reset_stats (const td_thragent_t *ta) {
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_reset_stats (ta) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/* Retrieve statistics from process associated with TA.  */
td_err_e test_td_ta_get_stats (const td_thragent_t *ta,
                               td_ta_stats_t *statsp)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_get_stats (ta, statsp) ;
  test_data.inside_tester = 0 ;

  return err ;
}

#if 0
/* Get address of thread local variable.  */
td_err_e test_td_thr_tls_get_addr (const td_thrhandle_t *th,
                                   psaddr_t map_address, size_t offset,
                                   psaddr_t *address)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_tls_get_addr (th, map_address, offset, address) ;
  test_data.inside_tester = 0 ;

  return err ;
}
#endif

#if 0
/* Get address of the given module's TLS storage area for the given thread.  */
td_err_e test_td_thr_tlsbase (const td_thrhandle_t *th,
                              unsigned long int modid,
                              psaddr_t *base)
{
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_thr_tlsbase (th, modid, base) ;
  test_data.inside_tester = 0 ;

  return err ;
}
#endif

/* Set suggested concurrency level for process associated with TA.  */
td_err_e test_td_ta_setconcurrency (const td_thragent_t *ta, int level) {
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_setconcurrency (ta, level) ;
  test_data.inside_tester = 0 ;

  return err ;
}

/* Get number of currently running threads in process associated with TA.  */
td_err_e test_td_ta_get_nthreads (const td_thragent_t *ta, int *np) {
  int err ;
  if (test_data.inside_tester != 0)
    return TD_CONTINUE ;

  test_data.inside_tester = 1 ;
  err = td_ta_get_nthreads (ta, np) ;
  test_data.inside_tester = 0 ;

  return err ;
}

#endif /* ENABLE_TEST */
