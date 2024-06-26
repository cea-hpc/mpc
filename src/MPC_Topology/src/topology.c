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
#include "topology.h"

#include "mpc_config.h"
#include "mpc_common_debug.h"
#include "machine_info.h"
#include "mpc_common_helper.h"
#include "mpc_common_flags.h"
#include "mpc_common_rank.h"
#include "mpc_common_flags.h"
#include "mpc_conf.h"
#include "topology_render.h"
#include "topology_device.h"

#ifdef MPC_USE_EXTLS
#include <extls.h>
#include <extls_hls.h>
#endif

#include <sctk_alloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/******************************
 * MPC TOPOLOGY CONFIGURATION *
 ******************************/

struct mpc_topology_config
{
	char hwloc_xml[MPC_CONF_STRING_SIZE];
	char pin_proc_list[MPC_CONF_STRING_SIZE];
  char latency_factors[MPC_CONF_STRING_SIZE];
  char bandwidth_factors[MPC_CONF_STRING_SIZE];
};


struct mpc_topology_config __mpc_topo_config = { 0 };

void __init_topology_config(void)
{
	mpc_conf_config_type_t *topo_conf = mpc_conf_config_type_init("topology",
																  PARAM("xml", __mpc_topo_config.hwloc_xml ,MPC_CONF_STRING, "HWLOC XML to be used to load machine topology"),
																  PARAM("bindings", __mpc_topo_config.pin_proc_list ,MPC_CONF_STRING, "List of cores to pin to ex: 1,3-4, only if one proc per node"),
																  PARAM("latencies", __mpc_topo_config.latency_factors ,MPC_CONF_STRING, "List of pair of hwloc_type & sleep factors for latency to apply on topology, ex: HWLOC_OBJ_L3,100,HWLOC_OBJ_CORE,1"),
																  PARAM("bandwidths", __mpc_topo_config.bandwidth_factors ,MPC_CONF_STRING, "List of pair of hwloc_type & sleep factors for bandwidth to apply on topology, ex: HWLOC_OBJ_L3,100,HWLOC_OBJ_CORE,1"),
																  NULL);


	mpc_conf_root_config_append("mpcframework", topo_conf, "MPC Topology Configuration");
}


void mpc_topology_module_register() __attribute__( (constructor) );

void mpc_topology_module_register()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Config Sources", "Config for MPC_Topology", __init_topology_config, 10);
}


/*********************************
 * MPC TOPOLOGY OBJECT INTERFACE *
 *********************************/

static inline void __restrict_topo_to_cpuset(hwloc_topology_t target_topology, hwloc_const_bitmap_t cpuset)
{

#if (HWLOC_API_VERSION < 0x00020000)
	int err = hwloc_topology_restrict( target_topology,
									   cpuset,
									   HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES | HWLOC_RESTRICT_FLAG_ADAPT_IO );
#else
	int err = hwloc_topology_restrict( target_topology,
									   cpuset,
									   HWLOC_RESTRICT_FLAG_ADAPT_IO );
#endif
    assume( !err );
}


void _mpc_topology_map_and_restrict_by_cpuset(hwloc_topology_t target_topology,
								const int processor_count,
								const unsigned int index_first_processor,
								hwloc_cpuset_t pinning_constraints)
{
	hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
	hwloc_bitmap_zero( cpuset );

	if ( hwloc_bitmap_iszero( pinning_constraints ) )
	{
		unsigned int i;
		for ( i = index_first_processor; i < index_first_processor + processor_count; ++i )
		{
            hwloc_obj_t pu = hwloc_get_obj_by_type( target_topology, HWLOC_OBJ_PU, i );
			assume(pu != NULL);
            hwloc_cpuset_t set = hwloc_bitmap_dup( pu->cpuset );
			hwloc_bitmap_singlify( set );
			hwloc_bitmap_or( cpuset, cpuset, set );
			hwloc_bitmap_free( set );
		}
	}
	else
	{
		unsigned int sum = hwloc_get_nbobjs_inside_cpuset_by_type( target_topology,
														  pinning_constraints,
														  HWLOC_OBJ_PU );

		if ( sum != mpc_common_get_flags()->processor_number )
		{
			bad_parameter("MPC_TOPOLOGY_BINDINGS is set with"
			           " a different number of processor than"
					   " requested: %d in the list, %d requested",
					   sum,
					   mpc_common_get_flags()->processor_number );
			mpc_common_debug_abort();
		}

		hwloc_bitmap_copy( cpuset, pinning_constraints);
	}

    __restrict_topo_to_cpuset(target_topology, cpuset);

	hwloc_bitmap_free(cpuset);
}

static void _topo_add_cpuid_to_pin_cpuset( hwloc_cpuset_t pin_cpuset,
										   int cpuid,
										   int processor_count )
{

	if ( cpuid > ( processor_count - 1 ) )
	{
		bad_parameter("Error in MPC_TOPOLOGY_BINDINGS:"
		           " processor %d is out of range "
				   "[0-%d]", cpuid, processor_count - 1);
	}

	hwloc_bitmap_set( pin_cpuset, cpuid );

}

hwloc_cpuset_t _mpc_topology_get_pinning_constraints(int processor_count)
{
	hwloc_cpuset_t pining_constraints = hwloc_bitmap_alloc();
	hwloc_bitmap_zero( pining_constraints );

	if ( strlen(__mpc_topo_config.pin_proc_list) == 0 )
	{
		/* Nothing to do */
		return pining_constraints;
	}

	/* We require one process per node to apply pinning */
	int nodes_number = mpc_common_get_node_count();

	if ( mpc_common_get_process_count() != nodes_number )
	{
		bad_parameter( "MPC_TOPOLOGY_BINDINGS cannot be set if more than 1 process per node is set. process_number=%d node_number=%d", mpc_common_get_process_count(), nodes_number );
	}

	char * current = __mpc_topo_config.pin_proc_list;
	char * token = NULL;

	while( (token = strtok_r(current, ",", &current)) != NULL)
	{
		char * hyfen = strstr(token, "-");

		if(hyfen)
		{
			/* Range definition */
			*hyfen = '\0';
			char * cv1 = token;
			char * cv2 = hyfen + 1;

			if(!strlen(cv1) || !strlen(cv2))
			{
				bad_parameter("Could not parse range-based pinning: '%s-%s'", cv1, cv2);
			}

			long v1 = mpc_common_parse_long(cv1);
			long v2 = mpc_common_parse_long(cv2);

			if( v2 < v1 )
			{
				bad_parameter("Bad range for pinning %d-%d"
							" expected ascending order\n", v1, v2);
			}

			int i;
			for( i = v1 ; i <= v2 ; i++)
			{
				_topo_add_cpuid_to_pin_cpuset(pining_constraints, i, processor_count);
			}

		}
		else
		{
			/* Regular number */
			long cpuid = mpc_common_parse_long(token);
			_topo_add_cpuid_to_pin_cpuset(pining_constraints, cpuid, processor_count);
		}
	}

	return pining_constraints;
}

void _mpc_topology_apply_smt_configuration(hwloc_topology_t target_topology,
					  		      int * processor_count)
{
restart_restrict:
	if(!mpc_common_get_flags()->enable_smt_capabilities)
	{
		unsigned int i;

		hwloc_cpuset_t topology_cpuset = hwloc_bitmap_alloc();
		hwloc_cpuset_t set = hwloc_bitmap_alloc();

		const unsigned int core_number = hwloc_get_nbobjs_by_type( target_topology,
																   HWLOC_OBJ_CORE );

		for ( i = 0; i < core_number; ++i )
		{
			hwloc_obj_t core = hwloc_get_obj_by_type( target_topology, HWLOC_OBJ_CORE, i );

			hwloc_bitmap_copy( set, core->cpuset );
			hwloc_bitmap_singlify( set );
			hwloc_bitmap_or( topology_cpuset, topology_cpuset, set );
		}

		/* restrict the topology to physical CPUs */
#if (HWLOC_API_VERSION < 0x00020000)
		int err = hwloc_topology_restrict( target_topology,
			    						   topology_cpuset,
										   HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES );
#else
		int err = hwloc_topology_restrict( target_topology,
			    						   topology_cpuset,
										   0 );
#endif

		if ( err )
		{
			hwloc_bitmap_free( topology_cpuset );
			hwloc_bitmap_free( set );
			mpc_common_get_flags()->enable_smt_capabilities = 1;
			mpc_common_debug_warning( "Failed to restrict topology : enabling SMT support" );
			goto restart_restrict;
		}

		hwloc_bitmap_free( set );
		hwloc_bitmap_free( topology_cpuset );
		*processor_count = hwloc_get_nbobjs_by_type( target_topology, HWLOC_OBJ_CORE );
	}
	else
	{
		*processor_count = hwloc_get_nbobjs_by_type( target_topology, HWLOC_OBJ_PU );
	}
}

void _mpc_topology_apply_mpc_process_constraints(hwloc_topology_t target_topology)
{
	int processor_count = 0;

	_mpc_topology_apply_smt_configuration(target_topology,
			 						  &processor_count);

	assume(processor_count > 0);

	hwloc_cpuset_t pinning_constraints = _mpc_topology_get_pinning_constraints(processor_count);

	int processor_per_process = processor_count;

	/* Now apply process restrictions */
	if (mpc_common_get_process_count() > 1)
	{
		/* Determine number of processes on this node */
		int local_process_count = mpc_common_get_local_process_count();
		int local_process_rank = mpc_common_get_local_process_rank();


		/* Determine processor number per process */
		processor_per_process = processor_count / local_process_count;
		int remaining_procs = processor_count % local_process_count;
		int start = processor_per_process * local_process_rank;

		if ( processor_per_process < 1 )
		{
			processor_per_process = 1;
			remaining_procs = 0;
			start = local_process_rank % processor_count;
		}

		if ( remaining_procs > 0 )
		{
			if ( local_process_rank < remaining_procs )
			{
				processor_per_process++;
				start += local_process_rank;
			}
			else
			{
				start += remaining_procs;
			}
		}

		_mpc_topology_map_and_restrict_by_cpuset(target_topology,
										processor_per_process,
										start,
										pinning_constraints);
	} /* share node */

	/* Apply processor number from command line if possible */
	{
		int requested_processor_per_process = mpc_common_get_flags()->processor_number;

		if ( requested_processor_per_process > 0 )
		{
			if ( requested_processor_per_process > processor_per_process )
			{
				bad_parameter( "Unable to allocate %d core (%d real cores)",
							requested_processor_per_process, processor_per_process );
			}
			else if ( requested_processor_per_process < processor_per_process )
			{
				mpc_common_debug_log("Process %d does not use all CPUs (%d/%d)",
							 mpc_common_get_process_rank(),
							 requested_processor_per_process, processor_per_process );

				_mpc_topology_map_and_restrict_by_cpuset(target_topology,
												requested_processor_per_process,
												0,
												pinning_constraints);
			}
		}
	}

	hwloc_bitmap_free(pinning_constraints);
}

void _mpc_topology_print( hwloc_topology_t target_topology, FILE *fd )
{
	char hostname[128];

#ifndef WINDOWS_SYS
	gethostname( hostname, 128 );
#else
	sprintf( hostname, "localhost" );
#endif

	static struct utsname utsname;
	uname( &utsname );

	int i;
	fprintf( fd, "Node %s: %s %s %s\n", hostname, utsname.sysname,
			 utsname.release, utsname.version );
	const int pu_number = hwloc_get_nbobjs_by_type( target_topology, HWLOC_OBJ_PU );

	for ( i = 0; i < pu_number; ++i )
	{
		hwloc_obj_t pu = hwloc_get_obj_by_type( target_topology, HWLOC_OBJ_PU, i );
		hwloc_obj_t tmp[3];
		unsigned int node_os_index;
#if ( HWLOC_API_VERSION < 0x00020000 )
		tmp[0] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_NODE, pu );
#else
		// As HWLOC_OBJ_NUMANODE has no attached cores since Hwloc 2.0.0
		// It it not possible to retrieve numa node id from pu ancestor
		tmp[0] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_NUMANODE, pu );
#endif
		tmp[1] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_PACKAGE, pu );
		tmp[2] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_CORE, pu );

#if ( HWLOC_API_VERSION < 0x00020000 )
		if ( tmp[0] == NULL )
		{
			node_os_index = 0;
		}
		else
		{
			node_os_index = tmp[0]->os_index;
		}
#else
		hwloc_obj_t parent = pu->parent;
		while ( parent && !parent->memory_arity )
		{
			parent = parent->parent; /* no memory child, walk up */
		}
		assume( parent );
		node_os_index = parent->logical_index;

#endif

		fprintf( fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
				 node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
	}

	fprintf( fd, "\tNUMA: %d\n", _mpc_topology_has_numa_nodes( target_topology ) );

	if ( _mpc_topology_has_numa_nodes( target_topology ) )
	{
		fprintf( fd, "\t\t* Node nb %d\n", _mpc_topology_get_numa_node_count( target_topology ) );
		int i;

		for ( i = 0; i < _mpc_topology_get_numa_node_count( target_topology ); ++i )
		{
			fprintf( fd, "Numa %d starts at %d\n", i, _mpc_topology_get_first_cpu_in_numa_node( target_topology, i ) );
		}
	}

	return;
}

int _mpc_topology_convert_os_pu_to_logical(hwloc_topology_t target_topo, int pu_os_id)
{
	hwloc_cpuset_t this_pu_cpuset = hwloc_bitmap_alloc();
	hwloc_bitmap_only( this_pu_cpuset, pu_os_id );

	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type( target_topo, this_pu_cpuset, HWLOC_OBJ_PU, 0 );

	hwloc_bitmap_free( this_pu_cpuset );

	if ( !pu )
	{
		return 0;
	}

	return pu->logical_index;
}

int _mpc_topology_convert_logical_pu_to_os(hwloc_topology_t target_topo, int cpuid)
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );

	if ( !target_pu )
	{
		/* Assume 0 in case of error */
		return 0;
	}

	return target_pu->os_index;
}


hwloc_cpuset_t _mpc_topology_get_pu_parent_cpuset_by_type(hwloc_topology_t target_topo,
												 hwloc_obj_type_t parent_type,
												 int cpuid)
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_PU, cpuid);
	assume( target_pu != NULL );

	hwloc_obj_t parent = target_pu->parent;

#if (HWLOC_API_VERSION < 0x00020000)
	while ( parent->type != parent_type )
	{
		if ( !parent )
	  	break;

	 	parent = parent->parent;
	}
#else
  if(parent_type == HWLOC_OBJ_NUMANODE)
  {
    while (parent && !parent->memory_arity)
    {
      parent = parent->parent; /* no memory child, walk up */
    }
  }
  else
  {
	  while ( parent->type != parent_type )
	  {
	  	if ( !parent )
	  		break;

	  	parent = parent->parent;
	  }
  }
#endif

	assume( parent != NULL );

	return parent->cpuset;
}


hwloc_const_cpuset_t _mpc_topology_get_process_cpuset(hwloc_topology_t target_topo)
{
	return hwloc_topology_get_allowed_cpuset( target_topo );
}

hwloc_cpuset_t _mpc_topology_get_parent_numa_cpuset( hwloc_topology_t target_topo, int cpuid )
{
	if ( !_mpc_topology_has_numa_nodes(target_topo) )
	{
		return (hwloc_cpuset_t)_mpc_topology_get_process_cpuset( target_topo );
	}
	else
	{
#if (HWLOC_API_VERSION < 0x00020000)
		return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_NODE, cpuid);
#else
		return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_NUMANODE, cpuid);
#endif
	}

	not_reachable();
}

hwloc_cpuset_t _mpc_topology_get_parent_socket_cpuset( hwloc_topology_t target_topo, int cpuid )
{
	return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_PACKAGE, cpuid);
}


hwloc_cpuset_t _mpc_topology_get_parent_core_cpuset(hwloc_topology_t target_topo, int cpuid)
{
	return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_CORE, cpuid);
}

hwloc_cpuset_t _mpc_topology_get_pu_cpuset(hwloc_topology_t target_topo,  int cpuid)
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_PU, cpuid);
	assume( target_pu != NULL );
	return target_pu->cpuset;
}

int _mpc_topology_get_socket_count( hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_PACKAGE );
}

int _mpc_topology_get_socket_id( hwloc_topology_t target_topo, int os_level )
{
	int cpu = mpc_topology_get_current_cpu();

	if ( cpu < 0 )
	{
		return cpu;
	}

	hwloc_obj_t target_pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpu );
	assume( target_pu != NULL );

	hwloc_obj_t parent = target_pu->parent;

	while ( parent->type != HWLOC_OBJ_PACKAGE )
	{
		if ( !parent )
			break;

		parent = parent->parent;
	}

	assume( parent != NULL );

	if ( os_level )
	{
		return parent->os_index;
	}
	else
	{
		return parent->logical_index;
	}
}


int _topo_get_first_pu_for_level( hwloc_obj_t obj, hwloc_cpuset_t roots )
{
	if ( obj->type == HWLOC_OBJ_PU )
	{
		hwloc_bitmap_or( roots, roots, obj->cpuset );
		return 1;
	}

	unsigned int i;

	int ret = 0;

	for ( i = 0; i < obj->arity; i++ )
	{
		ret = _topo_get_first_pu_for_level( obj->children[i], roots );

		if ( ret )
			break;
	}

	return ret;
}

hwloc_cpuset_t _mpc_topology_get_first_pu_for_level( hwloc_topology_t target_topo, hwloc_obj_type_t type )
{
	hwloc_cpuset_t roots = hwloc_bitmap_alloc();

	/* Handle the case where the node is not a numa node */
	if ( !mpc_topology_has_numa_nodes() )
	{
		/* Just tell that the expected value was MACHINE */
		type = HWLOC_OBJ_MACHINE;
	}

	int pu_count = hwloc_get_nbobjs_by_type( target_topo, type );

	int i;

	for ( i = 0; i < pu_count; i++ )
	{
		hwloc_obj_t obj = hwloc_get_obj_by_type( target_topo, type, i );
	 	_topo_get_first_pu_for_level( obj, roots );
	}

	if ( hwloc_bitmap_iszero( roots ) )
	{
		mpc_common_debug_fatal( "Did not find any roots for this level" );
	}

	return roots;
}

int __mpc_topology_get_distance_from_pu_fill_prefix( hwloc_obj_t *prefix,
										    hwloc_obj_t target_obj )
{
	/* Register in the tree */
	if ( !target_obj )
		return 0;

	int depth = 0;

	/* Compute depth as it is not set for
     * PCI devices ... */
	hwloc_obj_t cur = target_obj;

	while ( cur->parent )
	{
		depth++;
		cur = cur->parent;
	}

	/* Set depth +1 as NULL */
	prefix[depth + 1] = NULL;

	/* Prepare to walk up the tree */
	cur = target_obj;
	int cur_depth = depth;

	mpc_common_nodebug( "================\n" );

	while ( cur )
	{
		mpc_common_nodebug( "%d == %d == %s", cur_depth, cur->logical_index,
					  hwloc_obj_type_string( cur->type ) );
		prefix[cur_depth] = cur;
		cur_depth--;
		cur = cur->parent;
	}

	return depth;
}

int _mpc_topology_get_distance_from_pu(hwloc_topology_t target_topo, int source_pu, hwloc_obj_t target_obj )
{
	hwloc_obj_t source_pu_obj = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, source_pu );

	/* No such PU then no distance */
	if ( !source_pu_obj )
		return -1;

	/* Compute topology depth */
	int _mpc_topology_depth = hwloc_topology_get_depth( target_topo );

	/* Allocate prefix lists */
	hwloc_obj_t *prefix_PU =
		sctk_malloc( ( _mpc_topology_depth + 1 ) * sizeof( hwloc_obj_t ) );
	hwloc_obj_t *prefix_target =
		sctk_malloc( ( _mpc_topology_depth + 1 ) * sizeof( hwloc_obj_t ) );

	assume( prefix_PU != NULL );
	assume( prefix_target != NULL );

	/* Fill them with zeroes */
	int i;
	for ( i = 0; i < _mpc_topology_depth; i++ )
	{
		prefix_PU[i] = NULL;
		prefix_target[i] = NULL;
	}

	/* Compute the prefix of the two objects in the tree */
	int pu_depth =
	 __mpc_topology_get_distance_from_pu_fill_prefix( prefix_PU, source_pu_obj );
	int obj_depth =
	 __mpc_topology_get_distance_from_pu_fill_prefix( prefix_target, target_obj );

	/* Extract the common prefix */
	int common_prefix = 0;
	for ( i = 0; i < _mpc_topology_depth; i++ )
	{
		if ( prefix_PU[i] != NULL && prefix_PU[i]->type == prefix_target[i]->type &&
			 prefix_PU[i]->logical_index == prefix_target[i]->logical_index )
		{
			common_prefix = i;
		}
		else
		{
			break;
		}
	}

	/* From this we know that the distance is the sum of the two prefixes
         * without the common part plus one */
	int distance = ( pu_depth + obj_depth - ( common_prefix * 2 ) );

	mpc_common_nodebug( "Common prefix %d PU %d OBJ %d == %d", common_prefix,
				  pu_depth, obj_depth, distance );

	/* Free prefixes */
	sctk_free( prefix_PU );
	sctk_free( prefix_target );

	/* Return the distance */
	return distance;
}

/*! \brief Return the current core_id
*/
static inline int _mpc_topology_get_current_cpu(hwloc_topology_t target_topo)
{
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location( target_topo, set, HWLOC_CPUBIND_THREAD );

	assume( ret != -1 );
	assume( !hwloc_bitmap_iszero( set ) );

	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type( target_topo, set, HWLOC_OBJ_PU, 0 );

	if ( !pu )
	{
		hwloc_bitmap_free( set );
        return 0;
	}

	hwloc_bitmap_free( set );

	return pu->logical_index;
}

/*! \brief Return the current global core_id
*/
static inline int _mpc_topology_get_global_current_cpu(hwloc_topology_t target_topo)
{
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location( target_topo, set, HWLOC_CPUBIND_THREAD );

	assume( ret != -1 );
	assume( !hwloc_bitmap_iszero( set ) );

	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type( target_topo, set, HWLOC_OBJ_PU, 0 );

	if ( !pu )
	{
		hwloc_bitmap_free( set );
        return 0;
	}

	hwloc_bitmap_free( set );

	return pu->logical_index;
}

int _mpc_topology_get_pu_per_core_count(hwloc_topology_t target_topo, int cpuid)
{
	int pu_per_core;

	hwloc_obj_t core = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_CORE, cpuid);
	assert( core );

	pu_per_core = hwloc_get_nbobjs_inside_cpuset_by_type( target_topo, core->cpuset, HWLOC_OBJ_PU );
	assert( pu_per_core > 0 );

	return pu_per_core;
}


/* Restrict the binding to the current cpu_set */
void _mpc_topology_bind_to_process(hwloc_topology_t target_topo)
{
	int err;
	err = hwloc_set_cpubind( target_topo,
							 _mpc_topology_get_process_cpuset(target_topo),
							 HWLOC_CPUBIND_THREAD );
	assume( !err );
}

/*! \brief Bind the current thread. The function may return -1 if the thread
 * was previously bound to a core that was not managed by the HWLOC topology
 * @ param i The cpu_id to bind
 */
int _mpc_topology_bind_to_pu(hwloc_topology_t target_topo, int cpuid)
{
	static mpc_common_spinlock_t pin_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	if ( cpuid < 0 )
	{
		_mpc_topology_bind_to_process(target_topo);
		return -1;
	}

	mpc_common_spinlock_lock( &pin_lock);

	int ret = _mpc_topology_get_current_cpu(target_topo);

	hwloc_obj_t pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );
	assume( pu );

	int err = hwloc_set_cpubind( target_topo, pu->cpuset, HWLOC_CPUBIND_THREAD );

	if ( err )
	{
		const char *errmsg = strerror( errno );
		fprintf( stderr, "Bind to cpu: FAILED (%d, %s)\n", errno, errmsg );
	}

	mpc_common_spinlock_unlock( &pin_lock );
	return ret;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
 */
int _mpc_topology_get_first_cpu_in_numa_node(hwloc_topology_t target_topo, int node)
{
#if (HWLOC_API_VERSION < 0x00020000)
	hwloc_obj_t currentNode = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_NODE, node);
#else
	hwloc_obj_t currentNode = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_NUMANODE, node);

  // HWLOC_OBJ_NUMANODE has no children as it is not placed in the main tree
  // Need to go up to find the main node at an intersection that contains the cpuset
  hwloc_obj_t tmp = currentNode->parent;
  while (hwloc_obj_type_is_memory(tmp->type))
  {
    tmp = tmp->parent;
  }
  currentNode = tmp;
#endif

	while ( currentNode->type != HWLOC_OBJ_CORE )
	{
		currentNode = currentNode->children[0];
	}

	return currentNode->logical_index;
}


/*! \brief Return the total number of core for the process
*/
int _mpc_topology_get_pu_count(hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_PU );
}

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used in ethread
 */
int _mpc_topology_set_process_pu_count(hwloc_topology_t target_topo, int n)
{
	hwloc_cpuset_t empty_set = hwloc_bitmap_alloc();

	if ( n <= _mpc_topology_get_pu_count(target_topo) )
	{
		_mpc_topology_map_and_restrict_by_cpuset( target_topo,
										 n,
										 0,
										 empty_set);
	}

	hwloc_bitmap_free(empty_set);

	return n;
}


int _mpc_topology_has_numa_nodes(hwloc_topology_t target_topo)
{
#if (HWLOC_API_VERSION < 0x00020000)
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE ) != 0;
#else
  return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NUMANODE ) != 0;
#endif
}


int _mpc_topology_get_numa_node_count(hwloc_topology_t target_topo)
{
#if (HWLOC_API_VERSION < 0x00020000)
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE );
#else
  return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NUMANODE );
#endif
}


int _mpc_topology_get_numa_node_from_cpu(hwloc_topology_t target_topo, const int cpuid)
{
	if ( _mpc_topology_has_numa_nodes( target_topo ) )
	{
		const hwloc_obj_t pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );
		assume( pu );
#if (HWLOC_API_VERSION < 0x00020000)
		const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_NODE, pu );
		return node->logical_index;
#else
    hwloc_obj_t parent = pu->parent;
    while (parent && !parent->memory_arity)
    {
      parent = parent->parent; /* no memory child, walk up */
    }
    assume( parent );

    return parent->logical_index;
#endif

	}
	else
	{
		return 0;
	}
}

/* print the neighborhood*/
void _topo_print_cpu_neighborhood(hwloc_topology_t target_topo, int cpuid, int nb_cpus, int *neighborhood, hwloc_obj_t *objs)
{
	int i;
	hwloc_obj_t currentCPU = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );

#if (HWLOC_API_VERSION < 0x00020000)
	hwloc_obj_type_t type = ( mpc_topology_has_numa_nodes() ) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
	int currentCPU_node_id = hwloc_get_ancestor_obj_by_type( target_topo, type, currentCPU )->logical_index;
#else
	hwloc_obj_type_t type = ( mpc_topology_has_numa_nodes() ) ? HWLOC_OBJ_NUMANODE : HWLOC_OBJ_MACHINE;
	int currentCPU_node_id = _mpc_topology_get_numa_node_from_cpu(target_topo, cpuid);
#endif
	int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_PACKAGE, currentCPU )->logical_index;
	int currentCPU_core_id = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_CORE, currentCPU )->logical_index;

	fprintf( stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
			 cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id );

	for ( i = 0; i < nb_cpus; i++ )
	{
		hwloc_obj_t current = objs[i];

    hwloc_obj_t cur_node = hwloc_get_ancestor_obj_by_type( target_topo, type, current );
#if (HWLOC_API_VERSION >= 0x00020000)
    hwloc_obj_t parent = cur_node->parent;
    while (parent && !parent->memory_arity)
    {
      parent = parent->parent; /* no memory child, walk up */
    }
    assume( parent );
    cur_node = parent;
#endif

		fprintf( stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
				 i, neighborhood[i],
				 cur_node->logical_index,
				 hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_PACKAGE, current )->logical_index,
				 hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_CORE, current )->logical_index );
	}
}


/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
void _mpc_topology_get_pu_neighborhood(hwloc_topology_t target_topo, int cpuid, unsigned int nb_cpus, int *neighborhood)
{
	unsigned int i;
	hwloc_obj_t *objs;
	hwloc_obj_t currentCPU;
	unsigned nb_cpus_found;

	currentCPU = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );

	assume(currentCPU!=NULL);

	/* alloc size for retreiving objects. We could also use a static array */
	objs = sctk_malloc( nb_cpus * sizeof( hwloc_obj_t ) );
	assert( objs != NULL );

	/* The closest CPU is actually... the current CPU :-).
	* Set the closest CPU as the current CPU */
	objs[0] = currentCPU;

	/* get closest objects to the current CPU */
	nb_cpus_found = hwloc_get_closest_objs( target_topo, currentCPU, &objs[1], ( nb_cpus - 1 ) );
	/*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
	assume( ( nb_cpus_found + 1 ) == nb_cpus );

	/* fill the neighborhood variable */
	for ( i = 0; i < nb_cpus; i++ )
	{
		/*
		hwloc_obj_t current = objs[i];
		neighborhood[i] = hwloc_get_ancestor_obj_by_type(target_topo, HWLOC_OBJ_CORE, current)->logical_index;
		*/
		neighborhood[i] = objs[i]->logical_index;
	}

	/* uncomment for printing the returned result */
	//_topo_print_cpu_neighborhood(target_topo, cpuid, nb_cpus, neighborhood, objs);

	sctk_free( objs );
}

/***************************************
* MPC TOPOLOGY HARDWARE TOPOLOGY SPLIT*
***************************************/

static hwloc_obj_type_t __mpc_find_split_type(char *value, hwloc_obj_type_t *type_split)
{
	/* if new level added, change enum and arrays in mpc_topology.h accordingly */
	int i;

	for(i = 0; i < MPC_LOWCOMM_HW_TYPE_COUNT; i++)
	{
		if(!strcmp(value, mpc_topology_split_hardware_type_name[i]) )
		{
			*type_split = mpc_topology_split_hardware_hwloc_type[i];
			return 1;
		}
	}
	return 0;
}

int mpc_topology_unguided_compute_color(int *colors, int *cpuids, int size)
{
	int root = 0;
	int k;
	/* find common ancestor */
	hwloc_topology_t topology;

	topology = mpc_topology_global_get();
	hwloc_obj_t ancestor;
	hwloc_obj_t new_ancestor            = NULL;
	int         previous_ancestor_level = -1;
	hwloc_obj_t pivot = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[root]);

	for(k = 0; k < size; k++)
	{
		if(k == root)
		{
			continue;
		}
		hwloc_obj_t core_compare = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[2 * k]);
		ancestor = hwloc_get_common_ancestor_obj(topology, core_compare, pivot);
		if(ancestor->depth < previous_ancestor_level || previous_ancestor_level < 0)
		{
			new_ancestor            = ancestor;
			previous_ancestor_level = ancestor->depth;
		}
	}

	assume(new_ancestor != NULL);

	/* check oversubscribing */
	int split_over = 0;

	if(new_ancestor->type == HWLOC_OBJ_CORE || new_ancestor->type == HWLOC_OBJ_PU)
	{
		for(k = 0; k < size; k++)
		{
			colors[k] = -1;
		}
		split_over = 1;
	}

	/* find child common ancestor */
	if(!split_over)
	{
		for(k = 0; k < (int)new_ancestor->arity; k++) /* each child ancestor */
		{
			int            j;
			hwloc_obj_t    child = new_ancestor->children[k];
			hwloc_cpuset_t child_set;
			child_set = child->cpuset;
			for(j = 0; j < size; j++) /* each rank calling split */
			{
				int is_inside = hwloc_bitmap_isincluded(hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[j * 2])->cpuset, child_set);
				if(is_inside) /* if MPI process inside child ancestor */
				{
					if(mpc_common_get_node_count() > 1)
					{
						/* create color with node id and pu id to be globaly unique */
						char str_logical_idx[512];
						char str_node_idx[512];
						sprintf(str_logical_idx, "%d", k);
						sprintf(str_node_idx, "%d", mpc_common_get_node_rank() );
						strcat(str_node_idx, str_logical_idx);
						colors[j] = atoi(str_node_idx);
					}
					else
					{
						/* only need local object id to create color */
						colors[j] = k;
					}
				}
			}
		}
	}

	return 0;
}

int mpc_topology_guided_compute_color(char *value)
{
	hwloc_obj_type_t type_split;
	int ret = __mpc_find_split_type(value, &type_split);

	if(ret < 0)
	{
		return -1;
	}

	int color = -1;

	if(!ret) /* info value not find */
	{
		return -1;
	}
	if(type_split == HWLOC_OBJ_MACHINE)
	{
		color = mpc_common_get_node_rank();
		return color;
	}
#if (HWLOC_API_VERSION >= 0x00020000)
	// As HWLOC_OBJ_NUMANODE has no attached cores since Hwloc 2.0.0
	// It it not possible to retrieve numa node id from pu ancestor
	if(type_split == HWLOC_OBJ_NUMANODE)
	{
		hwloc_topology_t topology;
		topology = mpc_topology_global_get();
		int id_pu = mpc_topology_get_global_current_cpu();
		color = _mpc_topology_get_numa_node_from_cpu(topology, id_pu);
		return color;
	}
#endif
	hwloc_topology_t topology;
	topology = mpc_topology_global_get();

	int         id_pu    = mpc_topology_get_global_current_cpu();
	hwloc_obj_t ancestor = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, id_pu);
	if(ancestor == NULL)
	{
		return -1;
	}
#if (HWLOC_API_VERSION < 0x00020000)
	// As HWLOC_OBJ_CACHE has no specific level before Hwloc 2.0.0
	// It it not possible to retrieve specific level of cache id from pu ancestor
	if(type_split != HWLOC_OBJ_CACHE)
	{
		__mpc_find_ancestor_by_type(topology, ancestor, type_split);
		while(ancestor->type != type_split)
		{
			ancestor = ancestor->parent;
			if(ancestor == NULL)
			{
				return -1;
			}
		}
	}
	else
	{
		int cache_lvl;
		if(!strcmp(value, "L3Cache") )
		{
			cache_lvl = 3;
		}
		if(!strcmp(value, "L2Cache") )
		{
			cache_lvl = 2;
		}
		if(!strcmp(value, "L1Cache") )
		{
			cache_lvl = 1;
		}
		int cache_iterator = 0;
		if(ancestor->type == type_split)
		{
			cache_iterator++;
		}
		while(cache_lvl != cache_iterator)
		{
			ancestor = ancestor->parent;
			if(ancestor == NULL)
			{
				return -1;
			}
			if(ancestor->type == type_split)
			{
				cache_iterator++;
			}
		}
	}
#else
	while(ancestor->type != type_split)
	{
		ancestor = ancestor->parent;
		if(ancestor == NULL)
		{
			return -1;
		}
	}
#endif
	color = ancestor->logical_index;
	return color;
}

/**************************
* MPC TOPOLOGY ACCESSORS *
**************************/

static volatile int __mpc_module_topology_loaded = 0;
static hwloc_topology_t __mpc_module_topology;
static int __mpc_module_mcdram_node  = -1;
static int __mpc_module_avail_nvdimm = 0;
static hwloc_topology_t __mpc_module_topology_global;

hwloc_topology_t mpc_topology_get()
{
	return __mpc_module_topology;
}

hwloc_topology_t mpc_topology_global_get()
{
	return __mpc_module_topology_global;
}

#ifdef MPC_USE_EXTLS
static hwloc_topology_t *__extls_get_topology_addr(void)
{
	return &__mpc_module_topology;
}

#endif

void mpc_topology_init()
{
	hwloc_topology_init(&__mpc_module_topology);
	hwloc_topology_init(&__mpc_module_topology_global);

#if (HWLOC_API_VERSION < 0x00020000)
	hwloc_topology_set_flags(__mpc_module_topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
#else
	hwloc_topology_set_flags(__mpc_module_topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);
	hwloc_topology_set_io_types_filter(__mpc_module_topology, HWLOC_TYPE_FILTER_KEEP_ALL);
	hwloc_topology_set_type_filter(__mpc_module_topology, HWLOC_OBJ_GROUP, HWLOC_TYPE_FILTER_KEEP_ALL);
#endif

	if(strlen(__mpc_topo_config.hwloc_xml) )
	{
		fprintf(stderr, "USE XML file %s\n", __mpc_topo_config.hwloc_xml);

		if(hwloc_topology_set_xml(__mpc_module_topology, __mpc_topo_config.hwloc_xml) < 0 ||
				hwloc_topology_set_xml(__mpc_module_topology_global, __mpc_topo_config.hwloc_xml) < 0)
		{
			bad_parameter("MPC_TOPOLOGY_XML could not load HWLOC topology from %s", __mpc_topo_config.hwloc_xml);
		}

		hwloc_topology_set_flags(__mpc_module_topology       , HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM | hwloc_topology_get_flags(__mpc_module_topology));
		hwloc_topology_set_flags(__mpc_module_topology_global, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM | hwloc_topology_get_flags(__mpc_module_topology_global));
	}

	hwloc_topology_load(__mpc_module_topology);

	/* First make sure that the topology matches the allowed topology
	 * as what we are willing to do here is pin our cores */
	hwloc_const_bitmap_t allowed_topo = hwloc_topology_get_allowed_cpuset(__mpc_module_topology);

	__restrict_topo_to_cpuset(__mpc_module_topology, allowed_topo);

	hwloc_topology_load(__mpc_module_topology_global);

	_mpc_topology_render_init();

	_mpc_topology_apply_mpc_process_constraints(__mpc_module_topology);

	/*  load devices */
	_mpc_topology_device_init(__mpc_module_topology);


#if (HWLOC_API_VERSION >= 0x00020000)
	/* MCDRAM Detection */
	_mpc_topology_mcdram_detection(__mpc_module_topology);

	/* NVDIMM Detection*/
	_mpc_topology_nvdimm_detection(__mpc_module_topology);
#endif

#if defined(MPC_USE_EXTLS) && !defined(MPC_DISABLE_HLS)
	extls_set_topology_addr( (void *(*)(void) )__extls_get_topology_addr);
	extls_hls_topology_construct();
#endif

	__mpc_module_topology_loaded = 1;
}


#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
int _mpc_topology_get_effectors(char * input, int ** effectors_depth, long ** factors) {
  int size = 0;
  *effectors_depth = NULL;
  *factors = NULL;

  if(input == NULL || strlen(input) == 0) {
    return size;
  }

  for(unsigned long i = 0; i < strlen(input); i++) {
    size += (input[i] == ',');
  }
  if(!(size & 1)) {
    bad_parameter("Wrong number of parameter for latency/bandwidth sleep factors. Need to be pair of hwloc_type & long (type1,long1,type2,long2,...):\n\t%s\n", input);
  }
  size = (size + 1) / 2;

  hwloc_topology_t global_topology = mpc_topology_global_get();
  *effectors_depth = malloc(size * sizeof(int));
  *factors = malloc(size * sizeof(long));

  int current;
  char *str, *token, *saveptr, *endptr;
  for(str = input, current = 0; (token = strtok_r(str, ",", &saveptr)) != NULL; str = NULL, current++) {
    int ret = hwloc_type_sscanf_as_depth(token, NULL, global_topology, &((*effectors_depth)[current]));
    if(ret == HWLOC_TYPE_DEPTH_UNKNOWN || ret == HWLOC_TYPE_DEPTH_MULTIPLE) {
      bad_parameter("Unknown type for current topology in latency/bandwidth sleep factors: %s\n", token);
    }

    token = strtok_r(NULL, ",", &saveptr);
    long val = strtol(token, &endptr, 10);
    if((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
      bad_parameter("Failed to convert token to long in latency/bandwidth sleep factors: %s\n", token);
    }
    (*factors)[current] = val;
  }

  return size;
}


/**
 * Add an hwloc distance matrix to the global topology.
 * Primarily used for latency and bandwidth simulation.
 * \return \c 0 on success
 * \return \c -1 on error
 * \see hwloc_distances_add_create, hwloc_distances_add_values
 *      and hwloc_distances_add_commit from hwloc for more
 *      information on supplied parameters.
 */
int _mpc_topology_add_distance(hwloc_topology_t topology, int object_count,
    hwloc_obj_t* hwloc_objects, hwloc_uint64_t* distance_matrix,
    unsigned long hwloc_kind)
{
  hwloc_distances_add_handle_t handle;
  // Create the empty distance structure
  handle = hwloc_distances_add_create(topology, NULL, hwloc_kind, 0);

  if (!handle)
  {
    return -1;
  }

  // Add the latency/bandwidth/... distance matrix
  int err = hwloc_distances_add_values(topology, handle, object_count,
    hwloc_objects, distance_matrix, 0);

  if (!err)
  {
    // Insert distances into the topology
    err = hwloc_distances_add_commit(topology, handle, 0);
    if (!err)
    {
      // handle is already freed by hwloc_distances_add_values/commit on error
      sctk_free(handle);
    }
  }

  return err;
}

void mpc_topology_init_distance_simulation_factors(int * tab_cpuid, int size) {

  int * latency_effectors_depth;
  long * latency_factors;
  int latency_size = _mpc_topology_get_effectors(__mpc_topo_config.latency_factors, &latency_effectors_depth, &latency_factors);
  if (latency_size)
  {
    mpc_common_debug_warning("TOPOLOGY: Simulation latency factors have been set in your user configuration. "
        "Performance on send operations will be DEGRADED purposefully.");
  }

  int * bandwidth_effectors_depth;
  long * bandwidth_factors;
  int bandwidth_size = _mpc_topology_get_effectors(__mpc_topo_config.bandwidth_factors, &bandwidth_effectors_depth, &bandwidth_factors);
  if (bandwidth_size)
  {
    mpc_common_debug_warning("TOPOLOGY: Simulation bandwidth factors have been set in your user configuration. "
        "Performance on send operations will be DEGRADED purposefully.");
  }



  hwloc_topology_t global_topology = mpc_topology_global_get();
  int swap;

  int latency_hashtable[latency_size];
  for(int i = 0; i < latency_size; i++) {
    latency_hashtable[i] = i;
  }

  for(int i = 0; i < latency_size; i++) {
    for(int j = 0; j < latency_size - 1; j++) {
      if(latency_effectors_depth[j] > latency_effectors_depth[j+1]) {
        swap = latency_effectors_depth[j];
        latency_effectors_depth[j] = latency_effectors_depth[j+1];
        latency_effectors_depth[j+1] = swap;

        swap = latency_hashtable[j];
        latency_hashtable[j] = latency_hashtable[j+1];
        latency_hashtable[j+1] = swap;
      }
    }
  }

  int bandwidth_hashtable[bandwidth_size];
  for(int i = 0; i < bandwidth_size; i++) {
    bandwidth_hashtable[i] = i;
  }

  for(int i = 0; i < bandwidth_size; i++) {
    for(int j = 0; j < bandwidth_size - 1; j++) {
      if(bandwidth_effectors_depth[j] < bandwidth_effectors_depth[j+1]) {
        swap = bandwidth_effectors_depth[j];
        bandwidth_effectors_depth[j] = bandwidth_effectors_depth[j+1];
        bandwidth_effectors_depth[j+1] = swap;

        swap = bandwidth_hashtable[j];
        bandwidth_hashtable[j] = bandwidth_hashtable[j+1];
        bandwidth_hashtable[j+1] = swap;
      }
    }
  }



  hwloc_uint64_t * latency_matrix   = sctk_malloc(size * size * sizeof(hwloc_uint64_t));
  hwloc_uint64_t * bandwidth_matrix = sctk_malloc(size * size * sizeof(hwloc_uint64_t));

  for(int i = 0; i < size; i++) {
    hwloc_obj_t obj = hwloc_get_obj_by_type(global_topology, HWLOC_OBJ_PU, tab_cpuid[i * 2]);

    latency_matrix[i * size + i] = 0;
    bandwidth_matrix[i * size + i] = 0;
    for(int j = i+1; j < size; j++) {
      hwloc_obj_t obj2 = hwloc_get_obj_by_type(global_topology, HWLOC_OBJ_PU, tab_cpuid[j * 2]);
      int ancestor_depth = hwloc_get_common_ancestor_obj(global_topology, obj, obj2)->depth;

      latency_matrix[i * size + j] = 0;
      for(int k = 0; k < latency_size; k++) {
        if(ancestor_depth >= latency_effectors_depth[k]) {
          latency_matrix[i * size + j] = latency_factors[latency_hashtable[k]];
          break;
        }
      }

      bandwidth_matrix[i * size + j] = 0;
      for(int k = 0; k < bandwidth_size; k++) {
        if(ancestor_depth >= bandwidth_effectors_depth[k]) {
          bandwidth_matrix[i * size + j] = bandwidth_factors[latency_hashtable[k]];
          break;
        }
      }
    }
  }


  hwloc_obj_t objs[size];
  for(int i = 0; i < size; i++) {
    objs[i] = hwloc_get_obj_by_type(global_topology, HWLOC_OBJ_PU, tab_cpuid[i * 2]);
  }


  if(latency_size)
  {
    _mpc_topology_add_distance(global_topology, size, objs, latency_matrix,
        HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_LATENCY);
  }

  if(bandwidth_size) {
    _mpc_topology_add_distance(global_topology, size, objs, bandwidth_matrix,
        HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_BANDWIDTH);
  }


  sctk_free(latency_matrix);
  sctk_free(bandwidth_matrix);

  sctk_free(latency_effectors_depth);
  sctk_free(latency_factors);
  sctk_free(bandwidth_effectors_depth);
  sctk_free(bandwidth_factors);
}

int mpc_topology_has_simulation_latency() {
  return strlen(__mpc_topo_config.latency_factors) > 0;
}

int mpc_topology_has_simulation_bandwidth() {
  return strlen(__mpc_topo_config.bandwidth_factors) > 0;
}
#endif /* MPC_ENABLE_TOPOLOGY_SIMULATION */

#if (HWLOC_API_VERSION >= 0x00020000)
void _mpc_topology_mcdram_detection(hwloc_topology_t topology)
{
	hwloc_obj_t current = NULL;
	hwloc_obj_t prev    = NULL;

	while( (current = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NUMANODE, prev) ) )
	{
		if(current->subtype != NULL)
		{
			if(strcmp(current->subtype, "MCDRAM") == 0)
			{
				fprintf(stdout, "<Topology> %s Found ! (NUMA NODE #%d)\n", current->subtype, current->logical_index);
				__mpc_module_mcdram_node = current->logical_index;
			}
		}
		prev    = current;
		current = NULL;
	}
}

void _mpc_topology_nvdimm_detection(hwloc_topology_t topology)
{
	hwloc_obj_t current = NULL;
	hwloc_obj_t prev    = NULL;

	while( (current = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_OS_DEVICE, prev) ) )
	{
		if(current->subtype != NULL)
		{
			if(strcmp(current->subtype, "NVDIMM") == 0)
			{
				mpc_common_debug_log("<Topology> %s Found ! (OS Device #%d)\n", current->subtype, current->logical_index);
				++__mpc_module_avail_nvdimm;
			}
		}
		prev    = current;
		current = NULL;
	}
}

#endif

int mpc_topology_get_mcdram_node()
{
  return __mpc_module_mcdram_node;
}

int mpc_topology_has_nvdimm()
{
  return __mpc_module_avail_nvdimm;
}

void mpc_topology_destroy( void )
{
	__mpc_module_topology_loaded = 0;

	_mpc_topology_render_render();
	//_mpc_topology_device_release();

	hwloc_topology_destroy( __mpc_module_topology );

	hwloc_topology_destroy( __mpc_module_topology_global );
}

void mpc_topology_get_pu_neighborhood( int cpuid, unsigned int nb_cpus, int *neighborhood )
{
	_mpc_topology_get_pu_neighborhood( __mpc_module_topology, cpuid, nb_cpus, neighborhood );
}

int mpc_topology_get_numa_node_from_cpu(const int cpuid)
{
	return _mpc_topology_get_numa_node_from_cpu( __mpc_module_topology, cpuid );
}

int mpc_topology_get_numa_node_count()
{
	return _mpc_topology_get_numa_node_count( __mpc_module_topology);
}

int mpc_topology_get_socket_count()
{
  return _mpc_topology_get_socket_count( __mpc_module_topology);
}

int mpc_topology_has_numa_nodes(void)
{
	return _mpc_topology_has_numa_nodes(__mpc_module_topology);
}

int mpc_topology_set_pu_count( int n )
{
	return _mpc_topology_set_process_pu_count(__mpc_module_topology, n);
}

int mpc_topology_get_pu_count(void)
{
	return _mpc_topology_get_pu_count(__mpc_module_topology);
}

static __thread int __topo_cpu_pinning_caching_value = -1;

void mpc_topology_clear_cpu_pinning_cache()
{
	__topo_cpu_pinning_caching_value = -1;
}

void _set_cpu_pinning_cache(int logical_id)
{
	__topo_cpu_pinning_caching_value = logical_id;
}

int mpc_topology_bind_to_cpu(int cpuid)
{
	int ret = _mpc_topology_bind_to_pu(__mpc_module_topology, cpuid);

 	_set_cpu_pinning_cache(cpuid);

	return ret;
}

void mpc_topology_bind_to_process_cpuset(void)
{
	_mpc_topology_bind_to_process(__mpc_module_topology);
	mpc_topology_clear_cpu_pinning_cache();
}

int mpc_topology_get_current_cpu()
{
	if(!__mpc_module_topology_loaded)
	{
		return -1;
	}

	if ( __topo_cpu_pinning_caching_value < 0 )
	{
		return _mpc_topology_get_current_cpu(__mpc_module_topology);
	}
	else
	{
		return __topo_cpu_pinning_caching_value;
	}
}

//NOTE: used by sctk_topological_polling
int mpc_topology_is_loaded() {
	return __mpc_module_topology_loaded;
}

int mpc_topology_get_global_current_cpu()
{
    return _mpc_topology_get_global_current_cpu(__mpc_module_topology_global);
}

void mpc_topology_print(FILE *fd)
{
	_mpc_topology_print(__mpc_module_topology, fd);
}


int mpc_topology_get_ht_per_core(void)
{
	static int ret = -1;

	if(ret < 0)
	{
		hwloc_topology_t temp_topo;
		hwloc_topology_init( &temp_topo );
		hwloc_topology_load( temp_topo );

		ret = _mpc_topology_get_pu_per_core_count(temp_topo, 0);

		hwloc_topology_destroy(temp_topo);
	}

	return ret;
}

hwloc_const_cpuset_t mpc_topology_get_process_cpuset()
{
	return _mpc_topology_get_process_cpuset(__mpc_module_topology);
}

hwloc_cpuset_t mpc_topology_get_parent_numa_cpuset(int cpuid)
{
	return _mpc_topology_get_parent_numa_cpuset(__mpc_module_topology, cpuid);
}

hwloc_cpuset_t mpc_topology_get_parent_socket_cpuset(int cpuid)
{
	return _mpc_topology_get_parent_socket_cpuset(__mpc_module_topology, cpuid);
}

hwloc_cpuset_t mpc_topology_get_parent_core_cpuset(int cpuid)
{
	return _mpc_topology_get_parent_core_cpuset(__mpc_module_topology, cpuid);
}

hwloc_cpuset_t mpc_topology_get_pu_cpuset(int cpuid)
{
	return _mpc_topology_get_pu_cpuset(__mpc_module_topology, cpuid);
}

hwloc_cpuset_t mpc_topology_get_first_pu_for_level(hwloc_obj_type_t type)
{
	return _mpc_topology_get_first_pu_for_level(__mpc_module_topology, type);
}

int mpc_topology_convert_logical_pu_to_os( int cpuid )
{
	return _mpc_topology_convert_logical_pu_to_os(__mpc_module_topology, cpuid);
}

int mpc_topology_convert_os_pu_to_logical( int pu_os_id )
{
	return _mpc_topology_convert_os_pu_to_logical(__mpc_module_topology, pu_os_id);
}

#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
void mpc_topology_simulate_distance(int src_rank, int dest_rank, int size) {
  hwloc_topology_t topology = mpc_topology_global_get();

  struct hwloc_distances_s * latency_matrix;
  struct hwloc_distances_s * bandwidth_matrix;

  unsigned int nr1 = 1, nr2 = 1;
  float sleep_time = 0;

  if(mpc_topology_has_simulation_latency()) {
    hwloc_distances_get(topology, &nr1, &latency_matrix, HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_LATENCY, 0);
    if(nr1) {
      unsigned long latency = latency_matrix->values[src_rank + dest_rank * latency_matrix->nbobjs];
      hwloc_distances_release(topology, latency_matrix);

      sleep_time += latency;
    }
  }

  if(mpc_topology_has_simulation_bandwidth()) {
    hwloc_distances_get(topology, &nr2, &bandwidth_matrix, HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_BANDWIDTH, 0);
    if(nr2) {
      unsigned long bandwidth = bandwidth_matrix->values[src_rank + dest_rank * bandwidth_matrix->nbobjs];
      hwloc_distances_release(topology, bandwidth_matrix);

      if(bandwidth)
        sleep_time += (float)size / bandwidth;
    }
  }

  if(sleep_time)
    usleep(sleep_time);
}
#endif /* MPC_ENABLE_TOPOLOGY_SIMULATION */
