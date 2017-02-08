#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include<execinfo.h>

#ifdef __INTEL_COMPILER
static void * __ompt_get_return_address(unsigned int level)
{
	const int real_level = level + 3;
	void** array = (void**) malloc(real_level*sizeof(void*));
	const int size = backtrace( array, real_level );
	void* retval = ( size == real_level ) ? array[real_level-1] : NULL;
	free(array);	
	return retval;
}
#endif /* __INTEL_COMPILER */

void* __mpcomp_ompt_generic_get_return_address(unsigned int level)
{
	void* codeptr_ra = NULL;
#ifdef __INTEL_COMPILER
	codeptr_ra = __ompt_get_return_address(level);
#else /* __INTEL_COMPILER */
	codeptr_ra = __builtin_return_address(0);
#endif /* __INTEL_COMPILER */
	return codeptr_ra;
}
