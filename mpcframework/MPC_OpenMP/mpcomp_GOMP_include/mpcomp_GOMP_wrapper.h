#ifndef __MPCOMP_GOMP_WRAPPER_H__
#define __MPCOMP_GOMP_WRAPPER_H__

/** 
 *
 */
typedef struct __mpcomp_GOMP_wrapper_s
{
   void(*fn)(void*);
   void* data;
} __mpcomp_GOMP_wrapper_t;

/**
 * \fn void* __mpcomp_GOMP_wrapper_fn(void* args)
 * \brief Transform void(*fn)(void*) GOMP args to void*(*fn)(void*) \
 * unpack __mpcomp_gomp_wrapper_t.
 * \param arg opaque type
 */
static inline void *__mpcomp_GOMP_wrapper_fn(void* args)
{
   __mpcomp_GOMP_wrapper_t* w = (__mpcomp_GOMP_wrapper_t*) args;
   sctk_assert(w);
   sctk_assert(w->fn); 
   w->fn( w->data );
   return NULL;
}

#endif /* __MPCOMP_GOMP_WRAPPER_H__ */
