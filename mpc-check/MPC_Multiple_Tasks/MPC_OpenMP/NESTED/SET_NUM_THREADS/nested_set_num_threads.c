/* ############################# MPC License ############################## */
/* # Thu Apr 24 18:48:27 CEST 2008                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* #                                                                      # */
/* # This file is part of the MPC Library.                                # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <mpcomp.h>
#include <assert.h>

int num_threads;
int num_threads2;

void *
run2 (void *arg)
{
  assert (mpcomp_in_parallel ());
  sctk_debug( "[Par1] I am %d on %d = %d",
	   mpcomp_get_thread_num (), mpcomp_get_num_threads ()-1, num_threads2);
  assert (mpcomp_get_num_threads() == num_threads2);
  return NULL;
}

void *
run (void *arg)
{
  num_threads2 = 2;

  assert (mpcomp_in_parallel ());
  assert (mpcomp_get_num_threads () == num_threads);
  sctk_debug( "[Par0] I am %d on %d = %d", mpcomp_get_thread_num (),
	   mpcomp_get_num_threads ()-1, num_threads );

  {
    int shared;
    __mpcomp_start_parallel_region (num_threads2, run2, (void *) &shared);
  }
  return NULL;
}

int
main (int argc, char **argv)
{

  mpcomp_set_nested( 1 ) ;

  num_threads = 2;

  mpcomp_set_num_threads (num_threads);

  assert (mpcomp_get_thread_num () == 0);
  assert (mpcomp_get_num_threads () == 1);
  assert (mpcomp_in_parallel () == 0);

  sctk_debug( "[Seq] I am %d on %d max %d cpus %d",
	   mpcomp_get_thread_num (),
	   mpcomp_get_num_threads ()-1,
	   mpcomp_get_max_threads (), mpcomp_get_num_procs ());

  {
    int shared;
    __mpcomp_start_parallel_region (-1, run, (void *) &shared);
  }

  sctk_debug( "[Seq] Success!" ) ;

  return 0;
}

