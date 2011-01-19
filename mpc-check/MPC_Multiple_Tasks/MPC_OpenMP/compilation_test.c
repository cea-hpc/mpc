#include <omp.h>
#include <omp_abi.h>

int main() {
#if _OPENMP
  return 0 ;
#else
  return 1 ;
#endif
}
