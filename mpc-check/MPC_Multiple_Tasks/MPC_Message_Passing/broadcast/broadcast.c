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
  int i;
  MPC_Comm my_com;
  int my_rank;
  char msg[50];
  sprintf (msg, "nothing");
  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);


  if (my_rank == 0)
    {
      sprintf (msg, "it works");
    }

  mprintf (stderr, "Avant broadcast %d\n", my_rank);
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  mprintf (stderr, "Apres broadcast %d\n", my_rank);
  mprintf (stderr, "RECVED %d msg = %s\n", my_rank, msg);
  assert (strcmp (msg, "it works") == 0);

  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);
  msg[1] = 't';
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);

  sprintf (msg, "nothing");
  if (my_rank == 0)
    {
      sprintf (msg, "it works");
    }
  MPC_Bcast (msg, 40, MPC_CHAR, 0, my_com);

  mprintf (stderr, "%d msg = %s\n", my_rank, msg);
  assert (strcmp (msg, "it works") == 0);
  sprintf (msg, "nothing");

  sprintf (msg, "nothing");
  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);


  if (my_rank == 1)
    {
      sprintf (msg, "it works");
    }

  mprintf (stderr, "Avant broadcast %d\n", my_rank);
  MPC_Bcast (msg, 40, MPC_CHAR, 1, my_com);
  mprintf (stderr, "Apres broadcast %d\n", my_rank);
  mprintf (stderr, "RECVED %d msg = %s\n", my_rank, msg);
  assert (strcmp (msg, "it works") == 0);

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
