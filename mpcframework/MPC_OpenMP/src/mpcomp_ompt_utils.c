#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include<execinfo.h>

void * __ompt_get_return_address(unsigned int level)
{
	const int real_level = level + 2;
	void** array = (void**) malloc(real_level*sizeof(void*));
	const int size = backtrace( array, real_level );
	void* retval = ( size == real_level ) ? array[real_level-1] : NULL;
	free(array);	
	return retval;
}
