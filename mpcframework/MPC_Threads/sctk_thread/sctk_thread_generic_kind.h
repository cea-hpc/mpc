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
#ifndef __SCTK_THREAD_GENERIC_KIND_H_
#define __SCTK_THREAD_GENERIC_KIND_H_


//KIND_MASK should be a power of 2 (1,2,4,8,16,...)
#define KIND_MASK_MPI (1<<0)
#define KIND_MASK_OMP (1<<1)
#define KIND_MASK_PTHREAD (1<<2)
#define KIND_MASK_PROGRESS_THREAD (1<<3)
#define KIND_MASK_MPC_POLLING_THREAD (1<<4)
//TODO add all kinds

#define KIND_MASK_MPI_OMP (KIND_MASK_MPI | KIND_MASK_OMP)
#define KIND_MASK_MPI_PTHREAD (KIND_MASK_MPI | KIND_MASK_PTHREAD)

#define KIND_MASK_MPI_OMP_PTHREAD (KIND_MASK_MPI | KIND_MASK_OMP | KIND_MASK_PTHREAD)



/**
 * @brief  the member mask is a mask of bits
 *
 */
typedef struct sctk_thread_generic_kind_s{
    unsigned int mask;
    int priority;
} sctk_thread_generic_kind_t;
#define sctk_thread_generic_kind_init {0,-1}


///////////////////////////////////
//setter
//

/**
 * @brief set kind to the current thread
 *
 * @param kind type of the thread
 */
void sctk_thread_generic_setkind_self(sctk_thread_generic_kind_t kind);

/**
 * @brief set kind.mask to the current thread
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void sctk_thread_generic_setkind_mask_self(unsigned int kind_mask);

/**
 * @brief set kind.priority to the current thread
 *
 * @param priority scheduling priority of the current thread
 */
void sctk_thread_generic_setkind_priority_self(int priority);

/**
 * @brief set basic_priority
 *
 * @param basic_priority scheduling priority of beginning
 */
void sctk_thread_generic_set_basic_priority_self(int basic_priority);

/**
 * @brief set current_priority 
 *
 * @param current_priority currrent priority of the thread
 */
void sctk_thread_generic_set_current_priority_self(int current_priority);

///////////////////////////////////
//getter
//


/**
 * @brief get kind
 *
 * @return a copy of the current thread's kind
 */
sctk_thread_generic_kind_t sctk_thread_generic_getkind_self();

/**
 * @brief get kind.mask
 *
 * @return a copy of the current thread's kind.mask
 */
unsigned int sctk_thread_generic_getkind_mask_self();

/**
 * @brief get kind.priority
 *
 * @return a copy of the current thread's kind.priority
 */
int sctk_thread_generic_getkind_priority_self();


///////////////////////////////////
//functions
//


/**
 * @brief add kind.mask to the current mask 
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void sctk_thread_generic_addkind_mask_self(unsigned int kind_mask);

/**
 * @brief remove kind.mask to the current mask 
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void sctk_thread_generic_removekind_mask_self(unsigned int kind_mask);

#endif //__SCTK_THREAD_GENERIC_KIND_H_

