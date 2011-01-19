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
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <mpcthread.h>

#define NTHREADS 4
void *thread_function ();
mpc_thread_mutex_t mutex1 = MPC_THREAD_MUTEX_INITIALIZER;
int counter = 0;

int
main (int argc, char **argv)
{
  mpc_thread_t thread_id[NTHREADS];
  int i, j;

  for (i = 0; i < NTHREADS; i++)
    {
      mpc_thread_create (&thread_id[i], NULL, &thread_function, NULL);
    }

  for (j = 0; j < NTHREADS; j++)
    {
      mpc_thread_join (thread_id[j], NULL);
    }

  /* Now that all threads are complete I can print the final result.     */
  /* Without the join I could be printing a value before all the threads */
  /* have been completed.                                                */

  printf ("Final counter value: %d\n", counter);
  return 0;
}

void *
thread_function ()
{
  printf ("Thread number %ld\n", mpc_thread_self ());
  mpc_thread_mutex_lock (&mutex1);
  counter++;
  mpc_thread_mutex_unlock (&mutex1);
  return NULL;
}
