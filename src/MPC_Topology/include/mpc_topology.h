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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_COMMON_INCLUDE_MPC_TOPOLOGY_H_
#define MPC_COMMON_INCLUDE_MPC_TOPOLOGY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <hwloc.h>


/**
 * @addtogroup MPC_Topology
 * @{
 */

/****************
 * FILE CONTENT *
 ****************/

/**
 * @defgroup topology_interface General Topology Interface
 * This module provides the general topology getters for MPC
 */

        /**
         * @defgroup topology_interface_init Init and Release
         * These functions are the initalization and release points for
         * the topology interface
         */

        /**
         * @defgroup topology_interface_getters Topology query functions
         * These functions are used to retrieve topology for current process in MPC
         */

/**************************
 * MPC TOPOLOGY INTERFACE *
 **************************/

/**
 * @addtogroup topology_interface
 * @{
 */

/********************
 * INIT AND RELEASE *
 ********************/

/**
 * @addtogroup topology_interface_init
 * @{
 */

/** @brief Initialize the topology module
*/
void mpc_topology_init(void);

/** @brief Destroy the topology module
*/
void mpc_topology_destroy(void);

/*
   ! ALL the following functions are dependent
   ! from a previous call to the initialization
   ! of the topology module
*/

/* End topology_interface_init */
/**
 * @}
 */

/**************************
 * MPC TOPOLOGY ACCESSORS *
 **************************/

/**
 * @addtogroup topology_interface_getters
 * @{
 */

/**
 * @brief Retrieve the Main Topology Object from MPC
 *
 * @return hwloc_topology_t MPC's main topology object
 */
hwloc_topology_t mpc_topology_get(void);

/** @brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
*/
void mpc_topology_get_pu_neighborhood(int cpuid, unsigned int nb_cpus, int *neighborhood);

/** @brief return the Numa node associated with a given CPUID
 * @param cpuid The target CPUID (logical)
 * @return identifier of the numa node matching cpuid (0 if none)
 */
int mpc_topology_get_numa_node_from_cpu(const int cpuid);

/** @brief Return the number of NUMA nodes */
int mpc_topology_get_numa_node_count(void);

/** @brief Return 1 if the current node is a NUMA node, 0 otherwise */
int mpc_topology_has_numa_nodes(void);

/** @brief Bind the current thread
 * @ param i The cpu_id to bind
*/
int mpc_topology_bind_to_cpu(int i);

void mpc_topology_bind_to_process_cpuset();

/** @brief Print the topology tree into a file
 * @param fd Destination file descriptor
*/
void mpc_topology_print(FILE *fd);

/** @brief get the number of processing units (PU) as seen by MPC
 * @return Number of PUs in curent MPC topology
 */
int mpc_topology_get_pu_count(void);

/** @brief Return the current core_id
*/
int mpc_topology_get_current_cpu(void);

#ifndef MPC_Threads

/**
 * @brief Return the PU executing current thread
 * @note This version relies on HWLOC as MPC has no scheduler
 *
 * @return int current PU executing this thread
 */
static inline int mpc_topology_get_pu( void )
{
        return mpc_topology_get_current_cpu();
}

#else

int mpc_thread_get_pu(void);
#include "mpc_thread_accessor.h"

/**
 * @brief Return the PU executing current thread
 * @note This version relies on MPC scheduler
 *
 * @return int current PU executing this thread
 */
static inline int mpc_topology_get_pu( void )
{
        return mpc_thread_get_pu();
}

#endif


/**
 * @brief Convert a PU id from OS to logical numbering
 *
 * @param pu_os_id OS id of the source PU
 * @return int Logical ID of the PU (used in Hwloc)
 */
int mpc_topology_convert_os_pu_to_logical( int pu_os_id );

/**
 * @brief Convert a PU id from logical to OS numbering
 *
 * @param cpuid logical id of the source PU
 * @return int OS ID of the PU
 */
int mpc_topology_convert_logical_pu_to_os( int cpuid );


/**
 * @brief Return the overall cpuset of the MPC process
 *
 * @return hwloc_const_cpuset_t process cpuset
 */
hwloc_const_cpuset_t mpc_topology_get_process_cpuset();

/**
 * @brief Return the numa node (or whole machine if none) of cpuid
 *
 * @param cpuid PU id to query for
 * @return hwloc_cpuset_t corresponding NUMA cpuset
 */
hwloc_cpuset_t mpc_topology_get_parent_numa_cpuset(int cpuid);

/**
 * @brief Return the socket of cpuid
 *
 * @param cpuid PU id to query for
 * @return hwloc_cpuset_t corresponding socket cpuset
 */
hwloc_cpuset_t mpc_topology_get_parent_socket_cpuset(int cpuid);

/**
 * @brief Return the PU cupset of cpuid
 *
 * @param cpuid PU id to query for
 * @return hwloc_cpuset_t corresponding PU cpuset
 */
hwloc_cpuset_t mpc_topology_get_pu_cpuset(int cpuid);

/**
 * @brief Return the core cupset of cpuid
 *
 * @param cpuid PU id to query for
 * @return hwloc_cpuset_t corresponding core cpuset
 */
hwloc_cpuset_t mpc_topology_get_parent_core_cpuset(int cpuid);

/**
 * @brief Return the first PU in the given node level
 *
 * @param type Type of level to search for
 * @return hwloc_cpuset_t cpuset with a single core according to level
 */
hwloc_cpuset_t mpc_topology_get_first_pu_for_level(hwloc_obj_type_t type);

/**
 * @brief Number of active PU for the topology
 *
 * @return int number of active PUs
 */
int mpc_topology_get_pu_count();

/**
 * @brief Number of processing units per core (Hyperhtreads)
 *
 * @return int number of PU per CORE
 */
int mpc_topology_get_ht_per_core(void);

void mpc_topology_clear_cpu_pinning_cache();

/** @brief Set the number of core usable for the current process
 * @param n Number of cores used for ethread
*/
int mpc_topology_set_pu_count(int n);

/* End topology_interface_getters */
/**
 * @}
 */

/* End topology_interface */
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
#endif /* MPC_COMMON_INCLUDE_MPC_TOPOLOGY_H_ */
