#include "mpcomp_types.h"
#include "mpcomp_alloc.h"
#include "mpcomp_barrier.h"
#include "mpcomp_spinning_core.h"

#if defined( MPCOMP_OPENMP_3_0 )
#include "mpcomp_task_utils.h"
#endif /* defined( MPCOMP_OPENMP_3_0 ) */

#ifndef NDEBUG
#include "sctk_pm_json.h"
#endif /* !NDEBUG */

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
    int nthreads, i, first;
    int mvp_instance_rank;
    int* mvp_fathers;
    mpcomp_node_t* prev_node, *next_node;
    mpcomp_meta_tree_node_t* tree_array;

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

    return next_depth + 1;
}

static inline int
__mpcomp_scatter_compute_instance_tree_array_infos( mpcomp_instance_t* instance, const int expected_nb_mvps )
{
    mpcomp_node_t* root;
    int i, j, last_level_shape, tot_nnodes;
    int* num_nodes_per_depth, *shape, *num_children_per_depth, *first_rank_per_depth;

    sctk_assert( instance );
    root = instance->root;

    if( !root || expected_nb_mvps == 1 )
    {
        sctk_assert( expected_nb_mvps == 1 );
        int* singleton = (int*) mpcomp_alloc( sizeof( int ) );
        sctk_assert( singleton );
        singleton[0] = 1;
        first_rank_per_depth = (int*) mpcomp_alloc( sizeof( int ) );
        sctk_assert( first_rank_per_depth );
        first_rank_per_depth[0] = 0;
        instance->tree_base = singleton;
        instance->tree_cumulative = singleton;
        instance->tree_nb_nodes_per_depth = singleton;
        instance->tree_first_node_per_depth = first_rank_per_depth;
        return 1;
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
    num_nodes_per_depth = (int*) mpcomp_alloc( ( array_size ) * sizeof(int));
    sctk_assert( num_nodes_per_depth ); 
    memset( num_nodes_per_depth, 0, array_size * sizeof(int) );

    /* Instance tree_cumulative */
    num_children_per_depth = (int*) mpcomp_alloc( ( array_size + 1 ) * sizeof(int));
    sctk_assert( num_children_per_depth );           
    memset( num_children_per_depth, 0, ( array_size + 1 ) * sizeof(int) );

    /* Instance tree first level rank */
    first_rank_per_depth = (int*) mpcomp_alloc( array_size * sizeof(int));
    sctk_assert( num_children_per_depth );
    memset( num_children_per_depth, 0, array_size * sizeof(int) );
    
    /* Intermediate stage */
    tot_nnodes = 0;
    first_rank_per_depth[0] = 0;
    for( i = 0; i < array_size - 1; i++ )
    {
        shape[i] = root->tree_base[i];
        tot_nnodes += root->tree_nb_nodes_per_depth[i];
        num_nodes_per_depth[i] = root->tree_nb_nodes_per_depth[i];
        const int quot = last_level_shape / root->tree_base[i];
        const int rest = last_level_shape % root->tree_base[i];
        last_level_shape = ( rest ) ? ( quot + 1 ) : quot;
        first_rank_per_depth[i+1] = first_rank_per_depth[i] + shape[i];  
    }

    /* Last stage */
    shape[array_size-1] = last_level_shape;
    num_nodes_per_depth[array_size-1] = ( array_size > 2 ) ? num_nodes_per_depth[array_size-2] : 1;
    num_nodes_per_depth[array_size-1] *= shape[array_size-1];
    tot_nnodes += num_nodes_per_depth[array_size-1];

    /* tree_cumulative update */
    for( i = 0; i < array_size; i++ )
    {
        num_children_per_depth[i] = 1;
        for (j = i + 1; j < array_size; j++)
        {
            num_children_per_depth[i] *= shape[j];
            sctk_assert( num_children_per_depth[i] );
        }
    }

    /* Update instance */
    instance->tree_base = shape;
    instance->tree_nb_nodes_per_depth = num_nodes_per_depth;
    instance->tree_cumulative = num_children_per_depth;
    instance->tree_first_node_per_depth = first_rank_per_depth;

#if 0 // NDEBUG
        json_t* json_node_tree_base = json_array();
        json_t* json_node_tree_cumul = json_array();
        json_t* json_node_tree_lsize = json_array();

        for( i = 0; i < array_size; i++)
        {
            json_array_push( json_node_tree_base, json_int( instance->tree_base[i] ) );
            json_array_push( json_node_tree_cumul, json_int( instance->tree_cumulative[i] ) );
            json_array_push( json_node_tree_lsize, json_int( instance->tree_nb_nodes_per_depth[i] ) );
        }

        json_t* json_node_obj = json_object();
        json_object_set( json_node_obj, "tree_array_size", json_int( tot_nnodes) );
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

mpcomp_instance_t*
__mpcomp_scatter_instance_pre_init( mpcomp_thread_t* thread, const int num_mvps )
{

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
    instance->tree_array_size = __mpcomp_scatter_compute_instance_tree_array_infos( instance, num_mvps );

    //fprintf(stderr, "%s> tree_depth: %d tree_array: %d num_mvps : %d\n", __func__, instance->tree_depth, instance->tree_array_size, num_mvps);

    instance->tree_array = (mpcomp_generic_node_t*)mpcomp_alloc( sizeof( mpcomp_generic_node_t ) * instance->tree_array_size );
    sctk_assert( instance->tree_array );
    memset( instance->tree_array, 0, sizeof( mpcomp_generic_node_t ) * instance->tree_array_size );
 
    /* First Mvp entry in tree_array */
    const int instance_last_stage_size = instance->tree_cumulative[0];
    sctk_assert( !root || instance_last_stage_size <= root->tree_nb_nodes_per_depth[instance->tree_depth-1] );

    const int first_mvp = instance->tree_array_size - instance_last_stage_size;
    instance->mvps = &( instance->tree_array[first_mvp] );

    sctk_assert( instance_last_stage_size >= num_mvps );
 
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

    const int node_vdepth = node->depth - node->instance->root->depth + 1;
    const int num_vchildren = node->instance->tree_base[node_vdepth]; 
    const int num_children = node->nb_children;
    const int nthreads = node->num_threads;

    sctk_assert( num_children >= num_vchildren );
    
    const int node_first_mvp = node->mvp_first_id;
     
    sctk_assert( node->children.node );

    node->reduce_data = (void**) mpcomp_alloc( node->nb_children * 64 * sizeof( void* ));
    node->isArrived = (int *) mpcomp_alloc( node->nb_children * 64 * sizeof( int ) );
    for ( i = 0; i < node->nb_children * 64; i+=64 ) {
      node->reduce_data[i] = NULL;
      node->isArrived[i] = 0;
    }
    /** Instance id : First id next step + cur_node * tree_base + i */
    if( nthreads > num_vchildren )
    {
        const int min_nthreads = nthreads / num_children;
        const int ext_nthreads = nthreads % num_children;

        node->barrier_num_threads = num_vchildren;
        
        for( i = 0; i < num_vchildren; i++ )
        {

            child_node = node->children.node[i];
            sctk_assert( child_node );
            child_node->already_init = 0;
            if( !node->instance->buffered )
            {
                child_node->num_threads = min_nthreads + ( ( i < ext_nthreads ) ? 1 : 0 );
                child_node->mvp_first_id = min_nthreads * i + ( ( i < ext_nthreads ) ? i : ext_nthreads );
                child_node->mvp_first_id += node_first_mvp;
                child_node->instance = node->instance;
                __mpcomp_instance_tree_array_node_init( node, child_node, i );
            }
            child_node->spin_status = MPCOMP_MVP_STATE_AWAKE;
        }
    }
    else
    {
        int cur_node;
        mpcomp_mvp_t* mvp;
        const int min_shift = num_children / nthreads;
        const int ext_shift = num_children % nthreads;

        node->barrier_num_threads = nthreads;
        cur_node = 0;

        for( i = 0; i < nthreads; i++ )
        {
            mpcomp_thread_t* next;
            child_node = node->children.node[cur_node];
            sctk_assert( child_node );
            mvp = child_node->mvp;
            sctk_assert( mvp );
            child_node->already_init = 1;
            if( !node->instance->buffered )
            {
                child_node->num_threads = 1; 
                child_node->instance = node->instance;
                child_node->mvp_first_id = node_first_mvp + i;
                /* Set MVP infos */
                sctk_assert( mvp->threads );        
                next = mvp->threads->next; 
                memset( mvp->threads, 0, sizeof( mpcomp_thread_t ) );
                mvp->threads->mvp = mvp;
                mvp->threads->instance = child_node->instance;
                mvp->threads->root = child_node;
                mvp->threads->next = next;
                mvp->threads->father = node->instance->thread_ancestor;
                mvp->threads->father_node = node;
                mvp->threads->rank = node->mvp_first_id + i;
                mvp->instance = node->instance;
            int j; 
            for (j = 0; j < MPCOMP_MAX_ALIVE_FOR_DYN + 1; j++) 
	            OPA_store_int(&(mvp->threads->for_dyn_remain[j].i), -1);
                __mpcomp_instance_tree_array_mvp_init( node, mvp, i );
            }
            /* WakeUp NODE */
            child_node->spin_status = MPCOMP_MVP_STATE_AWAKE;
            cur_node += min_shift + ( ( i < ext_shift ) ? 1 : 0 ); 
        }
    }

    return node->children.node[0];  
}

static mpcomp_mvp_t*
__mpcomp_scatter_wakeup_final_mvp( mpcomp_node_t* node )
{
    int i, cur_mvp;

    sctk_assert( node );
    sctk_assert( node->child_type == MPCOMP_CHILDREN_LEAF ); 

        sctk_assert( node->instance->root->depth <= node->depth );
        const int node_vdepth = node->depth - node->instance->root->depth +1;
        const int num_vchildren = node->instance->tree_base[node_vdepth];               
        sctk_assert( node->num_threads <= num_vchildren );

        const int nthreads = node->num_threads;
        const int num_children = node->nb_children;
        sctk_assert( nthreads <= num_children );

        const int min_shift = num_children / nthreads; 
        const int ext_shift = num_children % nthreads;

        node->barrier_num_threads = node->num_threads;
        cur_mvp = 0; 
        node->reduce_data = (void**) mpcomp_alloc( node->nb_children * 64 * sizeof( void* ));
        node->isArrived = (int *) mpcomp_alloc( node->nb_children * 64 * sizeof( int ) );
        for ( i = 0; i < node->nb_children * 64; i+=64 ) {
		      node->reduce_data[i] = NULL;
		      node->isArrived[i] = 0;
        }

    for( i = 0; i < nthreads; i++ )
    {
        mpcomp_thread_t* next;
        mpcomp_mvp_t* mvp = node->children.leaf[cur_mvp]; 
        if( !node->instance->buffered )
        {
            mvp->instance = node->instance;
            /* Set MVP instance rank in thread structure */ 
            sctk_assert( mvp->threads );        
            next = mvp->threads->next;
            memset( mvp->threads, 0, sizeof( mpcomp_thread_t ) );
            mvp->threads->mvp = mvp;
            mvp->threads->instance = node->instance;
            mvp->threads->root = NULL;
            mvp->threads->next = next;
            mvp->threads->father = node->instance->thread_ancestor;
            mvp->threads->father_node = node;
            mvp->threads->rank = node->mvp_first_id + i;
            int j; 
            for (j = 0; j < MPCOMP_MAX_ALIVE_FOR_DYN + 1; j++) 
	            OPA_store_int(&(mvp->threads->for_dyn_remain[j].i), -1);
            __mpcomp_instance_tree_array_mvp_init( node, mvp, i );
        }
        /* WakeUp MVP */
        mvp->spin_status = MPCOMP_MVP_STATE_AWAKE;
        cur_mvp += min_shift + ( ( i < ext_shift ) ? 1 : 0 ); 
    } 

    return node->children.leaf[0];
}


static mpcomp_mvp_t*
__mpcomp_scatter_wakeup_final_node( mpcomp_node_t* node )
{
    assert( node );
    assert( node->mvp ); 
    node->already_init = 0;
    return node->mvp;
}

mpcomp_mvp_t*
__mpcomp_scatter_wakeup( mpcomp_node_t* node )
{
    mpcomp_node_t* cur_node;

    sctk_assert( node->instance );
    sctk_assert( node->instance->root );

    const int target_depth = node->instance->tree_depth + node->instance->root->depth - 1;

    /* Node Wake-Up */
    cur_node = node;

    while( cur_node->depth < target_depth )
    {
        if( cur_node->already_init || cur_node->child_type != MPCOMP_CHILDREN_NODE )
            break;
        
        cur_node = __mpcomp_scatter_wakeup_intermediate_node( cur_node );
    }    
    
    if( cur_node->depth == target_depth || cur_node->already_init )
    {
        return __mpcomp_scatter_wakeup_final_node( cur_node );
    }

    return __mpcomp_scatter_wakeup_final_mvp( cur_node );
}

void __mpcomp_scatter_instance_post_init( mpcomp_thread_t* thread )
{
    sctk_assert( thread );
    sctk_assert( thread->mvp );

    mpcomp_mvp_t* mvp = thread->mvp;
    int j; 
    for (j = 0; j < MPCOMP_MAX_ALIVE_FOR_DYN + 1; j++) 
	    OPA_store_int(&(mvp->threads->for_dyn_remain[j].i), -1);

	if(!thread->task_infos.reusable_tasks)
    thread->task_infos.reusable_tasks = (mpcomp_task_t**) mpcomp_alloc(MPCOMP_NB_REUSABLE_TASKS*sizeof(mpcomp_task_t*));

    if( ! thread->mvp->instance->buffered )
        __mpcomp_internal_full_barrier( thread->mvp );

#if 0  /* Check victim list for each thread */
    int  i, total, current, nbList;
    int* victim_list;
    const int nb_victims = __mpcomp_task_utils_extract_victims_list( &victim_list, &nbList, 0 );     
    char string_array[256];

   for( i = 0, total = 0; i < nb_victims; i++ )
   {
      current = snprintf( string_array+total, 256-total, " %d", victim_list[i] );
      total += current;
   }
   string_array[total] = '\0';

   const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(mvp, 0);
   fprintf(stderr, "#%d - Me : %d -- Stealing list =%s nbList : %d\n", thread->rank, node_rank, string_array, nbList );
    __mpcomp_internal_full_barrier( thread->mvp ); 
#endif /* Check victim list for each thread */

    thread->mvp->instance->buffered = 1;
#if 0
    if( thread->rank == 0 )
    {
    instance = thread->mvp->instance;
    for( i = 1; i < instance->tree_array_size; i++ )
    {
        switch( instance->tree_array[i].type )
        {
            case MPCOMP_CHILDREN_NODE:
                fprintf(stderr, ":: %s :: node> %d -- type> %s\n", __func__, instance->tree_array[i].ptr.node->global_rank, "MPCOMP_CHILDREN_NODE" );
                break;
            case MPCOMP_CHILDREN_LEAF:
                fprintf(stderr, ":: %s :: node> %d -- type> %s\n", __func__, instance->tree_array[i].ptr.mvp->global_rank, "MPCOMP_CHILDREN_LEAF" );
                break;
            case MPCOMP_CHILDREN_NULL:
                fprintf(stderr, ":: %s :: empty node -> %d\n", __func__, i ); 
                break;
            default:
                sctk_assert(0);
        }
    }
    }
#endif
//    MPCOMP_TASK_TEAM_CMPL_INIT(thread->instance->team);
}
