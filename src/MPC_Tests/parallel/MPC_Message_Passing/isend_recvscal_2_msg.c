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

int is_printing = 1;

#define mprintf if(is_printing) fprintf
#define REQUEST_COUNT 2

void
run ()
{
  MPI_Comm my_com;
  MPI_Request reqs[2];
  int my_rank;
  char msg[50];
  char msg2[50];

  my_com = MPI_COMM_WORLD;
  MPI_Comm_rank (my_com, &my_rank);

  sprintf (msg, "nothing");
  sprintf (msg2, "nothing");

  if (my_rank == 0)
    {
      sprintf (msg, "it works 1");
      sprintf (msg2, "it works 2");
      MPI_Isend (msg, 40, MPI_CHAR, 1, 0, my_com, &reqs[0]);
      MPI_Isend (msg2, 40, MPI_CHAR, 1, 1, my_com, &reqs[1]);
      MPI_Waitall (REQUEST_COUNT, reqs, NULL);
    }
  else
    {
      if (my_rank == 1)
	{
	  MPI_Irecv (msg, 40, MPI_CHAR, 0, 0, my_com, &reqs[0]);
	  MPI_Irecv (msg2, 40, MPI_CHAR, 0, 1, my_com, &reqs[1]);
      MPI_Waitall (REQUEST_COUNT, reqs, NULL);
	  mprintf (stderr, "msg = %s\n", msg);
	  assert (strcmp (msg2, "it works 2") == 0);
	  mprintf (stderr, "msg2 = %s\n", msg2);
	  assert (strcmp (msg, "it works 1") == 0);
	}
    }

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
