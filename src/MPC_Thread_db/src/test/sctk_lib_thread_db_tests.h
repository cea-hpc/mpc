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


#ifndef SCTK_LIB_THREAD_DB_TEST_H
#define SCTK_LIB_THREAD_DB_TEST_H

#define ENABLE_INIT_TEST
#define TD_CONTINUE ((td_err_e)(-123456))

#include <thread_db.h>
#include "../sctk_lib_thread_db.h"

struct test_td_data {
  int inside_tester ;
  td_thragent_t *ta;
} ;

extern struct test_td_data test_data ;

td_err_e test_td_init (void) ;

int test_td_check_infaddr_eq(psaddr_t addr, int value) ;
int test_td_check_thread_eq(const td_thrinfo_t *info1, const td_thrinfo_t *info2) ;

int test_td_return_codes (void) ;
int test_td_consistency (void) ;
int test_td_err_callback (const td_thrhandle_t *th_p, void *cbdata_p) ;
int test_td_consistency_callback (const td_thrhandle_t *th_p, void *cbdata_p) ;

/* Generate new thread debug library handle for process PS.  */
td_err_e test_td_ta_new (struct ps_prochandle *ps, td_thragent_t **ta) ;

/* Free resources allocated for TA.  */
td_err_e test_td_ta_delete (td_thragent_t *ta) ;

/* Get number of currently running threads in process associated with TA.  */
td_err_e test_td_ta_get_nthreads (const td_thragent_t *ta, int *np) ;

/* Return process handle passed in `td_ta_new' for process associated with
   TA.  */
td_err_e test_td_ta_get_ph (const td_thragent_t *ta,
			    struct ps_prochandle **ph) ;

/* Map thread library handle PT to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e test_td_ta_map_id2thr (const td_thragent_t *ta, pthread_t pt,
				td_thrhandle_t *th) ;

/* Map process ID LWPID to thread debug library handle for process
   associated with TA and store result in *TH.  */
td_err_e test_td_ta_map_lwp2thr (const td_thragent_t *ta, lwpid_t lwpid,
				 td_thrhandle_t *th) ;


/* Call for each thread in a process associated with TA the callback function
   CALLBACK.  */
td_err_e test_td_ta_thr_iter (const td_thragent_t *ta,
			      td_thr_iter_f *callback, void *cbdata_p,
			      td_thr_state_e state, int ti_pri,
			      sigset_t *ti_sigmask_p,
			      unsigned int ti_user_flags) ;

/* Call for each defined thread local data entry the callback function KI.  */
td_err_e test_td_ta_tsd_iter (const td_thragent_t *ta, td_key_iter_f *ki,
			      void *p) ;


/* Get event address for EVENT.  */
td_err_e test_td_ta_event_addr (const td_thragent_t *ta,
				td_event_e event, td_notify_t *ptr) ;

/* Enable EVENT in global mask.  */
td_err_e test_td_ta_set_event (const td_thragent_t *ta,
			       td_thr_events_t *event) ;

/* Disable EVENT in global mask.  */
td_err_e test_td_ta_clear_event (const td_thragent_t *ta,
				 td_thr_events_t *event) ;

/* Return information about last event.  */
td_err_e test_td_ta_event_getmsg (const td_thragent_t *ta,
				  td_event_msg_t *msg) ;


/* Set suggested concurrency level for process associated with TA.  */
td_err_e test_td_ta_setconcurrency (const td_thragent_t *ta, int level) ;


/* Enable collecting statistics for process associated with TA.  */
td_err_e test_td_ta_enable_stats (const td_thragent_t *ta, int enable) ;

/* Reset statistics.  */
td_err_e test_td_ta_reset_stats (const td_thragent_t *ta) ;

/* Retrieve statistics from process associated with TA.  */
td_err_e test_td_ta_get_stats (const td_thragent_t *ta,
			       td_ta_stats_t *statsp) ;


/* Validate that TH is a thread handle.  */
td_err_e test_td_thr_validate (const td_thrhandle_t *th) ;

/* Return information about thread TH.  */
td_err_e test_td_thr_get_info (const td_thrhandle_t *th,
                                 td_thrinfo_t *infop) ;

/* Retrieve floating-point register contents of process running thread TH.  */
td_err_e test_td_thr_getfpregs (const td_thrhandle_t *th,
				prfpregset_t *regset) ;

/* Retrieve general register contents of process running thread TH.  */
td_err_e test_td_thr_getgregs (const td_thrhandle_t *th,
			       prgregset_t gregs) ;

/* Retrieve extended register contents of process running thread TH.  */
td_err_e test_td_thr_getxregs (const td_thrhandle_t *th, void *xregs) ;

/* Get size of extended register set of process running thread TH.  */
td_err_e test_td_thr_getxregsize (const td_thrhandle_t *th, int *sizep) ;

/* Set floating-point register contents of process running thread TH.  */
td_err_e test_td_thr_setfpregs (const td_thrhandle_t *th,
				const prfpregset_t *fpregs) ;

/* Set general register contents of process running thread TH.  */
td_err_e test_td_thr_setgregs (const td_thrhandle_t *th,
                               CONST_REGSET prgregset_t gregs) ;

/* Set extended register contents of process running thread TH.  */
td_err_e test_td_thr_setxregs (const td_thrhandle_t *th,
                                 const void *addr) ;


/* Get address of the given module's TLS storage area for the given thread.  */
td_err_e test_td_thr_tlsbase (const td_thrhandle_t *th,
                                unsigned long int modid,
                                psaddr_t *base) ;

/* Get address of thread local variable.  */
td_err_e test_td_thr_tls_get_addr (const td_thrhandle_t *th,
                                     psaddr_t map_address, size_t offset,
                                     psaddr_t *address) ;


/* Enable reporting for EVENT for thread TH.  */
td_err_e test_td_thr_event_enable (const td_thrhandle_t *th, int event) ;

/* Enable EVENT for thread TH.  */
td_err_e test_td_thr_set_event (const td_thrhandle_t *th,
                                  td_thr_events_t *event) ;

/* Disable EVENT for thread TH.  */
td_err_e test_td_thr_clear_event (const td_thrhandle_t *th,
                                    td_thr_events_t *event) ;

/* Get event message for thread TH.  */
td_err_e test_td_thr_event_getmsg (const td_thrhandle_t *th,
                                     td_event_msg_t *msg) ;


/* Set priority of thread TH.  */
td_err_e test_td_thr_setprio (const td_thrhandle_t *th, int prio) ;


/* Set pending signals for thread TH.  */
/* Set pending signals for thread TH.  */
#if defined (TDB___linux__)
td_err_e test_td_thr_setsigpending (const td_thrhandle_t *th,
                                    unsigned char n, const sigset_t *ss) ;
#else
td_err_e test_td_thr_setsigpending (const td_thrhandle_t *th,
                                    unsigned char n, const sigset_t ss) ;
#endif

/* Set signal mask for thread TH.  */
#if defined (TDB___linux__)
td_err_e test_td_thr_sigsetmask (const td_thrhandle_t *th,
                                 const sigset_t *ss) ;
#else
td_err_e test_td_thr_sigsetmask (const td_thrhandle_t *th,
                                 const sigset_t ss) ;
#endif


/* Return thread local data associated with key TK in thread TH.  */
td_err_e test_td_thr_tsd (const td_thrhandle_t *th,
                            const thread_key_t tk, void **data) ;


/* Suspend execution of thread TH.  */
td_err_e test_td_thr_dbsuspend (const td_thrhandle_t *th) ;

/* Resume execution of thread TH.  */
td_err_e test_td_thr_dbresume (const td_thrhandle_t *th) ;

#endif
