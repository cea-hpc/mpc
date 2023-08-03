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
#include <mpc_lowcomm_handle_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

/** This defines an unknown integer value */
#define MPC_UNDEFINED    -32766

struct MPI_ABI_Group;
typedef struct MPI_ABI_Group mpc_lowcomm_group_t;

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

/**
 * @brief Duplicate a group handle
 *
 * @warning DUP may return the *same* handle !
 * 
 * @param g the group to duplicate
 * @return mpc_lowcomm_group_t* a pointer to the duplicated group
 */
mpc_lowcomm_group_t *mpc_lowcomm_group_dup(mpc_lowcomm_group_t *g);


/**
 * @brief Duplicate a group handle (full copy)
 *
 * @note This forces a handle copy (when storing info is needed)
 * 
 * @param g the group to duplicate
 * @return mpc_lowcomm_group_t* a pointer to the duplicated group
 */
mpc_lowcomm_group_t *mpc_lowcomm_group_copy(mpc_lowcomm_group_t *g);


/***************
* GROUP QUERY *
***************/

/**
 * @brief Resolve a group from its linear id
 *
 * @param linear_id linear id to search for
 * @return mpc_lowcomm_group_t* the corresponding group NULL if not found
 */
mpc_lowcomm_group_t *mpc_lowcomm_group_from_id(int linear_id);

/**
 * @brief Get the linear id for a group
 *
 * @param group the group to querry
 * @return int the corresponding linear ID
 */
int mpc_lowcomm_group_linear_id(mpc_lowcomm_group_t * group);

/**
 * @brief Result when comparing two groups
 *
 */
typedef enum
{
	MPC_GROUP_IDENT,     /**< Identical groups */
    MPC_GROUP_CONGRUENT, /**< Groups are congruent */
	MPC_GROUP_SIMILAR,   /**< Same ranks but different order */
	MPC_GROUP_UNEQUAL    /**< Different groups */
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
 * @brief Get the empty group
 *
 * @return mpc_lowcomm_group_t* a pointer to the empty group
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_empty(void);

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
int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank, mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Translate ranks from one group to another
 *
 * @param g1 the first group
 * @param n size of the ranklist (ranks1 (in) and ranks2(out))
 * @param ranks1 the input rank list to be translated from g1 to g2
 * @param g2 the second group
 * @param ranks2 the output rank list
 * @return int MPC_LOWCOMM_SUCCESS when all ok
 */
int mpc_lowcomm_group_translate_ranks(mpc_lowcomm_group_t *g1,
                                      int n,
                                      const int ranks1[],
                                      mpc_lowcomm_group_t *g2,
                                      int ranks2[]);

/**
 * @brief Exclude given ranks to build a new group
 *
 * @param grp the group to filter
 * @param n the number of ranks to exclude
 * @param ranks the ranks to exclude
 * @return mpc_lowcomm_group_t* new group (NULL if error)
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_excl(mpc_lowcomm_group_t *grp,
                                             int n,
                                             const int ranks[]);

/**
 * @brief Include given ranks to build a new group
 *
 * @param grp the group to filter
 * @param n the number of ranks to exclude
 * @param ranks the ranks to include
 * @return mpc_lowcomm_group_t* new group (NULL if error)
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_incl(mpc_lowcomm_group_t *grp,
                                             int n,
                                             const int ranks[]);

/**
 * @brief Substract member of a group from another to build a new group
 *
 * @param grp the group to use as reference
 * @param grp_to_sub the members to substract
 * @return mpc_lowcomm_group_t* new group (NULL if error)
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_difference(mpc_lowcomm_group_t *grp,
                                                   mpc_lowcomm_group_t *grp_to_sub);

/**
 * @brief Compute the group intersecting two groups
 *
 * @param grp first group
 * @param grp2 second group
 * @return mpc_lowcomm_group_t* the intersecting group
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_instersection(mpc_lowcomm_group_t *grp, mpc_lowcomm_group_t *grp2);

/**
 * @brief Merge two groups to build a new one
 *
 * @param grp first group
 * @param grp2 second group
 * @return mpc_lowcomm_group_t* new group (NULL if error)
 */
mpc_lowcomm_group_t * mpc_lowcomm_group_union(mpc_lowcomm_group_t *grp,
                                              mpc_lowcomm_group_t *grp2);

/**
 * @brief Check if a given comm workd rank is part of a group
 *
 * @param g the group to check
 * @param cw_rank the comm_world rank
 * @param uid the UID of the rank (to differenciate sets)
 * @return int true if the rank is part of the group
 */
int mpc_lowcomm_group_includes(mpc_lowcomm_group_t *g, int cw_rank, mpc_lowcomm_peer_uid_t uid);

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
mpc_lowcomm_peer_uid_t mpc_lowcomm_group_process_uid_for_rank(mpc_lowcomm_group_t *g, int rank);

/**
 * @brief Get the list of world ranks for a group (must be local)
 *
 * @param g the group
 * @return int* the rank list in comm_world (NULL if error)
 */
int * mpc_lowcomm_group_world_ranks(mpc_lowcomm_group_t *g);

/*****************************************
 * EXTRA CTX POINTERS (SESSIONS HANDLES) *
 *****************************************/

/**
 * @brief Get the context pointer from a group
 * 
 * @param g the group handle to query
 * @return mpc_lowcomm_handle_ctx_t the context pointer (NULL if None)
 */
mpc_lowcomm_handle_ctx_t mpc_lowcomm_group_get_context_pointer(mpc_lowcomm_group_t * g);

/**
 * @brief Add an extra context pointer to the handle (note MUST be copied)
 * 
 * @param g the group to add information to
 * @param ctxptr the context pointer to add
 * @return int 0 on success 
 */
int mpc_lowcomm_group_set_context_pointer(mpc_lowcomm_group_t * g, mpc_lowcomm_handle_ctx_t ctxptr);

/****************
* PROCESS SETS *
****************/

#define MPC_LOWCOMM_GROUP_MAX_PSET_NAME_LEN 256

/**
 * @brief This is the storage for a process set
 *
 */
struct mpc_lowcomm_proces_set_s
{
	char                 name[MPC_LOWCOMM_GROUP_MAX_PSET_NAME_LEN];  /*<< PSET name */
	mpc_lowcomm_group_t *group; /*<< PSET group */
};

typedef struct mpc_lowcomm_proces_set_s mpc_lowcomm_process_set_t;

/**
 * @brief Get the number of process set currently available
 *
 * @return int number of available process sets
 */
int mpc_lowcomm_group_pset_count(void);

/**
 * @brief Get the nth process set
 *
 * @param n index of the process set to get
 * @return mpc_lowcomm_process_set_t* pointer to process set null if not found
 */
mpc_lowcomm_process_set_t *mpc_lowcomm_group_pset_get_nth(int n);

/**
 * @brief Get a process set by name
 *
 * @param name name to retrieve
 * @return mpc_lowcomm_process_set_t* pointer to process set null if not found
 */
mpc_lowcomm_process_set_t *mpc_lowcomm_group_pset_get_by_name(const char *name);

/**
 * @brief Free a retrieved process set
 *
 * @param pset the pset to free
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int mpc_lowcomm_group_pset_free(mpc_lowcomm_process_set_t *pset);

/**
 * @brief Register PSET using initialized MPC
 *
 * @return int MPC_LOWCOMM_SUCCESS if all ok
 */
int _mpc_lowcomm_pset_bootstrap(void);


#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_GROUP_INC_H */
