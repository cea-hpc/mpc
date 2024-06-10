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
/* #   - VALAT Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_COMMON_INCLUDE_MPC_COMMON_DEVICE_TOPOLOGY_H_
#define MPC_COMMON_INCLUDE_MPC_COMMON_DEVICE_TOPOLOGY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hwloc.h>
#include <mpc_common_spinlock.h>

/**
 * @addtogroup MPC_Topology
 * @{
 */

/****************
 * FILE CONTENT *
 ****************/

/**
 * @defgroup topology_device_interface Device topological query interface
 * This modules provides all the functions to locate and query devices with
 * respect to the topology of current process
 */

/*********************************
 * MPC DEVICE TOPOLOGY INTERFACE *
 *********************************/

/**
 * @addtogroup topology_device_interface
 * @{
 */

/************************************************************************/
/* ENUM DEFINITION                                                      */
/************************************************************************/

/** Here we follow the HWloc nomenclature in
 * order to identify object classes this is
 * just for readability */
typedef enum
{
	MPC_TOPO_DEVICE_BLOCK = HWLOC_OBJ_OSDEV_BLOCK,		   /**< IO block device (hard-disk) */
	MPC_TOPO_DEVICE_NETWORK_HANDLE = HWLOC_OBJ_OSDEV_NETWORK,  /**< Network device (OS level) */
	MPC_TOPO_DEVICE_NETWORK_OFA = HWLOC_OBJ_OSDEV_OPENFABRICS, /**< An OpenFabric HCA */
	MPC_TOPO_DEVICE_GPU = HWLOC_OBJ_OSDEV_GPU,		   /**< A GPU */
	MPC_TOPO_DEVICE_COPROCESSOR = HWLOC_OBJ_OSDEV_COPROC,	   /**< A coprocessor (CUDA, OpenCL ...) */
	MPC_TOPO_DEVICE_DMA = HWLOC_OBJ_OSDEV_DMA,		   /**< A DMA engine */
	MPC_TOPO_DEVICE_UKNOWN = 99999				   /**< Something we did not get */
} mpc_topology_device_type_t;

/**
 * @brief Convert a topology device type to string
 *
 * @param type type to convert
 * @return const char* corresponding string
 */
const char *mpc_topology_device_type_to_char( mpc_topology_device_type_t type );

/**
 * @brief Convert an HWLOC type to an MPC type
 *
 * @param type hwloc type to be converted
 * @return mpc_topology_device_type_t MPC handled type
 */
mpc_topology_device_type_t mpc_topology_device_type_from_hwloc(hwloc_obj_osdev_type_t type);

/** This enum define the type of binding of a given device */
typedef enum
{
	MPC_TOPO_MACHINE_LEVEL_DEVICE,/**< Case where parent is
                                           the whole machine (or something not handled) */
	MPC_TOPO_TOPOLOGICAL_DEVICE   /**< Case where parent is a numa node */
} mpc_topology_device_container_t;

/**
 * @brief Convert a container type to string
 *
 * @param type container type to convert
 * @return const char* corresponding string
 */
const char *mpc_topology_device_container_to_char( mpc_topology_device_container_t type );

/************************************************************************/
/* DEVICE DEFINITION                                                    */
/************************************************************************/

/** This is the container defining a device */
typedef struct sctk_device_s
{
	/* Descriptor */
	int id;					   /**< ID of the device (offset in the device array) */
	mpc_topology_device_type_t type;	   /**< This is the type of the OSdev pointed to */
	mpc_topology_device_container_t container; /**< This is the topological level of the device */
	char *name;				   /**< Name of the device (called OSname in HWloc) */

	/* Hwloc REF */
	hwloc_obj_t obj;			   /**< This is the actual Hwloc object (PCI) for this device */
	hwloc_obj_t non_io_parent_obj;             /**< This is the first non IO parent
                                                        of the object (defining locality) */

	/* Topology */
	hwloc_cpuset_t cpuset;   /**< This is the CPUset of the parent non IO */
	hwloc_nodeset_t nodeset; /**< This is the NODEset of the parent non IO */

	int root_core; /**< This is the master core for this NUMA node */
	int root_numa; /**< This is the master NUMA node for this device */

	/* Attributes */
	const char *vendor; /**< PCI card vendor */
	const char *device; /**< PCI card descriptor */

	int device_id; /**< The internal ID of the device (for IB, card ID) set in enrich topology */

	size_t nb_res;	/**< How many times this device has been associated to a
                          resources (meaning loading level) */
	mpc_common_spinlock_t res_lock; /** lock on previous field to ensure thread safety */
} mpc_topology_device_t;

/************************************************************************/
/* MAIN INTERFACE                                                       */
/************************************************************************/

/*****************
 * DEVICE GETTER *
 *****************/

/**
 * @brief Retrieve a device descriptor from it os handle (for example eth0)
 *
 * @param handle name of the device handle to search for
 * @return mpc_topology_device_t* a pointer to a device descriptor (not to free)
 */
mpc_topology_device_t *mpc_topology_device_get_from_handle( char *handle );

/**
 * @brief Retrieve a device identifier from it os handle (for example eth0)
 *
 * @param handle name of the device handle to search for
 * @return int identifier of the corresponding device
 */
int mpc_topology_device_get_id_from_handle( char *handle );

/**
 * @brief Retrieve the list of devices matching a given regexp (on OSname)
 *
 * @param handle_reg_exp Regular expression to use for OSname handle search
 * @param count [out] number of devices found
 * @return mpc_topology_device_t** array of device descriptors (to be freed)
 */
mpc_topology_device_t **mpc_topology_device_get_from_handle_regexp( char *handle_reg_exp,
        int *count );

/**
 * @brief Print the content of a device as seen from MPC
 *
 * @param dev device to print information of
 */
void mpc_topology_device_print( mpc_topology_device_t *dev );

/**
 * @brief Get the closest device from PU matching the regexp "matching_regexp"
 *
 * @param pu_id PU to search a device for
 * @param matching_regexp handle_reg_exp the regular expression as a string format
 * @return mpc_topology_device_t* the closest device matching the regexpr for pu_id (not for free)
 */
mpc_topology_device_t *mpc_topology_device_get_closest_from_pu( int pu_id, char *matching_regexp );

/**
 * @brief  Get the list of closest device from PU matching the regexp "matching_regexp"
 *
 * @param pu_id  PU to search a device for
 * @param matching_regexp handle_reg_exp the regular expression as a string format
 * @param count [out] number of devices returner
 * @return mpc_topology_device_t** an array of the closest devices (count elem) (to free)
 */
mpc_topology_device_t **mpc_topology_device_matrix_get_list_closest_from_pu( int pu_id,
        char *matching_regexp,
        int *count );

/**
 * @brief Return 1 if the devices matching the regexp are equidistant
 *
 * @param matching_regexp device to check for iso-distance
 * @return int return true if device is palcement indiferent
 */
int mpc_topology_device_matrix_is_equidistant( char *matching_regexp );

/**
 * @brief Register as using the device
 * Increment the number of associated resource by 1
 * @param[in] device the device to update
 */
void mpc_topology_device_attach_resource( mpc_topology_device_t *device );

/**
 * @brief Notify as not using the device anymore
 * Decrement the number of associated resource by 1
 * @param[in] device the device to update
 */
void mpc_topology_device_detach_resource( mpc_topology_device_t *device );

/**
 * @brief Retrieve the least loaded device from a pool
 * Retrieve the device with the smallest number of associated resources from a
 * pool of device.
 *
 * To avoid unlocking the elected device between the search and the increment,
 * we register the
 * currently elected driver in freest_elem. In the case where this driver is
 * replaced by a better
 * one, we free the lock for the previous driver and lock the new ones.
 *
 * This is thread-safe with multiple calls to
 * mpc_topology_device_attach_freest_device_from() and other
 * calls like attach/detach() routines.
 *
 * @param[in] device_list the list of devices where we will have to look for.
 * @param[in] count the list size
 *
 * @return
 *   - NULL if count is lower than 1
 *   - A pointer to the freest device from the list
 */
mpc_topology_device_t *
mpc_topology_device_attach_freest_device_from( mpc_topology_device_t **device_list, int count );

/**
 * @brief Retrieve the device with the least associated resource using a regexpr
 * Retrieve the device with the smallest number of associated resources for a
 * matching regexp.
 *
 * @param[in] handle_reg_exp the regular expression as a string format
 * @return
 *   - NULL if the regexp is not found
 *   - A pointer to the elected device.
 */
mpc_topology_device_t *mpc_topology_device_attach_freest_device( char *handle_reg_exp );

/* End topology_device_interface */
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

#endif /* MPC_COMMON_INCLUDE_MPC_COMMON_DEVICE_TOPOLOGY_H_ */
