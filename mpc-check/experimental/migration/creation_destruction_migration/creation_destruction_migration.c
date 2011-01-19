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
  MPC_Comm my_com2;
  int my_rank;
  int my_rank2;
  int dom_size;
  int *tab;
  int my_process;
  int my_process_tot;
  int i;

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
  MPC_Barrier (my_com2);

  MPC_Errhandler_set(my_com,MPC_ERRORS_RETURN);
  MPC_Errhandler_set(my_com2,MPC_ERRORS_RETURN);

  mprintf(stderr,"Start Loop\n");

  for (i = 0; i < 10; i++)
    {
      int res;
      mprintf(stderr,"Start ier %d\n",i);
      MPC_Process_number (&my_process_tot);
      MPC_Process_rank (&my_process);
      res = MPC_Move_to ((my_process + 1) % my_process_tot, 0);
      if(res != MPC_SUCCESS){
	mprintf (stderr, "Fail to move %d\n", my_rank);
      } else {
	mprintf (stderr, "Succeed to move %d %d\n", my_rank,i);
      }
      mprintf(stderr,"Barrier on com 2\n");
      MPC_Barrier (my_com2);
      mprintf(stderr,"Barrier on com 2\n");
      MPC_Barrier (my_com2);
    }
  mprintf(stderr,"End Loop\n");


  mprintf (stderr, "Apres barrier %d\n", my_rank);
  MPC_Comm_free (&my_com2);
  mprintf (stderr, "Apres destruction %d\n", my_rank);
}

int
main (int argc, char **argv)
{
  char *printing;
  MPC_Init(&argc,&argv);

  printing = getenv ("MPC_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  run (NULL);
  MPC_Finalize();
  return 0;
}
