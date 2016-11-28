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

#ifndef __MPCOMP_LOOP_CORE_H__
#define __MPCOMP_LOOP_CORE_H__

static inline unsigned long long __mpcomp_internal_loop_get_num_iters_ull( unsigned long long start, unsigned long long end, unsigned long long step, bool up )
{
    unsigned long long ret = (unsigned long long) 0;
    ret = (  up && start < end ) ? ( end - start + step - (unsigned long long) 1 ) / step : ret;
    ret = ( !up && start > end ) ? ( start - end - step - (unsigned long long) 1 ) / -step : ret; 
    return ret; 
}

static inline long __mpcomp_internal_loop_get_num_iters( long start, long end, long step )
{
    long ret = 0;
    const bool up = ( step > 0 ); 
    const long abs_step = ( up ) ? abs_step : -abs_step;
    
    //ret = ( end - start + step + modulo_param ) / step;
    //ret = ( up   && start > end ) ? 0 : ret;
    //ret = ( !up  && start < end ) ? 0 : ret;
    ret = ( up  && start < end ) ? ( end - start + step - (long) 1 ) / step : ret;
    ret = ( !up && start > end ) ? ( start - end - step - (long) 1 ) / -step : ret;
    //sctk_error(" %s - %ld %ld %ld %ld", __func__, start, end, step, ret ); 
    return ( ret >= 0 ) ? ret : -ret ;
}


static inline void __mpcomp_loop_gen_infos_init( mpcomp_loop_gen_info_t* loop_infos, long lb, long b, long incr, long chunk_size )
{
    sctk_assert( loop_infos );

    loop_infos->type = MPCOMP_LOOP_TYPE_LONG; 
    loop_infos->loop.mpcomp_long.up = ( incr > 0 );
    loop_infos->loop.mpcomp_long.b = b;
    loop_infos->loop.mpcomp_long.lb = lb;
    loop_infos->loop.mpcomp_long.incr = incr;
    loop_infos->loop.mpcomp_long.chunk_size = chunk_size;
}

static inline void __mpcomp_loop_gen_infos_init_ull( mpcomp_loop_gen_info_t* loop_infos, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size, bool up )
{
    sctk_assert( loop_infos );

    loop_infos->type = MPCOMP_LOOP_TYPE_ULL; 
    loop_infos->loop.mpcomp_ull.up = ( incr > 0 );
    loop_infos->loop.mpcomp_ull.b = b;
    loop_infos->loop.mpcomp_ull.lb = lb;
    loop_infos->loop.mpcomp_ull.incr = incr;
    loop_infos->loop.mpcomp_ull.chunk_size = chunk_size;
}

static inline void __mpcomp_loop_gen_loop_infos_cpy( mpcomp_loop_gen_info_t* in, mpcomp_loop_gen_info_t* out )
{
    memcpy( out, in, sizeof( mpcomp_loop_gen_info_t ) );
}

unsigned long long __mpcomp_compute_static_nb_chunks_per_rank_ull(unsigned long long, unsigned long long, mpcomp_loop_ull_iter_t*);
#endif /* __MPCOMP_LOOP_CORE_H__ */

