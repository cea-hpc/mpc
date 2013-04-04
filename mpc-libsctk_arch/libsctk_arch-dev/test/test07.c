#include <libarch.h>
#include <libpause.h>
#include <libtimer.h>
#include <sctk_config_arch.h>
#include <stdio.h>
#include <assert.h>

#define NB_ITER 10000

int main(int argc, char** argv){
#ifdef SCTK_i686_ARCH_SCTK
  fprintf(stderr,"Arch I686\n");
#elif defined(SCTK_x86_64_ARCH_SCTK)
  fprintf(stderr,"Arch x86_64\n");
#elif defined(SCTK_ia64_ARCH_SCTK)
  fprintf(stderr,"Arch IA64\n");
#else
  #error "Unknown architecture"
#endif

  fprintf(stderr,"Test libpause\n");
  sctk_cpu_relax ();
  fprintf(stderr,"Test libpause DONE\n");

  fprintf(stderr,"Test libtimer\n");
  {
    double start; 
    double end;
    int i; 
    sctk_atomics_cpu_freq_init();
    fprintf(stderr,"\tCPU freq %4.2fGHz\n",sctk_atomics_get_cpu_freq()/1000000000.0);
    start = sctk_atomics_get_timestamp_tsc ();
    for(i = 0; i < NB_ITER; i++){
      sctk_cpu_relax ();
    }
    end = sctk_atomics_get_timestamp_tsc ();
    fprintf(stderr,"\tCpu_relax duration %e\n",(end-start)/NB_ITER);
  }
  fprintf(stderr,"Test libtimer DONE\n");

  fprintf(stderr,"Test config\n");
  fprintf(stderr,"Function %s\n",SCTK_FUNCTION);
  fprintf(stderr,"STRING %s\n",SCTK_STRING(TEST));
  fprintf(stderr,"COMPILER %s\n",SCTK_COMPILER_STRING);
  fprintf(stderr,"OS %s\n",SCTK_OS_STRING);

  fprintf(stderr,"sctk_sizeof_char %d\n",sctk_sizeof_char);assert(sctk_sizeof_char != 0);
  fprintf(stderr,"sctk_sizeof_short %d\n",sctk_sizeof_short);assert(sctk_sizeof_short != 0);
  fprintf(stderr,"sctk_sizeof_int %d\n",sctk_sizeof_int);assert(sctk_sizeof_int != 0);
  fprintf(stderr,"sctk_sizeof_float %d\n",sctk_sizeof_float);assert(sctk_sizeof_float != 0);
  fprintf(stderr,"sctk_sizeof_long %d\n",sctk_sizeof_long);assert(sctk_sizeof_long != 0);
  fprintf(stderr,"sctk_sizeof_double %d\n",sctk_sizeof_double);assert(sctk_sizeof_double != 0);
  fprintf(stderr,"sctk_sizeof_size_t %d\n",sctk_sizeof_size_t);assert(sctk_sizeof_size_t != 0);
  fprintf(stderr,"sctk_sizeof_void_p %d\n",sctk_sizeof_void_p);assert(sctk_sizeof_void_p != 0);
  fprintf(stderr,"sctk_sizeof_long_long %d\n",sctk_sizeof_long_long);assert(sctk_sizeof_long_long != 0);
  fprintf(stderr,"Wordsize %d\n",SCTK_WORD_SIZE);

  fprintf(stderr,"Test config DONE\n");
  return 0;
}
