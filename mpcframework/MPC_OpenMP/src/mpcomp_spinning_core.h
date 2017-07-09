#ifndef __MPCOMP_SPINNING_CORE_H__
#define __MPCOMP_SPINNING_CORE_H__

#include "mpcomp_types.h"

static inline void 
__mpcomp_instance_tree_array_node_init(struct mpcomp_node_s* parent, struct mpcomp_node_s* child, const int index)
{
    int i;
    struct mpcomp_generic_node_s* meta_node;

    const int vdepth = parent->depth - parent->instance->root->depth;
    const int tree_base_val = parent->instance->tree_base[vdepth];
    const int next_tree_base_val = parent->instance->tree_base[vdepth+1];

    const int global_rank = parent->instance_global_rank + index; 
    child->instance_stage_size = parent->instance_stage_size * tree_base_val;
    child->instance_stage_first_rank = parent->instance_stage_first_rank + parent->instance_stage_size;
    const int local_rank = global_rank - parent->instance_stage_first_rank;
    child->instance_global_rank = child->instance_stage_first_rank + local_rank * next_tree_base_val;
    
    meta_node = &( parent->instance->tree_array[global_rank] );
    sctk_assert( meta_node );
    meta_node->ptr.node = child;    
    meta_node->type = MPCOMP_CHILDREN_NODE;

    child->tree_array_rank = global_rank; 

    child->tree_array_ancestor_path = (int*) mpcomp_alloc( (vdepth + 1) * sizeof( int ) );
    sctk_assert( child->tree_array_ancestor_path );
    memset( child->tree_array_ancestor_path, 0, ( vdepth + 1) * sizeof( int ) );
    
    for( i = 0; i < vdepth; i++ )
        child->tree_array_ancestor_path[i] = parent->tree_array_ancestor_path[i];

    child->tree_array_ancestor_path[vdepth] = parent->tree_array_rank;

#if defined( MPCOMP_OPENMP_3_0 ) 
    __mpcomp_task_node_infos_init( parent, child );
#endif /* defined( MPCOMP_OPENMP_3_0 ) */
    
}

static inline void 
__mpcomp_instance_tree_array_mvp_init( struct mpcomp_node_s* parent, struct mpcomp_mvp_s* mvp, const int index )
{
    int i;
    struct mpcomp_generic_node_s* meta_node;

    const int vdepth = parent->depth - parent->instance->root->depth; 

    const int global_rank = parent->instance_global_rank + index;
    meta_node = &( parent->instance->tree_array[global_rank] );
    sctk_assert( meta_node );
    meta_node->ptr.mvp = mvp; 
    meta_node->type = MPCOMP_CHILDREN_LEAF;
    sctk_assert( mvp->threads );
    sctk_assert( mvp->threads );

    mvp->threads->tree_array_rank = global_rank;

    mvp->threads->tree_array_ancestor_path = (int*) mpcomp_alloc( ( vdepth + 1) * sizeof( int ) );
    sctk_assert( mvp->threads->tree_array_ancestor_path );
    memset( mvp->threads->tree_array_ancestor_path, 0,  ( vdepth + 1) * sizeof( int ) );
    
    for( i = 0; i < vdepth; i++ )
       mvp->threads->tree_array_ancestor_path[i] = parent->tree_array_ancestor_path[i];

    mvp->threads->tree_array_ancestor_path[vdepth] = parent->tree_array_rank;

#if defined( MPCOMP_OPENMP_3_0 ) 
    __mpcomp_task_mvp_infos_init( parent, mvp );
#endif /* defined( MPCOMP_OPENMP_3_0 ) */
}



static inline mpcomp_thread_t* 
__mpcomp_spinning_push_mvp_thread( mpcomp_mvp_t* mvp, mpcomp_thread_t* new_thread )
{
    sctk_assert( mvp ); 
    sctk_assert( new_thread ); 

    new_thread->father = mvp->threads;
    mvp->threads = new_thread;
   
    return new_thread; 
}

static inline mpcomp_thread_t*
__mpcomp_spinning_pop_mvp_thread( mpcomp_mvp_t* mvp )
{
    mpcomp_thread_t* cur_thread;

    sctk_assert( mvp );
    sctk_assert( mvp->threads );

    cur_thread = mvp->threads;    
    mvp->threads = cur_thread->father; 

    return cur_thread;
}

static inline mpcomp_thread_t*
__mpcomp_spinning_get_thread_ancestor( mpcomp_thread_t* thread, const int depth )
{
    int i;
    mpcomp_thread_t* ancestor;
   
    ancestor = thread; 
    
    for( i = 0; i < depth; i++ )
    {
        if( ancestor == NULL )
            break;

        ancestor = ancestor->father;
    } 

    return ancestor;
}

static inline mpcomp_thread_t*
__mpcomp_spinning_get_mvp_thread_ancestor( mpcomp_mvp_t* mvp, const int depth )
{
    mpcomp_thread_t *mvp_thread, *ancestor;
    sctk_assert( mvp );
    mvp_thread = mvp->threads;
    ancestor = __mpcomp_spinning_get_thread_ancestor( mvp_thread, depth ); 
    return ancestor;
} 

static inline mpcomp_node_t*
__mpcomp_spinning_get_thread_root_node( mpcomp_thread_t* thread )
{
    sctk_assert( thread );
    return thread->root;
}

static inline int
__mpcomp_spining_get_instance_max_depth( mpcomp_instance_t* instance )
{
    mpcomp_node_t* root = instance->root;
    assert( root ); //TODO instance root can be NULL when leaf create new parallel region
    return root->depth + root->tree_depth -1;
}

static inline char* 
__mpcomp_spinning_get_debug_thread_infos( mpcomp_thread_t* thread )
{
    return NULL; 
}

static inline mpcomp_node_t*
__mpcomp_spinning_get_mvp_father_node( mpcomp_mvp_t* mvp, mpcomp_instance_t* instance )
{
    unsigned int* mvp_father_array;
    mpcomp_meta_tree_node_t* tree_array_node, *tree_array; 

    sctk_assert( mvp );
    sctk_assert( instance );

    if( !( instance->root ) ) return  mvp->father;

    const int max_depth = mvp->depth;
    const int father_depth = instance->root->depth + instance->tree_depth -1;
    const int mvp_ancestor_node = max_depth - father_depth;

    if( !mvp_ancestor_node ) return mvp->father;

    sctk_assert( mvp->father );
    tree_array = mvp->father->tree_array;
    sctk_assert( tree_array );

    mvp_father_array = tree_array[mvp->global_rank].fathers_array;
    sctk_assert( mvp_ancestor_node < tree_array[mvp->global_rank].fathers_array_size );

    tree_array_node = &( tree_array[mvp_father_array[father_depth-1]] );
    return ( mpcomp_node_t* ) tree_array_node->user_pointer;
}

static inline int
__mpcomp_spinning_node_compute_rank( mpcomp_node_t* node, const int num_threads, int rank, int *first ) 
{
    sctk_assert( node );
    sctk_assert( num_threads > 0 );
    sctk_assert( first );

    const int quot = num_threads / node->nb_children;
    const int rest = num_threads % node->nb_children;
    const int min = (num_threads > node->nb_children) ? node->nb_children : num_threads; 

    *first = -1;
    if( rank >= min ) return 0;
    
    *first = ( rank < rest ) ? ((quot+1) * rank) : (quot * rank + rest);
    return ( rank < rest ) ? quot+1 : quot; 
}

static inline int
__mpcomp_spinning_leaf_compute_rank( mpcomp_node_t* node, const int num_threads, const int first_rank, const int rank )
{
    int i;
    sctk_assert( node );
    sctk_assert( num_threads > 0 );

    const int quot = node->nb_children / num_threads;
    for( i = 0; i < node->nb_children; i++ )
    {
        if( i * quot + first_rank == rank )
            break;
    }
    return i; 
}

#endif /* __MPCOMP_SPINNING_CORE_H__ */
