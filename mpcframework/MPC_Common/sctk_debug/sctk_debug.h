/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK___DEBUG__
#define __SCTK___DEBUG__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdarg.h>
#include <sctk_keywords.h>
#include <sctk_config.h>
#include <assert.h>
#include "sctk_bool.h"
#include "sctk_keywords.h"

#define SCTK_MAX_FILENAME_SIZE 1024
#define SCTK_DBG_INFO stderr,__LINE__,__FILE__,SCTK_FUNCTION

  extern bool sctk_restart_mode;
  extern bool sctk_check_point_restart_mode;
  extern bool sctk_migration_mode;
  extern char *sctk_multithreading_mode;
  extern int sctk_new_scheduler_engine_enabled;
  extern char *sctk_network_mode;
/*   extern const char *sctk_store_dir; */
  extern int sctk_is_in_fortran;
  extern int sctk_local_process_rank;
  extern int sctk_local_process_number;
  extern int sctk_node_rank;
  extern int sctk_node_number;

  void sctk_init (void);
  void sctk_leave (void);

  /* DEBUG FUNCTIONS START */
  void sctk_flush_version (void);
  void sctk_abort (void);
  void sctk_install_bt_sig_handler();
  void sctk_error (const char *fmt, ...);
  void sctk_formated_assert_print (FILE * stream, const int line,
				   const char *file, const char *func,
				   const char *fmt, ...);


  void MPC_printf (const char *fmt, ...);
  void sctk_debug_root(const char *fmt, ...);
  /* Print infos on processus and threads */
  char* sctk_print_debug_infos();
#ifdef SCTK_DEBUG_MESSAGES
  void sctk_debug (const char *fmt, ...);
  void sctk_info (const char *fmt, ...);
#else
  #if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
    #define sctk_debug(fmt,...) (void)(0)
    #define sctk_info(fmt,...) (void)(0)
  #else
    static inline void sctk_debug (const char *fmt, ...)
    {
    }
    static inline void sctk_info (const char *fmt, ...)
    {
    }
  #endif
#endif

#if (defined SCTK_HAVE_PRAGMA_MESSAGE) && (defined SCTK_DEBUG_MESSAGES)
    /* Add todo support (as stated in GCC doc
    * Supported since GCC 4.4.7 ignored in previous versions*/
    #define DO_PRAGMA(x) _Pragma (#x)

    #define TODO(x) DO_PRAGMA(message ("TODO - " #x))
    #define INFO(x) DO_PRAGMA(message ("INFO - " #x))
#else
     #define TODO(x)
     #define INFO(x)
#endif

  void sctk_silent_debug (const char *fmt, ...);
  void sctk_log (FILE * file, const char *fmt, ...);
  void sctk_warning (const char *fmt, ...);

  void sctk_size_checking_eq (size_t a, size_t b, char *ca, char *cb,
			      char *file, int line);
  void sctk_size_checking (size_t a, size_t b, char *ca, char *cb,
			   char *file, int line);

  void sctk_formated_dbg_print_abort (FILE * stream, const int line,
				      const char *file, const char *func,
				      const char *fmt, ...);

  size_t sctk_noalloc_fwrite (const void *ptr, size_t size, size_t nmemb,
			      FILE * stream);
  void sctk_noalloc_fprintf (FILE * stream, const char *format, ...);
  void sctk_noalloc_vfprintf (FILE * stream, const char *format, va_list ap);
  void sctk_noalloc_printf (const char *format, ...);
  void sctk_register_leave (void (*func) (void));
  void sctk_print_version (char *lib, int major, int minor);

  void sctk_debug_print_backtrace (const char *format, ...);
  void sctk_set_version_details (void);
  void sctk_unset_version_details (void);
  /* DEBUG FUNCTIONS END */

#ifndef MPC_Debugger
#define sctk_thread_add(a, b) (void)(0)
#define sctk_thread_remove(a) (void)(0)

/** ** **/
#define sctk_enable_lib_thread_db() (void)(0)

#define sctk_init_thread_debug(a) (void)(0)
#define sctk_refresh_thread_debug(a,b) (void)(0)
#define sctk_refresh_thread_debug_migration(a) (void)(0)

#define sctk_init_idle_thread_dbg(a,b) (void)(0)
#define sctk_free_idle_thread_dbg(a) (void)(0)

#define sctk_report_creation(a) (void)(0)
#define sctk_report_death(a) (void) (0)
  /** **/
#endif

#ifdef SCTK_64_BIT_ARCH
  static inline int sctk_safe_cast_long_int (long l)
  {
#ifdef SCTK_VERIFY_CAST
    assume (INT_MIN <= l);
    assume (l <= INT_MAX);
#endif
    return (int) l;
  }
#else
#define sctk_safe_cast_long_int(l) l
#endif

#define verb_abort() sctk_formated_assert_print(SCTK_DBG_INFO,	\
						"Verbose abort")

#define not_implemented() sctk_formated_dbg_print_abort(SCTK_DBG_INFO,"Function not implemented!!!!!")

#define not_available() sctk_formated_dbg_print_abort(SCTK_DBG_INFO,"Function not available!!!!!")

#define not_reachable() sctk_formated_dbg_print_abort(SCTK_DBG_INFO,"Function not reachable!!!!!")

#define sctk_check_equal_types(a,b) sctk_size_checking_eq(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)

#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#define sctk_nodebug(fmt,...) (void)(0)
#else
  static inline void sctk_nodebug (const char *fmt, ...)
  {
  }
#endif


/* Some Debug Helpers */
#define CRASH() do{ ((void (*)())0x0)();}while(0)

//If inline is not supported, disable assertions

#define assume_m(x, ...)                                                       \
  if (!(x)) {                                                                  \
    sctk_error("Error at %s!%d\n%s\n", __FILE__, __LINE__, #x);                \
    sctk_error(__VA_ARGS__);                                                   \
    sctk_abort();                                                              \
  }

  /** Print an error message and exit. It use the print formatting convention.
   * **/
#define sctk_fatal(...)                                                        \
  {                                                                            \
    sctk_error("Fatal error at %s!%d\n", __FILE__, __LINE__);                  \
    sctk_error(__VA_ARGS__);                                                   \
    sctk_abort();                                                              \
  }

#ifndef SCTK_NO_INLINE
#undef NO_INTERNAL_ASSERT
#define NO_INTERNAL_ASSERT
#endif

//for standard assert function, rewrite but maintain -DNDEBUG convention
#if !defined(NDEBUG) || !defined(NO_INTERNAL_ASSERT)
	#undef assert
	#define assert(op) do { if(expect_false(!(op))) sctk_formated_assert_print(SCTK_DBG_INFO, SCTK_STRING(op)); } while(0)
#endif //NDEBUG, NO_INTERNAL_ASSERT

/** Assume stay present independently of NDEBUG/NO_INTERNAL_ASSERT **/
#define assume(op) do { if(expect_false(!(op))) sctk_formated_assert_print(SCTK_DBG_INFO, SCTK_STRING(op)); } while(0)

#ifdef NO_INTERNAL_ASSERT
#define sctk_assert(op) (void)(0)
#define sctk_assert_func(op) (void)(0)
#else //NO_INTERNAL_ASSERT
#define sctk_assert_func(op) do{ op }while(0)
#define sctk_assert(op) if(expect_false(!(op)))				\
    sctk_formated_assert_print(SCTK_DBG_INFO,				\
			       SCTK_STRING(op))
#endif //NO_INTERNAL_ASSERT

  extern int sctk_only_once_while_val;
#define sctk_only_once() do{                                            \
    static int sctk_only_once_initialized = 0;                          \
    if(sctk_only_once_initialized == 1){                                \
      fprintf(stderr,"Multiple intialisation on line %d in file %s\n",__LINE__,__FILE__);       \
      sctk_abort();                                                             \
    }                                                                   \
    sctk_only_once_initialized++;                                       \
  }while(sctk_only_once_while_val)

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
