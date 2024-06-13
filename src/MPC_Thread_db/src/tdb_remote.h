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


#ifndef __TDB_REMOTE__
#define __TDB_REMOTE__

#include <assert.h>
#ifdef MPC_Thread_db
#include "mpc_config.h"
#endif
#include <thread_db.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

/* GDB does not follow the API design regarding name lookup :
 * global variables needed at initialization must be in a static
 * library.
 * DBX implements the correct behavior
 * */

//#define THREAD_DB_COMPLIANT

/* import of system-specific constants */


#include <sys/ucontext.h>

#define TDB_INTERNAL_ASSERT

#ifdef TDB_INTERNAL_ASSERT
//  #undef NDEBUG
  #define tdb_assert(a) assert(a)
#else
  #define tdb_assert(a) (void)(0)
#endif

#define RTDB_NOLOCK (void *)-1

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum {
    TDB_ERR,
    TDB_OK,
    TDB_MALLOC,
    TDB_NO_THR,

    TDB_LIB_NOT_STARTED,
    TDB_LIB_ALREADY,
    TDB_LIB_INHIBED,

    TDB_LOCK_UNINIT,

    TDB_DBERR
  } tdb_err_e ;

  typedef enum {
    TDB_LIB_INHIB,
    TDB_LIB_TO_START,
    TDB_LIB_STARTED,
  } tdb_lib_state_e ;

  typedef struct tdb_thread_debug_s {
    const void *tid ;

    void *context ;
    td_thrinfo_t info ;
    td_event_e last_event ;

    void *extls;

    volatile struct tdb_thread_debug_s *next ;
    volatile struct tdb_thread_debug_s *prev ;

  } tdb_thread_debug_t ;


#if defined(MPC_I686_ARCH)
#include <sys/reg.h>
  #define TDB_PC EIP
  #define TDB_SP UESP
  typedef struct {
    size_t off_eip ;
    size_t off_esp ;
    size_t off_ebp ;
    size_t off_edi ;
    size_t off_ebx ;
    size_t size ;
  } register_offsets_t ;
#elif defined(MPC_X86_64_ARCH)
#include <sys/reg.h>
  #define TDB_PC RIP
  #define TDB_SP RSP
  typedef struct {
    size_t off_rbx ;
    size_t off_rbp ;
    size_t off_r12 ;
    size_t off_r13 ;
    size_t off_r14 ;
    size_t off_r15 ;
    size_t off_rsp ;
    size_t off_pc ;
    size_t size ;
  } register_offsets_t ;
#else
  #define TDB_PC 0
  #define TDB_SP 0
  typedef struct {
    size_t size ;
  } register_offsets_t ;

  #warning "Architecture not supported"
#endif


  typedef struct {
    void *lock ;

    int (*acquire) (void *) ;
    int (*release) (void *) ;
    int (*free) (void *) ;
  } tdb_lock_t ;

  extern volatile register_offsets_t rtdb_reg_offsets ;
  extern volatile int rtdb_thread_number_alive ;
  extern volatile tdb_thread_debug_t *rtdb_thread_list;
  extern volatile tdb_lock_t rtdb_lock ;

  /* indicates whether lib_threaddb is already initialized */
  extern volatile int rtdb_lib_state ;
  /* static structure for enabled event, always reachable */
  extern volatile struct td_thr_events rtdb_ta_events ;

  void rtdb_log (const char *fmt, ...) ;

  void rtdb_bp_creation (void) ;
  void rtdb_bp_death (void) ;

  tdb_err_e rtdb_enable_lib_thread_db (void) ;
  tdb_err_e rtdb_disable_lib_thread_db (void) ;

  tdb_err_e rtdb_add_thread (const void *tid, volatile tdb_thread_debug_t **thread) ;
  tdb_err_e rtdb_remove_thread (volatile tdb_thread_debug_t *thread) ;
  volatile tdb_thread_debug_t *rtdb_get_thread (const void *tid) ;

  tdb_err_e rtdb_report_creation_event (volatile tdb_thread_debug_t *thread) ;
  tdb_err_e rtdb_report_death_event    (volatile tdb_thread_debug_t *thread) ;

  void rtdb_unknown_start_function(void) ;

  /*
   * NOLINTBEGIN(clang-diagnostic-unused-function):
   * Clang wrongly reports static inline functions defined
   * in header files as unused, even if they are actually
   * used through includes.
   */

  static inline tdb_err_e rtdb_set_thread_startfunc (
    volatile tdb_thread_debug_t *thread, void *fct){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    if (fct != NULL)
      thread->info.ti_startfunc = fct ;

    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_tls (
    volatile tdb_thread_debug_t *thread, char *tls){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    thread->info.ti_tls = tls ;

    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_stkbase (
    volatile tdb_thread_debug_t *thread, void *stkbase){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    thread->info.ti_stkbase = stkbase ;
    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_stksize (
    volatile tdb_thread_debug_t *thread, int stksize){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    thread->info.ti_stksize = stksize ;
    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_type (
    volatile tdb_thread_debug_t *thread, td_thr_type_e type){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    thread->info.ti_type = type ;
    rtdb_log("set type for thead %p : %d", thread, type);
    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_context (
    volatile tdb_thread_debug_t *thread, void *context){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    rtdb_log("set context for tid %p : %p", thread, context);
    thread->context = context ;

    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_thread_extls (
    volatile tdb_thread_debug_t *thread, void *extls){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    rtdb_log("set extls for tid %p : %p", thread, extls);
    thread->extls = extls ;

    return TDB_OK ;
  }

  /* *************************************************************** */

  static inline tdb_err_e rtdb_update_thread_state (
    volatile tdb_thread_debug_t *thread, td_thr_state_e state){
/*     td_thr_state_e old_state ; */

/*     tdb_assert(thread != NULL) ; */
/*     if (thread == NULL) return TDB_NO_THR ; */

/*     rtdb_log("new state for thead %p : %d (old: %d)", thread, state, thread->info.ti_state); */

    /*ensure that the thread is active for the assert*/
/*     old_state = thread->info.ti_state ; */
/*     thread->info.ti_state = TD_THR_ACTIVE ; */

/*     tdb_assert(old_state != TD_THR_ANY_STATE); */
/*     tdb_assert(old_state != state); */

    thread->info.ti_state = state ;
    return TDB_OK ;
  }

  static inline lwpid_t rtdb_get_lid (void) {
    lwpid_t lid ;
#if defined (TDB___linux__)
    lid = syscall(SYS_gettid);
#elif defined (TDB_SunOS_SYS)
    lid = lwp_self() ;
#else
    char *NO_SYSTEM_SPECIFIED = 0 ;
    rtdb_log("rtdb_update_thread_this_lid No system specified");
    tdb_assert(NO_SYSTEM_SPECIFIED) ;

    lid = 0 ;
#endif
    return lid ;
  }

  static inline tdb_err_e rtdb_update_thread_lid (
    volatile tdb_thread_debug_t *thread, lwpid_t lid){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    tdb_assert(lid != 0) ;
    tdb_assert(lid == rtdb_get_lid());

    thread->info.ti_lid = lid ;
    return TDB_OK ;
  }

#define rtdb_update_thread_this_lid(a) rtdb_update_thread_lid (a, rtdb_get_lid())

  static inline tdb_err_e rtdb_update_thread_pri (
    volatile tdb_thread_debug_t *thread, int pri){

    tdb_assert(thread != NULL) ;
    if (thread == NULL) return TDB_NO_THR ;

    thread->info.ti_pri = pri ;
    return TDB_OK ;
  }

  /* ******************************************************************/
#if defined(MPC_I686_ARCH)
  static inline tdb_err_e rtdb_set_eip_offset (size_t offset) {
    rtdb_reg_offsets.off_eip = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_esp_offset (size_t offset) {
    rtdb_reg_offsets.off_esp = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_ebp_offset (size_t offset) {
    rtdb_reg_offsets.off_ebp = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_ebx_offset (size_t offset) {
    rtdb_reg_offsets.off_ebx = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_edi_offset (size_t offset) {
    rtdb_reg_offsets.off_edi = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
#elif defined(MPC_X86_64_ARCH)
  static inline tdb_err_e rtdb_set_rbx_offset (size_t offset) {
    rtdb_reg_offsets.off_rbx = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_rbp_offset (size_t offset) {
    rtdb_reg_offsets.off_rbp = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_r12_offset (size_t offset) {
    rtdb_reg_offsets.off_r12 = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_r13_offset (size_t offset) {
    rtdb_reg_offsets.off_r13 = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_r14_offset (size_t offset) {
    rtdb_reg_offsets.off_r14 = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_r15_offset (size_t offset) {
    rtdb_reg_offsets.off_r15 = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_rsp_offset (size_t offset) {
    rtdb_reg_offsets.off_rsp = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
  static inline tdb_err_e rtdb_set_pc_offset (size_t offset) {
    rtdb_reg_offsets.off_pc = offset ;

    if (rtdb_reg_offsets.size < (offset + 1))
      rtdb_reg_offsets.size = offset + 1  ;

    rtdb_log ("%s -> %d, size = %d", __FUNCTION__, offset, rtdb_reg_offsets.size);
    return TDB_OK ;
  }
#else
  #warning System unsupported by lib_thread_db
#endif

  static inline tdb_err_e rtdb_set_lock (void *lock,
					 int (*acquire) (void *),
					 int (*release) (void *),
					 int (*lock_free) (void *))
  {
    rtdb_lock.lock = lock ;
    tdb_assert(lock != NULL) ;
    if (lock == NULL) return TDB_ERR ;

    rtdb_lock.acquire = acquire ;
    tdb_assert(acquire != NULL) ;
    if (acquire == NULL) return TDB_ERR ;

    rtdb_lock.release = release ;
    tdb_assert(release != NULL) ;
    if (release == NULL) return TDB_ERR ;

    /* optional, actually never called */
    rtdb_lock.free = lock_free ;

    return TDB_OK ;
  }

  static inline tdb_err_e rtdb_set_nolock (void) {
    rtdb_lock.lock = RTDB_NOLOCK ;
    return TDB_OK ;
  }

  // NOLINTEND(clang-diagnostic-unused-function)

#define rtdb_remove_thread_tid(a) rtdb_remove_thread (rtdb_get_thread(a))
#define rtdb_report_creation_event_tid(a) rtdb_report_creation_event (rtdb_get_thread(a))
#define rtdb_report_death_event_tid(a) rtdb_report_death_event (rtdb_get_thread(a))
#define rtdb_set_thread_startfunc_tid(a, b) rtdb_set_thread_startfunc (rtdb_get_thread(a),b)
#define rtdb_set_thread_tls_tid(a, b) rtdb_set_thread_tls (rtdb_get_thread(a),b)
#define rtdb_set_thread_stkbase_tid(a, b) rtdb_set_thread_stkbase (rtdb_get_thread(a),b)
#define rtdb_set_thread_stksize_tid(a, b) rtdb_set_thread_stksize (rtdb_get_thread(a),b)
#define rtdb_set_thread_type_tid(a, b) rtdb_set_thread_type (rtdb_get_thread(a),b)
#define rtdb_set_thread_context_tid(a, b) rtdb_set_thread_context (rtdb_get_thread(a),b)
#define rtdb_update_thread_state_tid(a, b) rtdb_update_thread_state (rtdb_get_thread(a),b)
#define rtdb_update_thread_lid_tid(a, b) rtdb_update_thread_lid (rtdb_get_thread(a),b)
#define rtdb_update_thread_this_lid_tid(a) rtdb_update_thread_this_lid (rtdb_get_thread(a))
#define rtdb_update_thread_pri_tid(a, b) rtdb_update_thread_pri (rtdb_get_thread(a),b)

#ifdef __cplusplus
}
#endif

#endif
