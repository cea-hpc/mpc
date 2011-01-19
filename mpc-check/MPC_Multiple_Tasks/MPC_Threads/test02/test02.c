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
#include <stdlib.h>
#include <mpcthread.h>

mpc_thread_mutex_t count_mutex = MPC_THREAD_MUTEX_INITIALIZER;
mpc_thread_mutex_t condition_mutex = MPC_THREAD_MUTEX_INITIALIZER;
mpc_thread_cond_t condition_cond = MPC_THREAD_COND_INITIALIZER;

void *functionCount1 ();
void *functionCount2 ();
volatile int count = 0;
#define COUNT_DONE  10
#define COUNT_HALT1  3
#define COUNT_HALT2  6

int
main (int argc, char **argv)
{
  mpc_thread_t thread1, thread2;

  mpc_thread_create (&thread1, NULL, &functionCount1, NULL);
  mpc_thread_create (&thread2, NULL, &functionCount2, NULL);
  mpc_thread_join (thread1, NULL);
  mpc_thread_join (thread2, NULL);

  return 0;
}

void *
functionCount1 ()
{
  for (;;)
    {
      mpc_thread_mutex_lock (&condition_mutex);
      while (count >= COUNT_HALT1 && count <= COUNT_HALT2)
	{
	  mpc_thread_cond_wait (&condition_cond, &condition_mutex);
	}
      mpc_thread_mutex_unlock (&condition_mutex);

      mpc_thread_mutex_lock (&count_mutex);
      count++;
      printf ("Counter value functionCount1: %d\n", count);
      mpc_thread_mutex_unlock (&count_mutex);

      if (count >= COUNT_DONE)
	return (NULL);
    }
}

void *
functionCount2 ()
{
  for (;;)
    {
      mpc_thread_mutex_lock (&condition_mutex);
      if (count < COUNT_HALT1 || count > COUNT_HALT2)
	{
	  mpc_thread_cond_signal (&condition_cond);
	}
      mpc_thread_mutex_unlock (&condition_mutex);

      mpc_thread_mutex_lock (&count_mutex);
      count++;
      printf ("Counter value functionCount2: %d\n", count);
      mpc_thread_mutex_unlock (&count_mutex);

      if (count >= COUNT_DONE)
	{
	  mpc_thread_mutex_lock (&condition_mutex);
	  mpc_thread_cond_signal (&condition_cond);
	  mpc_thread_mutex_unlock (&condition_mutex);
	  return (NULL);
	}
    }

}
