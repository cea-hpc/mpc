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
#include "sctk_device_topology.h"

#include "sctk_debug.h"
#include "sctk_thread.h"

/************************************************************************/
/* ENUM DEFINITION                                                      */
/************************************************************************/

const char * sctk_device_type_to_char( sctk_device_type_t type )
{
	switch( type )
	{
		case SCTK_DEVICE_BLOCK:
			return "SCTK_DEVICE_BLOCK";
		break;
		case SCTK_DEVICE_NETWORK_HANDLE:
			return "SCTK_DEVICE_NETWORK_HANDLE";
		break;
		case SCTK_DEVICE_NETWORK_OFA:
			return "SCTK_DEVICE_NETWORK_OFA";
		break;
		case SCTK_DEVICE_GPU:
			return "SCTK_DEVICE_GPU";
		break;
		case SCTK_DEVICE_COPROCESSOR:
			return "SCTK_DEVICE_COPROCESSOR";
		break;
		case SCTK_DEVICE_DMA:
			return "SCTK_DEVICE_DMA";
		break;
		case SCTK_DEVICE_UKNOWN:
			return "SCTK_DEVICE_UKNOWN";
		break;
	}
}


const char * sctk_device_container_to_char( sctk_device_container_t type )
{
	switch( type )
	{
		case SCTK_MACHINE_LEVEL_DEVICE:
			return "SCTK_MACHINE_LEVEL_DEVICE";
		break;
		case SCTK_NUMA_LEVEL_DEVICE:
			return "SCTK_NUMA_LEVEL_DEVICE";
		break;
	}	
	
}

/************************************************************************/
/* DEVICE STORAGE                                                       */
/************************************************************************/

static sctk_device_t * sctk_devices = NULL;
static int sctk_devices_count = 0;

#if 0
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
	hwloc_cpuset_t nodeset; /**< This is the NODEset of the parent non IO */
	
	int numa_id; /**< This is the ID of the NUMA node containing the device */
	int vp_id; /**< This is the ID of the core elected as master for this device */

	/* Attributes */
	char * vendor; /** PCI card vendor */
	char * device; /** PCI card descriptor */
	
}sctk_device_t;
#endif



/************************************************************************/
/* DEVICE                                                               */
/************************************************************************/

void sctk_device_print( sctk_device_t * dev )
{
	sctk_info("#######################################");
	sctk_info("Type : %s", sctk_device_type_to_char( dev->type) );
	sctk_info("Container : %s", sctk_device_container_to_char( dev->container) );
	sctk_info("Name : %s", dev->name );
	sctk_info("Numa ID : %d", dev->numa_id );
	sctk_info("VP ID : %d", dev->vp_id );
	sctk_info("Vendor : '%s'", dev->vendor );
	sctk_info("Device : '%s'", dev->device );
	
	char cpuset[512];
	hwloc_bitmap_list_snprintf (cpuset, 512, dev->cpuset);

	char cpusetraw[512];
	hwloc_bitmap_snprintf(cpusetraw, 512, dev->cpuset);
	
	sctk_info("CPU set : '%s' (%s)", cpuset , cpusetraw );
	
	char nodeset[512];
	hwloc_bitmap_list_snprintf (nodeset, 512, dev->nodeset);

	char nodesetraw[512];
	hwloc_bitmap_snprintf(nodesetraw, 512, dev->nodeset);
	
	sctk_info("NODE set : '%s' (%s)", nodeset, nodesetraw );
	
	sctk_info("#######################################");
}

void sctk_device_init( hwloc_topology_t topology, sctk_device_t * dev , hwloc_obj_t obj, int os_dev_offset )
{
	if( obj->type != HWLOC_OBJ_PCI_DEVICE )
	{
		sctk_fatal("%s only expect PCI devices", __FUNCTION__ );
	}
	
	/* First clear everything */
	
	memset( dev, 0, sizeof( sctk_device_t ) );
	
	/* Fill HWloc obj */
	dev->obj = obj;
	dev->non_io_parent_obj = hwloc_get_non_io_ancestor_obj( topology, obj );
	
	/* Fill PCI level info */
	
	if( obj->name )
	{
		dev->name = strdup( obj->name );
	}
	
	dev->vendor =  hwloc_obj_get_info_by_name( obj, "PCIVendor");
	dev->device = hwloc_obj_get_info_by_name( obj, "PCIDevice");
	
	if( dev->vendor == NULL )
	{
		dev->vendor = "UNKNOWN";
	}
	
	if( dev->device == NULL )
	{
		dev->device = "UNKNOWN";
	}
	
	/* Fill Topological info */
	
	if( dev->non_io_parent_obj )
	{
		dev->cpuset = hwloc_bitmap_dup( dev->non_io_parent_obj->cpuset );
		dev->nodeset = hwloc_bitmap_dup( dev->non_io_parent_obj->nodeset );
		
		if( dev->non_io_parent_obj->type == HWLOC_OBJ_NODE 
		|| dev->non_io_parent_obj->type == HWLOC_OBJ_GROUP  )
		{
			/* We set this flag to inform that the
			 * device is locality sensitive */
			dev->container = SCTK_NUMA_LEVEL_DEVICE;
		}
		else
		{
			dev->container = SCTK_MACHINE_LEVEL_DEVICE;
		}
		
		
	}
	else
	{
		/* No topology then assume we are machine level */
		dev->container = SCTK_MACHINE_LEVEL_DEVICE;
		/* Master is all zero */
		dev->numa_id = 0;
		dev->vp_id = 0;
	}
	
	/* Retrieve to OS level child */
	
	hwloc_obj_t os_level_obj = NULL;
	
	if( obj->children )
	{
		if( obj->children[os_dev_offset] )
		{
			if( obj->children[os_dev_offset]->type == HWLOC_OBJ_OS_DEVICE )
			{
				os_level_obj = obj->children[os_dev_offset];
			}
			else
			{
				sctk_error("Warning a non os object was passed to a PCI device initializer");
			}
		}
	}
	
	/* Type resolution */
	
	dev->type = SCTK_DEVICE_UKNOWN;
	
	if( os_level_obj != NULL )
	{
		/* Overide the PCI level name with the OS level one if found */
		if( os_level_obj->name )
		{
			if( dev->name )
			{
				free( dev->name );
			}
			
			dev->name = strdup( os_level_obj->name );	
		}
		
		/* Do we have an attr ? */
		
		struct hwloc_osdev_attr_s * dev_attr = NULL;
		
		if( os_level_obj->attr )
		{
			dev_attr = &os_level_obj->attr->osdev;
		}
		
		/* If we resolved ATTR then retrieve the type */
		if( dev_attr )
		{
			dev->type = dev_attr->type;
		}
	}
	
	sctk_device_print( dev );
}

/************************************************************************/
/* DEVICE INTERFACE                                                     */
/************************************************************************/

void sctk_device_load_from_topology( hwloc_topology_t topology )
{
	/* First walk to count */
	hwloc_obj_t pci_dev =  hwloc_get_next_pcidev( topology, NULL );
	int i;
	
	while( pci_dev )
	{
		/* Note that some PCI dev can create multiple OSDev */
		for( i = 0 ; i < pci_dev->arity ; i++ )
		{
			if( pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE )
			{
				sctk_devices_count ++;
			}
		}
		
		pci_dev = hwloc_get_next_pcidev( topology, pci_dev );
	}
	
	sctk_info("sctk_topology located %d PCI devices", sctk_devices_count );
	
	/* Allocate devices */
	sctk_devices = sctk_malloc( sizeof( sctk_device_t ) * sctk_devices_count );
	
	/* Then walk again to fill devices */
	pci_dev =  hwloc_get_next_pcidev( topology, NULL );
	
	int off = 0;
	
	while( pci_dev )
	{

		for( i = 0 ; i < pci_dev->arity ; i++ )
		{
			/* Unfold the PCI device to process HOST objs */
			if( pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE )
			{
				sctk_device_init( topology, &sctk_devices[ off ] , pci_dev, i );
				off++;
			}
		}
		
		pci_dev = hwloc_get_next_pcidev( topology, pci_dev );
	}
	
	
	hwloc_topology_export_xml(topology, "-");
}

void sctk_device_release()
{
	
	
}


sctk_device_t * sctk_device_get_from_handle( char * handle )
{
	
	
}


int sctk_device_vp_is_device_leader( sctk_device_t * device, int vp_id )
{
	
	
}

int sctk_device_vp_is_on_device_numa( sctk_device_t * device, int vp_id )
{
	
	
}

int sctk_device_vp_is_outside_device_numa( sctk_device_t * device, int vp_id )
{
	
	
}
