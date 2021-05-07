/*------------------------------------------------------------------------*/
/*-------------------------- MPCOMP GOMP API NAMES------------------------*/

//All"GOMP_1.0"symbols
#ifdef NO_OPTIMIZED_GOMP_1_0_API_SUPPORT
GOMP_ABI_FUNC(GOMP_atomic_end, "GOMP_1.0", mpcomp_GOMP_atomic_end,
              __mpcomp_atomic_end)
GOMP_ABI_FUNC(GOMP_atomic_start, "GOMP_1.0", mpcomp_GOMP_atomic_start,
              __mpcomp_atomic_begin)
GOMP_ABI_FUNC(GOMP_barrier, "GOMP_1.0", mpcomp_GOMP_barrier, __mpcomp_barrier)
GOMP_ABI_FUNC(GOMP_critical_end, "GOMP_1.0", mpcomp_GOMP_critical_end,
              __mpcomp_anonymous_critical_end)
GOMP_ABI_FUNC(GOMP_critical_name_end, "GOMP_1.0", mpcomp_GOMP_critical_name_end,
              __mpcomp_named_critical_end)
GOMP_ABI_FUNC(GOMP_critical_name_start, "GOMP_1.0",
              mpcomp_GOMP_critical_name_start, __mpcomp_named_critical_begin)
GOMP_ABI_FUNC(GOMP_critical_start, "GOMP_1.0", mpcomp_GOMP_critical_start,
              __mpcomp_anonymous_critical_begin)
GOMP_ABI_FUNC(GOMP_ordered_end, "GOMP_1.0", mpcomp_GOMP_ordered_end,
              __mpcomp_ordered_end)
GOMP_ABI_FUNC(GOMP_ordered_start, "GOMP_1.0", mpcomp_GOMP_ordered_start,
              __mpcomp_ordered_begin)
#endif /* OPTIMIZED_GOMP_4_0_API_SUPPORT */

GOMP_ABI_FUNC(GOMP_loop_dynamic_next, "GOMP_1.0", mpcomp_GOMP_loop_dynamic_next,
              __mpcomp_dynamic_loop_next)
GOMP_ABI_FUNC(GOMP_loop_dynamic_start, "GOMP_1.0",
              mpcomp_GOMP_loop_dynamic_start, mpcomp_GOMP_loop_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_end, "GOMP_1.0", mpcomp_GOMP_loop_end,
              mpcomp_GOMP_loop_end)
GOMP_ABI_FUNC(GOMP_loop_end_nowait, "GOMP_1.0", mpcomp_GOMP_loop_end_nowait,
              mpcomp_GOMP_loop_end)
GOMP_ABI_FUNC(GOMP_loop_guided_next, "GOMP_1.0", mpcomp_GOMP_loop_guided_next,
              __mpcomp_guided_loop_next)
GOMP_ABI_FUNC(GOMP_loop_guided_start, "GOMP_1.0", mpcomp_GOMP_loop_guided_start,
              mpcomp_GOMP_loop_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_dynamic_next, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_dynamic_next,
              __mpcomp_ordered_dynamic_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_dynamic_start, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_dynamic_start,
              mpcomp_GOMP_loop_ordered_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_guided_next, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_guided_next,
              __mpcomp_ordered_guided_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_guided_start, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_guided_start,
              mpcomp_GOMP_loop_ordered_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_runtime_next, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_runtime_next,
              __mpcomp_ordered_runtime_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_runtime_start, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_runtime_start,
              __mpcomp_ordered_runtime_loop_begin)
GOMP_ABI_FUNC(GOMP_loop_ordered_static_next, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_static_next,
              __mpcomp_ordered_static_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_static_start, "GOMP_1.0",
              mpcomp_GOMP_loop_ordered_static_start,
              mpcomp_GOMP_loop_ordered_static_start)
GOMP_ABI_FUNC(GOMP_loop_runtime_next, "GOMP_1.0", mpcomp_GOMP_loop_runtime_next,
              __mpcomp_runtime_loop_next)
GOMP_ABI_FUNC(GOMP_loop_runtime_start, "GOMP_1.0",
              mpcomp_GOMP_loop_runtime_start, __mpcomp_runtime_loop_begin)
GOMP_ABI_FUNC(GOMP_loop_static_next, "GOMP_1.0", mpcomp_GOMP_loop_static_next,
              __mpcomp_static_loop_next)
GOMP_ABI_FUNC(GOMP_loop_static_start, "GOMP_1.0", mpcomp_GOMP_loop_static_start,
              mpcomp_GOMP_loop_static_start)
GOMP_ABI_FUNC(GOMP_parallel_end, "GOMP_1.0", mpcomp_GOMP_parallel_end,
              mpcomp_GOMP_parallel_end)
GOMP_ABI_FUNC(GOMP_parallel_loop_dynamic_start, "GOMP_1.0",
              mpcomp_GOMP_parallel_loop_dynamic_start,
              mpcomp_GOMP_parallel_loop_dynamic_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_guided_start, "GOMP_1.0",
              mpcomp_GOMP_parallel_loop_guided_start,
              mpcomp_GOMP_parallel_loop_guided_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_runtime_start, "GOMP_1.0",
              mpcomp_GOMP_parallel_loop_runtime_start,
              mpcomp_GOMP_parallel_loop_runtime_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_static_start, "GOMP_1.0",
              mpcomp_GOMP_parallel_loop_static_start,
              mpcomp_GOMP_parallel_loop_static_start)
GOMP_ABI_FUNC(GOMP_parallel_sections_start, "GOMP_1.0",
              mpcomp_GOMP_parallel_sections_start,
              mpcomp_GOMP_parallel_sections_start)
GOMP_ABI_FUNC(GOMP_parallel_start, "GOMP_1.0", mpcomp_GOMP_parallel_start,
              mpcomp_GOMP_parallel_start)
GOMP_ABI_FUNC(GOMP_sections_end, "GOMP_1.0", mpcomp_GOMP_sections_end,
              mpcomp_GOMP_sections_end)
GOMP_ABI_FUNC(GOMP_sections_end_nowait, "GOMP_1.0",
              mpcomp_GOMP_sections_end_nowait, mpcomp_GOMP_sections_end_nowait)
GOMP_ABI_FUNC(GOMP_sections_next, "GOMP_1.0", mpcomp_GOMP_sections_next,
              __mpcomp_sections_next)
GOMP_ABI_FUNC(GOMP_sections_start, "GOMP_1.0", mpcomp_GOMP_sections_start,
              __mpcomp_sections_begin)
#ifdef NO_OPTIMIZED_GOMP_1_0_API_SUPPORT
GOMP_ABI_FUNC(GOMP_single_copy_end, "GOMP_1.0", mpcomp_GOMP_single_copy_end,
              __mpcomp_do_single_copyprivate_end)
GOMP_ABI_FUNC(GOMP_single_copy_start, "GOMP_1.0", mpcomp_GOMP_single_copy_start,
              __mpcomp_do_single_copyprivate_begin)
GOMP_ABI_FUNC(GOMP_single_start, "GOMP_1.0", mpcomp_GOMP_single_start,
              __mpcomp_do_single)
#endif /* OPTIMIZED_GOMP_4_0_API_SUPPORT */

//All"GOMP_2.0"symbols
GOMP_ABI_FUNC(GOMP_task, "GOMP_2.0", mpcomp_GOMP_task, mpcomp_GOMP_task)

GOMP_ABI_FUNC(GOMP_loop_ull_dynamic_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_dynamic_next, __mpcomp_loop_ull_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_ull_dynamic_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_dynamic_start,
              mpcomp_GOMP_loop_ull_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_guided_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_guided_next, __mpcomp_loop_ull_guided_next)
GOMP_ABI_FUNC(GOMP_loop_ull_guided_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_guided_start,
              mpcomp_GOMP_loop_ull_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_dynamic_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_dynamic_next,
              __mpcomp_loop_ull_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_dynamic_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_dynamic_start,
              mpcomp_GOMP_loop_ull_ordered_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_guided_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_guided_next,
              __mpcomp_loop_ull_ordered_guided_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_guided_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_guided_start,
              mpcomp_GOMP_loop_ull_ordered_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_runtime_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_runtime_next,
              __mpcomp_loop_ull_ordered_runtime_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_runtime_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_runtime_start,
              mpcomp_GOMP_loop_ull_ordered_runtime_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_static_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_static_next,
              __mpcomp_loop_ull_ordered_static_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_static_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_ordered_static_start,
              mpcomp_GOMP_loop_ull_ordered_static_start)
GOMP_ABI_FUNC(GOMP_loop_ull_runtime_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_runtime_next, __mpcomp_loop_ull_runtime_next)
GOMP_ABI_FUNC(GOMP_loop_ull_runtime_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_runtime_start,
              mpcomp_GOMP_loop_ull_runtime_start)
GOMP_ABI_FUNC(GOMP_loop_ull_static_next, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_static_next, __mpcomp_loop_ull_static_next)
GOMP_ABI_FUNC(GOMP_loop_ull_static_start, "GOMP_2.0",
              mpcomp_GOMP_loop_ull_static_start,
              mpcomp_GOMP_loop_ull_static_start)
GOMP_ABI_FUNC(GOMP_taskwait, "GOMP_2.0", mpcomp_GOMP_taskwait,
              _mpc_task_newwait)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_start, "GOMP_2.0",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_next, "GOMP_2.0",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_start, "GOMP_2.0",
              mpcomp_GOMP_loop_nonmonotonic_guided_start,
              mpcomp_GOMP_loop_nonmonotonic_guided_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_next, "GOMP_2.0",
              mpcomp_GOMP_loop_nonmonotonic_guided_next,
              mpcomp_GOMP_loop_nonmonotonic_guided_next)
//All"GOMP_3.0"symbols
GOMP_ABI_FUNC(GOMP_taskyield, "GOMP_3.0", mpcomp_GOMP_taskyield,
              _mpc_task_newyield)

//AllGOMP_4.0symbols

/*
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_BARRIER_CANCEL,GOMP_barrier_cancel,400,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_CANCEL,GOMP_cancel,401,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_CANCELLATION_POINT,GOMP_cancellation_point,402,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_LOOP_END_CANCEL,GOMP_loop_end_cancel,403,GOMP_4.0)
*/
#ifdef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
GOMP_ABI_FUNC(GOMP_parallel, "GOMP_4.0", __mpcomp_start_parallel_region,
              __mpcomp_start_parallel_region)
GOMP_ABI_FUNC(GOMP_parallel_loop_static, "GOMP_4.0",
              __mpcomp_start_parallel_static_loop,
              __mpcomp_start_parallel_static_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_dynamic, "GOMP_4.0",
              __mpcomp_start_parallel_dynamic_loop,
              __mpcomp_start_parallel_dynamic_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_guided, "GOMP_4.0",
              __mpcomp_start_parallel_guided_loop,
              __mpcomp_start_parallel_guided_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_runtime, "GOMP_4.0",
              __mpcomp_start_parallel_runtime_loop,
              __mpcomp_start_parallel_runtime_loop)
GOMP_ABI_FUNC(GOMP_parallel_sections, "GOMP_4.0",
              __mpcomp_start_sections_parallel_region,
              __mpcomp_start_sections_parallel_region)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_start, "GOMP_4.0",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_next, "GOMP_4.0",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_start, "GOMP_4.0",
              mpcomp_GOMP_loop_nonmonotonic_guided_start,
              mpcomp_GOMP_loop_nonmonotonic_guided_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_next, "GOMP_4.0",
              mpcomp_GOMP_loop_nonmonotonic_guided_next,
              mpcomp_GOMP_loop_nonmonotonic_guided_next)
GOMP_ABI_FUNC(GOMP_parallel_loop_nonmonotonic_dynamic, "GOMP_4.0",
              mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic,
              mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic)
GOMP_ABI_FUNC(GOMP_parallel_loop_nonmonotonic_guided, "GOMP_4.0",
              mpcomp_GOMP_parallel_loop_nonmonotonic_guided,
              mpcomp_GOMP_parallel_loop_nonmonotonic_guided)

#endif /* OPTIMIZED_GOMP_4_0_API_SUPPORT */

// GOMP_ABI_FUNC(GOMP_taskgroup_start, "GOMP_4.0", mpcomp_GOMP_taskgroup_start,
// mpcomp_GOMP_taskgroup_start )
// GOMP_ABI_FUNC(GOMP_taskgroup_end, "GOMP_4.0", mpcomp_GOMP_taskgroup_end,
// mpcomp_GOMP_taskgroup_end )
/*
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_SECTIONS_END_CANCEL,GOMP_sections_end_cancel,410,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET,GOMP_target,413,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_DATA,GOMP_target_data,414,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_END_DATA,GOMP_target_end_data,415,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_UPDATE,GOMP_target_update,416,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TEAMS,GOMP_teams,417,GOMP_4.0)
*/
