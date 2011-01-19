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
#include <omp.h>
#include <omp_abi.h>
#include <assert.h>

void *
run (void *arg)
{
  assert (mpcomp_in_parallel ());
  assert (mpcomp_get_num_threads () == 6);
  fprintf (stderr, "BARRIER 1\n");
  __mpcomp_barrier ();
  fprintf (stderr, "BARRIER 2\n");
  __mpcomp_barrier ();
  return NULL;
}

int
main (int argc, char **argv)
{

  assert (mpcomp_get_thread_num () == 0);
  assert (mpcomp_get_num_threads () == 1);
  assert (mpcomp_in_parallel () == 0);

  __mpcomp_barrier ();

  fprintf (stderr, "I am %d on %d max %d cpus %d\n",
	   mpcomp_get_thread_num (),
	   mpcomp_get_num_threads (),
	   mpcomp_get_max_threads (), mpcomp_get_num_procs ());

  {
    int shared;
    __mpcomp_start_parallel_region (6, run, (void *) &shared);
  }
  fprintf (stderr, "ALL DONE\n");
  return 0;
}

