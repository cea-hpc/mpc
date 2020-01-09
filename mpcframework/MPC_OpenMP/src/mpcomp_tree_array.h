#ifndef __MPCOMP_TREE_ARRAY_H__
#define __MPCOMP_TREE_ARRAY_H__

typedef enum 
{ 
    MPCOMP_META_TREE_UNDEF      = 0, 
    MPCOMP_META_TREE_NODE       = 1, 
    MPCOMP_META_TREE_LEAF       = 2,
    MPCOMP_META_TREE_NONE       = 3
} mpcomp_meta_tree_type_t;


#if 0
typedef struct mpcomp_tree_array_global_info_s
{
    int* tree_shape;
    int max_depth;
} mpcomp_tree_array_global_info_t;
#endif

typedef struct mpcomp_meta_tree_node_s
{ 
    /* Generic information */
    mpcomp_meta_tree_type_t type;
    /* Father information */
    int* fathers_array;
    unsigned int fathers_array_size;
    /* Children information */
    int* first_child_array;
    unsigned int* children_num_array;
    unsigned int children_array_size;

    /* Places */

    /* Min index */
    void* user_pointer;
    sctk_atomics_int children_ready;
} mpcomp_meta_tree_node_t; 

typedef struct mpcomp_mvp_thread_args_s
{
    unsigned int rank;
    mpcomp_meta_tree_node_t* array;
    unsigned int place_id;
    unsigned int target_vp;
    unsigned int place_depth;
} mpcomp_mvp_thread_args_t;

// Initialize Ghost OpenMP Tree Constraint Homogeneous Array 
mpcomp_meta_tree_node_t**  __mpcomp_allocate_tree_array( const int*, const int, int* );

static inline mpcomp_meta_tree_type_t
__mpcomp_tree_array_node_type_by_depth( __UNUSED__ mpcomp_meta_tree_node_t** root )
{
    return MPCOMP_META_TREE_UNDEF;
}

static inline mpcomp_meta_tree_type_t
__mpcomp_tree_array_node_type_from_node_by_depth( mpcomp_meta_tree_node_t* node, const unsigned int depth )
{
    // No more children at distance depth or node is already a leaf
    if( depth >= node->children_array_size || node->type == MPCOMP_META_TREE_LEAF )
        return MPCOMP_META_TREE_NONE;

    if( depth == node->children_array_size -1) 
        return MPCOMP_META_TREE_LEAF;

    return MPCOMP_META_TREE_NODE;
}

void
__mpcomp_alloc_openmp_tree_struct( int* shape, int max_depth, const int* cpus_order, const int place_depth, const int place_size, const mpcomp_local_icv_t icvs );

int*
__mpcomp_tree_array_compute_thread_openmp_min_rank( const int* shape, const int max_depth, const int rank, const int core_depth );

#endif /* __MPCOMP_TREE_ARRAY_H__ */
