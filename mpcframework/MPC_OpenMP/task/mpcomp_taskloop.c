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

#include "sctk_bool.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_task.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_taskgroup.h"
#include "mpcomp_task_gomp_constants.h"

static unsigned long 
mpcomp_taskloop_compute_num_iters( long start, long end, long step )
{
    long decal = ( step > 0 ) ? -1 : 1;

    if( (end - start) * decal >= 0 )
        return 0;
    
    return ( end - start + step + decal ) / step;
}

static unsigned long long
mpcomp_taskloop_compute_num_iters_ull( unsigned long long start, unsigned long long end, unsigned long long step, bool up )
{
    unsigned long long ret = (unsigned long long) 0;
    ret = (  up && start < end ) ? ( end - start + step - (unsigned long long) 1 ) / step : ret;
    ret = ( !up && start > end ) ? ( start - end - step - (unsigned long long) 1 ) / -step : ret; 
    return ret; 
}

static unsigned long
mpcomp_taskloop_compute_loop_value( long iteration_num, unsigned long num_tasks, long step, long* taskstep, unsigned long* extra_chunk)
{
    long compute_taskstep;
    mpcomp_thread_t* omp_thread_tls;
    unsigned long grainsize, compute_num_tasks, compute_extra_chunk;

	omp_thread_tls = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( omp_thread_tls );

    compute_taskstep = step;
    compute_extra_chunk = iteration_num;
    compute_num_tasks = num_tasks;

    if( !(compute_num_tasks) )
    {
       compute_num_tasks = ( omp_thread_tls->info.num_threads ) ? omp_thread_tls->info.num_threads : 1;
    }

    if( num_tasks >= iteration_num )
    {
        compute_num_tasks = iteration_num;
    }
    else
    {
        const long quotient = iteration_num / compute_num_tasks;
        const long reste = iteration_num % compute_num_tasks;
        compute_taskstep = quotient * step;
        if( reste )
        {
            compute_taskstep += step;
            compute_extra_chunk = reste -1;
        }
    }

    *taskstep = compute_taskstep;
    *extra_chunk = compute_extra_chunk;
    return compute_num_tasks;
}

static unsigned long
mpcomp_taskloop_compute_loop_value_grainsize( long iteration_num, unsigned long num_tasks, long step, long* taskstep, unsigned long* extra_chunk)
{
    long compute_taskstep;
    unsigned long grainsize, compute_num_tasks, compute_extra_chunk;

    grainsize = num_tasks;

    compute_taskstep = step;
    compute_extra_chunk = iteration_num;
    compute_num_tasks = iteration_num / grainsize;
    
    if( compute_num_tasks <= 1 )
    {
        compute_num_tasks = 1;
    }
    else
    {
        if( compute_num_tasks > grainsize )
        {
            const long mul = num_tasks * grainsize;
            const long reste = iteration_num - mul;
            compute_taskstep = grainsize * step;
            if( reste )
            {
                compute_taskstep += step;
                compute_extra_chunk = iteration_num - mul - 1;
            } 
        }
        else
        {
            const long quotient = iteration_num / compute_num_tasks;
            const long reste = iteration_num % compute_num_tasks;
            compute_taskstep = quotient * step;
            if( reste )
            {
                compute_taskstep += step;
                compute_extra_chunk = reste -1;
            }
        }
    }

    *taskstep = compute_taskstep;
    *extra_chunk = compute_extra_chunk;
    return compute_num_tasks;
}


void mpcomp_taskloop( void (*fn)(void*), void* data, void(*cpyfn)(void*, void*), long arg_size, long arg_align, unsigned flags, unsigned long num_tasks, int priority, long start, long end, long step)
{
    long taskstep;
    unsigned long extra_chunk, grainsize, i;

    mpcomp_init();

    const long num_iters = mpcomp_taskloop_compute_num_iters( start, end, step );

    if( !( num_iters ) )
    {
        return;
    }

    if( !( flags & MPCOMP_TASK_FLAG_GRAINSIZE ) )
    {
        num_tasks = mpcomp_taskloop_compute_loop_value(num_iters, num_tasks, step, &taskstep, &extra_chunk);
    } 
    else
    {
        num_tasks = mpcomp_taskloop_compute_loop_value_grainsize(num_iters, num_tasks, step, &taskstep, &extra_chunk);
        taskstep = ( num_tasks == 1 ) ? end - start : taskstep;
    }

    if( !( flags & MPCOMP_TASK_FLAG_NOGROUP ) )
    {
        mpcomp_taskgroup_start();
    }
  
    for( i = 0; i < num_tasks; i++)  
    {
        mpcomp_task_t* new_task = NULL;
        new_task = __mpcomp_task_alloc( fn, data, cpyfn, arg_size, arg_align, 0, flags, 0 /* no deps */ ); 
        ((long*)new_task->data)[0] = start;
        ((long*)new_task->data)[1] = start + taskstep;
        start += taskstep;
        taskstep -= ( i == extra_chunk ) ? step : 0;
        __mpcomp_task_process(  new_task, 0 );
    }
    
    if( !( flags & MPCOMP_TASK_FLAG_NOGROUP ) )
    {
        mpcomp_taskgroup_end();
    }
}

void mpcomp_taskloop_ull( void (*fn)(void*), void* data, void(*cpyfn)(void*, void*), long arg_size, long arg_align, unsigned flags, unsigned long num_tasks, int priority, unsigned long long start, unsigned long long end, unsigned long long step)
{

}


/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver mpcomp_taskloop, GOMP_taskloop@@GOMP_4.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
