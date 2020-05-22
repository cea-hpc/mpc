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
__mpcomp_loop_gen_infos_init( mpcomp_loop_gen_info_t *loop_infos, long lb,
                              long b, long incr, long chunk_size )
{
	sctk_assert( loop_infos );
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
__mpcomp_loop_gen_infos_init_ull( mpcomp_loop_gen_info_t *loop_infos,
                                  unsigned long long lb, unsigned long long b,
                                  unsigned long long incr,
                                  unsigned long long chunk_size )
{
	sctk_assert( loop_infos );
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
__mpcomp_loop_gen_loop_infos_cpy( mpcomp_loop_gen_info_t *in,
                                  mpcomp_loop_gen_info_t *out )
{
	memcpy( out, in, sizeof( mpcomp_loop_gen_info_t ) );
}

static inline void
__mpcomp_loop_gen_loop_infos_reset( mpcomp_loop_gen_info_t *loop )
{
	memset( loop, 0, sizeof( mpcomp_loop_gen_info_t ) );
}

/***************
 * LOOP STATIC *
 ***************/

struct mpcomp_thread_s;

void __mpcomp_static_loop_init( struct mpcomp_thread_s *, long, long, long,
                                long );


int __mpcomp_static_loop_begin( long, long, long, long, long *, long * );
int __mpcomp_static_loop_next( long *, long * );
void __mpcomp_static_loop_end( void );
void __mpcomp_static_loop_end_nowait( void );

int __mpcomp_ordered_static_loop_begin( long, long b, long, long, long *,
                                        long * );
int __mpcomp_ordered_static_loop_next( long *, long * );
void __mpcomp_ordered_static_loop_end( void );
void __mpcomp_ordered_static_loop_end_nowait( void );

int __mpcomp_static_loop_begin_ull( bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size,
                                    unsigned long long *from,
                                    unsigned long long *to );

int __mpcomp_static_loop_next_ull( unsigned long long *from,
                                   unsigned long long *to );

int __mpcomp_ordered_static_loop_begin_ull( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *from,
        unsigned long long *to );

int __mpcomp_ordered_static_loop_next_ull( unsigned long long *from,
        unsigned long long *to );

/***************
 * LOOP GUIDED *
 ***************/

int __mpcomp_guided_loop_begin( long, long, long, long, long *, long * );
int __mpcomp_guided_loop_next( long *, long * );
void __mpcomp_guided_loop_end( void );
void __mpcomp_guided_loop_end_nowait( void );

int __mpcomp_ordered_guided_loop_begin( long, long, long, long, long *, long * );
int __mpcomp_ordered_guided_loop_next( long *, long * );
void __mpcomp_ordered_guided_loop_end( void );
void __mpcomp_ordered_guided_loop_end_nowait( void );
int __mpcomp_loop_ull_guided_begin( bool, unsigned long long, unsigned long long,
                                    unsigned long long, unsigned long long,
                                    unsigned long long *, unsigned long long * );
int __mpcomp_loop_ull_guided_next( unsigned long long *, unsigned long long * );
void __mpcomp_guided_loop_ull_end( void );
void __mpcomp_guided_loop_ull_end_nowait( void );
int __mpcomp_loop_ull_ordered_guided_begin(
    bool, unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long *, unsigned long long * );
int __mpcomp_loop_ull_ordered_guided_next( unsigned long long *,
        unsigned long long * );

/****************
 * LOOP DYNAMIC *
 ****************/

struct mpcomp_thread_s;
struct mpcomp_instance_s;

void __mpcomp_dynamic_loop_init( struct mpcomp_thread_s *, long, long, long,
                                 long );

int __mpcomp_dynamic_loop_begin( long, long, long, long, long *, long * );
int __mpcomp_dynamic_loop_next( long *, long * );
int __mpcomp_loop_ull_dynamic_begin( bool, unsigned long long,
                                     unsigned long long, unsigned long long,
                                     unsigned long long, unsigned long long *,
                                     unsigned long long * );
int __mpcomp_loop_ull_dynamic_next( unsigned long long *, unsigned long long * );
void __mpcomp_dynamic_loop_end( void );
void __mpcomp_dynamic_loop_end_nowait( void );
int __mpcomp_dynamic_loop_next_ignore_nowait( long *, long * );

int __mpcomp_ordered_dynamic_loop_begin( long, long, long, long, long *, long * );
int __mpcomp_ordered_dynamic_loop_next( long *, long * );
int __mpcomp_loop_ull_ordered_dynamic_begin(
    bool, unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long *, unsigned long long * );
int __mpcomp_loop_ull_ordered_dynamic_next( unsigned long long *,
        unsigned long long * );
void __mpcomp_ordered_dynamic_loop_end( void );
void __mpcomp_ordered_dynamic_loop_end_nowait( void );

void __mpcomp_for_dyn_coherency_end_barrier( void );
void __mpcomp_for_dyn_coherency_end_parallel_region( struct mpcomp_instance_s * );

/****************
 * LOOP RUNTIME *
 ****************/

int __mpcomp_runtime_loop_begin( long, long, long, long *, long * );
int __mpcomp_runtime_loop_next( long *, long * );
int __mpcomp_loop_ull_runtime_begin( bool, unsigned long long,
                                     unsigned long long, unsigned long long,
                                     unsigned long long *, unsigned long long * );
int __mpcomp_loop_ull_runtime_next( unsigned long long *, unsigned long long * );
void __mpcomp_runtime_loop_end( void );
void __mpcomp_runtime_loop_end_nowait( void );

int __mpcomp_ordered_runtime_loop_begin( long, long, long, long *, long * );
int __mpcomp_ordered_runtime_loop_next( long *, long * );
int __mpcomp_loop_ull_ordered_runtime_begin( bool, unsigned long long,
        unsigned long long,
        unsigned long long,
        unsigned long long *,
        unsigned long long * );
int __mpcomp_loop_ull_ordered_runtime_next( unsigned long long *,
        unsigned long long * );
void __mpcomp_ordered_runtime_loop_end( void );
void __mpcomp_ordered_runtime_loop_end_nowait( void );

/************
 * TASKLOOP *
 ************/

#if MPCOMP_TASK

void mpcomp_taskloop(void (*)(void *), void *, void (*)(void *, void *), long,
                     long, unsigned, unsigned long, int, long, long, long);
void mpcomp_taskloop_ull(void (*)(void *), void *, void (*)(void *, void *),
                         long, long, unsigned, unsigned long, int,
                         unsigned long long, unsigned long long,
                         unsigned long long);

#endif

/***********
 * ORDERED *
 ***********/

void __mpcomp_ordered_begin( void );
void __mpcomp_ordered_end( void );

#endif /* MPCOMP_LOOP_H_ */
