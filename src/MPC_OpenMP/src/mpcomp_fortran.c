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
#include <mpc_omp.h>
#include <mpc_common_debug.h>
#include "mpcompt_frame.h"

/* FORTRAN MAPPING */

/* LIB FUNCTIONS */

void
omp_set_num_threads_ (int *num_threads)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_common_nodebug ("Fortran: omp_set_num_threads_ %d", *num_threads);
  omp_set_num_threads (*num_threads);
}

void
omp_set_num_threads__ (int *num_threads)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  mpc_common_nodebug ("Fortran: omp_set_num_threads__ %d", *num_threads);
  omp_set_num_threads (*num_threads);
}

int
omp_get_num_threads_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_num_threads ();
}

int
omp_get_num_threads__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_num_threads ();
}

int
omp_get_max_threads_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_max_threads ();
}

int
omp_get_max_threads__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_max_threads ();
}

int
omp_get_thread_num_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_thread_num ();
}

int
omp_get_thread_num__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_thread_num ();
}

int
omp_get_num_procs_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_num_procs ();
}

int
omp_get_num_procs__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_num_procs ();
}

int
omp_in_parallel_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_in_parallel ();
}

int
omp_in_parallel__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_in_parallel ();
}

/* DYNAMIC FUNCTIONS */
void
omp_set_dynamic_ (int *dynamic_threads)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  omp_set_dynamic (*dynamic_threads);
}

void
omp_set_dynamic__ (int *dynamic_threads)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  omp_set_dynamic (*dynamic_threads);
}

int
omp_get_dynamic_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_dynamic ();
}

int
omp_get_dynamic__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_dynamic ();
}

/* NESTED FUNCTIONS */
void
omp_set_nested_ (int *nested)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  omp_set_nested (*nested);
}

void
omp_set_nested__ (int *nested)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  omp_set_nested (*nested);
}

int
omp_get_nested_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_nested ();
}

int
omp_get_nested__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

  return omp_get_nested ();
}

void 
omp_set_schedule_ ( omp_sched_t * kind, int * modifier ) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    omp_set_schedule( *kind, *modifier ) ;
}

void 
omp_set_schedule__ ( omp_sched_t * kind, int * modifier ) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    omp_set_schedule( *kind, *modifier ) ;
}

void
omp_get_schedule_ ( omp_sched_t * kind, int * modifier )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    omp_get_schedule( kind, modifier );
}

void
omp_get_schedule__ ( omp_sched_t * kind, int * modifier )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    omp_get_schedule( kind, modifier );
}

int
omp_get_thread_limit_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_thread_limit() ;
}

int
omp_get_thread_limit__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_thread_limit() ;
}

void
omp_set_max_active_levels_ ( int * max_levels )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    fprintf(stderr, "%s - %d", __func__, *max_levels);
    omp_set_max_active_levels( *max_levels ) ;
}

void
omp_set_max_active_levels__ ( int * max_levels )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    omp_set_max_active_levels( *max_levels ) ;
}

int
omp_get_max_active_levels_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_max_active_levels() ;
}

int
omp_get_max_active_levels__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_max_active_levels() ;
}

int
omp_get_level_ (void)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_level() ;
}

int
omp_get_level__ (void)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

    return omp_get_level() ;
}

int
omp_get_ancestor_thread_num_ (int * level)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_ancestor_thread_num(*level);
}

int
omp_get_ancestor_thread_num__ (int * level)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_ancestor_thread_num(*level);
}

int 
omp_get_team_size_ (int * level)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_team_size(*level);
}

int 
omp_get_team_size__ (int * level)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_team_size(*level);
}

int 
omp_get_active_level_ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_active_level ();
}

int 
omp_get_active_level__ ()
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	return omp_get_active_level ();
}

int
omp_in_final_ ()
{
    return omp_in_final() ;
}

int
omp_in_final__ ()
{
    return omp_in_final() ;
}

int
omp_control_tool_ (int command, int modifier, void *arg)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    return omp_control_tool(command,modifier,arg);
}

int
omp_control_tool__ (int command, int modifier, void *arg)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    return omp_control_tool(command,modifier,arg);
}

/* LOCK FUNCTIONS */
void
omp_init_lock_ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_init_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_init_lock__ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_init_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_destroy_lock_ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_destroy_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_destroy_lock__ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_destroy_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_set_lock_ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_set_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_set_lock__ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_set_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_unset_lock_ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_unset_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_unset_lock__ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_unset_lock (lock);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

int
omp_test_lock_ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  return omp_test_lock( lock ) ;
}

int
omp_test_lock__ (omp_lock_t * lock)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  return omp_test_lock( lock ) ;
}

/* NEST-LOCK FUNCTIONS */
void
omp_init_nest_lock_ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_init_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void
omp_init_nest_lock__ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_init_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_destroy_nest_lock_ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_destroy_nest_lock( lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_destroy_nest_lock__ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_destroy_nest_lock( lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_set_nest_lock_ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_set_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_set_nest_lock__ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_set_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_unset_nest_lock_ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_unset_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void 
omp_unset_nest_lock__ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  omp_unset_nest_lock( lock ) ;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

int 
omp_test_nest_lock_ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  return omp_test_nest_lock( lock ) ;
}

int 
omp_test_nest_lock__ (omp_nest_lock_t * lock) 
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
  _mpc_omp_ompt_frame_get_infos();
  _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  return omp_test_nest_lock( lock ) ;
}


/* TIME FUNCTIONS */
double
omp_get_wtime_ ()
{
  return omp_get_wtime ();
}

double
omp_get_wtime__ ()
{
  return omp_get_wtime ();
}

double
omp_get_wtick_ ()
{
  return omp_get_wtick ();
}

double
omp_get_wtick__ ()
{
  return omp_get_wtick ();
}

