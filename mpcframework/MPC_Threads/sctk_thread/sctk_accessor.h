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
#ifndef MPC_THREADS_SCTK_THREAD_SCTK_ACCESSOR_H_
#define MPC_THREADS_SCTK_THREAD_SCTK_ACCESSOR_H_

#include <stdlib.h>

#include <mpc_common_topology.h>

#include "sctk_config.h"
#include "mpc_common_rank.h"
#include "sctk_debug.h"
#include "sctk_thread.h"

#include <mpc_launch_pmi.h>
#include "sctk_tls.h"

#ifdef MPC_MPI
#include <mpc_mpi_comm_lib.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif

static inline int mpc_common_get_task_rank (void)
{
#if defined(SCTK_PROCESS_MODE) || defined(SCTK_LIB_MODE)
  return mpc_common_get_process_rank();
#endif

  int can_be_disguised = 0;

#ifdef MPC_MPI
  can_be_disguised = __MPC_Maybe_disguised();
#endif

  int ret = -1;

  if (can_be_disguised == 0)
  {

      /* __mpc_task_rank is a manually switched
       * TLS entry. It is initialized at -2,
       * meaning that if we have this value
       * the request to the actual rank has
       * not been made. However, if different
       * it contains the task rank */

      ret = __mpc_task_rank;

      /* Was it initialized ? Yes then we are done */
      if (ret != -2)
        return ret;

      sctk_thread_data_t *data = sctk_thread_data_get();

      if (!data)
        return -1;

      ret = (int)(data->task_id);

      /* Save for next call */
      __mpc_task_rank = ret;
 }
 else
 {
    sctk_thread_data_t *data = sctk_thread_data_get();

    if (!data)
        return -1;

    ret = (int)(data->task_id);
 }

  return ret;
}

int sctk_get_total_tasks_number();

static inline int mpc_common_get_task_count(void)
{
	return sctk_get_total_tasks_number();
}

static inline int mpc_common_get_local_task_rank (void)
{
#ifdef SCTK_LIB_MODE
	return 0;
#endif

#ifdef SCTK_PROCESS_MODE
        return 0;
#endif
		sctk_thread_data_t *data = sctk_thread_data_get();

		if ( !data )
			return -1;

		return (int) ( data->local_task_id );
}

static inline int mpc_common_get_local_task_count (void)
{
#ifdef SCTK_LIB_MODE
	return 1;
#endif

#ifdef SCTK_PROCESS_MODE
        return 1;
#endif

        return sctk_thread_get_current_local_tasks_nb();
}

static inline int mpc_common_get_pu_rank (void)
{
	sctk_thread_data_t *data = sctk_thread_data_get();

	if ( !data )
		return -1;

	return data->virtual_processor;
}

static inline int mpc_common_get_pu_count (void)
{
  static int ret = -1;

  if (ret == -1) {
    ret = mpc_common_topo_get_pu_count();
  }

  return ret;
}

#ifdef __cplusplus
}
#endif
#endif //MPC_THREADS_SCTK_THREAD_SCTK_ACCESSOR_H_
