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
#include "mpc_common_rank.h"
#include "mpc_thread_accessor.h"
#include "mpc_threads_config.h"
#include <stdio.h>
#include <mpc.h>
#include <mpi.h>


static int *_is_Master;

void
init ()
{
  static volatile int done = 0;
  static mpc_thread_mutex_t lock;
  int is_master = 0;
  int process_rank;
  int root, RANK, i;
  mpc_lowcomm_communicator_t comm;
  static int *temp;


  int size;
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  mpc_thread_mutex_lock (&lock);
  if (done == 0)
    {
      done = 1;
      is_master = 1;
      MPI_Comm_rank (MPI_COMM_WORLD, &root);
      _is_Master = (int *) calloc (size, sizeof (int));
      temp = (int *) calloc (size, sizeof (int));
      printf (" root = %d\n", root);
    }
  mpc_thread_mutex_unlock (&lock);

  MPI_Comm_rank (MPI_COMM_WORLD, &RANK);
  process_rank = mpc_common_get_process_rank();
  printf (" RANK = %d process_rank = %d\n", RANK, process_rank);
  MPI_Comm_split (MPI_COMM_WORLD, is_master, process_rank, &comm);
  printf ("end of MPI_Comm_split %d \n", RANK);

  if (is_master == 1)
    {
      int rank;

      MPI_Comm_rank (comm, &rank);
      if (rank == 0)
	{
	  if (rank == 0)
	    temp[RANK] = -1;
	  else
	    temp[RANK] = 1;
	}
    }
  MPI_Reduce (temp, _is_Master, size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Bcast (_is_Master, size, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Barrier (MPI_COMM_WORLD);
  if (RANK == 0)
    {
      for (i = 0; i < size; i++)
	printf (" ===> %d %d\n", i, _is_Master[i]);
      printf ("\n");
    }

  MPI_Comm_free (&comm);
}

int
main (int argc, char **argv)
{

  MPI_Init (&argc, &argv);



  init ();

  MPI_Finalize ();
  return 0;
}
