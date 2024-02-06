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
/* #   -  DIONISI Thomas thomas.dionisi@uvsq.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_LOWCOMM_WORKSHARE_H_
#define MPC_LOWCOMM_WORKSHARE_H_

#include <mpc_config.h>

#include <mpc_lowcomm_types.h>
#include <mpc_lowcomm.h>
#include "mpc_common_spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MPC_ENABLE_WORKSHARE


typedef struct mpc_workshare{
  OPA_int_t is_last_iter;
  char* pad[64];
  OPA_ptr_t cur_index;
  char* pad2[64];
  OPA_ptr_t reverse_index;
  char* pad3[64];
  int threshold_number;
  char* pad4[64];
  OPA_int_t threshold_index;
  char* pad5[64];
  OPA_int_t rev_threshold_index;
  char* pad6[64];
  mpc_common_spinlock_t lock;
  mpc_common_spinlock_t lock2;
  char* pad7[64];
  OPA_int_t nb_threads_stealing;
  char* pad8[64];
  mpc_common_spinlock_t atomic_lock;
  mpc_common_spinlock_t critical_lock;
  char* pad9[64];
  long lb;
  long ub;
  long incr;
  int scheduling_type;
  long chunk_size;
  int steal_scheduling_type;
  long steal_chunk_size;
  char* pad10[64];
  OPA_ptr_t current_chunk_size;
  OPA_ptr_t current_chunk_size_anc;
  OPA_ptr_t steal_current_chunk_size;
  void* shareds;
  float inv_threshold_size;
  int nowait;
  int is_allowed_to_steal;
  void (*func) (void*, long long, long long); 
} mpc_workshare;

void mpc_lowcomm_workshare_start(void(*) (void*, long long, long long) , void* ,long long, long long, long long , int, int, int, int);
void mpc_lowcomm_workshare_stop_stealing();
void mpc_lowcomm_workshare_resteal();
void mpc_lowcomm_workshare_atomic_start();
void mpc_lowcomm_workshare_atomic_end();
void mpc_lowcomm_workshare_critical_start();
void mpc_lowcomm_workshare_critical_end();
void mpc_lowcomm_workshare_steal();
void mpc_lowcomm_workshare_steal_wait_for_value(volatile int*, int, void(*) (void*), void*);
void mpc_lowcomm_workshare_init_func_pointers();

#endif

#ifdef MPC_ENABLE_WORKSHARE
#define MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL() \
do { \
      if(mpc_conf_root_config_get("mpcframework.lowcomm.workshare.enablestealing")) \
        mpc_lowcomm_workshare_steal(); \
   }  while(0)
#else
#define MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL()    (void)(0)
#endif

#ifdef MPC_ENABLE_WORKSHARE
#define MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL_WAIT_FOR_VALUE(data,value,func,arg,rank) \
do { \
      if(mpc_conf_root_config_get("mpcframework.lowcomm.workshare.enablestealing")) \
        mpc_lowcomm_workshare_steal_wait_for_value(data,value,func,arg); \
   }  while(0)
#else
#define MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL_WAIT_FOR_VALUE(data,value,func,arg,rank)    (void)(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_WORKSHARE_H_ */
