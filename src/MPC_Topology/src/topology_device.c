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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpc_topology_device.h>

#include "mpc_common_debug.h"

#include "mpc_common_spinlock.h"

#include "topology.h"

#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include <sctk_alloc.h>

/************************************************************************/
/* ENUM DEFINITION                                                      */
/************************************************************************/

const char *mpc_topology_device_type_to_char( mpc_topology_device_type_t type )
{
	switch ( type )
	{
		case MPC_TOPO_DEVICE_BLOCK:
			return "MPC_TOPO_DEVICE_BLOCK";
			break;

		case MPC_TOPO_DEVICE_NETWORK_HANDLE:
			return "MPC_TOPO_DEVICE_NETWORK_HANDLE";
			break;

		case MPC_TOPO_DEVICE_NETWORK_OFA:
			return "MPC_TOPO_DEVICE_NETWORK_OFA";
			break;

		case MPC_TOPO_DEVICE_GPU:
			return "MPC_TOPO_DEVICE_GPU";
			break;

		case MPC_TOPO_DEVICE_COPROCESSOR:
			return "MPC_TOPO_DEVICE_COPROCESSOR";
			break;

		case MPC_TOPO_DEVICE_DMA:
			return "MPC_TOPO_DEVICE_DMA";
			break;

		case MPC_TOPO_DEVICE_UKNOWN:
		default:
			return "MPC_TOPO_DEVICE_UKNOWN";
			break;
	}

	not_reachable();
	return NULL;
}

mpc_topology_device_type_t mpc_topology_device_type_from_hwloc(hwloc_obj_osdev_type_t type)
{
	switch ( type )
	{
		case HWLOC_OBJ_OSDEV_BLOCK:
			return MPC_TOPO_DEVICE_BLOCK;
			break;

		case HWLOC_OBJ_OSDEV_NETWORK:
			return MPC_TOPO_DEVICE_NETWORK_HANDLE;
			break;

		case HWLOC_OBJ_OSDEV_OPENFABRICS:
			return MPC_TOPO_DEVICE_NETWORK_OFA;
			break;

		case HWLOC_OBJ_OSDEV_GPU:
			return MPC_TOPO_DEVICE_GPU;
			break;

		case HWLOC_OBJ_OSDEV_COPROC:
			return MPC_TOPO_DEVICE_COPROCESSOR;
			break;

		case HWLOC_OBJ_OSDEV_DMA:
			return MPC_TOPO_DEVICE_DMA;
	}

	return MPC_TOPO_DEVICE_UKNOWN;
}

const char *mpc_topology_device_container_to_char( mpc_topology_device_container_t type )
{
	switch ( type )
	{
		case MPC_TOPO_MACHINE_LEVEL_DEVICE:
			return "MPC_TOPO_MACHINE_LEVEL_DEVICE";
			break;

		case MPC_TOPO_TOPOLOGICAL_DEVICE:
			return "MPC_TOPO_TOPOLOGICAL_DEVICE";
			break;

		default:
			return NULL;
			break;
	}

	not_reachable();
	return NULL;
}

/************************************************************************/
/* DEVICE STORAGE                                                       */
/************************************************************************/

/** This is a structure defining the distance matrix
 * for devices */
typedef struct sctk_device_matrix_s
{
	int *distances;
	int pu_count;
	int device_count;
} sctk_device_matrix_t;

static mpc_topology_device_t *__mpc_topology_device_list = NULL;
static int __mpc_topology_device_list_count = 0;
static sctk_device_matrix_t __mpc_topology_device_matrix;

/************************************************************************/
/* Scattering Computation                                               */
/************************************************************************/

static mpc_common_spinlock_t ___counter_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static int ___numa_count = -1;
static int ___core_count = -1;
static int ___current_numa_id = 0;
static int *___current_core_id = NULL;

static inline int __topology_device_count_pu_on_numa( hwloc_obj_t obj )
{
	unsigned int i;

	if ( obj->type == HWLOC_OBJ_PU )
	{
		return 1;
	}

	int ret = 0;

	for ( i = 0; i < obj->arity; i++ )
	{
		ret += __topology_device_count_pu_on_numa( obj->children[i] );
	}

	return ret;
}

static inline void __topology_device_load_topology_limits( hwloc_topology_t topology )
{
	int numa = mpc_topology_get_numa_node_count();

	/* Assume 1 when not a numa machine */
	if ( numa == 0 )
	{
		numa = 1;
	}

	___numa_count = numa;
	/* Allocate a current core ID for each numa */
	___current_core_id = sctk_calloc( ___numa_count, sizeof( int ) );
	assume( ___current_core_id != NULL );
	hwloc_obj_t numa_node = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NUMANODE, NULL );
	/* Assume 1 to begin with */
	___core_count = 1;
	/* Prepare to walk numa nodes */
	int has_numa = 0;
	int last_count = -1;

	while ( numa_node )
	{
		has_numa |= 1;
		int local_count = __topology_device_count_pu_on_numa( numa_node );

		if ( local_count != 0 )
		{
			if ( last_count < 0 )
			{
				last_count = local_count;
			}

			if ( local_count < last_count )
			{
				mpc_common_debug_warning( "Machine seems to be asymetric" );
				last_count = local_count;
			}
		}

		numa_node = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NUMANODE, numa_node );
	}

	/* No numa found */
	if ( !has_numa )
	{
		___core_count = mpc_topology_get_pu_count();
	}
	else
	{
		/* If numa then use the minimum number of cores
		 * in order to adapt the scatering algorithm
		 * to every socket */
		___core_count = last_count;
	}

	/* Just in case we never know what HWLOC can do
	 * and which kind of machine we will get, put
	 * at least one in this value as we use it for modulos */

	if ( ___core_count == 0 )
	{
		___core_count = 1;
	}
}

static inline int __topology_device_get_ith_logical_on_numa( hwloc_topology_t topology,
															 int numa_id,
															 int core_id )
{
	int ret = -1;
	hwloc_cpuset_t local_cpuset;
	hwloc_obj_t numa = hwloc_get_obj_by_type( topology, HWLOC_OBJ_NUMANODE, numa_id );

	if ( !numa )
	{
		/* This machine has no numa with this id (can also be non-numa)
		* then work on the authorized CPUSET */
		local_cpuset = (hwloc_cpuset_t) hwloc_topology_get_allowed_cpuset( topology );
	}
	else
	{
		/* We found the target numa, use its cpuset */
		local_cpuset = numa->cpuset;
	}

	int i = 0;

	do
	{
		ret = hwloc_bitmap_next( local_cpuset, ret );

		/* Not found is core ID erroneous ? */
		if ( ret < 0 )
		{
			return 0;
		}

		/* We have the target logical core */
		if ( i == core_id )
		{
			/* Do not forget to convert it to logical */
			return mpc_topology_convert_os_pu_to_logical( ret );
		}

		i++;
	} while ( 0 <= ret );

	/* Not found */
	return 0;
}

/************************************************************************/
/* DEVICE                                                               */
/************************************************************************/

void mpc_topology_device_print( mpc_topology_device_t *dev )
{
	fprintf( stderr, "#######################################\n" );
	fprintf(  stderr, "Type : %s\n", mpc_topology_device_type_to_char( dev->type ) );
	fprintf(  stderr, "Container : %s\n", mpc_topology_device_container_to_char( dev->container ) );
	fprintf(  stderr, "Name : %s\n", dev->name );
	fprintf(  stderr, "Vendor : '%s'\n", dev->vendor );
	fprintf(  stderr, "Device : '%s'\n", dev->device );
	char cpuset[512];
	hwloc_bitmap_list_snprintf( cpuset, 512, dev->cpuset );
	char cpusetraw[512];
	hwloc_bitmap_snprintf( cpusetraw, 512, dev->cpuset );
	fprintf(  stderr, "CPU set : '%s' (%s)\n", cpuset, cpusetraw );
	char nodeset[512];
	hwloc_bitmap_list_snprintf( nodeset, 512, dev->nodeset );
	char nodesetraw[512];
	hwloc_bitmap_snprintf( nodesetraw, 512, dev->nodeset );
	fprintf(  stderr, "NODE set : '%s' (%s)\n", nodeset, nodesetraw );
	fprintf(  stderr, "Root numa : '%d'\n", dev->root_numa );
	fprintf(  stderr, "Root core : '%d'\n", dev->root_core );
	fprintf(  stderr, "Device ID : '%d'\n", dev->device_id );
	fprintf(  stderr, "#######################################\n" );
}

#if (HWLOC_API_VERSION > 0x00020000)
hwloc_obj_t __get_nth_io_child(hwloc_obj_t obj, int n)
{
    hwloc_obj_t io = NULL;
    int cnt = 0;

    for (io = obj->io_first_child; io; io = io->next_sibling)
    {
        if(cnt == n)
        {
            return io;
        }
        cnt++;
    }

    return NULL;
}
#endif

static void __topology_device_init( hwloc_topology_t topology, mpc_topology_device_t *dev, hwloc_obj_t obj, int os_dev_offset, int io_dev_offset )
{
	if ( obj->type != HWLOC_OBJ_PCI_DEVICE )
	{
		mpc_common_debug_fatal( "%s only expect PCI devices", __FUNCTION__ );
	}

	/* First clear everything */
	memset( dev, 0, sizeof( mpc_topology_device_t ) );
	/* Fill HWloc obj */
	dev->obj = obj;
	dev->non_io_parent_obj = hwloc_get_non_io_ancestor_obj( topology, obj );

	/* Fill PCI level info */

	if ( obj->name )
	{
		dev->name = strdup( obj->name );
	}
	else
	{
		dev->name = strdup( "" );
	}

	dev->vendor = hwloc_obj_get_info_by_name( obj, "PCIVendor" );
	dev->device = hwloc_obj_get_info_by_name( obj, "PCIDevice" );

	if ( dev->vendor == NULL )
	{
		dev->vendor = "UNKNOWN";
	}

	if ( dev->device == NULL )
	{
		dev->device = "UNKNOWN";
	}

	/* Fill Topological info */

	if ( dev->non_io_parent_obj )
	{
		dev->cpuset = hwloc_bitmap_dup( dev->non_io_parent_obj->cpuset );
		dev->nodeset = hwloc_bitmap_dup( dev->non_io_parent_obj->nodeset );
		hwloc_const_cpuset_t allowed_cpuset = hwloc_topology_get_allowed_cpuset( topology );

		/* If the device CPUset it diferent from the whole process
		 * cpuset then we can consider that this device might
		 * be facing some topological constraints */
		if ( !hwloc_bitmap_isequal( dev->cpuset, allowed_cpuset ) )
		{
			/* We set this flag to inform that the
			 * device is locality sensitive */
			dev->container = MPC_TOPO_TOPOLOGICAL_DEVICE;
			hwloc_obj_t numa_hwl = hwloc_get_obj_inside_cpuset_by_type( topology,
																		dev->cpuset,
																		HWLOC_OBJ_NUMANODE,
																		0 );

			if ( numa_hwl )
			{
				dev->root_numa = numa_hwl->logical_index;
			}

			hwloc_obj_t pu_hwl = hwloc_get_obj_inside_cpuset_by_type( topology,
																	  dev->cpuset,
																	  HWLOC_OBJ_PU,
																	  0 );

			if ( pu_hwl )
			{
				dev->root_core = pu_hwl->logical_index;
			}
		}
		else
		{
			/* Here we have a device with no particular locality
			 * we then apply a scatter algoritms to elect
			 * a preferential NUMA node */
			dev->container = MPC_TOPO_MACHINE_LEVEL_DEVICE;
			mpc_common_spinlock_lock( &___counter_lock );

			/* If needed first load the topology */
			if ( ( ___core_count < 0 ) || ( ___numa_count < 0 ) )
			{
				__topology_device_load_topology_limits( topology );
			}

			/* Increment Numa */
			___current_numa_id = ( ___current_numa_id + 1 ) % ___numa_count;
			dev->root_numa = ___current_numa_id;
			/* Increment Thread */
			___current_core_id[dev->root_numa] = ( ___current_core_id[dev->root_numa] + 1 ) % ___core_count;
			dev->root_core = __topology_device_get_ith_logical_on_numa( topology,
																		dev->root_numa,
																		___current_core_id[dev->root_numa] );
			mpc_common_spinlock_unlock( &___counter_lock );
		}
	}
	else
	{
		/* No topology then assume we are machine level */
		dev->container = MPC_TOPO_MACHINE_LEVEL_DEVICE;
	}

	/* Retrieve to OS level child */
	hwloc_obj_t os_level_obj = NULL;

	if ( (0 <= os_dev_offset) && obj->children )
	{
		if ( obj->children[os_dev_offset] )
		{
			if ( obj->children[os_dev_offset]->type == HWLOC_OBJ_OS_DEVICE )
			{
				os_level_obj = obj->children[os_dev_offset];
			}
			else
			{
				mpc_common_debug_error( "Warning a non os object was passed to a PCI device initializer" );
			}
		}
	}


#if (HWLOC_API_VERSION > 0x00020000)
	if ( 0 <= io_dev_offset )
	{
        hwloc_obj_t io = __get_nth_io_child(obj, io_dev_offset);

		if ( io->type == HWLOC_OBJ_OS_DEVICE )
		{
			os_level_obj = io;
		}
		else
		{
			mpc_common_debug_error( "Warning a non os object was passed to a PCI device initializer" );
	    }
	}
#endif

	/* Type resolution */
	dev->type = MPC_TOPO_DEVICE_UKNOWN;

	if ( os_level_obj != NULL )
	{
		/* Overide the PCI level name with the OS level one if found */
		if ( os_level_obj->name )
		{
			if ( dev->name )
			{
				free( dev->name );
			}

			dev->name = strdup( os_level_obj->name );
		}

		/* Do we have an attr ? */
		struct hwloc_osdev_attr_s *dev_attr = NULL;

		if ( os_level_obj->attr )
		{
			dev_attr = &os_level_obj->attr->osdev;
		}

		/* If we resolved ATTR then retrieve the type */
		if ( dev_attr )
		{
			dev->type = mpc_topology_device_type_from_hwloc(dev_attr->type);
		}
	}

	dev->device_id = -1;
	dev->nb_res = 0;
	mpc_common_spinlock_init(&dev->res_lock, 0);
}

static inline void __topology_device_enrich_topology()
{
	int i;
#if defined( MPC_USE_CUDA )
	int cuda_device_to_locate = 0;

	/* init() returns 0 if succeed (maybe not obvious) */
	if ( !sctk_accl_cuda_init() )
	{
		cudaGetDeviceCount( &cuda_device_to_locate );
	}

#endif

	for ( i = 0; i < __mpc_topology_device_list_count; i++ )
	{
#if defined( MPC_USE_CUDA )
		mpc_topology_device_t *device = &__mpc_topology_device_list[i];

		if ( cuda_device_to_locate > 0 )
		{
			char attrs[128], *save_ptr, busid_str[32];
			const char *attr_sep = "#";
			hwloc_obj_attr_snprintf( attrs, 128, device->obj, "#", 1 );
			char *cur_attr = NULL;

			/* if the current attr is the bus ID: STOP */
			while ( ( cur_attr = strtok_r( attrs, attr_sep, &save_ptr ) ) != NULL )
			{
				if ( strncmp( cur_attr, "busid", 5 ) == 0 )
				{
					break;
				}
			}

			/* Read the PCI bus ID from the found attr */
			sscanf( cur_attr, "busid=%s", busid_str );
			/* maybe the init() should be done only once ? */
			CUdevice dev = 0;
			CUresult test = sctk_cuDeviceGetByPCIBusId( &dev, busid_str );

			/* if the PCI bus ID matches a CUDA-enabled device */
			if ( test == CUDA_SUCCESS )
			{
				/** RENAMING THE DEVICE */
				const short name_prefix_size = 17;
				const char *name_prefix = "cuda-enabled-card";
				/* we suppose 4 digits to encode GPU ids is enough ! */
				const short name_size = name_prefix_size + 4;
				char *name = sctk_malloc( sizeof( char ) * name_size );
				int check;
				/* Creating one single string */
				check = snprintf( name, name_size, "%s%d", name_prefix, (int) dev );
				assert( check < name_size );
				device->name = name;
				device->device_id = (int) dev;
				mpc_common_debug( "Detected GPU: %s (%s)", device->name, busid_str );
				cuda_device_to_locate--;
			}
			else
			{
				if ( device->name == NULL )
				{
					device->name = "Unknown";
				}

				device->device_id = -1;
			}
		}

#endif
	}
}

/************************************************************************/
/* DISTANCE MATRIX                                                      */
/************************************************************************/

/** Return a pointer to cell */
static inline int *__topology_device_matrix_get_cell( int pu_id, int device_id )
{
	if ( !__mpc_topology_device_matrix.distances )
	{
		return NULL;
	}

	if ( ( pu_id < 0 ) || ( __mpc_topology_device_matrix.pu_count <= pu_id ) )
	{
		return NULL;
	}

	if ( ( device_id < 0 ) || ( __mpc_topology_device_matrix.device_count <= device_id ) )
	{
		return NULL;
	}

	return &__mpc_topology_device_matrix.distances[pu_id * __mpc_topology_device_matrix.device_count + device_id];
}

/** Return a cell value */
static inline int __topology_device_matrix_get_value( int pu_id, int device_id )
{
	int *ret = __topology_device_matrix_get_cell( pu_id, device_id );

	if ( !ret )
	{
		return -1;
	}

	return *ret;
}

static inline void __topology_device_matrix_release()
{
	__mpc_topology_device_matrix.device_count = 0;
	__mpc_topology_device_matrix.pu_count = 0;
	sctk_free( __mpc_topology_device_matrix.distances );
	__mpc_topology_device_matrix.distances = NULL;
}

/** Intializes the device matrix */
static inline void __topology_device_matrix_init( hwloc_topology_t topology )
{
	__mpc_topology_device_matrix.device_count = __mpc_topology_device_list_count;
	__mpc_topology_device_matrix.pu_count = mpc_topology_get_pu_count();
	__mpc_topology_device_matrix.distances = sctk_malloc( sizeof( int ) * __mpc_topology_device_matrix.device_count * __mpc_topology_device_matrix.pu_count );
	assume( __mpc_topology_device_matrix.distances != NULL );
	int i, j;

	/* For each device */
	for ( i = 0; i < __mpc_topology_device_matrix.device_count; i++ )
	{
		hwloc_obj_t device_obj = __mpc_topology_device_list[i].obj;

		/* For each PU */
		for ( j = 0; j < __mpc_topology_device_matrix.pu_count; j++ )
		{
			int *cell = __topology_device_matrix_get_cell( j, i );

			if ( !cell )
			{
				mpc_common_debug_warning( "Could not get cell" );
				continue;
			}

			/* Compute the distance */
			*cell = _mpc_topology_get_distance_from_pu( topology, j, device_obj );
			mpc_common_nodebug( "Distance (PU %d, DEV %d (%s)) == %d", j,
						  i, __mpc_topology_device_list[i].name, *cell );
		}
	}
}

/** Return 1 if the devices matching the regexp are equidistant */
int mpc_topology_device_matrix_is_equidistant( char *matching_regexp )
{
	/* Retrieve devices matching the regexp */
	int count;
	mpc_topology_device_t **device_list = mpc_topology_device_get_from_handle_regexp( matching_regexp, &count );

	if ( !count )
	{
		return 1;
	}

	int i, j;
	int ref_distance = -1;

	for ( i = 0; i < count; i++ )
	{
		for ( j = 0; j < __mpc_topology_device_matrix.pu_count; j++ )
		{
			mpc_topology_device_t *dev = device_list[i];
			int distance = __topology_device_matrix_get_value( j, dev->id );

			if ( ref_distance < 0 )
			{
				ref_distance = distance;
			}
			else
			{
				if ( distance != ref_distance )
				{
					sctk_free( device_list );
					return 0;
				}
			}
		}
	}

	sctk_free( device_list );
	/* If we are still here topology is even */
	return 1;
}

/********************
 * INIT AND RELEASE *
 ********************/

void _mpc_topology_device_init( hwloc_topology_t topology )
{
	/* First walk to count */
	hwloc_obj_t pci_dev = hwloc_get_next_pcidev( topology, NULL );
	unsigned int i;

	while ( pci_dev )
	{
#if (HWLOC_API_VERSION < 0x00020000)
		if ( !pci_dev->arity )
#else
		if ( !pci_dev->arity && !pci_dev->io_arity )
#endif
        {
			__mpc_topology_device_list_count++;
		}
		else
		{
			/* Note that some PCI dev can create multiple OSDev */
			for ( i = 0; i < pci_dev->arity; i++ )
			{
				if ( pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE )
				{
					__mpc_topology_device_list_count++;
				}
			}
#if (HWLOC_API_VERSION > 0x00020000)
			for ( i = 0; i < pci_dev->io_arity; i++ )
			{
                hwloc_obj_t io = __get_nth_io_child(pci_dev, i);

                if ( io->type == HWLOC_OBJ_OS_DEVICE )
				{
					__mpc_topology_device_list_count++;
				}
			}
#endif
		}

		pci_dev = hwloc_get_next_pcidev( topology, pci_dev );
	}

	mpc_common_debug( "sctk_topology located %d PCI devices",
				  __mpc_topology_device_list_count );
	/* Allocate devices */
	__mpc_topology_device_list = sctk_malloc( sizeof( mpc_topology_device_t ) * __mpc_topology_device_list_count );
	/* Then walk again to fill devices */
	pci_dev = hwloc_get_next_pcidev( topology, NULL );
	int off = 0;

	while ( pci_dev )
	{
		assume( off < __mpc_topology_device_list_count );
#if (HWLOC_API_VERSION < 0x00020000)
		if ( !pci_dev->arity )
#else
		if ( !pci_dev->arity && !pci_dev->io_arity )
#endif
        {
			__topology_device_init( topology, &__mpc_topology_device_list[off], pci_dev, -1 , -1);
			off++;
		}
		else
		{
			for ( i = 0; i < pci_dev->arity; i++ )
			{
				/* Unfold the PCI device to process HOST objs */
				if ( pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE )
				{
					__topology_device_init( topology, &__mpc_topology_device_list[off], pci_dev, i , -1);
					/* Set the ID of the device */
					__mpc_topology_device_list[off].id = off;
					off++;
				}
			}
#if (HWLOC_API_VERSION > 0x00020000)
			for ( i = 0; i < pci_dev->io_arity; i++ )
			{
                hwloc_obj_t io = __get_nth_io_child(pci_dev, i);

				if ( io->type == HWLOC_OBJ_OS_DEVICE )
				{
					__topology_device_init( topology, &__mpc_topology_device_list[off], pci_dev, -1, i );
					/* Set the ID of the device */
					__mpc_topology_device_list[off].id = off;
					off++;
				}
			}
#endif


        }

		pci_dev = hwloc_get_next_pcidev( topology, pci_dev );
	}

	__topology_device_enrich_topology();

#if 0
#if (HWLOC_API_VERSION > 0x00020000)
    hwloc_topology_export_xml(topology, "-", 0);
#else
    hwloc_topology_export_xml(topology, "-");
#endif
#endif

#if 0
	int j;
	for ( j = 0; j < __mpc_topology_device_list_count; j++ )
	{
	 mpc_topology_device_print( &__mpc_topology_device_list[i] );
	}
#endif
	/* Now initialize the device distance matrix */
	__topology_device_matrix_init( topology );
}

void _mpc_topology_device_release()
{
	sctk_free( __mpc_topology_device_list );
	__mpc_topology_device_list = NULL;
	__mpc_topology_device_list_count = 0;
	__topology_device_matrix_release();
}

/*****************
 * DEVICE GETTER *
 *****************/

mpc_topology_device_t *mpc_topology_device_get_from_handle( char *handle )
{
	int i;

	if ( !handle )
	{
		return NULL;
	}

	/* Try as an OS id  (eth0, eth1, sda, ... )*/
	for ( i = 0; i < __mpc_topology_device_list_count; i++ )
	{
		if ( !strcmp( handle, __mpc_topology_device_list[i].name ) )
		{
			return &__mpc_topology_device_list[i];
		}
	}

	/* Try as a PCI id (xxxx:yy:zz.t or yy:zz.t) */
	hwloc_obj_t pci_dev = hwloc_get_pcidev_by_busidstring( mpc_topology_get(), handle );

	if ( !pci_dev )
	{
		return NULL;
	}

	/* A pci dev was found now match to a device */
	for ( i = 0; i < __mpc_topology_device_list_count; i++ )
	{
		/* Just compare HWloc object pointers */
		if ( pci_dev == __mpc_topology_device_list[i].obj )
		{
			return &__mpc_topology_device_list[i];
		}
	}

	return NULL;
}

int mpc_topology_device_get_id_from_handle( char *handle )
{
	mpc_topology_device_t *dev = mpc_topology_device_get_from_handle( handle );

	if ( !dev )
	{
		return -1;
	}

	return dev->device_id;
}

mpc_topology_device_t **mpc_topology_device_get_from_handle_regexp( char *handle_reg_exp,
																	int *count )
{
	if ( !handle_reg_exp )
	{
		if ( count )
		{
			*count = -1;
		}
		return NULL;
	}

	/* For simplicity just allocate an array of the number of devices (make it NULL terminated) */
	mpc_topology_device_t **ret_dev = sctk_malloc( sizeof( mpc_topology_device_t * ) * ( __mpc_topology_device_list_count + 1 ) );
	/* Set it to NULL */
	int i;

	for ( i = 0; i < __mpc_topology_device_list_count + 1; i++ )
	{
		ret_dev[i] = NULL;
	}

	/* Prepare the regexp */
	regex_t regexp;
	char msg_buff[100];
	int ret = regcomp( &regexp, handle_reg_exp, 0 );

	if ( ret )
	{
		perror( "regcomp" );
		regerror( ret, &regexp, msg_buff, 100 );
		mpc_common_debug_fatal( "Could not compile device regexp %s ( %s )", handle_reg_exp, msg_buff );
	}

	/* Lets walk the devices to see who matches */
	int current_count = 0;

	for ( i = 0; i < __mpc_topology_device_list_count; i++ )
	{
		ret = regexec( &regexp, __mpc_topology_device_list[i].name, 0, NULL, 0 );

		if ( ret == 0 )
		{
			/* Match then push the device */
			mpc_common_nodebug( "Regex %s MATCH %s", handle_reg_exp, __mpc_topology_device_list[i].name );
			ret_dev[current_count] = &__mpc_topology_device_list[i];
			current_count++;
		}
		else if ( ret == REG_NOMATCH )
		{
			/* No Match */
		}
		else
		{
			regerror( ret, &regexp, msg_buff, 100 );
			mpc_common_debug_fatal( "Could not execute device regexp %s on %s ( %s )", handle_reg_exp,
						__mpc_topology_device_list[i].name,
						msg_buff );
		}
	}

	if ( count )
	{
		*count = current_count;
	}

	/* Free the regexp */
	regfree( &regexp );
	return ret_dev;
}

void mpc_topology_device_attach_resource( mpc_topology_device_t *device )
{
	mpc_common_spinlock_lock( &device->res_lock );
	device->nb_res++;
	mpc_common_spinlock_unlock( &device->res_lock );
}

void mpc_topology_device_detach_resource( mpc_topology_device_t *device )
{
	mpc_common_spinlock_lock( &device->res_lock );
	device->nb_res--;
	mpc_common_spinlock_unlock( &device->res_lock );
}

mpc_topology_device_t *mpc_topology_device_attach_freest_device_from( mpc_topology_device_t **device_list, int count )
{
	int i;
	int freest_value = -1;
	mpc_topology_device_t *freest_elem = NULL;

	if ( count < 1 )
	{
		return NULL;
	}

	/* for each selected device */
	for ( i = 0; i < count; i++ )
	{
		mpc_topology_device_t *current = device_list[i];
		/* lock the counter */
		mpc_common_spinlock_lock( &current->res_lock );

		/* if it the first checked driver */
		if ( freest_value < 0 )
		{
			freest_value = current->nb_res;
			freest_elem = current;
			mpc_common_nodebug( "First device: %d (%d)", freest_elem->device_id,
						  freest_elem->nb_res );
		}
		else if ( (int) current->nb_res < freest_value )
		{
			/* free the previous selected device */
			mpc_common_spinlock_unlock( &freest_elem->res_lock );
			freest_value = current->nb_res;
			freest_elem = current;
			mpc_common_nodebug( "New best device: %d (%d)", freest_elem->device_id,
						  freest_elem->nb_res );
		}
		else
		{
			/* this device is not elected as the freest ones -> unlock */
			mpc_common_spinlock_unlock( &current->res_lock );
		}
	}

	assert( freest_elem != NULL );
	freest_elem->nb_res++;
	mpc_common_spinlock_unlock( &freest_elem->res_lock );
	return freest_elem;
}

mpc_topology_device_t *mpc_topology_device_attach_freest_device( char *handle_reg_exp )
{
	int count;
	mpc_topology_device_t **device_list =
		mpc_topology_device_get_from_handle_regexp( handle_reg_exp, &count );
	mpc_topology_device_t *device_res =
		mpc_topology_device_attach_freest_device_from( device_list, count );
	sctk_free( device_list );
	return device_res;
}

/** Get the closest device from PU matching the regexp "matching_regexp" */
mpc_topology_device_t *mpc_topology_device_get_closest_from_pu( int pu_id, char *matching_regexp )
{
	int dummy;
	mpc_topology_device_t **device_list = mpc_topology_device_matrix_get_list_closest_from_pu(
		pu_id, matching_regexp, &dummy );
	mpc_topology_device_t *first = device_list[0];
	sctk_free( device_list );
	return first;
}

mpc_topology_device_t **mpc_topology_device_matrix_get_list_closest_from_pu( int pu_id, char *matching_regexp,
																			 int *count_out )
{
	/* Retrieve devices matching the regexp */
	int count;
	mpc_topology_device_t **device_list =
		mpc_topology_device_get_from_handle_regexp( matching_regexp, &count );

	if ( count < 1 )
	{
		return NULL;
	}

	mpc_topology_device_t **ret_list = sctk_malloc( count * sizeof( mpc_topology_device_t ) );
	int i;

	/* init the array */
	for ( i = 0; i < count; i++ )
	{
		ret_list[i] = NULL;
	}

	int minimal_distance = -1;
	*count_out = 0;

	/* for each found device */
	for ( i = 0; i < count; i++ )
	{
		mpc_topology_device_t *dev = device_list[i];
		int distance = __topology_device_matrix_get_value( pu_id, dev->id );

		/* if the distance is lower than 0, skip it (should we ? ) */
		if ( distance < 0 )
		{
			continue;
		}

		/* Lookup for the minimum distance */
		if ( ( minimal_distance < 0 ) || ( distance <= minimal_distance ) )
		{
			minimal_distance = distance;
		}
	}

	/* now we have the minimum distance, look for any device which match with it
	*/
	for ( i = 0; i < count; i++ )
	{
		mpc_topology_device_t *dev = device_list[i];
		int distance = __topology_device_matrix_get_value( pu_id, dev->id );

		if ( distance == minimal_distance )
		{
			ret_list[*count_out] = dev;
			( *count_out )++;
		}
	}

	/* don't forget to free the malloc'd vector from
	* mpc_topology_device_get_from_handle_regexp() */
	sctk_free( device_list );
	return ret_list;
}
