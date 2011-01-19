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
#ifndef __SCTK_TOPOLOGY_H_
#define __SCTK_TOPOLOGY_H_

#include <stdio.h>
#include "sctk_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void sctk_topology_init (void);
  int sctk_get_cpu (void);
  int sctk_get_node_from_cpu (const int cpu);
  int sctk_get_cpu_number (void);
  int sctk_is_numa_node (void);
  void sctk_print_topology (FILE * fd);
  int sctk_bind_to_cpu (int i);
  int sctk_get_first_cpu_in_node (int node);
  char *sctk_get_processor_name (void);
  int sctk_set_cpu_number (int n);
  char *sctk_get_node_name (void);
  void sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood);

#ifdef __cplusplus
}
#endif
#endif
