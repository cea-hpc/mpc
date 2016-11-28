/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef mpcomp_abi__H
#define mpcomp_abi__H

/*
   This file contains all functions used by compilers to generate OpenMP code
   compatible with MPC runtime.
   To program by hand such code (bypassing a compilation process), the
   following functions can be used directly.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <mpcomp.h>

/* Conditional compilation -> See section 2.2 */
#define _OPENMP 200805


/* 
   PARALLEL construct
   ------------------

   pragma omp parallel
   { A ; }
   ->
   mpcomp_start_parallel_region( -1, f, NULL ) ;

   void * f( void * shared ) {
     A ;
   }

   'num_threads' -> -1 if the number of threads is not specified through the
   corresponding clause (OpenMP 2.5) 
 */
  void __mpcomp_start_parallel_region( void *(*func) (void *), void *shared, unsigned num_threads );

/* 
   BARRIER construct
   ------------------

   pragma omp barrier
   ->
   __mpcomp_barrier() ;
 */
  void ____mpcomp_barrier (void);

/*
   CRITICAL construct
   ------------------
 */

  void __mpcomp_anonymous_critical_begin ();
  void __mpcomp_anonymous_critical_end ();
  void __mpcomp_named_critical_begin (void **l);
  void __mpcomp_named_critical_end (void **l);

/* 
 * SECTIONS/SECTION construct 
 * --------------------------
 *  pragma omp sections
 *  {
 *    pragma omp section
 *    { BODY1; }
 *    pragma omp section
 *    { BODY2; }
 *  }
 *  ->
 */
  int __mpcomp_sections_begin (int nb_sections);
  int __mpcomp_sections_next ();
  void __mpcomp_sections_end ();
  void __mpcomp_sections_end_nowait ();

/*
   SINGLE construct
   ----------------
   pragma omp single
   { BODY; }
   ->
   if ( mpcomp_do_single() ) { BODY; }
   __mpcomp_barrier() ;

   pragma omp single nowait
   { BODY; }
   ->
   if ( mpcomp_do_single() ) { BODY; }
 */
  int __mpcomp_do_single (void);

/* 
   SINGLE construct W/ COPYPRIVATE
   -------------------------------
   pragma omp single copyprivate(a)
   { BODY; }
   ->
   struct { type a ; } data ;
   data * d = (data *)mpcomp_do_single_copyprivate_begin() ;
   if ( d == NULL ) { 
   	BODY ; 
	d->a = a ; 
	__mpcomp_do_single_copyprivate_end(data) ; 
   } else { a = d->a ; } 

	Note: The 'nowait' clause has not effect here
 */
  void *__mpcomp_do_single_copyprivate_begin (void);
  void __mpcomp_do_single_copyprivate_end (void *data);

/*
   ATOMIC construct

   #pragma atomic
   	expr ;
   ->
   __mpcomp_atomic_begin() ;
   expr ;
   __mpcomp_atomic_end() ;

   This implementation is not very efficient, a better solution is to generate
   an atomic instruction related to the 'expr' statement.
*/
  void __mpcomp_atomic_begin ();
  void __mpcomp_atomic_end ();


/*
   FOR construct
   -------------
 */

/* STATIC schedule */


/* 
   Automatic chunk size 
   --------------------
   pragma omp for schedule(static)
   for ( ... ) { A; } 
   ->
   __mpcomp_static_schedule_get_single_chunk( ... ) ; 
   for ( ... ) { A; }
   __mpcomp_barrier() ;
 */

#if 0
/* See p34 for variable definition */
  int __mpcomp_static_schedule_get_single_chunk (long lb, long b, long incr, long
						  *from, long *to);

  int __mpcomp_static_schedule_get_single_chunk_ull (unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long
						  *from, unsigned long long *to);
#endif 
/* 
   Forced chunk size 
   pragma omp for schedule(static,chunk_size)
   for ( ... ) { A; } 
   ->
   int c = __mpcomp_static_schedule_get_nb_chunks( ... ) ;
   for ( i: 0 -> c step 1) {
     __mpcomp_static_schedule_get_specific_chunk( ... ) ;
     for ( ... ) { A; }
   }
   __mpcomp_barrier() ;
 */


#if 0
  int __mpcomp_static_schedule_get_nb_chunks (long lb, long b, long incr,
					      long chunk_size);
  unsigned long long __mpcomp_static_schedule_get_nb_chunks_ull (unsigned long long lb, unsigned long long b, unsigned long long incr,
					      unsigned long long chunk_size);
  void __mpcomp_static_schedule_get_specific_chunk (long lb, long b, long incr,
						    long chunk_size,
						    long chunk_num, long *from,
						    long *to);
  void __mpcomp_static_schedule_get_specific_chunk_ull (unsigned long long lb, unsigned long long b, unsigned long long incr,
						    unsigned long long chunk_size,
						    unsigned long long chunk_num, unsigned long long *from,
						    unsigned long long *to);
#endif 
  
  int __mpcomp_static_loop_begin (long lb, long b, long incr, long chunk_size,
				  long *from, long *to);
  int __mpcomp_loop_ull_static_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
				  unsigned long long *from, unsigned long long *to);
  int __mpcomp_static_loop_next (long *from, long *to);
  int __mpcomp_loop_ull_static_next (unsigned long long *from, unsigned long long *to);
  void __mpcomp_static_loop_end ();
  void __mpcomp_static_loop_end_nowait ();


/* DYNAMIC schedule */
/*
   pragma omp for schedule(dynamic,chunk_size)
   for ( i=lb ; i < b ; i += incr ) { 
     BODY ; 
   }
   ->
   if ( __mpcomp_dynamic_loop_begin( lb, b, incr, chunk_size, &f, &t ) ) {
     do {
       for ( i=f ; i<t ; i+=incr ) {
	 A ;
       } 
     } while ( __mpcomp_dynamic_loop_next( &f, &t ) ) ;
   }
   __mpcomp_dynamic_loop_end() ;

   If the 'nowait' clause is present, replace the call to
   'mpcomp_dynamic_loop_end' with a call to
   'mpcomp_dynamic_loop_end_nowait'.
 */
	int __mpcomp_dynamic_loop_begin(long lb, long b, long incr,
			long chunk_size, long *from, long *to);
	int __mpcomp_loop_ull_dynamic_begin(bool up, unsigned long long lb, unsigned long long b, unsigned long long incr,
			unsigned long long chunk_size, unsigned long long *from, unsigned long long *to);
	int __mpcomp_loop_ull_dynamic_next(unsigned long long *from, unsigned long long *to);
	int __mpcomp_dynamic_loop_next(long *from, long *to);
	void __mpcomp_dynamic_loop_end();
	void __mpcomp_dynamic_loop_end_nowait();

/* GUIDED schedule */
/*
   pragma omp for schedule(guided,chunk_size)
   for ( ... ) { A ; }
   ->
   if ( __mpcomp_guided_loop_begin( ... ) ) {
     do {
       for ( ... ) {
	 A ;
       } 
     } while ( __mpcomp_guided_loop_next( ... ) ) ;
   }
   __mpcomp_guided_loop_end() ;
 */
  int __mpcomp_guided_loop_begin (long lb, long b, long incr, long chunk_size,
				  long *from, long *to);
  int __mpcomp_loop_ull_guided_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
				  unsigned long long *from, unsigned long long *to);
  int __mpcomp_guided_loop_next (long *from, long *to);
  int __mpcomp_loop_ull_guided_next (unsigned long long *from, unsigned long long *to);
  void __mpcomp_guided_loop_end ();
  void __mpcomp_guided_loop_end_nowait ();

/* RUNTIME schedule */
/*
   pragma omp for schedule(runtime)
   for ( i=lb ; i COND b ; i+=incr ) { A ; }
   ->
   if ( __mpcomp_runtime_loop_begin( ... ) ) {
     do {
       for ( ... ) {
	 A ;
       } 
     } while ( __mpcomp_runtime_loop_next( ... ) ) ;
   }
   __mpcomp_runtime_loop_end() ;
 */
  int __mpcomp_runtime_loop_begin (long lb, long b, long incr,
				   long *from, long *to);
  int __mpcomp_loop_ull_runtime_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr,
				   unsigned long long *from, unsigned long long *to);
  int __mpcomp_runtime_loop_next (long *from, long *to);
  int __mpcomp_loop_ull_runtime_next (unsigned long long *from, unsigned long long *to);
  void __mpcomp_runtime_loop_end ();
  void __mpcomp_runtime_loop_end_nowait ();

/*
   ORDERED clause/construct
 */
/*
   pragma omp for schedule(XX) ordered
   for ( i=lb ; i COND b ; i+=incr ) { 
      A ; 
#pragma omp ordered
      {
         B ;
      }
   }
   ->
   if ( __mpcomp_ordered_XX_loop_begin( ... ) ) {
     do {
       for ( ... ) {
	 A ;
	 __mpcomp_ordered_begin() ;
	 B ;
	 __mpcomp_ordered_end() ;

       } 
     } while ( __mpcomp_ordered_XX_loop_next( ... ) ) ;
   }
   __mpcomp_ordered_XX_loop_end() ;


 Note: Combined parallel/for region can be done by splitting into 2 different
 directives (one for the parallel region and one for the 'for' loop)

 !Warning! 'Nowait' clause is not considerd with 'ordered' construct
 */

int __mpcomp_ordered_static_loop_begin (long lb, long b, long incr, long chunk_size, long *from, long *to) ;
int __mpcomp_ordered_static_loop_next(long *from, long *to) ;
int __mpcomp_loop_ull_ordered_static_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr, 
unsigned long long chunk_size, unsigned long long *from, unsigned long long *to) ;
int __mpcomp_loop_ull_ordered_static_next(unsigned long long *from, unsigned long long *to) ;
void __mpcomp_ordered_static_loop_end() ;
void __mpcomp_ordered_static_loop_end_nowait() ;

int __mpcomp_ordered_dynamic_loop_begin (long lb, long b, long incr, long chunk_size, long *from, long *to) ;
int __mpcomp_ordered_dynamic_loop_next(long *from, long *to) ;
int __mpcomp_loop_ull_ordered_dynamic_begin ( bool, unsigned long long lb, unsigned long long b, unsigned long long incr, 
unsigned long long chunk_size, unsigned long long *from, unsigned long long *to) ;
int __mpcomp_loop_ull_ordered_dynamic_next(unsigned long long *from, unsigned long long *to) ;
void __mpcomp_ordered_dynamic_loop_end() ;
void __mpcomp_ordered_dynamic_loop_end_nowait() ;

int __mpcomp_ordered_guided_loop_begin (long lb, long b, long incr, long chunk_size, long *from, long *to) ;
int __mpcomp_ordered_guided_loop_next (long *from, long *to) ;
int __mpcomp_loop_ull_ordered_guided_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr, 
unsigned long long chunk_size, unsigned long long *from, unsigned long long *to) ;
int __mpcomp_loop_ull_ordered_guided_next(unsigned long long *from, unsigned long long *to) ;
void __mpcomp_ordered_guided_loop_end () ;
void __mpcomp_ordered_guided_loop_end_nowait () ;

int __mpcomp_ordered_runtime_loop_begin (long lb, long b, long incr, long *from, long *to) ;
int __mpcomp_ordered_runtime_loop_next (long *from, long *to) ;
int __mpcomp_loop_ull_ordered_runtime_begin (bool, unsigned long long lb, unsigned long long b, unsigned long long incr, 
unsigned long long *from, unsigned long long *to) ;
int __mpcomp_loop_ull_ordered_runtime_next(unsigned long long *from, unsigned long long *to) ;
void __mpcomp_ordered_runtime_loop_end () ;
void __mpcomp_ordered_runtime_loop_end_nowait () ;
/* CHECKPOINT/RESTART */
/*
   If the application uses the OpenMP ABI (or API), the checkpoint/restart
   mechanism has to be called through the function 'mpcomp_checkpoint()'.
   
   This function will take care of saving everything related to OpenMP and the
   MPC tasks (message-passing interface).
 */
  int mpcomp_checkpoint ();


#ifdef __cplusplus
}
#endif

#endif
