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

int is_printing = 1;

#define mprintf if(is_printing) fprintf

void
run (void *arg)
{
  sctk_communicator_t my_com;
  int my_rank;
  char msg[50];
  int my_size;
  MPC_Request req;
  unsigned int beg[10];
  unsigned int end[10];

  unsigned int deb = 0;
  unsigned int fin = 40;

  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  MPC_Comm_size (my_com, &my_size);

  if (my_rank == 0)
    {
      sprintf (msg, "it works");

      MPC_Open_pack (&req);
      MPC_Add_pack (msg, 1, &deb, &fin, MPC_CHAR, &req);
      MPC_Isend_pack ((my_size - 1), 0, my_com, &req);

      MPC_Wait_pending (my_com);
      mprintf (stderr, "Send msg = |%s|\n", msg);
    }
  else
    {
      if (my_rank == (my_size - 1))
	{
	  sprintf (msg, "nothing");

	  MPC_Open_pack (&req);
	  MPC_Add_pack (msg, 1, &deb, &fin, MPC_CHAR, &req);
	  MPC_Irecv_pack (0, 0, my_com, &req);

	  MPC_Wait_pending (my_com);
	  mprintf (stderr, "Recv msg = |%s|\n", msg);
	  assert (strcmp (msg, "it works") == 0);
	}
    }

  MPC_Barrier (MPC_COMM_WORLD);

  if (my_rank == 0)
    {
      mprintf (stderr, "First done %d\n", my_rank);
      sprintf (msg, "it works");

      beg[0] = 0;
      end[0] = 7;

      MPC_Open_pack (&req);
      MPC_Add_pack (msg, 1, beg, end, MPC_CHAR, &req);
      MPC_Isend_pack ((my_size - 1), 0, my_com, &req);

      MPC_Wait_pending (my_com);
      mprintf (stderr, "All done %d\n", my_rank);
    }
  else
    {
      if (my_rank == (my_size - 1))
	{
	  mprintf (stderr, "First done %d\n", my_rank);
	  sprintf (msg, "nothing");

	  beg[0] = 4;
	  end[0] = 7;
	  beg[1] = 0;
	  end[1] = 3;

	  MPC_Open_pack (&req);
	  MPC_Add_pack (msg, 2, beg, end, MPC_CHAR, &req);
	  MPC_Irecv_pack (0, 0, my_com, &req);

	  MPC_Wait_pending (my_com);
	  mprintf (stderr, "msg = %s\n", msg);
	  assert (strcmp (msg, "orksit w") == 0);
	  mprintf (stderr, "All done %d\n", my_rank);
	}
    }

}

int
main (int argc, char **argv)
{
  char *printing;

  printing = getenv ("MPC_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  run (NULL);
  return 0;
}
