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
#ifndef MPC_LOWCOMM_MONITOR_H_
#define MPC_LOWCOMM_MONITOR_H_

#include <stdlib.h>
#include <mpc_common_types.h>

/***************
* ERROR CODES *
***************/

typedef enum
{
	MPC_LOWCOMM_MONITOR_RET_SUCCESS,
	MPC_LOWCOMM_MONITOR_RET_ERROR,
	MPC_LOWCOMM_MONITOR_RET_DUPLICATE_KEY,
	MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE,
	MPC_LOWCOMM_MONITOR_RET_INVALID_UID,
	MPC_LOWCOMM_MONITOR_RET_REFUSED
}mpc_lowcomm_monitor_retcode_t;

void mpc_lowcomm_monitor_retcode_print(mpc_lowcomm_monitor_retcode_t code, const char *ctx);

/*******************
* ID MANIPULATION *
*******************/

typedef uint32_t   mpc_lowcomm_set_uid_t;
typedef uint64_t   mpc_lowcomm_peer_uid_t;

mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid();
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_get_gid();

static inline mpc_lowcomm_set_uid_t mpc_lowcomm_peer_get_set(mpc_lowcomm_peer_uid_t uid)
{
	return uid >> 32;
}

static inline int mpc_lowcomm_peer_get_rank(mpc_lowcomm_peer_uid_t uid)
{
	return (int)uid;
}

static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_uid_of(uint32_t set_uid, int peer_rank)
{
	uint64_t ret = 0;
	uint64_t lset_uid = set_uid;

	ret |= lset_uid << 32;
	ret |= peer_rank;

	return ret;
}

static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_local_uid_of(int peer_rank)
{
	return mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), peer_rank);
}

/******************
 * PEER FUNCTIONS *
 ******************/

char * mpc_lowcomm_peer_format_r(mpc_lowcomm_peer_uid_t uid, char * buff, int len);
char * mpc_lowcomm_peer_format(mpc_lowcomm_peer_uid_t uid);
int mpc_lowcomm_peer_closer(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_peer_uid_t current, mpc_lowcomm_peer_uid_t candidate);
int mpc_lowcomm_monitor_peer_reachable_directly(mpc_lowcomm_peer_uid_t target_peer);

int mpc_lowcomm_monitor_peer_exists(mpc_lowcomm_peer_uid_t peer);

/*****************
 * SET INTERFACE *
 *****************/

typedef void * mpc_lowcomm_monitor_set_t;

int mpc_lowcomm_monitor_set_iterate(int (*callback)(mpc_lowcomm_monitor_set_t set, void * arg),
									void * arg);

char * mpc_lowcomm_monitor_set_name(mpc_lowcomm_monitor_set_t set);

mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_set_gid(mpc_lowcomm_monitor_set_t set);

mpc_lowcomm_monitor_set_t mpc_lowcomm_monitor_set_get(mpc_lowcomm_set_uid_t gid);

int mpc_lowcomm_monitor_set_contains(mpc_lowcomm_set_uid_t gid, mpc_lowcomm_peer_uid_t uid);

#define MPC_LOWCOMM_PEER_URI_SIZE     128

typedef struct mpc_lowcomm_monitor_peer_info_s
{
	mpc_lowcomm_peer_uid_t uid;
	uint64_t               local_task_count;
	char                   uri[MPC_LOWCOMM_PEER_URI_SIZE];
}mpc_lowcomm_monitor_peer_info_t;

uint64_t mpc_lowcomm_monitor_set_peer_count(mpc_lowcomm_monitor_set_t set);
int mpc_lowcomm_monitor_set_peers(mpc_lowcomm_monitor_set_t set, mpc_lowcomm_monitor_peer_info_t * peers, uint64_t peers_len);

/******************
 * PORT INTERFACE *
 ******************/

/* This mimicks the MPI Port Mechanism */

/**
 * @brief Open a new port for Connect accept
 *
 * @param id output string describing the new port
 * @param id_len the length of the string
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_open_port(char * id, int id_len);

/**
 * @brief Close a previously openned port
 *
 * @param id the port to be freed
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_close_port(const char * id);

/**
 * @brief Parse port information
 *
 * @param id the id to be parsed
 * @param uid the peer UID contained in the port description
 * @param port the port ID contained in the port description
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_parse_port(const char * id,  mpc_lowcomm_peer_uid_t *uid, int * port);


/************
* COMMANDS *
************/

typedef enum
{
	MPC_LOWCOMM_MONITOR_COMMAND_NONE,
	MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_PEER_INFO,
	MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_SET_INFO,
	MPC_LOWCOMM_MONITOR_PING,
	MPC_LOWCOMM_MONITOR_ON_DEMAND,
	MPC_LOWCOMM_MONITOR_CONNECTIVITY,
	MPC_LOWCOMM_MONITOR_COMM_INFO,
	MPC_LOWCOMM_MONITOR_NAMING
}mpc_lowcomm_monitor_command_t;

typedef enum
{
	MPC_LOWCOMM_MONITOR_NAMING_PUT,
	MPC_LOWCOMM_MONITOR_NAMING_GET,
	MPC_LOWCOMM_MONITOR_NAMING_DEL,
	MPC_LOWCOMM_MONITOR_NAMING_LIST
}mpc_lowcomm_monitor_command_naming_t;

const char * mpc_lowcomm_monitor_command_tostring(mpc_lowcomm_monitor_command_t cmd);

#define MPC_LOWCOMM_MONITOR_KEY_LEN    128
#define MPC_LOWCOMM_SET_NAME_LEN      256
#define MPC_LOWCOMM_ONDEMAND_TARGET_LEN 32
#define MPC_LOWCOMM_ONDEMAND_DATA_LEN MPC_LOWCOMM_PEER_URI_SIZE
#define MPC_LOWCOMM_MONITOR_MAX_CLIENTS 32

typedef union
{
	struct
	{
		mpc_lowcomm_monitor_peer_info_t info;
		mpc_lowcomm_peer_uid_t          requested_peer;
		mpc_lowcomm_monitor_retcode_t   retcode;
	}peer;

	struct
	{
		mpc_lowcomm_set_uid_t           uid;
		mpc_lowcomm_monitor_retcode_t   retcode;
		char                            name[MPC_LOWCOMM_SET_NAME_LEN];
		uint64_t                        total_task_count;
		uint64_t                        peer_count;
		mpc_lowcomm_monitor_peer_info_t peers[];
	}set_info;

	struct
	{
		uint64_t                      ping;
		mpc_lowcomm_monitor_retcode_t retcode;
	}ping;

	struct
	{
		char target[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];
		char data[MPC_LOWCOMM_ONDEMAND_DATA_LEN];
		mpc_lowcomm_monitor_retcode_t retcode;
	}on_demand;

	struct
	{
		uint64_t peers_count;
		mpc_lowcomm_peer_uid_t peers[0];
	}connectivity;

	struct
	{
		uint64_t id;
		int size;
		mpc_lowcomm_monitor_retcode_t retcode;
		mpc_lowcomm_peer_uid_t uids[];
	}comm_info;

	struct
	{
		mpc_lowcomm_monitor_command_naming_t operation;
		mpc_lowcomm_peer_uid_t hosting_peer;
		char name[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];
		char port_name[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];
		mpc_lowcomm_monitor_retcode_t retcode;
		char name_list[0];
	}naming;

}mpc_lowcomm_monitor_args_t;

typedef void *mpc_lowcomm_monitor_response_t;

mpc_lowcomm_monitor_args_t *mpc_lowcomm_monitor_response_get_content(mpc_lowcomm_monitor_response_t response);

int mpc_lowcomm_monitor_response_free(mpc_lowcomm_monitor_response_t response);

/****************
 * GET SET INFO *
 ****************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_get_set_info(mpc_lowcomm_peer_uid_t target_peer,
                                                                mpc_lowcomm_monitor_retcode_t *ret);

/*****************
 * GET PEER INFO *
 *****************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_command_get_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                         mpc_lowcomm_peer_uid_t requested_peer,
                                                                         mpc_lowcomm_monitor_retcode_t *ret);

/***************
 * PING REMOTE *
 ***************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ping(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

/***************
 * COMM INFO    *
 ***************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_comm_info(mpc_lowcomm_peer_uid_t dest,
															 uint64_t target_id,
															 mpc_lowcomm_monitor_retcode_t *ret);

/*********************
 * ON DEMAND SUPPORT *
 *********************/

typedef int (*mpc_lowcomm_on_demand_callback_t)(mpc_lowcomm_peer_uid_t from,
												char *data,
												char * return_data,
												int return_data_len,
												void *ctx);

int mpc_lowcomm_monitor_register_on_demand_callback(char *target,
                                                    mpc_lowcomm_on_demand_callback_t callback,
													void * ctx);

mpc_lowcomm_on_demand_callback_t mpc_lowcomm_monitor_get_on_demand_callback(char *target, void **ctx);

int mpc_lowcomm_monitor_unregister_on_demand_callback(char * target);

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ondemand(mpc_lowcomm_peer_uid_t dest,
															char *target,
															char *data,
															mpc_lowcomm_monitor_retcode_t *ret);

/**********************
 * EVENT LOOP SUPPORT *
 **********************/

typedef void (*mpc_lowcomm_monitor_worker_callback_t)(void *ctx);

int mpc_lowcomm_monitor_event_loop_push(mpc_lowcomm_monitor_worker_callback_t callback, void *arg);

/*************************
 * GET CONNECTIVITY INFO *
 *************************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_connectivity(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

void mpc_lowcomm_monitor_synchronous_connectivity_dump(void);

/* Set Naming */

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_naming(mpc_lowcomm_peer_uid_t dest,
														  mpc_lowcomm_monitor_command_naming_t operation,
														  mpc_lowcomm_peer_uid_t hosting_peer,
														  const char * name,
														  const char * port_name,
														  mpc_lowcomm_monitor_retcode_t *ret);


#endif /* MPC_LOWCOMM_MONITOR_H_ */
