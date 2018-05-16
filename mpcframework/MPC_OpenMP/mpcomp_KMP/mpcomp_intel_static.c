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
/* #   - DIONISI Thomas thomas.dionisi@exascale-computing.eu              # */
/* #                                                                      # */  
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_global.h"
#include "mpcomp_intel_static.h"

#include "mpcomp_loop.h"

#include "mpcomp_loop_static_ull.h"

void __kmpc_for_static_init_4(ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 *plastiter, kmp_int32 *plower,
    kmp_int32 *pupper, kmp_int32 *pstride,
    kmp_int32 incr, kmp_int32 chunk) {

  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  int rank=t->rank;
  int num_threads=t->info.num_threads;

  // save old pupper
  int pupper_old = *pupper;


  if (num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
      (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
    return;
  }

  sctk_nodebug("[%d] %s: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
      "*plastiter=%d *pstride=%d",
      t->rank, __func__, loc->psource, schedtype, *plastiter, *plower,
      *pupper, *pstride, incr, chunk, *plastiter, *pstride);

  int trip_count;
  if(incr > 0) 
  {
    trip_count = ((*pupper - *plower) / incr) + 1;
  }
  else 
  {
    trip_count = ((*plower - *pupper) / ( -incr)) +1;
  }

  switch (schedtype) {
    case kmp_sch_static: {                         
      if (trip_count <= num_threads) {
        if(rank < trip_count) {
          *pupper = *plower = *plower + rank * incr;
        } else {
          *plower = *pupper + incr;
          return;
        }
        if(plastiter != NULL)
          *plastiter = (rank == trip_count -1);
        return ;
      }

      else {
        int chunk_size = trip_count / num_threads;
        int extras = trip_count % num_threads;
        if (rank < extras) {
          /* The first part is homogeneous with a chunk size a little bit larger */
          *pupper = *plower + (rank + 1) * (chunk_size +1) * incr - incr;
          *plower = *plower + rank * (chunk_size + 1) * incr;
        } else {
          /* The final part has a smaller chunk_size, therefore the computation is
             splitted in 2 parts */
          *pupper = *plower + extras * (chunk_size + 1) * incr +
            (rank + 1 - extras) * chunk_size * incr - incr;        
          *plower = *plower + extras * (chunk_size + 1) * incr +
            (rank - extras) * chunk_size * incr;
        }
        /* Remarks:
           - MPC chunk has not-inclusive upper bound while Intel runtime includes
           upper bound for calculation
           - Need to cast from long to int because MPC handles everything has a
           long
           (like GCC) while Intel creates different functions
           */
        if ( plastiter != NULL )
        {
          if ( incr > 0 )
          {
            *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
          } else {
            *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
          }
        }
      }

      break;
    }
    case kmp_sch_static_chunked: {
      chunk = (chunk < 1) ? 1 : chunk;
      *pstride = ( chunk * incr ) * t->info.num_threads;
      *plower = *plower + ( ( chunk * incr ) * t->rank );
      *pupper = *plower + ( chunk * incr ) - incr;
      // plastiter computation
      if ( plastiter != NULL )
        *plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
      break;
    }
    default:
      not_implemented();
      break;
  }
}

void __kmpc_for_static_init_4u(ident_t *loc, kmp_int32 gtid,
    kmp_int32 schedtype, kmp_int32 *plastiter,
    kmp_uint32 *plower, kmp_uint32 *pupper,
    kmp_int32 *pstride, kmp_int32 incr,
    kmp_int32 chunk) {
  mpcomp_thread_t *t;
  mpcomp_loop_gen_info_t *loop_infos;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  int rank=t->rank;
  int num_threads=t->info.num_threads;

  // save old pupper
  kmp_uint32 pupper_old = *pupper;


  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
      (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

    return;
  }
  loop_infos = &(t->info.loop_infos);
  __mpcomp_loop_gen_infos_init_ull(
      loop_infos, (unsigned long long)*plower,
      (unsigned long long)*pupper + (unsigned long long)incr,
      (unsigned long long)incr, (unsigned long long)chunk);

  sctk_nodebug("[%d] __kmpc_for_static_init_4u: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
      "*plastiter=%d *pstride=%d",
      ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
      *plastiter, *pstride);

  unsigned long long trip_count;
  if(incr > 0) 
  {
    trip_count = ((*pupper - *plower) / incr) + 1;
  }
  else 
  {
    trip_count = ((*plower - *pupper) / ( -incr)) +1;
  }

  switch (schedtype) {
    case kmp_sch_static: {                         
      if (trip_count <= num_threads) {
        if(rank < trip_count) {
          *pupper = *plower = *plower + rank * incr;
        } else {
          *plower = *pupper + incr;
          return;
        }
        if(plastiter != NULL)
          *plastiter = (rank == trip_count -1);
        return ;
      }

      else {
        int chunk_size = trip_count / num_threads;
        int extras = trip_count % num_threads;
        if (rank < extras) {
          /* The first part is homogeneous with a chunk size a little bit larger */
          *pupper = *plower + (rank + 1) * (chunk_size +1) * incr - incr;
          *plower = *plower + rank * (chunk_size + 1) * incr;
        } else {
          /* The final part has a smaller chunk_size, therefore the computation is
             splitted in 2 parts */
          *pupper = *plower + extras * (chunk_size + 1) * incr +
            (rank + 1 - extras) * chunk_size * incr - incr;        
          *plower = *plower + extras * (chunk_size + 1) * incr +
            (rank - extras) * chunk_size * incr;
        }
        /* Remarks:
           - MPC chunk has not-inclusive upper bound while Intel runtime includes
           upper bound for calculation
           - Need to cast from long to int because MPC handles everything has a
           long
           (like GCC) while Intel creates different functions
           */
        if ( plastiter != NULL )
        {
          if ( incr > 0 )
          {
            *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
          } else {
            *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
          }
        }
      }

      break;
    }
    case kmp_sch_static_chunked: {
      chunk = (chunk < 1) ? 1 : chunk;
      *pstride = ( chunk * incr ) * t->info.num_threads;
      *plower = *plower + ( ( chunk * incr ) * t->rank );
      *pupper = *plower + ( chunk * incr ) - incr;
      // plastiter computation
      if ( plastiter != NULL )
        *plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
      break;
    }
    default:
      not_implemented();
      break;

      sctk_nodebug("[%d] Results: "
          "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
          t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
          *plastiter);

  }
}

void __kmpc_for_static_init_8(ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
    kmp_int32 *plastiter, kmp_int64 *plower,
    kmp_int64 *pupper, kmp_int64 *pstride,
    kmp_int64 incr, kmp_int64 chunk) {

  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  int rank=t->rank;
  int num_threads=t->info.num_threads;

  // save old pupper
  kmp_int64 pupper_old = *pupper;

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
      (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

    return;
  }

  sctk_nodebug("[%d] __kmpc_for_static_init_8: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
      "*plastiter=%d *pstride=%d",
      ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
      *plastiter, *pstride);

  long long trip_count;
  if(incr > 0) 
  {
    trip_count = ((*pupper - *plower) / incr) + 1;
  }
  else 
  {
    trip_count = ((*plower - *pupper) / ( -incr)) +1;
  }

  switch (schedtype) {
    case kmp_sch_static: {                         
      if (trip_count <= num_threads) {
        if(rank < trip_count) {
          *pupper = *plower = *plower + rank * incr;
        } else {
          *plower = *pupper + incr;
          return;
        }
        if(plastiter != NULL)
          *plastiter = (rank == trip_count -1);
        return ;
      }

      else {
        long long chunk_size = trip_count / num_threads;
        int extras = trip_count % num_threads;
        if (rank < extras) {
          /* The first part is homogeneous with a chunk size a little bit larger */
          *pupper = *plower + (rank + 1) * (chunk_size +1) * incr - incr;
          *plower = *plower + rank * (chunk_size + 1) * incr;
        } else {
          /* The final part has a smaller chunk_size, therefore the computation is
             splitted in 2 parts */
          *pupper = *plower + extras * (chunk_size + 1) * incr +
            (rank + 1 - extras) * chunk_size * incr - incr;        
          *plower = *plower + extras * (chunk_size + 1) * incr +
            (rank - extras) * chunk_size * incr;
        }
        /* Remarks:
           - MPC chunk has not-inclusive upper bound while Intel runtime includes
           upper bound for calculation
           - Need to cast from long to int because MPC handles everything has a
           long
           (like GCC) while Intel creates different functions
           */
        if ( plastiter != NULL )
        {
          if ( incr > 0 )
          {
            *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
          } else {
            *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
          }
        }
      }

      break;
    }
    case kmp_sch_static_chunked: {
      chunk = (chunk < 1) ? 1 : chunk;
      *pstride = ( chunk * incr ) * t->info.num_threads;
      *plower = *plower + ( ( chunk * incr ) * t->rank );
      *pupper = *plower + ( chunk * incr ) - incr;
      // plastiter computation
      if ( plastiter != NULL )
        *plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
      break;
    }
    default:
      not_implemented();
      break;

      sctk_nodebug("[%d] Results: "
          "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
          t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
          *plastiter);

  }
}

void __kmpc_for_static_init_8u(ident_t *loc, kmp_int32 gtid,
    kmp_int32 schedtype, kmp_int32 *plastiter,
    kmp_uint64 *plower, kmp_uint64 *pupper,
    kmp_int64 *pstride, kmp_int64 incr,
    kmp_int64 chunk) {
  /* TODO: the same as unsigned long long in GCC... */

  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  int rank=t->rank;
  int num_threads=t->info.num_threads;
  mpcomp_loop_gen_info_t *loop_infos;
  kmp_uint64 pupper_old = *pupper;

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
      (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

    return;
  }

  loop_infos = &(t->info.loop_infos);
  __mpcomp_loop_gen_infos_init_ull(
      loop_infos, (unsigned long long)*plower,
      (unsigned long long)*pupper + (unsigned long long)incr,
      (unsigned long long)incr, (unsigned long long)chunk);

  sctk_nodebug("[%d] __kmpc_for_static_init_8u: <%s> "
      "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
      "*plastiter=%d *pstride=%d",
      ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
      schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
      *plastiter, *pstride);

  unsigned long long trip_count;
  if(incr > 0) 
  {
    trip_count = ((*pupper - *plower) / incr) + 1;
  }
  else 
  {
    trip_count = ((*plower - *pupper) / ( -incr)) +1;
  }

  switch (schedtype) {
    case kmp_sch_static: {                         
      if (trip_count <= num_threads) {
        if(rank < trip_count) {
          *pupper = *plower = *plower + rank * incr;
        } else {
          *plower = *pupper + incr;
          return;
        }
        if(plastiter != NULL)
          *plastiter = (rank == trip_count -1);
        return ;
      }

      else {
        unsigned long long chunk_size = trip_count / num_threads;
        int extras = trip_count % num_threads;
        if (rank < extras) {
          /* The first part is homogeneous with a chunk size a little bit larger */
          *pupper = *plower + (rank + 1) * (chunk_size +1) * incr - incr;
          *plower = *plower + rank * (chunk_size + 1) * incr;
        } else {
          /* The final part has a smaller chunk_size, therefore the computation is
             splitted in 2 parts */
          *pupper = *plower + extras * (chunk_size + 1) * incr +
            (rank + 1 - extras) * chunk_size * incr - incr;        
          *plower = *plower + extras * (chunk_size + 1) * incr +
            (rank - extras) * chunk_size * incr;
        }
        /* Remarks:
           - MPC chunk has not-inclusive upper bound while Intel runtime includes
           upper bound for calculation
           - Need to cast from long to int because MPC handles everything has a
           long
           (like GCC) while Intel creates different functions
           */
        if ( plastiter != NULL )
        {
          if ( incr > 0 )
          {
            *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
          } else {
            *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
          }
        }
      }

      break;
    }
    case kmp_sch_static_chunked: {
      chunk = (chunk < 1) ? 1 : chunk;
      *pstride = ( chunk * incr ) * t->info.num_threads;
      *plower = *plower + ( ( chunk * incr ) * t->rank );
      *pupper = *plower + ( chunk * incr ) - incr;
      // plastiter computation
      if ( plastiter != NULL )
        *plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
      break;
    }
    default:
      not_implemented();
      break;

      sctk_nodebug("[%d] Results: "
          "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
          t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
          *plastiter);

  }
}

void __kmpc_for_static_fini(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> None", __func__);
}
