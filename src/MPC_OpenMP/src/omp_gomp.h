#ifndef OMP_GOMP_H_
#define OMP_GOMP_H_

#include <mpc_common_types.h>
#include <mpcomp_types.h>

/**********************
 * SYMBOL VERSIONNING *
 **********************/

#define MPCOMP_VERSION_SYMBOLS

#ifdef MPCOMP_VERSION_SYMBOLS
#define str( x ) #x
//#define versionify(gomp_api_name, version_str, mpcomp_api_name)
#define versionify( mpcomp_api_name, gomp_api_name, version_str ) \
	__asm__( ".symver " str( mpcomp_api_name ) "," str( gomp_api_name ) "@@" version_str );
#else // MPCOMP_USE_VERSION_SYMBOLS
#define str( x )
#define versionify( api_name, version_num, version_str ) /* Nothing */
#endif

#define MPCOMP_GOMP_UNUSED_VAR( var ) (void) ( sizeof( var ) )

#define MPCOMP_GOMP_NO_ALIAS ""
#define MPCOMP_GOMP_EMPTY_FN ""

#define GOMP_ABI_FUNC( gomp_name, version, wrapper, mpcomp_name ) \
	versionify( wrapper, gomp_name, version )

#include "omp_gomp_version.h"
#undef GOMP_ABI_FUNC

/***********
 * ORDERED *
 ***********/

void mpcomp_GOMP_ordered_start( void );
void mpcomp_GOMP_ordered_end( void );

/***********
 * BARRIER *
 ***********/

void mpcomp_GOMP_barrier( void );
bool GOMP_barrier_cancel( void );

/**********
 * SINGLE *
 **********/

bool mpcomp_GOMP_single_start( void );
void *mpcomp_GOMP_single_copy_start( void );
void mpcomp_GOMP_single_copy_end( void *data );

/********************
 * WRAPPER FUNCTION *
 ********************/

typedef struct mpcomp_GOMP_wrapper_s
{
	void ( *fn )( void * );
	void *data;
} mpcomp_GOMP_wrapper_t;

/*******************
 * PARALLEL REGION *
 *******************/

void mpcomp_GOMP_parallel( void ( *fn )( void * ), void *data, unsigned num_threads,
                           unsigned flags );
void mpcomp_GOMP_parallel_start( void ( *fn )( void * ), void *data,
                                 unsigned num_threads );
void mpcomp_GOMP_parallel_end( void );
bool mpcomp_GOMP_cancellation_point( int which );
bool mpcomp_GOMP_cancel( int which, bool do_cancel );

/********
 * LOOP *
 ********/

bool mpcomp_GOMP_loop_runtime_start( long istart, long iend, long incr,
                                     long *start, long *end );

/* Ordered Loop */

bool mpcomp_GOMP_loop_ordered_runtime_start( long istart, long iend, long incr,
        long *start, long *end );

bool mpcomp_GOMP_loop_ordered_runtime_next( long *start, long *end );

bool mpcomp_GOMP_loop_ordered_static_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpcomp_GOMP_loop_ordered_static_next( long *start, long *end );

bool mpcomp_GOMP_loop_ordered_dynamic_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpcomp_GOMP_loop_ordered_dynamic_next( long *start, long *end );

bool mpcomp_GOMP_loop_ordered_guided_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpcomp_GOMP_loop_ordered_guided_next( long *start, long *end );

/* Static */

void mpcomp_GOMP_parallel_loop_static_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );
bool mpcomp_GOMP_loop_static_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end );

bool mpcomp_GOMP_loop_static_next( long *start, long *end );

/* Dynamic */

void mpcomp_GOMP_parallel_loop_dynamic_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );

bool mpcomp_GOMP_loop_dynamic_start( long istart, long iend, long incr,
                                     long chunk_size, long *start, long *end );

bool mpcomp_GOMP_loop_dynamic_next( long *start, long *end );

/* Guided */

void mpcomp_GOMP_parallel_loop_guided_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );

bool mpcomp_GOMP_loop_guided_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end );

bool mpcomp_GOMP_loop_guided_next( long *start, long *end );

/* Parallel loop */

void mpcomp_GOMP_parallel_loop_runtime_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr );

/* loop nonmonotonic */

void mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags );

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end );

/* loop nonmonotonic */

void mpcomp_GOMP_parallel_loop_nonmonotonic_guided(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags );

bool mpcomp_GOMP_loop_nonmonotonic_guided_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end );

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_next( long *start, long *end );

bool mpcomp_GOMP_loop_nonmonotonic_guided_next( long *start, long *end );

/* Common loop ops */

bool mpcomp_GOMP_loop_runtime_next( long *start, long *end );

void mpcomp_GOMP_loop_end( void );
bool mpcomp_GOMP_loop_end_cancel( void );

void mpcomp_GOMP_loop_end_nowait( void );

/* Doacross */

bool mpcomp_GOMP_loop_doacross_static_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

bool mpcomp_GOMP_loop_doacross_dynamic_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

bool mpcomp_GOMP_loop_doacross_guided_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

/** OPENMP 4.0 **/

void mpcomp_GOMP_parallel_loop_static( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       unsigned flags );

void mpcomp_GOMP_parallel_loop_dynamic( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, long chunk_size,
                                        unsigned flags );

void mpcomp_GOMP_parallel_loop_guided( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       unsigned flags );

void mpcomp_GOMP_parallel_loop_runtime( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, unsigned flags );

/************
 * LOOP ULL *
 ************/

bool mpcomp_GOMP_loop_ull_runtime_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_runtime_start( bool up,
        unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_doacross_runtime_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_runtime_next( unsigned long long *istart,
                                        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_runtime_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_static_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_dynamic_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_guided_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_doacross_static_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_doacross_dynamic_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_doacross_guided_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_static_next( unsigned long long *istart,
                                       unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_dynamic_next( unsigned long long *istart,
                                        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_guided_next( unsigned long long *istart,
                                       unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_static_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_dynamic_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpcomp_GOMP_loop_ull_ordered_guided_next( unsigned long long *istart,
        unsigned long long *iend );

/***********
 * SECTION *
 ***********/

unsigned mpcomp_GOMP_sections_start( unsigned count );

void mpcomp_GOMP_sections_end( void );

unsigned mpcomp_GOMP_sections_next( void );

void mpcomp_GOMP_parallel_sections( void ( *fn )( void * ), void *data,
                                    unsigned num_threads, unsigned count,
                                    unsigned flags );

void mpcomp_GOMP_parallel_sections_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, unsigned count );

bool mpcomp_GOMP_sections_end_cancel( void );

void mpcomp_GOMP_sections_end_nowait( void );

/************
 * CRITICAL *
 ************/

void mpcomp_GOMP_critical_start( void );

void mpcomp_GOMP_critical_end( void );

void mpcomp_GOMP_critical_name_start( void **pptr );

void mpcomp_GOMP_critical_name_end( void **pptr );

void mpcomp_GOMP_atomic_start( void );

void mpcomp_GOMP_atomic_end( void );

/*********
 * TASKS *
 *********/

#if MPCOMP_USE_TASKDEP
void mpcomp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend );
#else
void mpcomp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend, int priority );
#endif

void mpcomp_GOMP_taskwait( void );

void mpcomp_GOMP_taskyield( void );

#if MPCOMP_TASK
	void mpcomp_GOMP_taskgroup_start( void );

	void mpcomp_GOMP_taskgroup_end( void );
#endif /* MPCOMP_TASK */

/***********
 * TARGETS *
 ***********/

void mpcomp_GOMP_offload_register_ver( unsigned version,  const void *host_table,
                                       int target_type,
                                       const void *target_data );

void mpcomp_GOMP_offload_register( const void *host_table,  int target_type,
                                   const void *target_data );

void mpcomp_GOMP_offload_unregister_ver( unsigned version,
        const void *host_table,  int target_type,
        const void *target_data );

void mpcomp_GOMP_offload_unregister( const void *host_table,  int target_type,
                                     const void *target_data );

void mpcomp_GOMP_target( int device,  void ( *fn )( void * ),  const void *unused,
                         size_t mapnum,  void **hostaddrs,  size_t *sizes,
                         unsigned char *kinds );

void mpcomp_GOMP_target_ext( int device,   void ( *fn )( void * ),  size_t mapnum,
                             void **hostaddrs,  size_t *sizes,
                             unsigned short *kinds,  unsigned int flags,
                             void **depend,  void **args );
void mpcomp_GOMP_target_data( int device,  const void *unused,  size_t mapnum,
                              void **hostaddrs,  size_t *sizes,
                              unsigned char *kinds );

void mpcomp_GOMP_target_data_ext( int device,  size_t mapnum,  void **hostaddrs,
                                  size_t *sizes,  unsigned short *kinds );

void mpcomp_GOMP_target_end_data( void );

void mpcomp_GOMP_target_update( int device,  const void *unused,  size_t mapnum,
                                void **hostaddrs,  size_t *sizes,
                                unsigned char *kinds );

void mpcomp_GOMP_target_update_ext( int device,  size_t mapnum,  void **hostaddrs,
                                    size_t *sizes,  unsigned short *kinds,
                                    unsigned int flags,  void **depend );
void mpcomp_GOMP_target_enter_exit_data( int device,  size_t mapnum,
        void **hostaddrs,  size_t *sizes,
        unsigned short *kinds,
        unsigned int flags,  void **depend );

void mpcomp_GOMP_teams( unsigned int num_teams,  unsigned int thread_limit );


#endif /* OMP_GOMP_H_ */