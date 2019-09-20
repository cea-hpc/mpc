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
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_COMMON_INCLUDE_SCTK_TOPOLOGY_H_
#define MPC_COMMON_INCLUDE_SCTK_TOPOLOGY_H_

#include <stdio.h>

#include <hwloc.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Hwloc topology accessors */

hwloc_topology_t mpc_common_topology_get();
hwloc_topology_t mpc_common_topology_full();


/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void mpc_common_topo_get_cpu_neighborhood(int cpuid, unsigned int nb_cpus, int *neighborhood);


/*! \brief return the Numa node associated with a given CPUID
 * @param cpuid The target CPUID (logical)
 * @return identifier of the numa node matching cpuid (0 if none)
 */
int mpc_common_topo_get_numa_node_from_cpu(const int cpuid);


/*! \brief Return the number of NUMA nodes */
int mpc_common_topo_get_numa_node_count(void);

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise */
int mpc_common_topo_has_numa_nodes(void);

/*! \brief Return the total number of core for the process
*/
int mpc_common_topo_get_cpu_count(void);

/*! \brief Bind the current thread
 * @ param i The cpu_id to bind
*/
int mpc_common_topo_bind_to_cpu(int i);

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
*/
void mpc_common_topo_print(FILE *fd);

/*! \brief get the number of processing units (PU) as seen by MPC
 * @return Number of PUs in curent MPC topology
 */
int mpc_common_topo_get_pu_count(void);

/*! \brief Return the current core_id
*/
int mpc_common_topo_get_current_cpu(void);

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



/*! \brief Return the Socket ID for current CPU
 */
int sctk_topology_get_socket_id(int os_level);

/*! \brief Return the Number of sockets
 */
int sctk_topology_get_socket_number();


/*! \brief Return the total number of core for the process (from a topology)
*/
int topo_get_cpu_count(hwloc_topology_t target_topo);


/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise (from a
 * topology)
*/
int topo_has_numa_nodes(hwloc_topology_t target_topo);

/*! \brief Return the total number of numa nodes (from a topology)
*/
int topo_get_numa_node_count(hwloc_topology_t target_topo);

/*! \brief Print a given topology tree into a file
 * @param target_topology the topology to print
 * @param fd Destination file descriptor
*/
void topo_print(hwloc_topology_t target_topology, FILE *fd);


/*! \brief returns the number of HT on each core
  @param cpuid target core
  @return number of HT in the given core
*/
int topo_get_pu_per_core_count(hwloc_topology_t target_topo, int cpuid);

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
*/
int topo_get_first_cpu_in_numa_node(hwloc_topology_t target_topo, int node);

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
char *sctk_get_processor_name(void);

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used for ethread
*/
int sctk_set_cpu_number(int n);

/*! \brief Number of processing units per core
*/
int mpc_common_topo_get_ht_per_core(void);


/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void topo_get_cpu_neighborhood(hwloc_topology_t topo, int cpuid,
                                    unsigned int nb_cpus, int *neighborhood);



/*! \brief Return the NUMA node according to the core_id number
 * @param cpuid Cpu which NUMA has to be queried
 */
int topo_get_numa_node_from_cpu(hwloc_topology_t target_topo, const int cpuid);

/*! \brief Return the hwloc topology object
*/
hwloc_topology_t mpc_common_topology_get(void);

void mpc_common_topo_bind_to_process_cpuset();

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
int topo_get_distance_from_pu(hwloc_topology_t topology, int source_pu, hwloc_obj_t target_obj);

void mpc_common_topo_clear_cpu_pinning_cache();

int mpc_common_topo_get_pu_count();

/* used by option graphic */
void create_placement_rendering(int pu, int master_pu,int task_id);

/* used by option text */
void create_placement_text(int os_pu, int os_master_pu, int task_id, int vp, int rank_open_mp, int* min_idex, int pid);

/* Get the os index from the topology_compute_node where the current thread is binding */
int sctk_get_cpu_compute_node_topology();

/* Get the logical index from the os one from the topology_compute_node */
int sctk_get_logical_from_os_compute_node_topology(unsigned int cpu_os);

/* Get the os index from the logical one from the topology_compute_node */
int sctk_get_cpu_compute_node_topology_from_logical( int logical_pu);

#ifdef __cplusplus
}
#endif
#endif /* MPC_COMMON_INCLUDE_SCTK_TOPOLOGY_H_ */
