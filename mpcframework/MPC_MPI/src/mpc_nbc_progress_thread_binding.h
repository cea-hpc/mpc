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
#include <stdio.h>
#include <stdlib.h>
#include <sctk_topology.h>
#include <sctk_debug.h>
#include <sctk_accessor.h>
#include <sctk_thread.h>

//This enumeration describes how to bind progress threads on one node of 8 cores with 2 numa nodes
//
//SCTK_PROGRESS_THREAD_BINDING_BIND
// / / / /   / / / / Polling threads
// | | | |   | | | | MPI task threads
// o o o o   o o o o cores
//
//SCTK_PROGRESS_THREAD_BINDING_SMART
// | / | /   | / | /
// o o o o   o o o o
//
//SCTK_PROGRESS_THREAD_BINDING_OVERWEIGHT
//   /              
//   /              
//   /              
//   /              
//   /              
//   /              
// | / | |  | | | | 
// o o o o  o o o o 
//
//SCTK_PROGRESS_THREAD_BINDING_NUMA
//       /         /
//       /         /
// | | | /   | | | /
// o o o o   o o o o 
//
typedef enum sctk_progress_thread_binding_e{
   SCTK_PROGRESS_THREAD_BINDING_BIND,
   SCTK_PROGRESS_THREAD_BINDING_SMART,
   SCTK_PROGRESS_THREAD_BINDING_OVERWEIGHT,
   SCTK_PROGRESS_THREAD_BINDING_NUMA
}sctk_progress_thread_binding_t;

//function used by the function sctk_thread_attr_setbindings in file mpc_nbc.c to
//define bindings of progress threads
int sctk_set_progress_thread_binding(sctk_progress_thread_binding_t binding);


#endif // __MPC_NBC_PROGRESS_THREAD_BINDING_H__
