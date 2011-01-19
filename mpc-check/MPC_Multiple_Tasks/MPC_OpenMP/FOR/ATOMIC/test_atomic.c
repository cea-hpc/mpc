#include <omp.h>
#include <omp_abi.h>

#define MIN_THREADS 1
#define MAX_THREADS 8 
#define INC_THREADS 1

#define MIN_VAL 1
#define MAX_VAL 5 
#define INC_VAL 1

#define MIN_ITER 1
#define MAX_ITER 20
#define INC_ITER 1

struct shared_s {
  int * res ;
  int val ;
  int iter ;
} ;

void * omp_f( void * shared ) {
  struct shared_s * s = (struct shared_s *) shared ;
  int * res = s->res ;
  int val = s->val ;
  int iter = s->iter ;

  int i ;

  for ( i = 0 ; i < iter ; i++ ) {
    __mpcomp_atomic_begin() ;
    *res += val ;
    __mpcomp_atomic_end() ;
  }
}

void run_omp( int * res, int n_threads, int val, int iter ) {
  struct shared_s s = { res, val, iter } ;
  __mpcomp_start_parallel_region( n_threads, omp_f, &s ) ;
}

int test_run_omp( int res, int n_threads, int val, int iter ) {
  int expected_res = 0 ;
  int i, j ;

  for ( i = 0 ; i < n_threads ; i++ ) {
    for ( j = 0 ; j < iter ; j++ ) {
      expected_res += val ;
    }
  }

  if ( expected_res == res ) {
    return 0 ;
  }

  return 1 ;
}

int main( int argc, char ** argv ) {
  int n_threads ;
  int val ;
  int iter ;
  int res ;
  long total_xps = 0 ;

  /* For all number of threads */
  for ( n_threads = MIN_THREADS ; n_threads <= MAX_THREADS ; n_threads += INC_THREADS ) {
    for ( val = MIN_VAL ; val <= MAX_VAL ; val += INC_VAL ) {
      for ( iter = MIN_ITER ; iter <= MAX_ITER ; iter += INC_ITER ) {
	res = 0 ;

	total_xps++ ;

	/* Run the corresponding test */
	run_omp( &res, n_threads, val, iter ) ;

	/* Check if the test failed */
	if ( test_run_omp( res, n_threads, val, iter ) ) {
	  exit( 1 ) ;
	}

	/* sctk_debug( "Experiment %d ok!", total_xps ) ; */
      }
    }
  }

  sctk_debug( "Success after %ld experiment(s)!", total_xps ) ;


}
