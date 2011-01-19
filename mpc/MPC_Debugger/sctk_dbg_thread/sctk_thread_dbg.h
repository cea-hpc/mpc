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
#ifndef __SCTK__THREAD_DBG__
#define __SCTK__THREAD_DBG__

#ifdef __cplusplus
extern "C"
{
#endif

#include "sctk_config.h"
#include "sctk_thread.h"
#include "sctk_dbg_mpc.h"

#define SCTK_USE_THREAD_DEBUG
  
#if defined(SCTK_USE_THREAD_DEBUG)

  /** ** **/
  #include <thread_db.h>
  #include <sys/syscall.h>
//  #include "sctk_ethread_internal.h"
  #include "tdb_remote.h"
  /** **/

  //int mpc_thread_number_alive = 0 ;

  void sctk_thread_add (sctk_thread_data_t * item, void* tid);
  void sctk_thread_remove (sctk_thread_data_t * item);

  void sctk_thread_print_stack_out (void);
  void sctk_thread_debug_printf (const char *format, ...);
  void sctk_thread_current (void);

  void sctk_thread_goto (int j);
  void sctk_thread_enable_debug (void);
  void sctk_thread_disable_debug (void);
  void sctk_thread_list (void);
  void sctk_thread_list_task (int j);
  void sctk_thread_print_stack_in (int j);
  void sctk_thread_print_stack_task (int j);
  void sctk_thread_print_stack_out (void);

  /** ** **/
  int sctk_enable_lib_thread_db (void) ;
  
  td_thr_state_e sctk_get_gdb_status (sctk_thread_status_t status) ;
  
  int sctk_init_thread_debug (sctk_thread_data_t *item) ;
  struct sctk_ethread_per_thread_s;
  int sctk_refresh_thread_debug (struct sctk_ethread_per_thread_s *tid, sctk_thread_status_t s) ;
  
  int sctk_init_idle_thread_dbg (void *tid, void *start_fact) ;  
  int sctk_free_idle_thread_dbg (void *tid) ;
  int sctk_remove_thread (void *tid) ;

  int sctk_report_creation (void *tid) ;
  int sctk_report_death (void *tid) ;
  /** ** **/
  
#else
#define sctk_thread_add(a, b) (void)(0)
#define sctk_thread_remove(a) (void)(0)
#define sctk_thread_enable_debug() (void)(0)
#define sctk_thread_disable_debug() (void)(0)
#define sctk_thread_list() (void)(0)

/** ** **/
#define sctk_enable_lib_thread_db() (void)(0)
#define sctk_attach_lib_thread_db() (void)(0)
  
#define sctk_init_thread_debug(a, b) (void)(0)
#define sctk_refresh_thread_debug(a) (void)(0)
#define sctk_free_thread_debug(a) (void)(0)
#define sctk_init_idle_thread_dbg(a) (void)(0)
#define sctk_free_idle_thread_dbg(a) (void)(0)
  
#define sctk_report_creation() (void)(0) 
#define sctk_report_death() (void) (0)
  /** **/
#endif

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
