#ifndef __MPC_OMP_SPINNING_CORE_H__
#define __MPC_OMP_SPINNING_CORE_H__

# include "mpcomp_types.h"
# include "mpcomp_task.h"

void _mpc_omp_start_openmp_thread( mpc_omp_mvp_t *mvp );
void __scatter_instance_post_init( mpc_omp_thread_t* thread );
mpc_omp_mvp_t *_mpc_spin_node_wakeup( mpc_omp_node_t *node );
void mpc_omp_slave_mvp_node( mpc_omp_mvp_t *mvp );
void _mpc_omp_exit_node_signal( mpc_omp_node_t* node );
mpc_omp_thread_t *__mvp_wakeup( mpc_omp_mvp_t *mvp );

static inline void _mpc_omp_instance_tree_array_root_init( struct mpc_omp_node_s *root, mpc_omp_instance_t *instance, const int nthreads )
{
	struct mpc_omp_generic_node_s *meta_node;

	if ( !root )
	{
		return;
	}

	root->mvp_first_id = 0;
	root->instance = instance;
	root->num_threads = nthreads;
	root->instance_global_rank = 1;
	root->instance_stage_first_rank = 1;
	meta_node = &( instance->tree_array[0] );
	meta_node->ptr.node = root;
	meta_node->type = MPC_OMP_CHILDREN_NODE;
	_mpc_omp_task_root_info_init( root );
}

#if 0

static inline mpc_omp_thread_t * _mpc_omp_spinning_push_mvp_thread( mpc_omp_mvp_t *mvp, mpc_omp_thread_t *new_thread )
{
	assert( mvp );
	assert( new_thread );
	new_thread->father = mvp->threads;
	mvp->threads = new_thread;
	return new_thread;
}

static inline mpc_omp_thread_t * _mpc_omp_spinning_pop_mvp_thread( mpc_omp_mvp_t *mvp )
{
	mpc_omp_thread_t *cur_thread;
	assert( mvp );
	assert( mvp->threads );
	cur_thread = mvp->threads;
	mvp->threads = cur_thread->father;
	return cur_thread;
}

static inline mpc_omp_thread_t * _mpc_omp_spinning_get_thread_ancestor( mpc_omp_thread_t *thread, const int depth )
{
	int i;
	mpc_omp_thread_t *ancestor;
	ancestor = thread;

	for ( i = 0; i < depth; i++ )
	{
		if ( ancestor == NULL )
		{
			break;
		}

		ancestor = ancestor->father;
	}

	return ancestor;
}

static inline mpc_omp_thread_t * _mpc_omp_spinning_get_mvp_thread_ancestor( mpc_omp_mvp_t *mvp, const int depth )
{
	mpc_omp_thread_t *mvp_thread, *ancestor;
	assert( mvp );
	mvp_thread = mvp->threads;
	ancestor = _mpc_omp_spinning_get_thread_ancestor( mvp_thread, depth );
	return ancestor;
}

static inline mpc_omp_node_t * _mpc_omp_spinning_get_thread_root_node( mpc_omp_thread_t *thread )
{
	assert( thread );
	return thread->root;
}

static inline int _mpc_omp_spining_get_instance_max_depth( mpc_omp_instance_t *instance )
{
	mpc_omp_node_t *root = instance->root;
	assert( root ); //TODO instance root can be NULL when leaf create new parallel region
	return root->depth + root->tree_depth - 1;
}

static inline char * _mpc_omp_spinning_get_debug_thread_infos( __UNUSED__ mpc_omp_thread_t *thread )
{
	return NULL;
}

static inline mpc_omp_node_t * _mpc_omp_spinning_get_mvp_father_node( mpc_omp_mvp_t *mvp, mpc_omp_instance_t *instance )
{
	int *mvp_father_array;
	mpc_omp_meta_tree_node_t *tree_array_node, *tree_array;
	assert( mvp );
	assert( instance );

	if ( !( instance->root ) )
	{
		return  mvp->father;
	}

	const int max_depth = mvp->depth;
	const int father_depth = instance->root->depth + instance->tree_depth - 1;
	const unsigned int mvp_ancestor_node = max_depth - father_depth;

	if ( !mvp_ancestor_node )
	{
		return mvp->father;
	}

	assert( mvp->father );
	tree_array = mvp->father->tree_array;
	assert( tree_array );
	mvp_father_array = tree_array[mvp->global_rank].fathers_array;
	assert( mvp_ancestor_node < tree_array[mvp->global_rank].fathers_array_size );
	tree_array_node = &( tree_array[mvp_father_array[father_depth - 1]] );
	return ( mpc_omp_node_t * ) tree_array_node->user_pointer;
}

static inline int _mpc_omp_spinning_node_compute_rank( mpc_omp_node_t *node, const int num_threads, int rank, int *first )
{
	assert( node );
	assert( num_threads > 0 );
	assert( first );
	const int quot = num_threads / node->nb_children;
	const int rest = num_threads % node->nb_children;
	const int min = ( num_threads > node->nb_children ) ? node->nb_children : num_threads;
	*first = -1;

	if ( rank >= min )
	{
		return 0;
	}

	*first = ( rank < rest ) ? ( ( quot + 1 ) * rank ) : ( quot * rank + rest );
	return ( rank < rest ) ? quot + 1 : quot;
}

static inline int _mpc_omp_spinning_leaf_compute_rank( mpc_omp_node_t *node, const int num_threads, const int first_rank, const int rank )
{
	int i;
	assert( node );
	assert( num_threads > 0 );
	const int quot = node->nb_children / num_threads;

	for ( i = 0; i < node->nb_children; i++ )
	{
		if ( i * quot + first_rank == rank )
		{
			break;
		}
	}

	return i;
}

#endif

#endif /* __MPC_OMP_SPINNING_CORE_H__ */
