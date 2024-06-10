/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:58 CEST 2021                                        # */
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
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpc_common_asm.h"
#include "mpcomp_sync.h"
#include "mpc_thread.h"
#include "mpcomp_spinning_core.h"
#include "mpcomp_tree.h"
#include "mpcompt_sync.h"
#include "mpcompt_task.h"
#include "mpcompt_frame.h"

#include "mpcomp_task.h"

#ifndef NDEBUG
	#include "sctk_pm_json.h"
#endif /* !NDEBUG */

void mpc_lowcomm_workshare_worker_steal(int rank);

static inline int __scatter_compute_node_num_thread( mpc_omp_node_t *node, const int num_threads, int rank, int *first )
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

static inline int __scatter_compute_instance_tree_depth( mpc_omp_node_t *node, const int expected_nb_mvps )
{
	int next_depth, num_nodes;
	assert( expected_nb_mvps > 0 );

	/* No more nested parallelism */
	if ( node == NULL || expected_nb_mvps == 1 )
	{
		return 1;
	}

	for ( next_depth = 0; next_depth < node->tree_depth; next_depth++ )
	{
		num_nodes = node->tree_nb_nodes_per_depth[next_depth];

		if ( num_nodes >= expected_nb_mvps )
		{
			break;
		}
	}

	return next_depth + 1;
}

static inline int __scatter_compute_instance_array_info( mpc_omp_instance_t *instance, const int expected_nb_mvps )
{
	mpc_omp_node_t *root;
	int i, j, last_level_shape, tot_nnodes;
	int *num_nodes_per_depth, *shape, *num_children_per_depth, *first_rank_per_depth;
	assert( instance );
	root = instance->root;

	if ( !root || expected_nb_mvps == 1 )
	{
		assert( expected_nb_mvps == 1 );
		int *singleton = ( int * ) mpc_omp_alloc( sizeof( int ) );
		assert( singleton );
		singleton[0] = 1;
		first_rank_per_depth = ( int * ) mpc_omp_alloc( sizeof( int ) );
		assert( first_rank_per_depth );
		first_rank_per_depth[0] = 0;
		instance->tree_base = singleton;
		instance->tree_cumulative = singleton;
		instance->tree_nb_nodes_per_depth = singleton;
		instance->tree_first_node_per_depth = first_rank_per_depth;
		return 1;
	}

	assert( root );
	assert( root->tree_base );
	const int array_size = instance->tree_depth;
	last_level_shape = expected_nb_mvps;
	/* Instance tree_base */
	shape = ( int * ) mpc_omp_alloc( array_size * sizeof( int ) );
	assert( shape );
	memset( shape, 0, array_size * sizeof( int ) );
	/* Instance tree_nb_nodes_per_depth */
	num_nodes_per_depth = ( int * ) mpc_omp_alloc( ( array_size ) * sizeof( int ) );
	assert( num_nodes_per_depth );
	memset( num_nodes_per_depth, 0, array_size * sizeof( int ) );
	/* Instance tree_cumulative */
	num_children_per_depth = ( int * ) mpc_omp_alloc( ( array_size + 1 ) * sizeof( int ) );
	assert( num_children_per_depth );
	memset( num_children_per_depth, 0, ( array_size + 1 ) * sizeof( int ) );
	/* Instance tree first level rank */
	first_rank_per_depth = ( int * ) mpc_omp_alloc( array_size * sizeof( int ) );
	assert( num_children_per_depth );
	memset( num_children_per_depth, 0, array_size * sizeof( int ) );
	/* Intermediate stage */
	tot_nnodes = 0;
	first_rank_per_depth[0] = 0;

	for ( i = 0; i < array_size - 1; i++ )
	{
		shape[i] = root->tree_base[i];
		tot_nnodes += root->tree_nb_nodes_per_depth[i];
		num_nodes_per_depth[i] = root->tree_nb_nodes_per_depth[i];
		const int quot = last_level_shape / root->tree_base[i];
		const int rest = last_level_shape % root->tree_base[i];
		last_level_shape = ( rest ) ? ( quot + 1 ) : quot;
		first_rank_per_depth[i + 1] = first_rank_per_depth[i] + shape[i];
	}

	/* Last stage */
	shape[array_size - 1] = last_level_shape;
	num_nodes_per_depth[array_size - 1] = ( array_size > 2 ) ? num_nodes_per_depth[array_size - 2] : 1;
	num_nodes_per_depth[array_size - 1] *= shape[array_size - 1];
	tot_nnodes += num_nodes_per_depth[array_size - 1];

	/* tree_cumulative update */
	for ( i = 0; i < array_size; i++ )
	{
		num_children_per_depth[i] = 1;

		for ( j = i + 1; j < array_size; j++ )
		{
			num_children_per_depth[i] *= shape[j];
			assert( num_children_per_depth[i] );
		}
	}

	/* Update instance */
	instance->tree_base = shape;
	instance->tree_nb_nodes_per_depth = num_nodes_per_depth;
	instance->tree_cumulative = num_children_per_depth;
	instance->tree_first_node_per_depth = first_rank_per_depth;
#if 0 // NDEBUG
	json_t *json_node_tree_base = json_array();
	json_t *json_node_tree_cumul = json_array();
	json_t *json_node_tree_lsize = json_array();

	for ( i = 0; i < array_size; i++ )
	{
		json_array_push( json_node_tree_base, json_int( instance->tree_base[i] ) );
		json_array_push( json_node_tree_cumul, json_int( instance->tree_cumulative[i] ) );
		json_array_push( json_node_tree_lsize, json_int( instance->tree_nb_nodes_per_depth[i] ) );
	}

	json_t *json_node_obj = json_object();
	json_object_set( json_node_obj, "tree_array_size", json_int( tot_nnodes ) );
	json_object_set( json_node_obj, "tree_base", json_node_tree_base );
	json_object_set( json_node_obj, "tree_cumulative", json_node_tree_cumul );
	json_object_set( json_node_obj, "tree_nb_nodes_per_depth", json_node_tree_lsize );;
	char *json_char_node = json_dump( json_node_obj, JSON_COMPACT );
	json_decref( json_node_obj );
	fprintf( stderr, ":: %s :: %s\n", __func__, json_char_node );
	free( json_char_node );
#endif /* NDEBUG */
	return tot_nnodes;
}


static inline mpc_omp_instance_t *__scatter_instance_pre_init( mpc_omp_thread_t *thread, const int num_mvps )
{
	mpc_omp_node_t *root;
	mpc_omp_instance_t *instance;
	assert( thread );
	assert( num_mvps > 0 );
	root = thread->root;
	instance = ( mpc_omp_instance_t * ) mpc_omp_alloc( sizeof( mpc_omp_instance_t ) );
	assert( instance );
	memset( instance, 0, sizeof( mpc_omp_instance_t ) );
	instance->root = root;
	instance->tree_depth = __scatter_compute_instance_tree_depth( root, num_mvps );
	instance->tree_array_size = __scatter_compute_instance_array_info( instance, num_mvps );
	//fprintf(stderr, "%s> tree_depth: %d tree_array: %d num_mvps : %d\n", __func__, instance->tree_depth, instance->tree_array_size, num_mvps);
	instance->tree_array = ( mpc_omp_generic_node_t * )mpc_omp_alloc( sizeof( mpc_omp_generic_node_t ) * instance->tree_array_size );
	assert( instance->tree_array );
	memset( instance->tree_array, 0, sizeof( mpc_omp_generic_node_t ) * instance->tree_array_size );
	/* First Mvp entry in tree_array */
	const int instance_last_stage_size = instance->tree_cumulative[0];
	assert( !root || instance_last_stage_size <= root->tree_nb_nodes_per_depth[instance->tree_depth - 1] );
	const int first_mvp = instance->tree_array_size - instance_last_stage_size;
	instance->mvps = &( instance->tree_array[first_mvp] );
	assert( instance_last_stage_size >= num_mvps );
	return instance;
}

static inline void __instance_tree_array_node_init( struct mpc_omp_node_s *parent, struct mpc_omp_node_s *child, const int index )
{
	int i;
	mpc_omp_instance_t *instance;
	struct mpc_omp_generic_node_s *meta_node;
	assert( parent->instance );
	instance = parent->instance;
	const int vdepth = parent->depth - parent->instance->root->depth + 1;
	const int next_tree_base_val = parent->instance->tree_base[vdepth + 1];
	const int global_rank = parent->instance_global_rank + index;
	child->instance_stage_first_rank = parent->instance_stage_first_rank + instance->tree_nb_nodes_per_depth[vdepth];
	const int local_rank = global_rank - parent->instance_stage_first_rank;
	child->instance_global_rank = child->instance_stage_first_rank + local_rank * next_tree_base_val;
	meta_node = &( parent->instance->tree_array[global_rank] );
	assert( meta_node );
	meta_node->ptr.node = child;
	meta_node->type = MPC_OMP_CHILDREN_NODE;
	child->tree_array_rank = global_rank;
	child->tree_array_ancestor_path = ( int * ) mpc_omp_alloc( ( vdepth + 1 ) * sizeof( int ) );
	assert( child->tree_array_ancestor_path );
	memset( child->tree_array_ancestor_path, 0, ( vdepth + 1 ) * sizeof( int ) );

	for ( i = 0; i < vdepth - 1; i++ )
	{
		child->tree_array_ancestor_path[i] = parent->tree_array_ancestor_path[i];
	}

	child->tree_array_ancestor_path[vdepth - 1] = parent->tree_array_rank;
	child->tree_array_ancestor_path[vdepth] = child->tree_array_rank;
	_mpc_omp_task_node_info_init( parent, child );
}

static inline void __instance_tree_array_mvp_init( struct mpc_omp_node_s *parent, struct mpc_omp_mvp_s *mvp, const int index )
{
	int i;
	struct mpc_omp_generic_node_s *meta_node;
	const int vdepth = parent->depth - parent->instance->root->depth + 1;
	const int global_rank = parent->instance_global_rank + index;
	meta_node = &( parent->instance->tree_array[global_rank] );
	assert( meta_node );
	meta_node->ptr.mvp = mvp;
	meta_node->type = MPC_OMP_CHILDREN_LEAF;
	assert( mvp->threads );
	assert( mvp->threads );
	mvp->threads->tree_array_rank = global_rank;
	mvp->threads->tree_array_ancestor_path = ( int * ) mpc_omp_alloc( ( vdepth + 1 ) * sizeof( int ) );
	assert( mvp->threads->tree_array_ancestor_path );
	memset( mvp->threads->tree_array_ancestor_path, 0,  ( vdepth + 1 ) * sizeof( int ) );

	for ( i = 0; i < vdepth - 1; i++ )
	{
		mvp->threads->tree_array_ancestor_path[i] = parent->tree_array_ancestor_path[i];
	}

	mvp->threads->tree_array_ancestor_path[vdepth - 1] = parent->tree_array_rank;
    mvp->threads->tree_array_ancestor_path[vdepth] = mvp->threads->tree_array_rank;
    _mpc_omp_task_mvp_info_init( parent, mvp );
}

static inline mpc_omp_node_t *__scatter_wakeup_intermediate_node( mpc_omp_node_t *node )
{
	int i;
	mpc_omp_node_t *child_node;
	assert( node->instance );
	assert( node->child_type == MPC_OMP_CHILDREN_NODE );
	assert( node->instance->root->depth <= node->depth );
	const int node_vdepth = node->depth - node->instance->root->depth + 1;
	const int num_vchildren = node->instance->tree_base[node_vdepth];
	const int num_children = node->nb_children;
	const int nthreads = node->num_threads;
	assert( num_children >= num_vchildren );
	const int node_first_mvp = node->mvp_first_id;
	assert( node->children.node );

#ifdef MPC_OMP_USE_INTEL_ABI
    struct common_table * th_pri_common;
#endif
#if OMPT_SUPPORT
    ompt_data_t data;
    mpc_omp_ompt_tool_status_t tool_status;
    mpc_omp_ompt_tool_instance_t* tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
    mpc_omp_ompt_frame_info_t frame_infos;
#endif
#endif /* OMPT_SUPPORT */

	for ( i = 0; i < node->nb_children * 64; i += 64 )
	{
		node->reduce_data[i] = NULL;
		node->isArrived[i] = 0;
	}

	/** Instance id : First id next step + cur_node * tree_base + i */
	if ( nthreads > num_vchildren )
	{
		const int min_nthreads = nthreads / num_children;
		const int ext_nthreads = nthreads % num_children;
		node->barrier_num_threads = num_vchildren;

		for ( i = 0; i < num_vchildren; i++ )
		{
			child_node = node->children.node[i];
			assert( child_node );
			child_node->already_init = 0;

			if ( !node->instance->buffered )
			{
				child_node->num_threads = min_nthreads + ( ( i < ext_nthreads ) ? 1 : 0 );
				child_node->mvp_first_id = min_nthreads * i + ( ( i < ext_nthreads ) ? i : ext_nthreads );
				child_node->mvp_first_id += node_first_mvp;
				child_node->instance = node->instance;
				__instance_tree_array_node_init( node, child_node, i );
			}

			child_node->spin_status = MPC_OMP_MVP_STATE_AWAKE;
		}
	}
	else
	{
		int cur_node;
		mpc_omp_mvp_t *mvp;
		const int min_shift = num_children / nthreads;
		const int ext_shift = num_children % nthreads;
		node->barrier_num_threads = nthreads;
		cur_node = 0;

		for ( i = 0; i < nthreads; i++ )
		{
			mpc_omp_thread_t *next;
			child_node = node->children.node[cur_node];
			assert( child_node );
			mvp = child_node->mvp;
			assert( mvp );
			child_node->already_init = 1;

			if ( !node->instance->buffered )
			{
#if OMPT_SUPPORT
                data = mvp->threads->ompt_thread_data;
                tool_status = mvp->threads->tool_status;
                tool_instance = mvp->threads->tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
                frame_infos = mvp->threads->frame_infos;
#endif
#endif /* OMPT_SUPPORT */
#ifdef MPC_OMP_USE_INTEL_ABI
                th_pri_common = mvp->threads->th_pri_common;
#endif
				child_node->num_threads = 1;
				child_node->instance = node->instance;
				child_node->mvp_first_id = node_first_mvp + i;
				/* Set MVP infos */
				assert( mvp->threads );
				next = mvp->threads->next;
				memset( mvp->threads, 0, sizeof( mpc_omp_thread_t ) );
				mvp->threads->mvp = mvp;
				mvp->threads->instance = child_node->instance;
				mvp->threads->root = child_node;
				mvp->threads->next = next;
				mvp->threads->father = node->instance->thread_ancestor;
				mvp->threads->father_node = node;
				mvp->threads->rank = node->mvp_first_id + i;
				mvp->instance = node->instance;
#ifdef MPC_OMP_USE_INTEL_ABI
                mvp->threads->th_pri_common = th_pri_common;
#endif
#if OMPT_SUPPORT
                mvp->threads->ompt_thread_data = data;
                mvp->threads->tool_status = tool_status;
                mvp->threads->tool_instance = tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
                mvp->threads->frame_infos = frame_infos;
#endif
#endif /* OMPT_SUPPORT */
				int j;

				for ( j = 0; j < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; j++ )
				{
					OPA_store_int( &( mvp->threads->for_dyn_remain[j].i ), -1 );
				}

				__instance_tree_array_mvp_init( node, mvp, i );
			}

			/* WakeUp NODE */
			child_node->spin_status = MPC_OMP_MVP_STATE_AWAKE;
			cur_node += min_shift + ( ( i < ext_shift ) ? 1 : 0 );
		}
	}

	return node->children.node[0];
}

static inline mpc_omp_mvp_t *__scatter_wakeup_final_mvp( mpc_omp_node_t *node )
{
	int i, cur_mvp;
	assert( node );
	assert( node->child_type == MPC_OMP_CHILDREN_LEAF );
	assert( node->instance->root->depth <= node->depth );
	const int node_vdepth = node->depth - node->instance->root->depth + 1;
	const int  __UNUSED__ num_vchildren = node->instance->tree_base[node_vdepth];
	assert( node->num_threads <= num_vchildren );
	const int nthreads = node->num_threads;
	const int num_children = node->nb_children;
	assert( nthreads <= num_children );
	const int min_shift = num_children / nthreads;
	const int ext_shift = num_children % nthreads;
	node->barrier_num_threads = node->num_threads;
	cur_mvp = 0;

#ifdef MPC_OMP_USE_INTEL_ABI
    struct common_table * th_pri_common;
#endif
#if OMPT_SUPPORT
    ompt_data_t data;
    mpc_omp_ompt_tool_status_t tool_status;
    mpc_omp_ompt_tool_instance_t* tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
    mpc_omp_ompt_frame_info_t frame_infos;
#endif
#endif /* OMPT_SUPPORT */

	for ( i = 0; i < node->nb_children * 64; i += 64 )
	{
		node->reduce_data[i] = NULL;
		node->isArrived[i] = 0;
	}

	for ( i = 0; i < nthreads; i++ )
	{
		mpc_omp_thread_t *next;
		mpc_omp_mvp_t *mvp = node->children.leaf[cur_mvp];

		if ( !node->instance->buffered )
		{
#if OMPT_SUPPORT
            data = mvp->threads->ompt_thread_data;
            tool_status = mvp->threads->tool_status;
            tool_instance = mvp->threads->tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
            frame_infos = mvp->threads->frame_infos;
#endif
#endif /* OMPT_SUPPORT */
#ifdef MPC_OMP_USE_INTEL_ABI
            th_pri_common = mvp->threads->th_pri_common;
#endif
			mvp->instance = node->instance;
			/* Set MVP instance rank in thread structure */
			assert( mvp->threads );
			next = mvp->threads->next;
			memset( mvp->threads, 0, sizeof( mpc_omp_thread_t ) );
			mvp->threads->mvp = mvp;
			mvp->threads->instance = node->instance;
			mvp->threads->root = NULL;
			mvp->threads->next = next;
			mvp->threads->father = node->instance->thread_ancestor;
			mvp->threads->father_node = node;
			mvp->threads->rank = node->mvp_first_id + i;
#ifdef MPC_OMP_USE_INTEL_ABI
            mvp->threads->th_pri_common = th_pri_common;
#endif
#if OMPT_SUPPORT
            mvp->threads->ompt_thread_data = data;
            mvp->threads->tool_status = tool_status;
            mvp->threads->tool_instance = tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
            mvp->threads->frame_infos = frame_infos;
#endif
#endif /* OMPT_SUPPORT */
			int j;

			for ( j = 0; j < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; j++ )
			{
				OPA_store_int( &( mvp->threads->for_dyn_remain[j].i ), -1 );
			}

			__instance_tree_array_mvp_init( node, mvp, i );
		}

		/* WakeUp MVP */
		mvp->spin_status = MPC_OMP_MVP_STATE_AWAKE;
		cur_mvp += min_shift + ( ( i < ext_shift ) ? 1 : 0 );
	}

	return node->children.leaf[0];
}


static inline mpc_omp_mvp_t *__scatter_wakeup_final_node( mpc_omp_node_t *node )
{
	assert( node );
	assert( node->mvp );
	node->already_init = 0;
	return node->mvp;
}

static inline mpc_omp_mvp_t *__scatter_wakeup( mpc_omp_node_t *node )
{
	mpc_omp_node_t *cur_node;
	assert( node->instance );
	assert( node->instance->root );
	const int target_depth = node->instance->tree_depth + node->instance->root->depth - 1;
	/* Node Wake-Up */
	cur_node = node;

	while ( cur_node->depth < target_depth )
	{
		if ( cur_node->already_init || cur_node->child_type != MPC_OMP_CHILDREN_NODE )
		{
			break;
		}

		cur_node = __scatter_wakeup_intermediate_node( cur_node );
	}

	if ( cur_node->depth == target_depth || cur_node->already_init )
	{
		return __scatter_wakeup_final_node( cur_node );
	}

	return __scatter_wakeup_final_mvp( cur_node );
}

void __scatter_instance_post_init( mpc_omp_thread_t *thread )
{
	assert( thread );
	assert( thread->mvp );
	mpc_omp_mvp_t *mvp = thread->mvp;
	int j;

	for ( j = 0; j < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; j++ )
	{
		OPA_store_int( &( mvp->threads->for_dyn_remain[j].i ), -1 );
	}

	if ( ! thread->mvp->instance->buffered )
	{
		mpc_omp_barrier(ompt_sync_region_barrier_implementation);
	}

#if 0  /* Check victim list for each thread */
	int  i, total, current, nbList;
	int *victim_list;
	const int nb_victims = _mpc_task_new_utils_extract_victims_list( &victim_list, &nbList, 0 );
	char string_array[256];

	for ( i = 0, total = 0; i < nb_victims; i++ )
	{
		current = snprintf( string_array + total, 256 - total, " %d", victim_list[i] );
		total += current;
	}

	string_array[total] = '\0';
	const int node_rank = MPC_OMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, 0 );
	fprintf( stderr, "#%d - Me : %d -- Stealing list =%s nbList : %d\n", thread->rank, node_rank, string_array, nbList );
	//mpc_omp_barrier();
#endif /* Check victim list for each thread */
	thread->mvp->instance->buffered = 1;
#if 0

	if ( thread->rank == 0 )
	{
		instance = thread->mvp->instance;

		for ( i = 1; i < instance->tree_array_size; i++ )
		{
			switch ( instance->tree_array[i].type )
			{
				case MPC_OMP_CHILDREN_NODE:
					fprintf( stderr, ":: %s :: node> %d -- type> %s\n", __func__, instance->tree_array[i].ptr.node->global_rank, "MPC_OMP_CHILDREN_NODE" );
					break;

				case MPC_OMP_CHILDREN_LEAF:
					fprintf( stderr, ":: %s :: node> %d -- type> %s\n", __func__, instance->tree_array[i].ptr.mvp->global_rank, "MPC_OMP_CHILDREN_LEAF" );
					break;

				case MPC_OMP_CHILDREN_NULL:
					fprintf( stderr, ":: %s :: empty node -> %d\n", __func__, i );
					break;

				default:
					assert( 0 );
			}
		}
	}

#endif
	//    MPC_OMP_TASK_TEAM_CMPL_INIT(thread->instance->team);
}

static OPA_int_t nb_teams = OPA_INT_T_INITIALIZER( 0 );

/** Reset mpcomp_team informations */
static inline void __team_reset( mpc_omp_team_t *team )
{
	assert( team );
	OPA_int_t *last_array_slot;
	memset( team, 0, sizeof( mpc_omp_team_t ) );
	last_array_slot = &( team->for_dyn_nb_threads_exited[MPC_OMP_MAX_ALIVE_FOR_DYN].i );
	OPA_store_int( last_array_slot, MPC_OMP_NOWAIT_STOP_SYMBOL );
	team->id = OPA_fetch_and_incr_int( &nb_teams );
	mpc_common_spinlock_init( &team->lock, 0 );
	mpc_common_spinlock_init( &team->atomic_lock, 0 );
	team->critical_lock = ( omp_lock_t * ) mpc_omp_alloc( sizeof( omp_lock_t ) );
	memset( team->critical_lock, 0, sizeof( omp_lock_t ) );
	mpc_common_spinlock_init( &team->critical_lock->lock, 0 );
}

static inline void __mvp_del_saved_ctx( mpc_omp_mvp_t *mvp )
{
	mpc_omp_mvp_saved_context_t *prev_mvp_context = NULL;
	assert( mvp );
	/* Get previous context */
	prev_mvp_context =  mvp->prev_node_father;
	assert( prev_mvp_context );
	/* Restore MVP previous status */
	mvp->father = prev_mvp_context->father;
	mvp->prev_node_father = prev_mvp_context->prev;
    TODO("reuse mechanism");
    mpc_omp_free( prev_mvp_context );
}

static inline void __mvp_add_saved_ctx( mpc_omp_mvp_t *mvp )
{
	mpc_omp_mvp_saved_context_t *prev_mvp_context = NULL;
	assert( mvp );
	/* Allocate new chained elt */
	prev_mvp_context = ( mpc_omp_mvp_saved_context_t * ) mpc_omp_alloc( sizeof( mpc_omp_mvp_saved_context_t ) );
	assert( prev_mvp_context );
	memset( prev_mvp_context, 0, sizeof( mpc_omp_mvp_saved_context_t ) );
	/* Get previous context */
	prev_mvp_context->prev = mvp->prev_node_father;
	prev_mvp_context->father = mvp->father;
	mvp->prev_node_father = prev_mvp_context;
}

mpc_omp_instance_t *_mpc_omp_tree_array_instance_init( mpc_omp_thread_t *thread, const int expected_nb_mvps )
{
	mpc_omp_thread_t *master;
	mpc_omp_instance_t *instance;
	assert( thread );
	assert( expected_nb_mvps > 0 );
	instance =  __scatter_instance_pre_init( thread, expected_nb_mvps );
	assert( instance );

	instance->nb_mvps = expected_nb_mvps;

	instance->team = ( mpc_omp_team_t * ) mpc_omp_alloc( sizeof( mpc_omp_team_t ) );
	assert( instance->team );
	memset( instance->team, 0, sizeof( mpc_omp_team_t ) );

	instance->thread_ancestor = thread;
	__team_reset( instance->team );

	_mpc_omp_task_team_info_init( instance->team, instance->tree_depth );

	/* Allocate new thread context */
	master = ( mpc_omp_thread_t * ) mpc_omp_alloc( sizeof( mpc_omp_thread_t ) );
	assert( master );
	memset( master, 0, sizeof( mpc_omp_thread_t ) );

	master->next = thread;
	master->mvp = thread->mvp;
	master->root = thread->root;
	master->father = thread->father;
	thread->mvp->threads = master;
	instance->master = master;

    _mpc_omp_thread_infos_init( master );

#if OMPT_SUPPORT
    master->ompt_thread_data = thread->ompt_thread_data;
    /* Transfer common tool instance infos */
    master->tool_status = thread->tool_status;
    master->tool_instance = thread->tool_instance;
#if MPCOMPT_HAS_FRAME_SUPPORT
    master->frame_infos.outter_caller = thread->frame_infos.outter_caller;
    master->frame_infos.ompt_return_addr = thread->frame_infos.ompt_return_addr;
#endif
#endif /* OMPT_SUPPORT */

    /* instance initialization */
    instance->task_infos.blocked_tasks.type     = MPC_OMP_TASK_LIST_TYPE_SCHEDULER;
    instance->task_infos.propagation.up.type    = MPC_OMP_TASK_LIST_TYPE_UP_DOWN;
    instance->task_infos.propagation.down.type  = MPC_OMP_TASK_LIST_TYPE_UP_DOWN;
# if MPC_OMP_BARRIER_COMPILE_COND_WAIT
    if (MPC_OMP_TASK_BARRIER_COND_WAIT_ENABLED)
    {
        pthread_mutex_init(&instance->task_infos.work_cond_mutex, NULL);
        pthread_cond_init(&instance->task_infos.work_cond, NULL);
        instance->task_infos.work_cond_nthreads = 0;
    }
# endif /* MPC_OMP_BARRIER_COMPILE_COND_WAIT */

	return instance;
}

mpc_omp_thread_t *__mvp_wakeup( mpc_omp_mvp_t *mvp )
{
	mpc_omp_thread_t *new_thread;
	assert( mvp );

	new_thread = mvp->threads;
	assert( new_thread );

    mpc_omp_tls = (void*) new_thread;

	mvp->father = new_thread->father_node;

#if MPC_OMP_TRANSFER_INFO_ON_NODES
	new_thread->info = mvp->info;
#else /* MPC_OMP_TRANSFER_INFO_ON_NODES */
	new_thread->info = mvp->instance->team->info;
#endif
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    new_thread->frame_infos.outter_caller =
        mvp->instance->team->frame_infos.outter_caller;
    new_thread->frame_infos.ompt_return_addr = mvp->instance->team->frame_infos.ompt_return_addr;
#endif /* OMPT_SUPPORT */

	/* Set thread rank */
	new_thread->mvp = mvp;
	new_thread->instance = mvp->instance;

	mpc_common_spinlock_init( &( new_thread->info.update_lock ), 0 );
	/* Reset pragma for dynamic internal */

    _mpc_omp_task_tree_init(new_thread);

	return new_thread;
}

mpc_omp_mvp_t *_mpc_spin_node_wakeup( mpc_omp_node_t *node )
{
	if ( !node )
	{
		return NULL;
	}

	return __scatter_wakeup( node );
}

void
_mpc_omp_exit_node_signal( mpc_omp_node_t* node ) {
    int i;
    assert( node );

    /* We are the mvp spinning on the deepest node, wake-up regular leaves and exit */
    if( node->child_type == MPC_OMP_CHILDREN_LEAF ) {
        for( i = 1; i < node->nb_children; i++ ) {
            node->children.leaf[i]->enable = 0;
            node->children.leaf[i]->spin_status = MPC_OMP_MVP_STATE_AWAKE;
        }

        return;
    }

    /* Wake-up intermediate nodes */
    for( i = 1; i < node->nb_children; i++ ) {
        node->children.node[i]->mvp->enable = 0;
        node->children.node[i]->spin_status = MPC_OMP_MVP_STATE_AWAKE;
    }

    _mpc_omp_exit_node_signal( node->children.node[0] );
}

void _mpc_omp_start_openmp_thread(mpc_omp_mvp_t * mvp)
{
    assert(mvp);
    __mvp_add_saved_ctx(mvp);

    mpc_omp_thread_t * cur_thread =  __mvp_wakeup(mvp);
    assert(cur_thread->mvp);

    mpc_omp_tls = (void *) cur_thread;

#ifdef TLS_ALLOCATORS
    mpc_omp_init_allocators();
#endif

    __scatter_instance_post_init(cur_thread);
    _mpc_omp_in_order_scheduler(cur_thread);

    /* Must be set before barrier for thread safety*/
    volatile int * spin_status = ( mvp->spin_node ) ? &( mvp->spin_node->spin_status ) : &( mvp->spin_status );
    *spin_status = MPC_OMP_MVP_STATE_SLEEP;

    mpc_omp_barrier(ompt_sync_region_barrier_implicit_parallel);
    _mpc_omp_task_tree_deinit(cur_thread);

    mpc_omp_tls = (void *) mvp->threads->next;

    if (mpc_omp_tls)
    {
        mvp->threads = mpc_omp_tls;
        // mpc_omp_free( cur_thread );
    }

    __mvp_del_saved_ctx(mvp);

    if (mpc_omp_tls)
    {
        cur_thread = (mpc_omp_thread_t *) mpc_omp_tls;
        mvp->instance = cur_thread->instance;
    }
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpc_omp_slave_mvp_node( mpc_omp_mvp_t *mvp )
{
	mpc_omp_node_t *spin_node;
	assert( mvp );
	spin_node = mvp->spin_node;

	if ( spin_node )
	{
		volatile int *spin_status = &( spin_node->spin_status );

		while ( mvp->enable )
		{
#ifdef MPC_ENABLE_WORKSHARE
			if(mpc_conf_type_elem_get_as_int(mpc_conf_root_config_get("mpcframework.lowcomm.workshare.enablestealing")))
			if(mpc_conf_type_elem_get_as_int(mpc_conf_root_config_get("mpcframework.lowcomm.workshare.enablestealing")))
			  mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE,(void*) mpc_lowcomm_workshare_worker_steal, mvp->root->worker_ws) ;
      else
			  mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE, NULL, NULL ) ;
#else
			  mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE, NULL, NULL ) ;
#endif
            if( expect_false( !mvp->enable ))
                break;
            else {
#if MPC_OMP_TRANSFER_INFO_ON_NODES
			    spin_node->info = spin_node->father->info;
#endif /* MPC_OMP_TRANSFER_INFO_ON_NODES */
			    _mpc_spin_node_wakeup( spin_node );
			    _mpc_omp_start_openmp_thread( mvp );
            }
		}

        _mpc_omp_exit_node_signal( spin_node );
	}
	else
	{
		volatile int *spin_status = &( mvp->spin_status ) ;

		while ( mvp->enable )
		{
#ifdef MPC_ENABLE_WORKSHARE
			if(mpc_conf_type_elem_get_as_int(mpc_conf_root_config_get("mpcframework.lowcomm.workshare.enablestealing")))
				mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE, (void*) mpc_lowcomm_workshare_worker_steal, mvp->root->worker_ws) ;
			else
			  mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE, NULL, NULL ) ;
#else
			  mpc_thread_wait_for_value_and_poll( spin_status, MPC_OMP_MVP_STATE_AWAKE, NULL, NULL ) ;
#endif

            if( expect_false( !mvp->enable ))
                break;
            else
			    _mpc_omp_start_openmp_thread( mvp );
		}
	}
}
