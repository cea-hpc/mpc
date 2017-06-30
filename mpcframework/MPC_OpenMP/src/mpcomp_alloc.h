#ifndef __MPCOMP_ALLOC_H__
#define __MPCOMP_ALLOC_H__

#include "sctk_alloc.h"

static inline void* mpcomp_alloc( int size ) 
{
  return sctk_malloc(size);
}

static inline void mpcomp_free( void *p ) 
{ 
    sctk_free(p); 
}

#endif /* __MPCOMP_ALLOC_H__ */

