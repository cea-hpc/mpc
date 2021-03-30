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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LOWCOMM_GROUP_INC_H
#define MPC_LOWCOMM_GROUP_INC_H

#include <mpc_lowcomm_monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

/** This defines an unknown integer value */
#define MPC_UNDEFINED    -1

struct _mpc_lowcomm_group_s;
typedef struct _mpc_lowcomm_group_s mpc_lowcomm_group_t;

/*******************************
* CONSTRUCTORS AND DESTUCTORS *
*******************************/

/**
 * @brief Create a group using comm world ranks
 *
 * @param size size of the groups
 * @param comm_world_ranks ranks to be part of the group
 * @return mpc_lowcomm_group_t* a newly allocated group
 */
mpc_lowcomm_group_t *mpc_lowcomm_group_create(unsigned int size, int *comm_world_ranks);

/**
 * @brief Release a group
 *
 * @param group pointer to the group to be freed (set to NULL)
 * @return int SCTK_SUCESS if all ok
 */
int mpc_lowcomm_group_free(mpc_lowcomm_group_t **group);


mpc_lowcomm_group_t *mpc_lowcomm_group_dup(mpc_lowcomm_group_t *g);

/***************
* GROUP QUERY *
***************/

/**
 * @brief Result when comparing two groups
 *
 */
typedef enum
{
	MPC_GROUP_IDENT,    /**< Identical groups */
	MPC_GROUP_SIMILAR,  /**< Same ranks but different order */
	MPC_GROUP_UNEQUAL   /**< Different groups */
}mpc_lowcomm_group_eq_e;

/**
 * @brief Compare two groups
 *
 * @param g1 the first group
 * @param g2 the second group
 * @return mpc_lowcomm_group_eq_e return the corresponding comparison value
 */
mpc_lowcomm_group_eq_e mpc_lowcomm_group_compare(mpc_lowcomm_group_t *g1,
                                                 mpc_lowcomm_group_t *g2);

/**
 * @brief Get the world rank for a member of group
 *
 * @param g group to look into
 * @param rank the rank to search for (in group)
 * @return int the corresponding world rank
 */
int mpc_lowcomm_group_world_rank(mpc_lowcomm_group_t *g, int rank);

/**
 * @brief Get the size of a group
 *
 * @param g the group to query
 * @return unsigned int the group size
 */
unsigned int mpc_lowcomm_group_size(mpc_lowcomm_group_t *g);

/**
 * @brief Get rank of current process in group
 *
 * @param g the group to query
 * @return int the rank or MPC_PROC_NULL if not present
 */
int mpc_lowcomm_group_rank(mpc_lowcomm_group_t *g);

/**
 * @brief Get rank of rank process in group
 *
 * @param g the group to query
 * @param rank the rank to search for
 *
 * @return int the rank or MPC_PROC_NULL if not present
 */
int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank);

/**
 * @brief Translate ranks from one group to another
 * 
 * @param g1 the first group
 * @param n size of the ranklist (ranks1 (in) and ranks2(out))
 * @param ranks1 the input rank list to be translated from g1 to g2
 * @param g2 the second group
 * @param ranks2 the output rank list
 * @return int SCTK_SUCCESS when all ok
 */
int mpc_lowcomm_group_translate_ranks(mpc_lowcomm_group_t *g1,
                                      int n,
                                      const int ranks1[],
                                      mpc_lowcomm_group_t *g2,
                                      int ranks2[]);
/**
 * @brief Check if a given comm workd rank is part of a group
 * 
 * @param g the group to check
 * @param cw_rank the comm_world rank
 * @return int true if the rank is part of the group
 */
int mpc_lowcomm_group_includes(mpc_lowcomm_group_t *g, int cw_rank);

/**
 * @brief Get the process local leader rank in a given group
 * 
 * @param g the group to query
 * @return int the group rank of the process leader
 */
int mpc_lowcomm_group_get_local_leader(mpc_lowcomm_group_t *g);

/*******************
* PROCESS RANKING *
*******************/

/**
 * @brief Get the process rank for the world rank
 *
 * @param comm_world_rank world rank
 * @return int the process rank
 */
int mpc_lowcomm_group_process_rank_from_world(int comm_world_rank);

/**
 * @brief Get host process UID for a given rank
 *
 * @param g the group to look into
 * @param rank the rank of interest
 * @return int the corresponding process UID
 */
mpc_lowcomm_peer_uid_t mpc_lowcomm_group_process_uid_for_rank(mpc_lowcomm_group_t * g, int rank);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_GROUP_INC_H */
