/**
  Check a for loop with static schedule built as if a runtime schedule was used
  */
#include <omp.h>
#include <omp_abi.h>

#define MIN_THREADS 2
#define MAX_THREADS 8 
#define INC_THREADS 2

#define MIN_LB 0 
#define MAX_LB 5
#define INC_LB 1

#define MIN_B 0 
#define MAX_B 10000
#define INC_B 4500

#define MIN_STEP 1
#define MAX_STEP 10
#define INC_STEP 1

#define MIN_CHUNK_SIZE 1
#define MAX_CHUNK_SIZE 15
#define INC_CHUNK_SIZE 4

#define ITERATION_RECORD_DEFAULT (0)


struct shared_s {
  int lb ;
  int b ;
  int step ;
  int chunk_size ;
  int ** iteration_log ;
} ;

void * omp_f( void * shared ) {
  struct shared_s * s = (struct shared_s *) shared ;
  int lb = s->lb ;
  int b = s->b ;
  int step = s->step ;
  int chunk_size = s->chunk_size ;
  int ** iteration_log = s-> iteration_log ;

  int from ;
  int to ;
  int i ;
  int c_i ;

  if ( __mpcomp_static_loop_begin( lb, b, step, chunk_size, &from, &to ) ) {
    do {
      for ( i = from ; i < to ; i += step ) {
	iteration_log[ omp_get_thread_num() ][ i ]++ ;
      }
    } while ( __mpcomp_static_loop_next( &from, &to ) ) ;
  }
  __mpcomp_static_loop_end() ;

  return NULL ;
}

void run_omp( int n_threads, int lb, int b, int step, int chunk_size, int ** iteration_log ) {

  /*
#pragma omp parallel num_threads(n_threads)
  {
#pragma omp for schedule(static,chunk_size)
    for ( i = lb ; i < b ; i += step ) {
      iteration_rec[ i ] = omp_get_thread_num() ;
    }
  }
  */

  struct shared_s s = { lb, b, step, chunk_size, iteration_log } ;

  __mpcomp_start_parallel_region( n_threads, omp_f, &s ) ;
}

int test_run_omp( int n_threads, int lb, int b, int step, int chunk_size, int ** iteration_log ) {
  int i, j ;

  /* Step 1: check that every iteration has been executed once */
  for ( i = lb ; i < b ; i += step ) {
    int found = -1 ;

    for ( j = 0 ; j < n_threads ; j++ ) {

      if ( iteration_log[j][i] > 0 ) {
	if ( found != -1 ) {
	  sctk_debug( "Error: iteration %d has been scheduled by threads %d and %d\n"
	      , i, found, j ) ;
	  return 1 ;

	} else {
	  if ( iteration_log[j][i] > 1 ) {
	    sctk_debug( "Error: Thread %d has scheduled several times iteration %d\n"
		, j, i ) ;
	    return 1 ;
	  }

	  found = j ;
	}
      }

    }
    if ( found == -1 ) {
      sctk_debug( "Error: iteration %d has not been scheduled", i ) ;
      return 1 ;
    }
  }

  /* Step 2: check that no other iteration has been executed */
  for ( i = MIN_LB ; i < MAX_B ; i ++ ) {
    /*
    printf( "Testing i=%d, lb=%d, b=%d, (i-lb)=%d and (i-lb)%%step=%d\n"
	, i, lb, b, i-lb, (i-lb)%step ) ;
	*/
    if ( ( (i==lb) || ( i > lb && i < b ) ) && ( ((i-lb)%step )==0) ) {
    } else {
      for ( j = 0 ; j < n_threads ; j++ ) {
	if ( iteration_log[j][i] > 0 ) {
	  sctk_debug( "Error: thread %d has scheduled an useless iteration %d\n"
	      , j, i ) ;
	  sctk_debug( "\ti=%d, lb=%d, b=%d, (i-lb)=%d and (i-lb)%%step=%d\n"
	      , i, lb, b, i-lb, (i-lb)%step ) ;
	  return 1 ;
	}
      }
    }
  }

  /* Step 3: check that every threads executes chunks of correct size */
  /* TODO */

  return 0 ;
}

int main( int argc, char ** argv ) {
  int n_threads, n_iterations, b, lb, step, i, j ;
  int chunk_size ;
  int ** iteration_log ; 

  iteration_log = (int **)calloc( MAX_THREADS, sizeof( int * ) ) ;
  for ( i = 0 ; i < MAX_THREADS ; i++ ) {
    iteration_log[i] = (int *)calloc( MAX_B-MIN_LB, sizeof( int ) ) ;
  }


  /* For all number of threads */
  for ( n_threads = MIN_THREADS ; n_threads < MAX_THREADS ; n_threads += INC_THREADS ) {
    /* For all bounds (range of the for loop */
    for ( lb = MIN_LB ; lb < MAX_LB ; lb += INC_LB ) {
      for ( b = MIN_B ; b < MAX_B ; b += INC_B ) {
	/* For all steps */
	for ( step = MIN_STEP ; step < MAX_STEP ; step += INC_STEP ) {

	  for ( chunk_size = MIN_CHUNK_SIZE ; chunk_size < MAX_CHUNK_SIZE ; 
	      chunk_size += INC_CHUNK_SIZE ) {

	    /* Clean the log */
	    for ( i = 0 ; i < MAX_THREADS ; i++ ) {
	      for ( j = 0 ; j < (MAX_B-MIN_LB) ; j++ ) {
		iteration_log[i][j] = ITERATION_RECORD_DEFAULT ;
	      }
	    }


	    n_iterations = (b - lb) / step ;

	    sctk_debug( "Running -> %d thread(s) from %d to %d step %d => %d it w/ chunk %d\n"
		, n_threads, lb, b, step, n_iterations, chunk_size ) ;

	    /* Run the corresponding test */
	    run_omp( n_threads, lb, b, step, chunk_size, iteration_log ) ;

	    /* Check if the test failed */
	    if ( test_run_omp( n_threads, lb, b, step, chunk_size, iteration_log ) ) {
	      exit( 1 ) ;
	    }

	  }

	}
      }
    }
  }

  for ( i = 0 ; i < MAX_THREADS ; i++ ) {
    free( iteration_log[i] ) ;
  }
  free( iteration_log ) ;


  return 0 ;
}
