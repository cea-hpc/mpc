#ifndef __MPCOMP_GOMP_WRAPPER_H__
#define __MPCOMP_GOMP_WRAPPER_H__

#include "sctk_debug.h"

/** 
 *
 */
typedef struct mpcomp_GOMP_wrapper_s
{
   void(*fn)(void*);
   void* data;
} mpcomp_GOMP_wrapper_t;

/**
 * \fn void* mpcomp_GOMP_wrapper_fn(void* args)
 * \brief Transform void(*fn)(void*) GOMP args to void*(*fn)(void*) \
 * unpack mpcomp_gomp_wrapper_t.
 * \param arg opaque type
 */
static inline void *mpcomp_GOMP_wrapper_fn(void* args)
{
   mpcomp_GOMP_wrapper_t* w = (mpcomp_GOMP_wrapper_t*) args;
   sctk_assert(w);
   sctk_assert(w->fn); 
   w->fn( w->data );
   return NULL;
}

#endif /* __MPCOMP_GOMP_WRAPPER_H__ */
