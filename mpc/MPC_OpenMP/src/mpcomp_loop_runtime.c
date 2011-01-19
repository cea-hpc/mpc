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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpcomp.h>
#include "mpcomp_internal.h"
#include <sctk_debug.h>

/* TODO need to handle chunk size for runtime schedule ('modifier' field) */

int
__mpcomp_runtime_loop_begin (int lb, int b, int incr, int chunk_size,
			     int *from, int *to)
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;
  int ret ;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      ret = __mpcomp_static_loop_begin (lb, b, incr, chunk_size, from, to);
      break ;
    case omp_sched_dynamic:
      ret = __mpcomp_dynamic_loop_begin (lb, b, incr, chunk_size, from, to);
      break ;
    case omp_sched_guided:
      ret = __mpcomp_guided_loop_begin (lb, b, incr, chunk_size, from, to);
      break ;
    case omp_sched_auto:
      ret = __mpcomp_static_loop_begin (lb, b, incr, chunk_size, from, to);
      break ;
    default:
      not_reachable() ;
  }
  return ret ;
}

int
__mpcomp_runtime_loop_next (int *from, int *to)
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;
  int ret ;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      ret = __mpcomp_static_loop_next (from, to);
      break ;
    case omp_sched_dynamic:
      ret = __mpcomp_dynamic_loop_next (from, to);
      break ;
    case omp_sched_guided:
      ret = __mpcomp_guided_loop_next (from, to);
      break ;
    case omp_sched_auto:
      ret = __mpcomp_static_loop_next (from, to);
      break ;
    default:
      not_reachable() ;
  }
  return ret ;
}

void
__mpcomp_runtime_loop_end ()
{
  /* TODO call the correpsonding method instead of barrier (specialized
     barriers inside) */
  /* FIXME use good barrier instead of OLD! */
  __mpcomp_old_barrier ();
}

void
__mpcomp_runtime_loop_end_nowait ()
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      __mpcomp_static_loop_end_nowait ();
      break ;
    case omp_sched_dynamic:
      __mpcomp_dynamic_loop_end_nowait ();
      break ;
    case omp_sched_guided:
      __mpcomp_guided_loop_end_nowait ();
      break ;
    case omp_sched_auto:
      __mpcomp_static_loop_end_nowait ();
      break ;
    default:
      not_reachable() ;
  }
  return ;
}


void
__mpcomp_start_parallel_runtime_loop (int arg_num_threads, void *(*func)
				      (void *), void *shared, int lb, int b,
				      int incr, int chunk_size)
{

  mpcomp_thread_info_t *info;
  omp_sched_t sched;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      __mpcomp_start_parallel_static_loop( arg_num_threads, func, shared, lb, b, incr, chunk_size ) ;
      break ;
    case omp_sched_dynamic:
      __mpcomp_start_parallel_dynamic_loop( arg_num_threads, func, shared, lb, b, incr, chunk_size ) ;
      break ;
    case omp_sched_guided:
      __mpcomp_start_parallel_guided_loop( arg_num_threads, func, shared, lb, b, incr, chunk_size ) ;
      break ;
    case omp_sched_auto:
      __mpcomp_start_parallel_static_loop( arg_num_threads, func, shared, lb, b, incr, chunk_size ) ;
      break ;
    default:
      not_reachable() ;
  }

}

int
__mpcomp_runtime_loop_next_ignore_nowait (int *from, int *to)
{
  not_implemented ();
  return 0;
}


/****
  *
  * ORDERED VERSION 
  *
  *
  *****/

int
__mpcomp_ordered_runtime_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to)
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;
  int ret ;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      ret = __mpcomp_ordered_static_loop_begin( lb, b, incr, chunk_size, from, to ) ;
      break ;
    case omp_sched_dynamic:
      ret = __mpcomp_ordered_dynamic_loop_begin( lb, b, incr, chunk_size, from, to ) ;
      break ;
    case omp_sched_guided:
      ret = __mpcomp_ordered_guided_loop_begin( lb, b, incr, chunk_size, from, to ) ;
      break ;
    case omp_sched_auto:
      ret = __mpcomp_ordered_static_loop_begin( lb, b, incr, chunk_size, from, to ) ;
      break ;
    default:
      not_reachable() ;
  }

  return ret ;
}

int
__mpcomp_ordered_runtime_loop_next(int *from, int *to)
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;
  int ret ;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      ret = __mpcomp_ordered_static_loop_next( from, to ) ;
      break ;
    case omp_sched_dynamic:
      ret = __mpcomp_ordered_dynamic_loop_next( from, to ) ;
      break ;
    case omp_sched_guided:
      ret = __mpcomp_ordered_guided_loop_next( from, to ) ;
      break ;
    case omp_sched_auto:
      ret = __mpcomp_ordered_static_loop_next( from, to ) ;
      break ;
    default:
      not_reachable() ;
  }

  return ret ;
}

void
__mpcomp_ordered_runtime_loop_end()
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      __mpcomp_ordered_static_loop_end() ;
      break ;
    case omp_sched_dynamic:
      __mpcomp_ordered_dynamic_loop_end() ;
      break ;
    case omp_sched_guided:
      __mpcomp_ordered_guided_loop_end() ;
      break ;
    case omp_sched_auto:
      __mpcomp_ordered_static_loop_end() ;
      break ;
    default:
      not_reachable() ;
  }
}

void
__mpcomp_ordered_runtime_loop_end_nowait()
{
  mpcomp_thread_info_t *info;
  omp_sched_t sched;

  info =
    (mpcomp_thread_info_t *) sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  /* Runtime schedule */
  sched = info->icvs.run_sched_var;

  /* Run the corresponding function */
  switch( sched ) {
    case omp_sched_static:
      __mpcomp_ordered_static_loop_end_nowait() ;
      break ;
    case omp_sched_dynamic:
      __mpcomp_ordered_dynamic_loop_end_nowait() ;
      break ;
    case omp_sched_guided:
      __mpcomp_ordered_guided_loop_end_nowait() ;
      break ;
    case omp_sched_auto:
      __mpcomp_ordered_static_loop_end_nowait() ;
      break ;
    default:
      not_reachable() ;
  }
}
