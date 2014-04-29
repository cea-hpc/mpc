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
using System;
using System.Runtime.InteropServices;

public class MPC
{
  [DllImport ("libmpc_mono_cs.so")] public static extern int MPC_init ();

  [DllImport ("libmpc_mono_cs.so")] public static extern int MPC_comm_size ();

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_send (int dest, double[]DATA, int start,
				    int count);

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_receive (int src, double[]DATA, int start,
				       int count);

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_allreduce_min (double[]DATAIN, double[]DATAOUT,
					     int start_in, int start_out,
					     int count);

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_allreduce_max (double[]DATAIN, double[]DATAOUT,
					     int start_in, int start_out,
					     int count);

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_allreduce_sum (double[]DATAIN, double[]DATAOUT,
					     int start_in, int start_out,
					     int count);

  [DllImport ("libmpc_mono_cs.so")] public static extern void MPC_end ();

  [DllImport ("libmpc_mono_cs.so")] public static extern void MPC_wait ();


  [DllImport ("libmpc_mono_cs.so")] public static extern void MPC_barrier ();

  [DllImport ("libmpc_mono_cs.so")]
    public static extern void MPC_mpc_gettimeofday (int[]time);

}
