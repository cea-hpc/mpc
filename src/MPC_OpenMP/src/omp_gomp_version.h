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
//All"GOMP_3.0"symbols
GOMP_ABI_FUNC(GOMP_taskyield, "GOMP_3.0", mpcomp_GOMP_taskyield,
              _mpc_task_newyield)

//GOMP_4.0 symbols

GOMP_ABI_FUNC(GOMP_parallel, "GOMP_4.0", mpcomp_GOMP_parallel,
              __mpcomp_start_parallel_region)
GOMP_ABI_FUNC(GOMP_parallel_loop_static, "GOMP_4.0",
              mpcomp_GOMP_parallel_loop_static,
              __mpcomp_start_parallel_static_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_dynamic, "GOMP_4.0",
							mpcomp_GOMP_parallel_loop_dynamic,
              __mpcomp_start_parallel_dynamic_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_guided, "GOMP_4.0",
							mpcomp_GOMP_parallel_loop_guided,
              __mpcomp_start_parallel_guided_loop)
GOMP_ABI_FUNC(GOMP_parallel_loop_runtime, "GOMP_4.0",
              mpcomp_GOMP_parallel_loop_runtime,
              __mpcomp_start_parallel_runtime_loop)
GOMP_ABI_FUNC(GOMP_parallel_sections, "GOMP_4.0",
              mpcomp_GOMP_parallel_sections,
              __mpcomp_start_sections_parallel_region)

GOMP_ABI_FUNC(GOMP_barrier_cancel, "GOMP_4.0", mpcomp_GOMP_barrier_cancel,
             mpcomp_GOMP_barrier_cancel)
GOMP_ABI_FUNC(GOMP_cancel, "GOMP_4.0", mpcomp_GOMP_cancel,
              mpcomp_GOMP_cancel)
GOMP_ABI_FUNC(GOMP_cancellation_point, "GOMP_4.0", mpcomp_GOMP_cancellation_point,
              mpcomp_GOMP_cancellation_point)
GOMP_ABI_FUNC(GOMP_loop_end_cancel, "GOMP_4.0", mpcomp_GOMP_loop_end_cancel,
              mpcomp_GOMP_loop_end_cancel)
GOMP_ABI_FUNC(GOMP_sections_end_cancel, "GOMP_4.0", mpcomp_GOMP_sections_end_cancel,
              mpcomp_GOMP_sections_end_cancel)
GOMP_ABI_FUNC(GOMP_taskgroup_start, "GOMP_4.0", mpcomp_GOMP_taskgroup_start,
              mpcomp_GOMP_taskgroup_start)
GOMP_ABI_FUNC(GOMP_taskgroup_end, "GOMP_4.0", mpcomp_GOMP_taskgroup_end,
              mpcomp_GOMP_taskgroup_end)
GOMP_ABI_FUNC(GOMP_target, "GOMP_4.0", mpcomp_GOMP_target,
              mpcomp_GOMP_target)
GOMP_ABI_FUNC(GOMP_target_data, "GOMP_4.0", mpcomp_GOMP_target_data,
              mpcomp_GOMP_target_data)
GOMP_ABI_FUNC(GOMP_target_end_data, "GOMP_4.0", mpcomp_GOMP_target_end_data,
              mpcomp_GOMP_target_end_data)
GOMP_ABI_FUNC(GOMP_target_update, "GOMP_4.0", mpcomp_GOMP_target_update,
              mpcomp_GOMP_target_update)
GOMP_ABI_FUNC(GOMP_teams, "GOMP_4.0", mpcomp_GOMP_teams,
              mpcomp_GOMP_teams)
/* GOMP 4.0.1 symbols */
GOMP_ABI_FUNC(GOMP_offload_register, "GOMP_4.0.1",
              mpcomp_GOMP_offload_register,
              mpcomp_GOMP_offload_register)
GOMP_ABI_FUNC(GOMP_offload_unregister, "GOMP_4.0.1",
              mpcomp_GOMP_offload_unregister,
              mpcomp_GOMP_offload_unregister)

/* GOMP 4.5 symbols */
GOMP_ABI_FUNC(GOMP_target_ext, "GOMP_4.5", mpcomp_GOMP_target_ext,
              mpcomp_GOMP_target_ext)
GOMP_ABI_FUNC(GOMP_target_data_ext, "GOMP_4.5", mpcomp_GOMP_target_data_ext,
              mpcomp_GOMP_target_data_ext)
GOMP_ABI_FUNC(GOMP_target_update_ext, "GOMP_4.5", mpcomp_GOMP_target_update_ext,
              mpcomp_GOMP_target_update_ext)
GOMP_ABI_FUNC(GOMP_target_enter_exit_data, "GOMP_4.5", mpcomp_GOMP_target_enter_exit_data,
              mpcomp_GOMP_target_enter_exit_data)
GOMP_ABI_FUNC(GOMP_taskloop, "GOMP_4.5", mpcomp_GOMP_taskloop,
              mpcomp_taskloop)
GOMP_ABI_FUNC(GOMP_taskloop_ull, "GOMP_4.5", mpcomp_GOMP_taskloop_ull,
              mpcomp_taskloop_ull)
GOMP_ABI_FUNC(GOMP_offload_register_ver, "GOMP_4.5",
              mpcomp_GOMP_offload_register_ver,
              mpcomp_GOMP_offload_register_ver)
GOMP_ABI_FUNC(GOMP_offload_unregister_ver, "GOMP_4.5",
              mpcomp_GOMP_offload_unregister_ver,
              mpcomp_GOMP_offload_unregister_ver)

GOMP_ABI_FUNC(GOMP_loop_doacross_static_start, "GOMP_4.5",
              mpcomp_GOMP_loop_doacross_static_start,
              mpcomp_GOMP_loop_doacross_static_start)
GOMP_ABI_FUNC(GOMP_loop_doacross_dynamic_start, "GOMP_4.5",
              mpcomp_GOMP_loop_doacross_dynamic_start,
              mpcomp_GOMP_loop_doacross_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_doacross_guided_start, "GOMP_4.5",
              mpcomp_GOMP_loop_doacross_guided_start,
              mpcomp_GOMP_loop_doacross_guided_start)
GOMP_ABI_FUNC(GOMP_loop_doacross_runtime_start, "GOMP_4.5",
              mpcomp_GOMP_loop_doacross_runtime_start,
              mpcomp_GOMP_loop_doacross_runtime_start)

GOMP_ABI_FUNC(GOMP_loop_ull_doacross_static_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_doacross_static_start,
              mpcomp_GOMP_loop_ull_doacross_static_start)
GOMP_ABI_FUNC(GOMP_loop_ull_doacross_dynamic_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_doacross_dynamic_start,
              mpcomp_GOMP_loop_ull_doacross_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_doacross_guided_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_doacross_guided_start,
              mpcomp_GOMP_loop_ull_doacross_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_doacross_runtime_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_doacross_runtime_start,
              mpcomp_GOMP_loop_ull_doacross_runtime_start)

GOMP_ABI_FUNC(GOMP_doacross_post, "GOMP_4.5",
              mpcomp_GOMP_doacross_post,
              mpcomp_GOMP_doacross_post)
GOMP_ABI_FUNC(GOMP_doacross_ull_post, "GOMP_4.5",
              mpcomp_GOMP_doacross_ull_post,
              mpcomp_GOMP_doacross_ull_post)
GOMP_ABI_FUNC(GOMP_doacross_wait, "GOMP_4.5",
              mpcomp_GOMP_doacross_wait,
              mpcomp_GOMP_doacross_wait)
GOMP_ABI_FUNC(GOMP_doacross_ull_wait, "GOMP_4.5",
              mpcomp_GOMP_doacross_ull_wait,
              mpcomp_GOMP_doacross_ull_wait)

GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_start, "GOMP_4.5",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_dynamic_next, "GOMP_4.5",
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next,
              mpcomp_GOMP_loop_nonmonotonic_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_start, "GOMP_4.5",
              mpcomp_GOMP_loop_nonmonotonic_guided_start,
              mpcomp_GOMP_loop_nonmonotonic_guided_start)
GOMP_ABI_FUNC(GOMP_loop_nonmonotonic_guided_next, "GOMP_4.5",
              mpcomp_GOMP_loop_nonmonotonic_guided_next,
              mpcomp_GOMP_loop_nonmonotonic_guided_next)

GOMP_ABI_FUNC(GOMP_loop_ull_nonmonotonic_dynamic_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_start,
              mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_nonmonotonic_dynamic_next, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_next,
              mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_ull_nonmonotonic_guided_start, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_nonmonotonic_guided_start,
              mpcomp_GOMP_loop_ull_nonmonotonic_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_nonmonotonic_guided_next, "GOMP_4.5",
              mpcomp_GOMP_loop_ull_nonmonotonic_guided_next,
              mpcomp_GOMP_loop_ull_nonmonotonic_guided_next)
GOMP_ABI_FUNC(GOMP_parallel_loop_nonmonotonic_dynamic, "GOMP_4.5",
              mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic,
              mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic)
GOMP_ABI_FUNC(GOMP_parallel_loop_nonmonotonic_guided, "GOMP_4.5",
              mpcomp_GOMP_parallel_loop_nonmonotonic_guided,
              mpcomp_GOMP_parallel_loop_nonmonotonic_guided)






