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


#ifndef __SCTK__LIB__THREAD_DBG__
#define __SCTK__LIB__THREAD_DBG__

/*allow the utilization of sys/regset.h::REG_X*/
#include "mpc_keywords.h"
#define _GNU_SOURCE

#include <thread_db.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>

#include "tdb_remote.h"

//#define ENABLE_TEST

//#define DIRECT_OUTPUT
#define DBG_OUTPUT
//#define NO_OUTPUT

/*
 * Library into which variables and functions must be looked up
 * Not used under GDB
 * We try to lookup either in LIBMPC_SO or in NULL in case of failure
 * of the former
 *
 * The name passed should be a basename, not a full pathname. (Ivan Soleimanipour)
 *
 * */
#ifndef LIB_RTDB_SO
  #define LIB_RTDB_SO "libmpc_framework.so"
#endif

#if defined (THREAD_DB_COMPLIANT)
  #define CONST_REGSET const
#else
  #define CONST_REGSET
#endif /* linux's thread_db.h is different from the API */

/*
 * Bottom end of the libthread_db : call back to GDB
 * (not present in the system under Linux)
 *
 * */
#if defined (TDB___linux__)
  #include "my_proc_service.h"
#else
  #include <proc_service.h>
#endif

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */

/*
 * Define ps_plog if necessary.
 *
 * glibc does not seem to provide ps_plog in proc_service.h.
 * GDB does provide this symbol unless the system already provides proc_service.h and HAVE_PROC_SERVICE_H is defined.
 */
#if !defined(GDB_PROC_SERVICE_H) || defined(HAVE_PROC_SERVICE_H)

static inline void __ps_plog(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  fprintf(stderr, fmt, args);
  fflush(stderr);

  va_end(args);
}

#pragma weak ps_plog=__ps_plog
#endif

#if defined(ENABLE_TEST)
  #include "sctk_lib_thread_db_tests.h"
#endif


/* implementation of the opaque structure*/
struct td_thragent {
  struct ps_prochandle *ph ;
  psaddr_t thread_list_p ; /*address of the list's head*/
  register_offsets_t reg_offsets    ;
};

/*
 * lookup table
 * Indices for the symbol names.
 *
 **/
enum {
  CREATE_EVENT,
  DEATH_EVENT,
  LIB_STATE,
  NB_THREAD,
  THREAD_LIST_HEAD,
  TA_EVENTS,
  REG_OFFSETS,
  NUM_MESSAGES
};

static const char *str_err_arr[] = {
  [TD_OK] =          "No error.",
  [TD_ERR] =         "No further specified error.",
  [TD_NOTHR] =       "No matching thread found.",
  [TD_NOSV] =        "No matching synchronization handle found.",
  [TD_NOLWP] =       "No matching light-weighted process found.",
  [TD_BADPH] =       "Invalid process handle.",
  [TD_BADTH] =       "Invalid thread handle.",
  [TD_BADSH] =       "Invalid synchronization handle.",
  [TD_BADTA] =       "Invalid thread agent.",
  [TD_BADKEY] =      "Invalid key.",
  [TD_NOMSG] =       "No event available.",
  [TD_NOFPREGS] =    "No floating-point register content available.",
  [TD_NOLIBTHREAD] = "Application not linked with thread library.",
  [TD_NOEVENT] =     "Requested event is not supported.",
  [TD_NOCAPAB] =     "Capability not available.",
  [TD_DBERR] =       "Internal debug library error.",
  [TD_NOAPLIC] =     "Operation is not applicable.",
  [TD_NOTSD] =       "No thread-specific data available.",
  [TD_MALLOC] =      "Out of memory.",
  [TD_PARTIALREG] =  "Not entire register set was read or written.",
  [TD_NOXREGS] =     "X register set not available for given thread.",
  [TD_TLSDEFER] =    "Thread has not yet allocated TLS for given module.",
  [TD_VERSION] =     "Version if libpthread and libthread_db do not match.",
  [TD_NOTLS] =       "There is no TLS segment in the given module."
};

const char **td_symbol_list (void) ;
ps_err_e ps_lookup (struct ps_prochandle *ps, int idx, psaddr_t *sym_addr);
td_err_e td_write(struct ps_prochandle *ph, int idx, int val) ;
td_err_e update_event_set (struct ps_prochandle *ph,
                           td_thr_events_t *event_addr,
                           td_thr_events_t *new_events,
                           int enable) ;

void td_utils_context_to_prgregset(const td_thragent_t *ta,
				   psaddr_t *context, prgregset_t prgregset) ;
void td_utils_prgregset_to_context(const td_thragent_t *ta,
				   prgregset_t prgregset, psaddr_t *context) ;

td_err_e td_is_ta_initialized (const td_thragent_t *ta) ;
td_err_e td_is_ta_ready (const td_thragent_t *ta) ;

void tdb_log (const char *fmt, ...) ;
void tdb_formated_assert_print (FILE * stream, const int line,
                                 const char *file, const char *func,
                                 const char *fmt, ...) ;

static inline const char *tdb_err_str (td_err_e err) {
  return str_err_arr[err] ;
}

ps_err_e ps_try_stop (struct ps_prochandle *ph) ;
ps_err_e ps_continue (struct ps_prochandle *ph) ;

static inline ps_err_e __ps_pstop(__UNUSED__ struct ps_prochandle *ph) {
  tdb_log("__ps_pstop WEAK");
  return PS_OK;
}

static inline ps_err_e __ps_pcontinue(__UNUSED__ struct ps_prochandle *ph) {
  tdb_log("__ps_pcontinue WEAK");
  return PS_OK;
}

/* NOLINTEND(clang-diagnostic-unused-function) */

#pragma weak ps_pstop=__ps_pstop
#pragma weak ps_pcontinue=__ps_pcontinue

/** our gdb only **/

#define INIT_VERBOSITY 0
extern int verbosity_level ;
void td_set_tdb_verbosity (int n) ;

int td_get_tdb_verbosity (void) ;

#endif
