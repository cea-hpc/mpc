#ifndef __MPCOMP_SPINNING_CORE_H__
#define __MPCOMP_SPINNING_CORE_H__

#include "mpcomp_types.h"

#if defined( MPCOMP_OPENMP_3_0 )
	#include "mpcomp_task.h"
#endif /* defined( MPCOMP_OPENMP_3_0 ) */

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp );
void __scatter_instance_post_init( mpcomp_thread_t* thread );
mpcomp_mvp_t *_mpc_spin_node_wakeup( mpcomp_node_t *node );
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp );
void __mpcomp_exit_node_signal( mpcomp_node_t* node );
mpcomp_thread_t *__mvp_wakeup( mpcomp_mvp_t *mvp );

static inline void __mpcomp_instance_tree_array_root_init( struct mpcomp_node_s *root, mpcomp_instance_t *instance, const int nthreads )
{
	struct mpcomp_generic_node_s *meta_node;

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
	meta_node->type = MPCOMP_CHILDREN_NODE;
#if defined( MPCOMP_OPENMP_3_0 )
	_mpc_task_root_info_init( root );
#endif /* defined( MPCOMP_OPENMP_3_0 )  */
}

#if 0

static inline mpcomp_thread_t * __mpcomp_spinning_push_mvp_thread( mpcomp_mvp_t *mvp, mpcomp_thread_t *new_thread )
{
	assert( mvp );
	assert( new_thread );
	new_thread->father = mvp->threads;
	mvp->threads = new_thread;
	return new_thread;
}

static inline mpcomp_thread_t * __mpcomp_spinning_pop_mvp_thread( mpcomp_mvp_t *mvp )
{
	mpcomp_thread_t *cur_thread;
	assert( mvp );
	assert( mvp->threads );
	cur_thread = mvp->threads;
	mvp->threads = cur_thread->father;
	return cur_thread;
}

static inline mpcomp_thread_t * __mpcomp_spinning_get_thread_ancestor( mpcomp_thread_t *thread, const int depth )
{
	int i;
	mpcomp_thread_t *ancestor;
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

static inline mpcomp_thread_t * __mpcomp_spinning_get_mvp_thread_ancestor( mpcomp_mvp_t *mvp, const int depth )
{
	mpcomp_thread_t *mvp_thread, *ancestor;
	assert( mvp );
	mvp_thread = mvp->threads;
	ancestor = __mpcomp_spinning_get_thread_ancestor( mvp_thread, depth );
	return ancestor;
}

static inline mpcomp_node_t * __mpcomp_spinning_get_thread_root_node( mpcomp_thread_t *thread )
{
	assert( thread );
	return thread->root;
}

static inline int __mpcomp_spining_get_instance_max_depth( mpcomp_instance_t *instance )
{
	mpcomp_node_t *root = instance->root;
	assert( root ); //TODO instance root can be NULL when leaf create new parallel region
	return root->depth + root->tree_depth - 1;
}

static inline char * __mpcomp_spinning_get_debug_thread_infos( __UNUSED__ mpcomp_thread_t *thread )
{
	return NULL;
}

static inline mpcomp_node_t * __mpcomp_spinning_get_mvp_father_node( mpcomp_mvp_t *mvp, mpcomp_instance_t *instance )
{
	int *mvp_father_array;
	mpcomp_meta_tree_node_t *tree_array_node, *tree_array;
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
	return ( mpcomp_node_t * ) tree_array_node->user_pointer;
}

static inline int __mpcomp_spinning_node_compute_rank( mpcomp_node_t *node, const int num_threads, int rank, int *first )
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

static inline int __mpcomp_spinning_leaf_compute_rank( mpcomp_node_t *node, const int num_threads, const int first_rank, const int rank )
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

#endif /* __MPCOMP_SPINNING_CORE_H__ */
