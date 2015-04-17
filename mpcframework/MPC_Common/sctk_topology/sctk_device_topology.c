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
#include "sctk_spinlock.h"
#include "sctk_topology.h"

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
		case SCTK_TOPOLOGICAL_DEVICE:
			return "SCTK_TOPOLOGICAL_DEVICE";
		break;
	}	
	
}

/************************************************************************/
/* DEVICE STORAGE                                                       */
/************************************************************************/

static sctk_device_t * sctk_devices = NULL;
static int sctk_devices_count = 0;

/************************************************************************/
/* Scattering Computation                                               */
/************************************************************************/

static sctk_spinlock_t ___counter_lock = SCTK_SPINLOCK_INITIALIZER;
static int ___numa_count = -1;
static int ___core_count = -1;
static int ___current_numa_id = 0;
static int * ___current_core_id = NULL;



int sctk_device_count_pu_on_numa( hwloc_obj_t obj  )
{
	int i;
	
	if( obj->type == HWLOC_OBJ_PU )
	{
		return 1;
	}
	
	int ret = 0;
	
	for( i = 0 ; i < obj->arity ; i++ )
	{
		ret += sctk_device_count_pu_on_numa( obj->children[i] );
	}
	
	return ret;
}



void sctk_device_load_topology_limits( hwloc_topology_t topology )
{
	int numa = sctk_get_numa_node_number();
	/* Assume 1 when not a numa machine */
	if( numa == 0 )
		numa = 1;

	___numa_count = numa;
	
    /* Allocate a current core ID for each numa */
    ___current_core_id = sctk_calloc( ___numa_count, sizeof( int ) );
    assume( ___current_core_id != NULL );
	
	hwloc_obj_t numa_node =  hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NODE, NULL );
	
	/* Assume 1 to begin with */
	___core_count = 1;
	
	/* Prepare to walk numa nodes */
	int has_numa = 0;
	int last_count = -1;
	
	while( numa_node )
	{
		has_numa |= 1;
		
		int local_count = sctk_device_count_pu_on_numa( numa_node );
		
		if( last_count < 0 )
			last_count = local_count;
		
		if( local_count < last_count  )
		{
			sctk_warning("Machine seems to be asymetric");
			last_count = local_count;
		}
		
		numa_node = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NODE, numa_node );
	}

	/* No numa found */
	if( !has_numa )
	{
		___core_count = sctk_get_cpu_number();
	}
	else
	{
		/* If numa then use the minimum number of cores
		 * in order to adapt the scatering algorithm
		 * to every socket */
		___core_count = last_count;
	}

	sctk_error(" %d NUMA for %d CORES", ___numa_count, ___core_count );
}

int sctk_device_get_ith_logical_on_numa(  hwloc_topology_t topology, int numa_id , int core_id )
{
   int ret = -1;
   
   int did_alloc = 0;
   hwloc_cpuset_t local_cpuset;
   hwloc_obj_t numa = hwloc_get_obj_by_type( topology, HWLOC_OBJ_NODE , numa_id); 	
   
   if( !numa )
   {
	   /* This machine has no numa with this id (can also be non-numa)
	    * then work on the autorized CPUSET */
	   local_cpuset = hwloc_topology_get_allowed_cpuset( topology );
   }
   else
   {
	   /* We found the target numa, use its cpuset */
	   local_cpuset = numa->cpuset;
   }
   
   int i = 0;
	

	do
	{
		ret = hwloc_bitmap_next ( local_cpuset , ret);
	
		/* Not found is core ID erroneous ? */
		if( ret < 0 )
		{
			return 0;
		}
		
		/* We have the target logical core */
		if( i == core_id )
		{
			return ret;
		}
		 
		i++; 
	}while( 0 <= ret ); 
	
	/* Not found */
	return 0;
}

/************************************************************************/
/* DEVICE                                                               */
/************************************************************************/

void sctk_device_print( sctk_device_t * dev )
{
	sctk_info("#######################################");
	sctk_info("Type : %s", sctk_device_type_to_char( dev->type) );
	sctk_info("Container : %s", sctk_device_container_to_char( dev->container) );
	sctk_info("Name : %s", dev->name );
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
	sctk_info("Root numa : '%d'", dev->root_numa );
	sctk_info("Root core : '%d'", dev->root_core );
	
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
		

		hwloc_const_cpuset_t allowed_cpuset = hwloc_topology_get_allowed_cpuset( topology );
		
		/* If the device CPUset it diferent from the whole process
		 * cpuset then we can consider that this device might
		 * be facing some topological constraints */
		if( ! hwloc_bitmap_isequal (dev->cpuset, allowed_cpuset)  )
		{
			/* We set this flag to inform that the
			 * device is locality sensitive */
			dev->container = SCTK_TOPOLOGICAL_DEVICE;
			
			/*TODO*/
			
		}
		else
		{
			/* Here we have a device with no particular locality
			 * we then apply a scatter algoritms to elect
			 * a preferential NUMA node */
			dev->container = SCTK_MACHINE_LEVEL_DEVICE;
			
			sctk_spinlock_lock( & ___counter_lock );
			
			/* If needed first load the topology */
			if( (___core_count < 0) || ( ___numa_count < 0 ) )
			{
				sctk_device_load_topology_limits( topology );
			}
			
			/* Increment Numa */
			___current_numa_id = ( ___current_numa_id + 1 ) % ___numa_count;
			dev->root_numa = ___current_numa_id;
			
			/* Increment Thread */
			___current_core_id[ dev->root_numa ] = (___current_core_id[ dev->root_numa ] + 1 ) % ___core_count;
			dev->root_core = sctk_device_get_ith_logical_on_numa( topology, dev->root_numa, 
	___current_core_id[ dev->root_numa ] );

			sctk_spinlock_unlock( & ___counter_lock );
			
		}
		
		
	}
	else
	{
		/* No topology then assume we are machine level */
		dev->container = SCTK_MACHINE_LEVEL_DEVICE;
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
	int i;
	
	/* Try as an OS id  (eth0, eth1, sda, ... )*/
	for( i = 0 ; i < sctk_devices_count ; i++ )
	{
		if( !strcmp( handle, sctk_devices[i].name ) )
		{
			return &sctk_devices[i];
		}
	}
	
	/* Try as a PCI id (xxxx:yy:zz.t or yy:zz.t) */
	
	hwloc_obj_t pci_dev = hwloc_get_pcidev_by_busidstring ( sctk_get_topology_object(), handle );
	
	if(! pci_dev )
	{
		return NULL;
	}
	
	/* A pci dev was found now match to a device */
	for( i = 0 ; i < sctk_devices_count ; i++ )
	{
		/* Just compare HWloc object pointers */
		if( pci_dev == sctk_devices[i].obj )
		{
			return &sctk_devices[i];
		}
	}
	
	return NULL;
	
}


int sctk_device_vp_is_device_leader( sctk_device_t * device, int vp_id )
{
	
	
}

int sctk_device_vp_is_on_device_socket( sctk_device_t * device, int vp_id )
{
	
	
}

int sctk_device_vp_is_on_device_numa( sctk_device_t * device, int vp_id )
{
	
	
}
