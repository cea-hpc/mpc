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
  mpcomp_lock_t *lock;

  assert (mpcomp_in_parallel ());

  lock = (mpcomp_lock_t *) arg ;

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
    mpcomp_lock_t lock;
    mpcomp_init_lock (&lock);
    __mpcomp_start_parallel_region (10, run, (void *) &lock);
    mpcomp_destroy_lock (&lock);
  }

  fprintf (stderr, "ALL DONE\n");
  return 0;
}

