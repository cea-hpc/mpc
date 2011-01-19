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

void *print_message_function (void *ptr);

int
main (int argc, char **argv)
{
  mpc_thread_t thread1, thread2;
  char *message1 = "Thread 1";
  char *message2 = "Thread 2";
  int iret1, iret2;

  /* Create independent threads each of which will execute function */

  iret1 =
    mpc_thread_create (&thread1, NULL, print_message_function,
		       (void *) message1);
  iret2 =
    mpc_thread_create (&thread2, NULL, print_message_function,
		       (void *) message2);

  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

  mpc_thread_join (thread1, NULL);
  mpc_thread_join (thread2, NULL);

  printf ("Thread 1 returns: %d\n", iret1);
  printf ("Thread 2 returns: %d\n", iret2);
  return 0;
}

void *
print_message_function (void *ptr)
{
  char *message;
  message = (char *) ptr;
  printf ("%s \n", message);
  return NULL;
}
