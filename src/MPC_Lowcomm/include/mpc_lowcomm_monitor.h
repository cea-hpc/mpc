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

/**
 * @brief Print the value of a return code
 *
 * @param code the code to print
 * @param ctx the context to add
 */
void mpc_lowcomm_monitor_retcode_print(mpc_lowcomm_monitor_retcode_t code, const char *ctx);

/*******************
* ID MANIPULATION *
*******************/

typedef uint32_t   mpc_lowcomm_set_uid_t;
typedef uint64_t   mpc_lowcomm_peer_uid_t;

/**
 * @brief Get the UID of current process
 *
 * @return mpc_lowcomm_peer_uid_t uid for current process
 */
mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid();

/**
 * @brief Get the current process GID
 *
 * @return mpc_lowcomm_set_uid_t GID of current process
 */
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_get_gid();

/**
 * @brief Get the set GID for a given peer
 *
 * @param uid the UID to decode the set from
 * @return mpc_lowcomm_set_uid_t the set for this UID
 */
static inline mpc_lowcomm_set_uid_t mpc_lowcomm_peer_get_set(mpc_lowcomm_peer_uid_t uid)
{
	return uid >> 32;
}

/**
 * @brief Get the rank from a peer UID
 *
 * @param uid the UID to decode
 * @return int the rank for this UID
 */
static inline int mpc_lowcomm_peer_get_rank(mpc_lowcomm_peer_uid_t uid)
{
	return (int)uid;
}

/**
 * @brief Build an UID from both the set and the rank
 *
 * @param set_uid hosting set
 * @param peer_rank peer rank
 * @return mpc_lowcomm_peer_uid_t corresponding UID
 */
static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_uid_of(uint32_t set_uid, int peer_rank)
{
	uint64_t ret      = 0;
	uint64_t lset_uid = set_uid;

	ret |= lset_uid << 32;
	ret |= peer_rank;

	return ret;
}

/**
 * @brief Get an UID for a rank in our local set
 *
 * @param peer_rank the rank to compute the local UID
 * @return mpc_lowcomm_peer_uid_t the corresponding UID in local set
 */
static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_local_uid_of(int peer_rank)
{
	return mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), peer_rank);
}

/******************
* PEER FUNCTIONS *
******************/

/**
 * @brief Pretty print a peer UID (external buffer version)
 *
 * @param uid the UID to print
 * @param buff the buffer to store in
 * @param len the lenght of the storage buffer
 * @return char* pointer to buff
 */
char *mpc_lowcomm_peer_format_r(mpc_lowcomm_peer_uid_t uid, char *buff, int len);

/**
 * @brief Pretty print peer UID (static buffer version)
 *
 * @warning not reentrant !
 * @param uid the UID to print
 * @return char* pointer to internal static buffer
 */
char *mpc_lowcomm_peer_format(mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Check if a peer is closer than anoter one
 *
 * @param dest the destination peer
 * @param current current peer to check for (0 means none)
 * @param candidate the peer to consider
 * @return int 1 if candidate is closer than current (!=0) to dest
 */
int mpc_lowcomm_peer_closer(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_peer_uid_t current, mpc_lowcomm_peer_uid_t candidate);

/**
 * @brief Check if the control plane is already connected to a given UID
 *
 * @param target_peer the target UID
 * @return int 1 if the peer is already connected
 */
int mpc_lowcomm_monitor_peer_reachable_directly(mpc_lowcomm_peer_uid_t target_peer);

/**
 * @brief Check if a peer exists
 *
 * @param peer the peer to check for existency
 * @return int 1 if it does exists
 */
int mpc_lowcomm_monitor_peer_exists(mpc_lowcomm_peer_uid_t peer);

/*****************
* SET INTERFACE *
*****************/

/**
 * @brief This is the public type of a set
 *
 */
typedef void *mpc_lowcomm_monitor_set_t;

/**
 * @brief Iterate all sets with a callback
 *
 * @param callback the callback function (returning -1 interupts walk)
 * @param arg extra argument to pass to the callback
 * @return int 0 if not interupted
 */
int mpc_lowcomm_monitor_set_iterate(int (*callback)(mpc_lowcomm_monitor_set_t set, void *arg),
                                    void *arg);

/**
 * @brief Get a set name from its handle
 *
 * @param set set handle
 * @return char* the set name
 */
char *mpc_lowcomm_monitor_set_name(mpc_lowcomm_monitor_set_t set);

/**
 * @brief Get a set GID from its handle
 *
 * @param set the set handle
 * @return mpc_lowcomm_set_uid_t the set GID
 */
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_set_gid(mpc_lowcomm_monitor_set_t set);

/**
 * @brief Get a set from its GID
 *
 * @param gid the GID to lookup
 * @return mpc_lowcomm_monitor_set_t the corresponding set (NULL if not found)
 */
mpc_lowcomm_monitor_set_t mpc_lowcomm_monitor_set_get(mpc_lowcomm_set_uid_t gid);

/**
 * @brief Check if a set contains a given UID
 *
 * @param gid the GID of the set to query
 * @param uid the UID to look for in the set
 * @return int 1 if the UID is in the given set
 */
int mpc_lowcomm_monitor_set_contains(mpc_lowcomm_set_uid_t gid, mpc_lowcomm_peer_uid_t uid);

#define MPC_LOWCOMM_PEER_URI_SIZE    128

/**
 * @brief This defines the informations for a given peer
 *
 */
typedef struct mpc_lowcomm_monitor_peer_info_s
{
	mpc_lowcomm_peer_uid_t uid;                            /**< Peer UID */
	uint64_t               local_task_count;               /**< Number of tasks in the peer */
	char                   uri[MPC_LOWCOMM_PEER_URI_SIZE]; /**< Control plane URI */
}mpc_lowcomm_monitor_peer_info_t;

/**
 * @brief Get the number of peers in a given set
 *
 * @param set the set handle
 * @return uint64_t the number of peers
 */
uint64_t mpc_lowcomm_monitor_set_peer_count(mpc_lowcomm_monitor_set_t set);

/**
 * @brief Get the peers in a given set
 *
 * @param set the set handle
 * @param peers pointer to an array of @ref mpc_lowcomm_monitor_peer_info_t
 * @param peers_len the length of the target array
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_set_peers(mpc_lowcomm_monitor_set_t set, mpc_lowcomm_monitor_peer_info_t *peers, uint64_t peers_len);

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
int mpc_lowcomm_monitor_open_port(char *id, int id_len);

/**
 * @brief Close a previously openned port
 *
 * @param id the port to be freed
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_close_port(const char *id);

/**
 * @brief Parse port information
 *
 * @param id the id to be parsed
 * @param uid the peer UID contained in the port description
 * @param port the port ID contained in the port description
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_parse_port(const char *id, mpc_lowcomm_peer_uid_t *uid, int *port);


/************
* COMMANDS *
************/

/**
 * @brief These are the commands supported by the cplane
 *
 */
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

/**
 * @brief Convert a @ref mpc_lowcomm_monitor_command_t to a string representation
 *
 * @param cmd the command to convert
 * @return const char* it string representation
 */
const char *mpc_lowcomm_monitor_command_tostring(mpc_lowcomm_monitor_command_t cmd);

/**
 * @brief These are the extra parameters for the @ref mpc_lowcomm_monitor_naming command
 *
 */
typedef enum
{
	MPC_LOWCOMM_MONITOR_NAMING_PUT, /**< Set a name */
	MPC_LOWCOMM_MONITOR_NAMING_GET, /**< Get a name */
	MPC_LOWCOMM_MONITOR_NAMING_DEL, /**< Delete a name */
	MPC_LOWCOMM_MONITOR_NAMING_LIST /**< List names */
}mpc_lowcomm_monitor_command_naming_t;


#define MPC_LOWCOMM_MONITOR_KEY_LEN        128
#define MPC_LOWCOMM_SET_NAME_LEN           256
#define MPC_LOWCOMM_ONDEMAND_TARGET_LEN    32
#define MPC_LOWCOMM_ONDEMAND_DATA_LEN      MPC_LOWCOMM_PEER_URI_SIZE
#define MPC_LOWCOMM_MONITOR_MAX_CLIENTS    32

/**
 * @brief This is the actual payload sent for the various commands
 *
 */
typedef union
{
	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_command_get_peer_info
	 *
	 */
	struct
	{
		mpc_lowcomm_monitor_peer_info_t info;           /**< Requested peer info */
		mpc_lowcomm_peer_uid_t          requested_peer; /**< ID of the requested peer */
		mpc_lowcomm_monitor_retcode_t   retcode;        /**< Remote retcode */
	}peer;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_get_set_info
	 *
	 */
	struct
	{
		mpc_lowcomm_set_uid_t           uid;                            /**< Requested set id */
		mpc_lowcomm_monitor_retcode_t   retcode;                        /**< Remote retcode */
		char                            name[MPC_LOWCOMM_SET_NAME_LEN]; /**< Set name */
		uint64_t                        total_task_count;               /**< Set task count */
		uint64_t                        peer_count;                     /**< Set peer count */
		mpc_lowcomm_monitor_peer_info_t peers[];                        /**< Set peers */
	}set_info;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_ping
	 *
	 */
	struct
	{
		uint64_t                      ping;    /**< Timestamp on remote end */
		mpc_lowcomm_monitor_retcode_t retcode; /**< Remote retcode */
	}ping;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_ondemand
	 *
	 */
	struct
	{
		char                          target[MPC_LOWCOMM_ONDEMAND_TARGET_LEN]; /**< Target callback name */
		char                          data[MPC_LOWCOMM_ONDEMAND_DATA_LEN];     /**< Sent / Returned string data */
		mpc_lowcomm_monitor_retcode_t retcode;                                 /**< Remote retcode */
	}on_demand;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_connectivity
	 *
	 */
	struct
	{
		uint64_t               peers_count; /**< Remote connectivity peer count */
		mpc_lowcomm_peer_uid_t peers[0];    /**< Remote connectivity peer list */
	}connectivity;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_comm_info
	 *
	 */
	struct
	{
		uint64_t                      id;      /**< Identifier of the queried comm */
		int                           size;    /**< Size of the queried comm */
		mpc_lowcomm_monitor_retcode_t retcode; /**< Remote return code */
		mpc_lowcomm_peer_uid_t        uids[];  /**< List of UIDs participating in comm */
	}comm_info;

	/**
	 * @brief Payload for @ref mpc_lowcomm_monitor_naming
	 *
	 */
	struct
	{
		mpc_lowcomm_monitor_command_naming_t operation;                                  /**< Operation to be done */
		mpc_lowcomm_peer_uid_t               hosting_peer;                               /**< UID of the peer pushing a given key (only for GET) */
		char                                 name[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];      /** Name of the key */
		char                                 port_name[MPC_LOWCOMM_ONDEMAND_TARGET_LEN]; /** Value of the key (only for PUT/GET) */
		mpc_lowcomm_monitor_retcode_t        retcode;                                    /**< Remote retcode */
		char                                 name_list[0];                               /** List of key names (only for list) */
	}naming;
}mpc_lowcomm_monitor_args_t;

/**
 * @brief This is the opaque type for a response
 *
 */
typedef void *mpc_lowcomm_monitor_response_t;

/**
 * @brief Get the content from a response
 *
 * @param response the response to extract the content from
 * @return mpc_lowcomm_monitor_args_t* the internal content in the response
 */
mpc_lowcomm_monitor_args_t *mpc_lowcomm_monitor_response_get_content(mpc_lowcomm_monitor_response_t response);

/**
 * @brief Free a response
 *
 * @param response the response to be freed
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_response_free(mpc_lowcomm_monitor_response_t response);


/**********************
* EVENT LOOP SUPPORT *
**********************/

/**
 * @brief This defines an event loop callback
 *
 */
typedef void (*mpc_lowcomm_monitor_worker_callback_t)(void *ctx);

/**
 * @brief Push an event in the monitor loop for later asynchronous execution
 *
 * @param callback callback to be called later on
 * @param arg extra argument to pass to the callback
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_event_loop_push(mpc_lowcomm_monitor_worker_callback_t callback, void *arg);

/* ################ Interface for CPLANE QUERIES ##################### */

/****************
* GET SET INFO *
****************/

/**
 * @brief Get informations about a remote peer's set
 *
 * @param target_peer the target peer
 * @param ret return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_get_set_info(mpc_lowcomm_peer_uid_t target_peer,
                                                                mpc_lowcomm_monitor_retcode_t *ret);

/*****************
* GET PEER INFO *
*****************/

/**
 * @brief Get informations about a remote peer as seen by anoter peer
 *
 * @param dest the peer to query on
 * @param requested_peer the peer which information are requested
 * @param ret the return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_command_get_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                         mpc_lowcomm_peer_uid_t requested_peer,
                                                                         mpc_lowcomm_monitor_retcode_t *ret);

/***************
* PING REMOTE *
***************/

/**
 * @brief Ping a remote peer
 *
 * @param dest the peer UID to ping
 * @param ret return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ping(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

/***************
* COMM INFO    *
***************/

/**
 * @brief Get remote communicator information
 *
 * @param dest the UID to query to comm from
 * @param target_id the target communicator ID
 * @param ret the return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_comm_info(mpc_lowcomm_peer_uid_t dest,
                                                             uint64_t target_id,
                                                             mpc_lowcomm_monitor_retcode_t *ret);

/*********************
* ON DEMAND SUPPORT *
*********************/

/**
 * @brief This is the On-demand callback footprint
 *
 */
typedef int (*mpc_lowcomm_on_demand_callback_t)(mpc_lowcomm_peer_uid_t from, /** < Who called the RPC (UID) */
                                                char *data,                  /**< The incoming data */
                                                char *return_data,           /**< The returned data (to be mutated by the RPC) */
                                                int return_data_len,         /**< The return data lenght */
                                                void *ctx);                  /**< Context pointer set at @ref mpc_lowcomm_monitor_register_on_demand_callback */

/**
 * @brief Register a new on-demand callback
 *
 * @param target the name of the callback (should be unique)
 * @param callback the callback pointer
 * @param ctx extra argument to pass at each call
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_register_on_demand_callback(char *target,
                                                    mpc_lowcomm_on_demand_callback_t callback,
                                                    void *ctx);

/**
 * @brief Retrieve the on demand callback
 *
 * @param target the target on-demand name
 * @param ctx the corresponding ctx pointer (out)
 * @return mpc_lowcomm_on_demand_callback_t pointer to the callback
 */
mpc_lowcomm_on_demand_callback_t mpc_lowcomm_monitor_get_on_demand_callback(char *target, void **ctx);

/**
 * @brief Remove a given callback using its key
 *
 * @param target the name of the on-demand callback to remove
 * @return int 0 on success
 */
int mpc_lowcomm_monitor_unregister_on_demand_callback(char *target);

/**
 * @brief This sends an on-demand query calling a remote callback
 *
 * @param dest the destination UID
 * @param target the target on-demand callback
 * @param data data to be sent
 * @param ret the return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ondemand(mpc_lowcomm_peer_uid_t dest,
                                                            char *target,
                                                            char *data,
                                                            mpc_lowcomm_monitor_retcode_t *ret);


/*************************
* GET CONNECTIVITY INFO *
*************************/

/**
 * @brief This returns the peer connectivity of the control plane ass seen by a given UID
 *
 * @param dest UID to check the connectivity for
 * @param ret the return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_connectivity(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

/**
 * @brief This dumps in graphviz current set connectivity (is collective on the whole SET (MPC_COMM_WORLD))
 *
 */
void mpc_lowcomm_monitor_synchronous_connectivity_dump(void);

/******************
* NAMING COMMAND *
******************/

/**
 * @brief Perform naming operations
 *
 * @param dest destination UID to do naming operation on
 * @param operation the operation to be done see @ref mpc_lowcomm_monitor_command_naming_t
 * @param hosting_peer the hosting peer (only meaningful on PUT)
 * @param name the key name (not meaningfull on LIST)
 * @param port_name the key value (only meaninfull on PUT)
 * @param ret the return code
 * @return mpc_lowcomm_monitor_response_t the response
 */
mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_naming(mpc_lowcomm_peer_uid_t dest,
                                                          mpc_lowcomm_monitor_command_naming_t operation,
                                                          mpc_lowcomm_peer_uid_t hosting_peer,
                                                          const char *name,
                                                          const char *port_name,
                                                          mpc_lowcomm_monitor_retcode_t *ret);

#endif /* MPC_LOWCOMM_MONITOR_H_ */
