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

#include "sctk_lib_thread_db.h"

/** ***************************************************************************** **/
/** *****************   STATISTICS   ******************************************** **/
/** ***************************************************************************** **/

/** *****************   NOT USED IN GDB   *************************************** **/
/* Enable collecting statistics for process associated with TA.  */
td_err_e td_ta_enable_stats (const td_thragent_t *ta, int enable)
{
  tdb_log("td_ta_enable_stats: Not available");
  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Reset statistics.  */
td_err_e td_ta_reset_stats (const td_thragent_t *ta)
{
  tdb_log("td_ta_reset_stats: Not available");
  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Retrieve statistics from process associated with TA.  */
td_err_e td_ta_get_stats (const td_thragent_t *ta,
                                 td_ta_stats_t *statsp)
{
  tdb_log("td_ta_get_stats: Not available") ;
  return TD_NOCAPAB ;
}

/** ***************************************************************************** **/
/** ************   CONCURRENCY and PRIORITY  ************************************ **/
/** ***************************************************************************** **/

/** *****************   NOT USED IN GDB   *************************************** **/
/* Set suggested concurrency level for process associated with TA.  */
td_err_e td_ta_setconcurrency (const td_thragent_t *ta, int level)
{
  tdb_log("td_ta_setconcurrency: Not available");
  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Set priority of thread TH.  */
td_err_e td_thr_setprio (const td_thrhandle_t *th, int prio)
{
  tdb_log("td_thr_setprio: Not available");
  return TD_NOCAPAB ;
}

/** ***************************************************************************** **/
/** *****************   SIGNALS  ************************************************ **/
/** ***************************************************************************** **/


/** *****************   NOT USED IN GDB   *************************************** **/
/* Set pending signals for thread TH.  */
#if defined (TDB___linux__)
td_err_e td_thr_setsigpending (const td_thrhandle_t *th,
                                      unsigned char n, const sigset_t *ss)
#else
td_err_e td_thr_setsigpending (const td_thrhandle_t *th,
                                      unsigned char n, const sigset_t ss)
#endif
{
  tdb_log("td_thr_setsigpending");
  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Set signal mask for thread TH.  */
#if defined (TDB___linux__)
td_err_e td_thr_sigsetmask (const td_thrhandle_t *th,
                                   const sigset_t *ss)
#else
td_err_e td_thr_sigsetmask (const td_thrhandle_t *th,
                                   const sigset_t ss)
#endif
{
  tdb_log("td_thr_sigsetmask");
  return TD_NOCAPAB ;
}

/** ***************************************************************************** **/
/** *****************   SUSPEND/RESUME EXEC  ************************************ **/
/** ***************************************************************************** **/

/** *****************   NOT USED IN GDB   *************************************** **/
/* Suspend execution of thread TH.  */
td_err_e td_thr_dbsuspend (const td_thrhandle_t *th)
{
  tdb_log("td_thr_dbsuspend");
  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Resume execution of thread TH.  */
td_err_e td_thr_dbresume (const td_thrhandle_t *th)
{
  tdb_log("td_thr_dbresume: Not available");
  return TD_NOCAPAB ;
}

/** ***************************************************************************** **/
/** **************  THREAD LOCAL DATA  (TSD) ************************************ **/
/** ***************************************************************************** **/

/** *****************   NOT USED IN GDB   *************************************** **/
/* Return thread local data associated with key TK in thread TH.  */
td_err_e td_thr_tsd (const td_thrhandle_t *th,
                     const thread_key_t tk, void **data)
{
  tdb_log("td_thr_tsd: Not available");
#if 0
  if ((tk >= SCTK_THREAD_KEYS_MAX) || (tk < 0))
    return TD_ERR ;

  /* *data = th->th_unique->tls[tk] */

  /* threadData = *th->th_unique */
  ret = ps_pdread(th->th_ta_p->ph, th->th_unique, &threadData, sizeof(sctk_thread_data_t *)) ;
  assert (ret == PS_OK) ;
  /* threadTls = threadData.tls */
  ret = ps_pdread(th->th_ta_p->ph, ((sctk_thread_data_t *) threadData)->tls, &threadTls, sizeof(void **)) ;
  assert (ret == PS_OK) ;
  /* *data = threadTls[tk]*/
  ret = ps_pdread(th->th_ta_p->ph, threadTls + tk, *data, sizeof(void *)) ;
  assert (ret == PS_OK) ;
#endif

  return TD_NOCAPAB ;
}

/** *****************   NOT USED IN GDB   *************************************** **/
/* Call for each defined thread local data entry the callback function KI.  */
td_err_e td_ta_tsd_iter (const td_thragent_t *ta, td_key_iter_f *callback,
                         void *cbdata_p)
{
  tdb_log("td_ta_tsd_iter: Not available");
#if 0
  if (ta == NULL)
    return TD_BADTA ;

  td_thrhandle_t handle ;
  handle.th_ta_p = (td_thragent_t *) ta ;

  sctk_thread_data_t data ;

  /*read the nb of keys currently used*/
  ret = ps_lookup (ta->ph, TLD_KEY_POS, &tmp_addr) ;
  assert(ret == PS_OK);
  ps_pdread (ta->ph, tmp_addr, &last_key, sizeof(int));
  assert(ret == PS_OK);

  /*read the @ of the destructor function array */
  ret = ps_lookup (ta->ph, DESTR_FUNCT_TAB, &tmp_addr) ;
  assert(ret == PS_OK);
  ps_pdread (ta->ph, tmp_addr, &destr_funct_tab, sizeof(stck_ethread_key_destr_function_t *));
  assert(ret == PS_OK);

  for (key = 0; key < last_key; key++) {
    /*read the @ of the ith destructor function */
    ret = ps_pdread (ta->ph, destr_funct_tab + key, &destr_funct, sizeof(stck_ethread_key_destr_function_t));
    assert(ret == PS_OK);
    if (callback (key, destr_funct, cbdata_p) != TD_OK)
      return TD_DBERR;
  }
#endif
  return TD_NOCAPAB ;
}

#if defined (TDB_SunOS_SYS)
/*
 * Iterate over all threads blocked on a synchronization object.
 */
td_err_e td_sync_waiters(const td_synchandle_t *sh_p, td_thr_iter_f *cb, void *cbdata_p) {
  tdb_log("td_sync_waiters: Not available") ;
  return TD_NOCAPAB ;
}

/*
 * Enable/disable a process's synchronization object tracking.
 */
td_err_e td_ta_sync_tracking_enable(const td_thragent_t *ta, int on_off){
  tdb_log("td_ta_sync_tracking_enable: Not available") ;
  return TD_NOCAPAB ;
}

/*
 * Get information about a synchronization object.
 */
td_err_e td_sync_get_info(const td_synchandle_t *sh_p, td_syncinfo_t *ss_p){
  tdb_log("td_sync_get_info: Not available") ;
  return TD_NOCAPAB ;
}

/*
 * Get statistics for a synchronization object.
 */
td_err_e td_sync_get_stats(const td_synchandle_t *sh_p, td_syncstats_t *si_p){
  tdb_log("td_sync_get_stats: Not available") ;
  return TD_NOCAPAB ;
}

/*
 * Set the state of a synchronization object.
 */
td_err_e td_sync_setstate(const td_synchandle_t *sh_p, int value) {
  tdb_log("td_sync_setstate: Not available") ;
  return TD_NOCAPAB ;
}

/*
 * Iterate over the set of locks owned by a thread.
 */
td_err_e td_thr_lockowner(const td_thrhandle_t *th, td_sync_iter_f *cb, void *cbdata_p) {
  tdb_log("td_thr_lockowner: Not available") ;
  return TD_NOCAPAB ;
}


/*
 * Return the sync. handle of the object this thread is sleeping on.
 */
td_err_e td_thr_sleepinfo(const td_thrhandle_t *th, td_synchandle_t *sh_p) {
  tdb_log("td_thr_sleepinfo: Not available") ;
  return TD_NOCAPAB ;
}

#endif /*SunOS*/
