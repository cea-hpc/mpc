/**
  Check a for loop with dynamic schedule and ordered clause.
  */
#include <omp.h>
#include <omp_abi.h>

#define MIN_THREADS 2
#define MAX_THREADS 9
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
#define ITERATION_ORDER_DEFAULT (-1)


struct shared_s {
  int lb ;
  int b ;
  int step ;
  int chunk_size ;
  int ** iteration_log ;
  int * count ;
  int * iteration_order ;
} ;

void * omp_f( void * shared ) {
  struct shared_s * s = (struct shared_s *) shared ;
  int lb = s->lb ;
  int b = s->b ;
  int step = s->step ;
  int chunk_size = s->chunk_size ;
  int ** iteration_log = s-> iteration_log ;
  int * count = s->count ;
  int * iteration_order = s->iteration_order ;

  int from ;
  int to ;
  int i ;
  int c_i ;

  if ( __mpcomp_ordered_dynamic_loop_begin( lb, b, step, chunk_size, &from, &to ) ) {
    do {
      for ( i = from ; i < to ; i += step ) {
	iteration_log[ omp_get_thread_num() ][ i ]++ ;
	__mpcomp_ordered_begin() ;
	iteration_order[i] = *count ;
	*count = *count + 1 ;
	__mpcomp_ordered_end() ;
      }
    } while ( __mpcomp_ordered_dynamic_loop_next( &from, &to ) ) ;
  }
  __mpcomp_ordered_dynamic_loop_end() ;

  return NULL ;
}

void run_omp( int n_threads, int lb, int b, int step, int chunk_size, int ** iteration_log, 
    int * count, int * iteration_order ) {

  /*
     step = 0 ;
#pragma omp parallel num_threads(n_threads)
  {
#pragma omp for schedule(static,chunk_size) ordered
    for ( i = lb ; i < b ; i += step ) {
      iteration_rec[ i ] = omp_get_thread_num() ;
#pragma omp ordered
      {
	iteration_order[ i ] = count ;
	step++ ;
      }
    }
  }
  */

  struct shared_s s = { lb, b, step, chunk_size, iteration_log, count, iteration_order } ;

  __mpcomp_start_parallel_region( n_threads, omp_f, &s ) ;
}

int test_run_omp( int n_threads, int lb, int b, int step, int chunk_size, int ** iteration_log, 
    int * count, int * iteration_order ) {
  int i, j ;
  int count_check ;

  /* Step 1: check that every iteration has been executed once */
  for ( i = lb ; i < b ; i += step ) {
    int found = -1 ;

    for ( j = 0 ; j < n_threads ; j++ ) {

      if ( iteration_log[j][i] > 0 ) {
	if ( found != -1 ) {
	  sctk_debug( "Error: iteration %d has been scheduled by (at least) threads %d and %d"
	      , i, found, j ) ;
	  return 1 ;

	} else {
	  if ( iteration_log[j][i] > 1 ) {
	    sctk_debug( "Error: Thread %d has scheduled several times iteration %d"
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

  /* Step 4: check that the ordered clause has been respected */
  count_check = 0 ;
  for ( i = lb ; i < b ; i +=step ) {
    /* For each scheduled iteration */
    if ( iteration_order[ i ] == ITERATION_ORDER_DEFAULT ) {
      sctk_debug( "Error: iteration %d has been skipped by the ordered construct",
	  i ) ;
      for ( j = 0 ; j < n_threads ; j++ ) {
	if ( iteration_log[j][i] > 0 ) {
	  sctk_debug( "\t-> this iteration has been scheduled by thread %d", j ) ;
	} 
      }

      return 1 ;
    }
    if ( iteration_order[ i ] != count_check ) {
      sctk_debug( "Error: iteration %d should have been scheduled at order %d and not %d",
	  i, count_check, iteration_order[ i ] ) ;
      return 1 ;
    }
    count_check++ ;

  }

  return 0 ;
}

int main( int argc, char ** argv ) {
  int n_threads, n_iterations, b, lb, step, i, j ;
  int chunk_size ;
  int ** iteration_log ; 
  int count ;
  int * iteration_order ;

  iteration_log = (int **)calloc( MAX_THREADS, sizeof( int * ) ) ;
  for ( i = 0 ; i < MAX_THREADS ; i++ ) {
    iteration_log[i] = (int *)calloc( MAX_B-MIN_LB, sizeof( int ) ) ;
  }

  iteration_order = (int *)calloc( MAX_B-MIN_LB, sizeof( int ) ) ;


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

	    /* Clean the order */
	    for ( i = 0 ; i < (MAX_B-MIN_LB) ; i++ ) {
	      iteration_order[i] = ITERATION_ORDER_DEFAULT ;
	    }

	    /* Re-initialize the iteration count */
	    count = 0 ;

	    n_iterations = (b - lb) / step ;
	    if ( (b - lb) % step != 0 ) {
	      n_iterations++ ;
	    }

	    sctk_debug( "Running -> %d thread(s) from %d to %d step %d => %d it w/ chunk %d\n"
		, n_threads, lb, b, step, n_iterations, chunk_size ) ;

	    /* Run the corresponding test */
	    run_omp( n_threads, lb, b, step, chunk_size, iteration_log, &count, iteration_order ) ;

	    /* Check if the test failed */
	    if ( test_run_omp( n_threads, lb, b, step, chunk_size, iteration_log, &count, iteration_order ) ) {
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

  free(iteration_order) ;


  return 0 ;
}
