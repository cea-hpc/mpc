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

/*********************************
 * MPC TOPOLOGY OBJECT INTERFACE *
 *********************************/

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
			bad_parameter("MPC_PIN_PROCESSOR_LIST is set with"
			           " a different number of processor than"
					   " requested: %d in the list, %d requested",
					   sum,
					   processor_count );
			mpc_common_debug_abort();
		}

		hwloc_bitmap_copy( cpuset, pinning_constraints);
	}

	int err = hwloc_topology_restrict( target_topology,
									   cpuset,
									   HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES | HWLOC_RESTRICT_FLAG_ADAPT_IO );
	assume( !err );

	hwloc_bitmap_free(cpuset);
}

static void _topo_add_cpuid_to_pin_cpuset( hwloc_cpuset_t pin_cpuset,
										   int cpuid,
										   int processor_count )
{

	if ( cpuid > ( processor_count - 1 ) )
	{
		bad_parameter("Error in MPC_PIN_PROCESSOR_LIST:"
		           " processor %d is out of range "
				   "[0-%d]", cpuid, processor_count - 1);
	}

	hwloc_bitmap_set( pin_cpuset, cpuid );

}

hwloc_cpuset_t _mpc_topology_get_pinning_constraints(int processor_count)
{
	hwloc_cpuset_t pining_constraints = hwloc_bitmap_alloc();
	hwloc_bitmap_zero( pining_constraints );

	char *pinning_env = getenv( "MPC_PIN_PROCESSOR_LIST" );

	if ( pinning_env == NULL )
	{
		/* Nothing to do */
		return pining_constraints;
	}

	/* We require one process per node to apply pinning */
	int nodes_number = mpc_common_get_node_count();

	if ( mpc_common_get_process_count() != nodes_number )
	{
		bad_parameter( "MPC_PIN_PROCESSOR_LIST cannot be set if more than 1 process per node is set. process_number=%d node_number=%d", mpc_common_get_process_count(), nodes_number );
	}

	char * current = pinning_env;
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
		int err = hwloc_topology_restrict( target_topology,
			    						   topology_cpuset,
										   HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES );

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
				mpc_common_debug_warning("Process %d does not use all avaiable CPUs (%d/%d)",
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
		tmp[0] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_NODE, pu );
		tmp[1] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_SOCKET, pu );
		tmp[2] = hwloc_get_ancestor_obj_by_type( target_topology, HWLOC_OBJ_CORE, pu );

		if ( tmp[0] == NULL )
		{
			node_os_index = 0;
		}
		else
		{
			node_os_index = tmp[0]->os_index;
		}

		fprintf( fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
				 node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
	}

	fprintf( fd, "\tNUMA: %d\n", _mpc_topology_has_numa_nodes(target_topology) );

	if ( _mpc_topology_has_numa_nodes(target_topology) )
	{
		fprintf( fd, "\t\t* Node nb %d\n", _mpc_topology_get_numa_node_count(target_topology) );
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

	while ( parent->type != parent_type )
	{
		if ( !parent )
			break;

		parent = parent->parent;
	}

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
		return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_NODE, cpuid);
	}

	not_reachable();
}

hwloc_cpuset_t _mpc_topology_get_parent_socket_cpuset( hwloc_topology_t target_topo, int cpuid )
{
	return _mpc_topology_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_SOCKET, cpuid);
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
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_SOCKET );
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

	while ( parent->type != HWLOC_OBJ_SOCKET )
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
		if ( prefix_PU[i]->type == prefix_target[i]->type &&
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
	static mpc_common_spinlock_t pin_lock = SCTK_SPINLOCK_INITIALIZER;

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
	hwloc_obj_t currentNode = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_NODE, node);

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
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE ) != 0;
}


int _mpc_topology_get_numa_node_count(hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE );
}


int _mpc_topology_get_numa_node_from_cpu(hwloc_topology_t target_topo, const int cpuid)
{
	if ( _mpc_topology_has_numa_nodes( target_topo ) )
	{
		const hwloc_obj_t pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );
		assume( pu );
		const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_NODE, pu );

		return node->logical_index;
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
	hwloc_obj_type_t type = ( mpc_topology_has_numa_nodes() ) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
	int currentCPU_node_id = hwloc_get_ancestor_obj_by_type( target_topo, type, currentCPU )->logical_index;
	int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_SOCKET, currentCPU )->logical_index;
	int currentCPU_core_id = hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_CORE, currentCPU )->logical_index;

	fprintf( stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
			 cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id );

	for ( i = 0; i < nb_cpus; i++ )
	{
		hwloc_obj_t current = objs[i];

		fprintf( stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
				 i, neighborhood[i],
				 hwloc_get_ancestor_obj_by_type( target_topo, type, current )->logical_index,
				 hwloc_get_ancestor_obj_by_type( target_topo, HWLOC_OBJ_SOCKET, current )->logical_index,
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
        for(i = 0; i < HW_TYPE_COUNT; i++)
        {
            if(!strcmp(value,mpc_mpi_split_hardware_type_name[i]))
            {
                *type_split = mpc_topology_split_hardware_type_name[i];
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
    hwloc_obj_t new_ancestor;
    int previous_ancestor_level = -1;
    hwloc_obj_t pivot = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[root]);
    for(k = 0; k < size; k++)
    {
        if(k == root) continue;
        hwloc_obj_t core_compare = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[2*k]);
        ancestor = hwloc_get_common_ancestor_obj(topology, core_compare, pivot);   
        if(ancestor->depth < previous_ancestor_level || previous_ancestor_level < 0)
        {
            new_ancestor = ancestor;
            previous_ancestor_level = ancestor->depth;
        }
    }

    /* check oversuscribing */
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
    if(!split_over){
        for(k = 0; k < new_ancestor->arity; k++) /* each child ancestor */
        {
            int j; 
            hwloc_obj_t child = new_ancestor->children[k];
            hwloc_cpuset_t child_set;
            child_set = child->cpuset; 
            for(j = 0; j < size; j++) /* each rank calling split */
            {
                int is_inside = hwloc_bitmap_isincluded (hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuids[j*2])->cpuset, child_set);
                if(is_inside) /* if MPI process inside child ancestor */
                {
                    if(mpc_common_get_node_count() > 1)
                    {
                        /* create color with node id and pu id to be globaly unique */
                        char str_logical_idx[512];
                        char str_node_idx[512];
                        sprintf(str_logical_idx, "%d", k); 
                        sprintf(str_node_idx, "%d", mpc_common_get_node_rank()); 
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
    if(type_split ==  HWLOC_OBJ_MACHINE)
    {
        color = mpc_common_get_node_rank();
        return color;
    }
    hwloc_topology_t topology;
    topology = mpc_topology_global_get();
    hwloc_obj_t obj;
    int id_pu = mpc_thread_get_global_pu();
    hwloc_obj_t ancestor = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, id_pu);
    if(ancestor == NULL)
    {
        return -1;
    }
    if(type_split != HWLOC_OBJ_CACHE)
    {
        //__mpc_find_ancestor_by_type(topology, ancestor, type_split);
        while(ancestor->type != type_split)
        {
            ancestor = ancestor->parent;
        }
        if(ancestor == NULL)
        {
            return -1;
        }
    }
    else
    {
        int cache_lvl;
        if(!strcmp(value,"L3Cache"))
        {
            cache_lvl = 3;
        }
        if(!strcmp(value,"L2Cache"))
        {
            cache_lvl = 2;
        }
        if(!strcmp(value,"L1Cache"))
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
    color = ancestor->logical_index;
    return color;
}

/**************************
 * MPC TOPOLOGY ACCESSORS *
 **************************/

static hwloc_topology_t __mpc_module_topology;
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
static hwloc_topology_t* __extls_get_topology_addr(void)
{
	return &__mpc_module_topology;
}

static hwloc_topology_t* __extls_get_topology_global_addr(void)
{
	return &__mpc_module_topology_global;
}
#endif

void mpc_topology_init()
{
	char *xml_path = getenv( "MPC_SET_XML_TOPOLOGY_FILE" );

	hwloc_topology_init( &__mpc_module_topology );

	hwloc_topology_init( &__mpc_module_topology_global );

	hwloc_topology_set_flags( __mpc_module_topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES );

	if ( xml_path != NULL )
	{
		fprintf( stderr, "USE XML file %s\n", xml_path );
		hwloc_topology_set_xml( __mpc_module_topology, xml_path );
	}

	hwloc_topology_load( __mpc_module_topology );

	hwloc_topology_load( __mpc_module_topology_global );

	_mpc_topology_render_init();

	_mpc_topology_apply_mpc_process_constraints( __mpc_module_topology );

	/*  load devices */
	_mpc_topology_device_init( __mpc_module_topology );

#if defined(MPC_USE_EXTLS) && !defined(MPC_DISABLE_HLS)
	extls_set_topology_addr((void*(*)(void))__extls_get_topology_addr);
	extls_hls_topology_construct();
#endif
}

void mpc_topology_destroy( void )
{
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
	if ( __topo_cpu_pinning_caching_value < 0 )
	{
		return _mpc_topology_get_current_cpu(__mpc_module_topology);
	}
	else
	{
		return __topo_cpu_pinning_caching_value;
	}
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
