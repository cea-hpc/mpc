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
#include <stdio.h>
#include <mpcomp_abi.h>
#include <assert.h>


void *
run (void *arg)
{
  mpc_omp_lock_t *lock;

  assert (mpcomp_in_parallel ());

  lock = (mpc_omp_lock_t *) arg;

  mpcomp_set_lock (lock);
  usleep (30);
  mpcomp_unset_lock (lock);
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
    mpc_omp_lock_t lock;
    mpcomp_init_lock (&lock);
    __mpcomp_start_parallel_region (10, run, (void *) &lock);
    mpcomp_destroy_lock (&lock);
  }

  fprintf (stderr, "ALL DONE\n");
  return 0;
}
