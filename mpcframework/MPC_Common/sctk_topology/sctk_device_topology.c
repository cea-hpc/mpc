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
#include "sctk_device_topology.h"

#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_spinlock.h"
#include "sctk_topology.h"

#include <sys/types.h>
#include <regex.h>

#if defined(MPC_Accelerators)
#include <sctk_accelerators.h>
#endif // MPC_Accelerators

#ifdef MPC_USE_INFINIBAND
#include <infiniband/verbs.h>
#endif

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
		default:
			return "SCTK_DEVICE_UKNOWN";
		break;
	}
	not_reachable();
	return NULL;
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

static sctk_device_t * sctk_devices = NULL;
static int sctk_devices_count = 0;
static sctk_device_matrix_t sctk_device_matrix;

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
		
		if( local_count != 0 )
		{
		
			if( last_count < 0 )
				last_count = local_count;
			
			if( local_count < last_count  )
			{
				sctk_warning("Machine seems to be asymetric");
				last_count = local_count;
			}
			
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
	
	/* Just in case we never know what HWLOC can do
	 * and which kind of machine we will get, put
	 * at least one in this value as we use it for modulos */
	
	if( ___core_count == 0 )
	{
		___core_count = 1;
	}
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
	   local_cpuset = (hwloc_cpuset_t)hwloc_topology_get_allowed_cpuset( topology );
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
			/* Do not forget to convert it to logical */
			return sctk_topology_convert_os_pu_to_logical( ret );
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
	sctk_info("Device ID : '%d'", dev->device_id );
	
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
	else
	{
		dev->name = strdup("");
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

	dev->device_id = -1;
        dev->nb_res = 0;
        dev->res_lock = SCTK_SPINLOCK_INITIALIZER;
}

/************************************************************************/
/* DEVICE INTERFACE                                                     */
/************************************************************************/

#ifdef MPC_USE_INFINIBAND
/** The purpose of this function is to resolve the device ids of the OFA devices */
void sctk_device_fill_in_infiniband_info( sctk_device_t * device, hwloc_topology_t topology )
{
	int devices_nb;
	/* Retrieve device list from verbs */
	struct ibv_device ** dev_list = ibv_get_device_list ( &devices_nb );

	int id;
	
	/* For all the devices */
	for( id = 0 ; id < devices_nb ; id++ )
	{
		/* Retrieve the HWLOC osdev */
		hwloc_obj_t ofa_osdev = hwloc_ibv_get_device_osdev ( topology, dev_list[id] );

		/* If one */
		if( ofa_osdev )
		{
			/* Compare the OS name with the one we have from the previously loaded device */
			if( !strcmp( device->name , ofa_osdev->name ) )
			{
				sctk_nodebug("OFA device %s has ID %d", ofa_osdev->name, id );
				device->device_id = id;
			}
		}
	}
}
#endif





void sctk_device_enrich_topology( hwloc_topology_t topology )
{
	int i;
#if defined(MPC_USE_CUDA)
        int num_devices = 0;
        /* init() returns 0 if succeed (maybe not obvious) */
		if( ! sctk_accl_cuda_init() )
		{
			cudaGetDeviceCount(&num_devices);
		}
#endif
        for (i = 0; i < sctk_devices_count; i++) {
          sctk_device_t *device = &sctk_devices[i];

          if (device->type == SCTK_DEVICE_NETWORK_OFA) {
#ifdef MPC_USE_INFINIBAND
            sctk_device_fill_in_infiniband_info(device, topology);
#endif
          }

#if defined(MPC_USE_CUDA)
          if (num_devices > 0) {
            char attrs[128], save_ptr[128], busid_str[32];
            const char *attr_sep = "#";
            hwloc_obj_attr_snprintf(attrs, 128, device->obj, "#", 1);

            char *cur_attr = NULL;

            /* if the current attr is the bus ID: STOP */
            while ((cur_attr = strtok_r(attrs, attr_sep, &save_ptr)) != NULL) {
              if (strncmp(cur_attr, "busid", 5) == 0) {
                break;
              }
            }

            /* Read the PCI bus ID from the found attr */
            sscanf(cur_attr, "busid=%s", busid_str);

            /* maybe the init() should be done only once ? */
            CUdevice dev = 0;
            CUresult test = sctk_cuDeviceGetByPCIBusId(&dev, busid_str);

            /* if the PCI bus ID matches a CUDA-enabled device */
            if (test == CUDA_SUCCESS) {

              /** RENAMING THE DEVICE */
              const short name_prefix_size = 17;
              const char *name_prefix = "cuda-enabled-card";

              /* we suppose 4 digits to encode GPU ids is enough ! */
              const short name_size = name_prefix_size + 4;
              char *name = malloc(sizeof(char) * name_size);
              int check;

              /* Creating one single string */
              check = snprintf(name, name_size, "%s%d", name_prefix, (int)dev);
              assert(check < name_size);

              device->name = name;
              device->device_id = (int)dev;
              sctk_nodebug("Detected GPU: %s (%s)", device->name, busid_str);
            } else {
              if (device->name == NULL)
                device->name = "Unknown";
              device->device_id = -1;
            }
          }
#endif
        }
}


void sctk_device_load_from_topology( hwloc_topology_t topology )
{
	/* First walk to count */
	hwloc_obj_t pci_dev =  hwloc_get_next_pcidev( topology, NULL );
	int i;
	
	while( pci_dev )
	{
		if( ! pci_dev->arity )
		{
			sctk_devices_count ++;
		}
		else
		{
			/* Note that some PCI dev can create multiple OSDev */
			for( i = 0 ; i < pci_dev->arity ; i++ )
			{
				if( pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE )
				{
					sctk_devices_count ++;
				}
				
			}
		}
		
		pci_dev = hwloc_get_next_pcidev( topology, pci_dev );
	}

        sctk_nodebug("sctk_topology located %d PCI devices",
                     sctk_devices_count);

        /* Allocate devices */
        sctk_devices = sctk_malloc(sizeof(sctk_device_t) * sctk_devices_count);

        /* Then walk again to fill devices */
        pci_dev = hwloc_get_next_pcidev(topology, NULL);

        int off = 0;

        while (pci_dev) {

          if (!pci_dev->arity) {
            sctk_device_init(topology, &sctk_devices[off], pci_dev, 0);
            off++;
          } else {
            for (i = 0; i < pci_dev->arity; i++) {
              /* Unfold the PCI device to process HOST objs */
              if (pci_dev->children[i]->type == HWLOC_OBJ_OS_DEVICE) {
                sctk_device_init(topology, &sctk_devices[off], pci_dev, i);
                /* Set the ID of the device */
                sctk_devices[off].id = off;
                off++;
              }
            }
          }

          pci_dev = hwloc_get_next_pcidev(topology, pci_dev);
        }

        sctk_device_enrich_topology(topology);
        // hwloc_topology_export_xml(topology, "-");

        //*
        for (i = 0; i < sctk_devices_count; i++) {
          sctk_device_print(&sctk_devices[i]);
        }
        //*/
        /* Now initialize the device distance matrix */
        sctk_device_matrix_init();
}

void sctk_device_release()
{
	
	
}




sctk_device_t * sctk_device_get_from_handle( char * handle )
{
	int i;
	
	if( !handle )
		return NULL;
	
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


int sctk_device_get_id_from_handle( char * handle )
{
	sctk_device_t * dev = sctk_device_get_from_handle(  handle );
	
	if( !dev )
	{
		return -1;
	}

	return dev->device_id;
}

sctk_device_t ** sctk_device_get_from_handle_regexp( char * handle_reg_exp, int * count )
{
	if( !handle_reg_exp )
	{
		return  NULL;
	}

	/* For simplicity just allocate an array of the number of devices (make it NULL terminated) */
	sctk_device_t ** ret_dev = sctk_malloc( sizeof( sctk_device_t *) * ( sctk_devices_count + 1 ) );
	
	/* Set it to NULL */
	int i;
	for( i = 0 ; i < sctk_devices_count + 1; i++ )
		ret_dev[i] = NULL;
	
	/* Prepare the regexp */
	regex_t regexp;
	char msg_buff[100];
	
	int ret = regcomp( &regexp, handle_reg_exp, 0 );
	
	if( ret )
	{
		perror("regcomp");
		regerror( ret, &regexp, msg_buff, 100 );
		sctk_fatal("Could not compile device regexp %s ( %s )", handle_reg_exp , msg_buff);
	}
	
	
	/* Lets walk the devices to see who matches */
	int current_count = 0;
	
	for( i = 0 ; i < sctk_devices_count ; i++ )
	{
		ret = regexec( &regexp, sctk_devices[i].name, 0, NULL, 0 );
		
		if( ret == 0 )
		{
			/* Match then push the device */
			sctk_nodebug("Regex %s MATCH %s", handle_reg_exp, sctk_devices[i].name);
			ret_dev[current_count] = &sctk_devices[i];
			current_count++;
		}
		else if( ret == REG_NOMATCH )
		{
			/* No Match */
		}
		else
		{
			regerror( ret, &regexp, msg_buff, 100 );
			sctk_fatal("Could not execute device regexp %s on %s ( %s )", handle_reg_exp, sctk_devices[i].name, msg_buff);
		}
	}
	
	if( count )
		*count = current_count;
	
	/* Free the regexp */
	regfree( 	&regexp );
	
	return ret_dev;
}

/************************************************************************/
/* DISTANCE MATRIX                                                      */
/************************************************************************/

/** Retrieves a pointer to the  static device matrix */
sctk_device_matrix_t * sctk_device_matrix_get()
{
	return &sctk_device_matrix;
}

/** Return a pointer to cell */
static inline int * sctk_device_matrix_get_cell( int pu_id, int device_id )
{
	if( !sctk_device_matrix.distances )
		return NULL;
	
	if( (pu_id < 0)||( sctk_device_matrix.pu_count <= pu_id ) )
		return NULL;
	
	if( (device_id < 0)||( sctk_device_matrix.device_count <= device_id))
		return NULL;
	
	return&sctk_device_matrix.distances[pu_id*sctk_device_matrix.device_count+device_id];
}

/** Return a cell value */
static inline int sctk_device_matrix_get_value( int pu_id, int device_id )
{
	int * ret = sctk_device_matrix_get_cell( pu_id, device_id );
	
	if( !ret )
		return -1;
		
	return *ret;
}


/** Intializes the device matrix */
void sctk_device_matrix_init()
{
	sctk_device_matrix.device_count = sctk_devices_count;
	sctk_device_matrix.pu_count =  sctk_get_cpu_number();
	
	sctk_device_matrix.distances = sctk_malloc( sizeof( int ) * sctk_device_matrix.device_count  * sctk_device_matrix.pu_count );
	
	assume( sctk_device_matrix.distances != NULL );
	
	int i,j;
	
	/* For each device */
	for( i = 0 ; i < sctk_device_matrix.device_count; i++ )
	{
		hwloc_obj_t device_obj = sctk_devices[i].obj;
		
		/* For each PU */
		for( j = 0 ; j < sctk_device_matrix.pu_count; j++ )
		{
			int * cell = sctk_device_matrix_get_cell( j, i );
			
			if(!cell)
			{
				sctk_warning("Could not get cell");
				continue;
			}
			
			/* Compute the distance */
			*cell = sctk_topology_distance_from_pu( j , device_obj );

                        sctk_nodebug("Distance (PU %d, DEV %d (%s)) == %d", j,
                                     i, sctk_devices[i].name, *cell);
                }
        }
}

/** Get the closest device from PU matching the regexp "matching_regexp" */
sctk_device_t * sctk_device_matrix_get_closest_from_pu( int pu_id, char * matching_regexp )
{
  int dummy;
  sctk_device_t **device_list = sctk_device_matrix_get_list_closest_from_pu(
      pu_id, matching_regexp, &dummy);
  sctk_device_t *first = device_list[0];

  sctk_free(device_list);

  return first;
}

/**
 * Get the list of closest devices from PU matching the regexp.
 *
 * @param[in] pu_id the PU id used as starting point
 * @param[in] matching_regexp the regex as a string
 * @param[out] count_out the number of elements in the returned list
 *
 * @return
 *   - NULL if the regex does not match any device
 *   - a malloc'd array of devices, which should be freed after use.
 */
sctk_device_t **
sctk_device_matrix_get_list_closest_from_pu(int pu_id, char *matching_regexp,
                                            int *count_out) {
  /* Retrieve devices matching the regexp */
  int count;
  sctk_device_t **device_list =
      sctk_device_get_from_handle_regexp(matching_regexp, &count);
  if (count < 1) {
    return NULL;
  }

  sctk_device_t **ret_list = sctk_malloc(count * sizeof(sctk_device_t));
  int i;
  /* init the array */
  for (i = 0; i < count; i++)
    ret_list[i] = NULL;

  int minimal_distance = -1;
  *count_out = 0;

  /* for each found device */
  for (i = 0; i < count; i++) {
    sctk_device_t *dev = device_list[i];
    int distance = sctk_device_matrix_get_value(pu_id, dev->id);

    /* if the distance is lower than 0, skip it (should we ? ) */
    if (distance < 0) {
      continue;
    }

    /* Lookup for the minimum distance */
    if ((minimal_distance < 0) || (distance <= minimal_distance)) {
      minimal_distance = distance;
    }
  }

  /* now we have the minimum distance, look for any device which match with it
   */
  for (i = 0; i < count; i++) {
    sctk_device_t *dev = device_list[i];
    int distance = sctk_device_matrix_get_value(pu_id, dev->id);
    if (distance == minimal_distance) {
      ret_list[*count_out] = dev;
      (*count_out)++;
    }
  }

  /* don't forget to free the malloc'd vector from
   * sctk_device_get_from_handle_regexp() */
  sctk_free(device_list);
  return ret_list;
}

/** Return 1 if the devices matching the regexp are equidistant */
int sctk_device_matrix_is_equidistant(char * matching_regexp)
{
/* Retrieve devices matching the regexp */
	int count;
	sctk_device_t ** device_list = sctk_device_get_from_handle_regexp( matching_regexp, &count );
	
	if( !count )
	{
		return 1;
	}
	
	int i,j;
	int ref_distance = -1;
	
	for( i = 0 ; i < count ; i++ )
	{
	
		for( j = 0 ; j < sctk_device_matrix.pu_count ; j++ )
		{
			sctk_device_t * dev = device_list[i];
			int distance = sctk_device_matrix_get_value( j, dev->id );
			
			if( ref_distance < 0 )
			{
				ref_distance = distance;
			}
			else
			{
				if( distance != ref_distance )
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

/**
 * Increment the number of associated resource by 1
 * @param[in] device the device to update
 */
void sctk_device_attach_resource(sctk_device_t *device) {
  sctk_spinlock_lock(&device->res_lock);
  device->nb_res++;
  sctk_spinlock_unlock(&device->res_lock);
}

/**
 * Decrement the number of associated resource by 1
 * @param[in] device the device to update
 */
void sctk_device_detach_resource(sctk_device_t *device) {
  sctk_spinlock_lock(&device->res_lock);
  device->nb_res--;
  assert(device->nb_res >= 0);
  sctk_spinlock_unlock(&device->res_lock);
}

/**
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
 * sctk_device_attach_freest_device_from() and other
 * calls like attach/detach() routines.
 *
 * @param[in] device_list the list of devices where we will have to look for.
 * @param[in] count the list size
 *
 * @return
 *   - NULL if count is lower than 1
 *   - A pointer to the freest device from the list
 */
sctk_device_t *
sctk_device_attach_freest_device_from(sctk_device_t **device_list, int count) {
  int i;
  int freest_value = -1;
  sctk_device_t *freest_elem = NULL;

  if (count < 1)
    return NULL;

  /* for each selected device */
  for (i = 0; i < count; i++) {
    sctk_device_t *current = device_list[i];

    /* lock the counter */
    sctk_spinlock_lock(&current->res_lock);

    /* if it the first checked driver */
    if (freest_value < 0) {
      freest_value = current->nb_res;
      freest_elem = current;
      sctk_nodebug("First device: %d (%d)", freest_elem->device_id,
                   freest_elem->nb_res);
    } else if (current->nb_res < freest_value) {
      /* free the previous selected device */
      sctk_spinlock_unlock(&freest_elem->res_lock);
      freest_value = current->nb_res;
      freest_elem = current;
      sctk_nodebug("New best device: %d (%d)", freest_elem->device_id,
                   freest_elem->nb_res);
    } else {
      /* this device is not elected as the freest ones -> unlock */
      sctk_spinlock_unlock(&current->res_lock);
    }
  }

  assert(freest_elem != NULL);
  freest_elem->nb_res++;
  sctk_spinlock_unlock(&freest_elem->res_lock);

  return freest_elem;
}

/**
 * Retrieve the device with the smallest number of associated resources for a
 * matching regexp.
 *
 * @param[in] handle_reg_exp the regular expression as a string format
 * @return
 *   - NULL if the regexp is not found
 *   - A pointer to the elected device.
 */
sctk_device_t *sctk_device_attach_freest_device(char *handle_reg_exp) {
  int count;
  sctk_device_t **device_list =
      sctk_device_get_from_handle_regexp(handle_reg_exp, &count);
  sctk_device_t *device_res =
      sctk_device_attach_freest_device_from(device_list, count);

  sctk_free(device_list);
  return device_res;
}
