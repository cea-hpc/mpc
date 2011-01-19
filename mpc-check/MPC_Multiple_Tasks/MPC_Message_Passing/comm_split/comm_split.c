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
#include <stdio.h>
#include <mpc.h>


static int *_is_Master;

void
init ()
{
  static volatile int done = 0;
  static mpc_thread_mutex_t lock;
  int is_master = 0;
  int process_rank;
  int root, RANK, i;
  MPC_Comm comm;
  static int *temp;


  int size;
  MPC_Comm_size (MPC_COMM_WORLD, &size);
  mpc_thread_mutex_lock (&lock);
  if (done == 0)
    {
      done = 1;
      is_master = 1;
      MPC_Comm_rank (MPC_COMM_WORLD, &root);
      _is_Master = (int *) calloc (size, sizeof (int));
      temp = (int *) calloc (size, sizeof (int));
      printf (" root = %d\n", root);
    }
  mpc_thread_mutex_unlock (&lock);

  MPC_Comm_rank (MPC_COMM_WORLD, &RANK);
  MPC_Process_rank (&process_rank);
  printf (" RANK = %d process_rank = %d\n", RANK, process_rank);
  MPC_Comm_split (MPC_COMM_WORLD, is_master, process_rank, &comm);
  printf ("fin de MPC_Comm_split %d \n", RANK);

  if (is_master == 1)
    {
      int rank;

      MPC_Comm_rank (comm, &rank);
      if (rank == 0)
	{
	  if (rank == 0)
	    temp[RANK] = -1;
	  else
	    temp[RANK] = 1;
	}
    }
  MPC_Reduce (temp, _is_Master, size, MPC_INT, MPC_SUM, 0, MPC_COMM_WORLD);
  MPC_Bcast (_is_Master, size, MPC_INT, 0, MPC_COMM_WORLD);
  MPC_Barrier (MPC_COMM_WORLD);
  if (RANK == 0)
    {
      for (i = 0; i < size; i++)
	printf (" ===> %d %d\n", i, _is_Master[i]);
      printf ("\n");
    }

  MPC_Comm_free (&comm);
}

int
main (int argc, char **argv)
{

  MPC_Init (&argc, &argv);



  init ();

  MPC_Finalize ();
  return 0;
}
