#include "mpcomp_macros.h"

#ifndef __MPCOMP_ALLOC_H__
#define __MPCOMP_ALLOC_H__

#include "sctk_alloc.h"

static inline void *
mpcomp_malloc(int numa_aware, int size, int node)
{
#ifdef MPCOMP_MALLOC_ON_NODE
	if (numa_aware)	
	{
   	return sctk_malloc_on_node( size, node );		
	}
#endif /* MPCOMP_MALLOC_ON_NODE */
	return sctk_malloc( size );			
}

static inline void 
mpcomp_free( void *p )
{
	sctk_free( p );			
}     
     
#endif /* __MPCOMP_ALLOC_H__ */

