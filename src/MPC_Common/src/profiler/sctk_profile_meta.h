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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_PROFILE_META_H
#define SCTK_PROFILE_META_H

#include <sys/time.h>
#include <mpc_common_types.h>

#include "mpc_common_spinlock.h"
#include "mpc_keywords.h"

struct sctk_profile_meta
{
	int task_count;
	int process_count;
	int node_count;

	uint64_t walltime;
	double ticks_per_second;

	char command[1000];

	uint64_t begin_ts;
	struct timeval calibration_time;

	mpc_common_spinlock_t lock;
	int status;
};


void sctk_profile_meta_init(struct sctk_profile_meta *meta);
__UNUSED__ void sctk_profile_meta_release(struct sctk_profile_meta *meta);


void sctk_profile_meta_begin_compute(struct sctk_profile_meta *meta);
void sctk_profile_meta_end_compute(struct sctk_profile_meta *meta);

#endif /* SCTK_PROFILE_META_H */
