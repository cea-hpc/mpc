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
#include <assert.h>

int is_printing = 1;

#define mprintf if(is_printing) fprintf
void
run (void *arg)
{
  mpc_mp_communicator_t my_com;
  mpc_mp_communicator_t my_com2;
  int my_rank;
  int my_rank2;
  int dom_size;
  int *tab;

  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  MPC_Comm_size (my_com, &dom_size);

  tab = (int *) malloc (dom_size * sizeof (int));

  for (my_rank = 0; my_rank < dom_size; my_rank++)
    {
      tab[my_rank] = (1 + my_rank) % dom_size;
    }

  MPC_Comm_rank (my_com, &my_rank);

  mprintf (stderr, "Avant creation %d\n", my_rank);
  MPC_Comm_create_list (my_com, tab, dom_size, &my_com2);
  MPC_Comm_rank (my_com2, &my_rank2);
  mprintf (stderr, "Apres creation %d %d\n", my_rank, my_rank2);
  assert(my_rank2 == (my_rank + dom_size - 1) % dom_size);
  MPC_Barrier (my_com2);
  mprintf (stderr, "Apres barrier %d\n", my_rank);
  MPC_Comm_free (&my_com2);
  mprintf (stderr, "Apres destruction %d\n", my_rank);
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
