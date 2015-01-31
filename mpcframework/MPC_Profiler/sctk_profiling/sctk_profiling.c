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
/* #                                                                      # */
/* ######################################################################## */
#define ___SCTK___PROFILING___C___
#include "sctk_profiling.h"
#include "sctk_asm.h"
#include <unistd.h>

#ifdef MPC_Threads
#include "sctk_pthread_compatible_structures.h"
#include "sctk_thread_api.h"
#include "sctk_thread.h"
#include "sctk_topology.h"

#else
#include <pthread.h>
typedef pthread_key_t sctk_thread_key_t;
#define sctk_thread_key_create pthread_key_create
#define sctk_thread_getspecific pthread_getspecific
#define sctk_thread_setspecific pthread_setspecific
#define sctk_thread_self pthread_self
#define sctk_thread_self_check pthread_self
#define sctk_thread_get_vp() (-1)
#endif

#include "sctk_debug.h"
#include "sctk_alloc.h"
#include <sched.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "sctk_trace.h"
#include "sctk_spinlock.h"
#include "sctk.h"

#if defined(SCTK_USE_TLS)
typedef struct
{
  void *th_id;
  int vp;
  sctk_trace_t *cur_trace;
} sctk_tls_trace_t;

__thread void *sctk_tls_trace = NULL;
#endif

#ifdef MPC_USE_ZLIB
#include <zlib.h>
static int compression = 1;
static int compress_level = Z_BEST_SPEED;
unsigned long tmp_buffer_compress_size;
#else
static int compression = 0;
#endif

static sctk_profiling_t global_profiling[SCTK_MAX_PROFILING_FUNC_NUMBER];
static sctk_thread_key_t profiling_key;

#if !defined(SCTK_USE_TLS)
static sctk_thread_key_t trace_key;
#else
static sctk_thread_key_t trace_key_free;
#endif

static double start_time;
static double end_time;

static volatile int sctk_trace_enable = 0;
static volatile int sctk_profiling_enable = 0;
static int trace_fd;

static sctk_spinlock_t sctk_profiling_spinlock = 0;

double
sctk_profiling_get_timestamp ()
{
  return sctk_get_time_stamp ();
}

void
sctk_profiling_init_keys ()
{
  /************************************************************************/
  /*PROFILING MPC                                                         */
  /************************************************************************/
  SCTK_PROF_INIT_KEY (MPC_Abort);
  SCTK_PROF_INIT_KEY (MPC_Add_pack);
  SCTK_PROF_INIT_KEY (MPC_Add_pack_absolute);
  SCTK_PROF_INIT_KEY (MPC_Add_pack_default);
  SCTK_PROF_INIT_KEY (MPC_Allgather);
  SCTK_PROF_INIT_KEY (MPC_Allgatherv);
  SCTK_PROF_INIT_KEY (MPC_Allreduce);
  SCTK_PROF_INIT_KEY (MPC_Alltoall);
  SCTK_PROF_INIT_KEY (MPC_Alltoallv);
  SCTK_PROF_INIT_KEY (MPC_Barrier);
  SCTK_PROF_INIT_KEY (MPC_Bcast);
  SCTK_PROF_INIT_KEY (MPC_Bsend);
  SCTK_PROF_INIT_KEY (MPC_Comm_create);
  SCTK_PROF_INIT_KEY (MPC_Comm_create_list);
  SCTK_PROF_INIT_KEY (MPC_Comm_dup);
  SCTK_PROF_INIT_KEY (MPC_Comm_free);
  SCTK_PROF_INIT_KEY (MPC_Comm_group);
  SCTK_PROF_INIT_KEY (MPC_Comm_rank);
  SCTK_PROF_INIT_KEY (MPC_Comm_size);
  SCTK_PROF_INIT_KEY (MPC_Comm_split);
  SCTK_PROF_INIT_KEY (MPC_Default_pack);
  SCTK_PROF_INIT_KEY (MPC_Derived_datatype);
  SCTK_PROF_INIT_KEY (MPC_Errhandler_create);
  SCTK_PROF_INIT_KEY (MPC_Errhandler_free);
  SCTK_PROF_INIT_KEY (MPC_Errhandler_get);
  SCTK_PROF_INIT_KEY (MPC_Errhandler_set);
  SCTK_PROF_INIT_KEY (MPC_Error_class);
  SCTK_PROF_INIT_KEY (MPC_Error_string);
  SCTK_PROF_INIT_KEY (MPC_Finalize);
  SCTK_PROF_INIT_KEY (MPC_Gather);
  SCTK_PROF_INIT_KEY (MPC_Gatherv);
  SCTK_PROF_INIT_KEY (MPC_Get_count);
  SCTK_PROF_INIT_KEY (MPC_Get_processor_name);
  SCTK_PROF_INIT_KEY (MPC_Group_difference);
  SCTK_PROF_INIT_KEY (MPC_Group_free);
  SCTK_PROF_INIT_KEY (MPC_Group_incl);
  SCTK_PROF_INIT_KEY (MPC_Ibsend);
  SCTK_PROF_INIT_KEY (MPC_Init);
  SCTK_PROF_INIT_KEY (MPC_Initialized);
  SCTK_PROF_INIT_KEY (MPC_Iprobe);
  SCTK_PROF_INIT_KEY (MPC_Irecv);
  SCTK_PROF_INIT_KEY (MPC_Irecv_pack);
  SCTK_PROF_INIT_KEY (MPC_Irsend);
  SCTK_PROF_INIT_KEY (MPC_Isend);
  SCTK_PROF_INIT_KEY (MPC_Isend_pack);
  SCTK_PROF_INIT_KEY (MPC_Issend);
  SCTK_PROF_INIT_KEY (MPC_Node_number);
  SCTK_PROF_INIT_KEY (MPC_Node_rank);
  SCTK_PROF_INIT_KEY (MPC_Op_create);
  SCTK_PROF_INIT_KEY (MPC_Op_free);
  SCTK_PROF_INIT_KEY (MPC_Open_pack);
  SCTK_PROF_INIT_KEY (MPC_Probe);
  SCTK_PROF_INIT_KEY (MPC_Proceed);
  SCTK_PROF_INIT_KEY (MPC_Process_number);
  SCTK_PROF_INIT_KEY (MPC_Process_rank);
  SCTK_PROF_INIT_KEY (MPC_Processor_number);
  SCTK_PROF_INIT_KEY (MPC_Processor_rank);
  SCTK_PROF_INIT_KEY (MPC_Recv);
  SCTK_PROF_INIT_KEY (MPC_Recv_init_message);
  SCTK_PROF_INIT_KEY (sctk_mpc_wait_message);
  SCTK_PROF_INIT_KEY (MPC_Reduce);
  SCTK_PROF_INIT_KEY (MPC_Rsend);
  SCTK_PROF_INIT_KEY (MPC_Scatter);
  SCTK_PROF_INIT_KEY (MPC_Scatterv);
  SCTK_PROF_INIT_KEY (MPC_Send);
  SCTK_PROF_INIT_KEY (MPC_Sendrecv);
  SCTK_PROF_INIT_KEY (MPC_Type_hcontiguous);
  SCTK_PROF_INIT_KEY (MPC_Ssend);
  SCTK_PROF_INIT_KEY (MPC_Test);
  SCTK_PROF_INIT_KEY (MPC_Test_no_check);
  SCTK_PROF_INIT_KEY (MPC_Test_check);
  SCTK_PROF_INIT_KEY (MPC_Type_free);
  SCTK_PROF_INIT_KEY (MPC_Type_size);
  SCTK_PROF_INIT_KEY (MPC_Wait);
  SCTK_PROF_INIT_KEY (MPC_Wait_pending);
  SCTK_PROF_INIT_KEY (MPC_Wait_pending_all_comm);
  SCTK_PROF_INIT_KEY (MPC_Waitall);
  SCTK_PROF_INIT_KEY (MPC_Waitany);
  SCTK_PROF_INIT_KEY (MPC_Waitsome);
  SCTK_PROF_INIT_KEY (MPC_Wtime);
  SCTK_PROF_INIT_KEY (malloc);
  SCTK_PROF_INIT_KEY (calloc);
  SCTK_PROF_INIT_KEY (realloc);
  SCTK_PROF_INIT_KEY (free);
  SCTK_PROF_INIT_KEY (sctk_malloc);
  SCTK_PROF_INIT_KEY (sctk_calloc);
  SCTK_PROF_INIT_KEY (sctk_realloc);
  SCTK_PROF_INIT_KEY (sctk_free);
  SCTK_PROF_INIT_KEY (sctk_sbrk);
  SCTK_PROF_INIT_KEY (sctk_sbrk_try_to_find_page);
  SCTK_PROF_INIT_KEY (sctk_sbrk_mmap);
  SCTK_PROF_INIT_KEY (sctk_thread_mutex_lock);
  SCTK_PROF_INIT_KEY (sctk_thread_mutex_unlock);
  SCTK_PROF_INIT_KEY (sctk_thread_mutex_spinlock);
  SCTK_PROF_INIT_KEY (sctk_free_distant);
  SCTK_PROF_INIT_KEY (sctk_thread_wait_for_value_and_poll);
  SCTK_PROF_INIT_KEY (sctk_thread_wait_for_value);
  SCTK_PROF_INIT_KEY (sctk_mmap);
  SCTK_PROF_INIT_KEY (sctk_alloc_ptr_small_serach_chunk);
  SCTK_PROF_INIT_KEY (sctk_alloc_ptr_big);
  SCTK_PROF_INIT_KEY (sctk_alloc_ptr_small);
  SCTK_PROF_INIT_KEY (sctk_free_small);
  SCTK_PROF_INIT_KEY (sctk_free_big);
  SCTK_PROF_INIT_KEY (sctk_elan_barrier);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver);
  SCTK_PROF_INIT_KEY (sctk_net_get_new_header);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver_inf);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver_sup);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver_sup_write);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver_split_header);
  SCTK_PROF_INIT_KEY (sctk_net_send_ptp_message_driver_send_header);

  /* MPC_MicroThreads */
  SCTK_PROF_INIT_KEY (__sctk_microthread_parallel_exec__last_barrier) ;

  /* MPC_OpenMP */
  SCTK_PROF_INIT_KEY (__mpcomp_start_parallel_region);
  SCTK_PROF_INIT_KEY (__mpcomp_start_parallel_region__creation);
  SCTK_PROF_INIT_KEY (__mpcomp_dynamic_loop_begin);
  SCTK_PROF_INIT_KEY (__mpcomp_dynamic_loop_next);
}

static void
sctk_profiling_init_intern ()
{
  if (sctk_profiling_enable == 0)
    {
      sctk_thread_key_create (&profiling_key, sctk_free);
      sctk_trace_init ();
      sctk_profiling_enable = 1;
      start_time = sctk_profiling_get_timestamp ();
    }
}

void
sctk_profiling_key_create (sctk_profiling_key_t * key, char *name)
{
  static sctk_profiling_key_t cur_key = 1;
  sctk_spinlock_lock (&sctk_profiling_spinlock);
  assume (cur_key < SCTK_MAX_PROFILING_FUNC_NUMBER);

  sctk_profiling_init_intern ();

  *key = cur_key;
  global_profiling[cur_key].func_name = name;
  sctk_nodebug ("Register %d %s", cur_key, name);
  global_profiling[cur_key].nb_call = 0;
  global_profiling[cur_key].elapsed_time = 0;
  cur_key++;
  sctk_spinlock_unlock (&sctk_profiling_spinlock);
}

void
sctk_profiling_set_usage (sctk_profiling_key_t key, double usage)
{
  if ((sctk_profiling_enable == 1) && (key > 0))
    {
      sctk_profiling_t *cur_prof;
      sctk_nodebug ("Key %d", key);
      cur_prof = (sctk_profiling_t *) sctk_thread_getspecific (profiling_key);
      if (cur_prof != NULL)
	{
	  cur_prof[key].nb_call++;
	  cur_prof[key].elapsed_time += usage;
	}
    }
}

static double elapse_time;
#ifndef SCTK_DO_NOT_USE_MPC_PROFILING
static int
sctk_profiling_compare (sctk_profiling_t * a, sctk_profiling_t * b)
{
  if ((100 - ((elapse_time - a->elapsed_time) * 100.0) / elapse_time) <
      (100 - ((elapse_time - b->elapsed_time) * 100.0) / elapse_time))
    {
      return 1;
    }
  else
    {
      return -1;
    }
}
#endif

void
sctk_profiling_result ()
{
  sctk_profiling_key_t key;
  sctk_profiling_enable = 0;
  if (sctk_trace_enable == 1)
    {
      sctk_trace_enable = 2;
    }
  end_time = sctk_profiling_get_timestamp ();
  elapse_time = end_time - start_time;
  sctk_noalloc_fprintf (stderr,
			"MPC Profiling: (elapsed time %30.0f cycles)\n",
			elapse_time);
  qsort (global_profiling, SCTK_MAX_PROFILING_FUNC_NUMBER,
	 sizeof (sctk_profiling_t),
	 (int (*)(const void *, const void *)) sctk_profiling_compare);

  for (key = 0; key < SCTK_MAX_PROFILING_FUNC_NUMBER; key++)
    {
      if (global_profiling[key].func_name != NULL
	  && global_profiling[key].nb_call > 0)
	{
	  sctk_noalloc_fprintf (stderr,
				"\t- %-80s %10lu calls %30.0f cycles elapsed %30.3f cycles/call (%10.3f%%)\n",
				global_profiling[key].func_name,
				global_profiling[key].nb_call,
				global_profiling[key].elapsed_time,
				global_profiling[key].elapsed_time /
				global_profiling[key].nb_call,
				(100 -
				 ((elapse_time -
				   global_profiling[key].elapsed_time) *
				  100.0) /
				 elapse_time) /*/sctk_get_cpu_number () */ );
	}
    }
  sctk_trace_end ();
}

void
sctk_profiling_init ()
{
  sctk_profiling_t *cur_prof;
  sctk_trace_t *cur_trace;

  sctk_spinlock_lock (&sctk_profiling_spinlock);
  sctk_profiling_init_intern ();
  sctk_spinlock_unlock (&sctk_profiling_spinlock);

  cur_prof =
    (sctk_profiling_t *) sctk_calloc (SCTK_MAX_PROFILING_FUNC_NUMBER,
				      sizeof (sctk_profiling_t));
  sctk_thread_setspecific (profiling_key, cur_prof);

  cur_trace = (sctk_trace_t *) sctk_calloc (1, sizeof (sctk_trace_t));
#ifdef MPC_USE_ZLIB
  cur_trace->tmp_buffer_compress = sctk_malloc (tmp_buffer_compress_size);
#else
  cur_trace->tmp_buffer_compress = NULL;
#endif
#if defined(SCTK_USE_TLS)
  sctk_tls_trace = sctk_malloc (sizeof (sctk_tls_trace_t));
  ((sctk_tls_trace_t *) sctk_tls_trace)->th_id = sctk_thread_self_check ();
  ((sctk_tls_trace_t *) sctk_tls_trace)->vp = sctk_thread_get_vp ();
  ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace = cur_trace;
  sctk_thread_setspecific (trace_key_free, sctk_tls_trace);
#else
  sctk_thread_setspecific (trace_key, cur_trace);
#endif
}

void
sctk_profiling_commit ()
{
  sctk_profiling_t *cur_prof;
  sctk_spinlock_lock (&sctk_profiling_spinlock);
  cur_prof = (sctk_profiling_t *) sctk_thread_getspecific (profiling_key);
  if (cur_prof != NULL)
    {
      sctk_profiling_key_t key;
      for (key = 0; key < SCTK_MAX_PROFILING_FUNC_NUMBER; key++)
	{
	  if (global_profiling[key].func_name != NULL
	      && cur_prof[key].nb_call > 0)
	    {
	      global_profiling[key].nb_call += cur_prof[key].nb_call;
	      global_profiling[key].elapsed_time +=
		cur_prof[key].elapsed_time;
	    }
	}
      sctk_free (cur_prof);
      sctk_thread_setspecific (profiling_key, NULL);
    }
  sctk_spinlock_unlock (&sctk_profiling_spinlock);
  sctk_trace_commit ();
}


void
mpc_profiling_key_create (mpc_profiling_key_t * key, char *name)
{
  sctk_profiling_key_create (key, name);
}

void
mpc_profiling_set_usage (mpc_profiling_key_t key, double usage)
{
  sctk_profiling_set_usage (key, usage);
}

double
mpc_profiling_get_timestamp (void)
{
  return sctk_profiling_get_timestamp ();
}

void
mpc_profiling_result (void)
{
#if !defined(MPC_Threads)
  sctk_profiling_result ();
#endif
}

void
mpc_profiling_commit (void)
{
#if !defined(MPC_Threads)
  sctk_profiling_commit ();
#endif
}

void
mpc_profiling_init (void)
{
#if !defined(MPC_Threads)
  sctk_profiling_init ();
#endif
}

static sctk_trace_t *
sctk_trace_disable_thread ()
{
  sctk_trace_t *tmp;
#if defined(SCTK_USE_TLS)
  tmp = ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace;
  ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace = NULL;
#else
  tmp = (sctk_trace_t *) sctk_thread_getspecific (trace_key);
  sctk_thread_setspecific (trace_key, NULL);
#endif
  return tmp;
}

static void
sctk_trace_enable_thread (sctk_trace_t * cur_trace)
{
#if defined(SCTK_USE_TLS)
  ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace = cur_trace;
#else
  sctk_thread_setspecific (trace_key, cur_trace);
#endif
}

void
sctk_trace_commit ()
{
  int i;
  if (sctk_trace_enable >= 1)
    {
      sctk_trace_t *cur_trace;
#if !defined(SCTK_USE_TLS)
      cur_trace = (sctk_trace_t *) sctk_thread_getspecific (trace_key);
#else
      cur_trace = NULL;
      if (sctk_tls_trace != NULL)
	{
	  cur_trace = ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace;
	}
#endif
      if (cur_trace != NULL)
	{

	  for (i = cur_trace->step; i < NB_ENTRIES; i++)
	    {
	      cur_trace->buf[i].date = 0;
	      cur_trace->buf[i].function = NULL;
	      cur_trace->buf[i].arg1 = NULL;
	      cur_trace->buf[i].arg2 = NULL;
	      cur_trace->buf[i].arg3 = NULL;
	      cur_trace->buf[i].arg4 = NULL;
	      cur_trace->buf[i].th = NULL;
	      cur_trace->buf[i].vp = 0;
	    }



#ifdef MPC_USE_ZLIB
	  if (compression)
	    {
	      uLongf dest_size;
	      char *tmp_b;
	      sctk_trace_t *cur_trace_save;
	      char *tmp_buffer_compress;
	      tmp_buffer_compress = cur_trace->tmp_buffer_compress;
	      cur_trace_save = sctk_trace_disable_thread ();
	      dest_size = tmp_buffer_compress_size - sizeof (uLongf);
	      tmp_b = tmp_buffer_compress + sizeof (uLongf);
	      sctk_nodebug ("Compress %d %d", dest_size,
			    sizeof (sctk_trace_t));

	      if (compress2 ((Bytef *) (tmp_b),
			     &(dest_size),
			     (Bytef *) cur_trace,
			     sizeof (sctk_trace_t), compress_level) != Z_OK)
		{
		  sctk_nodebug ("Compress fail");
		  sctk_noalloc_fprintf (stderr,
					"Fail to compress tracefile\n");
		  abort ();
		}

	      sctk_nodebug ("Compress done %d", dest_size);
	      (((uLongf *) tmp_buffer_compress)[0]) = dest_size;
	      sctk_spinlock_lock (&sctk_profiling_spinlock);
	      write (trace_fd, tmp_buffer_compress,
		     sizeof (uLongf) + dest_size);
	      sctk_trace_enable_thread (cur_trace_save);
	    }
	  else
	    {
#endif
	      sctk_spinlock_lock (&sctk_profiling_spinlock);
	      write (trace_fd, cur_trace, sizeof (sctk_trace_t));
#ifdef MPC_USE_ZLIB
	    }
#endif
	  sctk_spinlock_unlock (&sctk_profiling_spinlock);
	  cur_trace->step = 0;
	}
    }
}

static void
trace_key_free_func_cur_trace (void *arg)
{
  sctk_free (((sctk_trace_t *) arg)->tmp_buffer_compress);
  sctk_free (((sctk_trace_t *) arg));
}

#if defined(SCTK_USE_TLS)
static void
trace_key_free_func (void *arg)
{
  trace_key_free_func_cur_trace (((sctk_tls_trace_t *) arg)->cur_trace);
  sctk_free (arg);
  sctk_tls_trace = NULL;
}
#endif

static char *
sctk_trace_get_name ()
{
  static char *pathname = NULL;
  static char name[4096];

  if (sctk_trace_enable == 0)
    {
      pathname = getenv ("MPC_TRACE_FILE");
      if (pathname != NULL)
	{
	  mkdir (pathname, 0777);
	  sprintf (name, "%s/trace", pathname);
	}
      else
	{
	  return NULL;
	}
    }
  return name;
}

void
sctk_trace_init ()
{
  char *pathname = NULL;
#if !defined(SCTK_USE_TLS)
  sctk_thread_key_create (&trace_key, trace_key_free_func_cur_trace);
#else
  sctk_thread_key_create (&trace_key_free, trace_key_free_func);
#endif
#ifdef MPC_USE_ZLIB
  tmp_buffer_compress_size = (2 * sizeof (sctk_trace_t)) + 1024;
#endif
  if (sctk_trace_enable == 0)
    {
      pathname = sctk_trace_get_name ();
      if (pathname != NULL)
	{
	  char tmp[4096];
	  sprintf (tmp, "%s_%d", pathname, sctk_process_rank);
	  trace_fd = open (tmp, O_WRONLY | O_CREAT | O_TRUNC,
			   S_IRUSR | S_IWUSR);
	  if (trace_fd < 0)
	    {
	      sctk_noalloc_fprintf (stderr, "Fail to open %s\n", tmp);
	      perror ("Open fail");
	      abort ();
	    }
	  sprintf (tmp, "%s_header_%d", pathname, sctk_process_rank);
	  remove (tmp);
	  sctk_trace_enable = 1;
	}
    }
}

static inline int
sctk_trace_read_header (int fd_in, int fd, char *prototype, int *max_i,
			int *max_name)
{
  static sctk_trace_t cur_trace;
  static char sctk_trace_read_buffer[4 * 1024 * 1024];
  static char *tmp_buffer_compress = NULL;
  int i;
  int res;
#ifdef MPC_USE_ZLIB
  if (compression)
    {
      uLong source_len;
      uLongf dest_len;

      if (tmp_buffer_compress == NULL)
	{
	  tmp_buffer_compress = sctk_malloc (tmp_buffer_compress_size);
	}

      res = read (fd_in, &source_len, sizeof (uLong));
      if (0 == res)
	{
	  return res;
	}
      if (res != sizeof (uLong))
	{
	  sctk_noalloc_fprintf (stderr, "Fail to read tracefile\n");
	  abort ();
	}
      res = read (fd_in, tmp_buffer_compress, source_len);
      if ((uLong)res != source_len)
	{
	  sctk_noalloc_fprintf (stderr, "Fail to read tracefile\n");
	  abort ();
	}
      dest_len = sizeof (sctk_trace_t);
      res =
	uncompress ((Bytef *) (&cur_trace), &dest_len,
		    (Bytef *) tmp_buffer_compress, source_len);
      if ((res != Z_OK) || (dest_len != sizeof (sctk_trace_t)))
	{
	  sctk_noalloc_fprintf (stderr,
				"Fail to uncompress tracefile %d!=%d %d\n",
				dest_len, sizeof (sctk_trace_t), res);
	  abort ();
	}
      res = sizeof (sctk_trace_t);
    }
  else
    {
      res = read (fd_in, &cur_trace, sizeof (sctk_trace_t));
    }
#else
  res = read (fd_in, &cur_trace, sizeof (sctk_trace_t));
#endif
  if (0 == res)
    {
      return res;
    }
  else
    {
      if (res != sizeof (sctk_trace_t))
	{
	  sctk_noalloc_fprintf (stderr, "Fail to read tracefile\n");
	  abort ();
	}
    }
  for (i = 0; i < NB_ENTRIES; i++)
    {
      if (cur_trace.buf[i].function != NULL)
	{
	  if (*(cur_trace.buf[i].function))
	    {
	      sprintf (sctk_trace_read_buffer,
		       prototype,
		       (*max_i),
		       cur_trace.buf[i].function,
		       strlen (*(cur_trace.buf[i].function)),
		       *(cur_trace.buf[i].function));
	      write (fd, sctk_trace_read_buffer,
		     strlen (sctk_trace_read_buffer));
	      if (strlen (*(cur_trace.buf[i].function)) > (size_t)*max_name)
		{
		  *max_name = strlen (*(cur_trace.buf[i].function));
		}
	      *(cur_trace.buf[i].function) = NULL;
	      *max_i = (*max_i) + 1;
	    }
	}
    }
  return res;
}

void
sctk_trace_end ()
{
  char *pathname = NULL;
  char *tracename = NULL;
  int trace_fd_header;
  char tmp[4096];
  char tmp_header[4096];

  sctk_spinlock_lock (&sctk_profiling_spinlock);
  if (sctk_trace_enable >= 1)
    {
      sctk_trace_enable = 0;
      close (trace_fd);
      pathname = sctk_trace_get_name ();
      tracename = pathname;
      sprintf (tmp, "%s_%d", pathname, sctk_process_rank);
      sprintf (tmp_header, "%s_header_%d", pathname, sctk_process_rank);
      trace_fd = open (tmp, O_RDONLY);
      if (trace_fd < 0)
	{
	  sctk_noalloc_fprintf (stderr, "Fail to open %s\n", tmp);
	  perror ("Open fail");
	  abort ();
	}

      trace_fd_header = open (tmp_header, O_WRONLY | O_CREAT | O_TRUNC,
			      S_IRUSR | S_IWUSR);
      if (trace_fd_header < 0)
	{
	  sctk_noalloc_fprintf (stderr, "Fail to open %s\n", tmp);
	  perror ("Open fail");
	  abort ();
	}
      else
	{
	  int max_i = 0;
	  int max_name = 0;
	  char buf[4096];
	  char *prototype;
	  prototype = "%d|%x|%d|%s\n";
	  sctk_noalloc_fprintf (stderr, "Print trace header\n");
	  sprintf (buf, "%s_%d\n", tracename, sctk_process_rank);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", sctk_process_number);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", compression);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", sctk_process_rank);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", (int) sizeof (void *));
	  write (trace_fd_header, buf, strlen (buf));
	  write (trace_fd_header, prototype, strlen (prototype));
	  sprintf (buf, "%20d\n", max_i);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%20d\n", max_name);
	  write (trace_fd_header, buf, strlen (buf));
	  while (sctk_trace_read_header
		 (trace_fd, trace_fd_header, prototype, &max_i,
		  &max_name) > 0);

	  lseek (trace_fd_header, 0, SEEK_SET);
	  sprintf (buf, "%s_%d\n", tracename, sctk_process_rank);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", sctk_process_number);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", compression);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", sctk_process_rank);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%d\n", (int) sizeof (void *));
	  write (trace_fd_header, buf, strlen (buf));
	  write (trace_fd_header, prototype, strlen (prototype));
	  sprintf (buf, "%20d\n", max_i);
	  write (trace_fd_header, buf, strlen (buf));
	  sprintf (buf, "%20d\n", max_name);
	  write (trace_fd_header, buf, strlen (buf));
	  lseek (trace_fd_header, 0, SEEK_END);
	  close (trace_fd_header);
	  sctk_noalloc_fprintf (stderr, "Print trace header done\n");
	}
      close (trace_fd);
    }
  sctk_spinlock_unlock (&sctk_profiling_spinlock);
}

void
sctk_trace_point (char **function, void *arg1, void *arg2, void *arg3,
		  void *arg4)
{
  sctk_trace_t *cur_trace;
#if !defined(SCTK_USE_TLS)
  if (((void *) sctk_thread_self_check () != NULL))
    {
      cur_trace = (sctk_trace_t *) sctk_thread_getspecific (trace_key);
    }
  else
    {
      cur_trace = NULL;
    }
#else
  cur_trace = NULL;
  if (sctk_tls_trace != NULL)
    {
      cur_trace = ((sctk_tls_trace_t *) sctk_tls_trace)->cur_trace;
    }
#endif
  if ((sctk_trace_enable == 1) && (cur_trace != NULL))
    {
      int step;
      step = cur_trace->step;

      cur_trace->buf[step].date = sctk_profiling_get_timestamp ();
      cur_trace->buf[step].function = function;
      cur_trace->buf[step].arg1 = arg1;
      cur_trace->buf[step].arg2 = arg2;
      cur_trace->buf[step].arg3 = arg3;
      cur_trace->buf[step].arg4 = arg4;
#if defined(SCTK_USE_TLS)
      cur_trace->buf[step].th = ((sctk_tls_trace_t *) sctk_tls_trace)->th_id;
      cur_trace->buf[step].vp = ((sctk_tls_trace_t *) sctk_tls_trace)->vp;
#else
      cur_trace->buf[step].th = (void *) sctk_thread_self ();
      cur_trace->buf[step].vp = sctk_thread_get_vp ();
#endif

      step++;
      if (step == NB_ENTRIES)
	{
	  sctk_nodebug ("Dump to file");

#ifdef MPC_USE_ZLIB
	  if (compression)
	    {
	      uLongf dest_size;
	      char *tmp_b;
	      sctk_trace_t *cur_trace_save;
	      char *tmp_buffer_compress;
	      tmp_buffer_compress = cur_trace->tmp_buffer_compress;
	      cur_trace_save = sctk_trace_disable_thread ();
	      dest_size = tmp_buffer_compress_size - sizeof (uLongf);
	      tmp_b = tmp_buffer_compress + sizeof (uLongf);
	      sctk_nodebug ("Compress %d %d", dest_size,
			    sizeof (sctk_trace_t));

	      if (compress2 ((Bytef *) (tmp_b),
			     &(dest_size),
			     (Bytef *) cur_trace,
			     sizeof (sctk_trace_t), compress_level) != Z_OK)
		{
		  sctk_nodebug ("Compress fail");
		  sctk_noalloc_fprintf (stderr,
					"Fail to compress tracefile\n");
		  abort ();
		}

	      sctk_nodebug ("Compress done %d", dest_size);
	      (((uLongf *) tmp_buffer_compress)[0]) = dest_size;
	      sctk_spinlock_lock (&sctk_profiling_spinlock);
	      write (trace_fd, tmp_buffer_compress,
		     sizeof (uLongf) + dest_size);
	      sctk_trace_enable_thread (cur_trace_save);
	    }
	  else
	    {
#endif
	      sctk_spinlock_lock (&sctk_profiling_spinlock);
	      write (trace_fd, cur_trace, sizeof (sctk_trace_t));
#ifdef MPC_USE_ZLIB
	    }
#endif

	  step = 0;
	  sctk_spinlock_unlock (&sctk_profiling_spinlock);
	  sctk_nodebug ("Dump to file done");
	}
      cur_trace->step = step;
    }
}

void
mpc_trace_point (char **function, void *arg1, void *arg2, void *arg3,
		 void *arg4)
{
  sctk_trace_point (function, arg1, arg2, arg3, arg4);
}
