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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_API_H__
#define __MPC_OMP_API_H__

#include "mpcomp_types.h"

void omp_set_num_threads(int);
int omp_get_thread_num(void);
int omp_get_max_threads(void);
int omp_get_num_procs(void);
void omp_set_dynamic(int);
int omp_get_dynamic(void);
void omp_set_nested(int);
int omp_get_nested(void);
void omp_set_schedule(omp_sched_t, int);
void omp_get_schedule(omp_sched_t *, int *);
int omp_in_parallel(void);
int omp_get_level(void);
int omp_get_active_level(void);
int omp_get_ancestor_thread_num(int);
int omp_get_team_size(int);
int omp_get_num_threads(void);
double omp_get_wtime(void);
double omp_get_wtick(void);
int omp_get_thread_limit(void);
void omp_set_max_active_levels(int);
int omp_get_max_active_levels();
int omp_in_final(void);
int omp_control_tool(int, int, void*);
int mpc_omp_get_num_threads(void);
int mpc_omp_get_thread_num(void);

#endif /* __MPC_OMP_API_H__ */
