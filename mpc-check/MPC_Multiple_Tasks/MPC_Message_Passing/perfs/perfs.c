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
#include "mpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#ifndef LARGE_TEST
#define iters 10
#else
#define iters 10000
#endif

#define max_tab_size (4*1024*1024)

static double
rrrsctk_get_time_stamp ()
{
  struct timeval tp;
  double res;
  gettimeofday (&tp, NULL);

  res = (double) tp.tv_sec * 1000000.0;
  res += tp.tv_usec;

  return res;
}

#ifdef __mpc__H
extern void sctk_abort ();

static void
sctk_require_page_handler (int signo, siginfo_t * info, void *context)
{
  static mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
  mpc_thread_mutex_lock (&lock);
  fprintf (stderr, "Erreure de segmentation\n");
  sctk_abort ();
  mpc_thread_mutex_unlock (&lock);
}
#endif
int
main (int argc, char **argv)
{
  int i;
  double start, end;
  int my_rank;
  char *msg;
  int *sendbuf;
  int *recvbuf;
  struct sigaction sigparam;

  msg = malloc (max_tab_size);
  sendbuf = malloc (max_tab_size * sizeof (int));
  recvbuf = malloc (max_tab_size * sizeof (int));

#ifdef __mpc__H
  sigparam.sa_flags = SA_SIGINFO;
  sigparam.sa_sigaction = sctk_require_page_handler;
  sigemptyset (&sigparam.sa_mask);
  sigaction (SIGSEGV, &sigparam, NULL);
#endif
  MPC_Init (&argc, &argv);

  MPC_Comm_rank (MPC_COMM_WORLD, &my_rank);

#if 1
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Barrier (MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Barrier %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Bcast (msg, 1, MPC_CHAR, 0, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

#endif
  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Allreduce (sendbuf, recvbuf, 1, MPC_INT, MPC_SUM, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Bcast (msg, 512, MPC_CHAR, 0, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast 512 %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);


  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Allreduce (sendbuf, recvbuf, 128, MPC_INT, MPC_SUM, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce 512 %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Bcast (msg, 8 * 1024, MPC_CHAR, 0, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %d %fus %f\n", 8 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);


  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters; i++)
    {
      MPC_Allreduce (sendbuf, recvbuf, 8 * 1024 / 4,
		     MPC_INT, MPC_SUM, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %d %fus %f\n", 8 * 1024, (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters / 20; i++)
    {
      MPC_Bcast (msg, 1024 * 1024, MPC_CHAR, 0, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %d %fus %f\n", 1024 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);


  start = rrrsctk_get_time_stamp ();
  for (i = 0; i < iters / 20; i++)
    {
      MPC_Allreduce (sendbuf, recvbuf, 1024 * 1024 / 4,
		     MPC_INT, MPC_SUM, MPC_COMM_WORLD);
    }
  end = rrrsctk_get_time_stamp ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %d %fus %f\n", 1024 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);


  MPC_Finalize ();
  return 0;
}
