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
#ifndef LOWCOMM_MULTIRAIL_H_
#define LOWCOMM_MULTIRAIL_H_

#include <lowcomm_config.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_types.h>
#include <mpc_common_datastructure.h>
#include <mpc_lowcomm_monitor.h>

#include "sctk_rail.h"

/************************************************************************/
/* _mpc_lowcomm_endpoint_list                                           */
/************************************************************************/


/**
 * @brief This in an entry in the enpoint list associated with each destination.
 */
typedef struct _mpc_lowcomm_multirail_endpoint_list_entry_s
{
	int                                         in_use;     /*< The slot is in use */
	_mpc_lowcomm_endpoint_t *                   endpoint;   /*< The corresponding endpoint */
	/* Mirrored from RAIL for performance */
	int                                         priority;   /*< Priority for this entry */
	struct _mpc_lowcomm_config_struct_net_gate *gates;      /*< List of gates */
	int                                         gate_count; /*< Number of gates */
}_mpc_lowcomm_multirail_endpoint_list_entry_t;

/**
 * @brief This contains the endpoint list for each destination
 *        this list is ordered in decreasing priority
 */
typedef struct _mpc_lowcomm_multirail_endpoint_list_s
{
	_mpc_lowcomm_multirail_endpoint_list_entry_t *entries;
	unsigned int                                  size;
	unsigned int                                  elem_count;
}_mpc_lowcomm_multirail_endpoint_list_t;

/**
 * @brief Initialize the endpoint list
 *
 * @param list the list to initialize
 * @return int 0 on success
 */
int _mpc_lowcomm_multirail_endpoint_list_init(_mpc_lowcomm_multirail_endpoint_list_t *list);

/**
 * @brief Release the endpoint list
 *
 * @param list the endpoint list to release
 * @return int 0 on success
 */
int _mpc_lowcomm_multirail_endpoint_list_release(_mpc_lowcomm_multirail_endpoint_list_t *list);

/**
 * @brief Push a new endpoint in the endpoint list
 *
 * @param list the list to push to
 * @param endpoint the endpoint to insert in the list
 *                 (its rail priority determines resulting order)
 * @return int 0 on success
 */
int _mpc_lowcomm_multirail_endpoint_list_push(_mpc_lowcomm_multirail_endpoint_list_t *list,
                                              _mpc_lowcomm_endpoint_t *endpoint);

/**
 * @brief Remove an endpoint from the endpoint list
 *
 * @param list the list to remove the endpoint from
 * @param topop the endpoint to pop
 * @return int 0 on success
 */
int _mpc_lowcomm_multirail_endpoint_list_pop_endpoint(_mpc_lowcomm_multirail_endpoint_list_t *list,
                                                      _mpc_lowcomm_endpoint_t *topop);

/**
 * @brief Remove disabled (state == SCTK_RAIL_ST_DISABLED) rails from the endpoint list
 *
 * @param list the endpoint list to be scanned
 */
void _mpc_lowcomm_multirail_endpoint_list_prune(_mpc_lowcomm_multirail_endpoint_list_t *list);

/************************************************************************/
/* _mpc_lowcomm_multirail_table                                     */
/************************************************************************/

typedef struct _mpc_lowcomm_multirail_table_entry_s
{
	_mpc_lowcomm_multirail_endpoint_list_t endpoints;
	mpc_common_rwlock_t                    endpoints_lock;

	mpc_lowcomm_peer_uid_t                 destination;
}_mpc_lowcomm_multirail_table_entry_t;

void _mpc_lowcomm_multirail_table_entry_init(_mpc_lowcomm_multirail_table_entry_t *entry, mpc_lowcomm_peer_uid_t destination);
void _mpc_lowcomm_multirail_table_entry_release(_mpc_lowcomm_multirail_table_entry_t *entry);
void _mpc_lowcomm_multirail_table_entry_free(_mpc_lowcomm_multirail_table_entry_t *entry);

_mpc_lowcomm_multirail_table_entry_t *_mpc_lowcomm_multirail_table_entry_new(mpc_lowcomm_peer_uid_t destination);

void _mpc_lowcomm_multirail_table_entry_push_endpoint(_mpc_lowcomm_multirail_table_entry_t *entry, _mpc_lowcomm_endpoint_t *endpoint);
void _mpc_lowcomm_multirail_table_entry_pop_endpoint(_mpc_lowcomm_multirail_table_entry_t *entry, _mpc_lowcomm_endpoint_t *endpoint);

struct _mpc_lowcomm_multirail_table
{
	struct mpc_common_hashtable destination_table;
	mpc_common_rwlock_t         table_lock;
};

void _mpc_lowcomm_multirail_table_init();
void _mpc_lowcomm_multirail_table_release();

_mpc_lowcomm_multirail_table_entry_t *_mpc_lowcomm_multirail_table_acquire_routes(mpc_lowcomm_peer_uid_t destination);
void _mpc_lowcomm_multirail_table_relax_routes(_mpc_lowcomm_multirail_table_entry_t *entry);

void _mpc_lowcomm_multirail_table_push_endpoint(_mpc_lowcomm_endpoint_t *endpoint);
void _mpc_lowcomm_multirail_table_pop_endpoint(_mpc_lowcomm_endpoint_t *topop);
void _mpc_lowcomm_multirail_table_prune(void);

/************************************************************************/
/* Pending on-demand connections                                        */
/************************************************************************/

typedef struct sctk_pending_on_demand_s
{
	sctk_rail_info_t *               rail;
	mpc_lowcomm_peer_uid_t           dest;
	struct sctk_pending_on_demand_s *next;
}sctk_pending_on_demand_t;

void sctk_pending_on_demand_push(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);

/******************************
* MULTIRAIL DRIVER INTERFACE *
******************************/

void _mpc_lowcomm_multirail_send_message(mpc_lowcomm_ptp_message_t *msg);
void _mpc_lowcomm_multirail_notify_receive(mpc_lowcomm_ptp_message_t *msg);
void _mpc_lowcomm_multirail_notify_matching(mpc_lowcomm_ptp_message_t *msg);
void _mpc_lowcomm_multirail_notify_perform(mpc_lowcomm_peer_uid_t remote_process, int remote_task_id, int polling_task_id, int blocking);
void _mpc_lowcomm_multirail_notify_idle();
void _mpc_lowcomm_multirail_notify_anysource(int polling_task_id, int blocking);
void _mpc_lowcomm_multirail_notify_new_comm(int comm_idx, size_t comm_size);
void _mpc_lowcomm_multirail_notify_probe(mpc_lowcomm_ptp_message_header_t *hdr, int *status);

#endif /* LOWCOMM_MULTIRAIL_H_ */
