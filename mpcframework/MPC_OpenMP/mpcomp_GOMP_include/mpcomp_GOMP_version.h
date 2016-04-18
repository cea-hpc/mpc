/*------------------------------------------------------------------------*/
/*-------------------------- MPCOMP GOMP API NAMES------------------------*/

//All"GOMP_1.0"symbols
GOMP_ABI_FUNC(GOMP_atomic_end,"GOMP_1.0",__mpcomp_GOMP_atomic_end,__mpcomp_atomic_end)
GOMP_ABI_FUNC(GOMP_atomic_start,"GOMP_1.0",__mpcomp_GOMP_atomic_start,__mpcomp_atomic_begin)

GOMP_ABI_FUNC(GOMP_barrier,"GOMP_1.0",__mpcomp_GOMP_barrier,__mpcomp_barrier)
GOMP_ABI_FUNC(GOMP_critical_end,"GOMP_1.0",__mpcomp_GOMP_critical_end,__mpcomp_anonymous_critical_end)
GOMP_ABI_FUNC(GOMP_critical_name_end,"GOMP_1.0",__mpcomp_GOMP_critical_name_end,__mpcomp_named_critical_end)
GOMP_ABI_FUNC(GOMP_critical_name_start,"GOMP_1.0",__mpcomp_GOMP_critical_name_start,__mpcomp_named_critical_begin)
GOMP_ABI_FUNC(GOMP_critical_start,"GOMP_1.0",__mpcomp_GOMP_critical_start,__mpcomp_anonymous_critical_begin)
GOMP_ABI_FUNC(GOMP_loop_dynamic_next,"GOMP_1.0",__mpcomp_GOMP_loop_dynamic_next,__mpcomp_dynamic_loop_next)
GOMP_ABI_FUNC(GOMP_loop_dynamic_start,"GOMP_1.0",__mpcomp_GOMP_loop_dynamic_start,__mpcomp_GOMP_loop_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_end,"GOMP_1.0",__mpcomp_GOMP_loop_end,__mpcomp_GOMP_loop_end)
GOMP_ABI_FUNC(GOMP_loop_end_nowait,"GOMP_1.0",__mpcomp_GOMP_loop_end_nowait,__mpcomp_GOMP_loop_end)
GOMP_ABI_FUNC(GOMP_loop_guided_next,"GOMP_1.0",__mpcomp_GOMP_loop_guided_next,__mpcomp_guided_loop_next)
GOMP_ABI_FUNC(GOMP_loop_guided_start,"GOMP_1.0",__mpcomp_GOMP_loop_guided_start,__mpcomp_GOMP_loop_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_dynamic_next,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_dynamic_next,__mpcomp_ordered_dynamic_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_dynamic_start,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_dynamic_start,__mpcomp_GOMP_loop_ordered_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_guided_next,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_guided_next,__mpcomp_ordered_guided_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_guided_start,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_guided_start,__mpcomp_GOMP_loop_ordered_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ordered_runtime_next,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_runtime_next,__mpcomp_ordered_runtime_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_runtime_start,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_runtime_start,__mpcomp_ordered_runtime_loop_begin)
GOMP_ABI_FUNC(GOMP_loop_ordered_static_next,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_static_next,__mpcomp_ordered_static_loop_next)
GOMP_ABI_FUNC(GOMP_loop_ordered_static_start,"GOMP_1.0",__mpcomp_GOMP_loop_ordered_static_start,__mpcomp_GOMP_loop_ordered_static_start)
GOMP_ABI_FUNC(GOMP_loop_runtime_next,"GOMP_1.0",__mpcomp_GOMP_loop_runtime_next,__mpcomp_runtime_loop_next)
GOMP_ABI_FUNC(GOMP_loop_runtime_start,"GOMP_1.0",__mpcomp_GOMP_loop_runtime_start,__mpcomp_runtime_loop_begin)
GOMP_ABI_FUNC(GOMP_loop_static_next,"GOMP_1.0",__mpcomp_GOMP_loop_static_next,__mpcomp_static_loop_next)
GOMP_ABI_FUNC(GOMP_loop_static_start,"GOMP_1.0",__mpcomp_GOMP_loop_static_start,__mpcomp_GOMP_loop_static_start)
GOMP_ABI_FUNC(GOMP_ordered_end,"GOMP_1.0",__mpcomp_GOMP_ordered_end,__mpcomp_ordered_end)
GOMP_ABI_FUNC(GOMP_ordered_start,"GOMP_1.0",__mpcomp_GOMP_ordered_start,__mpcomp_ordered_begin)
GOMP_ABI_FUNC(GOMP_parallel_end,"GOMP_1.0",__mpcomp_GOMP_parallel_end,__mpcomp_GOMP_parallel_end)
GOMP_ABI_FUNC(GOMP_parallel_loop_dynamic_start,"GOMP_1.0",__mpcomp_GOMP_parallel_loop_dynamic_start,__mpcomp_GOMP_parallel_loop_dynamic_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_guided_start,"GOMP_1.0",__mpcomp_GOMP_parallel_loop_guided_start,__mpcomp_GOMP_parallel_loop_guided_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_runtime_start,"GOMP_1.0",__mpcomp_GOMP_parallel_loop_runtime_start,__mpcomp_GOMP_parallel_loop_runtime_start)
GOMP_ABI_FUNC(GOMP_parallel_loop_static_start,"GOMP_1.0",__mpcomp_GOMP_parallel_loop_static_start,__mpcomp_GOMP_parallel_loop_static_start)
GOMP_ABI_FUNC(GOMP_parallel_sections_start,"GOMP_1.0",__mpcomp_GOMP_parallel_sections_start,__mpcomp_GOMP_parallel_sections_start)
GOMP_ABI_FUNC(GOMP_parallel_start,"GOMP_1.0",__mpcomp_GOMP_parallel_start,__mpcomp_GOMP_parallel_start)
GOMP_ABI_FUNC(GOMP_sections_end,"GOMP_1.0",__mpcomp_GOMP_sections_end,__mpcomp_GOMP_sections_end)
GOMP_ABI_FUNC(GOMP_sections_end_nowait,"GOMP_1.0",GOMP_sections_end_nowait,GOMP_sections_end_nowait)
GOMP_ABI_FUNC(GOMP_sections_end_nowait,"GOMP_1.0",__mpcomp_GOMP_sections_end_nowait,__mpcomp_GOMP_sections_end_nowait)
GOMP_ABI_FUNC(GOMP_sections_next,"GOMP_1.0",__mpcomp_GOMP_sections_next,__mpcomp_sections_next)
GOMP_ABI_FUNC(GOMP_sections_start,"GOMP_1.0",__mpcomp_GOMP_sections_start,__mpcomp_sections_begin)
GOMP_ABI_FUNC(GOMP_single_copy_end,"GOMP_1.0",__mpcomp_GOMP_single_copy_end,__mpcomp_do_single_copyprivate_end)
GOMP_ABI_FUNC(GOMP_single_copy_start,"GOMP_1.0",__mpcomp_GOMP_single_copy_start,__mpcomp_do_single_copyprivate_begin)
GOMP_ABI_FUNC(GOMP_single_start,"GOMP_1.0",__mpcomp_GOMP_single_start,__mpcomp_do_single)

//All"GOMP_2.0"symbols
GOMP_ABI_FUNC(GOMP_task,"GOMP_2.0",__mpcomp_GOMP_task,__mpcomp_GOMP_task)
GOMP_ABI_FUNC(GOMP_loop_ull_dynamic_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_dynamic_next,__mpcomp_loop_ull_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_ull_dynamic_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_dynamic_start,__mpcomp_GOMP_loop_ull_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_guided_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_guided_next,__mpcomp_loop_ull_guided_next)
GOMP_ABI_FUNC(GOMP_loop_ull_guided_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_guided_start,__mpcomp_GOMP_loop_ull_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_dynamic_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_dynamic_next,__mpcomp_loop_ull_dynamic_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_dynamic_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_dynamic_start,__mpcomp_GOMP_loop_ull_ordered_dynamic_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_guided_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_guided_next,__mpcomp_loop_ull_ordered_guided_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_guided_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_guided_start,__mpcomp_GOMP_loop_ull_ordered_guided_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_runtime_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_runtime_next,__mpcomp_loop_ull_ordered_runtime_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_runtime_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_runtime_start,__mpcomp_GOMP_loop_ull_ordered_runtime_start)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_static_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_static_next,__mpcomp_loop_ull_ordered_static_next)
GOMP_ABI_FUNC(GOMP_loop_ull_ordered_static_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_ordered_static_start,__mpcomp_GOMP_loop_ull_ordered_static_start)
GOMP_ABI_FUNC(GOMP_loop_ull_runtime_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_runtime_next,__mpcomp_loop_ull_runtime_next)
GOMP_ABI_FUNC(GOMP_loop_ull_runtime_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_runtime_start,__mpcomp_GOMP_loop_ull_runtime_start)
GOMP_ABI_FUNC(GOMP_loop_ull_static_next,"GOMP_2.0",__mpcomp_GOMP_loop_ull_static_next,__mpcomp_loop_ull_static_next)
GOMP_ABI_FUNC(GOMP_loop_ull_static_start,"GOMP_2.0",__mpcomp_GOMP_loop_ull_static_start,__mpcomp_GOMP_loop_ull_static_start)
GOMP_ABI_FUNC(GOMP_taskwait,"GOMP_2.0",__mpcomp_GOMP_taskwait,__mpcomp_taskwait)

//All"GOMP_3.0"symbols
GOMP_ABI_FUNC(GOMP_taskyield,"GOMP_3.0",__mpcomp_GOMP_taskyield,__mpcomp_taskyield)

//AllGOMP_4.0symbols
GOMP_ABI_FUNC(GOMP_parallel,"GOMP_4.0",__mpcomp_GOMP_parallel,__mpcomp_GOMP_parallel)
/*
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_BARRIER_CANCEL,GOMP_barrier_cancel,400,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_CANCEL,GOMP_cancel,401,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_CANCELLATION_POINT,GOMP_cancellation_point,402,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_LOOP_END_CANCEL,GOMP_loop_end_cancel,403,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_PARALLEL_LOOP_DYNAMIC,GOMP_parallel_loop_dynamic,404,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_PARALLEL_LOOP_GUIDED,GOMP_parallel_loop_guided,405,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_PARALLEL_LOOP_RUNTIME,GOMP_parallel_loop_runtime,406,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_PARALLEL_LOOP_STATIC,GOMP_parallel_loop_static,407,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_PARALLEL_SECTIONS,GOMP_parallel_sections,408,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TASKGROUP_START,GOMP_taskgroup_start,411,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TASKGROUP_END,GOMP_taskgroup_end,412,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_SECTIONS_END_CANCEL,GOMP_sections_end_cancel,410,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET,GOMP_target,413,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_DATA,GOMP_target_data,414,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_END_DATA,GOMP_target_end_data,415,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TARGET_UPDATE,GOMP_target_update,416,GOMP_4.0)
GOMP_ABI_FUNC(MPCOMP_GOMP_ABI_TEAMS,GOMP_teams,417,GOMP_4.0)
*/
