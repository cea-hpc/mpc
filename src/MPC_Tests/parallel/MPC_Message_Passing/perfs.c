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
/* #                                                                      # */
/* ######################################################################## */
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#ifndef LARGE_TEST
#define iters 20
#else
#define iters 10000
#endif

#define max_tab_size (4*1024*1024)

static double
rrrmpc_arch_get_timestamp_gettimeofday ()
{
  struct timeval tp;
  double res;
  gettimeofday (&tp, NULL);

  res = (double) tp.tv_sec * 1000000.0;
  res += tp.tv_usec;

  return res;
}

#ifdef __mpc__H
extern void mpc_common_debug_abort ();

static void
sctk_require_page_handler (int signo, siginfo_t * info, void *context)
{
  static mpc_thread_mutex_t lock = MPI_THREAD_MUTEX_INITIALIZER;
  mpc_thread_mutex_lock (&lock);
  fprintf (stderr, "Erreure de segmentation\n");
  mpc_common_debug_abort ();
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

  msg = malloc (max_tab_size);
  sendbuf = malloc (max_tab_size * sizeof (int));
  recvbuf = malloc (max_tab_size * sizeof (int));

#ifdef MPC_MPI_MPC_CL_H
  struct sigaction sigparam;
  sigparam.sa_flags = SA_SIGINFO;
  sigparam.sa_sigaction = sctk_require_page_handler;
  sigemptyset (&sigparam.sa_mask);
  sigaction (SIGSEGV, &sigparam, NULL);
#endif
  MPI_Init (&argc, &argv);

  MPI_Comm_rank (MPC_COMM_WORLD, &my_rank);

#if 1
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters; i++)
    {
      MPI_Barrier (MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Barrier %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters; i++)
    {
      MPI_Bcast (msg, 1, MPI_CHAR, 0, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

#endif
  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters; i++)
    {
      MPI_Allreduce (sendbuf, recvbuf, 1, MPI_INT, MPI_SUM, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters; i++)
    {
      MPI_Bcast (msg, 512, MPI_CHAR, 0, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast 512 %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);


  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters; i++)
    {
      MPI_Allreduce (sendbuf, recvbuf, 128, MPI_INT, MPI_SUM, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce 512 %fus %f\n", (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters/2; i++)
    {
      MPI_Bcast (msg, 8 * 1024, MPI_CHAR, 0, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %d %fus %f\n", 8 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);


  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters/10; i++)
    {
      MPI_Allreduce (sendbuf, recvbuf, 8 * 1024 / 4,
		     MPI_INT, MPI_SUM, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %d %fus %f\n", 8 * 1024, (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters / 20; i++)
    {
      MPI_Bcast (msg, 1024 * 1024, MPI_CHAR, 0, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Broadcast %d %fus %f\n", 1024 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);


  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  for (i = 0; i < iters / 20; i++)
    {
      MPI_Allreduce (sendbuf, recvbuf, 1024 * 1024 / 4,
		     MPI_INT, MPI_SUM, MPC_COMM_WORLD);
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    fprintf (stderr, "Reduce %d %fus %f\n", 1024 * 1024,
	     (end - start) / iters, (((end - start) / iters) * 1600) / 4);

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);


  MPI_Finalize ();
  return 0;
}
