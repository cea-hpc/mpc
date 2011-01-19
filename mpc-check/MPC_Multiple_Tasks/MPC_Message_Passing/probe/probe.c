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

int is_printing = 1;

#define mprintf if(is_printing) fprintf

void
run (void *arg)
{
  MPC_Comm my_com;
  int my_rank;
  char msg[50];
  MPC_Status status;

  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);

  sprintf (msg, "nothing");

  if (my_rank == 0)
    {
      sprintf (msg, "it works");
      mprintf (stderr, "init msg = %s\n", msg);
      MPC_Send (msg, 40, MPC_CHAR, 1, 40, my_com);
      mprintf (stderr, "init msg = %s\n", msg);
    }
  else
    {
      if (my_rank == 1)
	{
	  mprintf (stderr, "msg = %s\n", msg);
	  MPC_Probe (0, 40, my_com, &status);
	  assert (status.MPC_SOURCE == 0);
	  assert (status.MPC_TAG == 40);
	  MPC_Recv (msg, 40, MPC_CHAR, 0, 40, my_com, NULL);
	  mprintf (stderr, "msg = %s\n", msg);
	  assert (strcmp (msg, "it works") == 0);
	}
    }
  mprintf (stderr, "milieu msg = %s\n", msg);

  sprintf (msg, "nothing");

  if (my_rank == 0)
    {
      sprintf (msg, "it works");
      mprintf (stderr, "init msg = %s\n", msg);
      MPC_Send (msg, 40, MPC_CHAR, 1, 10, my_com);
      mprintf (stderr, "init msg = %s\n", msg);
    }
  else
    {
      if (my_rank == 1)
	{
	  mprintf (stderr, "msg = %s\n", msg);
	  MPC_Probe (0, 10, my_com, &status);
	  assert (status.MPC_SOURCE == 0);
	  assert (status.MPC_TAG == 10);
	  MPC_Recv (msg, 40, MPC_CHAR, 0, 10, my_com, NULL);
	  mprintf (stderr, "msg = %s\n", msg);
	  assert (strcmp (msg, "it works") == 0);
	}
    }
  mprintf (stderr, "fin msg = %s\n", msg);
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
