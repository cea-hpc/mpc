/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:09 CEST 2021                                        # */
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
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
/* #                                                                      # */
/* ######################################################################## */

/*
 * This file is meant to be included from "omp_gomp.h",
 * which temporarily defines the GOMP_ABI_FUNC macro.
 */
#ifdef GOMP_ABI_FUNC

/*------------------------------------------------------------------------*/
/*-------------------------- MPCOMP GOMP API NAMES------------------------*/

//All GOMP_1.0 symbols
GOMP_ABI_FUNC(mpc_omp_GOMP_barrier, GOMP_barrier, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_atomic_start, GOMP_atomic_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_atomic_end, GOMP_atomic_end, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_critical_start, GOMP_critical_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_critical_end, GOMP_critical_end, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_critical_name_start, GOMP_critical_name_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_critical_name_end, GOMP_critical_name_end, "GOMP_1.0")

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_dynamic_next, GOMP_loop_dynamic_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_dynamic_start, GOMP_loop_dynamic_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_end, GOMP_loop_end, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_end_nowait, GOMP_loop_end_nowait, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_guided_next, GOMP_loop_guided_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_guided_start, GOMP_loop_guided_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_dynamic_next, GOMP_loop_ordered_dynamic_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_dynamic_start, GOMP_loop_ordered_dynamic_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_guided_next, GOMP_loop_ordered_guided_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_guided_start, GOMP_loop_ordered_guided_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_runtime_next, GOMP_loop_ordered_runtime_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_runtime_start, GOMP_loop_ordered_runtime_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_static_next, GOMP_loop_ordered_static_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ordered_static_start, GOMP_loop_ordered_static_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_runtime_next, GOMP_loop_runtime_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_runtime_start, GOMP_loop_runtime_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_static_next, GOMP_loop_static_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_static_start, GOMP_loop_static_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_end, GOMP_parallel_end, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_dynamic_start, GOMP_parallel_loop_dynamic_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_guided_start, GOMP_parallel_loop_guided_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_runtime_start, GOMP_parallel_loop_runtime_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_static_start, GOMP_parallel_loop_static_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_sections_start, GOMP_parallel_sections_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_start, GOMP_parallel_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_sections_end, GOMP_sections_end, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_sections_end_nowait, GOMP_sections_end_nowait, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_sections_next, GOMP_sections_next, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_sections_start, GOMP_sections_start, "GOMP_1.0")

GOMP_ABI_FUNC(mpc_omp_GOMP_single_start, GOMP_single_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_single_copy_start, GOMP_single_copy_start, "GOMP_1.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_single_copy_end, GOMP_single_copy_end, "GOMP_1.0")

//All GOMP_2.0 symbols
GOMP_ABI_FUNC(mpc_omp_GOMP_task, GOMP_task, "GOMP_2.0")

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_dynamic_next, GOMP_loop_ull_dynamic_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_dynamic_start, GOMP_loop_ull_dynamic_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_guided_next, GOMP_loop_ull_guided_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_guided_start, GOMP_loop_ull_guided_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_dynamic_next, GOMP_loop_ull_ordered_dynamic_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_dynamic_start, GOMP_loop_ull_ordered_dynamic_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_guided_next, GOMP_loop_ull_ordered_guided_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_guided_start, GOMP_loop_ull_ordered_guided_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_runtime_next, GOMP_loop_ull_ordered_runtime_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_runtime_start, GOMP_loop_ull_ordered_runtime_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_static_next, GOMP_loop_ull_ordered_static_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_ordered_static_start, GOMP_loop_ull_ordered_static_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_runtime_next, GOMP_loop_ull_runtime_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_runtime_start, GOMP_loop_ull_runtime_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_static_next, GOMP_loop_ull_static_next, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_static_start, GOMP_loop_ull_static_start, "GOMP_2.0")
GOMP_ABI_FUNC(mpc_omp_GOMP_taskwait, GOMP_taskwait, "GOMP_2.0")

//All GOMP_3.0 symbols
GOMP_ABI_FUNC(mpc_omp_GOMP_taskyield, GOMP_taskyield, "GOMP_3.0")

// All GOMP_4.0 symbols
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel, GOMP_parallel, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_sections, GOMP_parallel_sections, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_static, GOMP_parallel_loop_static, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_dynamic, GOMP_parallel_loop_dynamic, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_guided, GOMP_parallel_loop_guided, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_runtime, GOMP_parallel_loop_runtime, "GOMP_4.0");

GOMP_ABI_FUNC(mpc_omp_GOMP_taskgroup_start, GOMP_taskgroup_start, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_taskgroup_end, GOMP_taskgroup_end, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_taskloop, GOMP_taskloop, "GOMP_4.0");

GOMP_ABI_FUNC(mpc_omp_GOMP_ordered_start, GOMP_ordered_start, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_ordered_end, GOMP_ordered_end, "GOMP_4.0");

GOMP_ABI_FUNC(mpc_omp_GOMP_barrier_cancel, GOMP_barrier_cancel, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_cancel, GOMP_cancel, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_cancellation_point, GOMP_cancellation_point, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_end_cancel, GOMP_loop_end_cancel, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_sections_end_cancel, GOMP_sections_end_cancel, "GOMP_4.0");

GOMP_ABI_FUNC(mpc_omp_GOMP_target, GOMP_target, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_data, GOMP_target, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_end_data, GOMP_target_end_data, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_update, GOMP_target_update, "GOMP_4.0");
GOMP_ABI_FUNC(mpc_omp_GOMP_teams, GOMP_teams, "GOMP_4.0");

/* GOMP 4.0.1 symbols */
GOMP_ABI_FUNC(mpc_omp_GOMP_offload_register, GOMP_offload_register, "GOMP_4.0.1");
GOMP_ABI_FUNC(mpc_omp_GOMP_offload_unregister, GOMP_offload_unregister, "GOMP_4.0.1");

/* GOMP 4.5 symbols */
GOMP_ABI_FUNC(mpc_omp_GOMP_target_ext, GOMP_target_ext, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_data_ext, GOMP_target_data_ext, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_update_ext, GOMP_target_update_ext, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_target_enter_exit_data, GOMP_target_enter_exit_data, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_taskloop_ull, GOMP_taskloop_ull, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_offload_register_ver, GOMP_offload_register_ver, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_offload_unregister_ver, GOMP_offload_unregister_ver, "GOMP_4.5");

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_doacross_static_start, GOMP_loop_doacross_static_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_doacross_dynamic_start, GOMP_loop_doacross_dynamic_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_doacross_guided_start, GOMP_loop_doacross_guided_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_doacross_runtime_start, GOMP_loop_doacross_runtime_start, "GOMP_4.5");

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_doacross_static_start, GOMP_loop_ull_doacross_static_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_doacross_dynamic_start, GOMP_loop_ull_doacross_dynamic_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_doacross_guided_start, GOMP_loop_ull_doacross_guided_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_doacross_runtime_start, GOMP_loop_ull_doacross_runtime_start, "GOMP_4.5");

GOMP_ABI_FUNC(mpc_omp_GOMP_doacross_post, GOMP_doacross_post, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_doacross_ull_post, GOMP_doacross_ull_post, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_doacross_wait, GOMP_doacross_wait, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_doacross_ull_wait, GOMP_doacross_ull_wait, "GOMP_4.5");

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_nonmonotonic_dynamic_start, GOMP_loop_nonmonotonic_dynamic_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_nonmonotonic_dynamic_next, GOMP_loop_nonmonotonic_dynamic_next, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_nonmonotonic_guided_start, GOMP_loop_nonmonotonic_guided_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_nonmonotonic_guided_next, GOMP_loop_nonmonotonic_guided_next, "GOMP_4.5");

GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_start, GOMP_loop_ull_nonmonotonic_dynamic_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_next, GOMP_loop_ull_nonmonotonic_dynamic_next, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_nonmonotonic_guided_start, GOMP_loop_ull_nonmonotonic_guided_start, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_loop_ull_nonmonotonic_guided_next, GOMP_loop_ull_nonmonotonic_guided_next, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_nonmonotonic_dynamic, GOMP_parallel_loop_nonmonotonic_dynamic, "GOMP_4.5");
GOMP_ABI_FUNC(mpc_omp_GOMP_parallel_loop_nonmonotonic_guided, GOMP_parallel_loop_nonmonotonic_guided, "GOMP_4.5");

// All GOMP_5.0 symbols
GOMP_ABI_FUNC(mpc_omp_GOMP_taskwait_depend, GOMP_taskwait_depend, "GOMP_5.0")

#endif /* GOMP_ABI_FUNC */
