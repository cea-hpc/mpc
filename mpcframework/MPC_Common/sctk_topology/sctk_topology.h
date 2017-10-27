/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_TOPOLOGY_H_
#define __SCTK_TOPOLOGY_H_

#include <stdio.h>

#include "hwloc.h"
#ifdef MPC_USE_INFINIBAND
#include <infiniband/verbs.h>
#include <hwloc/openfabrics-verbs.h>
#endif

//TODO ifdef OPTION_GRAPHIC
char file_placement[128];
char placement_txt[128];
#ifdef __cplusplus
extern "C" {
#endif

/*
  Numbering rules

  Core_id are renumbered according to a topological numbering. All threads
  sharing a
  cache level must have continuous numbers.
*/

/*! \brief Initialize the topology module
*/
void sctk_topology_init(void);

/*! \brief Destroy the topology module
*/
void sctk_topology_destroy(void);

/*! \brief Return the current core_id
*/
int sctk_get_cpu(void);

/*! \brief Return the Socket ID for current CPU
 */
int sctk_topology_get_socket_id(int os_level);

/*! \brief Return the Number of sockets
 */
int sctk_topology_get_socket_number();

/*! \brief Return the total number of core for the process
*/
int sctk_get_cpu_number(void);

/*! \brief Return the total number of core for the process (from a topology)
*/
int sctk_get_cpu_number_topology(hwloc_topology_t topo);

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
int sctk_is_numa_node(void);

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise (from a
 * topology)
*/
int sctk_is_numa_node_topology(hwloc_topology_t topo);

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
*/
void sctk_print_topology(FILE *fd);

/*! \brief Print a given topology tree into a file
 * @param fd Destination file descriptor
 * @param topology the topology to print
*/
void sctk_print_specific_topology(FILE *fd, hwloc_topology_t topology);

/*! \brief Bind the current thread
 * @ param i The cpu_id to bind
*/
int sctk_bind_to_cpu(int i);

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
*/
int sctk_get_first_cpu_in_node(int node);

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
char *sctk_get_processor_name(void);

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used for ethread
*/
int sctk_set_cpu_number(int n);

/*! \brief Return the hostname
*/
char *sctk_get_node_name(void);

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void sctk_get_neighborhood(int cpuid, int nb_cpus, int *neighborhood);

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void sctk_get_neighborhood_topology(hwloc_topology_t topo, int cpuid,
                                    int nb_cpus, int *neighborhood);

/*! \brief Return the number of NUMA nodes
*/
int sctk_get_numa_node_number(void);

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
*/
int sctk_get_node_from_cpu(const int vp);

/*! \brief Return the PHYSICAL NUMA node according to the code_id number
 * @param vp VP
*/
int sctk_get_physical_node_from_cpu(const int vp);

/*! \brief Return the NUMA node for current CPU
*/
static inline int sctk_get_numa_node(int os_level) {
  if (!sctk_is_numa_node()) {
    return -1;
  }

  int cpu = sctk_get_cpu();

  if (0 <= cpu) {
    if (os_level)
      return sctk_get_physical_node_from_cpu(cpu);
    else
      return sctk_get_node_from_cpu(cpu);
  }

  return -1;
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
*/
int sctk_get_node_from_cpu_topology(hwloc_topology_t topo, const int vp);

/*! \brief Return the hwloc topology object
*/
hwloc_topology_t sctk_get_topology_object(void);

void sctk_restrict_binding();

/* Called only by __mpcomp_buid_tree */
int sctk_get_global_index_from_cpu(hwloc_topology_t topo, const int vp);

/*
 *  * Restrict the topology object of the current mpi task to 'nb_mvps' vps.
 *   * This function handles multiple PUs per core.
 *    */
int sctk_restrict_topology_for_mpcomp(hwloc_topology_t *restrictedTopology,
                                      int nb_mvps);

#ifdef MPC_USE_INFINIBAND
#include <infiniband/verbs.h>
/*! \brief Return if the core_id is close from the core_id. If an error occurs,
 * we get -1
*/
int sctk_topology_is_ib_device_close_from_cpu(struct ibv_device *dev,
                                              int core_id);
#endif

/*! \brief Convert a PU id to a logical index
*/
int sctk_topology_convert_os_pu_to_logical(int pu_os_id);

/*! \brief Convert an OS PU to a logical index
*/
int sctk_topology_convert_logical_pu_to_os(int pu_id);

/*! \brief Returns the complete allowed cpu set for this process
*/
hwloc_const_cpuset_t sctk_topology_get_machine_cpuset();

/*! \brief Return the cpuset of the node containing the PU with pu_id
 * if not numa it behaves like  sctk_topology_get_machine_cpuset
*/
hwloc_const_cpuset_t sctk_topology_get_numa_cpuset(int pu_id);

/*! \brief Return the cpuset of the socket containing the PU with pu_id
*/
hwloc_cpuset_t sctk_topology_get_socket_cpuset(int pu_id);

/*! \brief Return the cpuset of the PU with pu_id
*/
hwloc_cpuset_t sctk_topology_get_pu_cpuset(int pu_id);

/*! \brief Return the cpuset of the CORE hosting PU with pu_id
*/
hwloc_cpuset_t sctk_topology_get_core_cpuset(int pu_id);

/*! \brief Retrieve a CPUSET with the first core for each item of level
 * NUMA/MACHINE/SOCKET
*/
hwloc_cpuset_t sctk_topology_get_roots_for_level(hwloc_obj_type_t type);

/*! \brief Compute the distance between a given object and a PU
*/
int sctk_topology_distance_from_pu(int source_pu, hwloc_obj_t target_obj);

void sctk_topology_init_cpu();

int sctk_get_pu_number();

/* used by option graphic */
void create_placement_rendering(int pu, int master_pu,int task_id, int vp, int rank_open_mp, int *min_idex, int pid);

/* Get the os index from the topology_compute_node where the current thread is binding */
int sctk_get_cpu_compute_node_topology();

/* Get the logical index from the os one from the topology_compute_node */
int sctk_get_logical_from_os_compute_node_topology(int cpu_os);

/* Get the os index from the logical one from the topology_compute_node */
int sctk_get_cpu_compute_node_topology_from_logical( int logical_pu);
#ifdef __cplusplus
}
#endif
#endif
