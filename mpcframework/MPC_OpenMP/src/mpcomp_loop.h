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

#include "mpcomp_types_loop.h"

static inline unsigned long long
__mpcomp_internal_loop_get_num_iters_ull(unsigned long long start,
                                         unsigned long long end,
                                         unsigned long long step, bool up) {
  unsigned long long ret = (unsigned long long)0;
  ret = (up && start < end)
            ? (end - start + step - (unsigned long long)1) / step
            : ret;
  ret = (!up && start > end)
            ? (start - end - step - (unsigned long long)1) / -step
            : ret;
  return ret;
}

static inline long __mpcomp_internal_loop_get_num_iters(long start, long end,
                                                        long step) {
  long ret = 0;
  const bool up = (step > 0);
  const long abs_step = (up) ? abs_step : -abs_step;

  ret = (up && start < end) ? (end - start + step - (long)1) / step : ret;
  ret = (!up && start > end) ? (start - end - step - (long)1) / -step : ret;
  return (ret >= 0) ? ret : -ret;
}

static inline uint64_t __mpcomp_internal_loop_get_num_iters_gen( mpcomp_loop_gen_info_t* loop_infos )
{
	uint64_t count = 0;	

	if( loop_infos->type == MPCOMP_LOOP_TYPE_LONG )
	{
		mpcomp_loop_long_iter_t* long_loop = &( loop_infos->loop.mpcomp_long );
		count = __mpcomp_internal_loop_get_num_iters(long_loop->lb , long_loop->b, long_loop->incr );
	}
	else
	{
		mpcomp_loop_ull_iter_t* ull_loop = &( loop_infos->loop.mpcomp_ull );
		count = __mpcomp_internal_loop_get_num_iters_ull(ull_loop->lb , ull_loop->b, ull_loop->incr, ull_loop->up );
	}
	return count;
}

unsigned long long __mpcomp_get_static_nb_chunks_per_rank_ull(
    unsigned long long, unsigned long long, mpcomp_loop_ull_iter_t *);

int __mpcomp_get_static_nb_chunks_per_rank(int rank, int num_threads,
                                           mpcomp_loop_long_iter_t *loop);
#endif /* __MPCOMP_LOOP_CORE_H__ */
