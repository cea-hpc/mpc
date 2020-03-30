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
#include "mpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>


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

void
message (int my_rank, int my_size, char *msg, size_t size, size_t iters)
{
  int i;
  double start;
  double end;
  double t;
  double tmp_t;
  MPI_Barrier (SCTK_COMM_WORLD);
  MPI_Barrier (SCTK_COMM_WORLD);
  MPI_Barrier (SCTK_COMM_WORLD);

  if (my_rank % 2 == 0)
    {
      for (i = 0; i < iters; i++)
	{
	  MPI_Send (msg, size, MPI_CHAR,
		    (my_rank + my_size / 2 + 1) % my_size, 0, SCTK_COMM_WORLD);
	}
    }
  else
    {
      mpc_lowcomm_status_t status;
      for (i = 0; i < iters; i++)
	{
	  MPI_Recv (msg, size, MPI_CHAR,
		    (my_rank + my_size - my_size / 2 - 1) % my_size, 0,
		    SCTK_COMM_WORLD, &status);
	}
    }


  start = rrrmpc_arch_get_timestamp_gettimeofday ();

  if (my_rank % 2 == 0)
    {
      for (i = 0; i < iters; i++)
	{
	  MPI_Send (msg, size, MPI_CHAR,
		    (my_rank + my_size / 2 + 1) % my_size, 0, SCTK_COMM_WORLD);
	}
    }
  else
    {
      mpc_lowcomm_status_t status;
      for (i = 0; i < iters; i++)
	{
	  MPI_Recv (msg, size, MPI_CHAR,
		    (my_rank + my_size - my_size / 2 - 1) % my_size, 0,
		    SCTK_COMM_WORLD, &status);
	}
    }


/*   MPI_Barrier (SCTK_COMM_WORLD); */
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  t = end - start;
  tmp_t = t;

  MPI_Barrier (SCTK_COMM_WORLD);
  MPI_Allreduce (&tmp_t, &t, 1, MPI_DOUBLE, MPI_SUM, SCTK_COMM_WORLD);
  t = t / my_size;

  if (my_rank == 0)
    fprintf (stderr,
	     "Ping %d size %9lu (MPI_Send->MPI_Recv) %10.2fus %10.2f %10.2fMo/s\n",
	     my_rank, size, (t) / iters, (((t) / iters) * 1600) / 4,
	     ((double) ((iters) * (double) size) * (my_size / 2)) / (1024.0 *
								     1024.0) /
	     (((t)) / 1000000));
  fflush (stderr);
  MPI_Barrier (SCTK_COMM_WORLD);

/*   if(my_rank == my_size-1) */
/*     fprintf(stderr,"Ping %d size %9lu (MPI_Send->MPI_Recv) %10.2fus %10.2f %10.2fMo/s\n\n",my_rank,size,(end-start)/iters, */
/* 	    (((end-start)/iters)*1600)/4, */
/* 	    ((double)((iters)*(double)size))/(1024.0*1024.0)/(((end-start))/1000000)); */
/*   fflush(stderr); */

  MPI_Barrier (SCTK_COMM_WORLD);
  MPI_Barrier (SCTK_COMM_WORLD);
  MPI_Barrier (SCTK_COMM_WORLD);
}

#ifdef __mpc__H
extern void sctk_abort ();

static void
sctk_require_page_handler (int signo, siginfo_t * info, void *context)
{
  static mpc_thread_mutex_t lock = MPI_THREAD_MUTEX_INITIALIZER;
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
  int my_size;
  char *msg;
  size_t size;
  struct sigaction sigparam;

  msg = malloc (max_tab_size);
  memset (msg, 0, max_tab_size);

/*   sleep(150); */

#ifdef __mpc__H
  sigparam.sa_flags = SA_SIGINFO;
  sigparam.sa_sigaction = sctk_require_page_handler;
  sigemptyset (&sigparam.sa_mask);
  sigaction (SIGSEGV, &sigparam, NULL);
#endif

  MPI_Init (&argc, &argv);

  MPI_Comm_rank (SCTK_COMM_WORLD, &my_rank);
  MPI_Comm_size (SCTK_COMM_WORLD, &my_size);

#ifndef LARGE_TEST
  message (my_rank, my_size, msg, 1, 100);
  return 0;
#else
  for (size = 1; size < 1024 * 1024; size *= 2)
    {
      message (my_rank, my_size, msg, size, 10000);
    }
  message (my_rank, my_size, msg, max_tab_size, 1000);
#endif

  MPI_Finalize ();
  return 0;
}
