#ifndef __MPCOMP_SPINNING_CORE_H__
#define __MPCOMP_SPINNING_CORE_H__

#include "mpcomp_types.h"

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

#endif /* __MPCOMP_SPINNING_CORE_H__ */
