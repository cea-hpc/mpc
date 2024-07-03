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

#include <mpc_topology.h>
#include <mpc_common_flags.h>

#include "mpc_common_debug.h"

#include <mpc_thread.h>


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct mpc_thread_rank_info_s
{
	int rank;
	int local_rank;
	int app_rank;
}mpc_thread_rank_info_t;

static inline int ___get_task_rank()
{
	mpc_thread_rank_info_t *data = mpc_thread_rank_info_get();

	if(!data)
	{
		return -1;
	}

	return ( int )(data->rank);
}


/* From manually switched TLSs */
extern __thread int __mpc_task_rank;
extern int __process_rank;

/* NOLINTBEGIN(clang-diagnostic-unused-function) */

static inline int mpc_common_get_task_rank(void)
{
#if defined(MPC_IN_PROCESS_MODE)
	return __process_rank;
#endif
	int can_be_disguised = 0;
#ifdef MPC_MPI
	can_be_disguised = mpc_common_flags_disguised_get();
#endif
	int ret = -1;

	if(can_be_disguised == 0)
	{
		/* __mpc_task_rank is a manually switched
		 * TLS entry. It is initialized at -2,
		 * meaning that if we have this value
		 * the request to the actual rank has
		 * not been made. However, if different
		 * it contains the task rank */
		ret = __mpc_task_rank;

		/* Was it initialized ? Yes then we are done */
		if(ret != -2)
		{
			return ret;
		}

		ret = ___get_task_rank();

		/* Save for next call */
		__mpc_task_rank = ret;
	}
	else
	{
		ret = ___get_task_rank();
	}

	return ret;
}

static inline int mpc_common_get_app_rank(){
	return mpc_thread_rank_info_get()->app_rank;
}

static inline void mpc_common_set_app_rank(int app_rank){
	mpc_thread_rank_info_get()->app_rank = app_rank;
}

static inline int mpc_common_get_app_size(){
	return mpc_common_get_flags()->appsize;
}

static inline int mpc_common_get_app_num(){
	return mpc_common_get_flags()->appnum;
}

static inline int mpc_common_get_app_count(){
	return mpc_common_get_flags()->appcount;
}

static inline unsigned int mpc_common_get_task_count(void)
{
	return mpc_common_get_flags()->task_number;
}

static inline int mpc_common_get_local_task_rank(void)
{
#ifdef MPC_IN_PROCESS_MODE
	return 0;
#endif
	mpc_thread_rank_info_t *data = mpc_thread_rank_info_get();

	if(!data)
	{
		return -1;
	}

	return ( int )(data->local_rank);
}

static inline int mpc_common_get_local_task_count(void)
{
#ifdef MPC_IN_PROCESS_MODE
	return 1;
#endif
	return mpc_thread_get_current_local_tasks_nb();
}

static inline int mpc_common_get_thread_id(void)
{
	return mpc_thread_get_thread_id();
}

/* NOLINTEND(clang-diagnostic-unused-function) */

#ifdef __cplusplus
}
#endif
#endif //MPC_THREADS_SCTK_THREAD_SCTK_ACCESSOR_H_
