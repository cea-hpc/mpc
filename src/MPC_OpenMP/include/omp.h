/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:25:57 CEST 2021                                        # */
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
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* #                                                                      # */
/* ######################################################################## */

/**
 * This file contains standard OpenMP API prototypes
 */

#ifndef __OMP_H__
#define __OMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <errno.h>
#include <opa_primitives.h>
#include <pthread.h>
#include <stdint.h>

    /* OpenMP 2.5 API */
    void omp_set_num_threads( int num_threads );
    int omp_get_num_threads( void );
    int omp_get_max_threads( void );
    int omp_get_thread_num( void );
    int omp_get_num_procs( void );
    int omp_in_parallel( void );
    void omp_set_dynamic( int dynamic_threads );
    int omp_get_dynamic( void );
    void omp_set_nested( int nested );
    int omp_get_nested( void );

    typedef enum omp_lock_hint_t {
        omp_lock_hint_none = 0,
        omp_lock_hint_uncontended = 1,
        omp_lock_hint_contended = 2,
        omp_lock_hint_nonspeculative = 4,
        omp_lock_hint_speculative = 8
    } omp_lock_hint_t;

    typedef struct omp_lock_s
    {
        omp_lock_hint_t hint;
        OPA_int_t lock;
        void *opaque;
        uint64_t ompt_wait_id;
    } omp_lock_t;

    typedef struct omp_nest_lock_s
    {
        int nb_nested;		/* Number of times this lock is held */
        void *owner_thread; /* Owner of the lock */
        void *owner_task;   /* Owner of the lock */
        omp_lock_hint_t hint;
        OPA_int_t lock;
        void *opaque;
        uint64_t ompt_wait_id;
    } omp_nest_lock_t;

    typedef enum omp_event_handle_t
    {
        __omp_event_handle_t_max__ = UINTPTR_MAX
    } omp_event_handle_t;
    void omp_fulfill_event(omp_event_handle_t event);
    omp_event_handle_t omp_task_continuation_event(void);

    /* Lock Functions */
    void omp_init_lock( omp_lock_t *lock );
    void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint );
    void omp_destroy_lock( omp_lock_t *lock );
    void omp_set_lock( omp_lock_t *lock );
    void omp_unset_lock( omp_lock_t *lock );
    int omp_test_lock( omp_lock_t *lock );

    /* Nestable Lock Fuctions */
    void omp_init_nest_lock( omp_nest_lock_t *lock );
    void omp_init_nest_lock_with_hint( omp_nest_lock_t *lock,
            omp_lock_hint_t hint );
    void omp_destroy_nest_lock( omp_nest_lock_t *lock );
    void omp_set_nest_lock( omp_nest_lock_t *lock );
    void omp_unset_nest_lock( omp_nest_lock_t *lock );
    int omp_test_nest_lock( omp_nest_lock_t *lock );

    /* Timing Routines */
    double omp_get_wtime( void );
    double omp_get_wtick( void );

    /* OpenMP 3.0 API */
    typedef enum omp_sched_t {
        omp_sched_static = 1,
        omp_sched_dynamic = 2,
        omp_sched_guided = 3,
        omp_sched_auto = 4,
    } omp_sched_t;
    void omp_set_schedule( omp_sched_t kind, int modifier );
    void omp_get_schedule( omp_sched_t *kind, int *modifier );
    int omp_get_thread_limit( void );
    void omp_set_max_active_levels( int max_levels );
    int omp_get_max_active_levels( void );
    int omp_get_level( void );
    int omp_get_ancestor_thread_num( int level );
    int omp_get_team_size( int level );
    int omp_get_active_level( void );
    int omp_in_final();

    /* OpenMP 5.0 API */
    typedef enum omp_control_tool_result_e {
        omp_control_tool_notool = -2,
        omp_control_tool_nocallback = -1,
        omp_control_tool_success = 0,
        omp_control_tool_ignored = 1
    } omp_control_tool_result_t;

    typedef enum omp_control_tool_t {
        omp_control_tool_start = 1,
        omp_control_tool_pause = 2,
        omp_control_tool_flush = 3,
        omp_control_tool_end = 4
    } omp_control_tool_t;

    int omp_control_tool(int command, int modifier, void * arg);

    /* OpenMP 5.0 API */

    /*
     * define memory management types
     */

    typedef uintptr_t omp_uintptr_t;

    typedef enum omp_memspace_handle_t {
        omp_default_mem_space,
        omp_large_cap_mem_space,
        omp_const_mem_space,
        omp_high_bw_mem_space,
        omp_low_lat_mem_space
            /* Add vendor specific constants for memory spaces here.  */
    } omp_memspace_handle_t;

    typedef enum omp_allocator_handle_t {
        omp_null_allocator = 0,
        /* The rest of the enumerators have
           implementation specific values.  */
        // omp_default_mem_alloc = -12,
        omp_default_mem_alloc, // Utile ?
        omp_large_cap_mem_alloc,
        omp_const_mem_alloc,
        omp_high_bw_mem_alloc,
        omp_low_lat_mem_alloc,
        omp_cgroup_mem_alloc,
        omp_pteam_mem_alloc,
        omp_thread_mem_alloc
            /* Some range for dynamically allocated handles.  */
    } omp_allocator_handle_t;

    typedef enum omp_alloctrait_key_t {
        omp_atk_sync_hint = 1,
        omp_atk_alignment = 2,
        omp_atk_access = 3,
        omp_atk_pool_size = 4,
        omp_atk_fallback = 5,
        omp_atk_fb_data = 6,
        omp_atk_pinned = 7,
        omp_atk_partition = 8
    } omp_alloctrait_key_t;

    typedef enum omp_alloctrait_value_t {
        omp_atv_false = 0,
        omp_atv_true = 1,
        omp_atv_default = 2,
        omp_atv_contended = 3,
        omp_atv_uncontended = 4,
        omp_atv_sequential = 5,
        omp_atv_private = 6,
        omp_atv_all = 7,
        omp_atv_thread = 8,
        omp_atv_pteam = 9,
        omp_atv_cgroup = 10,
        omp_atv_default_mem_fb = 11,
        omp_atv_null_fb = 12,
        omp_atv_abort_fb = 13,
        omp_atv_allocator_fb = 14,
        omp_atv_environment = 15,
        omp_atv_nearest = 16,
        omp_atv_blocked = 17,
        omp_atv_interleaved = 18
    } omp_alloctrait_value_t;

    typedef struct omp_alloctrait_t {
        omp_alloctrait_key_t key;
        omp_uintptr_t value;
    } omp_alloctrait_t;

    omp_allocator_handle_t omp_init_allocator (
            omp_memspace_handle_t memspace,
            int ntraits,
            const omp_alloctrait_t traits[]
            );

    void omp_destroy_allocator (omp_allocator_handle_t allocator);
    void omp_set_default_allocator (omp_allocator_handle_t allocator);
    omp_allocator_handle_t omp_get_default_allocator (void);
    void *omp_alloc (size_t size, omp_allocator_handle_t allocator);
    void omp_free (void *ptr, omp_allocator_handle_t allocator);

    /** depobj */
    typedef struct  omp_depend_t
    {
        char __omp_depend_t__[2 * sizeof (void *)];
    }               omp_depend_t;

#ifdef __cplusplus
}
#endif

#endif /* __OMP_H__ */
