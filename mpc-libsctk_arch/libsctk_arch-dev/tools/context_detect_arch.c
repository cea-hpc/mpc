
/*
  copy this file in configure.in to modify architecture detection
*/

#include <stdio.h>

#if defined(__i686__) && !defined(__x86_64__)
#define SCTK_i686_ARCH_SCTK
#elif defined(__x86_64__)
#define SCTK_x86_64_ARCH_SCTK
#elif defined(__ia64__)
#define SCTK_ia64_ARCH_SCTK
#else
  #error "Unknown architecture"
#endif

#if defined(SCTK_i686_ARCH_SCTK) && !defined(CONTEXT_GEN)
#error "Wrong context detection"
#endif

#if defined(SCTK_x86_64_ARCH_SCTK) && !defined(CONTEXT_X86_64)
#error "Wrong context detection"
#endif

#if defined(SCTK_ia64_ARCH_SCTK) && !defined(CONTEXT_GEN)
#error "Wrong context detection"
#endif

int main(){
#ifdef SCTK_i686_ARCH_SCTK
  fprintf(stderr,"Arch I686\n");
#elif defined(SCTK_x86_64_ARCH_SCTK)
  fprintf(stderr,"Arch x86_64\n");
#elif defined(SCTK_ia64_ARCH_SCTK)
  fprintf(stderr,"Arch IA64\n");
#else
  #error "Unknown architecture"
#endif
  return 0;
}
