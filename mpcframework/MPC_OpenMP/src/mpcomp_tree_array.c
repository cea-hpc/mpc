#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "mpcthread.h"
#include "sctk_atomics.h"
#include "mpcomp_types.h"
#include "mpcomp_tree_array.h"
#include "mpcomp_tree_structs.h"

/*
#ifndef MPCOMP_AFFINITY_NB
#define MPCOMP_AFFINITY_COMPACT     0 
#define MPCOMP_AFFINITY_SCATTER     1 
#define MPCOMP_AFFINITY_BALANCED    2
#define MPCOMP_AFFINITY_NB          3 
#endif
*/

#define MPCOMP_TREE_ARRAY_COMPUTE_WITH_LOG 1

#if MPCOMP_TREE_ARRAY_COMPUTE_WITH_LOG
#include "sctk_pm_json.h"
static FILE* export_tree_array_info_json_stream;
static sctk_spinlock_t export_tree_array_info_json_lock; 

static void
__mpcomp_tree_export_tree_array_info_lock( void )
{
    sctk_spinlock_lock( &export_tree_array_info_json_lock ); 
}

static void
__mpcomp_tree_export_tree_array_info_unlock( void )
{
    sctk_spinlock_unlock( &export_tree_array_info_json_lock ); 
}

static void 
__mpcomp_tree_export_tree_array_info_json_open( char* filename )
{
    export_tree_array_info_json_stream = fopen( filename, "w" );
    assert( export_tree_array_info_json_stream != NULL );
}

static void 
__mpcomp_tree_export_tree_array_info_json_close( void )
{
    assert( export_tree_array_info_json_stream != NULL );
    close( export_tree_array_info_json_stream );
}
#endif /* MPCOMP_TREE_ARRAY_COMPUTE_WITH_LOG */

static int 
__mpcomp_tree_array_get_parent_nodes_num_per_depth( const int* shape, const int depth )
{
    int i, count, sum;        
    for( count = 1, sum = 1, i = 0; i < depth; i++ )
    {
        count *= shape[i];
        sum += count;
    }
    return sum; 
}

static int 
__mpcomp_tree_array_get_children_nodes_num_per_depth( const int* shape, const int max_depth, const int depth )
{
    int i, j, count;        
    for( count = 1, i = depth; i < max_depth; i++ )
        count *= shape[i];
    return count; 
}

static int 
__mpcomp_tree_array_get_stage_nodes_num_per_depth( const int* shape, const int max_depth, const int depth )
{
    int i, count;

    for( count = 1, i = 1; i < depth; i++ )
        count *= shape[i-1];
    return count;
}

static int*
__mpcomp_tree_array_get_stage_nodes_num_depth_array( const int* shape, const int max_depth, const int depth )
{
    int i;
    int *count;

    count = (int*) malloc( sizeof(int) * depth );
    assert( count );
    memset( count, 0, sizeof(int) * depth );

    for(count[0] = 1, i = 1; i < depth; i++ )
        count[i] = count[i-1] * shape[i-1]; 

    return count;
}


static int 
__mpcomp_tree_array_get_node_depth( const int* shape, const int max_depth, const int rank )
{
    int count, sum, i;
    for( count = 1, sum = 1, i = 0; i < max_depth; i++ )
    {
        if( rank < sum ) break;
        count *= shape[i];
        sum += count;
    } 
    return i;
}


static int
__mpcomp_tree_array_convert_global_to_stage( const int* shape, const int max_depth, const int rank )
{
    assert( shape );
    assert( rank >= 0 );
    assert( max_depth >= 0 );
    
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );

    if( !rank ) return 0;

    const int prev_nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth(  shape, depth-1 );
    const int stage_rank = rank - prev_nodes_num;

    assert( rank >= prev_nodes_num );
    return stage_rank;
}

static int
__mpcomp_tree_array_convert_stage_to_global( const int* shape, const int depth, const int rank )
{
    assert( shape );
    assert( rank >= 0 );
    assert( depth >= 0 );

    if( !depth ) return 0;
    const int prev_nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth(  shape, depth-1 );
    return prev_nodes_num + rank;
}

static int
__mpcomp_tree_array_convert_stage_to_local( const int* shape, const int depth, const int rank )
{
    assert( shape );
    assert( rank >= 0 );
    assert( depth >= 0 );

    if( !rank ) return 0;
    return rank % shape[depth-1];
}

static int
__mpcomp_tree_array_convert_global_to_local( const int* shape, const int max_depth, const int rank )
{
    assert( shape );
    assert( rank >= 0 );
    assert( max_depth >= 0 );

    if( !rank ) return 0;
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
    const int stage_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, rank );
    return __mpcomp_tree_array_convert_stage_to_local( shape, depth, stage_rank );
}

static int* 
__mpcomp_tree_array_get_tree_rank( const int* shape, const int max_depth, const int rank )
{
    int i, current_rank;
    int *tree_array;

    assert( shape );
    assert( rank >= 0 );
    assert( max_depth >= 0 );
   
    tree_array = (int*) malloc(sizeof(int) * max_depth);
    assert( tree_array );
    memset( tree_array, 0, sizeof(int) * max_depth ); 
 
    for( current_rank = rank, i = max_depth - 1; i >= 0; i-- )
    {
        const int stage_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, current_rank );
        tree_array[i] = stage_rank % shape[ i ];
        current_rank = __mpcomp_tree_array_convert_stage_to_global( shape, max_depth, stage_rank / shape[i] );
    } 
     
    return tree_array;
}

static int* __mpcomp_tree_array_get_tree_cumulative( int* shape, int max_depth )
{
    int i, j;
    int* tree_cumulative;

    tree_cumulative = (int*) malloc( sizeof(int) * max_depth );
    assert( tree_cumulative );
    memeset( tree_cumulative, 0, sizeof(int) * max_depth );
    
    for (i = 0; i < max_depth - 1; i++) 
    {
        tree_cumulative[i] = 1;
        for( j = 0; j < max_depth; j++ )
            tree_cumulative[i] *= shape[j];
    }
    tree_cumulative[max_depth-1] = 1;
    return tree_cumulative;
}

static char* 
__mpcomp_tree_array_convert_array_to_string( const int* tab, const int size )
{
    int i, ret, used;
    char *tmp, *string;
    
    if( size == 0 ) return NULL;
    
    string = (char*) malloc( sizeof(char) * 1024 );
    assert( string );
    memset( string, 0, sizeof(char) * 1024 );
    tmp = string;
    used = 0;

    ret = snprintf( tmp, 1024-used, "%d", tab[0] ); 
    tmp += ret;
    used += ret;

    for( i = 1; i < size; i++ )
    {
        ret = snprintf( tmp, 1024-used, ",%d", tab[i] ); 
        tmp += ret;
        used += ret;
    } 
    string[used] = '\0';
    return string; 
}

static int*
__mpcomp_tree_array_get_father_array_by_depth( const int* shape, const int max_depth, const int rank )
{
    int i;
    int tmp_rank = -1;
    int* parents_array = NULL;
    int nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );

    if( !rank ) return NULL;
    
    tmp_rank = rank;
    parents_array = (int*) malloc( sizeof(int) * depth ); 
    assert( parents_array );

    for( i = depth -1; i > 0; i--)
    { 
        const int level_up_nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth(shape, i-1);
        const int stage_node_rank = __mpcomp_tree_array_convert_global_to_stage( shape, depth, tmp_rank );
        tmp_rank = stage_node_rank / shape[i] + level_up_nodes_num;
        parents_array[i] = tmp_rank;
    }
        
    parents_array[0] = 0; // root 
    
    return parents_array;
}

static int*
__mpcomp_tree_array_get_father_array_by_level( const int* shape, const int max_depth, const int rank )
{
    int i;
    int tmp_rank = -1;
    int* parents_array = NULL;
    int nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );

    if( !rank ) return NULL;

    tmp_rank = rank;
    parents_array = (int*) malloc( sizeof(int) * depth );
    assert( parents_array );

    for( i = depth -1; i > 0; i--)
    {
        int distance = i - (depth -1); /* translate i [ a, b] to i [ 0, b - a] */ 
        const int level_up_nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth(shape, i-1);
        const int stage_node_rank = __mpcomp_tree_array_convert_global_to_stage( shape, depth, tmp_rank );
        tmp_rank = stage_node_rank / shape[i] + level_up_nodes_num;
        parents_array[distance] = tmp_rank;
    }

    parents_array[0] = 0; // root 
    return parents_array;
}



static int*
__mpcomp_tree_array_get_first_child_array( const int* shape, const int max_depth, const int rank )
{
    int i, last_father_rank, last_stage_size;
    int* first_child_array = NULL; 

    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
    const int children_levels = max_depth - depth;

    if(  !children_levels ) return NULL;

    first_child_array = (int*) malloc( sizeof(int) * children_levels );
    assert( first_child_array );
    memset( first_child_array, 0,  sizeof(int) * children_levels );

    last_father_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, rank );
    
    for( i = 0; i < children_levels; i++ )
    {
        last_stage_size = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, depth+i );
        last_father_rank = last_stage_size + shape[depth+i] * last_father_rank;
        first_child_array[i] = last_father_rank;
        last_father_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, last_father_rank ); 
     //   fprintf(stderr, "[%d - %d] <<%s>> last_stage_size: %d -- last_father_rank : %d -- shape : %d -- first_child_array : %d\n", rank, i, __func__, last_stage_size, last_father_rank, shape[depth+i], first_child_array[i] );
    } 
    
    return first_child_array; 
}

static int*
__mpcomp_tree_array_get_children_num_array( const int* shape, const int max_depth, const int rank )
{
    int i;
    int *children_num_array = NULL;

    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
    const int children_levels = max_depth - depth;

    if(  !children_levels ) return NULL;

    children_num_array = (int*) malloc( sizeof(int) * children_levels );
    assert( children_num_array );
    memset( children_num_array, 0,  sizeof(int) * children_levels );
    
    for( i = 0; i < children_levels; i++ )
    {
        children_num_array[i] = shape[depth + i]; 
    } 

    return children_num_array;
}

static int
__mpcomp_tree_array_compute_thread_openmp_min_rank_compact( const int* shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{
    int i, count;
    int *node_parents = NULL;
    int tmp_cumulative, tmp_local_rank;


    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        node_parents = __mpcomp_tree_array_get_father_array_by_depth( shape, depth, rank ); 

        tmp_local_rank = __mpcomp_tree_array_convert_global_to_local(shape, max_depth, rank);
        tmp_cumulative = __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, depth );
        count = tmp_local_rank;
        count *= ( depth < max_depth ) ? tmp_cumulative : 1;

        for( i = 0; i < depth; i++ )
        {
            const int parent_depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, node_parents[i] );
            tmp_local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, node_parents[i] );
            tmp_cumulative = __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, parent_depth );
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

static int
__mpcomp_tree_array_compute_thread_openmp_min_rank_balanced( const int* shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{
    int i, count;
    int *node_parents = NULL;
    int tmp_cumulative, tmp_nodes_per_depth, tmp_local_rank;

    const int core_depth = instance->core_depth;    

    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        const int tmp_cumulative_core = __mpcomp_tree_array_get_tree_cumulative( shape, max_depth);
        node_parents = __mpcomp_tree_array_get_father_array_by_depth( shape, depth, rank );
        tmp_local_rank = __mpcomp_tree_array_convert_global_to_local(shape, max_depth, rank);

        if( depth-1 < core_depth )
        {
            tmp_cumulative = __mpcomp_tree_array_get_tree_cumulative( shape, max_depth);
            count = tmp_local_rank * tmp_cumulative / tmp_cumulative_core;
        //    fprintf(stderr,  "case 1a -- count :  %d %d %d -- %d -- %d \n", tmp_local_rank,  depth-1, tmp_cumulative, tmp_cumulative_core, count);
        }
        else
        {
            tmp_nodes_per_depth = __mpcomp_tree_array_get_stage_nodes_num_per_depth( shape, max_depth, depth-1);
            count = tmp_local_rank * tmp_nodes_per_depth;
      //      fprintf(stderr,  "case 2a -- count : %d\n", count);
        }
        
        for( i = 0; i < depth-1; i++ ) 
        {
            const int parent_depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, node_parents[i] );
            tmp_local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, node_parents[i] );

            if( parent_depth-1 < core_depth )
            {
                tmp_cumulative = __mpcomp_tree_array_get_tree_cumulative( shape, max_depth); 
                count += ( tmp_local_rank * tmp_cumulative / tmp_cumulative_core); 
        //        fprintf(stderr,  "case 1b\n");
            }
            else
            {
                tmp_nodes_per_depth = __mpcomp_tree_array_get_stage_nodes_num_per_depth( shape, max_depth, parent_depth-1); 
                count += ( tmp_local_rank * tmp_nodes_per_depth);
         //       fprintf(stderr,  "case 2b\n");
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

static int
__mpcomp_tree_array_update_thread_openmp_min_rank_scatter (const int father_stage_size, const int father_min_index, const int node_local_rank )
{
    return father_min_index + node_local_rank * father_stage_size;
}

static int
__mpcomp_tree_array_compute_thread_openmp_min_rank_scatter( const int* shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{

    int i, count; 
    int *node_parents = NULL;
    int tmp_nodes_per_depth, tmp_local_rank;
    
    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        node_parents = __mpcomp_tree_array_get_father_array_by_depth( shape, depth, rank );

        tmp_local_rank = __mpcomp_tree_array_convert_global_to_local(shape, max_depth, rank);
        tmp_nodes_per_depth = __mpcomp_tree_array_get_stage_nodes_num_per_depth( shape, max_depth, depth); 
        count = tmp_local_rank * tmp_nodes_per_depth; 

        for( i = 0; i < depth; i++ ) 
        {
            const int parent_depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, node_parents[i] );
            tmp_local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, node_parents[i] );
            tmp_nodes_per_depth = __mpcomp_tree_array_get_stage_nodes_num_per_depth( shape, max_depth, parent_depth); 
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

static int*
__mpcomp_tree_array_compute_thread_openmp_min_rank( const int* shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{
    int index;
    int tmp_rank, stage_rank, count;
    int* min_index = NULL; 
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank ); 

    min_index = ( int* ) malloc( sizeof(int) * MPCOMP_AFFINITY_NB );
    assert( min_index );
    memset( min_index, 0, sizeof(int) * MPCOMP_AFFINITY_NB );

    /* MPCOMP_AFFINITY_COMPACT  */ 
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_compact(shape, max_depth, rank, instance);
    min_index[MPCOMP_AFFINITY_COMPACT] = index; 

    /* MPCOMP_AFFINITY_SCATTER  */
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_scatter(shape, max_depth, rank, instance);
    min_index[MPCOMP_AFFINITY_SCATTER] = index;

    /* MPCOMP_AFFINITY_BALANCED */
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_balanced(shape, max_depth, rank, instance);
    min_index[MPCOMP_AFFINITY_BALANCED] = index; 

    const int tmp_cumulative_core =  __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, instance->core_depth -1);
    char* string_val = __mpcomp_tree_array_convert_array_to_string( min_index, 3 ); 
    fprintf(stderr, "[%d> %s [%d %d]\n", rank, string_val, instance->core_depth, tmp_cumulative_core );
    free( string_val );
   
    return min_index;
} 

mpcomp_meta_tree_node_t** 
__mpcomp_allocate_tree_array( const int* shape, const int max_depth, int *tree_size )
{
    int i;
    mpcomp_meta_tree_node_t** root;

    const int n_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );    
    const int non_leaf_n_num = (max_depth > 0 ) ? __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth -1 ) : 1;
    
    root = (mpcomp_meta_tree_node_t**) malloc( sizeof(mpcomp_meta_tree_node_t*) ); 
    assert( root );
    memset( root, 0,  n_num * sizeof(mpcomp_meta_tree_node_t*) ); 

    for( i = 0; i < n_num; i++ )
    {
        /* Allocation */
        root[i] = (mpcomp_meta_tree_node_t*) malloc( sizeof(mpcomp_meta_tree_node_t));
        assert( root[i] );
        memset( root[i], 0, sizeof(mpcomp_meta_tree_node_t) );

        /* Generic infos */
        root[i]->rank = i;
        root[i]->numa_id = 0;   //TODO: use HWLOC topology
        root[i]->type  = ( i < non_leaf_n_num ) ? MPCOMP_META_TREE_NODE : MPCOMP_META_TREE_LEAF; 
        root[i]->depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, i );  
        root[i]->stage_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, i );
        root[i]->local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, i );

        /* Father initialisation */
        root[i]->fathers_array_size = root[i]->depth;
        root[i]->fathers_array = __mpcomp_tree_array_get_father_array_by_level( shape, max_depth, i );

        /* Children initialisation */
        root[i]->children_array_size = max_depth - root[i]->depth;
        root[i]->first_child_array = __mpcomp_tree_array_get_first_child_array( shape, max_depth, i );
        root[i]->children_num_array = __mpcomp_tree_array_get_children_num_array( shape, max_depth, i );
        
        /* Affinity initialisation */
        //root[i]->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( shape, max_depth, i, instance );
        
    }

    return root;
}

static int __mpcomp_tree_array_find_next_running_stage_depth(   const int* tree_shape, 
                                                                const int start_depth, 
                                                                const int max_depth, 
                                                                const int num_requested_mvps,
                                                                int *num_assigned_mvps)
{
    int i;
    int cumulative_children_num = 1;

    for( i = start_depth; i < max_depth; i++)
    {
        cumulative_children_num *= tree_shape[i];
        if( cumulative_children_num >= num_requested_mvps ) break;
    }

    *num_assigned_mvps = cumulative_children_num;
    return i;
}

static mpcomp_node_t*
__mpcomp_tree_array_wake_up_node( mpcomp_node_t *start_node, mpcomp_instance_t *instance, const int unsigned num_threads )
{
    mpcomp_node_t* current_node;
    int i, nb_children_involved, rest, quot, node_num_threads, target_node;

    current_node = start_node;
    node_num_threads = num_threads;

    while( current_node->nb_children < num_threads && current_node->child_type != MPCOMP_CHILDREN_LEAF )
    {
        quot = node_num_threads / ( current_node->nb_children ); 
        rest = node_num_threads % ( current_node->nb_children ); 
        assert( rest != 0 ); //TODO MANAGE REST

        /* Update the number of threads for the barrier */
        current_node->barrier_num_threads = quot + ( ( rest ) ? 1 : 0 );  
        
        for( i = 1; i < quot; i++ )
        {
            int target_node_idx = i * current_node->nb_children;
            mpcomp_node_t* target_node = current_node->children.node[target_node_idx];
#if MPCOMP_TRANSFER_INFO_ON_NODES
            target_node->info = current_node->info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */ 
            target_node->slave_running = 1;
        } 

        mpcomp_node_t* target_node = current_node->children.node[0];
#if MPCOMP_TRANSFER_INFO_ON_NODES
        target_node->info = current_node->info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
        current_node = target_node;
        node_num_threads = quot;
    }

    return current_node;
} 

static mpcomp_node_t*
__mpcomp_tree_array_wake_up_leaf( mpcomp_node_t *start_node, mpcomp_instance_t *instance, const int unsigned num_threads )
{
    int i;
    mpcomp_mvp_t* target_mvp;

    // Nothing to do ...
    if( num_threads == 1 ) return start_node;

    /* Wake up children leaf */
    const int quot = num_threads / ( start_node->nb_children ); 
    const int rest = num_threads % ( start_node->nb_children );
    assert( rest != 0 ); //TODO MANAGE REST 

    /* Update the number of threads for the barrier */
    start_node->barrier_num_threads = quot + ( ( rest ) ? 1 : 0 );
    
    for( i = 1; i < quot; i++ )
    {
        target_mvp->instance = instance;
        target_mvp = start_node->children.leaf[i]; 
#if MPCOMP_TRANSFER_INFO_ON_NODES 
        target_mvp->info = start_node->info; 
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
        target_mvp->slave_running = 1;
    }

    return start_node;
}

void __mpcomp_instance_alloc_and_init( mpcomp_thread_t* cur_thread, int num_requested_mvps ) 
{
    int* tree_shape;
    int i, max_mvp_children;

    mpcomp_local_icv_t icvs;
    mpcomp_team_t* new_team;
    mpcomp_mvp_t* thread_mvp;
    mpcomp_instance_t* new_instance;

    new_instance = ( mpcomp_instance_t* ) malloc( sizeof( mpcomp_instance_t ) ); 
    assert( new_instance );
    memset( new_instance, 0, sizeof( mpcomp_instance_t ) );

    new_instance->team = ( mpcomp_team_t* ) malloc( sizeof( mpcomp_team_t ) );
    assert( new_instance->team );
    __mpcomp_team_infos_init( new_instance->team );
   
    thread_mvp = cur_thread->mvp; 
    assert( thread_mvp );

    const int cur_thread_depth = cur_thread->depth; 
    const int mvps_tree_depth = cur_thread->mvp->depth;
    tree_shape = cur_thread->mvp->root->nb_children;
    assert( tree_shape );

    /* Sanity check */
    num_requested_mvps = ( num_requested_mvps ) ? num_requested_mvps : 1;
    
    /* Find next */
    const int next_depth = __mpcomp_tree_array_find_next_running_stage_depth(   tree_shape, cur_thread_depth, mvps_tree_depth, 
                                                                                num_requested_mvps, &max_mvp_children );
    
    new_instance->nb_mvps = ( max_mvp_children < num_requested_mvps ) ? max_mvp_children : num_requested_mvps;
    new_instance->mvps = (mpcomp_mvp_t **) malloc( sizeof( mpcomp_mvp_t * ) * new_instance->nb_mvps ); 
    assert( new_instance->mvps );
    memset( new_instance->mvps, 0, sizeof( mpcomp_mvp_t * ) * new_instance->nb_mvps );


    if( new_instance->nb_mvps > 1 )
    {
        mpcomp_node_t* next_node = (mpcomp_node_t* ) thread_mvp->root; 
        assert( next_node ); 

        for( i = cur_thread_depth; i <= next_depth; i++ )
        {
            next_node = next_node->children.node[0]; 
        }
        
        for( i = 0; i < new_instance->nb_mvps; i++ ) 
        {
            new_instance->mvps[i] = next_node->mvp; 
            __mpcomp_thread_infos_init_and_push(new_instance->mvps[i], icvs, new_instance, cur_thread );   
            next_node = next_node->next_brother;
        }
        
    }
    else /* new_instance->nb_mvps == 1 */
    {
        new_instance->mvps[0] = thread_mvp;
        __mpcomp_thread_infos_init_and_push(new_instance->mvps[0], icvs, new_instance, cur_thread );
    }
    
}

static inline int 
__mpcomp_openmp_node_initialisation( mpcomp_meta_tree_node_t* root, const int* tree_shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{
    int i, father_rank;
    mpcomp_node_t* new_node = NULL;
    mpcomp_meta_tree_node_t* me = NULL;

    me = &( root[rank] );
    // fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>>>>>> root : %p -- me : %p\n", root, me );

    /* Generic infos */
    me->rank = rank;
    me->numa_id = 0; //TODO: use HWLOC topology
    me->type = MPCOMP_META_TREE_LEAF;
    me->depth = __mpcomp_tree_array_get_node_depth( tree_shape, max_depth, rank ); 
    me->stage_rank = __mpcomp_tree_array_convert_global_to_stage( tree_shape, max_depth, rank );
    me->local_rank = __mpcomp_tree_array_convert_global_to_local( tree_shape, max_depth, rank );  
   
    const int children_num = tree_shape[me->depth];

    /* Father initialisation */
    me->fathers_array_size = me->depth; 
    me->fathers_array = __mpcomp_tree_array_get_father_array_by_depth( tree_shape, max_depth, rank );
    
    /* Children initialisation */
    me->children_array_size = max_depth - me->depth;
    me->first_child_array = __mpcomp_tree_array_get_first_child_array( tree_shape, max_depth, rank );
//    me->children_num_array = __mpcomp_tree_array_get_children_num_array( tree_shape, max_depth, rank );

    /* Affinity initialisation */
    me->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( tree_shape, max_depth, rank, instance );
    
    /* User defined value */
    new_node = (mpcomp_node_t *) ( (!me->rank) ? instance->root : malloc( sizeof(mpcomp_node_t) ) );
    assert( new_node );     
    memset( new_node, 0, sizeof(mpcomp_node_t) );

    // save Node
    me->user_pointer = (void*) new_node;
    //new_node->tree_array_node = (void*) me;
    // fprintf(stderr, "\t\t\t<<>> RANK : %d NODE ADDR : %p==%p <<>>\n", me->rank, new_node, me->user_pointer );

    // Get infos from parent node
    new_node->depth = me->depth;
    new_node->id_numa = me->numa_id;
    new_node->rank = me->local_rank; 
    new_node->nb_children = children_num;
    new_node->instance = instance;

    /* Memset already do it ... */
    new_node->barrier_done = 0;
    new_node->slave_running = 0;
    new_node->barrier_num_threads = 0;
    sctk_atomics_store_int( &( new_node->barrier ), 0);

    // Copy min_index from parent node
    memcpy( new_node->min_index, me->min_index, sizeof(int) * MPCOMP_AFFINITY_NB );

    /* Leaf or Node */
    if( new_node->depth < max_depth -1)
    {
        new_node->child_type = MPCOMP_CHILDREN_NODE; 
        new_node->children.node = (mpcomp_node_t ** ) 
                                    malloc( sizeof( mpcomp_node_t* ) * new_node->nb_children );
        assert( new_node->children.node );
        memset( new_node->children.node, 0, sizeof( mpcomp_node_t* ) * new_node->nb_children );
    }
    else
    {
        new_node->child_type = MPCOMP_CHILDREN_LEAF;
        new_node->children.leaf = (mpcomp_mvp_t **) 
                                    malloc( sizeof( mpcomp_mvp_t*) * new_node->nb_children );
        assert( new_node->children.leaf );
        memset( new_node->children.leaf, 0, sizeof(mpcomp_mvp_t *) * new_node->nb_children );
    }

    
    /* Wait our children */
    while( sctk_atomics_load_int( &( me->children_ready ) ) != children_num)  
    {
        sctk_thread_yield();
    }   
   
    /* Update father and children */
    for( i = 0; i < children_num; i++ )
    {
        mpcomp_meta_tree_node_t* child = &( root[me->first_child_array[0] + i] );
        assert( child && child->user_pointer );

        int prev_brother_idx =  ( i == 0 ) ? children_num - 1 : i -1; 
        int next_brother_idx =  ( i + 1 ) % children_num;

        if( new_node->child_type == MPCOMP_CHILDREN_NODE )
        {
            new_node->children.node[i] = (mpcomp_node_t*) child->user_pointer; 
            new_node->children.node[i]->father = new_node;
            if( i == 0 ) new_node->mvp = new_node->children.node[i]->mvp;
            new_node->children.node[i]->prev_brother = new_node->children.node[prev_brother_idx];
            new_node->children.node[i]->next_brother = new_node->children.node[next_brother_idx];
        }
        else
        {
            new_node->children.leaf[i] = (mpcomp_mvp_t*) child->user_pointer;
            new_node->children.leaf[i]->father = new_node;
            if( i == 0 ) new_node->mvp = new_node->children.leaf[i];
            new_node->children.leaf[i]->prev_brother = new_node->children.leaf[prev_brother_idx];
            new_node->children.leaf[i]->next_brother = new_node->children.leaf[next_brother_idx];
        }
    }

    if( !( me->fathers_array ) ) return 0; 

    father_rank = me->fathers_array[me->depth-1];
    const int value = sctk_atomics_fetch_and_incr_int( &( root[father_rank].children_ready) ) + 1; 
    return ( !(me->local_rank) ) ? 1 : 0;
}

static inline void* 
__mpcomp_openmp_mvp_initialisation( void* args )
{
    int target_node_rank; 
    mpcomp_mvp_t * new_mvp = NULL;
    mpcomp_instance_t* instance = NULL;
    mpcomp_meta_tree_node_t* me = NULL;
    mpcomp_meta_tree_node_t* root = NULL;
    mpcomp_mvp_thread_args_t* unpack_args = NULL; 

    /* Unpack thread start_routine arguments */
    unpack_args = (mpcomp_mvp_thread_args_t* ) args;
    const unsigned int rank = unpack_args->rank;
    const unsigned int max_depth = unpack_args->max_depth;
    unsigned int* tree_shape = unpack_args->tree_shape;
    mpcomp_thread_t* thread = (mpcomp_thread_t*) unpack_args->thread;

    root = unpack_args->array;
    me = &( root[rank] );

    /* Generic infos */
    me->rank = rank;
    me->type = MPCOMP_META_TREE_LEAF;
    me->depth = __mpcomp_tree_array_get_node_depth( tree_shape, max_depth, rank ); 
    me->stage_rank = __mpcomp_tree_array_convert_global_to_stage( tree_shape, max_depth, rank );
    me->local_rank = __mpcomp_tree_array_convert_global_to_local( tree_shape, max_depth, rank );  
    
    // TODO CHECK THAT ...
    me->numa_id = sctk_get_node_from_cpu_topology(instance->topology, me->stage_rank );

    // fprintf(stderr, "Global Rank : %d -- Stage Rank : %d -- Local Rank : %d\n", me->rank, me->stage_rank, me->local_rank);
   
    /* Father initialisation */
    me->fathers_array_size = me->depth; 
    me->fathers_array = __mpcomp_tree_array_get_father_array_by_depth( tree_shape, max_depth, rank );
    
#if 0 /* Memset already did it */
    /* Children initialisation */
    me->children_array_size = 0;
    me->first_child_array = NULL;
    me->children_num_array = NULL;
#endif /* Memset already did it */

    /* Affinity initialisation */
    me->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( tree_shape, max_depth, rank, instance );
    
    /* User defined infos */ 
    new_mvp = (mpcomp_mvp_t *) malloc( sizeof(mpcomp_mvp_t) );
    assert( new_mvp );
    memset( new_mvp, 0, sizeof(mpcomp_mvp_t) ); 
            
    // MVP specific infos
    new_mvp->tree_rank = __mpcomp_tree_array_get_tree_rank( tree_shape, max_depth, rank );
    
    /* Initialize the corresponding microVP (all but tree-related variables) */ 

#if 0 /* Memset already did it */
    new_mvp->nb_threads = 0;
    new_mvp->next_nb_threads = 0;
    new_mvp->slave_running = 0;
#endif /* Memset already did it */

    new_mvp->thread_self = thread;
    new_mvp->rank = me->local_rank;
 //   new_mvp->vp = me->stage_rank;
    memcpy( new_mvp->min_index, me->min_index, sizeof(int) * MPCOMP_AFFINITY_NB );

    new_mvp->enable = 1;
    new_mvp->root = instance->root;

    new_mvp->threads = NULL;
 //   new_mvp->threads = (mpcomp_thread_t*) malloc( 1 * sizeof( mpcomp_thread_t ) ); 
 //    assert( new_mvp->threads );
    
 //   mpcomp_local_icv_t icvs;
 //   __mpcomp_thread_infos_init( &( new_mvp->threads[0] ), icvs, instance, sctk_openmp_thread_tls ); 

 //   new_mvp->threads[0].mvp = new_mvp;
 //   new_mvp->threads[0].rank = me->stage_rank;

#if MPCOMP_TASK
    mpcomp_task_mvp_task_infos_reset( new_mvp );   
#endif /* MPCOMP_TASK */

    /* Store mvp pointer */
    me->user_pointer = (void*) new_mvp;
  //  new_mvp->tree_array_node = (void*) me;
    //instance->mvps[me->stage_rank] = new_mvp;

    //fprintf(stderr, "|%d - @%d> Update user_pointer : %p\n", me->rank, me->depth, me->user_pointer );

    /* Tree initialisation election */
    int current_depth = me->depth - 1;
    target_node_rank = me->fathers_array[current_depth]; 

    // Transfert to slave_node_leaf function ...
    const int value = sctk_atomics_fetch_and_incr_int( &( root[target_node_rank].children_ready) ) +1; 
 
    if(  !( me->local_rank ) )  /* The first child is spinning on a node */
    {
        while( __mpcomp_openmp_node_initialisation( root, tree_shape, max_depth, target_node_rank, instance ) )
        {
            current_depth--;
            target_node_rank = me->fathers_array[current_depth];
        }

        if( !( me->stage_rank ) ) 
        {
            return (void*) root[0].user_pointer;   /* Root never wait */
        }

   //     new_mvp->to_run = root[target_node_rank].user_pointer;
        mpcomp_slave_mvp_node( new_mvp );
    }
    else /* The other children are regular leaves */ 
    {
     //   new_mvp->to_run = NULL;
        mpcomp_slave_mvp_leaf( new_mvp ); 
    }

    return NULL;
}

void*
__mpcomp_alloc_openmp_tree_struct( const int* shape, const int max_depth, int *tree_size, mpcomp_instance_t* instance )
{
    int i, ret;
    sctk_thread_t* thread;
    mpcomp_meta_tree_node_t* root;
    mpcomp_mvp_thread_args_t* args;

    char * string_tree_shape = __mpcomp_tree_array_convert_array_to_string( shape, max_depth );
    //fprintf(stderr, "SHAPE : %s\n", string_tree_shape ); 
    free( string_tree_shape );

    const int n_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
    const int non_leaf_n_num = (max_depth > 0 ) ? __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth -1 ) : 1;
    const int leaf_n_num = n_num - non_leaf_n_num;

    root = (mpcomp_meta_tree_node_t*) malloc( n_num * sizeof(mpcomp_meta_tree_node_t) );
    assert( root );
    memset( root, 0,  n_num * sizeof( mpcomp_meta_tree_node_t ) );

   
    instance->mvps = (mpcomp_mvp_t**) malloc( leaf_n_num * sizeof( mpcomp_mvp_t* ) );
    assert(  instance->mvps  );
    memset( instance->mvps, 0, leaf_n_num * sizeof( mpcomp_mvp_t* )  );

    instance->root = (mpcomp_node_t* ) malloc( sizeof( mpcomp_node_t ) * 1 );
    assert( instance->root );
    memset( instance->root, 0, sizeof( mpcomp_node_t ) * 1 );

    for( i = n_num - 1; i >= non_leaf_n_num; i-- )
    {
        thread = ( sctk_thread_t* ) malloc( sizeof( sctk_thread_t ) );  
        assert( thread ); 
        memset( thread, 0, sizeof( sctk_thread_t ) );

        args = (mpcomp_mvp_thread_args_t*) malloc( sizeof( mpcomp_mvp_thread_args_t));
        assert( args );
        memset( args, 0,  sizeof( mpcomp_mvp_thread_args_t) );
        
        args->rank = i;
        args->array = root;
        args->tree_shape = shape;
        args->thread = ( void* ) thread;
        //args->instance = ( void* ) instance;
        args->max_depth = max_depth;

        if( i != non_leaf_n_num )
        {
            const int target_vp = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, i );
            const int pu_id = ( sctk_get_cpu() + target_vp ) % sctk_get_cpu_number(); 

            sctk_thread_attr_t __attr;
            sctk_thread_attr_init(&__attr);
            sctk_thread_attr_setbinding( &__attr, pu_id);
            sctk_thread_attr_setstacksize( &__attr, mpcomp_global_icvs.stacksize_var);

            ret = sctk_user_thread_create( thread, &__attr, __mpcomp_openmp_mvp_initialisation, args );
            assert( ret == 0 );

            sctk_thread_attr_destroy(&__attr);
        }

    }
    return (void*) __mpcomp_openmp_mvp_initialisation( args );
}

#ifdef MPCOMP_COMPILE_WITH_MAIN
int main( int argc, char* argv )
{
    hwloc_topology_t topology;
    
    int shape[3] = { 2, 4, 2 };
    num_depth = 1;

    int depth = 3;
    int tree_size;

    mpcomp_meta_tree_node_t** root = __mpcomp_allocate_tree_array( shape, depth, &tree_size );
   
    return 0;
}
#endif 
