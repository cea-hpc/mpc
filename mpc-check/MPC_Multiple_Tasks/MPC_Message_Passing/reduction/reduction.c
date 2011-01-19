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
#include <assert.h>

int is_printing = 1;

#define mprintf if(is_printing) fprintf


void
run (void *arg)
{
  MPC_Comm my_com;
  int my_rank;
  int my_size;
  int res;
  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  MPC_Comm_size (my_com, &my_size);


  MPC_Allreduce (&my_rank, &res, 1, MPC_INT, MPC_MIN, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == 0);
  MPC_Comm_rank (my_com, &res);
  assert (my_rank == res);

  MPC_Allreduce (&my_rank, &res, 1, MPC_INT, MPC_MAX, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == my_size - 1);
  MPC_Comm_rank (my_com, &res);
  assert (my_rank == res);

  MPC_Allreduce (&my_rank, &res, 1, MPC_INT, MPC_SUM, my_com);
  mprintf (stderr, "%d val %d\n", my_rank, res);
  assert (res == ((float) ((my_size - 1) * my_size) / ((float) 2)));
  MPC_Comm_rank (my_com, &res);
  assert (my_rank == res);
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
