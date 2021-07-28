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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPCOMP_LOOP_H_
#define MPCOMP_LOOP_H_

#include <string.h>

#include "mpc_common_debug.h"
#include <mpc_common_types.h>

#include "mpcomp_types.h"

/*******************
 * LOOP CORE TYPES *
 *******************/

static inline void
_mpc_omp_loop_gen_infos_init( mpc_omp_loop_gen_info_t *loop_infos, long lb,
                              long b, long incr, long chunk_size )
{
	assert( loop_infos );
	loop_infos->fresh = true;
	loop_infos->ischunked = ( chunk_size ) ? 1 : 0;
	loop_infos->type = MPCOMP_LOOP_TYPE_LONG;
	loop_infos->loop.mpcomp_long.up = ( incr > 0 );
	loop_infos->loop.mpcomp_long.b = b;
	loop_infos->loop.mpcomp_long.lb = lb;
	loop_infos->loop.mpcomp_long.incr = incr;
	/* Automatic chunk size -> at most one chunk */
	loop_infos->loop.mpcomp_long.chunk_size = ( chunk_size ) ? chunk_size : 1;
}

static inline void
_mpc_omp_loop_gen_infos_init_ull( mpc_omp_loop_gen_info_t *loop_infos,
                                  unsigned long long lb, unsigned long long b,
                                  unsigned long long incr,
                                  unsigned long long chunk_size )
{
	assert( loop_infos );
	loop_infos->fresh = true;
	loop_infos->ischunked = ( chunk_size ) ? 1 : 0;
	loop_infos->type = MPCOMP_LOOP_TYPE_ULL;
	loop_infos->loop.mpcomp_ull.up = ( incr > 0 );
	loop_infos->loop.mpcomp_ull.b = b;
	loop_infos->loop.mpcomp_ull.lb = lb;
	loop_infos->loop.mpcomp_ull.incr = incr;
	/* Automatic chunk size -> at most one chunk */
	loop_infos->loop.mpcomp_ull.chunk_size = ( chunk_size ) ? chunk_size : 1;
}

static inline void
_mpc_omp_loop_gen_loop_infos_cpy( mpc_omp_loop_gen_info_t *in,
                                  mpc_omp_loop_gen_info_t *out )
{
	memcpy( out, in, sizeof( mpc_omp_loop_gen_info_t ) );
}

static inline void
_mpc_omp_loop_gen_loop_infos_reset( mpc_omp_loop_gen_info_t *loop )
{
	memset( loop, 0, sizeof( mpc_omp_loop_gen_info_t ) );
}

/***************
 * LOOP STATIC *
 ***************/

struct mpc_omp_thread_s;

void _mpc_omp_static_loop_init( struct mpc_omp_thread_s *, long, long, long,
                                long );


int mpc_omp_static_loop_begin( long, long, long, long, long *, long * );
int mpc_omp_static_loop_next( long *, long * );
void mpc_omp_static_loop_end( void );
void mpc_omp_static_loop_end_nowait( void );

int mpc_omp_ordered_static_loop_begin( long, long b, long, long, long *,
                                        long * );
int mpc_omp_ordered_static_loop_next( long *, long * );
void mpc_omp_ordered_static_loop_end( void );
void mpc_omp_ordered_static_loop_end_nowait( void );

int mpc_omp_static_loop_begin_ull( bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size,
                                    unsigned long long *from,
                                    unsigned long long *to );

int mpc_omp_static_loop_next_ull( unsigned long long *from,
                                   unsigned long long *to );

int mpc_omp_ordered_static_loop_begin_ull( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *from,
        unsigned long long *to );

int mpc_omp_ordered_static_loop_next_ull( unsigned long long *from,
        unsigned long long *to );

/***************
 * LOOP GUIDED *
 ***************/

int mpc_omp_guided_loop_begin( long, long, long, long, long *, long * );
int mpc_omp_guided_loop_next( long *, long * );
void mpc_omp_guided_loop_end( void );
void mpc_omp_guided_loop_end_nowait( void );

int mpc_omp_ordered_guided_loop_begin( long, long, long, long, long *, long * );
int mpc_omp_ordered_guided_loop_next( long *, long * );
void mpc_omp_ordered_guided_loop_end( void );
void mpc_omp_ordered_guided_loop_end_nowait( void );
int mpc_omp_loop_ull_guided_begin( bool, unsigned long long, unsigned long long,
                                    unsigned long long, unsigned long long,
                                    unsigned long long *, unsigned long long * );
int mpc_omp_loop_ull_guided_next( unsigned long long *, unsigned long long * );
void _mpc_omp_guided_loop_ull_end( void );
void _mpc_omp_guided_loop_ull_end_nowait( void );
int mpc_omp_loop_ull_ordered_guided_begin(
    bool, unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long *, unsigned long long * );
int mpc_omp_loop_ull_ordered_guided_next( unsigned long long *,
        unsigned long long * );

/****************
 * LOOP DYNAMIC *
 ****************/

struct mpc_omp_thread_s;
struct mpc_omp_instance_s;

void _mpc_omp_dynamic_loop_init( struct mpc_omp_thread_s *, long, long, long,
                                 long );

int mpc_omp_dynamic_loop_begin( long, long, long, long, long *, long * );
int mpc_omp_dynamic_loop_next( long *, long * );
int mpc_omp_loop_ull_dynamic_begin( bool, unsigned long long,
                                     unsigned long long, unsigned long long,
                                     unsigned long long, unsigned long long *,
                                     unsigned long long * );
int mpc_omp_loop_ull_dynamic_next( unsigned long long *, unsigned long long * );
void mpc_omp_dynamic_loop_end( void );
void mpc_omp_dynamic_loop_end_nowait( void );
int mpc_omp_dynamic_loop_next_ignore_nowait( long *, long * );

int mpc_omp_ordered_dynamic_loop_begin( long, long, long, long, long *, long * );
int mpc_omp_ordered_dynamic_loop_next( long *, long * );
int mpc_omp_loop_ull_ordered_dynamic_begin(
    bool, unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long *, unsigned long long * );
int mpc_omp_loop_ull_ordered_dynamic_next( unsigned long long *,
        unsigned long long * );
void mpc_omp_ordered_dynamic_loop_end( void );
void mpc_omp_ordered_dynamic_loop_end_nowait( void );

void _mpc_omp_for_dyn_coherency_end_barrier( void );
void _mpc_omp_for_dyn_coherency_end_parallel_region( struct mpc_omp_instance_s * );

/****************
 * LOOP RUNTIME *
 ****************/

int mpc_omp_runtime_loop_begin( long, long, long, long *, long * );
int mpc_omp_runtime_loop_next( long *, long * );
int mpc_omp_loop_ull_runtime_begin( bool, unsigned long long,
                                     unsigned long long, unsigned long long,
                                     unsigned long long *, unsigned long long * );
int mpc_omp_loop_ull_runtime_next( unsigned long long *, unsigned long long * );
void mpc_omp_runtime_loop_end( void );
void mpc_omp_runtime_loop_end_nowait( void );

int mpc_omp_ordered_runtime_loop_begin( long, long, long, long *, long * );
int mpc_omp_ordered_runtime_loop_next( long *, long * );
int mpc_omp_loop_ull_ordered_runtime_begin( bool, unsigned long long,
        unsigned long long,
        unsigned long long,
        unsigned long long *,
        unsigned long long * );
int mpc_omp_loop_ull_ordered_runtime_next( unsigned long long *,
        unsigned long long * );
void mpc_omp_ordered_runtime_loop_end( void );
void mpc_omp_ordered_runtime_loop_end_nowait( void );

/************
 * TASKLOOP *
 ************/

void mpc_omp_taskloop(void (*)(void *), void *, void (*)(void *, void *), long,
                     long, unsigned, unsigned long, int, long, long, long);
void mpc_omp_taskloop_ull(void (*)(void *), void *, void (*)(void *, void *),
                         long, long, unsigned, unsigned long, int,
                         unsigned long long, unsigned long long,
                         unsigned long long);

/***********
 * ORDERED *
 ***********/

void _mpc_omp_ordered_begin( void );
void _mpc_omp_ordered_end( void );

#endif /* MPCOMP_LOOP_H_ */
