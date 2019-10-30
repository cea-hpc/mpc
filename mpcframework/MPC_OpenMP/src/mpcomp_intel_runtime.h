
#ifndef __MPCOMP_INTEL_RUNTIME_H__
#define __MPCOMP_INTEL_RUNTIME_H__

#include "mpcomp_intel_types.h"

void __kmpc_begin(ident_t *loc, kmp_int32 flags);
void __kmpc_end(ident_t *loc);

#endif /* __MPCOMP_INTEL_RUNTIME_H__ */
