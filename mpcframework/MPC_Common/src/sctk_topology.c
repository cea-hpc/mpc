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
#include "sctk_config.h"
#include "sctk_topology.h"
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk.h"
#include "sctk_kernel_thread.h"
#include "sctk_device_topology.h"

#include "MPC_Common/include/sctk_rank.h"
#include "MPC_Common/include/mpc_topology_graph.h"
#include "MPC_Common/include/machine_info.h"
#include "MPC_Common/include/sctk_io_helper.h"


#ifdef MPC_USE_EXTLS
#include <extls.h>
#endif

#include <dirent.h>

#include <stdio.h>
#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SCTK_MAX_NODE_NAME 512

struct mpc_topology_context
{
	/* Set in sctk_topology_init */
	char *xml_specific_path;

	/* Set in sctk_topology_init */
	char hostname[SCTK_MAX_NODE_NAME];

	/* Initialized static */
	sctk_spinlock_t lock;

	/* Mainly restrict topology */
	hwloc_bitmap_t topology_cpuset;

	/* Set in sctk_topology_init */
	hwloc_topology_t topology_full;
};



static struct mpc_topology_context topology_ctx;


void mpc_topology_context_clear(void)
{
	memset(&topology_ctx, 0, sizeof(struct mpc_topology_context));
}


/* The topology of the machine + its cpuset */
static hwloc_topology_t topology;


/* Describe the full topology of the machine.
 * Only used for binding*/


const struct hwloc_topology_support *support;

hwloc_topology_t mpc_common_topology_get()
{
	return topology;
}

hwloc_topology_t mpc_common_topology_full()
{
	return topology_ctx.topology_full;
}


/* print a cpuset in a human readable way */
void print_cpuset( hwloc_bitmap_t cpuset )
{
	char buff[256];
	hwloc_bitmap_list_snprintf( buff, 256, cpuset );
	fprintf( stdout, "cpuset: %s\n", buff );
	fflush( stdout );
}

void
topo_map_and_restrict_by_cpuset(hwloc_topology_t target_topology,
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
		int sum = hwloc_get_nbobjs_inside_cpuset_by_type( target_topology,
														  pinning_constraints,
														  HWLOC_OBJ_PU );

		if ( sum != sctk_get_processor_nb() )
		{
			sctk_error("MPC_PIN_PROCESSOR_LIST is set with"
			           " a different number of processor than"
					   " requested: %d in the list, %d requested",
					   sum,
					   sctk_get_processor_nb() );
			sctk_abort();
		}

		hwloc_bitmap_copy( cpuset, pinning_constraints);
	}

	int err = hwloc_topology_restrict( target_topology,
									   cpuset,
									   HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES );
	assume( !err );

	hwloc_bitmap_free(cpuset);
}

static void _topo_add_cpuid_to_pin_cpuset( hwloc_cpuset_t pin_cpuset,
										   int cpuid,
										   int processor_count )
{

	if ( cpuid > ( processor_count - 1 ) )
	{
		sctk_error("Error in MPC_PIN_PROCESSOR_LIST:"
		           " processor %d is more than the max "
				   "number of cores %d", cpuid, processor_count );
	}

	hwloc_bitmap_set( pin_cpuset, cpuid );

}

hwloc_cpuset_t topo_get_pinning_constraints(int processor_count)
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
		sctk_fatal( "MPC_PIN_PROCESSOR_LIST cannot be set if more than 1 process per node is set. process_number=%d node_number=%d", mpc_common_get_process_count(), nodes_number );
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
				sctk_fatal("Could not parse range-based pinning: '%s-%s'", cv1, cv2);
			}

			long v1 = mpc_common_parse_long(cv1);
			long v2 = mpc_common_parse_long(cv2);

			if( v2 < v1 )
			{
				sctk_fatal("Bad range for pinning %d-%d"
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

void topo_apply_smt_configuration(hwloc_topology_t target_topology,
					  		      int * processor_count)
{
restart_restrict:
	if(!sctk_enable_smt_capabilities)
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
			sctk_enable_smt_capabilities = 1;
			sctk_warning( "Failed to restrict topology : enabling SMT support" );
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


// OUT topology_ctx.processor_count_on_node
// IN TOPOLOGY
// OUT topology_ctx.topology_cpuset
void
topo_apply_mpc_process_constraints(hwloc_topology_t target_topology)
{
	int processor_count = 0;
	sctk_only_once();

	topo_apply_smt_configuration(target_topology,
								 &processor_count);

	hwloc_cpuset_t pinning_constraints = topo_get_pinning_constraints(processor_count);

	if (mpc_common_get_process_count() > 1)
	{
		/* Determine number of processes on this node */
		int local_process_count = mpc_common_get_local_process_count();
		int local_process_rank = mpc_common_get_local_process_rank();


		/* Determine processor number per process */
		int processor_per_process = processor_count / local_process_count;
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


		topo_map_and_restrict_by_cpuset(target_topology,
										processor_per_process,
										start,
										pinning_constraints);

	} /* share node */

	/* Choose the number of processor used */
	{
		int processor_per_process;

		processor_per_process = sctk_get_processor_nb();

		if ( processor_per_process > 0 )
		{
			if ( processor_per_process > processor_count )
			{
				sctk_error( "Unable to allocate %d core (%d real cores)",
							processor_per_process, processor_count );
				/* we cannot exit with a sctk_abort() because thread
				* library is not initialized at this moment */
				/* sctk_abort (); */
				exit( 1 );
			}
			else if ( processor_per_process < processor_count )
			{
				sctk_warning( "Do not use the entire node %d on %d",
							  processor_per_process, processor_count );
				topo_map_and_restrict_by_cpuset(target_topology,
												processor_per_process,
												0,
												pinning_constraints);
			}
		}
	}

	hwloc_bitmap_free(pinning_constraints);
}


void topo_print( hwloc_topology_t target_topology, FILE *fd )
{
	int i;
	fprintf( fd, "Node %s: %s %s %s\n", topology_ctx.hostname, utsname.sysname,
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

	fprintf( fd, "\tNUMA: %d\n", topo_has_numa_nodes(target_topology) );

	if ( topo_has_numa_nodes(target_topology) )
	{
		fprintf( fd, "\t\t* Node nb %d\n", topo_get_numa_node_count(target_topology) );
	}

	return;
}

hwloc_topology_t* sctk_get_topology_addr(void)
{
	return &topology;
}

int topo_convert_os_pu_to_logical(hwloc_topology_t target_topo, int pu_os_id)
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

int topo_convert_logical_pu_to_os(hwloc_topology_t target_topo, int cpuid)
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );

	if ( !target_pu )
	{
		/* Assume 0 in case of error */
		return 0;
	}

	return target_pu->os_index;
}


hwloc_cpuset_t topo_get_pu_parent_cpuset_by_type(hwloc_topology_t target_topo,
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


hwloc_const_cpuset_t topo_get_process_cpuset(hwloc_topology_t target_topo)
{
	return hwloc_topology_get_allowed_cpuset( target_topo );
}

hwloc_cpuset_t topo_get_parent_numa_cpuset( hwloc_topology_t target_topo, int cpuid )
{
	if ( !topo_has_numa_nodes(target_topo) )
	{
		return (hwloc_cpuset_t)topo_get_process_cpuset( target_topo );
	}
	else
	{
		return topo_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_NODE, cpuid);
	}

	not_reachable();
}

hwloc_cpuset_t topo_get_parent_socket_cpuset( hwloc_topology_t target_topo, int cpuid )
{
	return topo_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_SOCKET, cpuid);
}


hwloc_cpuset_t topo_get_parent_core_cpuset(hwloc_topology_t target_topo, int cpuid)
{
	return topo_get_pu_parent_cpuset_by_type(target_topo, HWLOC_OBJ_CORE, cpuid);
}

hwloc_cpuset_t topo_get_pu_cpuset(hwloc_topology_t target_topo,  int cpuid)
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(target_topo, HWLOC_OBJ_PU, cpuid);
	assume( target_pu != NULL );
	return target_pu->cpuset;
}


int topo_get_socket_count( hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_SOCKET );
}

int topo_get_socket_id( hwloc_topology_t target_topo, int os_level )
{
	int cpu = mpc_common_topo_get_current_cpu();

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

hwloc_cpuset_t topo_get_first_pu_for_level( hwloc_topology_t target_topo, hwloc_obj_type_t type )
{
	hwloc_cpuset_t roots = hwloc_bitmap_alloc();

	/* Handle the case where the node is not a numa node */
	if ( !mpc_common_topo_has_numa_nodes() )
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
		sctk_fatal( "Did not find any roots for this level" );
	}

	return roots;
}

int _topo_get_distance_from_pu_fill_prefix( hwloc_obj_t *prefix,
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

	sctk_nodebug( "================\n" );

	while ( cur )
	{
		sctk_nodebug( "%d == %d == %s", cur_depth, cur->logical_index,
					  hwloc_obj_type_string( cur->type ) );
		prefix[cur_depth] = cur;
		cur_depth--;
		cur = cur->parent;
	}

	return depth;
}

int topo_get_distance_from_pu(hwloc_topology_t target_topo, int source_pu, hwloc_obj_t target_obj )
{
	hwloc_obj_t source_pu_obj = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, source_pu );

	/* No such PU then no distance */
	if ( !source_pu_obj )
		return -1;

	/* Compute topology depth */
	int topo_depth = hwloc_topology_get_depth( target_topo );

	/* Allocate prefix lists */
	hwloc_obj_t *prefix_PU =
		sctk_malloc( ( topo_depth + 1 ) * sizeof( hwloc_obj_t ) );
	hwloc_obj_t *prefix_target =
		sctk_malloc( ( topo_depth + 1 ) * sizeof( hwloc_obj_t ) );

	assume( prefix_PU != NULL );
	assume( prefix_target != NULL );

	/* Fill them with zeroes */
	int i;
	for ( i = 0; i < topo_depth; i++ )
	{
		prefix_PU[i] = NULL;
		prefix_target[i] = NULL;
	}

	/* Compute the prefix of the two objects in the tree */
	int pu_depth =
	 _topo_get_distance_from_pu_fill_prefix( prefix_PU, source_pu_obj );
	int obj_depth =
	 _topo_get_distance_from_pu_fill_prefix( prefix_target, target_obj );

	/* Extract the common prefix */
	int common_prefix = 0;
	for ( i = 0; i < topo_depth; i++ )
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

	sctk_nodebug( "Common prefix %d PU %d OBJ %d == %d", common_prefix,
				  pu_depth, obj_depth, distance );

	/* Free prefixes */
	sctk_free( prefix_PU );
	sctk_free( prefix_target );

	/* Return the distance */
	return distance;
}

/*! \brief Return the current core_id
*/
static inline int topo_get_current_cpu(hwloc_topology_t target_topo)
{
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location( target_topo, set, HWLOC_CPUBIND_THREAD );

	assume( ret != -1 );
	assume( !hwloc_bitmap_iszero( set ) );

	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type( target_topo, set, HWLOC_OBJ_PU, 0 );

	if ( !pu )
	{
		hwloc_bitmap_free( set );
		return -1;
	}

	hwloc_bitmap_free( set );

	return pu->logical_index;
}


int topo_get_pu_count(hwloc_topology_t target_topo)
{
	int core_number;

	int depth = hwloc_get_type_depth( target_topo, HWLOC_OBJ_PU );

	if ( depth == HWLOC_TYPE_DEPTH_UNKNOWN )
	{
		core_number = -1;
	}
	else
	{
		core_number = hwloc_get_nbobjs_by_depth( target_topo, depth );
	}

	return core_number;
}

int topo_get_pu_per_core_count(hwloc_topology_t target_topo, int cpuid)
{
	int pu_per_core;

#ifdef MPC_USE_EXTLS && !defined(MPC_DISABLE_HLS)
	extls_set_topology_addr((void*(*)(void))sctk_get_topology_addr);
#endif

	hwloc_obj_t core = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_CORE, cpuid);
	sctk_assert( core );

	pu_per_core = hwloc_get_nbobjs_inside_cpuset_by_type( target_topo, core->cpuset, HWLOC_OBJ_PU );
	assert( pu_per_core > 0 );

	return pu_per_core;
}


/* Restrict the binding to the current cpu_set */
void topo_bind_to_process(hwloc_topology_t target_topo)
{
	int err;
	err = hwloc_set_cpubind( target_topo,
							 topo_get_process_cpuset(target_topo),
							 HWLOC_CPUBIND_THREAD );
	assume( !err );
}

/*! \brief Bind the current thread. The function may return -1 if the thread
 * was previously bound to a core that was not managed by the HWLOC topology
 * @ param i The cpu_id to bind
 */
int topo_bind_to_cpu(hwloc_topology_t target_topo, int cpuid)
{

	if ( cpuid < 0 )
	{
		topo_bind_to_process(target_topo);
		return -1;
	}

	sctk_spinlock_lock( &topology_ctx.lock );

	int ret = topo_get_current_cpu(target_topo);

	hwloc_obj_t pu = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );
	assume( pu );

	int err = hwloc_set_cpubind( target_topo, pu->cpuset, HWLOC_CPUBIND_THREAD );

	if ( err )
	{
		const char *errmsg = strerror( errno );
		int supported = support->cpubind->set_thisthread_cpubind;
		char *msg = "Bind to cpu";
		fprintf( stderr, "%-40s: %sFAILED (%d, %s)\n", msg, supported ? "" : "X", errno, errmsg );
	}

	sctk_spinlock_unlock( &topology_ctx.lock );
	return ret;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
 */
int topo_get_first_cpu_in_numa_node(hwloc_topology_t target_topo, int node)
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
int topo_get_cpu_count(hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_PU );
}

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used in ethread
 */
int topo_set_process_cpu_count(hwloc_topology_t target_topo, int n)
{
	hwloc_cpuset_t empty_set = hwloc_bitmap_alloc();

	if ( n <= topo_get_cpu_count(target_topo) )
	{
		topo_map_and_restrict_by_cpuset( target_topo,
										 n,
										 0,
										 empty_set);
	}

	hwloc_bitmap_free(empty_set);

	return n;
}


int topo_has_numa_nodes(hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE ) != 0;
}


int topo_get_numa_node_count(hwloc_topology_t target_topo)
{
	return hwloc_get_nbobjs_by_type( target_topo, HWLOC_OBJ_NODE );
}


int topo_get_numa_node_from_cpu(hwloc_topology_t target_topo, const int cpuid)
{
	if ( topo_has_numa_nodes( target_topo ) )
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
	hwloc_obj_type_t type = ( mpc_common_topo_has_numa_nodes() ) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
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
void topo_get_cpu_neighborhood(hwloc_topology_t target_topo, int cpuid, unsigned int nb_cpus, int *neighborhood)
{
	unsigned int i;
	hwloc_obj_t *objs;
	hwloc_obj_t currentCPU;
	unsigned nb_cpus_found;

	currentCPU = hwloc_get_obj_by_type( target_topo, HWLOC_OBJ_PU, cpuid );

	/* alloc size for retreiving objects. We could also use a static array */
	objs = sctk_malloc( nb_cpus * sizeof( hwloc_obj_t ) );
	sctk_assert( objs != NULL );

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


/*

 TOPO INIT & RELEASE

*/


/*! \brief Initialize the topology module
*/
void sctk_topology_init()
{
	mpc_topology_context_clear();

	char *xml_path = getenv( "MPC_SET_XML_TOPOLOGY_FILE" );
	topology_ctx.xml_specific_path = xml_path;

	hwloc_topology_init( &topology );

	if ( xml_path != NULL )
	{
		fprintf( stderr, "USE XML file %s\n", xml_path );
		hwloc_topology_set_xml( topology, xml_path );
	}

	hwloc_topology_load( topology );
	hwloc_topology_init( &topology_ctx.topology_full );

	if ( xml_path != NULL )
	{
		hwloc_topology_set_xml( topology_ctx.topology_full, xml_path );
	}


	hwloc_topology_load( topology_ctx.topology_full );

	topology_graph_init();

	support = hwloc_topology_get_support( topology );

	sctk_only_once();

	topo_apply_mpc_process_constraints(topology);

	/*  load devices */
	sctk_device_load_from_topology( topology_ctx.topology_full );

#ifndef WINDOWS_SYS
	gethostname( topology_ctx.hostname, SCTK_MAX_NODE_NAME );
#else
	sprintf( topology_ctx.hostname, "localhost" );
#endif

	uname( &utsname );


}

/*! \brief Destroy the topology module
*/
void sctk_topology_destroy( void )
{
	topology_graph_render();
	hwloc_topology_destroy( topology );
}


/*

MPC Common Topology Interface

*/

void mpc_common_topo_get_cpu_neighborhood( int cpuid, unsigned int nb_cpus, int *neighborhood )
{
	topo_get_cpu_neighborhood( topology, cpuid, nb_cpus, neighborhood );
}

int mpc_common_topo_get_numa_node_from_cpu(const int cpuid)
{
	return topo_get_numa_node_from_cpu( topology, cpuid );
}

int mpc_common_topo_get_numa_node_count()
{
	return topo_get_numa_node_count( topology);
}

int mpc_common_topo_has_numa_nodes(void)
{
	return topo_has_numa_nodes(topology);
}

int sctk_set_cpu_number( int n )
{
	return topo_set_process_cpu_count(topology, n);
}

int mpc_common_topo_get_cpu_count(void)
{
	return topo_get_cpu_count(topology);
}

static __thread int __topo_cpu_pinning_caching_value = -1;

void mpc_common_topo_clear_cpu_pinning_cache()
{
	__topo_cpu_pinning_caching_value = -1;
}

void _set_cpu_pinning_cache(int logical_id)
{
	__topo_cpu_pinning_caching_value = logical_id;
}

int mpc_common_topo_bind_to_cpu(int cpuid)
{
	int ret = topo_bind_to_cpu(topology, cpuid);

 	_set_cpu_pinning_cache(cpuid);

	return ret;
}

void mpc_common_topo_bind_to_process_cpuset(void)
{
	topo_bind_to_process(topology);
	mpc_common_topo_clear_cpu_pinning_cache();
}

int mpc_common_topo_get_current_cpu()
{
	if ( __topo_cpu_pinning_caching_value < 0 )
	{
		return topo_get_current_cpu(topology);
	}
	else
	{
		return __topo_cpu_pinning_caching_value;
	}
}

void mpc_common_topo_print(FILE *fd)
{
	topo_print(topology, fd);
}

int mpc_common_topo_get_pu_count(void)
{
	return topo_get_pu_count(topology);
}


int mpc_common_topo_get_ht_per_core(void)
{
	static int ret = -1;

	if(ret < 0)
	{
		hwloc_topology_t temp_topo;
		hwloc_topology_init( &temp_topo );
		hwloc_topology_load( temp_topo );

		ret = topo_get_pu_per_core_count(temp_topo, 0);

		hwloc_topology_destroy(temp_topo);
	}

	return ret;
}

hwloc_const_cpuset_t mpc_common_topo_get_process_cpuset()
{
	return topo_get_process_cpuset(topology);
}

hwloc_cpuset_t mpc_common_topo_get_parent_numa_cpuset(int cpuid)
{
	return topo_get_parent_numa_cpuset(topology, cpuid);
}

hwloc_cpuset_t mpc_common_topo_get_parent_socket_cpuset(int cpuid)
{
	return topo_get_parent_socket_cpuset(topology, cpuid);
}

hwloc_cpuset_t mpc_common_topo_get_pu_cpuset(int cpuid)
{
	return topo_get_pu_cpuset(topology, cpuid);
}

hwloc_cpuset_t mpc_common_topo_get_parent_core_cpuset(int cpuid)
{
	return topo_get_parent_core_cpuset(topology, cpuid);
}

hwloc_cpuset_t mpc_common_topo_get_first_pu_for_level(hwloc_obj_type_t type)
{
	return topo_get_first_pu_for_level(topology, type);
}

int mpc_common_topo_convert_logical_pu_to_os( int cpuid )
{
	return topo_convert_logical_pu_to_os(topology, cpuid);
}

int mpc_common_topo_convert_os_pu_to_logical( int pu_os_id )
{
	return topo_convert_os_pu_to_logical(topology, pu_os_id);
}