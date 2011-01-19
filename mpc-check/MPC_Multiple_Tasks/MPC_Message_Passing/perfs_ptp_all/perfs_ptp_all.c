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

void
message (int my_rank, int my_size, char *msg, size_t size, size_t iters)
{
  int i;
  double start;
  double end;
  double t;
  double tmp_t;
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);

  if (my_rank % 2 == 0)
    {
      for (i = 0; i < iters; i++)
	{
	  MPC_Send (msg, size, MPC_CHAR,
		    (my_rank + my_size / 2 + 1) % my_size, 0, MPC_COMM_WORLD);
	}
    }
  else
    {
      MPC_Status status;
      for (i = 0; i < iters; i++)
	{
	  MPC_Recv (msg, size, MPC_CHAR,
		    (my_rank + my_size - my_size / 2 - 1) % my_size, 0,
		    MPC_COMM_WORLD, &status);
	}
    }


  start = rrrsctk_get_time_stamp ();

  if (my_rank % 2 == 0)
    {
      for (i = 0; i < iters; i++)
	{
	  MPC_Send (msg, size, MPC_CHAR,
		    (my_rank + my_size / 2 + 1) % my_size, 0, MPC_COMM_WORLD);
	}
    }
  else
    {
      MPC_Status status;
      for (i = 0; i < iters; i++)
	{
	  MPC_Recv (msg, size, MPC_CHAR,
		    (my_rank + my_size - my_size / 2 - 1) % my_size, 0,
		    MPC_COMM_WORLD, &status);
	}
    }


/*   MPC_Barrier (MPC_COMM_WORLD); */
  end = rrrsctk_get_time_stamp ();
  t = end - start;
  tmp_t = t;

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Allreduce (&tmp_t, &t, 1, MPC_DOUBLE, MPC_SUM, MPC_COMM_WORLD);
  t = t / my_size;

  if (my_rank == 0)
    fprintf (stderr,
	     "Ping %d size %9lu (MPC_Send->MPC_Recv) %10.2fus %10.2f %10.2fMo/s\n",
	     my_rank, size, (t) / iters, (((t) / iters) * 1600) / 4,
	     ((double) ((iters) * (double) size) * (my_size / 2)) / (1024.0 *
								     1024.0) /
	     (((t)) / 1000000));
  fflush (stderr);
  MPC_Barrier (MPC_COMM_WORLD);

/*   if(my_rank == my_size-1) */
/*     fprintf(stderr,"Ping %d size %9lu (MPC_Send->MPC_Recv) %10.2fus %10.2f %10.2fMo/s\n\n",my_rank,size,(end-start)/iters, */
/* 	    (((end-start)/iters)*1600)/4, */
/* 	    ((double)((iters)*(double)size))/(1024.0*1024.0)/(((end-start))/1000000)); */
/*   fflush(stderr); */

  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
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

  MPC_Init (&argc, &argv);

  MPC_Comm_rank (MPC_COMM_WORLD, &my_rank);
  MPC_Comm_size (MPC_COMM_WORLD, &my_size);

  assert(my_size % 2 == 0);
  assert(my_size > 2);

  for (size = 1; size < 1024 * 1024; size *= 2)
    {
      message (my_rank, my_size, msg, size, 10);
    }
  message (my_rank, my_size, msg, max_tab_size, 5);

  MPC_Finalize ();
  return 0;
}
