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
/* #   - MAHEO Aurèle aurele.maheo@exascale-computing.eu                  # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_OMPT_INTERNALS_H
#define MPC_OMPT_INTERNALS_H

#include "ompt.h"
#include "mpcomp_types.h"
#include "sctk_atomics.h"
#include "mpcomp_types.h"
#include "mpcomp_ompt_specific.h"
#include "mpcomp_ompt_types_internal.h"

static sctk_atomics_int mpcomp_OMPT_wait_id = OPA_INT_T_INITIALIZER(1); 
static sctk_atomics_int mpcomp_OMPT_task_id = OPA_INT_T_INITIALIZER(1); 
static sctk_atomics_int mpcomp_OMPT_thread_id = OPA_INT_T_INITIALIZER(1); 

ompt_wait_id_t mpcomp_OMPT_gen_wait_id( void )
{
	const int prev = sctk_atomics_fetch_and_incr_int( &( mpcomp_OMPT_wait_id ) );
	return ( ompt_wait_id_t ) prev; 
}

ompt_task_id_t mpcomp_OMPT_gen_task_id( void )
{
	const int prev = sctk_atomics_fetch_and_incr_int( &( mpcomp_OMPT_task_id ) );
	return ( ompt_task_id_t ) prev; 
}

ompt_thread_id_t mpcomp_OMPT_gen_thread_id( void )
{
	const int prev = sctk_atomics_fetch_and_incr_int( &( mpcomp_OMPT_thread_id ) );
	return ( ompt_thread_id_t ) prev; 
}

ompt_parallel_id_t mpcomp_OMPT_gen_parallel_id( void )
{
	const int prev = sctk_atomics_fetch_and_incr_int( &( mpcomp_OMPT_thread_id ) );
	return ( ompt_thread_id_t ) prev; 
}

ompt_state_t ompt_get_state_internal(__UNUSED__ ompt_wait_id_t *wait_id)
{
	mpcomp_thread_t *t;

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert( t != NULL);

	if(t)
		return t->state;
	else
		return ompt_state_undefined;
}

ompt_frame_t * ompt_get_task_frame(int ancestor_level)
{
 int i;
 mpcomp_thread_t *t;
 mpcomp_mvp_t *mvp;
 //mpc_ompt_team_t *team;

 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);

 mvp = t->mvp;
 
 sctk_assert( mvp != NULL);

 //team = mvp->mpc_ompt_thread.team;
 
 for( i = 0; i < ancestor_level; i++) {
  //team = team->parent;
 }

 return NULL;
}

#if 0
#if OMPT_SUPPORT
mpc_ompt_task_t ompt_get_task(mpc_ompt_taskteam_t *ompt_team)
{
 return ompt_team->mpc_ompt_task;
}

ompt_frame_t ompt_get_task_frame_internal(int depth)
{
 mpc_ompt_taskteam_t *ompt_team;
 mpc_ompt_task_t ompt_task;

 //ompt_team = ompt_get_team(depth);

 if(ompt_team == NULL) {
  return;
 }

 //ompt_task = ompt_get_task(&ompt_team);

 //return ompt_task.frame;
 return;
}

ompt_task_id_t ompt_get_task_id_internal(int depth)
{
 mpc_ompt_taskteam_t *ompt_team;
 mpc_ompt_task_t ompt_task;

 //ompt_team = ompt_get_team(depth);
 //ompt_task = ompt_get_task(ompt_team);

 return ompt_task.task_id; 
}

ompt_task_id_t ompt_get_parent_task_id(mpcomp_thread_t *t)
{
 mpc_ompt_taskteam_t *team;
 team = t->mpc_ompt_thread.taskteam;

 if(team == NULL) {
  //fprintf(stderr,"ompt_get_parent_task_id: team NULL (@ %p), t @ %p\n", team, t);
  return;
 }
 return t->mpc_ompt_thread.taskteam->parent->mpc_ompt_task.task_id;
}

ompt_frame_t ompt_get_parent_task_frame(mpcomp_thread_t *t)
{
 return;
}

void ompt_taskteam_init(mpc_ompt_taskteam_t *taskteam, ompt_parallel_id_t parallel_id, ompt_task_id_t task_id)
{
 taskteam->parallel_id =  parallel_id;
 taskteam->mpc_ompt_task.task_id = task_id;
 taskteam->mpc_ompt_task.frame.reenter_runtime_frame = 0;
 taskteam->mpc_ompt_task.frame.exit_runtime_frame = 0;
 taskteam->parent = 0;
}

void ompt_push_taskteam(mpc_ompt_taskteam_t *taskteam, mpcomp_thread_t *t)
{
 mpcomp_thread_t *t;
 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);

 if(t->mpc_ompt_thread.taskteam == NULL) {
  return;
 }

 mpc_ompt_taskteam_t *team_parent = t->mpc_ompt_thread.taskteam;
 taskteam->parent = team_parent;
 t->mpc_ompt_thread.taskteam = taskteam;

 if(t->mpc_ompt_thread.taskteam == NULL) {
  return;
 }

 //fprintf(stderr, "[TEST] ompt_push_taskteam: task_id = %d\n", t->mpc_ompt_thread.taskteam->parent->mpc_ompt_task.task_id);
}

void ompt_pull_taskteam(mpc_ompt_taskteam_t *taskteam, mpcomp_thread_t *t)
{
 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);

 t->mpc_ompt_thread.taskteam = taskteam->parent;
}

mpc_ompt_team_t * ompt_get_team(int ancestor_level)
{
 int i;
 mpcomp_thread_t *t;
 mpcomp_mvp_t *mvp;
 mpc_ompt_team_t *team;

 fprintf(stderr, "ompt_get_team: ancestor_level = %d\n", ancestor_level);

 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
 sctk_assert( t != NULL);
 //mvp = t->mvp;

 sctk_assert( mvp != NULL);

 //team = mvp->mpc_ompt_thread.team;
 
 for( i = 0; i < ancestor_level; i++) {
  //fprintf(stderr, "ompt_get_team: getting team parent\n");
  team = team->parent;
 }

 return team;
}

void ompt_parallel_region_init()
{
 mpcomp_thread_t *t;

 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);

 mpc_ompt_parallel_t ompt_parallel = t->mpc_ompt_thread.ompt_parallel_info;

 if(ompt_parallel != NULL) {
   return;
 } else {
   t->mpc_ompt_thread.ompt_parallel_info.parallel_id = OMPT_gen_parallel_id();   
 }
}
#endif
#endif

#endif /* MPC_OMPT_INTERNALS_H */
