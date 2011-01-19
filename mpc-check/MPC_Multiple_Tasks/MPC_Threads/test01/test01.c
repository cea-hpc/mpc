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
#include "mpc.h"

#define TH_NB 4

mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
int pos = 0;

void *
run (void *arg)
{
  mpc_thread_mutex_lock (&lock);
  pos++;
  fprintf (stdout, "I am %ld, pos %d\n", (long) arg, pos);
  mpc_thread_mutex_unlock (&lock);
  return NULL;
}

int
main (int argc, char **argv)
{
  long i;
  void *res;
  mpc_thread_t pids[TH_NB];
  for (i = 0; i < TH_NB; i++)
    {
      mpc_thread_create (&pids[i], NULL, run, (void *) i);

    }
  for (i = 0; i < TH_NB; i++)
    {
      mpc_thread_join (pids[i], &res);
    }
  return 0;
}
