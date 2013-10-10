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
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_ACCESSORS_H_
#define __SCTK_ACCESSORS_H_

#include <stdlib.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_topology.h"
#include "sctk.h"
#ifdef __cplusplus
extern "C"
{
#endif

  static inline int sctk_get_task_rank (void)
  {
    return (int) (sctk_thread_data_get ()->task_id);
  }

  static inline int sctk_get_local_task_rank (void)
  {
    return (int) (sctk_thread_data_get ()->local_task_id);
  }

  static inline int sctk_get_local_task_number (void)
  {
    return sctk_thread_get_current_local_tasks_nb();
  }

  static inline int sctk_get_processor_rank (void)
  {
    return (int) (sctk_thread_data_get ()->virtual_processor);
  }

  static inline int sctk_get_processor_number (void)
  {
    return sctk_get_cpu_number ();
  }

  static inline int sctk_get_process_rank (void)
  {
    return sctk_process_rank;
  }

  static inline int sctk_get_process_number (void)
  {
    return sctk_process_number;
  }

  static inline int sctk_get_node_rank (void)
  {
    return sctk_node_rank;
  }

  static inline int sctk_get_node_number (void)
  {
    return sctk_node_number;
  }

  static inline int sctk_get_local_process_rank (void)
  {
    return sctk_local_process_rank;
  }

  static inline int sctk_get_local_process_number (void)
  {
    return sctk_local_process_number;
  }

#ifdef __cplusplus
}
#endif
#endif
