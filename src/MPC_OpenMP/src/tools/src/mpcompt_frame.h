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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMPT_FRAME_H__
#define __MPCOMPT_FRAME_H__

#include "mpcompt_frame_types.h"

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
#include "mpcomp_types.h"

#ifdef MPCOMPT_FRAME_ADDRESS_SUPPORT
#define MPCOMPT_GET_FRAME_ADDRESS __builtin_frame_address( 0 )
#else
#define MPCOMPT_GET_FRAME_ADDRESS NULL
#endif

#ifdef MPCOMPT_RETURN_ADDRESS_SUPPORT
#define MPCOMPT_GET_RETURN_ADDRESS __builtin_return_address( 0 )
#else
#define MPCOMPT_GET_RETURN_ADDRESS NULL
#endif

static inline void
_mpc_omp_ompt_frame_set_exit( void* rt_exit_addr ) {
    mpc_omp_thread_t* thread;
    mpc_omp_task_t* curr_task;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );
    assert( thread->task_infos.current_task );

    /* Update current task with enter frame address */
    curr_task = thread->task_infos.current_task;
    curr_task->ompt_task_frame.exit_frame.ptr = rt_exit_addr;
}

static inline void
_mpc_omp_ompt_frame_set_enter( void* rt_enter_addr ) {
    mpc_omp_thread_t* thread;
    mpc_omp_task_t* curr_task;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( thread->task_infos.current_task ) {
        /* Update current task with enter frame address */
        curr_task = thread->task_infos.current_task;
        curr_task->ompt_task_frame.enter_frame.ptr = rt_enter_addr;
    }
    else {
        /* Early call before initial and implicit tasks are created,
         * save enter address in thread frame infos. */
        thread->frame_infos.ompt_frame_infos.enter_frame.ptr = rt_enter_addr;
    }
}

static inline void
_mpc_omp_ompt_frame_set_no_reentrant() {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    /* Indicates that frames infos have already been saved
     * from runtime outter entry function */
    thread->frame_infos.outter_caller = 1;
}

static inline void
_mpc_omp_ompt_frame_unset_no_reentrant() {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    /* Reset */
    thread->frame_infos.outter_caller = 0;
}

/*
 */
static inline mpc_omp_ompt_frame_info_t
_mpc_omp_ompt_frame_reset_infos() {
    mpc_omp_thread_t* thread;
    mpc_omp_ompt_frame_info_t prev;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    prev = thread->frame_infos;

    memset( &thread->frame_infos, 0, sizeof( mpc_omp_ompt_frame_info_t ));

    return prev;
}

static inline void
_mpc_omp_ompt_frame_set_infos( mpc_omp_ompt_frame_info_t* frame_infos ) {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    thread->frame_infos = *frame_infos;
}

/* Only used once to retrieve frame infos needed by callbacks during 
 * initialization of the runtime.
 */
static inline void
_mpc_omp_ompt_frame_get_pre_init_infos() {
    mpc_omp_thread_t* thread;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;

    if( !thread ) {
        /* Allocate thread structure to save frame infos */
        thread = (mpc_omp_thread_t*) sctk_malloc( sizeof( mpc_omp_thread_t ));
        assert( thread );
        memset( thread, 0, sizeof( mpc_omp_thread_t ));

        /* Save enter frame address */
        thread->frame_infos.ompt_frame_infos.enter_frame.ptr =
            MPCOMPT_GET_FRAME_ADDRESS;

        /* Save frame return address */
        thread->frame_infos.ompt_return_addr =
            MPCOMPT_GET_RETURN_ADDRESS;

        mpc_omp_tls = (void*) thread;
    }
}

/*
 */
static inline void
_mpc_omp_ompt_frame_get_infos() {
    mpc_omp_thread_t* thread;
    mpc_omp_task_t* curr_task;
    mpc_omp_ompt_frame_info_t* frame_infos;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;

    /* 1rst time */
    if( !thread ) {
        thread = (mpc_omp_thread_t*) sctk_malloc( sizeof( mpc_omp_thread_t ));
        assert( thread );
        memset( thread, 0, sizeof( mpc_omp_thread_t ));

        /* Save enter frame address */
        thread->frame_infos.ompt_frame_infos.enter_frame.ptr =
            MPCOMPT_GET_FRAME_ADDRESS;

        /* Save frame return address */
        thread->frame_infos.ompt_return_addr =
            MPCOMPT_GET_RETURN_ADDRESS;

        mpc_omp_tls = (void*) thread;
    }
    else {
        frame_infos = &thread->frame_infos;

        if( !frame_infos->outter_caller ) {
            /* Save enter frame address */
            curr_task = thread->task_infos.current_task;
            curr_task->ompt_task_frame.enter_frame.ptr =
                MPCOMPT_GET_FRAME_ADDRESS;

            /* Save frame return address */
            frame_infos->ompt_return_addr =
                MPCOMPT_GET_RETURN_ADDRESS;
        }
    }
}

/* Retrieves frame infos at each entry function of the runtime.
 */
static inline void
_mpc_omp_ompt_frame_get_wrapper_infos( mpc_omp_ompt_wrapper_t w ) {
    mpc_omp_thread_t* thread;
    mpc_omp_task_t* curr_task;
    mpc_omp_ompt_frame_info_t* frame_infos;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;

    /* 1rst time */
    if( !thread ) {
        thread = (mpc_omp_thread_t*) sctk_malloc( sizeof( mpc_omp_thread_t ));
        assert( thread );
        memset( thread, 0, sizeof( mpc_omp_thread_t ));

        /* Save enter frame address */
        thread->frame_infos.ompt_frame_infos.enter_frame.ptr =
            MPCOMPT_GET_FRAME_ADDRESS;

        /* Save frame return address */
        thread->frame_infos.ompt_return_addr =
            MPCOMPT_GET_RETURN_ADDRESS;

        /* 1rst entry point is a pragma directive, save wrapper info */
        thread->frame_infos.omp_wrapper = w;

        mpc_omp_tls = (void*) thread;
    }
    else {
        frame_infos = &thread->frame_infos;

        /* Update wrapper info */
        if( frame_infos->omp_wrapper == MPCOMP_UNDEF )
            frame_infos->omp_wrapper = w;

        if( frame_infos->omp_wrapper == w
            && !frame_infos->outter_caller ) {
            /* Save enter frame address */
            curr_task = thread->task_infos.current_task;
            curr_task->ompt_task_frame.enter_frame.ptr =
                MPCOMPT_GET_FRAME_ADDRESS;

            /* Save frame return address */
            frame_infos->ompt_return_addr =
                MPCOMPT_GET_RETURN_ADDRESS;
        }
    }
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_FRAME_H__ */
