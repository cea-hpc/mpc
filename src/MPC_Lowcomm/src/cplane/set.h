/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.fr                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef _MPC_LOWCOMM_SET_H_
#define _MPC_LOWCOMM_SET_H_

#include <mpc_common_types.h>

#include "peer.h"

typedef struct _mpc_lowcomm_set_s
{
	int                    is_lead; /**< True if this process is the root for the set */
	mpc_lowcomm_set_uid_t  gid; /**< GID of this set */
	mpc_lowcomm_peer_uid_t local_peer; /**w UID for the current process in this set */
	char                   name[MPC_LOWCOMM_SET_NAME_LEN]; /* Name of this set */
	uint64_t               total_task_count; /**w Number of tasks in the set */

	_mpc_lowcomm_peer_t ** peers;	/**< List of processes in the set */
	uint64_t               peer_count; /**< Number of processes in the set */
}_mpc_lowcomm_set_t;

/*****************
* SETUP TEADOWN *
*****************/

/**
 * @brief Initialize the set interface
 *
 * @return int 0 if all ok
 */
int _mpc_lowcomm_set_setup();

/**
 * @brief Release the set interface
 *
 * @return int 0 if all ok
 */
int _mpc_lowcomm_set_teardown();

/**********************
* REGISTER INTERFACE *
**********************/

/**
 * @brief Initialize a new set
 *
 * @param gid group id of this new set
 * @param name name of this new set
 * @param total_task_count total number of tasks in the set
 * @param peers_uids List of uids for the participating peers
 * @param peer_count number of participating peers
 * @param is_lead is current process the lead (rank 0)
 * @param local_peer local process UID
 * @return _mpc_lowcomm_set_t* pointer to a newly allocated
 *                             set to free with @ref _mpc_lowcomm_set_free
 */
_mpc_lowcomm_set_t *_mpc_lowcomm_set_init(mpc_lowcomm_set_uid_t gid,
                                          char *name,
                                          uint64_t total_task_count,
                                          uint64_t *peers_uids,
                                          uint64_t peer_count,
                                          int is_lead,
										  mpc_lowcomm_peer_uid_t local_peer);

/**
 * @brief Free a given set
 *
 * @param set the set to free
 * @return int 0 if all ok
 */
int _mpc_lowcomm_set_free(_mpc_lowcomm_set_t *set);

/**
 * @brief This loads the sets from the file-system
 *
 * @return int 0 if all ok
 */
int _mpc_lowcomm_set_load_from_system(void);

/*******************
* QUERY INTERFACE *
*******************/

/**
 * @brief Get a set information from its GID
 *
 * @param gid the id to retrieve
 * @return _mpc_lowcomm_set_t* pointer to set if found NULL otherwise
 */
_mpc_lowcomm_set_t *_mpc_lowcomm_set_get(mpc_lowcomm_set_uid_t gid);

/**
 * @brief Check if a set contains a given peer UID
 *
 * @param set the set to check
 * @param peer_uid the Peer UID to test
 * @return int true if the set contains peer_uid
 */
int _mpc_lowcomm_set_contains(_mpc_lowcomm_set_t * set, mpc_lowcomm_peer_uid_t peer_uid);

/**
 * @brief Call a given callback on all the known sets
 *
 * @param set_cb the callback to call
 * @param arg argument to pass to the callback (if callback returns != 0 iteration stops)
 * @return int 0 if all OK
 */
int _mpc_lowcomm_set_iterate(int (*set_cb)(mpc_lowcomm_monitor_set_t set, void *arg), void *arg);

/**
 * @brief Retun a list of process set root UIDs (current is always first)
 *
 * @param root_table_len the length of the returned table
 * @return mpc_lowcomm_peer_uid_t* the table of process root (current first)
 */
mpc_lowcomm_peer_uid_t * _mpc_lowcomm_get_set_roots(int * root_table_len);

/**
 * @brief Free the process root table from @ref _mpc_lowcomm_get_set_roots
 *
 * @param roots process root table to be freed
 */
void _mpc_lowcomm_free_set_roots(mpc_lowcomm_peer_uid_t * roots);

#endif /* _MPC_LOWCOMM_SET_H_ */
