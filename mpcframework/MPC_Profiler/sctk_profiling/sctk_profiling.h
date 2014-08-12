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
#ifndef __SCTK_PROFILING_H__
#define __SCTK_PROFILING_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "sctk_config.h"
#include <mpcprofil.h>

  typedef mpc_profiling_key_t sctk_profiling_key_t;

  typedef mpc_profiling_t sctk_profiling_t;

  void sctk_trace_point (char **function, void *arg1, void *arg2,
			 void *arg3, void *arg4);

#define SCTK_MAX_PROFILING_FUNC_NUMBER 1024

  void sctk_profiling_key_create (sctk_profiling_key_t * key, char *name);
  void sctk_profiling_set_usage (sctk_profiling_key_t key, double usage);
  void sctk_profiling_result (void);
  void sctk_profiling_commit (void);
  void sctk_profiling_init (void);
  double sctk_profiling_get_timestamp (void);
  void sctk_profiling_init_keys (void);


#define SCTK_TRACE_START(func,a,b,c,d) do{static char* START_MPC_TRACE__##func = SCTK_STRING(START_MPC_TRACE__##func); sctk_trace_point(&START_MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)
#define SCTK_TRACE_END(func,a,b,c,d)   do{static char* END___MPC_TRACE__##func = SCTK_STRING(END___MPC_TRACE__##func); sctk_trace_point(&END___MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)
#define SCTK_TRACE_POINT(func,a,b,c,d) do{static char* POINT_MPC_TRACE__##func = SCTK_STRING(POINT_MPC_TRACE__##func); sctk_trace_point(&POINT_MPC_TRACE__##func,(void*)((long)(a)),(void*)((long)(b)),(void*)((long)(c)),(void*)((long)(d)));}while(0)

#define SCTK_PROFIL_START(name)						\
  double __sctk_profiling__start__##name;				\
    double __sctk_profiling__end__##name;				\
    __sctk_profiling__start__##name = sctk_profiling_get_timestamp();	\
    SCTK_TRACE_START(name,NULL,NULL,NULL,NULL)

#define SCTK_PROFIL_TRACE_START(name,a,b,c,d)				\
  double __sctk_profiling__start__##name;				\
    double __sctk_profiling__end__##name;				\
    __sctk_profiling__start__##name = sctk_profiling_get_timestamp();	\
    SCTK_TRACE_START(name,(void*)a,(void*)b,(void*)c,(void*)d)

#define SCTK_PROFIL_INIT(name)					\
  double __sctk_profiling__start__##name;			\
    double __sctk_profiling__end__##name

#define SCTK_PROFIL_TIME(name)						\
  __sctk_profiling__start__##name = sctk_profiling_get_timestamp();	\
    SCTK_TRACE_START(name,NULL,NULL,NULL,NULL)

#define SCTK_PROFIL_END(name)						\
  __sctk_profiling__end__##name = sctk_profiling_get_timestamp();	\
    sctk_profiling_set_usage(sctk_profiling_key##name,__sctk_profiling__end__##name-__sctk_profiling__start__##name);\
    SCTK_TRACE_END(name,NULL,NULL,NULL,NULL)

#define SCTK_PROFIL_TRACE_END(name,a,b,c,d)				\
  __sctk_profiling__end__##name = sctk_profiling_get_timestamp();	\
    sctk_profiling_set_usage(sctk_profiling_key##name,__sctk_profiling__end__##name-__sctk_profiling__start__##name);\
    SCTK_TRACE_END(name,a,b,c,d)


#ifdef ___SCTK___PROFILING___C___
#define SCTK_PROFIL_KEY(name) sctk_profiling_key_t sctk_profiling_key##name = 0
#else
#define SCTK_PROFIL_KEY(name) extern sctk_profiling_key_t sctk_profiling_key##name
#endif

#define SCTK_PROF_INIT_KEY(name) sctk_profiling_key_create(&(sctk_profiling_key##name), SCTK_STRING(name))


  /************************************************************************/
  /*PROFILING MPC                                                         */
  /************************************************************************/
    SCTK_PROFIL_KEY (MPC_Struct_datatype);
    SCTK_PROFIL_KEY (MPC_Abort);
    SCTK_PROFIL_KEY (MPC_Add_pack);
    SCTK_PROFIL_KEY (MPC_Add_pack_absolute);
    SCTK_PROFIL_KEY (MPC_Add_pack_default);
    SCTK_PROFIL_KEY (MPC_Allgather);
    SCTK_PROFIL_KEY (MPC_Allgatherv);
    SCTK_PROFIL_KEY (MPC_Allreduce);
    SCTK_PROFIL_KEY (MPC_Alltoall);
    SCTK_PROFIL_KEY (MPC_Alltoallv);
    SCTK_PROFIL_KEY (MPC_Barrier);
    SCTK_PROFIL_KEY (MPC_Bcast);
    SCTK_PROFIL_KEY (MPC_Bsend);
    SCTK_PROFIL_KEY (MPC_Comm_create);
    SCTK_PROFIL_KEY (MPC_Comm_create_list);
    SCTK_PROFIL_KEY (MPC_Comm_dup);
    SCTK_PROFIL_KEY (MPC_Comm_free);
    SCTK_PROFIL_KEY (MPC_Comm_group);
    SCTK_PROFIL_KEY (MPC_Comm_rank);
    SCTK_PROFIL_KEY (MPC_Comm_size);
    SCTK_PROFIL_KEY (MPC_Comm_split);
    SCTK_PROFIL_KEY (MPC_Default_pack);
    SCTK_PROFIL_KEY (MPC_Errhandler_create);
    SCTK_PROFIL_KEY (MPC_Errhandler_free);
    SCTK_PROFIL_KEY (MPC_Errhandler_get);
    SCTK_PROFIL_KEY (MPC_Errhandler_set);
    SCTK_PROFIL_KEY (MPC_Error_class);
    SCTK_PROFIL_KEY (MPC_Error_string);
    SCTK_PROFIL_KEY (MPC_Finalize);
    SCTK_PROFIL_KEY (MPC_Gather);
    SCTK_PROFIL_KEY (MPC_Gatherv);
    SCTK_PROFIL_KEY (MPC_Get_count);
    SCTK_PROFIL_KEY (MPC_Get_processor_name);
    SCTK_PROFIL_KEY (MPC_Group_difference);
    SCTK_PROFIL_KEY (MPC_Group_free);
    SCTK_PROFIL_KEY (MPC_Group_incl);
    SCTK_PROFIL_KEY (MPC_Ibsend);
    SCTK_PROFIL_KEY (MPC_Init);
    SCTK_PROFIL_KEY (MPC_Initialized);
    SCTK_PROFIL_KEY (MPC_Iprobe);
    SCTK_PROFIL_KEY (MPC_Irecv);
    SCTK_PROFIL_KEY (MPC_Irecv_pack);
    SCTK_PROFIL_KEY (MPC_Irsend);
    SCTK_PROFIL_KEY (MPC_Isend);
    SCTK_PROFIL_KEY (MPC_Isend_pack);
    SCTK_PROFIL_KEY (MPC_Issend);
    SCTK_PROFIL_KEY (MPC_Node_number);
    SCTK_PROFIL_KEY (MPC_Node_rank);
    SCTK_PROFIL_KEY (MPC_Op_create);
    SCTK_PROFIL_KEY (MPC_Op_free);
    SCTK_PROFIL_KEY (MPC_Open_pack);
    SCTK_PROFIL_KEY (MPC_Probe);
    SCTK_PROFIL_KEY (MPC_Proceed);
    SCTK_PROFIL_KEY (MPC_Process_number);
    SCTK_PROFIL_KEY (MPC_Process_rank);
    SCTK_PROFIL_KEY (MPC_Processor_number);
    SCTK_PROFIL_KEY (MPC_Processor_rank);
    SCTK_PROFIL_KEY (MPC_Recv);
    SCTK_PROFIL_KEY (MPC_Reduce);
    SCTK_PROFIL_KEY (MPC_Rsend);
    SCTK_PROFIL_KEY (MPC_Scatter);
    SCTK_PROFIL_KEY (MPC_Scatterv);
    SCTK_PROFIL_KEY (MPC_Send);
    SCTK_PROFIL_KEY (MPC_Sendrecv);
    SCTK_PROFIL_KEY (MPC_Type_hcontiguous);
    SCTK_PROFIL_KEY (MPC_Ssend);
    SCTK_PROFIL_KEY (MPC_Test);
    SCTK_PROFIL_KEY (MPC_Test_no_check);
    SCTK_PROFIL_KEY (MPC_Test_check);
    SCTK_PROFIL_KEY (MPC_Type_free);
    SCTK_PROFIL_KEY (MPC_Type_size);
    SCTK_PROFIL_KEY (MPC_Wait);
    SCTK_PROFIL_KEY (MPC_Wait_pending);
    SCTK_PROFIL_KEY (MPC_Wait_pending_all_comm);
    SCTK_PROFIL_KEY (MPC_Waitall);
    SCTK_PROFIL_KEY (MPC_Waitany);
    SCTK_PROFIL_KEY (MPC_Waitsome);
    SCTK_PROFIL_KEY (MPC_Wtime);
    SCTK_PROFIL_KEY (malloc);
    SCTK_PROFIL_KEY (realloc);
    SCTK_PROFIL_KEY (calloc);
    SCTK_PROFIL_KEY (free);
    SCTK_PROFIL_KEY (sctk_malloc);
    SCTK_PROFIL_KEY (sctk_realloc);
    SCTK_PROFIL_KEY (sctk_calloc);
    SCTK_PROFIL_KEY (sctk_free);
    SCTK_PROFIL_KEY (sctk_sbrk);
    SCTK_PROFIL_KEY (sctk_sbrk_try_to_find_page);
    SCTK_PROFIL_KEY (sctk_sbrk_mmap);
    SCTK_PROFIL_KEY (sctk_thread_mutex_lock);
    SCTK_PROFIL_KEY (sctk_thread_mutex_unlock);
    SCTK_PROFIL_KEY (sctk_thread_mutex_spinlock);
    SCTK_PROFIL_KEY (sctk_free_distant);
    SCTK_PROFIL_KEY (sctk_thread_wait_for_value_and_poll);
    SCTK_PROFIL_KEY (sctk_thread_wait_for_value);
    SCTK_PROFIL_KEY (sctk_mmap);
    SCTK_PROFIL_KEY (sctk_alloc_ptr_small_serach_chunk);
    SCTK_PROFIL_KEY (sctk_alloc_ptr_big);
    SCTK_PROFIL_KEY (sctk_alloc_ptr_small);
    SCTK_PROFIL_KEY (sctk_free_small);
    SCTK_PROFIL_KEY (sctk_free_big);
    SCTK_PROFIL_KEY (sctk_elan_barrier);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver);
    SCTK_PROFIL_KEY (sctk_net_get_new_header);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver_inf);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver_sup);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver_sup_write);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver_split_header);
    SCTK_PROFIL_KEY (sctk_net_send_ptp_message_driver_send_header);
  /* MPC_MicroThreads */
    SCTK_PROFIL_KEY (__sctk_microthread_parallel_exec__last_barrier) ;
  /* MPC_OpenMP */
    SCTK_PROFIL_KEY (__mpcomp_start_parallel_region);
    SCTK_PROFIL_KEY (__mpcomp_start_parallel_region__creation);
    SCTK_PROFIL_KEY (__mpcomp_dynamic_steal);
    SCTK_PROFIL_KEY (__mpcomp_dynamic_loop_begin);
    SCTK_PROFIL_KEY (__mpcomp_dynamic_loop_next);


#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
