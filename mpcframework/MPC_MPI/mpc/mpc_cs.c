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
#include <sys/time.h>
#include <mpc.h>
#include <sctk_debug.h>

/*
  This file contain the C# binding of MPC (deprecated)
*/

#if 0
int
MPC_init (void)
{
  int n;
  MPC_Comm_rank (MPC_COMM_WORLD, &n);
  return n;
}

int
MPC_comm_size (void)
{
  int n;
  MPC_Comm_size (MPC_COMM_WORLD, &n);
  return n;
}

int
MPC_get_task_count (void)
{
  int n;
  MPC_Comm_rank (MPC_COMM_WORLD, &n);
  return n;
}


void
MPC_barrier (void)
{
  sctk_nodebug ("barrier");
  MPC_Barrier (MPC_COMM_WORLD);
}

void
MPC_wait (void)
{
  sctk_nodebug ("wait");
  MPC_Wait_pending (MPC_COMM_WORLD);
}

void
MPC_send (int dest, double DATA[], int start, int count)
{
  sctk_nodebug ("send : %d", dest);
  MPC_Isend (DATA + start, count, MPC_DOUBLE, dest, 0, MPC_COMM_WORLD, NULL);
}

void
MPC_receive (int src, double DATA[], int start, int count)
{
  sctk_nodebug ("rcv : %d", src);
  MPC_Irecv (DATA + start, count, MPC_DOUBLE, src, 0, MPC_COMM_WORLD, NULL);
}

void
MPC_allreduce_min (double DATAIN[], double DATAOUT[], int start_in,
		   int start_out, int count)
{
  sctk_nodebug ("all reduce ");
  MPC_Allreduce (DATAIN + start_in, DATAOUT + start_out, count,
		 MPC_DOUBLE, MPC_MIN, MPC_COMM_WORLD);
}

void
MPC_allreduce_max (double DATAIN[], double DATAOUT[], int start_in,
		   int start_out, int count)
{
  sctk_nodebug ("all reduce ");
  MPC_Allreduce (DATAIN + start_in, DATAOUT + start_out, count,
		 MPC_DOUBLE, MPC_MAX, MPC_COMM_WORLD);
}

void
MPC_allreduce_sum (double DATAIN[], double DATAOUT[], int start_in,
		   int start_out, int count)
{
  sctk_nodebug ("all reduce ");
  MPC_Allreduce (DATAIN + start_in, DATAOUT + start_out, count,
		 MPC_DOUBLE, MPC_SUM, MPC_COMM_WORLD);
}

void
MPC_mpc_gettimeofday (int timestruct[])
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  timestruct[0] = tv.tv_sec;
  timestruct[1] = tv.tv_usec;
}

void
MPC_end (void)
{

}
#endif
