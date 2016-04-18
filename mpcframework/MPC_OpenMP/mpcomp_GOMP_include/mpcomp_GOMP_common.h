#ifndef __MPCOMP_GOMP_COMMON_H__
#define __MPCOMP_GOMP_COMMON_H__

#include "mpcomp_GOMP_versionning.h"

#define MPCOMP_GOMP_UNUSED_VAR(var) (void)(sizeof(var))

#define MPCOMP_GOMP_NO_ALIAS ""
#define MPCOMP_GOMP_EMPTY_FN ""
 
 
#define GOMP_ABI_FUNC(gomp_name,version, wrapper, mpcomp_name) \
	versionify(wrapper, gomp_name,version)
#include "mpcomp_GOMP_version.h"
#undef GOMP_ABI_FUNC

#endif /* __MPCOMP_GOMP_COMMON_H__ */
