/*
   This test check the execution of several shared for loops with a 'nowait'
   clause and a specific chunk size.
 */

#include <omp.h>
#include <omp_abi.h>

#if 1
/* Number of threads to share the for-loop execution */
#define MIN_THREADS 1
#define MAX_THREADS 8 
#define INC_THREADS 1

/* Lower bound of the for loop */
#define MIN_LB 0 
#define MAX_LB 2
#define INC_LB 1

/* Upper bound of the for loop */
#define MIN_B 1000 
#define MAX_B 2001
#define INC_B 1000

/* Increment of the for loop */
#define MIN_STEP 1
#define MAX_STEP 2
#define INC_STEP 4

/* Chunk size */
#define MIN_CHUNK_SIZE 1
#define MAX_CHUNK_SIZE 11
#define INC_CHUNK_SIZE 4

/* Number of loops to execute in a row (w/ nowait clause) */
#define MIN_NOWAIT_LOOPS 2
#define MAX_NOWAIT_LOOPS 5 
#define INC_NOWAIT_LOOPS 1
#else
#define MIN_THREADS 30 
#define MAX_THREADS 31 
#define INC_THREADS 1

#define MIN_LB 0 
#define MAX_LB 1
#define INC_LB 1

#define MIN_B 2000 
#define MAX_B 2001
#define INC_B 1000

#define MIN_STEP 1
#define MAX_STEP 2
#define INC_STEP 4

#define MIN_CHUNK_SIZE 10
#define MAX_CHUNK_SIZE 11
#define INC_CHUNK_SIZE 4

#define MIN_NOWAIT_LOOPS 4
#define MAX_NOWAIT_LOOPS 5 
#define INC_NOWAIT_LOOPS 1

#endif

#define ITERATION_RECORD_DEFAULT (0)


struct shared_s {
  int now ;
  int lb ;
  int b ;
  int step ;
  int chunk_size ;
  int *** iteration_log ;
} ;

void * omp_f( void * shared ) {
  struct shared_s * s = (struct shared_s *) shared ;
  int now = s->now ;
  int lb = s->lb ;
  int b = s->b ;
  int step = s->step ;
  int chunk_size = s->chunk_size ;
  int *** iteration_log = s-> iteration_log ;

  int from ;
  int to ;
  int i ;
  int c_i ;
  int n ;

  for ( n = 0 ; n < now ; n++ ) {
    if ( __mpcomp_dynamic_loop_begin( lb, b, step, chunk_size, &from, &to ) ) {
      do {
	for ( i = from ; i < to ; i += step ) {
	  iteration_log[n][ omp_get_thread_num() ][ i ]++ ;
	}
      } while( __mpcomp_dynamic_loop_next( &from, &to ) ) ;
    }
    __mpcomp_dynamic_loop_end_nowait() ;
  }

  return NULL ;
}

void run_omp( int n_threads, int now, int lb, int b, int step, int chunk_size,
    int *** iteration_log ) {

  /*
#pragma omp parallel num_threads(n_threads)
  {
    for ( n = 0 ; n < now ; n++ ) {
#pragma omp for schedule(dynamic,chunk_size) nowait
      for ( i = lb ; i < b ; i += step ) {
	iteration_log[n][omp_get_thread_num()][i]++ ;
      }
    }
  }
  */

  struct shared_s s = { now, lb, b, step, chunk_size, iteration_log } ;

  __mpcomp_start_parallel_region( n_threads, omp_f, &s ) ;
}

int test_run_omp( int n_threads, int now, int lb, int b, int step, int
    chunk_size, int *** iteration_log ) {
  int i, j ;
  int n ;

  /* Step 1: check that every iteration of every loop has been executed once */
  for ( n = 0 ; n < now ; n++ ) {
    for ( i = lb ; i < b ; i += step ) {
      int found = -1 ;

      for ( j = 0 ; j < n_threads ; j++ ) {

	if ( iteration_log[n][j][i] > 0 ) {
	  if ( found != -1 ) {
	    sctk_debug( 
		"Error: iteration %d of loop %d has been scheduled by threads %d and %d\n"
		, i, n, found, j ) ;
	    return 1 ;

	  } else {
	    if ( iteration_log[n][j][i] > 1 ) {
	      sctk_debug( 
		  "Error: Thread %d has scheduled several %d times iteration %d of loop %d\n"
		  , j, iteration_log[n][j][i], i, n ) ;
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
  }

  /* Step 2: check that no other iteration of any loop has been executed */
  for ( n = 0 ; n < now ; n++ ) {
    for ( i = MIN_LB ; i < MAX_B ; i ++ ) {
      if ( ( (i==lb) || ( i > lb && i < b ) ) && ( ((i-lb)%step )==0) ) {
	/* Correct iteration */
      } else {
	/* Incorrect iteration */
	for ( j = 0 ; j < n_threads ; j++ ) {
	  if ( iteration_log[n][j][i] > 0 ) {
	    sctk_debug( 
		"Error: thread %d has scheduled an useless iteration %d of loop %d\n"
		, j, i, n ) ;
	    sctk_debug( "\ti=%d, lb=%d, b=%d, (i-lb)=%d and (i-lb)%%step=%d\n"
		, i, lb, b, i-lb, (i-lb)%step ) ;
	    return 1 ;
	  }
	}
      }
    }
  }

  /* Step 3: check that every threads executes chunks of correct size */
  /* TODO */

  return 0 ;
}

int main( int argc, char ** argv ) {
  int n_threads, n_iterations, now, b, lb, step, i, j, k ;
  int chunk_size ;
  int *** iteration_log ; 
  long total_xps = 0 ;

  iteration_log = (int ***)calloc( MAX_NOWAIT_LOOPS, sizeof( int ** ) ) ;
  for ( i = 0 ; i < MAX_NOWAIT_LOOPS ; i++ ) {
    iteration_log[i] = (int **)calloc( MAX_THREADS, sizeof( int * ) ) ;
    for ( j = 0 ; j < MAX_THREADS ; j++ ) {
      iteration_log[i][j] = (int *)calloc( MAX_B-MIN_LB, sizeof( int ) ) ;
    }
  }


  /* For all number of threads */
  for ( n_threads = MIN_THREADS ; n_threads < MAX_THREADS ; n_threads += INC_THREADS ) {
    /* For all the number of consecutive nowait */
    for ( now = MIN_NOWAIT_LOOPS ; now < MAX_NOWAIT_LOOPS ; now += INC_NOWAIT_LOOPS ) {
      /* For all bounds (range of the for loop */
      for ( lb = MIN_LB ; lb < MAX_LB ; lb += INC_LB ) {
	for ( b = MIN_B ; b < MAX_B ; b += INC_B ) {
	  /* For all steps */
	  for ( step = MIN_STEP ; step < MAX_STEP ; step += INC_STEP ) {

	    for ( chunk_size = MIN_CHUNK_SIZE ; chunk_size < MAX_CHUNK_SIZE ; 
		chunk_size += INC_CHUNK_SIZE ) {

	      /* Clean the log */
	      for ( i = 0 ; i < MAX_NOWAIT_LOOPS ; i++ ) {
		for ( j = 0 ; j < MAX_THREADS ; j++ ) {
		  for ( k = 0 ; k < (MAX_B-MIN_LB) ; k++ ) {
		    iteration_log[i][j][k] = ITERATION_RECORD_DEFAULT ;
		  }
		}
	      }


	      /* Compute the total number of iterations of the next for loops
	      */
	      n_iterations = (b - lb) / step ;

	      /* One more experiment */
	      total_xps++ ;

	      sctk_debug( 
		 "Running -> %d thread(s), %d loop(s), from %d to %d step %d"
		 " => %d it w/ chunk %d\n"
		  , n_threads, now, lb, b, step, n_iterations, chunk_size ) ;

	      /* Run the corresponding test */
	      run_omp( n_threads, now, lb, b, step, chunk_size, iteration_log ) ;

	      /* Check if the test failed */
	      if ( test_run_omp( n_threads, now, lb, b, step, chunk_size, iteration_log ) ) {
		exit( 1 ) ;
	      }

	    }

	  }
	}
      }
    }
  }

  sctk_debug( "Success after %ld experiment(s)!", total_xps ) ;

  for ( i = 0 ; i < MAX_NOWAIT_LOOPS ; i++ ) {
    for ( j = 0 ; j < MAX_THREADS ; j++ ) {
      free( iteration_log[i][j] ) ;
    }
    free( iteration_log[i] ) ;
  }
  free( iteration_log ) ;


  return 0 ;
}
