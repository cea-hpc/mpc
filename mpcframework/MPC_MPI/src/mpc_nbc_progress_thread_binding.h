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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPC_NBC_PROGRESS_THREAD_BINDING_H__
#define __MPC_NBC_PROGRESS_THREAD_BINDING_H__
#include <sctk_accessor.h>
#include <sctk_debug.h>
#include <sctk_thread.h>
#include <sctk_topology.h>
#include <stdio.h>
#include <stdlib.h>

// function pointed by 'sctk_get_progress_thread_binding' in file mpc_nbc.c to
// define progress threads binding

// progress threads binding on one node of 8 cores with 2 numa nodes
// BIND
// / / / /   / / / / Polling threads
// | | | |   | | | | MPI task threads
// o o o o   o o o o cores
int sctk_get_progress_thread_binding_bind(void);

// progress threads binding on one node of 8 cores with 2 numa nodes
// SMART
// | / | /   | / | /
// o o o o   o o o o
int sctk_get_progress_thread_binding_smart(void);

// progress threads binding on one node of 8 cores with 2 numa nodes
// OVERWEIGHT
//   /
//   /
//   /
//   /
//   /
//   /
// | / | |  | | | |
// o o o o  o o o o
int sctk_get_progress_thread_binding_overweight(void);

// progress threads binding on one node of 8 cores with 2 numa nodes
// NUMA
//       /         /
//       /         /
// | | | /   | | | /
// o o o o   o o o o
int sctk_get_progress_thread_binding_numa_iter(void);
int sctk_get_progress_thread_binding_numa(void);

#endif // __MPC_NBC_PROGRESS_THREAD_BINDING_H__
