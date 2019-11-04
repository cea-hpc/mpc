#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>



#include "mpcthread.h"
#include "mpc_common_asm.h"
#include "mpcomp_types.h"
#include "mpcomp_tree_array.h"
#include "mpcomp_tree_structs.h"
#include "mpcomp_spinning_core.h"

#include "mpcomp_tree_array_utils.h"

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

static inline int __tree_array_get_children_nodes_num_per_depth( const int *shape, const int max_depth, const int depth )
{
	int i, count;

	for ( count = 1, i = depth; i < max_depth; i++ )
	{
		count *= shape[i];
	}

	return count;
}

static inline int __tree_array_get_stage_nodes_num_per_depth( const int *shape, __UNUSED__ const int max_depth, const int depth )
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
__mpcomp_tree_array_get_stage_nodes_num_depth_array( const int *shape, const int max_depth, const int depth )
{
	int i;
	int *count;
	count = ( int * ) mpcomp_alloc( sizeof( int ) * depth );
	assert( count );
	memset( count, 0, sizeof( int ) * depth );

	for ( count[0] = 1, i = 1; i < depth; i++ )
	{
		count[i] = count[i - 1] * shape[i - 1];
	}

	return count;
}
#endif

static inline int __tree_array_get_node_depth( const int *shape, const int max_depth, const int rank )
{
	int count, sum, i;

	for ( count = 1, sum = 1, i = 0; i < max_depth; i++ )
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


static inline int __tree_array_global_to_stage( const int *shape, const int max_depth, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( max_depth >= 0 );
	const int depth = __tree_array_get_node_depth( shape, max_depth, rank );

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

static inline int __tree_array_global_to_local( const int *shape, const int max_depth, const int rank )
{
	assert( shape );
	assert( rank >= 0 );
	assert( max_depth >= 0 );

	if ( !rank )
	{
		return 0;
	}

	const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
	const int stage_rank = __tree_array_global_to_stage( shape, max_depth, rank );
	return __tree_array_stage_to_local( shape, depth, stage_rank );
}

static inline int * __tree_array_get_rank( const int *shape, const int max_depth, const int rank )
{
	int i, current_rank;
	int *tree_array;
	assert( shape );
	assert( rank >= 0 );
	assert( max_depth >= 0 );
	tree_array = ( int * ) mpcomp_alloc( sizeof( int ) * max_depth );
	assert( tree_array );
	memset( tree_array, 0, sizeof( int ) * max_depth );

	for ( current_rank = rank, i = max_depth - 1; i >= 0; i-- )
	{
		const int stage_rank = __tree_array_global_to_stage( shape, max_depth, current_rank );
		tree_array[i] = stage_rank % shape[ i ];
		current_rank = __tree_array_stage_to_global( shape, max_depth, stage_rank / shape[i] );
	}

	return tree_array;
}

static inline int *__tree_array_get_tree_cumulative( const int *shape, int max_depth )
{
	int i, j;
	int *tree_cumulative;
	tree_cumulative = ( int * ) mpcomp_alloc( sizeof( int ) * max_depth );
	assert( tree_cumulative );
	memset( tree_cumulative, 0, sizeof( int ) * max_depth );

	for ( i = 0; i < max_depth - 1; i++ )
	{
		tree_cumulative[i] = 1;

		for ( j = 0; j < max_depth; j++ )
		{
			tree_cumulative[i] *= shape[j];
		}
	}

	tree_cumulative[max_depth - 1] = 1;
	return tree_cumulative;
}

static inline int * __tree_array_get_father_array_by_depth( const int *shape, const int max_depth, const int rank )
{
	int i;
	int tmp_rank = -1;
	int *parents_array = NULL;
	int depth = __tree_array_get_node_depth( shape, max_depth, rank );

	if ( !rank )
	{
		return NULL;
	}

	if ( max_depth == 0 )
	{
		depth = 1;
	}

	tmp_rank = rank;
	parents_array = ( int * ) mpcomp_alloc( sizeof( int ) * depth );
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
__mpcomp_tree_array_get_father_array_by_level( const int *shape, const int max_depth, const int rank )
{
	int i;
	int tmp_rank = -1;
	int *parents_array = NULL;
	int nodes_num = __tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
	const int depth = __tree_array_get_node_depth( shape, max_depth, rank );

	if ( !rank )
	{
		return NULL;
	}

	tmp_rank = rank;
	parents_array = ( int * ) mpcomp_alloc( sizeof( int ) * depth );
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


static inline int *
__tree_array_get_first_child_array( const int *shape, const int max_depth, const int rank )
{
	int i, last_father_rank, last_stage_size;
	int *first_child_array = NULL;
	const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
	const int children_levels = max_depth - depth;

	if (  !children_levels )
	{
		return NULL;
	}

	first_child_array = ( int * ) mpcomp_alloc( sizeof( int ) * children_levels );
	assert( first_child_array );
	memset( first_child_array, 0,  sizeof( int ) * children_levels );
	last_father_rank = __tree_array_global_to_stage( shape, max_depth, rank );

	for ( i = 0; i < children_levels; i++ )
	{
		last_stage_size = __tree_array_get_parent_nodes_num_per_depth( shape, depth + i );
		last_father_rank = last_stage_size + shape[depth + i] * last_father_rank;
		first_child_array[i] = last_father_rank;
		last_father_rank = __tree_array_global_to_stage( shape, max_depth, last_father_rank );
	}

	return first_child_array;
}

#if 0
static inline int *
__mpcomp_tree_array_get_children_num_array( const int *shape, const int max_depth, const int rank )
{
	int i;
	int *children_num_array = NULL;
	const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
	const int children_levels = max_depth - depth;

	if (  !children_levels )
	{
		return NULL;
	}

	children_num_array = ( int * ) mpcomp_alloc( sizeof( int ) * children_levels );
	assert( children_num_array );
	memset( children_num_array, 0,  sizeof( int ) * children_levels );

	for ( i = 0; i < children_levels; i++ )
	{
		children_num_array[i] = shape[depth + i];
	}

	return children_num_array;
}
#endif

static inline int __tree_array_compute_thread_min_compact( const int *shape, const int max_depth, const int rank )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_cumulative, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, max_depth, rank );
		tmp_cumulative = __tree_array_get_children_nodes_num_per_depth( shape, max_depth, depth );
		count = tmp_local_rank;
		count *= ( depth < max_depth ) ? tmp_cumulative : 1;

		for ( i = 0; i < depth; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, max_depth, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, max_depth, node_parents[i] );
			tmp_cumulative = __tree_array_get_children_nodes_num_per_depth( shape, max_depth, parent_depth );
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

static inline int __tree_array_compute_thread_min_balanced( const int *shape, const int max_depth, const int rank, const int core_depth )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_cumulative, tmp_nodes_per_depth, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
		const int *tmp_cumulative_core_array = __tree_array_get_tree_cumulative( shape, max_depth );
		const int tmp_cumulative_core = tmp_cumulative_core_array[depth - 1];
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, max_depth, rank );

		if ( depth - 1 < core_depth )
		{
			tmp_cumulative = tmp_cumulative_core_array[depth - 1];
			count = tmp_local_rank * tmp_cumulative / tmp_cumulative_core;
		}
		else
		{
			tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, max_depth, depth - 1 );
			count = tmp_local_rank * tmp_nodes_per_depth;
		}

		for ( i = 0; i < depth - 1; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, max_depth, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, max_depth, node_parents[i] );

			if ( parent_depth - 1 < core_depth )
			{
				tmp_cumulative = tmp_cumulative_core_array[depth - ( i + 1 )];
				count += ( tmp_local_rank * tmp_cumulative / tmp_cumulative_core );
			}
			else
			{
				tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, max_depth, parent_depth - 1 );
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
__mpcomp_tree_array_update_thread_openmp_min_rank_scatter ( const int father_stage_size, const int father_min_index, const int node_local_rank )
{
	return father_min_index + node_local_rank * father_stage_size;
}
#endif

static inline int __tree_array_compute_thread_min_scatter( const int *shape, const int max_depth, const int rank )
{
	int i, count;
	int *node_parents = NULL;
	int tmp_nodes_per_depth, tmp_local_rank;

	if ( rank )
	{
		const int depth = __tree_array_get_node_depth( shape, max_depth, rank );
		node_parents = __tree_array_get_father_array_by_depth( shape, depth, rank );
		tmp_local_rank = __tree_array_global_to_local( shape, max_depth, rank );
		tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, max_depth, depth );
		count = tmp_local_rank * tmp_nodes_per_depth;

		for ( i = 0; i < depth; i++ )
		{
			const int parent_depth = __tree_array_get_node_depth( shape, max_depth, node_parents[i] );
			tmp_local_rank = __tree_array_global_to_local( shape, max_depth, node_parents[i] );
			tmp_nodes_per_depth = __tree_array_get_stage_nodes_num_per_depth( shape, max_depth, parent_depth );
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

int * mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int max_depth, const int rank, const int core_depth )
{
	int index;
	int *min_index = NULL;
	min_index = ( int * ) mpcomp_alloc( sizeof( int ) * MPCOMP_AFFINITY_NB );
	assert( min_index );
	memset( min_index, 0, sizeof( int ) * MPCOMP_AFFINITY_NB );
	/* MPCOMP_AFFINITY_COMPACT  */
	index = __tree_array_compute_thread_min_compact( shape, max_depth, rank );
	min_index[MPCOMP_AFFINITY_COMPACT] = index;
	/* MPCOMP_AFFINITY_SCATTER  */
	index = __tree_array_compute_thread_min_scatter( shape, max_depth, rank );
	min_index[MPCOMP_AFFINITY_SCATTER] = index;
	/* MPCOMP_AFFINITY_BALANCED */
	index = __tree_array_compute_thread_min_balanced( shape, max_depth, rank, core_depth );
	min_index[MPCOMP_AFFINITY_BALANCED] = index;
	return min_index;
}

#if 0
static inline int __mpcomp_tree_array_find_next_running_stage_depth(   const int *tree_shape,
        const int start_depth,
        const int max_depth,
        const int num_requested_mvps,
        int *num_assigned_mvps )
{
	int i;
	int cumulative_children_num = 1;

	for ( i = start_depth; i < max_depth; i++ )
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

static inline int __tree_node_init( mpcomp_meta_tree_node_t *root, const int *tree_shape, const int max_depth, const int rank )
{
	int i, j, father_rank;
	mpcomp_meta_tree_node_t *me;
	mpcomp_node_t *new_node, *root_node;
	sctk_assert( root );
	sctk_assert( tree_shape );
	me = &( root[rank] );
	root_node = ( mpcomp_node_t * )root[0].user_pointer;
	sctk_assert( root_node );

	if ( !( me->user_pointer )  )
	{
		sctk_assert( rank );
		me->type = MPCOMP_META_TREE_NODE;
		mpcomp_node_t *new_alloc_node = ( mpcomp_node_t * ) mpcomp_alloc( sizeof( mpcomp_node_t ) );
		assert( new_alloc_node );
		memset( new_alloc_node, 0, sizeof( mpcomp_node_t ) );
		me->user_pointer = ( void * ) new_alloc_node;
	}

	new_node = ( mpcomp_node_t * )me->user_pointer;
	new_node->tree_array = root;

	// Get infos from parent node
	new_node->depth = __tree_array_get_node_depth( tree_shape, max_depth, rank );
	const int num_children = tree_shape[new_node->depth];
	new_node->nb_children = num_children;
	new_node->global_rank = rank;
	new_node->stage_rank = __tree_array_global_to_stage( tree_shape, max_depth, rank );
	new_node->local_rank = __tree_array_global_to_local( tree_shape, max_depth, rank );
	new_node->rank = new_node->local_rank;

	if ( rank )
	{
		me->fathers_array_size = new_node->depth;
		me->fathers_array = ( int * )mpcomp_alloc( sizeof( int ) * new_node->depth );
		sctk_assert( me->fathers_array );
	}

	/* Children initialisation */
	me->children_array_size = max_depth - new_node->depth;
	me->first_child_array = ( int * ) __tree_array_get_first_child_array( tree_shape, max_depth, rank );

	/* -- TREE BASE -- */
	if ( rank )
	{
		new_node->tree_depth = max_depth - new_node->depth + 1;
		sctk_assert( new_node->tree_depth > 1 );
		new_node->tree_base = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
		sctk_assert( new_node->tree_base );
		new_node->tree_cumulative = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
		sctk_assert( new_node->tree_cumulative );
		new_node->tree_nb_nodes_per_depth = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
		sctk_assert( new_node->tree_nb_nodes_per_depth );
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
	if ( new_node->depth < max_depth - 1 )
	{
		mpcomp_node_t **children = NULL;
		new_node->child_type = MPCOMP_CHILDREN_NODE;
		children = ( mpcomp_node_t ** )mpcomp_alloc( sizeof( mpcomp_node_t * ) * num_children );
		sctk_assert( children );
		new_node->children.node = children;
	}
	else
	{
		mpcomp_mvp_t **children = NULL;
		new_node->child_type = MPCOMP_CHILDREN_LEAF;
		children = ( mpcomp_mvp_t ** )mpcomp_alloc( sizeof( mpcomp_mvp_t * ) * num_children );
		assert( children );
		new_node->children.leaf = children;
	}

	/* Wait our children */
	while ( OPA_load_int( &( me->children_ready ) ) != num_children )
	{
		sctk_thread_yield();
	}

	//(void) __mpcomp_tree_array_compute_node_parents( root_node, new_node );
	const int first_idx = me->first_child_array[0];

	switch ( new_node->child_type )
	{
		case MPCOMP_CHILDREN_NODE:
			__mpcomp_update_node_children_node_ptr( first_idx, new_node, root );
			break;

		case MPCOMP_CHILDREN_LEAF:
			__mpcomp_update_node_children_mvp_ptr( first_idx, new_node, root );
			break;

		default:
			sctk_abort();
	}

	if ( !( me->fathers_array ) )
	{
		return 0;
	}

	father_rank = me->fathers_array[new_node->depth - 1];
	( void ) OPA_fetch_and_incr_int( &( root[father_rank].children_ready ) );
	return ( !( new_node->local_rank ) ) ? 1 : 0;
}

static inline void * __tree_mvp_init( void *args )
{
	int *tree_shape;
	int target_node_rank, current_depth;
	mpcomp_mvp_t *new_mvp = NULL;
	mpcomp_node_t *root_node;
	mpcomp_meta_tree_node_t *me = NULL;
	mpcomp_meta_tree_node_t *root = NULL;
	mpcomp_mvp_thread_args_t *unpack_args = NULL;
	/* Unpack thread start_routine arguments */
	unpack_args = ( mpcomp_mvp_thread_args_t * ) args;
	const unsigned int rank = unpack_args->rank;
	root = unpack_args->array;
	me = &( root[rank] );
	root_node = ( mpcomp_node_t * ) root[0].user_pointer;

        /* Shift root tree array to get OMP_TREE shape */
	const int max_depth = root_node->tree_depth - 1;
	tree_shape = root_node->tree_base + 1;

	/* User defined infos */
	new_mvp = ( mpcomp_mvp_t * ) mpcomp_alloc( sizeof( mpcomp_mvp_t ) );
	assert( new_mvp );
	memset( new_mvp, 0, sizeof( mpcomp_mvp_t ) );

	/* Initialize the corresponding microVP (all but tree-related variables) */
	new_mvp->root = root_node;
	new_mvp->thread_self = sctk_thread_self();

	/* MVP ranking */
	new_mvp->global_rank = rank;
	new_mvp->stage_rank = __tree_array_global_to_stage( tree_shape, max_depth, rank );
	new_mvp->local_rank = __tree_array_global_to_local( tree_shape, max_depth, rank );
	new_mvp->rank = new_mvp->local_rank;

	/* Father initialisation */
	me->fathers_array_size = ( max_depth ) ? max_depth : 1;
	me->fathers_array = ( int * )__tree_array_get_father_array_by_depth( tree_shape, max_depth, rank );
	new_mvp->tree_rank = __tree_array_get_rank( tree_shape, max_depth, rank );
	new_mvp->enable = 1;
	new_mvp->threads = ( mpcomp_thread_t * ) mpcomp_alloc( sizeof( mpcomp_thread_t ) );
	sctk_assert( new_mvp->threads );
	memset( new_mvp->threads, 0, sizeof( mpcomp_thread_t ) );

#if defined( MPCOMP_OPENMP_3_0 )
	mpcomp_tree_array_task_thread_init( new_mvp->threads );
#endif /* MPCOMP_OPENMP_3_0 */

	new_mvp->threads->next = NULL;
	new_mvp->threads->info.num_threads = 1;
	new_mvp->depth = max_depth;

	/* Generic infos */
	me->type = MPCOMP_META_TREE_LEAF;
	me->user_pointer = ( void * ) new_mvp;

	// Transfert to slave_node_leaf function ...
	current_depth = max_depth - 1;
	target_node_rank = me->fathers_array[current_depth];
	( void ) OPA_incr_int( &( root[target_node_rank].children_ready ) );

	if (  !( new_mvp->local_rank ) )
	{
		while ( __tree_node_init( root, tree_shape, max_depth, target_node_rank ) )
		{
			current_depth--;
			target_node_rank = me->fathers_array[current_depth];
		}

		if ( !( new_mvp->stage_rank ) )
		{
			return ( void * ) root[0].user_pointer;
		}

		new_mvp->spin_node = ( mpcomp_node_t * ) root[target_node_rank].user_pointer;
		sctk_assert( new_mvp->spin_node );
		new_mvp->spin_node->spin_status = MPCOMP_MVP_STATE_SLEEP;
	}
	else /* The other children are regular leaves */
	{
		new_mvp->spin_node = NULL;
		new_mvp->spin_status = MPCOMP_MVP_STATE_SLEEP;
	}

	mpcomp_slave_mvp_node( new_mvp );
	return NULL;
}

static inline void __tree_master_thread_init( mpcomp_node_t *root, mpcomp_meta_tree_node_t *tree_array, const mpcomp_local_icv_t *icvs )
{
	int *singleton;
	mpcomp_thread_t *master;
	mpcomp_instance_t *seq_instance;
	sctk_assert( root );
	sctk_assert( tree_array );

	// Create an OpenMP context
	master = root->mvp->threads;
	sctk_assert( master );

	master->root = root;
	master->mvp = root->mvp;
	seq_instance = ( mpcomp_instance_t * ) mpcomp_alloc( sizeof( mpcomp_instance_t ) );

	sctk_assert( seq_instance );
	memset( seq_instance, 0, sizeof( mpcomp_instance_t ) );

	seq_instance->team = ( mpcomp_team_t * ) mpcomp_alloc( sizeof( mpcomp_team_t ) );
	sctk_assert( seq_instance->team );
	memset( seq_instance->team, 0, sizeof( mpcomp_team_t ) );

	seq_instance->team->depth = 0;
	seq_instance->root = NULL;
	seq_instance->nb_mvps = 1;

	seq_instance->tree_array = ( mpcomp_generic_node_t * )mpcomp_alloc( sizeof( mpcomp_generic_node_t ) );
	sctk_assert( seq_instance->tree_array );
	seq_instance->tree_array[0].ptr.mvp = root->mvp;
	seq_instance->tree_array[0].type = MPCOMP_CHILDREN_LEAF;

	seq_instance->mvps = seq_instance->tree_array;

	seq_instance->tree_depth = 1;

	singleton = ( int * ) mpcomp_alloc( sizeof( int ) );
	sctk_assert( singleton );
	singleton[0] = 1;

	seq_instance->tree_base = singleton;
	seq_instance->tree_cumulative = singleton;
	seq_instance->tree_nb_nodes_per_depth = singleton;

	master->instance = seq_instance;

	// Protect Orphan OpenMP construct
	master->info.icvs = *icvs;
	master->info.num_threads = 1;

	assert( root->mvp );

#if defined( MPCOMP_OPENMP_3_0 )
	mpcomp_tree_array_task_thread_init( master );
#endif /* MPCOMP_OPENMP_3_0 */
}



void _mpc_omp_tree_alloc( int *shape, int max_depth, const int *cpus_order, const int place_depth, const int place_size, const mpcomp_local_icv_t *icvs )
{
	int i, n_num, ret, place_id;

	sctk_thread_t *threads;
	mpcomp_mvp_thread_args_t *args;
	mpcomp_meta_tree_node_t *tree_array;

        mpcomp_node_t *root = ( mpcomp_node_t * ) mpcomp_alloc( sizeof( mpcomp_node_t ) );
	sctk_assert( root );
	memset( root, 0, sizeof( mpcomp_node_t ) );

#if 0

	for ( i = 0; i < max_depth; i++ )
	{
		fprintf( stderr, ":: %s :: shape %d -> %d\n", __func__, i, shape[i] );
	}

#endif
	root->tree_depth = max_depth + 1;
	root->tree_base = __mpcomp_tree_array_compute_tree_shape( root, shape, max_depth );
	root->tree_cumulative = __mpcomp_tree_array_compute_cumulative_num_nodes_per_depth( root );
	root->tree_nb_nodes_per_depth = __mpcomp_tree_array_compute_tree_num_nodes_per_depth( root, &n_num );

	const int leaf_n_num = root->tree_nb_nodes_per_depth[max_depth];
	const int non_leaf_n_num = n_num - leaf_n_num;

	tree_array = ( mpcomp_meta_tree_node_t * ) mpcomp_alloc( n_num * sizeof( mpcomp_meta_tree_node_t ) );
	assert( tree_array );
	memset( tree_array, 0,  n_num * sizeof( mpcomp_meta_tree_node_t ) );
	root->tree_array = tree_array;
	tree_array[0].user_pointer = root;

	threads = ( sctk_thread_t * ) mpcomp_alloc( ( leaf_n_num - 1 ) * sizeof( sctk_thread_t ) );
	sctk_assert( threads );
	memset( threads, 0, ( leaf_n_num - 1 ) * sizeof( sctk_thread_t ) );
	args = ( mpcomp_mvp_thread_args_t * ) mpcomp_alloc( sizeof( mpcomp_mvp_thread_args_t ) * leaf_n_num );
	sctk_assert( args );

	for ( i = 0, place_id = 0; i < leaf_n_num; i++ )
	{
		place_id += ( !( i % place_size ) && !i ) ? 1 : 0;
		args[i].array = tree_array;
		args[i].place_id = place_id;
		args[i].place_depth = place_depth;
		args[i].rank = i + non_leaf_n_num;
	}

	/* Worker threads */
	for ( i = 1; i < leaf_n_num; i++ )
	{
		const int target_vp = ( cpus_order ) ? cpus_order[i] : i;
        const int pu_id = ( mpc_topology_get_pu() + target_vp ) % mpc_topology_get_pu_count(); 

		place_id += ( !( i % place_size ) ) ? 1 : 0;
		args[i].target_vp = target_vp;

		sctk_thread_attr_t __attr;

		sctk_thread_attr_init( &__attr );
		sctk_thread_attr_setbinding( &__attr, pu_id );
		sctk_thread_attr_setstacksize( &__attr, mpcomp_global_icvs.stacksize_var );

		ret = sctk_user_thread_create( &( threads[i - 1] ), &__attr, __tree_mvp_init, &( args[i] ) );
		sctk_assert( !ret );

		sctk_thread_attr_destroy( &__attr );
	}

	/* Root initialisation */
	__tree_mvp_init( &( args[0] ) );
	__tree_master_thread_init( root, tree_array, icvs );

	sctk_openmp_thread_tls = ( void * ) root->mvp->threads;

#if OMPT_SUPPORT
    mpcomp_thread_t *t = (mpcomp_thread_t*) sctk_thread_tls;
    sctk_assert(t != NULL);

    if( mpcomp_ompt_is_enabled() )
    {
      if( OMPT_Callbacks )
      {
        ompt_callback_thread_begin_t callback;
        callback = (ompt_callback_thread_begin_t) OMPT_Callbacks[ompt_callback_thread_begin];
        if( callback )
        {
          t->ompt_thread_data = ompt_data_none;
          callback( ompt_thread_initial, &( t->ompt_thread_data ) );
        }
      }
    }
#endif /* OMPT_SUPPORT */

	/* Free is thread-safety due to half barrier perform by tree build */
	mpcomp_free( args );
}
