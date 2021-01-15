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
/* #   - BOUHROUR Stephane stephane.bouhrour@exascale-computing.eu        # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_COMMON_SRC_TOPOLOGY_H_
#define MPC_COMMON_SRC_TOPOLOGY_H_

#include <stdio.h>
#include <hwloc.h>

#include <mpc_topology.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup MPC_Topology
 * @{
 */

/****************
 * FILE CONTENT *
 ****************/

/**
 * @defgroup local_topology_interface Module-level topology interface
 * Defines functions for topology used only in the MPC_Topology module
 */


/*********************************
 * MPC TOPOLOGY OBJECT INTERFACE *
 *********************************/

/**
 * @addtogroup local_topology_interface
 * @{
 */

/*
 * Note These functions can be used on independent HWLOC topologies
 */

/** @brief Return the Socket ID for current CPU
 */
int _mpc_topology_get_socket_id(hwloc_topology_t target_topo, int os_level);

/** @brief Return the Number of sockets
 */
int _mpc_topology_get_socket_count(hwloc_topology_t target_topo);

/** @brief Return the total number of core for the process (from a topology)
*/
int _mpc_topology_get_pu_count(hwloc_topology_t target_topo);

/** @brief Return 1 if the current node is a NUMA node, 0 otherwise (from a
 * topology)
*/
int _mpc_topology_has_numa_nodes(hwloc_topology_t target_topo);

/** @brief Return the total number of numa nodes (from a topology)
*/
int _mpc_topology_get_numa_node_count(hwloc_topology_t target_topo);

/** @brief Print a given topology tree into a file
 * @param target_topology the topology to print
 * @param fd Destination file descriptor
*/
void _mpc_topology_print(hwloc_topology_t target_topology, FILE *fd);

/** @brief returns the number of HT on each core
  @param cpuid target core
  @return number of HT in the given core
*/
int _mpc_topology_get_pu_per_core_count(hwloc_topology_t target_topo, int cpuid);

/** @brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
*/
int _mpc_topology_get_first_cpu_in_numa_node(hwloc_topology_t target_topo, int node);

/** @brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void _mpc_topology_get_pu_neighborhood(hwloc_topology_t topo, int cpuid,
                                    unsigned int nb_cpus, int *neighborhood);

/** @brief Return the NUMA node according to the core_id number
 * @param cpuid Cpu which NUMA has to be queried
 */
int _mpc_topology_get_numa_node_from_cpu(hwloc_topology_t target_topo, const int cpuid);

/** @brief Convert a PU id to a logical index
*/
int _mpc_topology_convert_os_pu_to_logical(hwloc_topology_t target_topo, int pu_os_id);

/** @brief Convert an OS PU to a logical index
*/
int _mpc_topology_convert_logical_pu_to_os(hwloc_topology_t target_topo, int cpuid);

/** @brief Returns the complete allowed cpu set for this process
*/
hwloc_const_cpuset_t _mpc_topology_get_process_cpuset();

/** @brief Return the cpuset of the node containing the PU with pu_id
 * if not numa it behaves like  _mpc_topology_get_process_cpuset
*/
hwloc_cpuset_t _mpc_topology_get_parent_numa_cpuset(hwloc_topology_t target_topo, int cpuid);

/** @brief Return the cpuset of the socket containing the PU with pu_id
*/
hwloc_cpuset_t _mpc_topology_get_parent_socket_cpuset(hwloc_topology_t target_topo, int cpuid);

/** @brief Return the cpuset of the PU with pu_id
*/
hwloc_cpuset_t _mpc_topology_get_pu_cpuset(hwloc_topology_t target_topo, int cpuid);

/** @brief Return the cpuset of the CORE hosting PU with pu_id
*/
hwloc_cpuset_t _mpc_topology_get_parent_core_cpuset(hwloc_topology_t target_topo, int cpuid);

/** @brief Retrieve a CPUSET with the first core for each item of level
 * NUMA/MACHINE/SOCKET
*/
hwloc_cpuset_t _mpc_topology_get_first_pu_for_level(hwloc_topology_t topology, hwloc_obj_type_t type);

/** @brief Compute the distance between a given object and a PU
*/
int _mpc_topology_get_distance_from_pu(hwloc_topology_t topology, int source_pu, hwloc_obj_t target_obj);

#if (HWLOC_API_VERSION >= 0x00020000)
/** @brief Detect if MCDRAM memory bank is available and save MCDRAM node identifier
*/
void _mpc_topology_mcdram_detection(hwloc_topology_t topology);

/** @brief Detect if the system is equiped with NVDIMM storage and save OS device identifiers
*/
void _mpc_topology_nvdimm_detection(hwloc_topology_t topology);
#endif

/* End local_topology_interface */
/**
 * @}
 */

/* End MPC_Topology */
/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* MPC_COMMON_SRC_TOPOLOGY_H_ */
