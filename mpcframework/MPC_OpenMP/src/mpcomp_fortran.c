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
#include <sctk_debug.h>

/* FORTRAN MAPPING */

void
omp_barrier_ ()
{
	__mpcomp_barrier();
}

void
omp_barrier__ ()
{
	__mpcomp_barrier();
}

void
omp_flush_ ()
{
	__mpcomp_flush();
}

void
omp_flush__ ()
{
	__mpcomp_flush();
}

/* LIB FUNCTIONS */


void
omp_set_num_threads_ (int *num_threads)
{
  sctk_nodebug ("Fortran: omp_set_num_threads_ %d", *num_threads);
  mpcomp_set_num_threads (*num_threads);
}

void
omp_set_num_threads__ (int *num_threads)
{
  sctk_nodebug ("Fortran: omp_set_num_threads__ %d", *num_threads);
  mpcomp_set_num_threads (*num_threads);
}

int
omp_get_num_threads_ ()
{
  return mpcomp_get_num_threads ();
}

int
omp_get_num_threads__ ()
{
  return mpcomp_get_num_threads ();
}

int
omp_get_max_threads_ ()
{
  return mpcomp_get_max_threads ();
}

int
omp_get_max_threads__ ()
{
  return mpcomp_get_max_threads ();
}

int
omp_get_thread_num_ ()
{
  return mpcomp_get_thread_num ();
}

int
omp_get_thread_num__ ()
{
  return mpcomp_get_thread_num ();
}

int
omp_get_num_procs_ ()
{
  return mpcomp_get_num_procs ();
}

int
omp_get_num_procs__ ()
{
  return mpcomp_get_num_procs ();
}

int
omp_in_parallel_ ()
{
  return mpcomp_in_parallel ();
}

int
omp_in_parallel__ ()
{
  return mpcomp_in_parallel ();
}

int 
omp_get_active_level_ ()
{
	return mpcomp_get_active_level ();
}

int 
omp_get_active_level__ ()
{
	return mpcomp_get_active_level ();
}

int
omp_get_ancestor_thread_num_ (int level)
{
	return mpcomp_get_ancestor_thread_num(level);
}

int
omp_get_ancestor_thread_num__ (int level)
{
	return mpcomp_get_ancestor_thread_num(level);
}

int 
omp_get_team_size_ (int level)
{
	return mpcomp_get_team_size(level);
}

int 
omp_get_team_size__ (int level)
{
	return mpcomp_get_team_size(level);
}

/* DYNAMIC FUNCTIONS */
void
omp_set_dynamic_ (int *dynamic_threads)
{
  mpcomp_set_dynamic (*dynamic_threads);
}

void
omp_set_dynamic__ (int *dynamic_threads)
{
  mpcomp_set_dynamic (*dynamic_threads);
}

int
omp_get_dynamic_ ()
{
  return mpcomp_get_dynamic ();
}

int
omp_get_dynamic__ ()
{
  return mpcomp_get_dynamic ();
}

/* NESTED FUNCTIONS */
void
omp_set_nested_ (int *nested)
{
  mpcomp_set_nested (*nested);
}

void
omp_set_nested__ (int *nested)
{
  mpcomp_set_nested (*nested);
}

int
omp_get_nested_ ()
{
  return mpcomp_get_nested ();
}

int
omp_get_nested__ ()
{
  return mpcomp_get_nested ();
}

/* TIME FUNCTIONS */
double
omp_get_wtime_ ()
{
  return mpcomp_get_wtime ();
}

double
omp_get_wtime__ ()
{
  return mpcomp_get_wtime ();
}

double
omp_get_wtick_ ()
{
  return mpcomp_get_wtick ();
}

double
omp_get_wtick__ ()
{
  return mpcomp_get_wtick ();
}

/* LOCK FUNCTIONS */
void
omp_init_lock_ (omp_lock_t * lock)
{
  mpcomp_init_lock (lock);
}

void
omp_init_lock__ (omp_lock_t * lock)
{
  mpcomp_init_lock (lock);
}

void
omp_destroy_lock_ (omp_lock_t * lock)
{
  mpcomp_destroy_lock (lock);
}

void
omp_destroy_lock__ (omp_lock_t * lock)
{
  mpcomp_destroy_lock (lock);
}

void
omp_set_lock_ (omp_lock_t * lock)
{
  mpcomp_set_lock (lock);
}

void
omp_set_lock__ (omp_lock_t * lock)
{
  mpcomp_set_lock (lock);
}

void
omp_unset_lock_ (omp_lock_t * lock)
{
  mpcomp_unset_lock (lock);
}

void
omp_unset_lock__ (omp_lock_t * lock)
{
  mpcomp_unset_lock (lock);
}

int
omp_test_lock_ (omp_lock_t * lock)
{
  return mpcomp_test_lock( lock ) ;
}

int
omp_test_lock__ (omp_lock_t * lock)
{
  return mpcomp_test_lock( lock ) ;
}

/* NEST-LOCK FUNCTIONS */
void
omp_init_nest_lock_ (omp_nest_lock_t * lock) 
{
  mpcomp_init_nest_lock( lock ) ;
}

void
omp_init_nest_lock__ (omp_nest_lock_t * lock) 
{
  mpcomp_init_nest_lock( lock ) ;
}

void 
omp_destroy_nest_lock_ (omp_nest_lock_t * lock) 
{
  mpcomp_destroy_nest_lock( lock );
}

void 
omp_destroy_nest_lock__ (omp_nest_lock_t * lock) 
{
  mpcomp_destroy_nest_lock( lock );
}

void 
omp_set_nest_lock_ (omp_nest_lock_t * lock) 
{
  mpcomp_set_nest_lock( lock ) ;
}

void 
omp_set_nest_lock__ (omp_nest_lock_t * lock) 
{
  mpcomp_set_nest_lock( lock ) ;
}

void 
omp_unset_nest_lock_ (omp_nest_lock_t * lock) 
{
  mpcomp_unset_nest_lock( lock ) ;
}

void 
omp_unset_nest_lock__ (omp_nest_lock_t * lock) 
{
  mpcomp_unset_nest_lock( lock ) ;
}

int 
omp_test_nest_lock_ (omp_nest_lock_t * lock) 
{
  return mpcomp_test_nest_lock( lock ) ;
}

int 
omp_test_nest_lock__ (omp_nest_lock_t * lock) 
{
  return mpcomp_test_nest_lock( lock ) ;
}
