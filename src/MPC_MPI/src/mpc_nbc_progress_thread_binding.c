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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_nbc_progress_thread_binding.h"
// Documentation is readable in .h file

int sctk_get_progress_thread_binding_bind(void) {
  int cpu_id_to_bind_progress_thread;
  cpu_id_to_bind_progress_thread = mpc_topology_get_current_cpu();
  return cpu_id_to_bind_progress_thread;
}

int sctk_get_progress_thread_binding_smart(void) {
  int cpu_id_to_bind_progress_thread;
  cpu_id_to_bind_progress_thread = mpc_topology_get_current_cpu() + 1;
  return cpu_id_to_bind_progress_thread;
}

int sctk_get_progress_thread_binding_overweight(void) {
  int cpu_id_to_bind_progress_thread;
  cpu_id_to_bind_progress_thread = 1; // with MPC default binding for MPI tasks
  return cpu_id_to_bind_progress_thread;
}

int sctk_get_progress_thread_binding_numa_iter(void) {
  int cpu_id_to_bind_progress_thread;

  int task_nb = mpc_common_get_local_task_count();
  int numa_node_per_node_nb = mpc_topology_get_numa_node_count();

  int current_cpu = mpc_topology_get_current_cpu();

  int nbVp;
  int global_id = mpc_common_get_local_task_rank();
  int proc_global = mpc_thread_get_task_placement_and_count(global_id, &nbVp);

  int numa_node_id = (global_id * numa_node_per_node_nb) / task_nb;
  int task_per_numa_node =
      (((numa_node_id + 1) * task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb) -
      (((numa_node_id)*task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb);

  int local_id =
      global_id - (((numa_node_id * task_nb) + numa_node_per_node_nb - 1) /
                   numa_node_per_node_nb);

  // DEBUG
  // char hostname[1024];
  // gethostname(hostname,1024);
  // FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
  // fprintf(hostname_fd,"NBC BINDING task_nb %d cpu_per_node %d
  // numa_node_per_node_nb %d numa_node_id %d task_per_numa_node %d local_id %d
  // global_id %d proc_global %d current_cpu %d mpc_common_get_local_task_count
  // %d\n\n",
  //        task_nb,
  //        cpu_per_node ,
  //        numa_node_per_node_nb,
  //        numa_node_id ,
  //        task_per_numa_node,
  //        local_id  ,
  //        global_id ,
  //        proc_global,
  //        current_cpu,
  //        mpc_common_get_local_task_count()
  //      );
  // fflush(hostname_fd);
  // fclose(hostname_fd);
  // ENDDEBUG

  assert(proc_global == current_cpu);

  // if we have a free slot to bind the progress thread
  if (nbVp > 1) {
    mpc_common_debug("if nbVp > 1 : return %d", proc_global + 1);
    return proc_global + 1;
  } else {

    // calculate the next free slot to bind progress thread
    int global_id_tmp;
    int local_id_tmp;
    int proc_global_tmp;
    int nbVp_tmp;

    for (global_id_tmp = global_id + 1, local_id_tmp = local_id + 1;
         local_id_tmp < task_per_numa_node;
         global_id_tmp += 1, local_id_tmp += 1) {
      proc_global_tmp = mpc_thread_get_task_placement_and_count(global_id_tmp, &nbVp_tmp);
      if (nbVp_tmp > 1) {
        mpc_common_debug("if nbVp_tmp > 1 : return %d", proc_global_tmp + 1);
        return proc_global_tmp + 1;
      }
      mpc_common_debug("global_id_tmp : %d", global_id_tmp);
    }
    // if we dont have a slot to bind the progress threads,
    // we bind the progress thread in self
    mpc_common_debug("we bind in self : %d %d", proc_global, task_per_numa_node);
    cpu_id_to_bind_progress_thread = proc_global;
    return cpu_id_to_bind_progress_thread;
  }
  printf("SCTK_PROGRESS_THREAD_BINDING_NUMA failed. Program will be abort\n");
  mpc_common_debug_abort();
  return -1;
}

int sctk_get_progress_thread_binding_numa(void) {
  int cpu_id_to_bind_progress_thread;

  int task_nb = mpc_common_get_local_task_count();

  int cpu_per_node = mpc_topology_get_pu_count();

  int numa_node_per_node_nb = mpc_topology_get_numa_node_count();

  int cpu_per_numa_node = cpu_per_node / numa_node_per_node_nb;

  int current_cpu = mpc_topology_get_current_cpu();

  int nbVp;

  int global_id = mpc_common_get_local_task_rank();

  int proc_global = mpc_thread_get_task_placement_and_count(global_id, &nbVp);

  int numa_node_id = (global_id * numa_node_per_node_nb) / task_nb;

  int task_per_numa_node =
      (((numa_node_id + 1) * task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb) -
      (((numa_node_id)*task_nb + numa_node_per_node_nb - 1) /
       numa_node_per_node_nb);

  assert(proc_global == current_cpu);

  // compas2016 articlempc commit 82e59e3b049a67bcfe2e9a1889fc3d7e5adb50bd
  if (task_per_numa_node >= cpu_per_numa_node) {
    cpu_id_to_bind_progress_thread = proc_global;
  } else {
    cpu_id_to_bind_progress_thread =
        (((((proc_global / (cpu_per_numa_node /
                            (cpu_per_numa_node - task_per_numa_node)) +
             1) *
            cpu_per_numa_node) +
           (cpu_per_numa_node - task_per_numa_node - 1)) /
          (cpu_per_numa_node - task_per_numa_node)) -
         1);
  }
  return cpu_id_to_bind_progress_thread;
}
