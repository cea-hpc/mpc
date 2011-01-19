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
#ifndef __MPC_PROFILING_H__
#define __MPC_PROFILING_H__
#include "sctk_config.h"
#ifdef __cplusplus
extern "C"
{
#endif


  typedef int mpc_profiling_key_t;
  typedef struct
  {
    char *func_name;
    unsigned long nb_call;
    double elapsed_time;
  } mpc_profiling_t;


  void mpc_profiling_key_create (mpc_profiling_key_t * key, char *name);

  void mpc_profiling_set_usage (mpc_profiling_key_t key, double usage);

  void mpc_profiling_result (void);

  void mpc_profiling_init (void);
  void mpc_profiling_commit (void);

  double mpc_profiling_get_timestamp (void);

  void mpc_trace_point (char **function, void *arg1, void *arg2,
			void *arg3, void *arg4);

#define MPC_PROFIL_START(name)						\
  double __mpc_profiling__start__##name;				\
    double __mpc_profiling__end__##name;				\
    __mpc_profiling__start__##name = mpc_profiling_get_timestamp()


#define MPC_PROFIL_INIT(name)						\
  double __mpc_profiling__start__##name;				\
    double __mpc_profiling__end__##name

#define MPC_PROFIL_TIME(name)						\
    __mpc_profiling__start__##name = mpc_profiling_get_timestamp()


#define MPC_PROFIL_END(name)						\
  __mpc_profiling__end__##name = mpc_profiling_get_timestamp();	\
    mpc_profiling_set_usage(mpc_profiling_key##name,__mpc_profiling__end__##name-__mpc_profiling__start__##name)

#define MPC_PROFIL_KEY(name) mpc_profiling_key_t mpc_profiling_key##name
#define MPC_PROF_INIT_KEY(name) mpc_profiling_key_create(&(mpc_profiling_key##name), SCTK_STRING(name))

#define MPC_TRACE_START(func,a,b,c,d) do{static char* START_MPC_TRACE__##func = SCTK_STRING(START_MPC_TRACE__##func); mpc_trace_point(&START_MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)
#define MPC_TRACE_END(func,a,b,c,d)   do{static char* END___MPC_TRACE__##func = SCTK_STRING(END___MPC_TRACE__##func); mpc_trace_point(&END___MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)
#define MPC_TRACE_POINT(func,a,b,c,d) do{static char* POINT_MPC_TRACE__##func = SCTK_STRING(POINT_MPC_TRACE__##func); mpc_trace_point(&POINT_MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
