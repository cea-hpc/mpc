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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <mpc.h>
#include <assert.h>


int
main (int argc, char **argv)
{
  int rank, size;
  char name[1024];
  MPC_Init (&argc, &argv);
  MPC_Comm_size (MPC_COMM_WORLD, &size);
  MPC_Comm_rank (MPC_COMM_WORLD, &rank);
  assert(size == atoi(argv[1]));
  assert(rank < size);
  gethostname (name, 1023);
  printf ("Hello world from process %d of %d %s\n", rank, size, name);
  MPC_Finalize ();
  return 0;
}
