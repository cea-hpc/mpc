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
#include <assert.h>
#include <omp.h>
#include <omp_abi.h>

void *
run (void *arg)
{
  int shared = *(int *)arg ;
  assert (mpcomp_in_parallel ());
  assert( mpcomp_get_thread_num () >= 0 ) ;
  assert( mpcomp_get_thread_num () < mpcomp_get_num_threads() ) ;
  printf ("Thread# %d -> shared value = %d\n", mpcomp_get_thread_num (), shared);
  return NULL;
}

int
main (int argc, char **argv)
{
  int shared = 4;

  assert (mpcomp_get_thread_num () == 0);
  assert (mpcomp_get_num_threads () == 1);
  assert (mpcomp_in_parallel () == 0);

  __mpcomp_start_parallel_region (-1, run, (void *) &shared);
  return 0;
}

