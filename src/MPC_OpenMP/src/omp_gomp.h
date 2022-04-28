/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:08 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef OMP_GOMP_H_
#define OMP_GOMP_H_

#include <mpc_common_types.h>
#include <mpcomp_types.h>

/**********************
 * SYMBOL VERSIONNING *
 **********************/

#define MPC_OMP_VERSION_SYMBOLS

#ifdef MPC_OMP_VERSION_SYMBOLS
# define str(x) #x
# define versionify( mpc_name, gomp_name, version ) \
    __asm__( ".symver " str( mpc_name ) "," str( gomp_name ) "@@" version );
# define GOMP_ABI_FUNC(mpc_name, gomp_name, version) versionify(mpc_name, gomp_name, version)
# include "omp_gomp_version.h"
# undef GOMP_ABI_FUNC
# undef str
#endif /* MPC_OMP_VERSION_SYMBOLS */

#define MPC_OMP_GOMP_UNUSED_VAR( var ) (void) ( sizeof( var ) )
#define MPC_OMP_GOMP_NO_ALIAS ""
#define MPC_OMP_GOMP_EMPTY_FN ""

/***********
 * ORDERED *
 ***********/

void mpc_omp_GOMP_ordered_start( void );
void mpc_omp_GOMP_ordered_end( void );

/***********
 * BARRIER *
 ***********/

void mpc_omp_GOMP_barrier( void );
bool GOMP_barrier_cancel( void );

/**********
 * SINGLE *
 **********/

bool mpc_omp_GOMP_single_start( void );
void *mpc_omp_GOMP_single_copy_start( void );
void mpc_omp_GOMP_single_copy_end( void *data );

/********************
 * WRAPPER FUNCTION *
 ********************/

typedef struct mpc_omp_GOMP_wrapper_s
{
	void ( *fn )( void * );
	void *data;
} mpc_omp_GOMP_wrapper_t;

/*******************
 * PARALLEL REGION *
 *******************/

void mpc_omp_GOMP_parallel( void ( *fn )( void * ), void *data, unsigned num_threads,
                           unsigned flags );
void mpc_omp_GOMP_parallel_start( void ( *fn )( void * ), void *data,
                                 unsigned num_threads );
void mpc_omp_GOMP_parallel_end( void );
bool mpc_omp_GOMP_cancellation_point( int which );
bool mpc_omp_GOMP_cancel( int which, bool do_cancel );

/********
 * LOOP *
 ********/

bool mpc_omp_GOMP_loop_runtime_start( long istart, long iend, long incr,
                                     long *start, long *end );

/* Ordered Loop */

bool mpc_omp_GOMP_loop_ordered_runtime_start( long istart, long iend, long incr,
        long *start, long *end );

bool mpc_omp_GOMP_loop_ordered_runtime_next( long *start, long *end );

bool mpc_omp_GOMP_loop_ordered_static_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpc_omp_GOMP_loop_ordered_static_next( long *start, long *end );

bool mpc_omp_GOMP_loop_ordered_dynamic_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpc_omp_GOMP_loop_ordered_dynamic_next( long *start, long *end );

bool mpc_omp_GOMP_loop_ordered_guided_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end );

bool mpc_omp_GOMP_loop_ordered_guided_next( long *start, long *end );

/* Static */

void mpc_omp_GOMP_parallel_loop_static_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );
bool mpc_omp_GOMP_loop_static_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end );

bool mpc_omp_GOMP_loop_static_next( long *start, long *end );

/* Dynamic */

void mpc_omp_GOMP_parallel_loop_dynamic_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );

bool mpc_omp_GOMP_loop_dynamic_start( long istart, long iend, long incr,
                                     long chunk_size, long *start, long *end );

bool mpc_omp_GOMP_loop_dynamic_next( long *start, long *end );

/* Guided */

void mpc_omp_GOMP_parallel_loop_guided_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size );

bool mpc_omp_GOMP_loop_guided_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end );

bool mpc_omp_GOMP_loop_guided_next( long *start, long *end );

/* Parallel loop */

void mpc_omp_GOMP_parallel_loop_runtime_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr );

/* loop nonmonotonic */

void mpc_omp_GOMP_parallel_loop_nonmonotonic_dynamic(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags );

bool mpc_omp_GOMP_loop_nonmonotonic_dynamic_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end );

/* loop nonmonotonic */

void mpc_omp_GOMP_parallel_loop_nonmonotonic_guided(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags );

bool mpc_omp_GOMP_loop_nonmonotonic_guided_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end );

bool mpc_omp_GOMP_loop_nonmonotonic_dynamic_next( long *start, long *end );

bool mpc_omp_GOMP_loop_nonmonotonic_guided_next( long *start, long *end );

/* Common loop ops */

bool mpc_omp_GOMP_loop_runtime_next( long *start, long *end );

void mpc_omp_GOMP_loop_end( void );
bool mpc_omp_GOMP_loop_end_cancel( void );

void mpc_omp_GOMP_loop_end_nowait( void );

/* Doacross */

bool mpc_omp_GOMP_loop_doacross_static_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

bool mpc_omp_GOMP_loop_doacross_dynamic_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

bool mpc_omp_GOMP_loop_doacross_guided_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end );

/** OPENMP 4.0 **/

void mpc_omp_GOMP_parallel_loop_static( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       unsigned flags );

void mpc_omp_GOMP_parallel_loop_dynamic( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, long chunk_size,
                                        unsigned flags );

void mpc_omp_GOMP_parallel_loop_guided( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       unsigned flags );

void mpc_omp_GOMP_parallel_loop_runtime( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, unsigned flags );

/************
 * LOOP ULL *
 ************/

bool mpc_omp_GOMP_loop_ull_runtime_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_runtime_start( bool up,
        unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_doacross_runtime_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_runtime_next( unsigned long long *istart,
                                        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_runtime_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_static_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_dynamic_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_guided_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_nonmonotonic_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_doacross_static_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_doacross_dynamic_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_doacross_guided_start( unsigned ncounts,
        unsigned long long *counts,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_static_next( unsigned long long *istart,
                                       unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_dynamic_next( unsigned long long *istart,
                                        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_guided_next( unsigned long long *istart,
                                       unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_nonmonotonic_guided_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_static_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_dynamic_next( unsigned long long *istart,
        unsigned long long *iend );

bool mpc_omp_GOMP_loop_ull_ordered_guided_next( unsigned long long *istart,
        unsigned long long *iend );

/***********
 * SECTION *
 ***********/

unsigned mpc_omp_GOMP_sections_start( unsigned count );

void mpc_omp_GOMP_sections_end( void );

unsigned mpc_omp_GOMP_sections_next( void );

void mpc_omp_GOMP_parallel_sections( void ( *fn )( void * ), void *data,
                                    unsigned num_threads, unsigned count,
                                    unsigned flags );

void mpc_omp_GOMP_parallel_sections_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, unsigned count );

bool mpc_omp_GOMP_sections_end_cancel( void );

void mpc_omp_GOMP_sections_end_nowait( void );

/************
 * CRITICAL *
 ************/

void mpc_omp_GOMP_critical_start( void );

void mpc_omp_GOMP_critical_end( void );

void mpc_omp_GOMP_critical_name_start( void **pptr );

void mpc_omp_GOMP_critical_name_end( void **pptr );

void mpc_omp_GOMP_atomic_start( void );

void mpc_omp_GOMP_atomic_end( void );

/*********
 * TASKS *
 *********/

void mpc_omp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend, int priority);

void mpc_omp_GOMP_taskwait( void );
void mpc_omp_GOMP_taskwait_depend( void ** depend);

void mpc_omp_GOMP_taskyield( void );

void mpc_omp_GOMP_taskgroup_start( void );

void mpc_omp_GOMP_taskgroup_end( void );

/***********
 * TARGETS *
 ***********/

void mpc_omp_GOMP_offload_register_ver( unsigned version,  const void *host_table,
                                       int target_type,
                                       const void *target_data );

void mpc_omp_GOMP_offload_register( const void *host_table,  int target_type,
                                   const void *target_data );

void mpc_omp_GOMP_offload_unregister_ver( unsigned version,
        const void *host_table,  int target_type,
        const void *target_data );

void mpc_omp_GOMP_offload_unregister( const void *host_table,  int target_type,
                                     const void *target_data );

void mpc_omp_GOMP_target( int device,  void ( *fn )( void * ),  const void *unused,
                         size_t mapnum,  void **hostaddrs,  size_t *sizes,
                         unsigned char *kinds );

void mpc_omp_GOMP_target_ext( int device,   void ( *fn )( void * ),  size_t mapnum,
                             void **hostaddrs,  size_t *sizes,
                             unsigned short *kinds,  unsigned int flags,
                             void **depend,  void **args );
void mpc_omp_GOMP_target_data( int device,  const void *unused,  size_t mapnum,
                              void **hostaddrs,  size_t *sizes,
                              unsigned char *kinds );

void mpc_omp_GOMP_target_data_ext( int device,  size_t mapnum,  void **hostaddrs,
                                  size_t *sizes,  unsigned short *kinds );

void mpc_omp_GOMP_target_end_data( void );

void mpc_omp_GOMP_target_update( int device,  const void *unused,  size_t mapnum,
                                void **hostaddrs,  size_t *sizes,
                                unsigned char *kinds );

void mpc_omp_GOMP_target_update_ext( int device,  size_t mapnum,  void **hostaddrs,
                                    size_t *sizes,  unsigned short *kinds,
                                    unsigned int flags,  void **depend );
void mpc_omp_GOMP_target_enter_exit_data( int device,  size_t mapnum,
        void **hostaddrs,  size_t *sizes,
        unsigned short *kinds,
        unsigned int flags,  void **depend );

void mpc_omp_GOMP_teams( unsigned int num_teams,  unsigned int thread_limit );


#endif /* OMP_GOMP_H_ */
