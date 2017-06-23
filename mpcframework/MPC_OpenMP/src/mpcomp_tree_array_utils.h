#ifndef __MPCOMP_TREE_ARRAY_UTILS_H__
#define __MPCOMP_TREE_ARRAY_UTILS_H__

static char*
__mpcomp_tree_array_convert_array_to_string( int* tab, const int size )
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

static inline int*
__mpcomp_tree_array_compute_tree_shape( mpcomp_node_t* node, int* tree_shape, const int size  )
{
    int i;
    int *node_tree_shape;
 
    sctk_assert( node );
    sctk_assert( tree_shape );
    
    const int node_tree_size = size - node->depth + 1; 
    sctk_assert( node_tree_size );

    node_tree_shape = (int*) malloc( sizeof(int) * node_tree_size ); 
    sctk_assert( node_tree_shape );
    memset( node_tree_shape, 0, sizeof(int) * node_tree_size );
    
    const int start_depth = node->depth-1;
    node_tree_shape[0] = 1;
    for( i = 1; i < node_tree_size; i++ )
    {
        node_tree_shape[i] = tree_shape[start_depth + i];
        sctk_assert( node_tree_shape[i] );
    }

    return node_tree_shape;
}

static inline int*
__mpcomp_tree_array_compute_tree_num_nodes_per_depth( mpcomp_node_t* node, int* total )
{
    int i;
    int* num_nodes_per_depth;
    int total_nodes;

    sctk_assert( node );
    sctk_assert( total );

    const int node_tree_size = node->tree_depth;
    sctk_assert( node_tree_size );

    num_nodes_per_depth = (int*) malloc( sizeof( int ) * node_tree_size );
    sctk_assert( num_nodes_per_depth );
    memset( num_nodes_per_depth, 0, sizeof( int ) * node_tree_size );

    sctk_assert( node->tree_base );

    
    num_nodes_per_depth[0] = 1;
    total_nodes = num_nodes_per_depth[0]; 
    for (i = 1; i < node_tree_size ; i++)
    {
        num_nodes_per_depth[i] = num_nodes_per_depth[i-1];
        num_nodes_per_depth[i] *= node->tree_base[i];
        sctk_assert( num_nodes_per_depth[i] );
        total_nodes += num_nodes_per_depth[i];
    }
   
    *total = total_nodes;     
    return num_nodes_per_depth;
}

static inline int*
__mpcomp_tree_array_compute_cumulative_num_nodes_per_depth( mpcomp_node_t* node )
{
    int i, j;
    int* cumulative_num_nodes;

    sctk_assert( node );

    const int node_tree_size = node->tree_depth;
    sctk_assert( node_tree_size ); 
    
    cumulative_num_nodes = (int*) malloc( sizeof( int ) * node_tree_size );
    sctk_assert( cumulative_num_nodes );
    memset( cumulative_num_nodes, 0, sizeof( int ) * node_tree_size );

    sctk_assert( node->tree_base );
    
    for (i = 0; i < node_tree_size ; i++)
    {
        cumulative_num_nodes[i] = 1;
        for (j = i + 1; j < node_tree_size; j++)
        {
            cumulative_num_nodes[i] *= node->tree_base[j];
            sctk_assert( cumulative_num_nodes[i] );
        }
        sctk_assert( cumulative_num_nodes[i] );
    }    
   
    return cumulative_num_nodes; 
}

static inline int* 
__mpcomp_tree_array_compute_tree_first_children( mpcomp_node_t* root, mpcomp_node_t* node )
{
    int i, shift_next_stage_stage, stage_rank;
    int* node_first_children; 
    
    sctk_assert( node );
    sctk_assert( root );
    
    const int root_tree_size = root->tree_depth;
    sctk_assert( root_tree_size );
    sctk_assert( node->depth < root_tree_size );
    const int node_tree_size = root_tree_size - node->depth; 
    
    node_first_children = (int*) malloc( sizeof(int) * node_tree_size ); 
    sctk_assert( node_first_children );
    memset( node_first_children, 0, sizeof(int) * node_tree_size );

    node_first_children[0] = node->global_rank;
    const int node_stage_size = root->tree_base[node->depth];

    shift_next_stage_stage = 0;
    for( i = 0; i <= node->depth; i++ )
        shift_next_stage_stage += root->tree_nb_nodes_per_depth[i]; 
    
    stage_rank = node->stage_rank;
    for( i = 1; i < node_tree_size; i++ )
    {
        stage_rank = stage_rank * root->tree_base[node->depth+i];
        node_first_children[i] = shift_next_stage_stage + stage_rank;
        shift_next_stage_stage += root->tree_nb_nodes_per_depth[node->depth+i];
    }

    return node_first_children;
}  

static inline int*
__mpcomp_tree_array_compute_node_parents(  mpcomp_node_t* root, mpcomp_node_t* node )
{
    int i, prev_num_nodes, stage_rank, shift_next_stage_stage, cur_rank;
    int *node_parents;

    sctk_assert( node );
    sctk_assert( root ); 

    const int node_tree_size = node->depth + 1;
    
    node_parents = (int*) malloc( sizeof(int) * node_tree_size ); 
    sctk_assert( node_parents );
    memset( node_parents, 0, sizeof(int) * node_tree_size );

    node_parents[0] = node->global_rank;
  
    prev_num_nodes = 1; 
    for( i = 1; i < node->depth; i++ )
        prev_num_nodes += root->tree_nb_nodes_per_depth[i]; 

    for(i = 1; i < node_tree_size; i++ )
    {
        if( root->tree_nb_nodes_per_depth[node->depth-i] == 1 )
        {
            node_parents[i] = 0;
            continue;
        }

        stage_rank = node_parents[i-1] - prev_num_nodes;
        prev_num_nodes -= root->tree_nb_nodes_per_depth[node->depth-i];
        node_parents[i] = stage_rank / ( root->tree_nb_nodes_per_depth[node->depth-i+1] / root->tree_base[i] ) + prev_num_nodes; 
        
    }
}

static void inline 
__mpcomp_update_node_children_node_ptr( const int first_idx, 
                                        mpcomp_node_t* node, 
                                        mpcomp_meta_tree_node_t* root )
{
    int i;
    mpcomp_meta_tree_node_t* me;

    sctk_assert( node );
    sctk_assert( first_idx > 0 );

    sctk_assert( root );
    assert( node->child_type == MPCOMP_CHILDREN_NODE );

    const int children_num = node->nb_children;
    me = &(root[node->global_rank]);
    assert(sctk_atomics_load_int(&(me->children_ready)) == children_num);

    for( i = 0; i < children_num; i++ ) 
    { 
        const int global_child_idx = first_idx + i; 
        mpcomp_meta_tree_node_t* child = &( root[global_child_idx] );
        node->children.node[i] = (mpcomp_node_t*) child->user_pointer;
        sctk_assert( node->children.node[i]->local_rank == i );
        node->children.node[i]->father = node;
    }
    
    node->mvp = node->children.node[0]->mvp;

    if( !node->global_rank ) return;

    sctk_assert( me->fathers_array_size == root[first_idx].fathers_array_size -1 );
    sctk_assert( me->fathers_array );

    for( i = 0; i < me->fathers_array_size; i++ )
        me->fathers_array[i] = root[first_idx].fathers_array[i];
}  

static void inline
__mpcomp_update_node_children_mvp_ptr(  const int first_idx, 
                                        mpcomp_node_t* node, 
                                        mpcomp_meta_tree_node_t* root )
{
    int i;

    mpcomp_meta_tree_node_t* me;

    sctk_assert( node );
    sctk_assert( first_idx > 0 );

    sctk_assert( root );
    assert( node->child_type == MPCOMP_CHILDREN_LEAF );

    const int children_num = node->nb_children;
    me = &(root[node->global_rank]);
    assert(sctk_atomics_load_int(&(me->children_ready)) == children_num);

    for( i = 0; i < children_num; i++ ) 
    { 
        const int global_child_idx = first_idx + i; 
        mpcomp_meta_tree_node_t* child = &( root[global_child_idx] );
        node->children.leaf[i] = (mpcomp_node_t*) child->user_pointer;
        sctk_assert( node->children.leaf[i]->local_rank == i );
        node->children.leaf[i]->father = node;
    }
    
    node->mvp = node->children.leaf[0];

    sctk_assert( me->fathers_array_size == root[first_idx].fathers_array_size -1 );
    sctk_assert( me->fathers_array );

    for( i = 0; i < me->fathers_array_size; i++ )
        me->fathers_array[i] = root[first_idx].fathers_array[i];
}  

#endif /* __MPCOMP_TREE_ARRAY_UTILS_H__ */
