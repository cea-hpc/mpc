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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_DEVICE_TOPOLOGY_H
#define SCTK_DEVICE_TOPOLOGY_H

#include <hwloc.h>


/************************************************************************/
/* ENUM DEFINITION                                                      */
/************************************************************************/

/** Here we follow the HWloc nomenclature in
 * order to identify object classes this is
 * just for readability */
typedef enum
{
	/** IO block device (hard-disk) */
	SCTK_DEVICE_BLOCK = HWLOC_OBJ_OSDEV_BLOCK,
	/** Network device (OS level) */
	SCTK_DEVICE_NETWORK_HANDLE = HWLOC_OBJ_OSDEV_NETWORK,
	/** An OpenFabric HCA */
	SCTK_DEVICE_NETWORK_OFA = HWLOC_OBJ_OSDEV_OPENFABRICS,
	/** A GPU */
	SCTK_DEVICE_GPU = HWLOC_OBJ_OSDEV_GPU,
	/** A coprocessor (CUDA, OpenCL ...) */
	SCTK_DEVICE_COPROCESSOR = HWLOC_OBJ_OSDEV_COPROC,
	/** A DMA engine */
	SCTK_DEVICE_DMA = HWLOC_OBJ_OSDEV_DMA,
	/** Something we did not get */
	SCTK_DEVICE_UKNOWN = 99999999
}sctk_device_type_t;

const char * sctk_device_type_to_char( sctk_device_type_t type );

/** This enum define the type of binding of a given device */
typedef enum
{
	SCTK_MACHINE_LEVEL_DEVICE, /**< Case where parent is the whole machine (or something not handled) */
	SCTK_TOPOLOGICAL_DEVICE /**< Case where parent is a numa node */
}sctk_device_container_t;

const char * sctk_device_container_to_char( sctk_device_container_t type );


/************************************************************************/
/* DEVICE DEFINITION                                                    */
/************************************************************************/

/** This is the container defining a device */
typedef struct sctk_device_s
{
	/* Descriptor */
	sctk_device_type_t type; /**< This is the type of the OSdev pointed to */
	sctk_device_container_t container; /**< This is the topological level of the device */
	char * name; /**< Name of the device (called OSname in HWloc) */
	
	/* Hwloc REF */
	hwloc_obj_t obj; /**< This is the actual Hwloc object (PCI) for this device */
	hwloc_obj_t non_io_parent_obj; /**< This is the first non IO parent of the object (defining locality) */
	
	/* Topology */
	hwloc_cpuset_t cpuset; /**< This is the CPUset of the parent non IO */
	hwloc_nodeset_t nodeset; /**< This is the NODEset of the parent non IO */

	int root_core; /**< This is the master core for this NUMA node */
	int root_numa; /**< This is the master NUMA node for this device */

	/* Attributes */
	const char * vendor; /** PCI card vendor */
	const char * device; /** PCI card descriptor */
	
}sctk_device_t;


void sctk_device_init( hwloc_topology_t topology, sctk_device_t * dev , hwloc_obj_t obj, int os_dev_offset );
void sctk_device_print( sctk_device_t * dev );

/************************************************************************/
/* MAIN INTERFACE                                                       */
/************************************************************************/

/* Init / Release */

void sctk_device_load_from_topology( hwloc_topology_t topology );
void sctk_device_release();

/* Device getter */

sctk_device_t * sctk_device_get_from_handle( char * handle );

/* Locality helpers */

int sctk_device_vp_is_device_leader( sctk_device_t * device, int vp_id );
int sctk_device_vp_is_on_device_numa( sctk_device_t * device, int vp_id );
int sctk_device_vp_is_outside_device_numa( sctk_device_t * device, int vp_id );

#endif /* SCTK_DEVICE_TOPOLOGY_H */
