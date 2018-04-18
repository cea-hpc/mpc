
#include "mpcomp_types_def.h"

#if( MPCOMP_TASK || defined( MPCOMP_OPENMP_3_0 ) )

#include "sctk_debug.h"
#include "sctk_atomics.h"
#include "mpcomp_types.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_macros.h"

static inline int* 
mpcomp_get_tree_array_ancestor_path( mpcomp_instance_t* instance, const int globalRank, int* depth )
{
    int* path;
    mpcomp_generic_node_t* gen_node;

    sctk_assert( instance );
    sctk_assert( depth );
    sctk_assert(globalRank >= 0 && globalRank < instance->tree_array_size);
    
    sctk_assert( instance->tree_array );
    gen_node = &( instance->tree_array[globalRank] );

    /* Extra node to preserve regular tree_shape */
    if( gen_node->type == MPCOMP_CHILDREN_NULL )
        return NULL;

    if( gen_node->type == MPCOMP_CHILDREN_LEAF )
    {
        path = gen_node->ptr.mvp->threads->tree_array_ancestor_path;
        sctk_assert( gen_node->ptr.mvp->threads );
        *depth = gen_node->ptr.mvp->threads->father_node->depth + 1;
    }
    else
    {
        sctk_assert( gen_node->type == MPCOMP_CHILDREN_NODE );
        path =  gen_node->ptr.node->tree_array_ancestor_path;
        *depth = gen_node->ptr.node->depth;
    }

    return path;
}

/* Return the ith neighbour of the element at rank 'globalRank' in the
 * tree_array */
int mpcomp_get_neighbour( const int globalRank, const int index ) 
{
    int i, currentDepth;
    int *path, *treeShape, *treeNumNodePerDepth;
    mpcomp_thread_t* thread;
    mpcomp_instance_t* instance;
    mpcomp_generic_node_t* gen_node;

    /* Retrieve the current thread information */
    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *) sctk_openmp_thread_tls;

    sctk_assert( thread->instance );
    instance = thread->instance;

    /* Shift ancestor array 0-depth -> 1-depth : Root is depth 0 */
    path = mpcomp_get_tree_array_ancestor_path( instance, globalRank, &currentDepth ) +1; 
    sctk_assert( path && currentDepth > 0 );

    treeShape = instance->tree_base + 1;
    sctk_assert( treeShape );
    treeNumNodePerDepth = instance->tree_nb_nodes_per_depth;
    sctk_assert( treeNumNodePerDepth );

#if 0 /* DEBUG PRINT */
    char __treeShape_string[256], __treeNumNodePerDepth_string[256], __path_string[256];
    int _a, _b, _c, _tota, _totb, _totc;

//    currentDepth++;

    for( i = 0, _tota = 0, _totb = 0, _totc = 0; i < currentDepth; i++ )  
    {
       _a = snprintf( __treeShape_string+_tota, 256-_tota, " %d", treeShape[currentDepth - i -1] );
       _b = snprintf( __treeNumNodePerDepth_string+_totb, 256-_totb, " %d", treeNumNodePerDepth[i] );
       _c = snprintf( __path_string+_totc, 256-_totc, " %d", path[currentDepth - i -1] );
       _tota += _a; _totb += _b; _totc += _c;
    }

    fprintf(stderr, "%s:%d << %d >> treebase: %s nodePerDepth: %s, path : %s\n", __func__, __LINE__, thread->tree_array_rank, __treeShape_string, __treeNumNodePerDepth_string, __path_string );
#endif /* DEBUG PRINT */

    int r = index;
    int id = 0;
    int firstRank = 0;
    int nbSubleaves = 1;
    int v[currentDepth], res[currentDepth];
    
    for( i = 0; i < currentDepth; i++ ) 
    {
        sctk_assert( currentDepth - 1 - i >= 0 ); 
        sctk_assert( currentDepth - 1 - i < thread->father_node->depth + 1 );
        sctk_assert( i <= thread->father_node->depth + 1 );

        const int base = treeShape[currentDepth - i -1];
        const int level_size = treeNumNodePerDepth[i];
        
        /* Somme de I avec le codage de 0 dans le vecteur de l'arbre */
        v[i] = r % base;
        r /= base;

        /* Addition sans retenue avec le vecteur de n */
        res[i] = (path[currentDepth -1 - i] + v[i]) % base;

        /* Calcul de l'identifiant du voisin dans l'arbre */
        id += res[i] * nbSubleaves;
        nbSubleaves *= base;
        firstRank += level_size;
    }

    return id + firstRank;
}

/* Return the ancestor of element at rank 'globalRank' at depth 'depth' */
int mpcomp_get_ancestor( const int globalRank, const int depth ) 
{
    int i, currentDepth;
    int *path, *treeShape, *treeNumNodePerDepth;
    mpcomp_thread_t* thread;
    mpcomp_instance_t* instance;

    /* If it's the root, ancestor is itself */
    if( !globalRank ) return 0;

    /* Retrieve the current thread information */
    sctk_assert(sctk_openmp_thread_tls);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

    sctk_assert( thread->instance );
    instance = thread->instance;

    path = mpcomp_get_tree_array_ancestor_path( instance, globalRank, &currentDepth ); 
    sctk_assert( path && currentDepth > 0 );

    if (currentDepth == depth) return globalRank;
    sctk_assert(depth >= 0 && depth < currentDepth);

    treeShape = instance->tree_base;
    sctk_assert( treeShape );
    treeNumNodePerDepth = instance->tree_nb_nodes_per_depth;
    sctk_assert( treeNumNodePerDepth );

    int ancestor_id = 0;
    int firstRank = 0;
    int nbSubLeaves = 1;

    for (i = 0; i < depth; i++) 
    {
        ancestor_id += path[depth - 1 - i] * nbSubLeaves;
        nbSubLeaves *= treeShape[depth - 1 - i];
        firstRank += treeNumNodePerDepth[i];
    }

    return firstRank + ancestor_id;
}

/* Recursive call for checking neighbourhood from node n */
static void __mpcomp_task_check_neighbourhood_r( mpcomp_node_t *node ) 
{
    int i, j;
    mpcomp_thread_t *thread;
    mpcomp_instance_t* instance;

    sctk_assert( node );

    /* Retrieve the current thread information */
    sctk_assert(sctk_openmp_thread_tls);
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

    sctk_assert( thread->instance );
    sctk_assert( thread->instance->tree_nb_nodes_per_depth );
    
    
    const int instance_level_size = instance->tree_nb_nodes_per_depth[node->depth + 1];
    
    switch (node->child_type) 
    {
        case MPCOMP_CHILDREN_NODE:
            /* Call recursively for all children nodes */
            for ( i = 0; i < node->nb_children; i++ ) 
                __mpcomp_task_check_neighbourhood_r(node->children.node[i]);
            break;

        case MPCOMP_CHILDREN_LEAF:
            /* All the children are leafs */
            for( i = 0; i < node->nb_children; i++) 
            {
                mpcomp_mvp_t *mvp = node->children.leaf[i];
                sctk_assert( mvp && mvp->threads );
            }
            break;

        default:
            sctk_nodebug("not reachable");
    }
}

/* Check all neighbour of all nodes */
void __mpcomp_task_check_neighbourhood(void) 
{
    mpcomp_thread_t *thread;

    /* Retrieve the current thread information */
    sctk_assert(sctk_openmp_thread_tls);
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
    
    sctk_assert( thread->instance ); 
    __mpcomp_task_check_neighbourhood_r( thread->instance->root );
}

#endif /* MPCOMP_TASK */
