#ifndef __MPCOMP_TREE_ARRAY_H__
#define __MPCOMP_TREE_ARRAY_H__

typedef enum 
{ 
    MPCOMP_META_TREE_UNDEF      = 0, 
    MPCOMP_META_TREE_NODE       = 1, 
    MPCOMP_META_TREE_LEAF       = 2,
    MPCOMP_META_TREE_NONE       = 3
} mpcomp_meta_tree_type_t;


typedef struct mpcomp_tree_array_global_info_s
{
    int* tree_shape;
    int max_depth;
} mpcomp_tree_array_global_info_t;

typedef struct mpcomp_meta_tree_node_s
{ 
    /* Generic information */
    unsigned int rank;
    unsigned int depth;
    unsigned int numa_id;
    unsigned int local_rank;
    unsigned int stage_rank;
    mpcomp_meta_tree_type_t type;
    /* Father information */
    unsigned int* fathers_array;
    unsigned int fathers_array_size;
    /* Children information */
    unsigned int* first_child_array;
    unsigned int* children_num_array;
    unsigned int children_array_size;
    /* Min index */
    int * min_index;
    /* User infos */
    void* user_pointer;
    sctk_atomics_int children_ready;
    sctk_atomics_int user_init_step;
} mpcomp_meta_tree_node_t; 

typedef struct mpcomp_mvp_thread_args_s
{
    void* thread;
    void* root_node;        /* prevent inondation initialisation */
    unsigned int rank;
    unsigned int max_depth;
    unsigned int* tree_shape;
    unsigned int mvp_global_rank;
    unsigned int numa_id;
    int core_depth;
    mpcomp_meta_tree_node_t* array;
    void* options;
} mpcomp_mvp_thread_args_t;

// Initialize Ghost OpenMP Tree Constraint Homogeneous Array 
mpcomp_meta_tree_node_t**  __mpcomp_allocate_tree_array( const int*, const int, int* );

static inline mpcomp_meta_tree_type_t
__mpcomp_tree_array_node_type_by_depth( mpcomp_meta_tree_node_t** root )
{
    return MPCOMP_META_TREE_UNDEF;
}

static inline mpcomp_meta_tree_type_t
__mpcomp_tree_array_node_type_from_node_by_depth( mpcomp_meta_tree_node_t* node, const int depth )
{
    // No more children at distance depth or node is already a leaf
    if( depth >= node->children_array_size || node->type == MPCOMP_META_TREE_LEAF )
        return MPCOMP_META_TREE_NONE;

    if( depth == node->children_array_size -1) 
        return MPCOMP_META_TREE_LEAF;

    return MPCOMP_META_TREE_NODE;
}


#endif /* __MPCOMP_TREE_ARRAY_H__ */
