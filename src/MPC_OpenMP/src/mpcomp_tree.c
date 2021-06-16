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
#if( MPCOMP_TASK || defined( MPCOMP_OPENMP_3_0 ) )

/**************
 * TASK UTILS *
 **************/

static inline int *__tree_array_ancestor_path( mpcomp_instance_t *instance, const int globalRank, int *depth )
{
	int *path;
	mpcomp_generic_node_t *gen_node;
	assert( instance );
	assert( depth );
	assert( globalRank >= 0 && globalRank < instance->tree_array_size );
	assert( instance->tree_array );
	gen_node = &( instance->tree_array[globalRank] );

	/* Extra node to preserve regular tree_shape */
	if ( gen_node->type == MPCOMP_CHILDREN_NULL )
	{
		return NULL;
	}

	if ( gen_node->type == MPCOMP_CHILDREN_LEAF )
	{
		path = gen_node->ptr.mvp->threads->tree_array_ancestor_path;
		assert( gen_node->ptr.mvp->threads );
		*depth = gen_node->ptr.mvp->threads->father_node->depth + 1;
	}
	else
	{
		assert( gen_node->type == MPCOMP_CHILDREN_NODE );
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
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;
	/* Retrieve the current thread information */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
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
	mpcomp_thread_t *thread;
	mpcomp_instance_t *instance;

	/* If it's the root, ancestor is itself */
	if ( !globalRank )
	{
		return 0;
	}

	/* Retrieve the current thread information */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
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
static void ___tree_task_check_neigborhood( mpcomp_node_t *node )
{
	int i;
	mpcomp_thread_t  __UNUSED__ *thread = NULL;
	assert( node );
	/* Retrieve the current thread information */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	assert( thread->instance->tree_nb_nodes_per_depth );

	switch ( node->child_type )
	{
		case MPCOMP_CHILDREN_NODE:

			/* Call recursively for all children nodes */
			for ( i = 0; i < node->nb_children; i++ )
			{
				___tree_task_check_neigborhood( node->children.node[i] );
			}

			break;

		case MPCOMP_CHILDREN_LEAF:

			/* All the children are leafs */
			for ( i = 0; i < node->nb_children; i++ )
			{
				mpcomp_mvp_t *  __UNUSED__ mvp = node->children.leaf[i];
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
	mpcomp_thread_t *thread;
	/* Retrieve the current thread information */
	assert( sctk_openmp_thread_tls );
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( thread->instance );
	___tree_task_check_neigborhood( thread->instance->root );
}

#endif /* MPCOMP_TASK */

/**************
 * TREE ARRAY *
 **************/


#if 0
static char *
__mpcomp_tree_array_convert_array_to_string( int *tab, const int size )
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

static inline int *__tree_array_compute_shape( mpcomp_node_t *node, int *tree_shape, const int size  )
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

static inline int *__tree_array_compute_num_nodes_for_depth( mpcomp_node_t *node, int *total )
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

static inline int *__tree_array_compute_cumulative_num_nodes_for_depth( mpcomp_node_t *node )
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

static inline int *__tree_array_compute_first_children( mpcomp_node_t *root, mpcomp_node_t *node )
{
	int i, shift_next_stage_stage, stage_rank;
	int *node_first_children;
	assert( node );
	assert( root );
	const int root_tree_size = root->tree_depth;
	assert( root_tree_size );
	assert( node->depth < root_tree_size );
	const int node_tree_size = root_tree_size - node->depth;
	node_first_children = ( int * ) malloc( sizeof( int ) * node_tree_size );
	assert( node_first_children );
	memset( node_first_children, 0, sizeof( int ) * node_tree_size );
	node_first_children[0] = node->global_rank;
	shift_next_stage_stage = 0;

	for ( i = 0; i <= node->depth; i++ )
	{
		shift_next_stage_stage += root->tree_nb_nodes_per_depth[i];
	}

	stage_rank = node->stage_rank;

	for ( i = 1; i < node_tree_size; i++ )
	{
		stage_rank = stage_rank * root->tree_base[node->depth + i];
		node_first_children[i] = shift_next_stage_stage + stage_rank;
		shift_next_stage_stage += root->tree_nb_nodes_per_depth[node->depth + i];
	}

	return node_first_children;
}

static inline int *__tree_array_compute_parents(  mpcomp_node_t *root, mpcomp_node_t *node )
{
	int i, prev_num_nodes, stage_rank;
	int *node_parents;
	assert( node );
	assert( root );
	const int node_tree_size = node->depth + 1;
	node_parents = ( int * ) malloc( sizeof( int ) * node_tree_size );
	assert( node_parents );
	memset( node_parents, 0, sizeof( int ) * node_tree_size );
	node_parents[0] = node->global_rank;
	prev_num_nodes = 1;

	for ( i = 1; i < node->depth; i++ )
	{
		prev_num_nodes += root->tree_nb_nodes_per_depth[i];
	}

	for ( i = 1; i < node_tree_size; i++ )
	{
		if ( root->tree_nb_nodes_per_depth[node->depth - i] == 1 )
		{
			node_parents[i] = 0;
			continue;
		}

		stage_rank = node_parents[i - 1] - prev_num_nodes;
		prev_num_nodes -= root->tree_nb_nodes_per_depth[node->depth - i];
		node_parents[i] = stage_rank / ( root->tree_nb_nodes_per_depth[node->depth - i + 1] / root->tree_base[i] ) + prev_num_nodes;
	}

	return NULL;
}

static inline void __tree_array_update_children_node( const int first_idx,
        mpcomp_node_t *node,
        mpcomp_meta_tree_node_t *root )
{
	int i;
	mpcomp_meta_tree_node_t *me;
	assert( node );
	assert( first_idx > 0 );
	assert( root );
	assert( node->child_type == MPCOMP_CHILDREN_NODE );
	const int children_num = node->nb_children;
	me = &( root[node->global_rank] );
	assert( OPA_load_int( &( me->children_ready ) ) == children_num );

	for ( i = 0; i < children_num; i++ )
	{
		const int global_child_idx = first_idx + i;
		mpcomp_meta_tree_node_t *child = &( root[global_child_idx] );
		node->children.node[i] = ( mpcomp_node_t * ) child->user_pointer;
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
        mpcomp_node_t *node,
        mpcomp_meta_tree_node_t *root )
{
	int i;
	mpcomp_meta_tree_node_t *me;
	assert( node );
	assert( first_idx > 0 );
	assert( root );
	assert( node->child_type == MPCOMP_CHILDREN_LEAF );
	const int children_num = node->nb_children;
	me = &( root[node->global_rank] );
	assert( OPA_load_int( &( me->children_ready ) ) == children_num );

	for ( i = 0; i < children_num; i++ )
	{
		const int global_child_idx = first_idx + i;
		mpcomp_meta_tree_node_t *child = &( root[global_child_idx] );
		node->children.leaf[i] = ( mpcomp_mvp_t * ) child->user_pointer;
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

static inline int *__tree_array_get_rank( const int *shape, const int max_depth, const int rank )
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

static inline int *__tree_array_get_father_array_by_depth( const int *shape, const int max_depth, const int rank )
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


static inline int *__tree_array_get_first_child_array( const int *shape, const int max_depth, const int rank )
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

int *mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int max_depth, const int rank, const int core_depth )
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
	assert( root );
	assert( tree_shape );
	me = &( root[rank] );
	root_node = ( mpcomp_node_t * )root[0].user_pointer;
	assert( root_node );

	if ( !( me->user_pointer )  )
	{
		assert( rank );
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
		assert( me->fathers_array );
	}

	/* Children initialisation */
	me->children_array_size = max_depth - new_node->depth;
	me->first_child_array = ( int * ) __tree_array_get_first_child_array( tree_shape, max_depth, rank );

	/* -- TREE BASE -- */
	if ( rank )
	{
		new_node->tree_depth = max_depth - new_node->depth + 1;
		assert( new_node->tree_depth > 1 );
		new_node->tree_base = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
		assert( new_node->tree_base );
		new_node->tree_cumulative = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
		assert( new_node->tree_cumulative );
		new_node->tree_nb_nodes_per_depth = ( int * )mpcomp_alloc( sizeof( int ) * new_node->tree_depth );
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
	if ( new_node->depth < max_depth - 1 )
	{
		mpcomp_node_t **children = NULL;
		new_node->child_type = MPCOMP_CHILDREN_NODE;
		children = ( mpcomp_node_t ** )mpcomp_alloc( sizeof( mpcomp_node_t * ) * num_children );
		assert( children );
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
		mpc_thread_yield();
	}

	//(void) __tree_array_compute_parents( root_node, new_node );
	const int first_idx = me->first_child_array[0];

	switch ( new_node->child_type )
	{
		case MPCOMP_CHILDREN_NODE:
			__tree_array_update_children_node( first_idx, new_node, root );
			break;

		case MPCOMP_CHILDREN_LEAF:
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
    mpcomp_thread_t *ithread, *thread;
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
    /* MVP of initial thread becomes master thread MVP for upcoming parallel region */
    if( sctk_openmp_thread_tls ) {
        ithread = sctk_openmp_thread_tls;
        new_mvp = ithread->mvp;
        ithread->root = root_node;
    }
    else {
        new_mvp = (mpcomp_mvp_t *) mpcomp_alloc( sizeof(mpcomp_mvp_t) );
        assert( new_mvp );
        memset( new_mvp, 0, sizeof(mpcomp_mvp_t) );

        new_mvp->pu_id = unpack_args->pu_id;

        new_mvp->threads = (mpcomp_thread_t*) mpcomp_alloc( sizeof( mpcomp_thread_t) );
        assert( new_mvp->threads );
        memset( new_mvp->threads, 0, sizeof( mpcomp_thread_t ) );

#if OMPT_SUPPORT
        new_mvp->threads->mvp = new_mvp;
        new_mvp->threads->tool_status = unpack_args->tool_status;
        new_mvp->threads->tool_instance = unpack_args->tool_instance;

        __mpcompt_callback_thread_begin( new_mvp->threads, ompt_thread_other );
        __mpcompt_callback_thread_begin( new_mvp->threads, ompt_thread_worker );
#endif /* OMPT_SUPPORT */
    }

	/* Initialize the corresponding microVP (all but tree-related variables) */
	new_mvp->root = root_node;
	new_mvp->thread_self = mpc_thread_self();

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

    thread = new_mvp->threads;
	thread->next = NULL;
	thread->info.num_threads = 1;
	new_mvp->depth = max_depth;

    __mpcomp_thread_infos_init( thread );

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
		assert( new_mvp->spin_node );
		new_mvp->spin_node->spin_status = MPCOMP_MVP_STATE_SLEEP;
	}
	else /* The other children are regular leaves */
	{
		new_mvp->spin_node = NULL;
		new_mvp->spin_status = MPCOMP_MVP_STATE_SLEEP;
	}

	mpcomp_slave_mvp_node( new_mvp );

#if OMPT_SUPPORT
    __mpcompt_callback_thread_end( thread, ompt_thread_worker );
    __mpcompt_callback_thread_end( thread, ompt_thread_other );

    /* Mvp exit */
    if( thread->tool_status == active )
        OPA_incr_int( &thread->tool_instance->nb_native_threads_exited );
#endif /* OMPT_SUPPORT */

	return NULL;
}

void
__mpcomp_init_seq_region() {
	mpcomp_instance_t *seq_instance;
    mpcomp_mvp_t* new_mvp;
	int *singleton;

    /* Create an OpenMP context */
    /* Init sequential instance */
	seq_instance = ( mpcomp_instance_t * ) mpcomp_alloc( sizeof( mpcomp_instance_t ) );
	assert( seq_instance );
	memset( seq_instance, 0, sizeof( mpcomp_instance_t ) );

    /* Init sequential team */
	seq_instance->team = ( mpcomp_team_t * ) mpcomp_alloc( sizeof( mpcomp_team_t ) );
	assert( seq_instance->team );
	memset( seq_instance->team, 0, sizeof( mpcomp_team_t ) );

	seq_instance->team->depth = 0;
	seq_instance->root = NULL;
	seq_instance->nb_mvps = 1;

    /* Init MVP of sequential region */
    new_mvp = (mpcomp_mvp_t *) mpcomp_alloc( sizeof(mpcomp_mvp_t) );
    assert( new_mvp );
    memset( new_mvp, 0, sizeof(mpcomp_mvp_t) );

    /* Set PU on wich microvp is running */
    new_mvp->pu_id = mpc_topology_get_pu();

    /* Create associated initial thread */
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    new_mvp->threads = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
#else
    new_mvp->threads = (mpcomp_thread_t*) mpcomp_alloc( sizeof( mpcomp_thread_t ));
    assert( new_mvp->threads );
    memset( new_mvp->threads, 0, sizeof( mpcomp_thread_t ));

    sctk_openmp_thread_tls = ( void* ) new_mvp->threads;
#endif /* OMPT_SUPPORT */

    new_mvp->threads->mvp = new_mvp;
    new_mvp->threads->instance = seq_instance;

    /* Init tree infos of sequential instance */
	seq_instance->tree_array = (mpcomp_generic_node_t *) mpcomp_alloc( sizeof( mpcomp_generic_node_t ));
	assert( seq_instance->tree_array );
    memset( seq_instance->tree_array, 0, sizeof( mpcomp_generic_node_t ) );

	seq_instance->tree_array[0].ptr.mvp = new_mvp;
	seq_instance->tree_array[0].type = MPCOMP_CHILDREN_LEAF;
	seq_instance->mvps = seq_instance->tree_array;
	seq_instance->tree_depth = 1;

	singleton = ( int * ) mpcomp_alloc( sizeof( int ) );
	assert( singleton );

	singleton[0] = 1;
	seq_instance->tree_base = singleton;
	seq_instance->tree_cumulative = singleton;
	seq_instance->tree_nb_nodes_per_depth = singleton;

  /* Initialize the various allocators (OpenMP 5.0 Mem Management) */
  //master->default_allocator = icvs->def_allocator_var;

}

void
__mpcomp_init_initial_thread( const mpcomp_local_icv_t icvs )
{
    mpcomp_thread_t* ith;
    ith = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    assert( ith );

#if OMPT_SUPPORT
    __mpcompt_callback_thread_begin( ith, ompt_thread_initial );
#endif /* OMPT_SUPPORT */

    __mpcomp_thread_infos_init( ith );

#if defined( MPCOMP_OPENMP_3_0 )
	_mpc_task_tree_array_thread_init( ith );
#endif /* MPCOMP_OPENMP_3_0 */
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    /* Update initial task with enter frame info */
    __mpcompt_frame_set_enter( ith->frame_infos.ompt_frame_infos.enter_frame.ptr );
#endif /* OMPT_SUPPORT */

	/* Protect Orphan OpenMP construct */
	ith->info.icvs = icvs;
    ith->info.num_threads = 1;
}

void _mpc_omp_tree_alloc( int *shape, int max_depth, const int *cpus_order, const int place_depth, const int place_size )
{
	int i, n_num, place_id;
	mpc_thread_t *threads;
	mpcomp_mvp_thread_args_t *args;
	mpcomp_meta_tree_node_t *tree_array;

#if OMPT_SUPPORT
    mpcomp_thread_t* t = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    assert( t );
#endif /* OMPT_SUPPORT */

	mpcomp_node_t *root = ( mpcomp_node_t * ) mpcomp_alloc( sizeof( mpcomp_node_t ) );
	assert( root );
	memset( root, 0, sizeof( mpcomp_node_t ) );
#if 0

	for ( i = 0; i < max_depth; i++ )
	{
		fprintf( stderr, ":: %s :: shape %d -> %d\n", __func__, i, shape[i] );
	}

#endif
	root->tree_depth = max_depth + 1;
	root->tree_base = __tree_array_compute_shape( root, shape, max_depth );
	root->tree_cumulative = __tree_array_compute_cumulative_num_nodes_for_depth( root );
	root->tree_nb_nodes_per_depth = __tree_array_compute_num_nodes_for_depth( root, &n_num );
#ifdef MPC_Lowcomm
  root->worker_ws = mpcomp_alloc(sizeof(worker_workshare_t));
  root->worker_ws->mpi_rank = mpc_common_get_local_task_rank();
  root->worker_ws->workshare = mpc_thread_data_get()->workshare;
#endif
	const int leaf_n_num = root->tree_nb_nodes_per_depth[max_depth];
	const int non_leaf_n_num = n_num - leaf_n_num;

	tree_array = ( mpcomp_meta_tree_node_t * ) mpcomp_alloc( n_num * sizeof( mpcomp_meta_tree_node_t ) );
	assert( tree_array );
	memset( tree_array, 0,  n_num * sizeof( mpcomp_meta_tree_node_t ) );

	root->tree_array = tree_array;
	tree_array[0].user_pointer = root;

	threads = ( mpc_thread_t * ) mpcomp_alloc( ( leaf_n_num - 1 ) * sizeof( mpc_thread_t ) );
	assert( threads );
	memset( threads, 0, ( leaf_n_num - 1 ) * sizeof( mpc_thread_t ) );

	args = ( mpcomp_mvp_thread_args_t * ) mpcomp_alloc( sizeof( mpcomp_mvp_thread_args_t ) * leaf_n_num );
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
  mpcomp_alloc_init_allocators();

	/* Free is thread-safety due to half barrier perform by tree build */
	mpcomp_free( args );
}
