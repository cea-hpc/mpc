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
#include "mpcomp_parallel_region.h"
#include <stdio.h>
#include <mpc_omp_abi.h>
#include <assert.h>
#include <unistd.h>


void
run (void *arg)
{
  omp_lock_t *lock;

  assert (omp_in_parallel ());

  lock = (omp_lock_t *) arg;

  omp_set_lock (lock);
  usleep (30);
  omp_unset_lock (lock);
}

int
main ()
{
  assert (omp_get_thread_num () == 0);
  assert (omp_get_num_threads () == 1);
  assert (omp_in_parallel () == 0);

  mpc_omp_barrier (ompt_sync_region_barrier_implementation);

  fprintf (stderr, "I am %d on %d max %d cpus %d\n",
	   omp_get_thread_num (),
	   omp_get_num_threads (),
	   omp_get_max_threads (), omp_get_num_procs ());


  {
    omp_lock_t lock;
    omp_init_lock (&lock);
    _mpc_omp_start_parallel_region (run, (void *) &lock, 10);
    omp_destroy_lock (&lock);
  }

  fprintf (stderr, "ALL DONE\n");
  return 0;
}
