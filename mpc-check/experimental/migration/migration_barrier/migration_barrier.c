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

int
main (int argc, char **argv)
{
  char *printing;
  MPC_Comm my_com;
  int my_rank;
  int i;
  int j;
  int my_proc;
  int my_proc_tot;
  int my_process;
  int my_process_tot;

  printing = getenv ("MPC_TEST_SILENCE");
  if (printing != NULL)
    is_printing = 0;

  my_com = MPC_COMM_WORLD;
  MPC_Comm_rank (my_com, &my_rank);
  MPC_Processor_number (&my_proc_tot);
  MPC_Processor_rank (&my_proc);
  MPC_Process_number (&my_process_tot);
  MPC_Process_rank (&my_process);

  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  if (my_rank == 0)
    {
      MPC_Move_to (my_process_tot - 1, 0);
    }
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);
  MPC_Barrier (my_com);

  for (j = 0; j < 5; j++)
    {
      for (i = j; i < j + 3; i++)
	{
	  int val;
	  int res;

	  if (my_rank == 0)
	    {
	      mprintf (stderr,
		       "********************* STEP %d,%d *********************\n",
		       i - j, j);
	    }

	  val = my_rank;

	  mprintf (stderr, "TASK %d on CPU %d/%d PROCESS %d/%d\n", my_rank,
		   my_proc, my_proc_tot, my_process, my_process_tot);

	  if (i % 3 == 0)
	    {
	      if (my_rank == 0)
		{
		  mprintf (stderr, "BARRIER\n");
		}
	      MPC_Barrier (my_com);
	      MPC_Barrier (my_com);
	    }
	  else if (i % 3 == 1)
	    {
	      if (my_rank == 0)
		{
		  mprintf (stderr, "BROADCAST\n");
		}
	      MPC_Bcast (&val, 1, MPC_INT, 0, my_com);
	      MPC_Barrier (my_com);
	      assert (val == 0);
	    }
	  else
	    {
	      if (my_rank == 0)
		{
		  mprintf (stderr, "ALLREDUCE\n");
		}
	      MPC_Allreduce (&val, &res, 1, MPC_INT, MPC_MIN, my_com);
	      MPC_Barrier (my_com);
	      assert (res == 0);
	    }

	  MPC_Move_to (my_process, (my_proc + 1) % my_proc_tot);

	  MPC_Processor_rank (&my_proc);

	  if (my_rank == 0)
	    {
	      mprintf (stderr,
		       "********************* STEP %d,%d DONE ****************\n",
		       i - j, j);
	    }
	}

      mprintf (stderr, "PROCESS MOVE TASK %d on %d\n", my_rank,
	       (my_process + 1) % my_process_tot);
      MPC_Move_to ((my_process + 1) % my_process_tot, 0);

      MPC_Process_rank (&my_process);
      MPC_Processor_number (&my_proc_tot);
      MPC_Processor_rank (&my_proc);
/*     mprintf(stderr,"PROCESS MOVE TASK %d on CPU %d/%d PROCESS %d/%d\n",my_rank,my_proc,my_proc_tot, */
/* 	    my_process,my_process_tot); */
    }
  mprintf (stderr, "TASK %d DONE\n", my_rank);
  return 0;
}
