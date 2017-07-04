#include "mpcomp_types.h"
#include "mpcomp_alloc.h"

static inline int
__mpcomp_scatter_compute_node_num_threads( mpcomp_node_t* node, const int num_threads, int rank, int *first ) 
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
__mpcomp_scatter_compute_mvp_index( mpcomp_node_t* node, const int num_threads, const int first_rank, const int rank )
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


int __mpcomp_scatter_compute_global_rank_from_instance_rank( mpcomp_instance_t* instance, const int instance_rank )
{
    mpcomp_mvp_t* mvp;
    mpcomp_node_t* start_node;
    int i, j, num_threads, local_rank;

    /* Init Search Parameter */
    local_rank = instance_rank; 
    start_node = instance->root;
    num_threads = instance->nb_mvps;

    /** Intermediate level **/
    for( i = 1; i < instance->tree_depth -1; i++ )
    {
        const int quot = num_threads / start_node->nb_children;
        const int rest = num_threads % start_node->nb_children;
        
        for( j = 1; j < start_node->nb_children; j++ )
        {
            const int max = ( j < rest ) ? ( quot+1 ) * j : ( quot * j + rest );
            if( local_rank < max )
                break; 
        } 
        
        const int index = j - 1;
        sctk_assert( start_node->child_type == MPCOMP_CHILDREN_NODE );
        start_node = start_node->children.node[index];
        num_threads = ( index < rest ) ? ( quot+1 ) : quot;  
        local_rank -= ( index < rest ) ? ( quot+1 ) * index : ( quot * index + rest );  
    } 

    /** Last level **/
    {
        const int quot = start_node->nb_children / num_threads;
        const int rest = start_node->nb_children % num_threads;

        for( i = 1; i < num_threads; i++ )
        {
            const int max = ( i < rest ) ? ( quot+1 ) * i : ( quot * i + rest );
            if( local_rank < max )
                break; 
        }

        const int index = i - 1;
        if( start_node->child_type == MPCOMP_CHILDREN_NODE  )
        {
            mvp = start_node->children.node[index]->mvp;        
        }
        else
        {
            mvp = start_node->children.leaf[index];
        }
    }

    return mvp->global_rank;
}

int __mpcomp_scatter_compute_instance_rank_from_mvp( mpcomp_instance_t* instance, mpcomp_mvp_t* mvp )
{
    int nthreads, max, i, first;
    int* mvp_fathers, mvp_instance_rank;
    mpcomp_node_t* prev_node, *next_node;
    mpcomp_meta_tree_node_t* tree_array_node, *tree_array;

    sctk_assert( mvp );
    sctk_assert( instance );

    if( !( instance->root ) ) return 0;

    sctk_assert( mvp->father );
    tree_array = mvp->father->tree_array;
    sctk_assert( tree_array );

    mvp_fathers = tree_array[mvp->global_rank].fathers_array;
    sctk_assert( mvp_fathers );

    const int start_depth = instance->root->depth;

    mvp_instance_rank = 0;
    prev_node = instance->root;
    nthreads = instance->nb_mvps;

    /** Intermediate level **/
    for( i = 1; i < instance->tree_depth - 1; i++ )
    {
        const int depth = start_depth + i;
        next_node = (mpcomp_node_t*) tree_array[mvp_fathers[depth]].user_pointer;
        nthreads = __mpcomp_scatter_compute_node_num_threads( prev_node, nthreads, next_node->local_rank, &first );
        mvp_instance_rank += first;
        prev_node = next_node;
    }

    /** Last level **/
    {
            
    }
   // const int first_local_mvp = mvp->global_rank - mvp->local_rank;
        // += __mpcomp_spinning_leaf_compute_rank( prev_node, nthreads, first_local_mvp, mvp->global_rank);
    return mvp_instance_rank;
}

static inline int
__mpcomp_scatter_compute_instance_tree_depth( mpcomp_node_t* node, const int expected_nb_mvps )
{
    int next_depth, num_nodes;

    sctk_assert( expected_nb_mvps > 0 );

    /* No more nested parallelism */
    if( node == NULL || expected_nb_mvps == 1 )
        return 1;

    for( next_depth = 0; next_depth < node->tree_depth; next_depth++ ) 
    {
        num_nodes = node->tree_nb_nodes_per_depth[next_depth];
        if( num_nodes >= expected_nb_mvps ) break;
    }

    return next_depth;
}

static inline void
__mpcomp_scatter_compute_instance_tree_array_infos( mpcomp_instance_t* instance, const int expected_nb_mvps )
{
    mpcomp_node_t* root;
    int i, last_level_shape, cumul;
    int* num_nodes_per_depth, *shape, *num_children_per_depth;

    sctk_assert( instance );
    root = instance->root;
    
    sctk_assert( instance->tree_depth > 0 );

    if( instance->tree_depth == 1 )
    {
        int* singleton = (int*) mpcomp_alloc( sizeof( int ) );
        singleton[0] = 1;
        instance->tree_base = singleton;
        instance->tree_cumulative = singleton;
        instance->tree_nb_nodes_per_depth = singleton;
        return;
    }

    sctk_assert( root );
    sctk_assert( root->tree_base );

    const int array_size = instance->tree_depth;
    last_level_shape = expected_nb_mvps; 

    /* Instance tree_base */
    shape = (int*) mpcomp_alloc( array_size * sizeof(int));
    sctk_assert( shape );
    memset( shape, 0, array_size * sizeof(int) );

    /* Instance tree_nb_nodes_per_depth */
    num_nodes_per_depth = (int*) mpcomp_alloc( array_size * sizeof(int));
    sctk_assert( num_nodes_per_depth ); 
    memset( num_nodes_per_depth, 0, array_size * sizeof(int) );

    /* Instance tree_cumulative */
    num_children_per_depth = (int*) mpcomp_alloc( array_size * sizeof(int));
    sctk_assert( num_children_per_depth );           
    memset( num_children_per_depth, 0, array_size * sizeof(int) );

    /* Intermediate stage */
    for( i = 0; i < array_size - 1; i++ )
    {
        shape[i] = root->tree_base[i+1];
        num_nodes_per_depth[i] = root->tree_nb_nodes_per_depth[i+1];
        const int quot = last_level_shape / root->tree_base[i+1];
        const int rest = last_level_shape % root->tree_base[i+1];
        last_level_shape = ( rest ) ? ( quot + 1 ) : quot;
    }

    /* Last stage */
    shape[array_size-1] = last_level_shape;
    num_nodes_per_depth[array_size-1] = num_nodes_per_depth[array_size-2];
    num_nodes_per_depth[array_size-1] *= shape[array_size-1];

    /* tree_cumulative update */
    num_children_per_depth[array_size-1] = 1;
    for( i = array_size-2; i >= 0; i-- )
    {
        num_children_per_depth[i] = num_children_per_depth[i+1];
        num_children_per_depth[i] *= shape[i+1];
    }

    /* Update instance */
    instance->tree_base = shape;
    instance->tree_nb_nodes_per_depth = num_nodes_per_depth;
    instance->tree_cumulative = num_children_per_depth;
}

mpcomp_instance_t*
__mpcomp_scatter_instance_pre_init( mpcomp_thread_t* thread, const int num_mvps )
{
    int shift, i;
    mpcomp_node_t* root;
    mpcomp_instance_t* instance;

    sctk_assert( thread );
    sctk_assert( num_mvps > 0 );

    root = thread->root;

    instance = ( mpcomp_instance_t* ) mpcomp_alloc( sizeof( mpcomp_instance_t ) );
    sctk_assert( instance );
    memset( instance, 0, sizeof( mpcomp_instance_t ) );

    instance->root = root;
    instance->tree_depth = __mpcomp_scatter_compute_instance_tree_depth( root, num_mvps );
    __mpcomp_scatter_compute_instance_tree_array_infos( instance, num_mvps );

    const int instance_last_stage_size = instance->tree_cumulative[0];
    sctk_assert( instance_last_stage_size <= root->tree_nb_nodes_per_depth[instance->tree_depth] );

    instance->mvps = mpcomp_alloc( sizeof( mpcomp_mvp_t*) * instance_last_stage_size );
    sctk_assert( instance->mvps );
    /* Do not change memset -> We assume that extra MVPS are NULL */
    memset( instance->mvps, 0, sizeof( mpcomp_mvp_t*) * instance_last_stage_size );
   
    sctk_assert( thread->root );

    if( thread->root )
    {
        thread->root->num_threads = num_mvps; 
        thread->root->mvp_first_id = 0;
        thread->root->instance = instance;
    }
    return instance; 
}

static mpcomp_node_t*
__mpcomp_scatter_wakeup_intermediate_node( mpcomp_node_t* node )
{
    int i;
    mpcomp_node_t* child_node;

    sctk_assert( node->instance );
    sctk_assert( node->child_type == MPCOMP_CHILDREN_NODE );
    sctk_assert( node->instance->root->depth <= node->depth );

    const int node_vdepth = node->depth - node->instance->root->depth;
    const int num_vchildren = node->instance->tree_base[node_vdepth]; 
    const int num_children = node->nb_children;
    const int nthreads = node->num_threads;

    sctk_assert( num_children >= num_vchildren );
    
    const int node_first_mvp = node->mvp_first_id;

    sctk_assert( node->children.node );

    if( nthreads > num_vchildren )
    {
        const int min_nthreads = nthreads / num_children;
        const int ext_nthreads = nthreads % num_children;

        node->barrier_num_threads = num_vchildren;
        
        for( i = 0; i < num_vchildren; i++ )
        {
            child_node = node->children.node[i];
            sctk_assert( child_node );
            child_node->num_threads = min_nthreads + ( ( i < ext_nthreads ) ? 1 : 0 );
            child_node->mvp_first_id = min_nthreads * i + ( ( i < ext_nthreads ) ? i : ext_nthreads );
            child_node->mvp_first_id += node_first_mvp;
            child_node->instance = node->instance;
            child_node->spin_status = MPCOMP_MVP_STATE_AWAKE;
        }
    }
    else
    {
        int cur_node, cur_vnode;
        mpcomp_mvp_t* mvp;

        const int min_shift = num_children / nthreads;
        const int ext_shift = num_children % nthreads;

        const int min_vshift = num_vchildren / nthreads;
        const int ext_vshift = num_vchildren % nthreads;

        node->barrier_num_threads = nthreads;
        cur_node = 0;
        cur_vnode = 0;

        for( i = 0; i < nthreads; i++ )
        {
            child_node = node->children.node[cur_node];
            sctk_assert( child_node );
            mvp = child_node->mvp;
            sctk_assert( mvp );
            child_node->num_threads = 1; 
            child_node->instance = node->instance;
            child_node->mvp_first_id = node_first_mvp + i;
            /* Set MVP infos */
            sctk_assert( mvp->threads );        
            mvp->threads->father_node = node;
            mvp->threads->instance_ghost_rank = node->mvp_first_id + cur_vnode;
            mvp->threads->rank = node->mvp_first_id + i;
            mvp->instance = node->instance;
            /* WakeUp NODE */
            child_node->spin_status = MPCOMP_MVP_STATE_AWAKE;
            cur_node += min_shift + ( ( i < ext_shift ) ? 1 : 0 ); 
            cur_vnode += min_vshift + ( ( i < ext_vshift ) ? 1 : 0 ); 
        }
    }

    return node->children.node[0];  
}

static mpcomp_mvp_t*
__mpcomp_scatter_wakeup_final_mvp( mpcomp_node_t* node )
{
    int i, cur_mvp, cur_vmvp; 

    sctk_assert( node );
    sctk_assert( node->child_type == MPCOMP_CHILDREN_LEAF ); 

    sctk_assert( node->instance->root->depth <= node->depth );
    const int node_vdepth = node->depth - node->instance->root->depth;
    const int num_vchildren = node->instance->tree_base[node_vdepth];               
    sctk_assert( node->num_threads <= num_vchildren );

    const int nthreads = node->num_threads;
    const int num_children = node->nb_children;
    sctk_assert( nthreads <= num_children );

    const int min_shift = num_children / nthreads; 
    const int ext_shift = num_children % nthreads;

    const int min_vshift = num_vchildren / nthreads;
    const int ext_vshift = num_vchildren % nthreads;

    node->barrier_num_threads = node->num_threads;
    cur_mvp = 0; 
    cur_vmvp = 0; 

    for( i = 0; i < nthreads; i++ )
    {
        mpcomp_mvp_t* mvp = node->children.leaf[cur_mvp]; 
        mvp->instance = node->instance;
        /* Set MVP instance rank in thread structure */ 
        sctk_assert( mvp->threads );        
        mvp->threads->father_node = node;
        mvp->threads->instance_ghost_rank = node->mvp_first_id + cur_vmvp;
        mvp->threads->rank = node->mvp_first_id + i;
        /* WakeUp MVP */
        mvp->spin_status = MPCOMP_MVP_STATE_AWAKE;
        cur_mvp += min_shift + ( ( i < ext_shift ) ? 1 : 0 ); 
        cur_vmvp += min_vshift + ( ( i < ext_vshift ) ? 1 : 0 ); 
    } 

    return node->children.leaf[0];
}


static mpcomp_mvp_t*
__mpcomp_scatter_wakeup_final_node( mpcomp_node_t* node )
{
    assert( node );
    assert( node->mvp ); 
    return node->mvp;
}

mpcomp_mvp_t*
__mpcomp_scatter_wakeup( mpcomp_node_t* node )
{
    mpcomp_node_t* cur_node;

    sctk_assert( node->instance );
    sctk_assert( node->instance->root );

    const int target_depth = node->instance->tree_depth + node->instance->root->depth;

    /* Node Wake-Up */
    cur_node = node;
    while( cur_node->depth < target_depth )
    {
        if( cur_node->child_type != MPCOMP_CHILDREN_NODE )
            break;
        cur_node = __mpcomp_scatter_wakeup_intermediate_node( cur_node );
    }    

    if( cur_node->depth == target_depth )
        return __mpcomp_scatter_wakeup_final_node( cur_node );

    return __mpcomp_scatter_wakeup_final_mvp( cur_node );
}

void __mpcomp_scatter_instance_post_init( mpcomp_thread_t* thread )
{
    sctk_assert( thread );
    sctk_assert( thread->mvp );
    mpcomp_mvp_t* mvp = thread->mvp;
    fprintf(stderr, ":: %s :: thread> %d mvp> %d mvp->father> %p\n", __func__, mvp->rank,  mvp->global_rank,  mvp->father );
    __mpcomp_internal_full_barrier( thread->mvp );
}
