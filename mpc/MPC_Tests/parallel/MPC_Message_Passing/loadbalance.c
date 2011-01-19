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

static int load_enable = 1;

static double
d_min (double a, double b)
{
  if (a < b)
    {
      return a;
    }
  else
    {
      return b;
    }
}
static double
d_max (double a, double b)
{
  if (a < b)
    {
      return b;
    }
  else
    {
      return a;
    }
}

static void
loadbalance (int nb_cpu, MPC_Activity_t * tab, int vp, int process)
{
  double process_activity;
  int i;
  double min_val;
  double max_val;
  int min_i;
  int max_i;

  MPC_Get_activity (nb_cpu, tab, &process_activity);

  min_val = tab[0].usage;
  max_val = tab[0].usage;
  min_i = 0;
  max_i = 0;

  for (i = 0; i < nb_cpu; i++)
    {
      min_val = d_min (min_val, tab[i].usage);
      if (min_val == tab[i].usage)
	min_i = i;
      max_val = d_max (min_val, tab[i].usage);
      if (max_val == tab[i].usage)
	max_i = i;
    }

  if (tab[vp].usage > process_activity)
    {
      if (min_i != vp)
	{
	  int rank;
	  MPC_Comm_rank (MPC_COMM_WORLD, &rank);
	  if (load_enable == 1)
	    {
	      fprintf (stderr,
		       "%d from process %d cpuid %d %f move to process %d cpuid %d %f (process %f)\n",
		       rank, process, vp, tab[vp].usage, process, min_i,
		       tab[min_i].usage, process_activity);
	      MPC_Move_to (process, min_i);
	    }
	}
      else
	{
	  fprintf (stderr, "Already on %d->%d %f->%f\n", vp, min_i,
		   tab[vp].usage, tab[min_i].usage);
	}
    }
}

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
  char *loadbalance_env;

  loadbalance_env = getenv ("MPC_TEST_SILENCE");

  MPC_Comm_rank (MPC_COMM_WORLD, &rank);
  MPC_Comm_size (MPC_COMM_WORLD, &size);

  if (loadbalance_env != NULL)
    {
      load_enable = 0;
    }

  MPC_Processor_rank (&vp);
  MPC_Processor_number (&nb_cpu);

  MPC_Process_rank (&process);
  MPC_Process_number (&nb_process);

  gethostname (name, 1023);
  tmp_buf = malloc (1024 * 1024);
  tmp_buf[0] = '\0';

  sprintf (&(tmp_buf[strlen (tmp_buf)]),
	   "Task %d/%d cpu %d/%d process %d/%d\n", rank, size, vp, nb_cpu,
	   process, nb_process);
  tab = malloc (nb_cpu * sizeof (MPC_Activity_t));

  for (i = 1; i < 30; i++)
    {
      for (j = 0; j < (5000000 / size); j++)
	{
	  res += i;
	  res = (sqrt (res) + (double) j) / ((double) i);
	}
      loadbalance (nb_cpu, tab, vp, process);
      MPC_Processor_rank (&vp);
      MPC_Isend (&res, 1, MPC_DOUBLE, (rank + 1) % size, 0, MPC_COMM_WORLD,
		 NULL);
      MPC_Irecv (&dist_res, 1, MPC_DOUBLE, (rank - 1 + size) % size, 0,
		 MPC_COMM_WORLD, NULL);
      MPC_Wait_pending (MPC_COMM_WORLD);
      MPC_Barrier (MPC_COMM_WORLD);
      res += dist_res;
      if (rank == 0)
	{
	  int k;
	  fprintf (stderr, "Step %ld\n", i);
	  MPC_Get_activity (nb_cpu, tab, &process_activity);
	  fprintf (stderr, "Activity (%s) %f:\n", name, process_activity);
	  for (k = 0; k < nb_cpu; k++)
	    {
	      fprintf (stderr, "\t- cpu %d activity %f\n",
		       tab[k].virtual_cpuid, tab[k].usage);
	    }
	}
    }
  sprintf (&(tmp_buf[strlen (tmp_buf)]), "Result %f dist %f\n",
	   res - dist_res, dist_res);
  MPC_Get_activity (nb_cpu, tab, &process_activity);

  printf ("%s", tmp_buf);
  return 0;
}
