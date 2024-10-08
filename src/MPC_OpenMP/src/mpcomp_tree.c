/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:59 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Stephane Bouhrour <stephane.bouhrour@exascale-computing.eu>        # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpcomp_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <mpc_thread.h>
#include <mpc_topology.h>
#include "mpcomp_alloc.h"
#include "mpcomp_core.h"
#include "mpc_common_asm.h"
#include "mpcomp_types.h"
#include "mpcomp_spinning_core.h"
#include "mpcompt_thread.h"
#include "mpcompt_frame.h"

#include "thread.h"

/**************
 * TASK UTILS *
 **************/

static inline int *__tree_array_ancestor_path( mpc_omp_instance_t *instance, const int globalRank, int *depth )
{
	int *path;
	mpc_omp_generic_node_t *gen_node;
	assert( instance );
	assert( depth );
	assert( globalRank >= 0 && globalRank < instance->tree_array_size );
	assert( instance->tree_array );
	gen_node = &( instance->tree_array[globalRank] );

	/* Extra node to preserve regular tree_shape */
	if ( gen_node->type == MPC_OMP_CHILDREN_NULL )
	{
		return NULL;
	}

	if ( gen_node->type == MPC_OMP_CHILDREN_LEAF )
	{
		path = gen_node->ptr.mvp->threads->tree_array_ancestor_path;
		assert( gen_node->ptr.mvp->threads );
		*depth = gen_node->ptr.mvp->threads->father_node->depth + 1;
	}
	else
	{
		assert( gen_node->type == MPC_OMP_CHILDREN_NODE );
		path =  gen_node->ptr.node->tree_array_ancestor_path;
		*depth = gen_node->ptr.node->depth;
	}

	return path;
}

/* Return the ith neighbour of the element at rank 'globalRank' in the
 * tree_array */
int mpc_omp_tree_array_get_neighbor( const int globalRank, const int index )
{
	int i, currentDepth = 0;
	int *path, *treeShape, *treeNumNodePerDepth;
	mpc_omp_thread_t *thread;
	mpc_omp_instance_t *instance;
	/* Retrieve the current thread information */
	assert( mpc_omp_tls );
	thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( thread->instance );
	instance = thread->instance;
	/* Shift ancestor array 0-depth -> 1-depth : Root is depth 0 */
	path = __tree_array_ancestor_path( instance, globalRank, &currentDepth ) + 1;
	assert( path && currentDepth > 0 );
	treeShape = instance->tree_base + 1;
	assert( treeShape );
	treeNumNodePerDepth = instance->tree_nb_nodes_per_depth;
	assert( treeNumNodePerDepth );
#if 0 /* DEBUG PRINT */
	char __treeShape_string[256], __treeNumNodePerDepth_string[256], __path_string[256];
	int _a, _b, _c, _tota, _totb, _totc;

	//    currentDepth++;

	for ( i = 0, _tota = 0, _totb = 0, _totc = 0; i < currentDepth; i++ )
	{
		_a = snprintf( __treeShape_string + _tota, 256 - _tota, " %d", treeShape[currentDepth - i - 1] );
		_b = snprintf( __treeNumNodePerDepth_string + _totb, 256 - _totb, " %d", treeNumNodePerDepth[i] );
		_c = snprintf( __path_string + _totc, 256 - _totc, " %d", path[currentDepth - i - 1] );
		_tota += _a;
		_totb += _b;
		_totc += _c;
	}

	fprintf( stderr, "%s:%d << %d >> treebase: %s nodePerDepth: %s, path : %s\n", __func__, __LINE__, thread->tree_array_rank, __treeShape_string, __treeNumNodePerDepth_string, __path_string );
#endif /* DEBUG PRINT */
	int r = index;
	int id = 0;
	int firstRank = 0;
	int nbSubleaves = 1;
	int v[currentDepth], res[currentDepth];

	for ( i = 0; i < currentDepth; i++ )
	{
		assert( currentDepth - 1 - i >= 0 );
		assert( currentDepth - 1 - i < thread->father_node->depth + 1 );
		assert( i <= thread->father_node->depth + 1 );
		const int base = treeShape[currentDepth - i - 1];
		const int level_size = treeNumNodePerDepth[i];
		/* Somme de I avec le codage de 0 dans le vecteur de l'arbre */
		v[i] = r % base;
		r /= base;
		/* Addition sans retenue avec le vecteur de n */
		res[i] = ( path[currentDepth - 1 - i] + v[i] ) % base;
		/* Calcul de l'identifiant du voisin dans l'arbre */
		id += res[i] * nbSubleaves;
		nbSubleaves *= base;
		firstRank += level_size;
	}

	return id + firstRank;
}

/* Return the ancestor of element at rank 'globalRank' at depth 'depth' */
int mpc_omp_tree_array_ancestor_get( const int globalRank, const int depth )
{
	int i, currentDepth = 0;
	int *path, *treeShape, *treeNumNodePerDepth;
	mpc_omp_thread_t *thread;
	mpc_omp_instance_t *instance;

	/* If it's the root, ancestor is itself */
	if ( !globalRank )
	{
		return 0;
	}

	/* Retrieve the current thread information */
	assert( mpc_omp_tls );
	thread = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( thread->instance );
	instance = thread->instance;
	path = __tree_array_ancestor_path( instance, globalRank, &currentDepth );
	assert( path && currentDepth > 0 );

	if ( currentDepth == depth )
	{
		return globalRank;
	}

	assert( depth >= 0 && depth < currentDepth );
	treeShape = instance->tree_base;
	assert( treeShape );
	treeNumNodePerDepth = instance->tree_nb_nodes_per_depth;
	assert( treeNumNodePerDepth );
	int ancestor_id = 0;
	int firstRank = 0;
	int nbSubLeaves = 1;

	for ( i = 0; i < depth; i++ )
	{
		ancestor_id += path[depth - 1 - i] * nbSubLeaves;
		nbSubLeaves *= treeShape[depth - 1 - i];
		firstRank += treeNumNodePerDepth[i];
	}

	return firstRank + ancestor_id;
}

/* Recursive call for checking neighbourhood from node n */
static void ___tree_task_check_neigborhood( mpc_omp_node_t *node )
{
	int i;
	mpc_omp_thread_t  __UNUSED__ *thread = NULL;
	assert( node );
	/* Retrieve the current thread information */
	assert( mpc_omp_tls );
	thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( thread->instance );
	assert( thread->instance->tree_nb_nodes_per_depth );

	switch ( node->child_type )
	{
		case MPC_OMP_CHILDREN_NODE:

			/* Call recursively for all children nodes */
			for ( i = 0; i < node->nb_children; i++ )
			{
				___tree_task_check_neigborhood( node->children.node[i] );
			}

			break;

		case MPC_OMP_CHILDREN_LEAF:

			/* All the children are leafs */
			for ( i = 0; i < node->nb_children; i++ )
			{
				mpc_omp_mvp_t *  __UNUSED__ mvp = node->children.leaf[i];
				assert( mvp && mvp->threads );
			}

			break;

		default:
			mpc_common_nodebug( "not reachable" );
	}
}

/* Check all neighbour of all nodes */
void _mpc_omp_tree_task_check_neigborhood( void )
{
	mpc_omp_thread_t *thread;
	/* Retrieve the current thread information */
	assert( mpc_omp_tls );
	thread = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( thread->instance );
	___tree_task_check_neigborhood( thread->instance->root );
}

/**************
 * TREE ARRAY *
 **************/


#if 0
static char *
_mpc_omp_tree_array_convert_array_to_string( int *tab, const int size )
{
	int i, ret, used;
	char *tmp, *string;

	if ( size == 0 )
	{
		return NULL;
	}

	string = ( char * ) malloc( sizeof( char ) * 1024 );
	assert( string );
	memset( string, 0, sizeof( char ) * 1024 );
	tmp = string;
	used = 0;
	ret = snprintf( tmp, 1024 - used, "%d", tab[0] );
	tmp += ret;
	used += ret;

	for ( i = 1; i < size; i++ )
	{
		ret = snprintf( tmp, 1024 - used, ",%d", tab[i] );
		tmp += ret;
		used += ret;
	}

	string[used] = '\0';
	return string;
}
#endif

static inline int *__tree_array_compute_shape( mpc_omp_node_t *node, int *tree_shape, const int size  )
{
	int i;
	int *node_tree_shape;
	assert( node );
	assert( tree_shape );
	const int node_tree_size = size - node->depth + 1;
	assert( node_tree_size );
	node_tree_shape = ( int * ) malloc( sizeof( int ) * node_tree_size );
	assert( node_tree_shape );
	memset( node_tree_shape, 0, sizeof( int ) * node_tree_size );
	const int start_depth = node->depth - 1;
	node_tree_shape[0] = 1;

	for ( i = 1; i < node_tree_size; i++ )
	{
		node_tree_shape[i] = tree_shape[start_depth + i];
		assert( node_tree_shape[i] );
	}

	return node_tree_shape;
}

static inline int *__tree_array_compute_num_nodes_for_depth( mpc_omp_node_t *node, int *total )
{
	int i;
	int *num_nodes_per_depth;
	int total_nodes;
	assert( node );
	assert( total );
	const int node_tree_size = node->tree_depth;
	assert( node_tree_size );
	num_nodes_per_depth = ( int * ) malloc( sizeof( int ) * node_tree_size );
	assert( num_nodes_per_depth );
	memset( num_nodes_per_depth, 0, sizeof( int ) * node_tree_size );
	assert( node->tree_base );
	num_nodes_per_depth[0] = 1;
	total_nodes = num_nodes_per_depth[0];

	for ( i = 1; i < node_tree_size ; i++ )
	{
		num_nodes_per_depth[i] = num_nodes_per_depth[i - 1];
		num_nodes_per_depth[i] *= node->tree_base[i];
		assert( num_nodes_per_depth[i] );
		total_nodes += num_nodes_per_depth[i];
	}

	*total = total_nodes;
	return num_nodes_per_depth;
}

static inline int *__tree_array_compute_cumulative_num_nodes_for_depth( mpc_omp_node_t *node )
{
	int i, j;
	int *cumulative_num_nodes;
	assert( node );
	const int node_tree_size = node->tree_depth;
	assert( node_tree_size );
	cumulative_num_nodes = ( int * ) malloc( sizeof( int ) * node_tree_size );
	assert( cumulative_num_nodes );
	memset( cumulative_num_nodes, 0, sizeof( int ) * node_tree_size );
	assert( node->tree_base );

	for ( i = 0; i < node_tree_size ; i++ )
	{
		cumulative_num_nodes[i] = 1;

		for ( j = i + 1; j < node_tree_size; j++ )
		{
			cumulative_num_nodes[i] *= node->tree_base[j];
			assert( cumulative_num_nodes[i] );
		}

		assert( cumulative_num_nodes[i] );
	}

	return cumulative_num_nodes;
}

static inline void __tree_array_update_children_node( const int first_idx,
        mpc_omp_node_t *node,
        mpc_omp_meta_tree_node_t *root )
{
	int i;
	mpc_omp_meta_tree_node_t *me;
	assert( node );
	assert( first_idx > 0 );
	assert( root );
	assert( node->child_type == MPC_OMP_CHILDREN_NODE );
	const int children_num = node->nb_children;
	me = &( root[node->global_rank] );
	assert( OPA_load_int( &( me->children_ready ) ) == children_num );

	for ( i = 0; i < children_num; i++ )
	{
		const int global_child_idx = first_idx + i;
		mpc_omp_meta_tree_node_t *child = &( root[global_child_idx] );
		node->children.node[i] = ( mpc_omp_node_t * ) child->user_pointer;
		assert( node->children.node[i]->local_rank == i );
		node->children.node[i]->father = node;
	}

	node->mvp = node->children.node[0]->mvp;

	/* ROOT issue */
	if ( !node->global_rank )
	{
		return;
	}

	assert( me->fathers_array_size == root[first_idx].fathers_array_size - 1 );
	assert( me->fathers_array );
	size_t j;

	for ( j = 0; j < me->fathers_array_size; j++ )
	{
		me->fathers_array[j] = root[first_idx].fathers_array[j];
	}
}

static inline void __tree_array_update_children_mvp(  const int first_idx,
        mpc_omp_node_t *node,
        mpc_omp_meta_tree_node_t *root )
{
	int i;
	mpc_omp_meta_tree_node_t *me;
	assert( node );
	assert( first_idx > 0 );
	assert( root );
	assert( node->child_type == MPC_OMP_CHILDREN_LEAF );
	const int children_num = node->nb_children;
	me = &( root[node->global_rank] );
	assert( OPA_load_int( &( me->children_ready ) ) == children_num );

	for ( i = 0; i < children_num; i++ )
	{
		const int global_child_idx = first_idx + i;
		mpc_omp_meta_tree_node_t *child = &( root[global_child_idx] );
		node->children.leaf[i] = ( mpc_omp_mvp_t * ) child->user_pointer;
		assert( node->children.leaf[i]->local_rank == i );
		node->children.leaf[i]->father = node;
	}

	node->mvp = node->children.leaf[0];

	/* MVP singleton issue */
	if ( !( me->fathers_array ) )
	{
		return;
	}

	assert( me->fathers_array_size == root[first_idx].fathers_array_size - 1 );
	size_t j;

	for ( j = 0; j < me->fathers_array_size; j++ )
	{
		me->fathers_array[j] = root[first_idx].fathers_array[j];
	}
}

/* Tree Array Implem */


#ifndef NDEBUG
	#include "sctk_pm_json.h"
#endif /* !NDEBUG */

static inline int __tree_array_get_parent_nodes_num_per_depth( const int *shape, const int depth )
{
	int i, count, sum;

	for ( count = 1, sum = 1, i = 0; i < depth; i++ )
	{
		count *= shape[i];
		sum += count;
	}

	return sum;
}

static inline int __tree_array_get_children_nodes_num_per_depth( const int *shape, const int top_level, const int depth )
{
	int i, count;

	for ( count = 1, i = depth; i < top_level; i++ )
	{
		count *= shape[i];
	}

	return count;
}

static inline int __tree_array_get_stage_nodes_num_per_depth( const int *shape, __UNUSED__ const int top_level, const int depth )
{
	int i, count;

	for ( count = 1, i = 1; i < depth; i++ )
	{
		count *= shape[i - 1];
	}

	return count;
}

#if 0
static inline int *
_mpc_omp_tree_array_get_stage_nodes_num_depth_array( const int *shape, const int top_level, const int depth )
{
	int i;
	int *count;
	count = ( int * ) mpc_omp_alloc( sizeof( int ) * depth );
	assert( count );
	memset( count, 0, sizeof( int ) * depth );

	for ( count[0] = 1, i = 1; i < depth; i++ )
	{
		count[i] = count[i - 1] * shape[i - 1];
	}

	return count;
}
#endif

static inline int __tree_array_get_node_depth( const int *shape, const int top_level, const int rank )
{
	int count, sum, i;

	for ( count = 1, sum = 1, i = 0; i < top_level; i++ )
	{
		if ( rank < sum )
		{
			break;
		}

		count *= shape[i];
		sum += count;
	}

	return i;
}


static inline int __tree_array_global_to_stage( const int *shape, const int top_level, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( top_level >= 0 );
	const int depth = __tree_array_get_node_depth( shape, top_level, rank );

	if ( !rank )
	{
		return 0;
	}

	const int prev_nodes_num = __tree_array_get_parent_nodes_num_per_depth(  shape, depth - 1 );
	const int stage_rank = rank - prev_nodes_num;
	assert( rank >= prev_nodes_num );
	return stage_rank;
}

static inline int __tree_array_stage_to_global( const int *shape, const int depth, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( depth >= 0 );

	if ( !depth )
	{
		return 0;
	}

	const int prev_nodes_num = __tree_array_get_parent_nodes_num_per_depth(  shape, depth - 1 );
	return prev_nodes_num + rank;
}

static inline int __tree_array_stage_to_local( const int *shape, const int depth, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( depth >= 0 );

	if ( !rank )
	{
		return 0;
	}

	return rank % shape[depth - 1];
}

static inline int __tree_array_global_to_local( const int *shape, const int top_level, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( top_level >= 0 );

	if ( !rank )
	{
		return 0;
	}

	const int depth = __tree_array_get_node_depth( shape, top_level, rank );
	const int stage_rank = __tree_array_global_to_stage( shape, top_level, rank );
	return __tree_array_stage_to_local( shape, depth, stage_rank );
}

static inline int *__tree_array_get_rank( const int *shape, const int top_level, const int rank )
{
	int i, current_rank;
	int *tree_array;
	assert( shape );
	assert( rank >= 0 );
	assert( top_level >= 0 );
	tree_array = ( int * ) mpc_omp_alloc( sizeof( int ) * top_level );
	assert( tree_array );
	memset( tree_array, 0, sizeof( int ) * top_level );

	for ( current_rank = rank, i = top_level - 1; i >= 0; i-- )
	{
		const int stage_rank = __tree_array_global_to_stage( shape, top_level, current_rank );
		tree_array[i] = stage_rank % shape[ i ];
		current_rank = __tree_array_stage_to_global( shape, top_level, stage_rank / shape[i] );
	}

	return tree_array;
}

static inline int *__tree_array_get_tree_cumulative( const int *shape, int top_level )
{
	int i, j;
	int *tree_cumulative;
	tree_cumulative = ( int * ) mpc_omp_alloc( sizeof( int ) * top_level );
	assert( tree_cumulative );
	memset( tree_cumulative, 0, sizeof( int ) * top_level );

	for ( i = 0; i < top_level - 1; i++ )
	{
		tree_cumulative[i] = 1;

		for ( j = 0; j < top_level; j++ )
		{
			tree_cumulative[i] *= shape[j];
		}
	}

	tree_cumulative[top_level - 1] = 1;
	return tree_cumulative;
}

static inline int *__tree_array_get_father_array_by_depth( const int *shape, const int top_level, const int rank )
{
	int i;
	int tmp_rank = -1;
	int *parents_array = NULL;
	int depth = __tree_array_get_node_depth( shape, top_level, rank );

	if ( !rank )
	{
		return NULL;
	}

	if ( top_level == 0 )
	{
		depth = 1;
	}

	tmp_rank = rank;
	parents_array = ( int * ) mpc_omp_alloc( sizeof( int ) * depth );
	assert( parents_array );

	for ( i = depth - 1; i > 0; i-- )
	{
		const int level_up_nodes_num = __tree_array_get_parent_nodes_num_per_depth( shape, i - 1 );
		const int stage_node_rank = __tree_array_global_to_stage( shape, depth, tmp_rank );
		tmp_rank = stage_node_rank / shape[i] + level_up_nodes_num;
		parents_array[i] = tmp_rank;
	}

	parents_array[0] = 0; // root
	return parents_array;
}

#if 0
static inline int *
_mpc_omp_tree_array_get_father_array_by_level( const int *shape, const int top_level, const int rank )
{
	int i;
	int tmp_rank = -1;
	int *parents_array = NULL;
	int nodes_num = __tree_array_get_parent_nodes_num_per_depth( shape, top_level );
	const int depth = __tree_array_get_node_depth( shape, top_level, rank );

	if ( !rank )
	{
		return NULL;
	}

	tmp_rank = rank;
	parents_array = ( int * ) mpc_omp_alloc( sizeof( int ) * depth );
	assert( parents_array );

	for ( i = depth - 1; i > 0; i-- )
	{
		int distance = i - ( depth - 1 ); /* translate i [ a, b] to i [ 0, b - a] */
		const int level_up_nodes_num = __tree_array_get_parent_nodes_num_per_depth( shape, i - 1 );
		const int stage_node_rank = __tree_array_global_to_stage( shape, depth, tmp_rank );
		tmp_rank = stage_node_rank / shape[i] + level_up_nodes_num;
		parents_array[distance] = tmp_rank;
	}

	parents_array[0] = 0; // root
	return parents_array;
}
#endif


static inline int *__tree_array_get_first_child_array( const int *shape, const int top_level, const int rank )
{
	int i, last_father_rank, last_stage_size;
	int *first_child_array = NULL;
	const int depth = __tree_array_get_node_depth( shape, top_level, rank );
	const int children_levels = top_level - depth;

	if (  !children_levels )
	{
		return NULL;
	}

	first_child_array = ( int * ) mpc_omp_alloc( sizeof( int ) * children_levels );
	assert( first_child_array );
	memset( first_child_array, 0,  sizeof( int ) * children_levels );
	last_father_rank = __tree_array_global_to_stage( shape, top_level, rank );

	for ( i = 0; i < children_levels; i++ )
	{
		last_stage_size = __tree_array_get_parent_nodes_num_per_depth( shape, depth + i );
		last_father_rank = last_stage_size + shape[depth + i] * last_father_rank;
		first_child_array[i] = last_father_rank;
		last_father_rank = __tree_array_global_to_stage( shape, top_level, last_father_rank );
	}

	return first_child_array;
}

#if 0
static inline int *
_mpc_omp_tree_array_get_children_num_array( const int *shape, const int top_level, const int rank )
{
	int i;
	int *children_num_array = NULL;
	const int depth = __tree_array_get_node_depth( shape, top_level, rank );
	const int children_levels = top_level - depth;

	if (  !children_levels )
	{
		return NULL;
	}

	children_num_array = ( int * ) mpc_omp_alloc( sizeof( int ) * children_levels );
	assert( children_num_array );
	memset( children_num_array, 0,  sizeof( int ) * children_levels );

	for ( i = 0; i < children_levels; i++ )
	{
		children_num_array[i] = shape[depth + i];
	}

	return children_num_array;
}
#endif

static inline int __tree_array_compute_thread_min_compact( const int *shape, const int top_level, const int rank )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_cumulative, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, top_level, rank );
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, top_level, rank );
		tmp_cumulative = __tree_array_get_children_nodes_num_per_depth( shape, top_level, depth );
		count = tmp_local_rank;
		count *= ( depth < top_level ) ? tmp_cumulative : 1;

		for ( i = 0; i < depth; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, top_level, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, top_level, node_parents[i] );
			tmp_cumulative = __tree_array_get_children_nodes_num_per_depth( shape, top_level, parent_depth );
			count += tmp_local_rank * tmp_cumulative;
		}

		free( node_parents );
	}
	else //Root Node
	{
		count = 0; // Root
	}

	return count;
}

static inline int __tree_array_compute_thread_min_balanced( const int *shape, const int top_level, const int rank, const int core_depth )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_cumulative, tmp_nodes_per_depth, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, top_level, rank );
		const int *tmp_cumulative_core_array = __tree_array_get_tree_cumulative( shape, top_level );
		const int tmp_cumulative_core = tmp_cumulative_core_array[depth - 1];
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, top_level, rank );

		if ( depth - 1 < core_depth )
		{
			tmp_cumulative = tmp_cumulative_core_array[depth - 1];
			count = tmp_local_rank * tmp_cumulative / tmp_cumulative_core;
		}
		else
		{
			tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, top_level, depth - 1 );
			count = tmp_local_rank * tmp_nodes_per_depth;
		}

		for ( i = 0; i < depth - 1; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, top_level, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, top_level, node_parents[i] );

			if ( parent_depth - 1 < core_depth )
			{
				tmp_cumulative = tmp_cumulative_core_array[depth - ( i + 1 )];
				count += ( tmp_local_rank * tmp_cumulative / tmp_cumulative_core );
			}
			else
			{
				tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, top_level, parent_depth - 1 );
				count += ( tmp_local_rank * tmp_nodes_per_depth );
			}
		}

		free( node_parents );
	}
	else //Root Node
	{
		count = 0;
	}

	return count;
}

#if 0
static inline int
_mpcomp_tree_array_update_thread_openmp_min_rank_scatter ( const int father_stage_size, const int father_min_index, const int node_local_rank )
{
	return father_min_index + node_local_rank * father_stage_size;
}
#endif

static inline int __tree_array_compute_thread_min_scatter( const int *shape, const int top_level, const int rank )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_nodes_per_depth, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, top_level, rank );
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, top_level, rank );
		tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, top_level, depth );
		count = tmp_local_rank * tmp_nodes_per_depth;

		for ( i = 0; i < depth; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, top_level, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, top_level, node_parents[i] );
			tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, top_level, parent_depth );
			count += tmp_local_rank * tmp_nodes_per_depth;
		}

		free( node_parents );
	}
	else //Root Node
	{
		count = 0;
	}

	return count;
}

int *mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int top_level, const int rank, const int core_depth )
{
	int index;
	int *min_index = NULL;
	min_index = ( int * ) mpc_omp_alloc( sizeof( int ) * MPC_OMP_AFFINITY_NB );
	assert( min_index );
	memset( min_index, 0, sizeof( int ) * MPC_OMP_AFFINITY_NB );
	/* MPC_OMP_AFFINITY_COMPACT  */
	index = __tree_array_compute_thread_min_compact( shape, top_level, rank );
	min_index[MPC_OMP_AFFINITY_COMPACT] = index;
	/* MPC_OMP_AFFINITY_SCATTER  */
	index = __tree_array_compute_thread_min_scatter( shape, top_level, rank );
	min_index[MPC_OMP_AFFINITY_SCATTER] = index;
	/* MPC_OMP_AFFINITY_BALANCED */
	index = __tree_array_compute_thread_min_balanced( shape, top_level, rank, core_depth );
	min_index[MPC_OMP_AFFINITY_BALANCED] = index;
	return min_index;
}

#if 0
static inline int _mpc_omp_tree_array_find_next_running_stage_depth(   const int *tree_shape,
        const int start_depth,
        const int top_level,
        const int num_requested_mvps,
        int *num_assigned_mvps )
{
	int i;
	int cumulative_children_num = 1;

	for ( i = start_depth; i < top_level; i++ )
	{
		cumulative_children_num *= tree_shape[i];

		if ( cumulative_children_num >= num_requested_mvps )
		{
			break;
		}
	}

	*num_assigned_mvps = cumulative_children_num;
	return i;
}
#endif

static inline int __tree_node_init( mpc_omp_meta_tree_node_t *root, const int *tree_shape, const int top_level, const int rank )
{
	int i, j, father_rank;
	mpc_omp_meta_tree_node_t *me;
	mpc_omp_node_t *new_node, *root_node;
	assert( root );
	assert( tree_shape );
	me = &( root[rank] );
	root_node = ( mpc_omp_node_t * )root[0].user_pointer;
	assert( root_node );

	if ( !( me->user_pointer )  )
	{
		assert( rank );
		me->type = MPC_OMP_META_TREE_NODE;
		mpc_omp_node_t *new_alloc_node = ( mpc_omp_node_t * ) mpc_omp_alloc( sizeof( mpc_omp_node_t ) );
		assert( new_alloc_node );
		memset( new_alloc_node, 0, sizeof( mpc_omp_node_t ) );
		me->user_pointer = ( void * ) new_alloc_node;
	}

	new_node = ( mpc_omp_node_t * )me->user_pointer;
	new_node->tree_array = root;
	// Get infos from parent node
	new_node->depth = __tree_array_get_node_depth( tree_shape, top_level, rank );
	const int num_children = tree_shape[new_node->depth];
	new_node->nb_children = num_children;
	new_node->global_rank = rank;
	new_node->stage_rank = __tree_array_global_to_stage( tree_shape, top_level, rank );
	new_node->local_rank = __tree_array_global_to_local( tree_shape, top_level, rank );
	new_node->rank = new_node->local_rank;
	new_node->reduce_data = ( void ** ) mpc_omp_alloc( new_node->nb_children * 64 * sizeof( void * ) );
	new_node->isArrived = ( int * ) mpc_omp_alloc( new_node->nb_children * 64 * sizeof( int ) );


	if ( rank )
	{
		me->fathers_array_size = new_node->depth;
		me->fathers_array = ( int * )mpc_omp_alloc( sizeof( int ) * new_node->depth );
		assert( me->fathers_array );
	}

	/* Children initialisation */
	me->children_array_size = top_level - new_node->depth;
	me->first_child_array = ( int * ) __tree_array_get_first_child_array( tree_shape, top_level, rank );

	/* -- TREE BASE -- */
	if ( rank )
	{
		new_node->tree_depth = top_level - new_node->depth + 1;
		assert( new_node->tree_depth > 1 );
		new_node->tree_base = ( int * )mpc_omp_alloc( sizeof( int ) * new_node->tree_depth );
		assert( new_node->tree_base );
		new_node->tree_cumulative = ( int * )mpc_omp_alloc( sizeof( int ) * new_node->tree_depth );
		assert( new_node->tree_cumulative );
		new_node->tree_nb_nodes_per_depth = ( int * )mpc_omp_alloc( sizeof( int ) * new_node->tree_depth );
		assert( new_node->tree_nb_nodes_per_depth );
		const int tmp = root_node->tree_nb_nodes_per_depth[new_node->depth];
		new_node->tree_base[0] = 1;
		new_node->tree_nb_nodes_per_depth[0] = 1;
		new_node->tree_cumulative[0] = root_node->tree_cumulative[ new_node->depth ];

		for ( i = 1, j = new_node->depth + 1; i < new_node->tree_depth; i++, j++ )
		{
			new_node->tree_base[i] = root_node->tree_base[j];
			new_node->tree_cumulative[i] = root_node->tree_cumulative[j];
			new_node->tree_nb_nodes_per_depth[i] = root_node->tree_nb_nodes_per_depth[j] / tmp;
		}
	}

#if 0
	json_t *json_node_tree_base = json_array();
	json_t *json_node_tree_cumul = json_array();
	json_t *json_node_tree_lsize = json_array();

	for ( i = 0; i < new_node->tree_depth; i++ )
	{
		json_array_push( json_node_tree_base, json_int( new_node->tree_base[i] ) );
		json_array_push( json_node_tree_cumul, json_int( new_node->tree_cumulative[i] ) );
		json_array_push( json_node_tree_lsize, json_int( new_node->tree_nb_nodes_per_depth[i] ) );
	}

	json_t *json_node_obj = json_object();
	json_object_set( json_node_obj, "depth", json_int( new_node->depth ) );
	json_object_set( json_node_obj, "global_rank", json_int( new_node->global_rank ) );
	json_object_set( json_node_obj, "tree_base", json_node_tree_base );
	json_object_set( json_node_obj, "tree_cumulative", json_node_tree_cumul );
	json_object_set( json_node_obj, "tree_nb_nodes_per_depth", json_node_tree_lsize );;
	char *json_char_node = json_dump( json_node_obj, JSON_COMPACT );
	json_decref( json_node_obj );
	fprintf( stderr, ":: %s :: %s\n", __func__, json_char_node );
	free( json_char_node );
#endif /* NDEBUG */

	/* Leaf or Node */
	if ( new_node->depth < top_level - 1 )
	{
		mpc_omp_node_t **children = NULL;
		new_node->child_type = MPC_OMP_CHILDREN_NODE;
		children = ( mpc_omp_node_t ** )mpc_omp_alloc( sizeof( mpc_omp_node_t * ) * num_children );
		assert( children );
		new_node->children.node = children;
	}
	else
	{
		mpc_omp_mvp_t **children = NULL;
		new_node->child_type = MPC_OMP_CHILDREN_LEAF;
		children = ( mpc_omp_mvp_t ** )mpc_omp_alloc( sizeof( mpc_omp_mvp_t * ) * num_children );
		assert( children );
		new_node->children.leaf = children;
	}

	/* Wait our children */
	while ( OPA_load_int( &( me->children_ready ) ) != num_children )
	{
		mpc_thread_yield();
	}

	//(void) __tree_array_compute_parents( root_node, new_node );
	const int first_idx = me->first_child_array[0];

	switch ( new_node->child_type )
	{
		case MPC_OMP_CHILDREN_NODE:
			__tree_array_update_children_node( first_idx, new_node, root );
			break;

		case MPC_OMP_CHILDREN_LEAF:
			__tree_array_update_children_mvp( first_idx, new_node, root );
			break;

		default:
			mpc_common_debug_abort();
	}

	if ( !( me->fathers_array ) )
	{
		return 0;
	}

	father_rank = me->fathers_array[new_node->depth - 1];
	( void ) OPA_fetch_and_incr_int( &( root[father_rank].children_ready ) );
	return ( !( new_node->local_rank ) ) ? 1 : 0;
}

static inline void *__tree_mvp_init( void *args )
{
	int *tree_shape;
	int target_node_rank, current_depth;
    mpc_omp_thread_t *ithread, *thread;
	mpc_omp_mvp_t *new_mvp = NULL;
	mpc_omp_node_t *root_node;
	mpc_omp_meta_tree_node_t *me = NULL;
	mpc_omp_meta_tree_node_t *root = NULL;
	mpc_omp_mvp_thread_args_t *unpack_args = NULL;

	/* Unpack thread start_routine arguments */
	unpack_args = ( mpc_omp_mvp_thread_args_t * ) args;
	const unsigned int rank = unpack_args->rank;
	root = unpack_args->array;
	me = &( root[rank] );
	root_node = ( mpc_omp_node_t * ) root[0].user_pointer;

	/* Shift root tree array to get OMP_TREE shape */
	const int top_level = root_node->tree_depth - 1;
	tree_shape = root_node->tree_base + 1;

	/* User defined infos */
    /* MVP of initial thread becomes master thread MVP for upcoming parallel region */
    if( mpc_omp_tls ) {
        ithread = mpc_omp_tls;
        new_mvp = ithread->mvp;
        ithread->root = root_node;
    }
    else {
        new_mvp = (mpc_omp_mvp_t *) mpc_omp_alloc( sizeof(mpc_omp_mvp_t) );
        assert( new_mvp );
        memset( new_mvp, 0, sizeof(mpc_omp_mvp_t) );

        new_mvp->pu_id = unpack_args->pu_id;

        new_mvp->threads = (mpc_omp_thread_t*) mpc_omp_alloc( sizeof( mpc_omp_thread_t) );
        assert( new_mvp->threads );
        memset( new_mvp->threads, 0, sizeof( mpc_omp_thread_t ) );

#if OMPT_SUPPORT
        new_mvp->threads->mvp = new_mvp;
        new_mvp->threads->tool_status = unpack_args->tool_status;
        new_mvp->threads->tool_instance = unpack_args->tool_instance;

        _mpc_omp_ompt_callback_thread_begin( new_mvp->threads, ompt_thread_other );
        _mpc_omp_ompt_callback_thread_begin( new_mvp->threads, ompt_thread_worker );
#endif /* OMPT_SUPPORT */
    }

	/* Initialize the corresponding microVP (all but tree-related variables) */
	new_mvp->root = root_node;
	new_mvp->thread_self = mpc_thread_self();

	/* MVP ranking */
	new_mvp->global_rank = rank;
	new_mvp->stage_rank = __tree_array_global_to_stage( tree_shape, top_level, rank );
	new_mvp->local_rank = __tree_array_global_to_local( tree_shape, top_level, rank );
	new_mvp->rank = new_mvp->local_rank;

	/* Father initialisation */
	me->fathers_array_size = ( top_level ) ? top_level : 1;
	me->fathers_array = ( int * )__tree_array_get_father_array_by_depth( tree_shape, top_level, rank );
	new_mvp->tree_rank = __tree_array_get_rank( tree_shape, top_level, rank );
	new_mvp->enable = 1;

    thread = new_mvp->threads;
	thread->next = NULL;
	thread->info.num_threads = 1;
	new_mvp->depth = top_level;

    _mpc_omp_thread_infos_init( thread );

	/* Generic infos */
	me->type = MPC_OMP_META_TREE_LEAF;
	me->user_pointer = ( void * ) new_mvp;

	// Transfert to slave_node_leaf function ...
	current_depth = top_level - 1;
	target_node_rank = me->fathers_array[current_depth];

	( void ) OPA_incr_int( &( root[target_node_rank].children_ready ) );

	if (  !( new_mvp->local_rank ) )
	{
		while ( __tree_node_init( root, tree_shape, top_level, target_node_rank ) )
		{
			current_depth--;
			target_node_rank = me->fathers_array[current_depth];
		}

		if ( !( new_mvp->stage_rank ) )
		{
			return ( void * ) root[0].user_pointer;
		}

		new_mvp->spin_node = ( mpc_omp_node_t * ) root[target_node_rank].user_pointer;
		assert( new_mvp->spin_node );
		new_mvp->spin_node->spin_status = MPC_OMP_MVP_STATE_SLEEP;
	}
	else /* The other children are regular leaves */
	{
		new_mvp->spin_node = NULL;
		new_mvp->spin_status = MPC_OMP_MVP_STATE_SLEEP;
	}

	mpc_omp_slave_mvp_node( new_mvp );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_thread_end( thread, ompt_thread_worker );
    _mpc_omp_ompt_callback_thread_end( thread, ompt_thread_other );

    /* Mvp exit */
    if( thread->tool_status == active )
        OPA_incr_int( &thread->tool_instance->nb_native_threads_exited );
#endif /* OMPT_SUPPORT */

	return NULL;
}

void
mpc_omp_init_seq_region() {
	mpc_omp_instance_t *seq_instance;
    mpc_omp_mvp_t* new_mvp;
	int *singleton;

    /* Create an OpenMP context */
    /* Init sequential instance */
	seq_instance = ( mpc_omp_instance_t * ) mpc_omp_alloc( sizeof( mpc_omp_instance_t ) );
	assert( seq_instance );
	memset( seq_instance, 0, sizeof( mpc_omp_instance_t ) );

    /* Init sequential team */
	seq_instance->team = ( mpc_omp_team_t * ) mpc_omp_alloc( sizeof( mpc_omp_team_t ) );
	assert( seq_instance->team );
	memset( seq_instance->team, 0, sizeof( mpc_omp_team_t ) );

	seq_instance->team->critical_lock = ( omp_lock_t * ) mpc_omp_alloc( sizeof( omp_lock_t ) );

	seq_instance->team->depth = 0;
	seq_instance->root = NULL;
	seq_instance->nb_mvps = 1;

    /* Init MVP of sequential region */
    new_mvp = (mpc_omp_mvp_t *) mpc_omp_alloc( sizeof(mpc_omp_mvp_t) );
    assert( new_mvp );
    memset( new_mvp, 0, sizeof(mpc_omp_mvp_t) );

    /* Set PU on wich microvp is running */
    new_mvp->pu_id = mpc_topology_get_pu();

    /* Create associated initial thread */
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    new_mvp->threads = ( mpc_omp_thread_t* ) mpc_omp_tls;
#else
    new_mvp->threads = (mpc_omp_thread_t*) mpc_omp_alloc( sizeof( mpc_omp_thread_t ));
    assert( new_mvp->threads );
    memset( new_mvp->threads, 0, sizeof( mpc_omp_thread_t ));

    mpc_omp_tls = ( void* ) new_mvp->threads;
#endif /* OMPT_SUPPORT */

    new_mvp->threads->mvp = new_mvp;
    new_mvp->threads->instance = seq_instance;

    /* Init tree infos of sequential instance */
	seq_instance->tree_array = (mpc_omp_generic_node_t *) mpc_omp_alloc( sizeof( mpc_omp_generic_node_t ));
	assert( seq_instance->tree_array );
    memset( seq_instance->tree_array, 0, sizeof( mpc_omp_generic_node_t ) );

	seq_instance->tree_array[0].ptr.mvp = new_mvp;
	seq_instance->tree_array[0].type = MPC_OMP_CHILDREN_LEAF;
	seq_instance->mvps = seq_instance->tree_array;
	seq_instance->tree_depth = 1;

	singleton = ( int * ) mpc_omp_alloc( sizeof( int ) );
	assert( singleton );

	singleton[0] = 1;
	seq_instance->tree_base = singleton;
	seq_instance->tree_cumulative = singleton;
	seq_instance->tree_nb_nodes_per_depth = singleton;

  /* Initialize the various allocators (OpenMP 5.0 Mem Management) */
  //master->default_allocator = icvs->def_allocator_var;

}

void
mpc_omp_init_initial_thread( const mpc_omp_local_icv_t icvs )
{
    mpc_omp_thread_t* ith;
    ith = ( mpc_omp_thread_t* ) mpc_omp_tls;
    assert( ith );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_thread_begin( ith, ompt_thread_initial );
#endif /* OMPT_SUPPORT */

    _mpc_omp_thread_infos_init( ith );

    _mpc_omp_task_tree_init(ith);
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    /* Update initial task with enter frame info */
    _mpc_omp_ompt_frame_set_enter( ith->frame_infos.ompt_frame_infos.enter_frame.ptr );
#endif /* OMPT_SUPPORT */

	/* Protect Orphan OpenMP construct */
	ith->info.icvs = icvs;
    ith->info.num_threads = 1;
}

void _mpc_omp_tree_alloc( int *shape, int top_level, const int *cpus_order, const int place_depth, const int place_size )
{
	int i, n_num, place_id;
	mpc_thread_t *threads;
	mpc_omp_mvp_thread_args_t *args;
	mpc_omp_meta_tree_node_t *tree_array;

#if OMPT_SUPPORT
    mpc_omp_thread_t* t = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( t );
#endif /* OMPT_SUPPORT */

	mpc_omp_node_t *root = ( mpc_omp_node_t * ) mpc_omp_alloc( sizeof( mpc_omp_node_t ) );
	assert( root );
	memset( root, 0, sizeof( mpc_omp_node_t ) );
#if 0

	for ( i = 0; i < top_level; i++ )
	{
		fprintf( stderr, ":: %s :: shape %d -> %d\n", __func__, i, shape[i] );
	}

#endif
	root->tree_depth = top_level + 1;
	root->tree_base = __tree_array_compute_shape( root, shape, top_level );
	root->tree_cumulative = __tree_array_compute_cumulative_num_nodes_for_depth( root );
	root->tree_nb_nodes_per_depth = __tree_array_compute_num_nodes_for_depth( root, &n_num );
#ifdef MPC_Lowcomm
  root->worker_ws = mpc_omp_alloc(sizeof(worker_workshare_t));
  root->worker_ws->mpi_rank = mpc_common_get_local_task_rank();
#ifdef MPC_ENABLE_WORKSHARE
  root->worker_ws->workshare = mpc_thread_data_get()->workshare;
#endif
#endif
	const int leaf_n_num = root->tree_nb_nodes_per_depth[top_level];
	const int non_leaf_n_num = n_num - leaf_n_num;

	tree_array = ( mpc_omp_meta_tree_node_t * ) mpc_omp_alloc( n_num * sizeof( mpc_omp_meta_tree_node_t ) );
	assert( tree_array );
	memset( tree_array, 0,  n_num * sizeof( mpc_omp_meta_tree_node_t ) );

	root->tree_array = tree_array;
	tree_array[0].user_pointer = root;

	threads = ( mpc_thread_t * ) mpc_omp_alloc( ( leaf_n_num - 1 ) * sizeof( mpc_thread_t ) );
	assert( threads );
	memset( threads, 0, ( leaf_n_num - 1 ) * sizeof( mpc_thread_t ) );

	args = ( mpc_omp_mvp_thread_args_t * ) mpc_omp_alloc( sizeof( mpc_omp_mvp_thread_args_t ) * leaf_n_num );
	assert( args );

	for ( i = 0, place_id = 0; i < leaf_n_num; i++ )
	{
		place_id += ( !( i % place_size ) && i ) ? 1 : 0;
		args[i].array = tree_array;
		args[i].place_id = place_id;
		args[i].place_depth = place_depth;
		args[i].rank = i + non_leaf_n_num;
#if OMPT_SUPPORT
		args[i].tool_status = t->tool_status;
		args[i].tool_instance = t->tool_instance;
#endif /* OMPT_SUPPORT */
	}

	/* Worker threads */
	for ( i = 1; i < leaf_n_num; i++ )
	{
		const int target_vp = ( cpus_order ) ? cpus_order[i] : i;
		const int pu_id = ( mpc_topology_get_pu() + target_vp ) % mpc_topology_get_pu_count();
		args[i].target_vp = target_vp;
		args[i].pu_id = pu_id;
		mpc_thread_attr_t __attr;
		mpc_thread_attr_init( &__attr );
		mpc_thread_attr_setbinding( &__attr, pu_id );
		mpc_thread_attr_setstacksize( &__attr, mpcomp_global_icvs.stacksize_var );
		int  __UNUSED__  ret = mpc_thread_core_thread_create( &( threads[i - 1] ), &__attr, __tree_mvp_init, &( args[i] ) );
		assert( !ret );
		mpc_thread_attr_destroy( &__attr );
	}

	/* Root initialisation */
	__tree_mvp_init( &( args[0] ) );
  mpc_omp_alloc_init_allocators();

	/* Free is thread-safety due to half barrier perform by tree build */
	mpc_omp_free( args );
}
