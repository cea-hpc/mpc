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
#include "mpc_keywords.h"
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>


#ifdef __mpc__H
extern void mpc_common_debug_abort ();

static void
sctk_require_page_handler (int signo, siginfo_t * info, void *context)
{
  static mpc_thread_mutex_t lock = MPI_THREAD_MUTEX_INITIALIZER;
  mpc_thread_mutex_lock (&lock);
  fprintf (stderr, "Handler caught segmentation fault\n");
  mpc_common_debug_abort ();
  mpc_thread_mutex_unlock (&lock);
}
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


void
message (int my_rank, int my_size, char *msg, size_t size, size_t iters)
{
  size_t i;
  double start;
  double end;
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);

  start = rrrmpc_arch_get_timestamp_gettimeofday ();
  if (my_rank == 0)
    {
      for (i = 0; i < iters; i++)
	{
	  MPI_Send (msg, size, MPI_CHAR, my_size - 1, 0, MPC_COMM_WORLD);
	}
    }
  if (my_rank == my_size - 1)
    {
      mpc_lowcomm_status_t status;
      for (i = 0; i < iters; i++)
	{
	  MPI_Recv (msg, size, MPI_CHAR, 0, 0, MPC_COMM_WORLD, &status);
	}
    }
  MPI_Barrier (MPC_COMM_WORLD);
  end = rrrmpc_arch_get_timestamp_gettimeofday ();
  MPI_Barrier (MPC_COMM_WORLD);
  if (my_rank == 0)
    fprintf (stderr,
	     "Ping %d size %9lu (MPI_Send->MPI_Recv) %10.2fus %10.2f %10.2fMo/s\n",
	     my_rank, size, (end - start) / iters,
	     (((end - start) / iters) * 1600) / 4,
	     ((double) ((iters) * (double) size)) / (1024.0 * 1024.0) /
	     (((end - start)) / 1000000));
  fflush (stderr);
  MPI_Barrier (MPC_COMM_WORLD);

/*   if(my_rank == my_size-1) */
/*     fprintf(stderr,"Ping %d size %9lu (MPI_Send->MPI_Recv) %10.2fus %10.2f %10.2fMo/s\n\n",my_rank,size,(end-start)/iters, */
/* 	    (((end-start)/iters)*1600)/4, */
/* 	    ((double)((iters)*(double)size))/(1024.0*1024.0)/(((end-start))/1000000)); */
/*   fflush(stderr); */

  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
  MPI_Barrier (MPC_COMM_WORLD);
}

int
main (int argc, char **argv)
{
  int my_rank;
  int my_size;
  char *msg;
  size_t size;

#ifdef MPC_MPI_MPC_CL_H
  struct sigaction sigparam;

  sigparam.sa_flags = SA_SIGINFO;
  sigparam.sa_sigaction = sctk_require_page_handler;
  sigemptyset (&sigparam.sa_mask);
  sigaction (SIGSEGV, &sigparam, NULL);
#endif

  MPI_Init (&argc, &argv);
  msg = malloc (max_tab_size);
  memset (msg, 'a', max_tab_size);


  MPI_Comm_rank (MPC_COMM_WORLD, &my_rank);
  MPI_Comm_size (MPC_COMM_WORLD, &my_size);

//#ifndef LARGE_TEST
//  message (my_rank, my_size, msg, 7*1024, 100000);
//  return 0;
//#endif

  if(my_rank == 0)
  fprintf(stderr,"To last\n");
  for (size = 1; size < 1024 * 1024; size *= 2)
    {
      message (my_rank, my_size, msg, size, 10000);
    }
  message (my_rank, my_size, msg, max_tab_size, 1000);

  if(my_rank == 0)
  fprintf(stderr,"To next\n");
  for (size = 1; size < 1024 * 1024; size *= 2)
    {
      message (my_rank, 2, msg, size, 50000);
    }
  message (my_rank, 2, msg, max_tab_size, 1000);

  MPI_Finalize ();
  return 0;
}
