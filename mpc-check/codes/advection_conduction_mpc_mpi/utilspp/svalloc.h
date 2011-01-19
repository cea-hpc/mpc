
/*
 * svalloc.h : quelques fonctions d'allocation mémoire.
 *
 */

#ifndef SEVY_SVALLOC_H
#define SEVY_SVALLOC_H


#include <assert.h>
#include <stdlib.h>
#if HAVE_ALLOCA
#include <alloca.h>
#endif

#define svfree(var) \
    do { \
	void* __svf_var = (void*) (var); \
        \
	assert((long) __svf_var); \
	free(__svf_var); \
    } while (0)

#define svtfree(var) \
    do { \
	void* __svf_var = (void*) (var); \
        \
	if (__svf_var)\
	    free(__svf_var); \
    } while (0)

#define svzfree(var, type) \
    do { \
	 svfree(var); \
         (var) = (type*) 0; \
    } while (0)

#define svalloc(var, type, thesz) \
    do { \
	size_t __size = (thesz); \
	\
	assert((long) __size); \
	(var) = (type*) malloc(__size * sizeof(type)); \
	assert((long)var); \
    } while (0)

#if HAVE_ALLOCA
#define svalloca(var, type, thesz) \
    do { \
	size_t __size = (thesz); \
	\
	assert((long)__size); \
	(var) = (type*) alloca(__size * sizeof(type)); \
	assert((long)var); \
    } while (0)
#endif

#define svcalloc(var, type, thesz) \
    do { \
	size_t __size = (thesz); \
	\
	assert((long)__size); \
	(var) = (type*) calloc(__size, sizeof(type));\
	assert((long)var); \
    } while (0)

#define svrealloc(var, type, thesz) \
    do { \
	size_t __size = (thesz); \
	\
	assert((long)__size); \
	if (var) { \
	    (var) = (type*) realloc((var), __size * sizeof(type)); \
	} else { \
	    svalloc(var, type, thesz); \
        } \
	assert((long)var); \
    } while (0)

#define svmayreallocoralloc(var, type, thesz, maxsz, thedft, theincrem) \
    do { \
	if (var) { \
	    svmayrealloc(var, type, thesz, maxsz, theincrem); \
	} else { \
	    svalloc(var, type, maxsz = thedft); \
	} \
    } while (0)

#define svmayrealloc(var, type, thesz, maxsz, theincrem) \
    do { \
	size_t __svmr_sz = (thesz); \
	\
        if (__svmr_sz >= (maxsz)) { \
	    size_t __svmr_diff = __svmr_sz - (maxsz); \
	    size_t __svmr_incr = (theincrem); \
	    size_t __svmr_nblk = ((__svmr_diff + __svmr_incr - 1) \
		/__svmr_incr); \
	    \
	    if (!__svmr_nblk) __svmr_nblk = 1; \
	    maxsz += __svmr_incr * __svmr_nblk; \
	    svrealloc(var, type, maxsz); \
	} \
    } while (0)

#endif
