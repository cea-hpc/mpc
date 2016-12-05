#ifndef __MPCOMP_INTEL_WRAPPER_H__
#define __MPCOMP_INTEL_WRAPPER_H__

#include "mpcomp_intel_types.h"

extern int __kmp_invoke_microtask(kmpc_micro pkfn, int gtid, int npr, int argc,
                                  void *argv[]);

typedef struct mpcomp_intel_wrapper_s {
  microtask_t f;
  int argc;
  void **args;
} mpcomp_intel_wrapper_t;

void mpcomp_intel_wrapper_func(void *);

#endif /* __MPCOMP_INTEL_WRAPPER_H__ */
