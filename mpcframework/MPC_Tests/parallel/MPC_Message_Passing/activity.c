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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int
main (int argc, char **argv)
{
  MPC_Activity_t *tab;
  int rank;
  int size;
  int vp;
  int nb_cpu;
  int process;
  int nb_process;
  long i;
  long j;
  char name[1024];
  char *tmp_buf;
  double res = 0;
  double dist_res = 0;
  double process_activity;

  MPC_Comm_rank (SCTK_COMM_WORLD, &rank);
  MPC_Comm_size (SCTK_COMM_WORLD, &size);

  MPC_Processor_rank (&vp);
  MPC_Processor_number (&nb_cpu);

  MPC_Process_rank (&process);
  MPC_Process_number (&nb_process);

  gethostname (name, 1023);
  tmp_buf = malloc (1024 * 1024);

  sprintf (tmp_buf, "Task %d/%d cpu %d/%d process %d/%d\n", rank, size, vp,
	   nb_cpu, process, nb_process);
  tab = malloc (nb_cpu * sizeof (MPC_Activity_t));

  MPC_Get_activity (nb_cpu, tab, &process_activity);
  sprintf (&(tmp_buf[strlen (tmp_buf)]), "Activity (%s) %f:\n", name,
	   process_activity);
  for (i = 0; i < nb_cpu; i++)
    {
      sprintf (&(tmp_buf[strlen (tmp_buf)]), "\t- cpu %d activity %f\n",
	       tab[i].virtual_cpuid, tab[i].usage);
    }

  for (i = 1; i < 3; i++)
    {
      for (j = 0; j < (500 / size); j++)
	{
	  res += i;
	  res = (sqrt (res) + (double) j) / ((double) i);
	}
      MPC_Isend (&res, 1, MPC_DOUBLE, (rank + 1) % size, 0, SCTK_COMM_WORLD,
		 NULL);
      MPC_Irecv (&dist_res, 1, MPC_DOUBLE, (rank - 1 + size) % size, 0,
		 SCTK_COMM_WORLD, NULL);
      MPC_Wait_pending (SCTK_COMM_WORLD);
      res += dist_res;
    }
  sprintf (&(tmp_buf[strlen (tmp_buf)]), "Result %f dist %f\n",
	   res - dist_res, dist_res);

  MPC_Get_activity (nb_cpu, tab, &process_activity);
  sprintf (&(tmp_buf[strlen (tmp_buf)]), "Activity (%s) %f:\n", name,
	   process_activity);
  for (i = 0; i < nb_cpu; i++)
    {
      sprintf (&(tmp_buf[strlen (tmp_buf)]), "\t- cpu %d activity %f\n",
	       tab[i].virtual_cpuid, tab[i].usage);
    }

  printf ("%s", tmp_buf);
  return 0;
}
