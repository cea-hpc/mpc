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
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                  # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_spinlock.h"
#include "sctk.h"
#include "sctk_shell_colors.h"



#ifdef MPC_Debugger
#include <sctk_debugger.h>
#endif

#define WRITE_BUFFER_SIZE (4*1024*1024)
#define SMALL_BUFFER_SIZE (4*1024)
#define HAVE_SHELL_COLORS 1

int sctk_only_once_while_val;
static int sctk_debug_version_details = 0;
static char sctk_version_buff[WRITE_BUFFER_SIZE];
static char ret[SMALL_BUFFER_SIZE];

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

static char*
sctk_print_debug_infos()
{
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

#if HAVE_SHELL_COLORS == 1
	sprintf(ret,
    SCTK_COLOR_GREEN([%4d/%4d/%4d/)
    SCTK_COLOR_GREEN_BOLD(%4d)
    SCTK_COLOR_GREEN(/%4d/%4d]),
		 sctk_node_rank,
		 sctk_process_rank, sctk_thread_get_vp (), task_id, thread_id, sctk_local_process_rank);
#else
	sprintf(ret,
    "[%4d/%4d/%4d/%4d/%4d/%4d]",
		 sctk_node_rank,
		 sctk_process_rank, sctk_thread_get_vp (), task_id, thread_id, sctk_local_process_rank);

#endif

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
void
sctk_debug_print_backtrace (const char *format, ...)
{
#ifdef MPC_Debugger
  va_list ap;
  va_start (ap, format);
  sctk_vprint_backtrace (format, ap);
  va_end (ap);
#endif
#ifdef MPC_Profiler
  sctk_profiling_result ();
#endif
}

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
void
sctk_abort (void)
{
  sctk_debug_print_backtrace ("Abort\n");
#ifdef MPC_Message_Passing
  sctk_net_abort ();
#endif
  fflush (stderr);
  abort ();
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
  sctk_printf (const char *fmt, ...)
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
  #if HAVE_SHELL_COLORS == 1
    char info_message[SMALL_BUFFER_SIZE];
    sctk_snprintf (info_message, SMALL_BUFFER_SIZE, SCTK_COLOR_GRAY_BOLD("%s"), fmt);

    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                  "%s INFO %s\n",
      sctk_print_debug_infos(),
                  info_message);
  #else
    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                  "%s INFO %s\n",
      sctk_print_debug_infos(),
                  fmt);
  #endif
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
  #if HAVE_SHELL_COLORS == 1
    char debug_message[SMALL_BUFFER_SIZE];
    sctk_snprintf (debug_message, SMALL_BUFFER_SIZE, SCTK_COLOR_CYAN_BOLD("%s"), fmt);

    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                  "%s DEBUG %s\n",
      sctk_print_debug_infos(),
                  debug_message);
  #else
    sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                  "%s DEBUG %s\n",
      sctk_print_debug_infos(),
                  fmt);
  #endif
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
#if HAVE_SHELL_COLORS == 1
  char error_message[SMALL_BUFFER_SIZE];
  sctk_snprintf (error_message, SMALL_BUFFER_SIZE, SCTK_COLOR_RED_BOLD("%s"), fmt);

  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                 "%s ERROR %s\n",
     sctk_print_debug_infos(),
                 error_message);
#else
  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                 "%s ERROR %s\n",
     sctk_print_debug_infos(),
                 fmt);
#endif
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
#if defined(MPC_Message_Passing) || defined(MPC_Threads)
  if( sctk_get_verbosity() < 1 )
    return;
#endif

  va_list ap;
  char buff[SMALL_BUFFER_SIZE];
  int task_id;
  int thread_id;

  sctk_get_thread_info (&task_id, &thread_id);

  va_start (ap, fmt);
#if HAVE_SHELL_COLORS == 1
  char warning_message[SMALL_BUFFER_SIZE];
  sctk_snprintf (warning_message, SMALL_BUFFER_SIZE, SCTK_COLOR_YELLOW_BOLD("%s"), fmt);

  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
		 "%s WARNING %s\n",
     sctk_print_debug_infos(),
		 warning_message);
#else
  sctk_snprintf (buff, SMALL_BUFFER_SIZE,
                 "%s WARNING %s\n",
     sctk_print_debug_infos(),
                 fmt);
#endif
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
}
void
sctk_leave (void)
{
}
