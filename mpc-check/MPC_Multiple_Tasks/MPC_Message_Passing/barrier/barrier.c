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

int is_printing = 1;

#define mprintf if(is_printing) fprintf

int
main (int argc, char **argv)
{
  char *printing;
  int my_rank;
  MPC_Comm my_com;

  printing = getenv ("MPC_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  MPC_Barrier (my_com);
  mprintf (stderr, "Avant barrier init %d\n", my_rank);
  MPC_Barrier (my_com);
  mprintf (stderr, "Avant barriers %d\n", my_rank);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  mprintf (stderr, "Apres barriers %d\n", my_rank);
  return 0;
}
