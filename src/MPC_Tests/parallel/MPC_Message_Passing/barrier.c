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
#include "mpc_lowcomm_communicator.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

int is_printing = 1;

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

#define mprintf if(is_printing) fprintf
#define NB_BARRIER 100

int
main ()
{
  char *printing;
  int my_rank;
  mpc_lowcomm_communicator_t my_com;
  int i;
  double begin, end;

  printing = getenv ("MPC_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  my_com = MPC_COMM_WORLD;
  MPI_Comm_rank (my_com, &my_rank);
  MPI_Barrier (my_com);
  mprintf (stderr, "Before barrier init %d\n", my_rank);
  MPI_Barrier (my_com);
  mprintf (stderr, "Before barriers %d\n", my_rank);

  begin = rrrmpc_arch_get_timestamp_gettimeofday();
  for (i=0; i<NB_BARRIER; ++i)
  {
    MPI_Barrier (my_com);
  }
  end = rrrmpc_arch_get_timestamp_gettimeofday();
  mprintf (stderr, "After barriers %d\n", my_rank);

  if (my_rank == 0)
    mprintf (stderr, "Barrier total time %f. Mean time %f\n", end-begin, (end-begin) / NB_BARRIER);

  return 0;
}
