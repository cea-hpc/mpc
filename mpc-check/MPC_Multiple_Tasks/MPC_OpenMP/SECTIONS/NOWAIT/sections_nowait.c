/*
   This test check the execution of several sections a 'nowait' clause.
 */

#include <omp.h>
#include <omp_abi.h>

/* Number of threads to share the for-loop execution */
#define MIN_THREADS 2
#define MAX_THREADS 9 
#define INC_THREADS 1

/* Number of loops to execute in a row (w/ nowait clause) */
#define MIN_NOWAIT_SECTIONS 3
#define MAX_NOWAIT_SECTIONS 7
#define INC_NOWAIT_SECTIONS 1

#define ITERATION_RECORD_DEFAULT (0)


struct shared_s {
  int nb_nowaits ;
  int *** iteration_log ;
} ;

void * omp_f( void * shared ) {
  struct shared_s * s = (struct shared_s *) shared ;
  int nb_nowaits = s->nb_nowaits ;
  int *** iteration_log = s-> iteration_log ;

  int n ;

  sctk_debug( "[%d] executing %d construct(s)"
      , omp_get_thread_num(), nb_nowaits ) ;

  for ( n = 0 ; n < nb_nowaits ; n++ ) {
    int s = __mpcomp_sections_begin( 8 ) ;

    sctk_debug( "[%d] construct %d -> first section %d"
	, omp_get_thread_num(), n, s ) ;

    while ( s != 0 ) {

    sctk_debug( "[%d] construct %d: section %d"
	, omp_get_thread_num(), n, s ) ;

      switch( s ) {
	case 0:
	  break ;
	case 1:
	  iteration_log[n][omp_get_thread_num()][1]++ ;
	  break ;
	case 2:
	  iteration_log[n][omp_get_thread_num()][2]++ ;
	  break ;
	case 3:
	  iteration_log[n][omp_get_thread_num()][3]++ ;
	  break ;
	case 4:
	  iteration_log[n][omp_get_thread_num()][4]++ ;
	  break ;
	case 5:
	  iteration_log[n][omp_get_thread_num()][5]++ ;
	  break ;
	case 6:
	  iteration_log[n][omp_get_thread_num()][6]++ ;
	  break ;
	case 7:
	  iteration_log[n][omp_get_thread_num()][7]++ ;
	  break ;
	case 8:
	  iteration_log[n][omp_get_thread_num()][8]++ ;
	  break ;
      }
      s = __mpcomp_sections_next() ;
    }

    __mpcomp_sections_end_nowait() ;
  }
}

void run_omp( int n_threads, int nb_nowaits, int *** iteration_log ) {

  /*
#pragma omp parallel num_threads(n_threads)
{
  for ( n = 0 ; n < nb_nowaits ; n++ ) {
#pragma omp sections nowait
    {
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][1]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][2]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][3]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][4]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][5]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][6]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][7]++ ;
	}
	#pragma omp section
	{
		iteration_log[omp_get_thread_num()][n][8]++ ;
	}
    }
  }
}
  */

  struct shared_s s = { nb_nowaits, iteration_log } ;

  __mpcomp_start_parallel_region( n_threads, omp_f, &s ) ;
}

int test_run_omp( int n_threads, int nb_nowaits, int *** iteration_log ) {
  int n ;
  int i, s ;

  /* Step 1: check that every section has been executed exactly once */
  for ( n = 0 ; n < nb_nowaits ; n++ ) {
    for ( s = 1 ; s < 9 ; s++ ) {
      int nb_executions = 0 ; 
      for ( i = 0 ; i < n_threads ; i++ ) {
	nb_executions += iteration_log[ n ][ i ][ s ] ;
      }

      if ( nb_executions != 1 ) {
	sctk_debug(
	    "Error: section %d of construct %d has been executed %d time(s)"
	    , s, n, nb_executions ) ;
	return 1 ;
      }
    }
  }


  return 0 ;
}

int main( int argc, char ** argv ) {
  int n_threads, nb_nowaits ;
  int i, j, k ;
  int *** iteration_log ; 
  long total_xps = 0 ;

  iteration_log = (int ***)calloc( MAX_NOWAIT_SECTIONS, sizeof( int ** ) ) ;
  for ( i = 0 ; i < MAX_NOWAIT_SECTIONS ; i++ ) {
    iteration_log[i] = (int **)calloc( MAX_THREADS, sizeof( int * ) ) ;
    for ( j = 0 ; j < MAX_THREADS ; j++ ) {
      iteration_log[i][j] = (int *)calloc( 9, sizeof( int ) ) ;
    }
  }


  /* For all number of threads */
  for ( n_threads = MIN_THREADS ; n_threads < MAX_THREADS ; n_threads += INC_THREADS ) {
    /* For all the number of consecutive nb_nowaitsait */
    for ( nb_nowaits = MIN_NOWAIT_SECTIONS; nb_nowaits < MAX_NOWAIT_SECTIONS;
	nb_nowaits += INC_NOWAIT_SECTIONS ) {

      /* Clean the log */
      for ( i = 0 ; i < MAX_NOWAIT_SECTIONS ; i++ ) {
	for ( j = 0 ; j < MAX_THREADS ; j++ ) {
	  for ( k = 0 ; k < 9 ; k++ ) {
	    iteration_log[i][j][k] = ITERATION_RECORD_DEFAULT ;
	  }
	}
      }

      /* One more experiment */
      total_xps++ ;

      sctk_debug( 
	  "******** Running -> %d thread(s), %d section construct(s) *******"
	  , n_threads, nb_nowaits ) ;

      /* Run the corresponding test */
      run_omp( n_threads, nb_nowaits, iteration_log ) ;

      /* Check if the test failed */
      if ( test_run_omp( n_threads, nb_nowaits, iteration_log ) ) {
	exit( 1 ) ;
      }

    }

  }

  sctk_debug( "Success after %ld experiment(s)!", total_xps ) ;

  for ( i = 0 ; i < MAX_NOWAIT_SECTIONS; i++ ) {
    for ( j = 0 ; j < MAX_THREADS ; j++ ) {
      free( iteration_log[i][j] ) ;
    }
    free( iteration_log[i] ) ;
  }
  free( iteration_log ) ;


  return 0 ;
}
