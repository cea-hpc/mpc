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
#include <assert.h>

int is_printing = 1;

#define mprintf if(is_printing) fprintf


void
run ()
{
  mpc_lowcomm_communicator_t my_com;
  int my_rank;
  int my_size;
  int res;
  my_com = MPC_COMM_WORLD;
  MPI_Comm_rank (my_com, &my_rank);
  MPI_Comm_size (my_com, &my_size);


  MPI_Allreduce (&my_rank, &res, 1, MPI_INT, MPI_MIN, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == 0);
  MPI_Comm_rank (my_com, &res);
  assert (my_rank == res);

  MPI_Allreduce (&my_rank, &res, 1, MPI_INT, MPI_MAX, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == my_size - 1);
  MPI_Comm_rank (my_com, &res);
  assert (my_rank == res);

  MPI_Allreduce (&my_rank, &res, 1, MPI_INT, MPI_SUM, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == ((float) ((my_size - 1) * my_size) / ((float) 2)));
  MPI_Comm_rank (my_com, &res);
  assert (my_rank == res);
}

int
main (int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  char *printing;

  printing = getenv ("MPI_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  run ();

  MPI_Finalize();

  return 0;
}
