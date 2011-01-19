#include <omp.h>
#include <omp_abi.h>
#include <stdio.h>

void * omp_f( void * s ) {
  int f, t ;
  int i ;


  __mpcomp_static_schedule_get_single_chunk( 0, 100, 1, &f, &t ) ;


  for( i = f ; i < t ; i++ ) {
    printf( "Hello World from %d\n", omp_get_thread_num()  ) ;
  }

  return NULL ;
}

int main() {

  __mpcomp_start_parallel_region( -1, omp_f, NULL ) ;

  return 0 ;
}
