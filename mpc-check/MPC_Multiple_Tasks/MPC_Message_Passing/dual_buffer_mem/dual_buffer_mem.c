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

void
sctk_switch_to_short_life_alloc ()
{
}

void
sctk_switch_to_long_life_alloc ()
{
}

#define mprintf if(is_printing) fprintf

void
run (void *arg)
{
  MPC_Comm my_com;
  int my_rank;
  char name[4096];
  int i;
  void *tmp1;
  void *tmp2;
  void *tmp3;
  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  gethostname (name, 4095);
  mprintf (stderr, "coucou from %d %s\n", my_rank, name);
  free (calloc (1, 8 * 1024 * 1024));
  free (calloc (1, 4 * 1024 * 1024));
  free (calloc (1, 1024 * 1024));
  free (calloc (1, 512 * 1024));
  tmp1 = calloc (1, 512 * 1024);
  tmp2 = calloc (1, 512 * 1024);
  tmp3 = calloc (1, 512 * 1024);
  free (tmp1);
  free (tmp2);
  free (tmp3);
  for (i = 0; i < 10; i++)
    {
      sctk_switch_to_short_life_alloc ();
      free (calloc (1, 4 * 1024 * 1024));
      free (calloc (1, 1024 * 1024));
      free (calloc (1, 512 * 1024));
      sctk_switch_to_long_life_alloc ();
      free (calloc (1, 1024 * 1024));
      free (calloc (1, 1024 * 1024));
      free (calloc (1, 512 * 1024));
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
