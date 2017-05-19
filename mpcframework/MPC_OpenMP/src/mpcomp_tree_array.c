#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "mpcomp_tree_array.h"

#ifndef MPCOMP_AFFINITY_NB
#define MPCOMP_AFFINITY_COMPACT     0 
#define MPCOMP_AFFINITY_SCATTER     1 
#define MPCOMP_AFFINITY_BALANCED    2
#define MPCOMP_AFFINITY_NB          3 
#endif /* MPCOMP_AFFINITY_NB */

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
    
    if( !rank ) return 0;

    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
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

static char* 
__mpcomp_tree_array_convert_array_to_string( int* tab, int size )
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
__mpcomp_tree_array_get_father_array( const int* shape, const int max_depth, const int rank )
{
    int i;
    int tmp_rank = -1;
    int* parents_array = NULL;
    int nodes_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
    
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

    last_father_rank = rank;
    for( i = 0; i < children_levels; i++ )
    {
        last_stage_size = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, depth+i-1 );
        last_father_rank = last_stage_size + shape[depth+i] * last_father_rank;
        first_child_array[i] = last_father_rank;
    } 
    
    return NULL; 
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
__mpcomp_tree_array_compute_thread_openmp_min_rank_compact( const int* shape, const int max_depth, const int rank )
{
    int i, count;
    int *node_parents = NULL;
    int tmp_cumulative, tmp_local_rank;


    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        node_parents = __mpcomp_tree_array_get_father_array( shape, depth, rank ); 

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
__mpcomp_tree_array_compute_thread_openmp_min_rank_balanced( const int* shape, const int max_depth, const int rank )
{
    int i, count;
    int *node_parents = NULL;
    int tmp_cumulative, tmp_nodes_per_depth, tmp_local_rank;

    const int core_depth = max_depth;    

    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        const int tmp_cumulative_core =  __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, core_depth -1);
        node_parents = __mpcomp_tree_array_get_father_array( shape, depth, rank );
        
        tmp_local_rank = __mpcomp_tree_array_convert_global_to_local(shape, max_depth, rank);
        if( depth-1 < core_depth )
        {
            tmp_cumulative = __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, depth-1);
            count = tmp_local_rank * tmp_cumulative / tmp_cumulative_core;
        //    fprintf(stderr,  "case 1a -- count :  %d %d %d -- %d -- %d \n", tmp_local_rank,  depth-1, tmp_cumulative, tmp_cumulative_core, count);
        }
        else
        {
            tmp_nodes_per_depth = __mpcomp_tree_array_get_stage_nodes_num_per_depth( shape, max_depth, depth-1);
            count = tmp_local_rank * tmp_nodes_per_depth;
      //      fprintf(stderr,  "case 2a -- count : %d\n", count);
        }
        
        for( i = 0; i < depth; i++ ) 
        {
            const int parent_depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, node_parents[i] );
            tmp_local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, node_parents[i] );

            if( parent_depth-1 < core_depth )
            {
                tmp_cumulative = __mpcomp_tree_array_get_children_nodes_num_per_depth( shape, max_depth, parent_depth-1); 
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
__mpcomp_tree_array_compute_thread_openmp_min_rank_scatter( const int* shape, const int max_depth, const int rank )
{

    int i, count; 
    int *node_parents = NULL;
    int tmp_nodes_per_depth, tmp_local_rank;
    
    if( rank )
    {
        const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank );
        node_parents = __mpcomp_tree_array_get_father_array( shape, depth, rank );

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
__mpcomp_tree_array_compute_thread_openmp_min_rank( const int* shape, const int max_depth, const int rank )
{
    int index;
    int tmp_rank, stage_rank, count;
    int* min_index = NULL; 
    const int depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank ); 

    min_index = ( int* ) malloc( sizeof(int) * MPCOMP_AFFINITY_NB );
    assert( min_index );
    memset( min_index, 0, sizeof(int) * MPCOMP_AFFINITY_NB );

    /* MPCOMP_AFFINITY_COMPACT  */ 
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_compact(shape, max_depth, rank);
    min_index[MPCOMP_AFFINITY_COMPACT] = index; 

    /* MPCOMP_AFFINITY_SCATTER  */
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_scatter(shape, max_depth, rank);
    min_index[MPCOMP_AFFINITY_SCATTER] = index;

    /* MPCOMP_AFFINITY_BALANCED */
    index = __mpcomp_tree_array_compute_thread_openmp_min_rank_balanced(shape, max_depth, rank);
    min_index[MPCOMP_AFFINITY_BALANCED] = index; 

    char* string_val = __mpcomp_tree_array_convert_array_to_string( min_index, 3 ); 
    fprintf( stderr, "min index rank %d -- %s\n", rank, string_val );
    free( string_val );
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
        root[i]->fathers_array = __mpcomp_tree_array_get_father_array( shape, max_depth, i );

        /* Children initialisation */
        root[i]->children_array_size = max_depth - root[i]->depth;
        root[i]->first_child_array = __mpcomp_tree_array_get_first_child_array( shape, max_depth, i );
        root[i]->children_num_array = __mpcomp_tree_array_get_children_num_array( shape, max_depth, i );
        
        /* Affinity initialisation */
        root[i]->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( shape, max_depth, i );
        
    }

    return root;
}

static inline int 
__mpcomp_openmp_node_initialisation( mpcomp_meta_tree_node_t* root, const int tree_shape, const int max_depth, const int rank, mpcomp_instance_t* instance )
{
    int father_rank;
    mpcomp_node_t* new_node = NULL;
    mpcomp_meta_tree_node_t* me = NULL;

    me = &( root[rank] );

    /* Generic infos */
    me->rank = rank;
    me->numa_id = 0; //TODO: use HWLOC topology
    me->type = MPCOMP_META_TREE_LEAF;
    me->depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank ); 
    me->stage_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, rank );
    me->local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, rank );  
   
    /* Father initialisation */
    me->fathers_array_size = me->depth; 
    me->fathers_array = __mpcomp_tree_array_get_father_array( shape, max_depth, rank );
    
    /* Children initialisation */
    me->children_array_size = max_depth - me->depth; ;
    me->first_child_array = __mpcomp_tree_array_get_first_child_array( shape, max_depth, rank );
    me->children_num_array = __mpcomp_tree_array_get_children_num_array( shape, max_depth, rank );

    /* Affinity initialisation */
    me->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( shape, max_depth, rank );
    
    /* User defined value */
    new_node = (mpcomp_node_t *) malloc( sizeof(mpcomp_node_t) );
    assert( new_node );     
    memset( new_node, 0, sizeof(mpcomp_node_t) );

    // Get infos from parent node
    new_node->depth = me->depth;
    new_node->id_numa = me->numa_id;
    new_node->rank = me->local_rank; 
    new_node->nb_children = me->children_num_array[0];

    // Copy min_index from parent node
    memcpy( new_node->min_index, me->min_index, sizeof(int) * MPCOMP_AFFINITY_NB );
        
    // save Node
    me->user_pointer = (void*) new_node;
    
    /* Wait our children */
    while( sctk_atomics_load_int( me->children_ready ) != me->children_array_size[0] )  
    {
        sctk_thread_yield();
    }
     
    if( !( me->fathers ) ) return 0; 

    father_rank = node_tree->fathers[0];
    father_rank = __mpcomp_tree_array_convert_global_to_local( tree_shape, max_depth, father_rank );
    return ( father_rank ) ? 0 : 1;
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

    unpack_args = (mpcomp_mvp_thread_args_t* ) args;

    const unsigned int rank = args->rank;
    const unsigned int max_depth = args->max_depth;
    const unsigned int tree_shape = args->tree_shape;
    
    instance = (mpcomp_instance_t*) args->instance; 
    root = args->array;
    me = &( root[rank] );

    /* Generic infos */
    me->rank = rank;
    me->numa_id = 0; //TODO: use HWLOC topology
    me->type = MPCOMP_META_TREE_LEAF;
    me->depth = __mpcomp_tree_array_get_node_depth( shape, max_depth, rank ); 
    me->stage_rank = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, rank );
    me->local_rank = __mpcomp_tree_array_convert_global_to_local( shape, max_depth, rank );  
   
    /* Father initialisation */
    me->fathers_array_size = me->depth; 
    me->fathers_array = __mpcomp_tree_array_get_father_array( shape, max_depth, rank );
    
    /* Children initialisation */
    me->children_array_size = 0;
    me->first_child_array = NULL;
    me->children_num_array = NULL;

    /* Affinity initialisation */
    me->min_index = __mpcomp_tree_array_compute_thread_openmp_min_rank( shape, max_depth, rank );
    
    /* User defined infos */ 
    new_mvp = (mpcomp_mvp_t *) malloc( sizeof(mpcomp_mvp_t) );
    assert( new_mvp );
    memset( new_mvp, 0, sizeof(mpcomp_mvp_t) ); 
            
    // MVP specific infos
    new_mvp->enable = 1;
    new_mvp->to_run = NULL;
    new_mvp->slave_running = 0;

    /* Copy min_index from parent node */
    memcpy( new_node->min_index, me->min_index, sizeof(int) * MPCOMP_AFFINITY_NB );

    /* Store mvp pointer */
    me->user_infos = (void*) new_mvp;

    /* Tree initialisation election */
    target_node_rank = me->fathers[0]; 
    // TODO: transfert to slave_node_leaf function ...
    sctk_atomics_incr_int( &( root[target_node_rank].children_ready) ); 
 
    if(  !( me->local_rank ) )  /* The first child is spinning on a node */
    {
        while( __mpcomp_openmp_node_initialisation( tree_shape, max_depth, target_node_rank, instance ) )
            target_node_rank = target_node_rank->fathers[0];

        if( !( me->stage_rank ) ) return;   /* Root never wait */

        new_mvp->to_run = root[target_node_rank].user_pointer;
        mpcomp_slave_mvp_node( new_mvp );
    }
    else /* The other children are regular leaves */ 
    {
        new_mvp->to_run = NULL;
        mpcomp_slave_mvp_leaf( new_mvp ); 
    }
}

static inline void
__mpcomp_alloc_openmp_tree_struct( const int* shape, const int max_depth, int *tree_size, mpcomp_instance_t* instance )
{

    int i, ret;
    sctk_thread_t* thread;
    mpcomp_meta_tree_node_t* root;
    mpcomp_mvp_thread_args_t* args;

    const int n_num = __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth );
    const int non_leaf_n_num = (max_depth > 0 ) ? __mpcomp_tree_array_get_parent_nodes_num_per_depth( shape, max_depth -1 ) : 1;

    root = (mpcomp_meta_tree_node_t*) malloc( sizeof(mpcomp_meta_tree_node_t) );
    assert( root );
    memset( root, 0,  n_num * sizeof( mpcomp_meta_tree_node_t ) );

    for( i = non_leaf_n_num; i < n_num; i++ )
    {
        thread = ( sctk_thread_t* ) malloc( sizeof( sctk_thread_t ) );  
        assert( thread ); 
        memset( thread, 0, sizeof( sctk_thread_t ) );

        args = (mpcomp_mvp_thread_args_t*) malloc( sizeof( mpcomp_mvp_thread_args_t));
        assert( args );
        memset( args, 0,  sizeof( mpcomp_mvp_thread_args_t) )
        
        args->rank = i;
        args->array = root;
        args->tree_shape = shape;
        args->instance = ( void* ) instance;
        args->max_depth = max_depth;
        args->opaque = ( void ) thread;
 
        if( i == non_leaf_n_num ) 
        {
            (void) __mpcomp_openmp_mvp_initialisation( args );
        }
        else
        {
            const int vp_target = __mpcomp_tree_array_convert_global_to_stage( shape, max_depth, i );
            const int pu_id = ( sctk_get_cpu() + target_vp ) % sctk_get_cpu_number(); 

            sctk_thread_attr_t __attr;
            sctk_thread_attr_init(&__attr);
            sctk_thread_attr_setbinding( &__attr, mpcomp_global_icvs.stacksize_var);
            sctk_thread_attr_setstacksize( &__attr, mpcomp_global_icvs.stacksize_var);

            ret = sctk_user_thread_create( thread, &__attr, __mpcomp_openmp_mvp_initialisation, args );
            assert( ret == 0 )

            sctk_thread_attr_destroy(&__attr);
        }
    }
}

#ifdef MPCOMP_COMPILE_WITH_MAIN
int main( int argc, char* argv )
{
    hwloc_topology_t topology;
    
    int shape[3] = { 2, 4, 2 };
    num_depth = 1;

    int depth = 3;
    int tree_size;
    char * string_tree_shape = __mpcomp_tree_array_convert_array_to_string( shape, depth );
    
    fprintf(stderr, "Test on tree shape : %s\n", string_tree_shape );
    free( string_tree_shape );

    mpcomp_meta_tree_node_t** root = __mpcomp_allocate_tree_array( shape, depth, &tree_size );
   
    return 0;
}
#endif 
