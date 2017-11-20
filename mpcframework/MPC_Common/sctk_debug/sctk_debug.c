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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>

#define HAS_UCTX

#ifdef HAS_UCTX
#include <sys/ucontext.h>
#endif

#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_spinlock.h"
#include "sctk.h"


#include "sctk_shell_colors.h"
#include "sctk_runtime_config.h"

#ifdef MPC_Debugger
#include <sctk_debugger.h>
#endif

#define WRITE_BUFFER_SIZE (4*1024)
#define SMALL_BUFFER_SIZE (4*1024)
#define DEBUG_INFO_SIZE (64)
static bool sctk_have_shell_colors;

int sctk_only_once_while_val;
static int sctk_debug_version_details = 0;
static char sctk_version_buff[WRITE_BUFFER_SIZE];
static __thread char ret[DEBUG_INFO_SIZE];

int sctk_process_number = 1;
int sctk_process_rank = 0;
int sctk_node_number = 1;
int sctk_node_rank = 0;
int sctk_is_in_fortran = 0;

/* rank of the process inside the node */
int sctk_local_process_rank = 0;
/* number of processes inside the node */
int sctk_local_process_number = 1;

volatile int sctk_multithreading_initialised = 0;

static int
sctk_vsnprintf (char *s, size_t n, const char *format, va_list ap)
{
  int res;
#ifdef HAVE_VSNPRINTF
  res = vsnprintf (s, n, format, ap);
#else
  res = vsprintf (s, format, ap);
#endif
  return res;
}

static int
sctk_snprintf (char *restrict s, size_t n, const char *restrict format, ...)
{
  va_list ap;
  int res;
  va_start (ap, format);
  res = sctk_vsnprintf (s, n, format, ap);
  va_end (ap);
  return res;
}

/**********************************************************************/
/*Threads support                                                     */
/**********************************************************************/
#ifndef MPC_Threads
void
sctk_get_thread_info (int *task_id, int *thread_id)
{
  *task_id = -1;
  *thread_id = -1;
}

int
sctk_thread_get_vp ()
{
  return -1;
}
#endif

char*
sctk_print_debug_infos()
{
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  if (sctk_have_shell_colors) {
    snprintf(ret,
			DEBUG_INFO_SIZE,
        SCTK_COLOR_GREEN([%4d/%4d/%4d/)
        SCTK_COLOR_GREEN_BOLD(%4d)
        SCTK_COLOR_GREEN(/%4d/%4d]),
        sctk_node_rank,
        sctk_process_rank, sctk_thread_get_vp (), task_id, thread_id, sctk_local_process_rank);
  }
  else {
    snprintf(ret,
			DEBUG_INFO_SIZE,
        "[%4d/%4d/%4d/%4d/%4d/%4d]",
        sctk_node_rank,
        sctk_process_rank, sctk_thread_get_vp (), task_id, thread_id, sctk_local_process_rank);
  }

  return ret;
}

/**********************************************************************/
/*No alloc IO                                                         */
/**********************************************************************/
size_t
sctk_noalloc_fwrite (const void *ptr, size_t size, size_t nmemb,
		     FILE * stream)
{
  int fd;
  size_t tmp;

  fd = 2;

  if (stream == stderr)
    {
      fd = 2;
    }
  else if (stream == stdout)
    {
      fd = 1;
    }
  else
    {
      sctk_error ("Unknown file descriptor");
      abort ();
    }

  tmp = write (fd, ptr, size * nmemb);
  return tmp;
}

void
sctk_noalloc_fprintf (FILE * stream, const char *format, ...)
{
  va_list ap;
  static char tmp[WRITE_BUFFER_SIZE];
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_spinlock_lock (&lock);
  va_start (ap, format);
  sctk_vsnprintf (tmp, WRITE_BUFFER_SIZE, format, ap);
  va_end (ap);
  sctk_noalloc_fwrite (tmp, 1, strlen (tmp), stream);
  sctk_spinlock_unlock (&lock);
}

void
sctk_noalloc_vfprintf (FILE * stream, const char *format, va_list ap)
{
  static char tmp[WRITE_BUFFER_SIZE];
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_spinlock_lock (&lock);
  sctk_vsnprintf (tmp, WRITE_BUFFER_SIZE, format, ap);
  sctk_noalloc_fwrite (tmp, 1, strlen (tmp), stream);
  sctk_spinlock_unlock (&lock);
}

void
sctk_noalloc_printf (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  sctk_noalloc_vfprintf (stdout, format, ap);
  va_end (ap);
}

/**********************************************************************/
/*Backtrace                                                           */
/**********************************************************************/

void sctk_c_backtrace(char *reason, void * override) {
  fprintf(stderr, "============== BACKTRACE =============== [%s]\n", reason);
  void *bts[100];
  int bs = backtrace(bts, 100);

  if (override) {
    bts[1] = override;
  }

  backtrace_symbols_fd(bts + 2, bs - 2, STDERR_FILENO);
  fprintf(stderr, "========================================\n");
}

void
sctk_debug_print_backtrace (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
#ifdef MPC_Debugger
  sctk_vprint_backtrace (format, ap);
#else
  char reason[100];
  vsnprintf(reason, 100, format, ap);
  sctk_c_backtrace(reason, NULL);
#endif
  va_end(ap);
}

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
#ifdef MPC_Message_Passing
void sctk_net_abort ();
#endif
void
sctk_abort (void)
{
  static volatile int done = 0;
  sctk_debug_print_backtrace("sctk_abort");
#ifdef MPC_Message_Passing
  if(done == 0){
    done = 1;
    sctk_net_abort ();
  }
#endif
  abort();
}

/**********************************************************************/
/* Backtrace on Signal                                                */
/**********************************************************************/

void backtrace_sig_handler(int sig, siginfo_t *info, void *arg) {
  char *ssig = strsignal(sig);

  if(sig == SIGINT)
  {
	  fprintf(stderr, "MPC has been interrupted by user --> SIGINT\n");
	  exit(sig);
  }
  sctk_error("==========================================================");
  sctk_error("                                                          ");
  sctk_error("Process Caught signal %s(%d)", ssig, sig);
  sctk_error("                                                          ");
  switch (sig) {
  case SIGILL:
  case SIGFPE:
  case SIGTRAP:
    sctk_error("Instruction : %p ", info->si_addr);
    break;

  case SIGBUS:
  case SIGSEGV:
    sctk_error("Faulty address was : %p ", info->si_addr);
    break;
  }
  sctk_error("                                                          ");
  sctk_error(" INFO : Disable signal capture by exporting MPC_BT_SIG=0  ");
  sctk_error("        or through MPC's configuration file (mpc_bt_sig)  ");
  sctk_error("                                                          ");
  sctk_error("!!! MPC will now segfault indiferently from the signal !!!");
  sctk_error("==========================================================");

#ifdef HAS_UCTX
  fprintf(stderr, "\n");
  ucontext_t *context = (ucontext_t *)arg;
  void *caller_add = NULL;

  if (context) {
#if defined(__i386__)
    caller_add = (void *)context->uc_mcontext.gregs[REG_EIP];
#elif defined(__x86_64__)
    caller_add = (void *)context->uc_mcontext.gregs[REG_RIP];
#endif
  }

  sctk_c_backtrace(ssig, caller_add);
  fprintf(stderr, "\n");

  fprintf(stderr, "*************************************************\n");
  fprintf(stderr, "* MPC has tried to backtrace but note that      *\n");
  fprintf(stderr, "* this operation is not signal safe you may     *\n");
  fprintf(stderr, "* get either an incomplete or invalid backtrace *\n");
  fprintf(stderr, "* you may now launch GDB -- good debugging      *\n");
  fprintf(stderr, "*************************************************\n");

  fprintf(stderr, "\n");
#endif /* HAS_UCTX */

  sctk_error(
      "/!\\ CHECK THE ACTUAL SIGNAL IN PREVIOUS ERROR (may not be SIGSEV)");

  CRASH();
}

void sctk_install_bt_sig_handler() {
  char *cmd = getenv("MPC_BT_SIG");

  struct sigaction action;

  memset(&action, 0, sizeof(action));

  action.sa_sigaction = backtrace_sig_handler;
  action.sa_flags = SA_SIGINFO;

  if (cmd) {
    /* Forced disable */
    if (!strcmp(cmd, "0")) {
      return;
    }
  }

  if (cmd || sctk_runtime_config_get()->modules.debugger.mpc_bt_sig) {

    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
  }
}

/**********************************************************************/
/*Messages                                                            */
/**********************************************************************/

int
MPC_check_compatibility_lib (int major, int minor, char *pre)
{
  static char errro_msg[4096];
  if ((major != SCTK_VERSION_MAJOR) || (minor != SCTK_VERSION_MINOR)
      || (strcmp (pre, SCTK_VERSION_PRE) != 0))
    {
      sprintf (errro_msg,
	       "MPC version used for this file (%d.%d%s) differs from the library used for the link (%d.%d%s)\n",
	       major, minor, pre, SCTK_VERSION_MAJOR, SCTK_VERSION_MINOR,
	       SCTK_VERSION_PRE);
      sctk_warning (errro_msg);
      return 1;
    }
  return 0;
}

  void
  MPC_printf (const char *fmt, ...)
  {

    va_list ap;
    char buff[SMALL_BUFFER_SIZE];
    int task_id;
    int thread_id;

    sctk_get_thread_info (&task_id, &thread_id);

    va_start (ap, fmt);

    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                  "%s %s",
                  sctk_print_debug_infos(),
                  fmt);

    sctk_noalloc_vfprintf (stderr, buff, ap);
    fflush (stderr);
    va_end (ap);
  }

/* Sometimes it's interesting if only rank 0 show messages... */
  void
  sctk_debug_root(const char *fmt, ...)
  {
    va_list ap;
    char buff[SMALL_BUFFER_SIZE];
    int task_id;
    int thread_id;

    if (sctk_process_rank!=0)
      return;

    sctk_get_thread_info (&task_id, &thread_id);

    va_start (ap, fmt);
    if (sctk_have_shell_colors) {
      sctk_snprintf (buff, SMALL_BUFFER_SIZE, SCTK_COLOR_RED_BOLD("%s")"\n", fmt);
    }
    else {
      sctk_snprintf (buff, SMALL_BUFFER_SIZE, "%s\n", fmt);
    }
    sctk_noalloc_vfprintf (stderr, buff, ap);
    fflush (stderr);
    va_end (ap);
  }


#ifdef SCTK_DEBUG_MESSAGES
  void
  sctk_info (const char *fmt, ...)
  {
#if defined(MPC_Message_Passing) || defined(MPC_Threads)
    if( sctk_get_verbosity() < 2 )
      return;
#endif

    va_list ap;
    char buff[SMALL_BUFFER_SIZE];
    int task_id;
    int thread_id;

    sctk_get_thread_info (&task_id, &thread_id);

    va_start (ap, fmt);
    if (sctk_have_shell_colors) {
      char info_message[SMALL_BUFFER_SIZE];
      sctk_snprintf (info_message, SMALL_BUFFER_SIZE, SCTK_COLOR_GRAY_BOLD("%s"), fmt);

      sctk_snprintf (buff, SMALL_BUFFER_SIZE,
          "%s INFO %s\n",
          sctk_print_debug_infos(),
          info_message);
    }
    else {
      sctk_snprintf (buff, SMALL_BUFFER_SIZE,
          "%s INFO %s\n",
          sctk_print_debug_infos(),
          fmt);
    }
    sctk_noalloc_vfprintf (stderr, buff, ap);
    va_end (ap);
  }

  void
  sctk_debug (const char *fmt, ...)
  {
#if defined(MPC_Message_Passing) || defined(MPC_Threads)
    if( sctk_get_verbosity() < 3 )
      return;
#endif

    va_list ap;
    char buff[SMALL_BUFFER_SIZE];
    int task_id;
    int thread_id;

    sctk_get_thread_info (&task_id, &thread_id);

    va_start (ap, fmt);
    if (sctk_have_shell_colors) {
      char debug_message[SMALL_BUFFER_SIZE];
      sctk_snprintf (debug_message, SMALL_BUFFER_SIZE, SCTK_COLOR_CYAN_BOLD("%s"), fmt);

      sctk_snprintf (buff, SMALL_BUFFER_SIZE,
          "%s DEBUG %s\n",
          sctk_print_debug_infos(),
          debug_message);
    }
    else {
      sctk_snprintf (buff, SMALL_BUFFER_SIZE,
          "%s DEBUG %s\n",
          sctk_print_debug_infos(),
          fmt);
    }
    sctk_noalloc_vfprintf (stderr, buff, ap);
    fflush (stderr);
    va_end (ap);
  }
#endif


void
sctk_error (const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  va_start (ap, fmt);
  if (sctk_have_shell_colors) {
	  char error_message[SMALL_BUFFER_SIZE];
	  sctk_snprintf (error_message, SMALL_BUFFER_SIZE, SCTK_COLOR_RED_BOLD("%s"), fmt);

	  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
			  "%s ERROR %s\n",
			  sctk_print_debug_infos(),
			  error_message);
  }
  else {
	  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
			  "%s ERROR %s\n",
			  sctk_print_debug_infos(),
			  fmt);
  }
  sctk_noalloc_vfprintf (stderr, buff, ap);
  va_end (ap);
}

void
sctk_formated_assert_print (FILE * stream, const int line, const char *file,
			    const char *func, const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  if (func == NULL)
    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
		   "%s Assertion %s fail at line %d file %s\n",
     sctk_print_debug_infos(),
		   fmt, line,
		   file);
  else
    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
		   "%s Assertion %s fail at line %d file %s func %s\n",
      sctk_print_debug_infos(),
		   fmt, line, file, func);
  va_start (ap, fmt);
  sctk_noalloc_vfprintf (stream, buff, ap);
  va_end (ap);
  sctk_abort ();
}

void
sctk_silent_debug (const char *fmt, ...)
{
}

void
sctk_log (FILE * file, const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  va_start (ap, fmt);
  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
		 "%s %s\n",
     sctk_print_debug_infos(),
		 fmt);
  vfprintf (file, buff, ap);
  va_end (ap);
  fflush (file);
}

void
sctk_warning (const char *fmt, ...)
{
/* #if defined(MPC_Message_Passing) || defined(MPC_Threads) */
/*   if( sctk_get_verbosity() < 1 ) */
/*     return; */
/* #endif */

  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  va_start (ap, fmt);
  if (sctk_have_shell_colors) {
	  char warning_message[SMALL_BUFFER_SIZE];
	  sctk_snprintf (warning_message, SMALL_BUFFER_SIZE, SCTK_COLOR_YELLOW_BOLD("%s"), fmt);

	  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
			  "%s WARNING %s\n",
			  sctk_print_debug_infos(),
			  warning_message);
  }
  else {
	  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
			  "%s WARNING %s\n",
			  sctk_print_debug_infos(),
			  fmt);
  }
  sctk_noalloc_vfprintf (stderr, buff, ap);
  va_end (ap);

}

void
sctk_formated_dbg_print_abort (FILE * stream, const int line,
			       const char *file, const char *func,
			       const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  if (func == NULL)
    sctk_snprintf (buff, SMALL_BUFFER_SIZE, "Debug at line %d file %s: %s\n",
		   line, file, fmt);
  else
    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
		   "Debug at line %d file %s func %s: %s\n", line, file, func,
		   fmt);
  va_start (ap, fmt);
  sctk_noalloc_vfprintf (stream, buff, ap);
  va_end (ap);
  sctk_abort ();
}

/**********************************************************************/
/*Version                                                             */
/**********************************************************************/
void
sctk_flush_version ()
{
  if (sctk_debug_version_details && (sctk_process_rank == 0))
    {
      sctk_noalloc_fprintf (stderr, sctk_version_buff);
      sctk_version_buff[0] = '\0';
    }
}

void
sctk_print_version (char *lib, int major, int minor)
{
  size_t msg_len;

  msg_len = strlen (sctk_version_buff);
  sctk_snprintf (&(sctk_version_buff[strlen (sctk_version_buff)]),
		 ((WRITE_BUFFER_SIZE) - msg_len),
		 "SCTK %s version %d.%d (global %d.%d)\n", lib, major, minor,
		 SCTK_VERSION_MAJOR, SCTK_VERSION_MINOR);
}

void
sctk_set_version_details ()
{
  sctk_debug_version_details = 1;
}

void
sctk_unset_version_details ()
{
  sctk_debug_version_details = 0;
}

/**********************************************************************/
/*Sizes                                                               */
/**********************************************************************/
void
sctk_size_checking (size_t a, size_t b, char *ca, char *cb, char *file,
		    int line)
{
  if (!(a <= b))
    {
      sctk_noalloc_fprintf (stderr,
			    "Internal error !(%s <= %s) at line %d in %s\n",
			    ca, cb, line, file);
      abort ();
    }
}
void
sctk_size_checking_eq (size_t a, size_t b, char *ca, char *cb, char *file,
		       int line)
{
  if (a != b)
    {
      sctk_noalloc_fprintf (stderr,
			    "Internal error %s != %s at line %d in %s\n",
			    ca, cb, line, file);
      abort ();
    }
}

/**********************************************************************/
/*Other                                                               */
/**********************************************************************/

void
sctk_init (void)
{
  sctk_have_shell_colors = sctk_runtime_config_get()->modules.debugger.colors;
}
void
sctk_leave (void)
{
}
