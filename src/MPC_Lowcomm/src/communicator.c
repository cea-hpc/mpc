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
#include "communicator.h"
#include "group.h"

#include "coll.h"

#include <mpc_config.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>

#include <mpc_common_datastructure.h>
#include <lowcomm_config.h>
#include "lowcomm.h"
#include <mpc_lowcomm.h>

#ifdef MPC_LOWCOMM_PROTOCOL
#include <lcp.h>
#endif

#ifdef MPC_Threads
#include <mpc_thread.h>
#endif

/*************************
* COMMUNICATOR PRINTING *
*************************/
void mpc_lowcomm_communicator_print(mpc_lowcomm_communicator_t comm, int root_only)
{
	if(!comm)
	{
		mpc_common_debug_error("COMM Is NULL");
		return;
	}

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	int is_lead      = 1;
	int is_intercomm = mpc_lowcomm_communicator_is_intercomm(comm);

	/* In the low level layers of MPC the rank might not be availaible
	 * in this case we print for all */
	if(0 <= mpc_lowcomm_get_rank() )
	{
		is_lead = (mpc_lowcomm_communicator_rank(comm) == 0);

		if(is_intercomm)
		{
			is_lead &= mpc_lowcomm_communicator_in_master_group(comm);
		}
	}

	if(!is_lead && root_only)
	{
		return;
	}

	mpc_common_debug_error("========COMM %lu @ %p=======", comm->id, comm);

	mpc_common_debug_error("TYPE: %s", (comm->group || comm->is_comm_self)?"intracomm":"intercomm");
	mpc_common_debug_error("IS SELF: %s", (comm->is_comm_self)?"yes":"no");
	mpc_common_debug_error("GID: %lu", mpc_lowcomm_get_comm_gid(comm->id) );

	if(is_intercomm)
	{
		mpc_common_debug_error("LEFT");
		mpc_lowcomm_communicator_print(comm->left_comm, 0);

		mpc_common_debug_error("RIGHT");
		mpc_lowcomm_communicator_print(comm->right_comm, 0);
	}
	else
	{
		int size = mpc_lowcomm_communicator_size(comm);
		mpc_common_debug_error("SIZE: %u", size);

		mpc_common_debug_error("--- RANK LIST ---");

		int i;
		for(i = 0 ; i < size; i++)
		{
			mpc_common_debug_error("COMM %d is CW %d UID %lu",
			                       i,
			                       mpc_lowcomm_communicator_world_rank_of(comm, i),
			                       mpc_lowcomm_communicator_uid(comm, i) );
		}
		mpc_common_debug_error("-----------------");
	}


	mpc_common_debug_error("==========================");
}

/****************************
* COMMUNICATOR REFCOUNTING *
****************************/

void _mpc_lowcomm_communicator_acquire(mpc_lowcomm_internal_communicator_t *comm)
{
	assume(comm != NULL);
	OPA_incr_int(&comm->refcount);
}

int _mpc_lowcomm_communicator_relax(mpc_lowcomm_internal_communicator_t *comm)
{
	assume(comm != NULL);
	return OPA_fetch_and_decr_int(&comm->refcount);
}

/*****************
* UNIVERSE COMM *
*****************/


static inline mpc_lowcomm_internal_communicator_t *__init_communicator_with_id(mpc_lowcomm_communicator_id_t comm_id,
                                                                               mpc_lowcomm_group_t *group,
                                                                               int is_comm_self,
                                                                               mpc_lowcomm_communicator_t left_comm,
                                                                               mpc_lowcomm_communicator_t right_comm,
                                                                               int forced_linear_id);

int mpc_lowcomm_communicator_build_remote(mpc_lowcomm_peer_uid_t remote,
                                          const mpc_lowcomm_communicator_id_t id,
                                          mpc_lowcomm_communicator_t *outcomm)
{
	/* First make sure the comm targets an existing set different than ourselves */
	mpc_lowcomm_set_uid_t gid = mpc_lowcomm_get_comm_gid(id);

	mpc_lowcomm_monitor_set_t set = mpc_lowcomm_monitor_set_get(gid);

	if(set == NULL)
	{
		mpc_common_debug_error("Could not resolve remote set %llu", gid);
		return MPC_LOWCOMM_ERROR;
	}

	if(gid == mpc_lowcomm_monitor_get_gid() )
	{
		mpc_common_debug_error("Cannot resolve local set as a remote one");
		return MPC_LOWCOMM_ERROR;
	}

	/* Now we need set info from the remote
	 * not for now it only resolves world, we need a call to target specific comm */

	mpc_lowcomm_monitor_retcode_t cret;

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_comm_info(remote,
	                                                                    id,
	                                                                    &cret);

	if(cret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		return -1;
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	if(content->comm_info.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		return -1;
	}

	uint64_t peer_count = content->comm_info.size;

	/* Now create the group description */
	_mpc_lowcomm_group_rank_descriptor_t *remote_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * peer_count);

	assume(remote_desc != NULL);

	uint64_t i;

	for(i = 0; i < peer_count; i++)
	{
		remote_desc[i].comm_world_rank  = content->comm_info.rds[i].comm_world_rank;
		remote_desc[i].host_process_uid = content->comm_info.rds[i].uid;
	}

	mpc_lowcomm_monitor_response_free(resp);

	mpc_lowcomm_group_t *remote_group        = _mpc_lowcomm_group_create(peer_count, remote_desc, 1);
	mpc_lowcomm_internal_communicator_t *ret = __init_communicator_with_id(id,
	                                                                       remote_group,
	                                                                       0,
	                                                                       MPC_COMM_NULL,
	                                                                       MPC_COMM_NULL,
	                                                                       0);

	mpc_lowcomm_group_free(&remote_group);

	if(!ret)
	{
		mpc_common_debug_error("Failed to build remote comm");
		return MPC_LOWCOMM_ERROR;
	}

	*outcomm = __mpc_lowcomm_communicator_to_predefined(ret);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_communicator_build_remote_world(const mpc_lowcomm_set_uid_t gid, mpc_lowcomm_communicator_t *comm)
{
	return mpc_lowcomm_communicator_build_remote(mpc_lowcomm_monitor_uid_of(gid, 0),
	                                             mpc_lowcomm_get_comm_world_id_gid(gid),
	                                             comm);
}

/***********************************
* COMMUNICATOR CONNECT AND ACCEPT *
***********************************/

struct _mpc_lowcomm_connect_desc_s
{
	mpc_lowcomm_peer_uid_t        uid;
	mpc_lowcomm_communicator_id_t comm_id;
	mpc_lowcomm_communicator_id_t new_comm_id;
};


static inline int __broadcast_new_comm(mpc_lowcomm_communicator_t local_comm,
                                       mpc_lowcomm_communicator_t tobcast,
                                       int root,
                                       mpc_lowcomm_communicator_t *newcomm)
{
	assume(local_comm != MPC_COMM_NULL);
	local_comm = __mpc_lowcomm_communicator_from_predefined(local_comm);

	int my_rank = mpc_lowcomm_get_comm_rank(local_comm);

	if(root == my_rank)
	{
		assume(tobcast != MPC_COMM_NULL);
		*newcomm = tobcast;

		tobcast = __mpc_lowcomm_communicator_from_predefined(tobcast);

		mpc_lowcomm_bcast(&tobcast->id,
		                  sizeof(mpc_lowcomm_communicator_id_t),
		                  root,
		                  local_comm);

		mpc_lowcomm_bcast(&tobcast->group->size,
		                  sizeof(unsigned int),
		                  root,
		                  local_comm);

		mpc_lowcomm_bcast(tobcast->group->ranks,
		                  tobcast->group->size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
		                  root,
		                  local_comm);
	}
	else
	{
		mpc_lowcomm_communicator_id_t comm_id = 0;
		mpc_lowcomm_bcast(&comm_id,
		                  sizeof(mpc_lowcomm_communicator_id_t),
		                  root,
		                  local_comm);

		unsigned int group_size;
		mpc_lowcomm_bcast(&group_size, sizeof(unsigned int), root, local_comm);

		_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(group_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(descs != NULL);

		mpc_lowcomm_bcast(descs, group_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t), root, local_comm);

		/* Now we create a group from descriptors */
		mpc_lowcomm_group_t *comm_group = _mpc_lowcomm_group_create(group_size, descs, 1);

		static mpc_common_spinlock_t __bcast_creation_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

		mpc_common_spinlock_lock(&__bcast_creation_lock);

		*newcomm = mpc_lowcomm_get_communicator_from_id(comm_id);

		if(*newcomm == NULL)
		{
			/* And eventually our comm of interest */
			*newcomm = __init_communicator_with_id(comm_id,
			                                       comm_group,
			                                       0,
			                                       MPC_COMM_NULL,
			                                       MPC_COMM_NULL,
			                                       0);
		}
		else
		{
			_mpc_lowcomm_communicator_acquire(*newcomm);
		}


		mpc_common_spinlock_unlock(&__bcast_creation_lock);


		mpc_lowcomm_group_free(&comm_group);
	}

	*newcomm = __mpc_lowcomm_communicator_to_predefined(*newcomm);

	return MPC_LOWCOMM_SUCCESS;
}

static inline mpc_lowcomm_communicator_id_t  __communicator_id_new(void);

static inline mpc_lowcomm_communicator_id_t __send_connect_infos(mpc_lowcomm_peer_uid_t dest,
                                                                 int port_tag,
                                                                 mpc_lowcomm_communicator_t local_comm,
                                                                 mpc_lowcomm_communicator_t remote_world,
                                                                 int gen_new_comm_id)
{
	struct _mpc_lowcomm_connect_desc_s desc;

	desc.uid     = mpc_lowcomm_monitor_get_uid();
	local_comm   = __mpc_lowcomm_communicator_from_predefined(local_comm);
	desc.comm_id = mpc_lowcomm_communicator_id(local_comm);

	if(gen_new_comm_id)
	{
		desc.new_comm_id = __communicator_id_new();
	}
	else
	{
		desc.new_comm_id = MPC_LOWCOMM_COMM_NULL_ID;
	}

	mpc_lowcomm_request_t req;

	remote_world = __mpc_lowcomm_communicator_from_predefined(remote_world);
	mpc_lowcomm_universe_isend(dest,
	                           &desc,
	                           sizeof(struct _mpc_lowcomm_connect_desc_s),
	                           port_tag,
	                           remote_world,
	                           &req);

	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	return desc.new_comm_id;
}

static inline void __recv_connect_infos(struct _mpc_lowcomm_connect_desc_s *remote_desc,
                                        mpc_lowcomm_peer_uid_t from,
                                        int port_tag)
{
	mpc_lowcomm_request_t req;

	/* Now Post a RECV on universe for incomming connections
	 * this recv will contain the remote root ID and communicator ID as the rank in peer comm */
	mpc_lowcomm_universe_irecv(from,
	                           remote_desc,
	                           sizeof(struct _mpc_lowcomm_connect_desc_s),
	                           port_tag,
	                           &req);

	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);
}

static inline mpc_lowcomm_communicator_t __new_communicator(mpc_lowcomm_communicator_t comm,
                                                            mpc_lowcomm_group_t *group,
                                                            mpc_lowcomm_communicator_t left_comm,
                                                            mpc_lowcomm_communicator_t right_comm,
                                                            int is_comm_self,
                                                            int check_if_current_rank_belongs,
                                                            mpc_lowcomm_communicator_id_t forced_id);


static inline int __comm_connect_accept(const char *port_name,
                                        int root,
                                        mpc_lowcomm_communicator_t comm,
                                        mpc_lowcomm_communicator_t *new_comm,
                                        int is_accept)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	/* Start with a dup */
	mpc_lowcomm_communicator_t orig_comm = comm;

	comm = mpc_lowcomm_communicator_dup(comm);

	/* Only meaningfull at root */
	mpc_lowcomm_communicator_t remote_world = MPC_COMM_NULL;
	/* Only meaningfull at root */
	struct _mpc_lowcomm_connect_desc_s remote_desc;


	int rank = mpc_lowcomm_get_comm_rank(comm);

	int retained_root = root;

	mpc_lowcomm_peer_uid_t listening_peer;
	int port_tag = 0;
	mpc_lowcomm_communicator_id_t new_comm_id = MPC_LOWCOMM_COMM_NULL_ID;

	if(rank == root)
	{
		/* First decode the port */
		if(mpc_lowcomm_monitor_parse_port(port_name, &listening_peer, &port_tag) < 0)
		{
			return MPC_LOWCOMM_ERROR;
		}

		int listening_peer_rank = mpc_lowcomm_peer_get_rank(listening_peer);

		if(is_accept)
		{
			/* Now we make sure the listening peer is part of our local comm */
			if(!mpc_lowcomm_communicator_contains(comm, listening_peer) )
			{
				mpc_common_debug_warning("This port is not part of our comm");
				return MPC_LOWCOMM_ERROR;
			}

			retained_root = listening_peer_rank;
		}
	}

	/* Bcast the tag to local comm */
	mpc_lowcomm_bcast(&port_tag, sizeof(int), retained_root, comm);

	mpc_lowcomm_communicator_t remote_comm = MPC_COMM_NULL;


	if(rank == retained_root)
	{
		if(is_accept)
		{
			/* Receive remote connection infos */
			__recv_connect_infos(&remote_desc, MPC_ANY_SOURCE, port_tag);

			/* Connect generates the comm_id */
			new_comm_id = remote_desc.new_comm_id;

			mpc_lowcomm_communicator_build_remote_world(mpc_lowcomm_peer_get_set(remote_desc.uid),
			                                            &remote_world);


			/* Send my connection infos */
			__send_connect_infos(remote_desc.uid, port_tag, comm, remote_world, 0);
		}
		else
		{
			mpc_lowcomm_communicator_build_remote_world(mpc_lowcomm_peer_get_set(listening_peer),
			                                            &remote_world);

			/* Send my connection infos
			 * note that it is in this function that we generate comm id */
			new_comm_id = __send_connect_infos(listening_peer,
			                                   port_tag,
			                                   comm,
			                                   remote_world,
			                                   1);

			/* Receive remote connection infos */
			__recv_connect_infos(&remote_desc, MPC_ANY_SOURCE, port_tag);
		}

		mpc_common_debug_error("REMOTE is %lu", remote_desc.comm_id);

		static mpc_common_spinlock_t __creation_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

		mpc_common_spinlock_lock(&__creation_lock);


		remote_comm = mpc_lowcomm_get_communicator_from_id(remote_desc.comm_id);

		if(!remote_comm)
		{
			if(mpc_lowcomm_communicator_build_remote(remote_desc.uid,
			                                         remote_desc.comm_id,
			                                         &remote_comm) )
			{
				mpc_common_debug_warning("Failed to retrieve remote comm info");
				return MPC_LOWCOMM_ERROR;
			}
		}
		else
		{
			_mpc_lowcomm_communicator_acquire(remote_comm);
		}

		mpc_common_spinlock_unlock(&__creation_lock);

		assume(remote_comm != MPC_COMM_NULL);
	}

	mpc_lowcomm_communicator_t _bcast_remote_comm = MPC_COMM_NULL;

	__broadcast_new_comm(comm,
	                     remote_comm,
	                     retained_root,
	                     &_bcast_remote_comm);

	assume(_bcast_remote_comm != MPC_COMM_NULL);

	/* At this point we have both the local and the remote comm
	 * available everywhere */

	mpc_lowcomm_bcast(&new_comm_id, sizeof(mpc_lowcomm_communicator_id_t), retained_root, comm);

	*new_comm = __new_communicator(orig_comm,
	                               NULL,
	                               comm,
	                               _bcast_remote_comm,
	                               0,
	                               0,
	                               new_comm_id);


	mpc_lowcomm_communicator_print(*new_comm, 0);


	/* Now Manual Barrier to ensure comm is known everywhere */
	mpc_lowcomm_barrier(comm);

	if(rank == retained_root)
	{
		mpc_lowcomm_request_t sync_reqs[2];
		int dummy;

		mpc_lowcomm_peer_uid_t sync_peer = -1;

		if(is_accept)
		{
			sync_peer = remote_desc.uid;
		}
		else
		{
			sync_peer = listening_peer;
		}

		mpc_lowcomm_universe_isend(sync_peer,
		                           &rank,
		                           sizeof(int),
		                           port_tag,
		                           remote_world,
		                           &sync_reqs[0]);
		mpc_lowcomm_universe_irecv(sync_peer,
		                           &dummy,
		                           sizeof(int),
		                           port_tag,
		                           &sync_reqs[1]);


		mpc_lowcomm_waitall(2, sync_reqs, MPC_LOWCOMM_STATUS_NULL);

		/* We do not need the remote world anymore */
		mpc_lowcomm_communicator_free(&remote_world);
	}

	mpc_lowcomm_barrier(comm);
	*new_comm = __mpc_lowcomm_communicator_to_predefined(*new_comm);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_communicator_connect(const char *port_name,
                                     int root,
                                     mpc_lowcomm_communicator_t comm,
                                     mpc_lowcomm_communicator_t *new_comm)
{
	return __comm_connect_accept(port_name, root, comm, new_comm, 0);
}

int mpc_lowcomm_communicator_accept(const char *port_name,
                                    int root,
                                    mpc_lowcomm_communicator_t comm,
                                    mpc_lowcomm_communicator_t *new_comm)
{
	return __comm_connect_accept(port_name, root, comm, new_comm, 1);
}

/********************
* CONTEXT POINTERS *
********************/

int mpc_lowcomm_communicator_set_context_pointer(mpc_lowcomm_communicator_t comm, mpc_lowcomm_handle_ctx_t ctxptr)
{
	if(!comm)
	{
		return 1;
	}
	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	comm->extra_ctx_ptr = ctxptr;

	return 0;
}

mpc_lowcomm_handle_ctx_t mpc_lowcomm_communicator_get_context_pointer(mpc_lowcomm_communicator_t comm)
{
	if(!comm)
	{
		return MPC_LOWCOMM_HANDLE_CTX_NULL;
	}
	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	return comm->extra_ctx_ptr;
}

/*********************************
* COLL GETTERS FOR COMMUNICATOR *
*********************************/


int mpc_lowcomm_communicator_is_shared_mem(const mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);

	return local_comm->shm_coll != NULL;
}

struct sctk_comm_coll *mpc_communicator_shm_coll_get(const mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);

	return local_comm->shm_coll;
}

int mpc_lowcomm_communicator_is_shared_node(const mpc_lowcomm_communicator_t comm)
{
	UNUSED(comm);
	return 0;
}

int mpc_lowcomm_communicator_attributes(const mpc_lowcomm_communicator_t comm,
                                        int *is_intercomm,
                                        int *is_shm,
                                        int *is_shared_node)
{
	mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);

	*is_intercomm   = mpc_lowcomm_communicator_is_intercomm(local_comm);
	*is_shm         = mpc_lowcomm_communicator_is_shared_mem(local_comm);
	*is_shared_node = mpc_lowcomm_communicator_is_shared_node(local_comm);

	return MPC_LOWCOMM_SUCCESS;
}

/**
 * @brief Find the index of the topological communicators associated with the root parameter.
 *
 * @param comm Target communicator.
 * @param root Target root.
 *
 * @return The index of the topological communicators associated with the root parameter or -1.
 */
static inline int ___topo_find_comm_index(mpc_lowcomm_communicator_t comm, int root)
{
	int task_rank = mpc_common_get_task_rank();
	int i;

	mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);

	for(i = 0; i < comm->topo_comms[task_rank].size; i++)
	{
		if(local_comm->topo_comms[task_rank].roots[i] == root)
		{
			return i;
		}
	}

	return -1;
}

mpc_hardware_split_info_t * mpc_lowcomm_topo_comm_get(mpc_lowcomm_communicator_t comm, int root)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	int task_rank = mpc_common_get_task_rank();
	int index     = ___topo_find_comm_index(comm, root);

	if(index != -1)
	{
#ifdef MPC_ENABLE_DEBUG_MESSAGES
		mpc_common_debug_log("GET | TASK %d | ROOT %d -> INDEX %d | ADR %p", task_rank, root, index, comm->topo_comms[task_rank].hw_infos[index]);
#endif
		return comm->topo_comms[task_rank].hw_infos[index];
	}

#ifdef MPC_ENABLE_DEBUG_MESSAGES
	mpc_common_debug_log("GET | TASK %d | ROOT %d -> NULL | ADR %p", task_rank, root, NULL);
#endif
	return NULL;
}

void mpc_lowcomm_topo_comm_set(mpc_lowcomm_communicator_t comm, int root, mpc_hardware_split_info_t *hw_info)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	int task_rank = mpc_common_get_task_rank();
	int index     = ___topo_find_comm_index(comm, root);

	if(index != -1)
	{
#ifdef MPC_ENABLE_DEBUG_MESSAGES
		mpc_common_debug_log("SET | TASK %d | UPDATE | ROOT %d -> INDEX %d | ADR %p", task_rank, root, index, hw_info);
#endif
		comm->topo_comms[task_rank].hw_infos[index] = hw_info;
		return;
	}

	index = ___topo_find_comm_index(comm, -1);
	if(index == -1)
	{
		comm->topo_comms[task_rank].roots    = sctk_realloc(comm->topo_comms[task_rank].roots, sizeof(int) * comm->topo_comms[task_rank].size * 2);
		comm->topo_comms[task_rank].hw_infos = sctk_realloc(comm->topo_comms[task_rank].hw_infos, sizeof(mpc_hardware_split_info_t *) * comm->topo_comms[task_rank].size * 2);
		int i;
		for( i = comm->topo_comms[task_rank].size; i < comm->topo_comms[task_rank].size * 2; i++)
		{
			comm->topo_comms[task_rank].roots[i] = -1;
		}

		index = comm->topo_comms[task_rank].size;
		comm->topo_comms[task_rank].size *= 2;
	}

#ifdef MPC_ENABLE_DEBUG_MESSAGES
	mpc_common_debug_log("SET | TASK %d | ROOT %d -> INDEX %d | ADR %p", task_rank, root, index, hw_info);
#endif
	comm->topo_comms[task_rank].roots[index]    = root;
	comm->topo_comms[task_rank].hw_infos[index] = hw_info;
}

static inline void ___init_topo_comm(mpc_lowcomm_internal_communicator_t *comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	int task_count = mpc_common_get_task_count();

	comm->topo_comms = sctk_malloc(sizeof(mpc_lowcomm_topo_comms) * task_count);
	int i;

	for(i = 0; i < task_count; i++)
	{
		comm->topo_comms[i].size     = MPC_INITIAL_TOPO_COMMS_SIZE;
		comm->topo_comms[i].roots    = sctk_malloc(sizeof(int) * MPC_INITIAL_TOPO_COMMS_SIZE);
		comm->topo_comms[i].hw_infos = sctk_malloc(sizeof(mpc_hardware_split_info_t *) * MPC_INITIAL_TOPO_COMMS_SIZE);
		int j;
		for(j = 0; j < MPC_INITIAL_TOPO_COMMS_SIZE; j++)
		{
			comm->topo_comms[i].roots[j] = -1;
		}
	}
}

//needed function prototype
static inline void __comm_free(mpc_lowcomm_communicator_t comm);

static inline void ___free_hardware_info(mpc_hardware_split_info_t *hw_info)
{
	int i;

	for(i = 0; i < hw_info->deepest_hardware_level; i++)
	{
		__comm_free(hw_info->hwcomm[i + 1]);
		__comm_free(hw_info->rootcomm[i]);

		if(hw_info->childs_data_count != NULL)
		{
			sctk_free(hw_info->childs_data_count[i]);
		}
	}

	sctk_free(hw_info->hwcomm);
	sctk_free(hw_info->rootcomm);
	sctk_free(hw_info->childs_data_count);
	sctk_free(hw_info->send_data_count);
	sctk_free(hw_info->swap_array);

	sctk_free(hw_info);
}

static inline void ___free_topo_comm(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	int task_count = mpc_common_get_task_count();
	int i;

	for(i = 0; i < task_count; i++)
	{
		int j = 0;
		while(comm->topo_comms[i].roots[j] != -1 && j < comm->topo_comms[i].size)
		{
			___free_hardware_info(comm->topo_comms[i].hw_infos[j]);
			j++;
		}

		sctk_free(comm->topo_comms[i].roots);
		sctk_free(comm->topo_comms[i].hw_infos);
	}

	sctk_free(comm->topo_comms);
}

/***************************
* BASIC COMMUNICATOR INIT *
***************************/

static inline void __communicator_id_register(mpc_lowcomm_communicator_t comm, int forced_linear_id);

static inline mpc_lowcomm_internal_communicator_t *__init_communicator_with_id(mpc_lowcomm_communicator_id_t comm_id,
                                                                               mpc_lowcomm_group_t *group,
                                                                               int is_comm_self,
                                                                               mpc_lowcomm_communicator_t left_comm,
                                                                               mpc_lowcomm_communicator_t right_comm,
                                                                               int forced_linear_id)
{
	mpc_lowcomm_internal_communicator_t *ret = sctk_malloc(sizeof(mpc_lowcomm_internal_communicator_t) );

	assume(ret != NULL);

	ret->id             = comm_id;
	ret->group          = group;
	ret->process_span   = 1;
	ret->shm_coll       = NULL;
	ret->linear_comm_id = -1;
	ret->extra_ctx_ptr  = MPC_LOWCOMM_HANDLE_CTX_NULL;

	left_comm  = __mpc_lowcomm_communicator_from_predefined(left_comm);
	right_comm = __mpc_lowcomm_communicator_from_predefined(right_comm);

	/* Intercomm ctx */
	ret->is_comm_self = is_comm_self;

	/* Intercommm ctx we acquire a ref to inner comms */

	ret->left_comm = left_comm;

	if(ret->left_comm != MPC_COMM_NULL)
	{
		_mpc_lowcomm_communicator_acquire(ret->left_comm);
	}

	ret->right_comm = right_comm;

	if(ret->right_comm != MPC_COMM_NULL)
	{
		_mpc_lowcomm_communicator_acquire(ret->right_comm);
	}

	/* Topo comms */
	___init_topo_comm(ret);


	if(comm_id == MPC_LOWCOMM_COMM_SELF_ID)
	{
		/* Comm self is practically
		 * a virtual communicator without internal
		 * group to handle as it is different
		 * for each MPI Process */
		assume(group == NULL);
		ret->is_comm_self = 1;
	}
	else if(group != NULL)
	{
		/* This is an intracomm */
		assume(group != NULL);

		/* Increase group refcount by 1 as the comm
		 * now holds a reference to it */
		_mpc_lowcomm_group_acquire(group);

		ret->process_span = _mpc_lowcomm_group_process_count(group);

		/* SET SHM Collective context if needed */
		if(ret->process_span == 1 && (mpc_lowcomm_group_size(group) > 1) )
		{
			ret->shm_coll = sctk_comm_coll_init(group->size);
		}
	}

	/* The comm is initialized with one reference to it */
	OPA_store_int(&ret->refcount, 1);
	OPA_store_int(&ret->free_count, 0);
	__communicator_id_register(ret, forced_linear_id);

	/* Initialize coll for this comm */
	mpc_lowcomm_coll_init_hook(ret);

	ret->left_comm  = __mpc_lowcomm_communicator_to_predefined(ret->left_comm);
	ret->right_comm = __mpc_lowcomm_communicator_to_predefined(ret->right_comm);

	return ret;
}

/**************************
* COMMUNICATOR NUMBERING *
**************************/
static inline int __is_comm_global_lead(mpc_lowcomm_communicator_t comm, int lead_rank)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	assert(lead_rank < mpc_lowcomm_communicator_size(comm) );
	int my_rank = mpc_lowcomm_communicator_rank(comm);

	return my_rank == lead_rank;
}

static inline int __get_comm_world_lead(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	int group_local_lead = mpc_lowcomm_group_get_local_leader(comm->group);

	return mpc_lowcomm_communicator_world_rank_of(comm, group_local_lead);
}

struct __communicator_id_factory
{
	unsigned int                first_local;
	unsigned int                local_count;
	unsigned int                local_used;
	mpc_common_spinlock_t       lock;
	struct mpc_common_hashtable id_table;
	struct mpc_common_hashtable int_id_table;
	struct mpc_common_hashtable comm_table;
	struct mpc_common_bit_array comm_bit_array;
};

struct __communicator_id_factory __id_factory = { 0 };

#define COMM_ID_TO_SKIP    5

static inline void __communicator_id_factory_init(void)
{
	int process_count = mpc_common_get_process_count();

	/* We have the whole range except two */
	uint32_t global_dynamic = ( ( (uint32_t)-1) - COMM_ID_TO_SKIP);

	/* Ensure it can be divided evenly */
	if(global_dynamic % process_count)
	{
		global_dynamic -= (global_dynamic % process_count);
	}

	__id_factory.local_count = global_dynamic / process_count;

	int process_rank = mpc_common_get_process_rank();

	__id_factory.first_local = COMM_ID_TO_SKIP + process_rank * __id_factory.local_count;
	__id_factory.local_used  = 0;

	mpc_common_spinlock_init(&__id_factory.lock, 0);

	mpc_common_hashtable_init(&__id_factory.id_table, 4096);
	mpc_common_hashtable_init(&__id_factory.int_id_table, 4096);
	mpc_common_hashtable_init(&__id_factory.comm_table, 4096);
	mpc_common_bit_array_init(&__id_factory.comm_bit_array, 8192);
}

static inline void __communicator_id_factory_release(void)
{
	mpc_common_hashtable_release(&__id_factory.id_table);
	mpc_common_hashtable_release(&__id_factory.int_id_table);
	mpc_common_hashtable_release(&__id_factory.comm_table);
	mpc_common_bit_array_release(&__id_factory.comm_bit_array);
}

static inline void __communicator_id_register(mpc_lowcomm_communicator_t comm, int forced_linear_id)
{
	static uint32_t __linear_comm_id = 1000;
	static mpc_common_spinlock_t __linear_comm_id_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(0 < forced_linear_id)
	{
		comm->linear_comm_id = forced_linear_id;
	}
	else
	{
		mpc_common_spinlock_lock(&__linear_comm_id_lock);
		/* Acquire and save a linear comm id */
		comm->linear_comm_id = __linear_comm_id;
		__linear_comm_id++;
		mpc_common_spinlock_unlock(&__linear_comm_id_lock);
	}

	uint64_t key = comm->id;

	assume(0 < comm->linear_comm_id);
	uint64_t linear_key = comm->linear_comm_id;

	mpc_common_debug_info("COMM: register key=%llu, int key=%d, linear key=%llu", key, key, linear_key);


	/* It is forbidden to add an existing comm */
	assume(mpc_common_hashtable_get(&__id_factory.id_table, key) == NULL);
	mpc_common_hashtable_set(&__id_factory.int_id_table, linear_key, (void *)comm);
	mpc_common_hashtable_set(&__id_factory.id_table, key, (void *)comm);
	mpc_common_hashtable_set(&__id_factory.comm_table, (uint64_t)comm, (void *)comm);
	mpc_common_bit_array_set(&__id_factory.comm_bit_array, (uint64_t)comm, 1);
}

static inline void __communicator_id_release(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	uint64_t key        = comm->id;
	uint64_t linear_key = comm->linear_comm_id;

	mpc_common_debug_info("COMM: release key=%llu, int key=%d, linear key=%llu", key, key, linear_key);

	mpc_common_hashtable_delete(&__id_factory.id_table, key);
	mpc_common_hashtable_delete(&__id_factory.int_id_table, linear_key);
	mpc_common_hashtable_delete(&__id_factory.comm_table, (uint64_t)comm);
	mpc_common_bit_array_set(&__id_factory.comm_bit_array, (uint64_t)comm, 0);
}

mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_linear_id(int linear_id)
{
	mpc_lowcomm_communicator_t ret = NULL;

	assume(0 <= linear_id);
	uint64_t key = linear_id;

	ret = (mpc_lowcomm_communicator_t)mpc_common_hashtable_get(&__id_factory.int_id_table, key);
	return ret;
}

mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_id(mpc_lowcomm_communicator_id_t id)
{
	mpc_lowcomm_communicator_t ret = MPC_COMM_NULL;

	ret = (mpc_lowcomm_communicator_t)mpc_common_hashtable_get(&__id_factory.id_table, id);
	return ret;
}

int mpc_lowcomm_communicator_scan(void (*callback)(mpc_lowcomm_communicator_t comm, void *arg), void *arg)
{
	if(!callback)
	{
		return MPC_LOWCOMM_ERROR;
	}

	void *tmp = NULL;

	MPC_HT_ITER(&__id_factory.id_table, tmp)
	{
		mpc_lowcomm_communicator_t pcomm = (mpc_lowcomm_communicator_t)tmp;

		(callback)(tmp, arg);
	}
	MPC_HT_ITER_END(&__id_factory.id_table)

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_communicator_exists(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	/* In debug we actually check */
	return mpc_common_hashtable_get(&__id_factory.comm_table, (uint64_t)comm) != NULL;
}

static inline mpc_lowcomm_communicator_id_t  __communicator_id_new(void)
{
	mpc_lowcomm_communicator_id_t ret = 0;

	mpc_common_spinlock_lock_yield(&__id_factory.lock);

	if(__id_factory.local_used == __id_factory.local_count)
	{
		mpc_common_debug_fatal("You exhausted communicator id dynamic (up to %d) communicators on ranl %d", __id_factory.local_count, mpc_common_get_task_rank() );
	}

	ret = __id_factory.first_local + __id_factory.local_used;
	__id_factory.local_used++;

	uint64_t gid = mpc_lowcomm_monitor_get_gid();

	mpc_common_nodebug("BEFORE %lu", ret);
	ret |= gid << 32;
	mpc_common_nodebug("AFTER %lu", ret);


	mpc_common_spinlock_unlock(&__id_factory.lock);

	return ret;
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_gen_local_id(void)
{
	return __communicator_id_new();
}

static inline mpc_lowcomm_communicator_id_t __communicator_id_new_for_comm_intercomm(mpc_lowcomm_communicator_t intercomm)
{
	intercomm = __mpc_lowcomm_communicator_from_predefined(intercomm);
	mpc_lowcomm_communicator_id_t new_id = 0;

	int rank = mpc_lowcomm_communicator_rank(intercomm);

	if(rank == 0)
	{
		/* Leaders pre exchange the ID */
		if(mpc_lowcomm_communicator_in_master_group(intercomm) )
		{
			/* In the master group we set the id */
			new_id = __communicator_id_new();
			mpc_lowcomm_send(0, &new_id, sizeof(mpc_lowcomm_communicator_id_t), 123, intercomm);
		}
		else
		{
			mpc_lowcomm_recv(0, &new_id, sizeof(mpc_lowcomm_communicator_id_t), 123, intercomm);
		}
		assume(new_id != 0);
	}

	/* Then all do bcast local */
	mpc_lowcomm_communicator_t local_comm = mpc_lowcomm_communicator_get_local(intercomm);

	mpc_lowcomm_bcast(&new_id, sizeof(mpc_lowcomm_communicator_id_t), 0, local_comm);

	return new_id;
}

static inline mpc_lowcomm_communicator_id_t  __communicator_id_new_for_comm(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		return __communicator_id_new_for_comm_intercomm(comm);
	}

	/* Here we pick a random leader to spread
	 * communicator numbering pressure */
	int lead = comm->id % mpc_lowcomm_communicator_size(comm);

	mpc_lowcomm_communicator_id_t comm_id = 0;

	if(__is_comm_global_lead(comm, lead) )
	{
		comm_id = __communicator_id_new();
	}

	mpc_lowcomm_bcast( (void *)&comm_id, sizeof(mpc_lowcomm_communicator_id_t), lead, comm);

	assume(comm_id != 0);

	return comm_id;
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_id(mpc_lowcomm_communicator_t comm)
{
	if(comm != MPC_COMM_NULL)
	{
		comm = __mpc_lowcomm_communicator_from_predefined(comm);
		return comm->id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}

int mpc_lowcomm_communicator_linear_id(mpc_lowcomm_communicator_t comm)
{
	if(comm != MPC_COMM_NULL)
	{
		comm = __mpc_lowcomm_communicator_from_predefined(comm);
		return comm->linear_comm_id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}

/*****************************
* COMMUNICATOR CONSTRUCTORS *
*****************************/

static inline void __comm_free(mpc_lowcomm_communicator_t comm)
{
	static mpc_common_spinlock_t free_lock = { 0 };

	int exists = 0;

	mpc_common_spinlock_lock(&free_lock);
	comm   = __mpc_lowcomm_communicator_from_predefined(comm);
	exists = mpc_lowcomm_communicator_exists(comm);


	if(exists)
	{
		/* Release the ID */
		__communicator_id_release(comm);
	}

	mpc_common_spinlock_unlock(&free_lock);

	if(!exists)
	{
		return;
	}

	/* Clear the SHM data */
	if(comm->shm_coll)
	{
		sctk_comm_coll_release(comm->shm_coll);
	}

	/* Relax the groups */
	mpc_lowcomm_group_free(&comm->group);

	if(comm->left_comm)
	{
		int lrefcount = _mpc_lowcomm_communicator_relax(comm->left_comm);

		mpc_common_debug("FREE LEFT %d", lrefcount);

		assume(lrefcount >= 0);

		if(lrefcount == 1)
		{
			__comm_free(comm->left_comm);
		}
	}

	if(comm->right_comm)
	{
		int rrefcount = _mpc_lowcomm_communicator_relax(comm->right_comm);

		mpc_common_debug("FREE RIGHT %d", rrefcount);

		assume(rrefcount >= 0);

		if(rrefcount == 1)
		{
			__comm_free(comm->right_comm);
		}
	}

	___free_topo_comm(comm);
}

int mpc_lowcomm_communicator_free(mpc_lowcomm_communicator_t *pcomm)
{
	if(!pcomm)
	{
		/* Already freed */
		return MPC_LOWCOMM_SUCCESS;
	}

	mpc_lowcomm_communicator_t comm = *pcomm;

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(comm == MPC_COMM_NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	int local_task_count = mpc_lowcomm_communicator_local_task_count(comm);

	/* Only barrier when freeing local comms or intercomms */
	if(mpc_lowcomm_communicator_rank(comm) != MPC_PROC_NULL)
	{
		mpc_lowcomm_barrier(comm);
	}

	int current_refc = OPA_fetch_and_incr_int(&comm->free_count);

	mpc_common_nodebug("%d / %d", current_refc, local_task_count);

	if( (current_refc + 1) == local_task_count)
	{
		int refcount = _mpc_lowcomm_communicator_relax(comm);


		if(refcount == 1)
		{
			/* It is now time to free this comm */
			__comm_free(comm);
		}
	}

	*pcomm = MPC_COMM_NULL;

	return MPC_LOWCOMM_SUCCESS;
}

static inline mpc_lowcomm_communicator_t __new_communicator(mpc_lowcomm_communicator_t comm,
                                                            mpc_lowcomm_group_t *group,
                                                            mpc_lowcomm_communicator_t left_comm,
                                                            mpc_lowcomm_communicator_t right_comm,
                                                            int is_comm_self,
                                                            int check_if_current_rank_belongs,
                                                            mpc_lowcomm_communicator_id_t forced_id)
{
	/* Only the group leader should allocate and register the others should wait */
	static mpc_common_spinlock_t lock = { 0 };

	mpc_lowcomm_internal_communicator_t *ret = NULL;

	mpc_lowcomm_communicator_id_t new_id = forced_id;

	comm       = __mpc_lowcomm_communicator_from_predefined(comm);
	left_comm  = __mpc_lowcomm_communicator_from_predefined(left_comm);
	right_comm = __mpc_lowcomm_communicator_from_predefined(right_comm);
	/* We need a new context ID */
	if(new_id == MPC_LOWCOMM_COMM_NULL_ID)
	{
		new_id = __communicator_id_new_for_comm(comm);
	}

	mpc_common_nodebug("New ID  %u => %u (forced : %u) is inter ? %d", comm->id, new_id, forced_id, mpc_lowcomm_communicator_is_intercomm(comm) );


	int comm_local_lead = mpc_lowcomm_communicator_local_lead(comm);
	int my_rank         = mpc_lowcomm_communicator_rank(comm);

	int current_rank_belongs = 1;


	int at_least_one_local_rank_belongs = 0;

	if(group == NULL)
	{
		if(!is_comm_self)
		{
			/* Intercomm or comm self */
			assume(left_comm && right_comm);
			assume(left_comm != right_comm);
		}
		/* No need to check group membership as there is no group*/
		at_least_one_local_rank_belongs = 1;
	}
	else
	{
		/* Intracomm */
		assume(!left_comm && !right_comm);

		if(check_if_current_rank_belongs)
		{
			/* If the process is not in the group we do return MPI_COMM_NULL */
			if(!mpc_lowcomm_group_includes(group, mpc_lowcomm_get_rank(), mpc_lowcomm_monitor_get_uid() ) )
			{
				current_rank_belongs = 0;
				ret = MPC_COMM_NULL;
			}
		}

		/* Now proceed to check group membership */
		int i;

		unsigned int g_size = mpc_lowcomm_group_size(group);

		for( i = 0 ; i < g_size; i++)
		{
			mpc_lowcomm_peer_uid_t tuid = mpc_lowcomm_group_process_uid_for_rank(group, i);


			if(tuid == mpc_lowcomm_monitor_get_uid() )
			{
				if(mpc_lowcomm_group_includes(group, mpc_lowcomm_group_world_rank(group, i), mpc_lowcomm_monitor_get_uid() ) )
				{
					at_least_one_local_rank_belongs = 1;
					ret = MPC_COMM_NULL;
				}
			}
		}
	}

	if(at_least_one_local_rank_belongs || !check_if_current_rank_belongs)
	{
		//mpc_common_debug_error("LOCAL LEAD %d MY RANK %d", comm_local_lead, my_rank);

		if(comm_local_lead == my_rank)
		{
			/* I am a local lead in my comm I take the lock
			 * to see if the new communicator is known */
			mpc_common_spinlock_lock_yield(&lock);

			ret = mpc_lowcomm_get_communicator_from_id(new_id);

			/* It is not known so I do create it I'm sure I'm the only
			 * as I hold the creation lock */
			if(ret == MPC_COMM_NULL)
			{
				/* We can then directly create a new group using the current group */
				ret = __init_communicator_with_id(new_id, group, is_comm_self, left_comm, right_comm, -1);
				/* Make sure that dups of comm self behave as comm self */

				/* If parent has a ctx pointer make sure to propagate */
				if(comm->extra_ctx_ptr)
				{
					ret->extra_ctx_ptr = comm->extra_ctx_ptr;
				}
			}

			mpc_common_spinlock_unlock(&lock);
		}
	}

	/* Do a barrier when done to ensure new comm is posted */
	mpc_lowcomm_barrier(comm);

	if(current_rank_belongs)
	{
		if(comm_local_lead != my_rank)
		{
			ret = mpc_lowcomm_get_communicator_from_id(new_id);
			assume(ret != NULL);
			assume(ret->id == new_id);
		}
	}

	/* Do a barrier when done to ensure dup do not interleave */
	mpc_lowcomm_barrier(comm);

	ret = __mpc_lowcomm_communicator_to_predefined(ret);

	return ret;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group_forced_id(mpc_lowcomm_group_t *group,
                                                                         mpc_lowcomm_communicator_id_t forced_id)
{
	mpc_lowcomm_communicator_t ret = __init_communicator_with_id(forced_id, group, 0, MPC_COMM_NULL, MPC_COMM_NULL, -1);

	/* Make sure we inherit ctx pointer from the group */
	mpc_lowcomm_communicator_set_context_pointer(ret, mpc_lowcomm_group_get_context_pointer(group) );
	ret = __mpc_lowcomm_communicator_to_predefined(ret);
	return ret;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group(mpc_lowcomm_communicator_t comm,
                                                               mpc_lowcomm_group_t *group)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	/* This is necessarily an intracomm */
	mpc_lowcomm_communicator_t ret = __new_communicator(comm, group, NULL, NULL, comm->is_comm_self, 1, MPC_LOWCOMM_COMM_NULL_ID);

	/* Make sure we inherit ctx pointer from the group */
	mpc_lowcomm_communicator_set_context_pointer(ret, mpc_lowcomm_group_get_context_pointer(group) );
	ret = __mpc_lowcomm_communicator_to_predefined(ret);
	return ret;
}

mpc_lowcomm_group_t * mpc_lowcomm_communicator_group(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	if(comm->is_comm_self)
	{
		int my_rank = mpc_common_get_task_rank();
		return mpc_lowcomm_group_create(1, &my_rank);
	}

	mpc_lowcomm_communicator_t local = MPC_COMM_NULL;

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		/* We assume comm_group means local for intercomm */
		local = mpc_lowcomm_communicator_get_local(comm);
	}
	else
	{
		local = comm;
	}

	return mpc_lowcomm_group_dup(local->group);
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_dup(mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	return __new_communicator(comm, comm->group, comm->left_comm, comm->right_comm, comm->is_comm_self, 0, MPC_LOWCOMM_COMM_NULL_ID);
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create(const mpc_lowcomm_communicator_t comm,
                                                           const int size,
                                                           const int *members)
{
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	/* Time to create a new group */
	_mpc_lowcomm_group_rank_descriptor_t *rank_desc = sctk_malloc(size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	int i;

	if(!local_comm->is_comm_self)
	{
		for(i = 0; i < size; i++)
		{
			_mpc_lowcomm_group_rank_descriptor_set(&rank_desc[i], members[i]);
		}
	}
	else
	{
		assume(members[0] == mpc_lowcomm_get_rank() );
		_mpc_lowcomm_group_rank_descriptor_set(&rank_desc[0], mpc_lowcomm_get_rank() );
	}

	mpc_lowcomm_group_t *      new = _mpc_lowcomm_group_create(size, rank_desc, 1);
	mpc_lowcomm_communicator_t ret = mpc_lowcomm_communicator_from_group(local_comm, new);

	mpc_lowcomm_group_free(&new);

	ret = __mpc_lowcomm_communicator_to_predefined(ret);

	return ret;
}

/************************
* COMMON COMMUNICATORS *
************************/

static mpc_lowcomm_internal_communicator_t *__comm_world = NULL;
static mpc_lowcomm_internal_communicator_t *__comm_self  = NULL;

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_world()
{
	assume(__comm_world != NULL);
	return __comm_world;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_self()
{
	assume(__comm_self != NULL);
	return __comm_self;
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_world_id_gid(mpc_lowcomm_set_uid_t gid)
{
	uint64_t lgid = gid;

	return (lgid << 32) | MPC_LOWCOMM_COMM_WORLD_NUMERIC_ID;
}

mpc_lowcomm_set_uid_t mpc_lowcomm_get_comm_gid(mpc_lowcomm_communicator_id_t comm_id)
{
	return comm_id >> 32;
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_world_id(void)
{
	return mpc_lowcomm_get_comm_world_id_gid(mpc_lowcomm_monitor_get_gid() );
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_self_id(void)
{
	uint64_t gid = mpc_lowcomm_monitor_get_gid();

	return (gid << 32) | MPC_LOWCOMM_COMM_SELF_NUMERIC_ID;
}

void _mpc_lowcomm_communicator_init(void)
{
	__communicator_id_factory_init();
	/* This creates the world group */
	_mpc_lowcomm_group_create_world();
	mpc_lowcomm_group_t *cw_group = _mpc_lowcomm_group_world();

	assume(__comm_world == NULL);
	__comm_world = __init_communicator_with_id(MPC_LOWCOMM_COMM_WORLD_ID,
	                                           cw_group,
	                                           0,
	                                           MPC_COMM_NULL,
	                                           MPC_COMM_NULL,
	                                           MPC_LOWCOMM_COMM_WORLD_NUMERIC_ID);

	assume(__comm_self == NULL);
	__comm_self = __init_communicator_with_id(MPC_LOWCOMM_COMM_SELF_ID,
	                                          NULL,
	                                          1,
	                                          MPC_COMM_NULL,
	                                          MPC_COMM_NULL,
	                                          MPC_LOWCOMM_COMM_SELF_NUMERIC_ID);
}

void _mpc_lowcomm_communicator_release(void)
{
	_mpc_lowcomm_group_release_world();
	__communicator_id_factory_release();
}

/*************
* ACCESSORS *
*************/

int mpc_lowcomm_communicator_local_lead(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(comm->is_comm_self)
	{
		return 0;
	}

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		if(!mpc_lowcomm_communicator_in_left_group(comm) )
		{
			return MPC_PROC_NULL;
		}

		return mpc_lowcomm_communicator_local_lead(comm->left_comm);
	}
	else
	{
		assert(comm->group != NULL);
		return mpc_lowcomm_group_get_local_leader(comm->group);
	}

	not_reachable();

	return MPC_PROC_NULL;
}

int mpc_lowcomm_communicator_contains(const mpc_lowcomm_communicator_t comm,
                                      const mpc_lowcomm_peer_uid_t uid)
{
	assert(comm != NULL);

	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	assume(tcomm->group);

	unsigned int i;

	for(i = 0 ; i < tcomm->group->size; i++)
	{
		if(tcomm->group->ranks[i].host_process_uid == uid)
		{
			return 1;
		}
	}

	return 0;
}

int mpc_lowcomm_communicator_rank_of_as(const mpc_lowcomm_communicator_t comm,
                                        const int comm_world_rank,
                                        const int lookup_cw_rank,
                                        mpc_lowcomm_peer_uid_t lookup_uid)
{
	assert(0 <= comm_world_rank);
	assert(0 <= lookup_cw_rank);
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);

	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local_as(local_comm, lookup_cw_rank, lookup_uid);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 0;
	}

	return mpc_lowcomm_group_rank_for(tcomm->group, comm_world_rank, lookup_uid);
}

int mpc_lowcomm_communicator_rank_of(const mpc_lowcomm_communicator_t comm,
                                     const int comm_world_rank)
{
	return mpc_lowcomm_communicator_rank_of_as(comm, comm_world_rank, mpc_lowcomm_get_rank(), mpc_lowcomm_monitor_get_uid() );
}

int mpc_lowcomm_communicator_rank(const mpc_lowcomm_communicator_t comm)
{
	return mpc_lowcomm_communicator_rank_of(comm, mpc_lowcomm_get_rank() );
}

int mpc_lowcomm_communicator_size(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 1;
	}

	return mpc_lowcomm_group_size(tcomm->group);
}

int mpc_lowcomm_communicator_local_task_count(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		return mpc_lowcomm_communicator_local_task_count(comm->left_comm) + mpc_lowcomm_communicator_local_task_count(comm->right_comm);
	}


	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 1;
	}

	if(tcomm->process_span == 1)
	{
		return mpc_lowcomm_communicator_size(tcomm);
	}

	return _mpc_lowcomm_group_local_process_count(tcomm->group);
}

int mpc_lowcomm_communicator_world_rank_of(const mpc_lowcomm_communicator_t comm, int rank)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		/* For self rank is always the process itself */
		return mpc_common_get_task_rank();
	}

	return mpc_lowcomm_group_world_rank(tcomm->group, rank);
}

int mpc_lowcomm_communicator_get_process_count(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 1;
	}

	return _mpc_lowcomm_group_process_count(tcomm->group);
}

int *mpc_lowcomm_communicator_get_process_list(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		/* Here we always have current process */
		return &__process_rank;
	}

	return _mpc_lowcomm_group_process_list(tcomm->group);
}

mpc_lowcomm_peer_uid_t mpc_lowcomm_communicator_remote_uid(const mpc_lowcomm_communicator_t comm, int rank)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_remote(local_comm);

	assert(tcomm != NULL);

	return mpc_lowcomm_group_process_uid_for_rank(tcomm->group, rank);
}

mpc_lowcomm_peer_uid_t mpc_lowcomm_communicator_uid(const mpc_lowcomm_communicator_t comm, int rank)
{
	assert(comm != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_lowcomm_communicator_t       tcomm      = mpc_lowcomm_communicator_get_local(local_comm);

	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return mpc_lowcomm_monitor_get_uid();
	}

	return mpc_lowcomm_group_process_uid_for_rank(tcomm->group, rank);
}

typedef struct
{
	int color;
	int key;
	int rank;
} mpc_comm_split_t;

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_split(mpc_lowcomm_communicator_t comm, int color, int key)
{
	int i, j, k;

	mpc_lowcomm_communicator_t comm_out = MPC_COMM_NULL;

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	int size = mpc_lowcomm_communicator_size(comm);
	int rank = mpc_lowcomm_communicator_rank(comm);

#if 0
	/* This code is to check the gather if we have doubts as we recently changed it */
	int *itable = sctk_malloc(size * sizeof(int) );

	mpc_lowcomm_allgather(&rank, itable, sizeof(int), comm);


	for(i = 0 ; i < size; i++)
	{
		assume(i == itable[i]);
	}


	sctk_free(itable);
#endif

	mpc_comm_split_t *tab = ( mpc_comm_split_t * )sctk_malloc(size * sizeof(mpc_comm_split_t) );
	assume(tab != NULL);

	mpc_comm_split_t tab_this;

	tab_this.rank  = rank;
	tab_this.color = color;
	tab_this.key   = key;

	mpc_lowcomm_allgather(&tab_this, tab, sizeof(mpc_comm_split_t), comm);

	assert(tab[rank].rank == tab_this.rank);
	assert(tab[rank].color == tab_this.color);
	assert(tab[rank].key == tab_this.key);

	mpc_comm_split_t tab_tmp;
	/*Sort the new tab */
	for(i = 0; i < size; i++)
	{
		for(j = 0; j < size - 1; j++)
		{
			if(tab[j].color > tab[j + 1].color)
			{
				tab_tmp    = tab[j + 1];
				tab[j + 1] = tab[j];
				tab[j]     = tab_tmp;
			}
			else
			{
				if( (tab[j].color == tab[j + 1].color) &&
				    (tab[j].key > tab[j + 1].key) )
				{
					tab_tmp    = tab[j + 1];
					tab[j + 1] = tab[j];
					tab[j]     = tab_tmp;
				}
			}
		}
	}

	int *color_tab = ( int * )sctk_malloc(size * sizeof(int) );
	assume(color_tab != NULL);

	int color_number = 0;

	for(i = 0; i < size; i++)
	{
		/* For each cell  check if color is known */
		for(j = 0; j < color_number; j++)
		{
			if(color_tab[j] == tab[i].color)
			{
				break;
			}
		}

		/* If we reached the end of the loop new color */
		if(j == color_number)
		{
			color_tab[j] = tab[i].color;
			color_number++;
		}

		mpc_common_nodebug("COL rank %d color %d", i, tab[i].rank, tab[i].color);
	}

	mpc_common_nodebug("%d colors", color_number);

	/*We need on comm_create per color */
	for(k = 0; k < color_number; k++)
	{
		int group_size = 0;
		int tmp_color  = color_tab[k];

		if(tmp_color != MPC_UNDEFINED)
		{
			group_size = 0;

			for(i = 0; i < size; i++)
			{
				if(tab[i].color == tmp_color)
				{
					group_size++;
				}
			}

			mpc_common_nodebug("GROUP is %d", group_size);


			int *comm_world_ranks = ( int * )sctk_malloc(group_size * sizeof(int) );
			assume(comm_world_ranks != NULL);

			j = 0;

			for(i = 0; i < size; i++)
			{
				if(tab[i].color == tmp_color)
				{
					comm_world_ranks[j] = mpc_lowcomm_communicator_world_rank_of(comm, tab[i].rank);
					//mpc_common_debug_error("Thread %d color (%d) size %d on %d rank %d", tmp_color,
					//                   k, j, group_size, tab[i].rank);
					j++;
				}
			}

			mpc_lowcomm_group_t *comm_group = mpc_lowcomm_group_create(group_size, comm_world_ranks);

			mpc_lowcomm_communicator_t new_comm = mpc_lowcomm_communicator_from_group(comm, comm_group);

			if(tmp_color == color)
			{
				comm_out = new_comm;
			}

			/* We free here as the comm now holds a ref */
			mpc_lowcomm_group_free(&comm_group);
			sctk_free(comm_world_ranks);
			//mpc_common_debug_error("Split color %d done", tmp_color);
		}
	}

	sctk_free(color_tab);
	sctk_free(tab);
	//mpc_common_debug_error("Split done");

	comm_out = __mpc_lowcomm_communicator_to_predefined(comm_out);

	return comm_out;
}

/**********************
* INTERCOMMUNICATORS *
**********************/
mpc_lowcomm_communicator_t mpc_lowcomm_intercommunicator_merge(mpc_lowcomm_communicator_t intercomm, int high)
{
	intercomm = __mpc_lowcomm_communicator_from_predefined(intercomm);
	assume(mpc_lowcomm_communicator_is_intercomm(intercomm) );

	int local_size  = mpc_lowcomm_communicator_size(intercomm);
	int remote_size = mpc_lowcomm_communicator_remote_size(intercomm);

	int total_size = local_size + remote_size;

	_mpc_lowcomm_group_rank_descriptor_t *remote_descriptors = sctk_malloc(total_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(remote_descriptors != NULL);

	/* Determine order */
	int local_rank = mpc_lowcomm_communicator_rank(intercomm);

	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(intercomm);
	mpc_lowcomm_communicator_t local_comm  = mpc_lowcomm_communicator_get_local(intercomm);


	assume(remote_comm != NULL);
	assume(local_comm != NULL);

	mpc_lowcomm_communicator_id_t intracomm_id = MPC_LOWCOMM_COMM_NULL_ID;

	/* Roots build their list according to high */
	if(local_rank == 0)
	{
		/* ########################################
		 * Build the process list according to high
		 ########################################### */
		int remote_high = 0;
		mpc_lowcomm_sendrecv(&high, sizeof(int), 0, 0, &remote_high, 0, intercomm);

		int local_root_rank  = mpc_lowcomm_communicator_world_rank_of(intercomm, 0);
		int remote_root_rank = mpc_lowcomm_communicator_remote_world_rank(intercomm, 0);

		mpc_lowcomm_group_t *first  = NULL;
		mpc_lowcomm_group_t *second = NULL;

		/* By default we order according to lead ranks */
		if(local_root_rank < remote_root_rank)
		{
			first  = local_comm->group;
			second = remote_comm->group;
		}
		else
		{
			first  = remote_comm->group;
			second = local_comm->group;
		}

		if(remote_high != high)
		{
			if(remote_high < high)
			{
				/* I am willing to have the upper part
				 * note that equal is ignored here */
				if(second != local_comm->group)
				{
					first  = remote_comm->group;
					second = local_comm->group;
				}
			}
			else
			{
				/* The other is willing to have the high part */
				if(second == local_comm->group)
				{
					first  = local_comm->group;
					second = remote_comm->group;
				}
			}
		}

		int          cnt = 0;
		unsigned int i;
		for(i = 0; i < first->size; i++)
		{
			memcpy(&remote_descriptors[cnt],
			       mpc_lowcomm_group_descriptor(first, i),
			       sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
			cnt++;
		}

		for(i = 0; i < second->size; i++)
		{
			memcpy(&remote_descriptors[cnt],
			       mpc_lowcomm_group_descriptor(second, i),
			       sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
			cnt++;
		}

	#if 0
		for( i = 0 ; i < cnt; i++)
		{
			mpc_common_debug("MERGING %d is %d", i, remote_descriptors[i].comm_world_rank);
		}
	#endif

		/* ########################################
		 * Elect who is going to choose the ID
		 ########################################### */

		/* Here two cases, same group higher comm world sends
		 * second case different groups higher set sends */

		int does_recv_id = 0;

		mpc_common_nodebug("ABOUT TO GEN ID GREM %lu GLOC %lu", mpc_lowcomm_get_comm_gid(remote_comm->id), mpc_lowcomm_get_comm_gid(local_comm->id) );

		if(mpc_lowcomm_get_comm_gid(remote_comm->id) < mpc_lowcomm_get_comm_gid(local_comm->id) )
		{
			/* Note strictly lower also covers being != !! */
			does_recv_id = 1;
		}
		else
		{
			int my_world_rank     = mpc_lowcomm_communicator_world_rank_of(intercomm, 0);
			int remote_world_rank = mpc_lowcomm_communicator_remote_world_rank(intercomm, 0);

			if(my_world_rank < remote_world_rank)
			{
				does_recv_id = 1;
			}
		}


		if(does_recv_id)
		{
			/* I do receive the ID */
			mpc_lowcomm_recv(0, &intracomm_id, sizeof(mpc_lowcomm_communicator_id_t), 128, intercomm);
		}
		else
		{
			/* I do create and send the ID */
			intracomm_id = __communicator_id_new();
			mpc_lowcomm_send(0, &intracomm_id, sizeof(mpc_lowcomm_communicator_id_t), 128, intercomm);
		}

		mpc_common_nodebug("HERE INTRACOMM ID is %lu", intracomm_id);
	}

	/* Root broadcasts its finding to local comm */

	/* First all the descriptors */
	mpc_lowcomm_bcast(remote_descriptors,
	                  total_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
	                  0,
	                  local_comm);

	/* And second the id of the new intracomm */
	mpc_lowcomm_bcast(&intracomm_id,
	                  sizeof(mpc_lowcomm_communicator_id_t),
	                  0,
	                  local_comm);

	mpc_lowcomm_group_t *intracomm_group = _mpc_lowcomm_group_create(total_size, remote_descriptors, 1);

	mpc_lowcomm_communicator_t intracomm = __new_communicator(intercomm,
	                                                          intracomm_group,
	                                                          NULL,
	                                                          NULL,
	                                                          0,
	                                                          0,
	                                                          intracomm_id);
	mpc_lowcomm_group_free(&intracomm_group);

	intracomm = __mpc_lowcomm_communicator_to_predefined(intracomm);

	return intracomm;
}

static inline mpc_lowcomm_communicator_id_t __intercomm_root_id_exchange(const mpc_lowcomm_communicator_t local_comm,
                                                                         const mpc_lowcomm_communicator_t peer_comm,
                                                                         const int tag,
                                                                         const int local_comm_rank,
                                                                         const int local_leader,
                                                                         const int remote_leader)
{
	mpc_lowcomm_communicator_id_t new_id = MPC_LOWCOMM_COMM_NULL_ID;

	int cw_rank = mpc_lowcomm_get_rank();

	if(local_comm_rank == local_leader)
	{
		/* We are local leaders */

		const mpc_lowcomm_communicator_t peer_comm_valid = __mpc_lowcomm_communicator_from_predefined(peer_comm);
		/* Get remote world rank */
		int remote_cw_rank = mpc_lowcomm_communicator_world_rank_of(peer_comm_valid, remote_leader);

		assume(cw_rank != remote_cw_rank);

		if(cw_rank < remote_cw_rank)
		{
			/* I do receive the id */
			mpc_lowcomm_recv(remote_leader, &new_id, sizeof(mpc_lowcomm_communicator_id_t), tag, peer_comm_valid);
		}
		else
		{
			new_id = __communicator_id_new();
			mpc_lowcomm_send(remote_leader, &new_id, sizeof(mpc_lowcomm_communicator_id_t), tag, peer_comm_valid);
		}
	}

	const mpc_lowcomm_communicator_t local_comm_valid = __mpc_lowcomm_communicator_from_predefined(local_comm);

	mpc_lowcomm_bcast(&new_id, sizeof(mpc_lowcomm_communicator_id_t), local_leader, local_comm_valid);

	return new_id;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create(const mpc_lowcomm_communicator_t local_comm,
                                                                     const int local_leader,
                                                                     const mpc_lowcomm_communicator_t peer_comm,
                                                                     const int remote_leader,
                                                                     const int tag)
{
	if(0 && local_leader == remote_leader)
	{
		return MPC_COMM_NULL;
	}

	if( (peer_comm == MPC_COMM_NULL) || (local_comm == MPC_COMM_NULL) )
	{
		return MPC_COMM_NULL;
	}


	/* #####################################
	 * Create left comm as a dup of local comm
	 #################################### */

	/* First step is to duplicate the local_comm which is left */
	mpc_lowcomm_communicator_t left_comm = local_comm;

	if(left_comm == MPC_COMM_NULL)
	{
		return left_comm;
	}

	/* Now we need to create right comm locally by exchanging comm info */

	int local_comm_rank = mpc_lowcomm_communicator_rank(left_comm);
	int local_comm_size = mpc_lowcomm_communicator_size(left_comm);

	/* ######################################
	 *   First of all get this new comm an id
	 ##################################### */

	mpc_lowcomm_communicator_id_t intercomm_id = __intercomm_root_id_exchange(left_comm, peer_comm, tag, local_comm_rank, local_leader, remote_leader);

	/* ###################################
	*   First attemps exchange comm ids
	################################### */
	mpc_lowcomm_communicator_id_t left_comm_id  = mpc_lowcomm_communicator_id(left_comm);
	mpc_lowcomm_communicator_id_t right_comm_id = MPC_LOWCOMM_COMM_NULL_ID;

	/* Roots exchange ids */
	if(local_comm_rank == local_leader)
	{
		mpc_lowcomm_sendrecv(&left_comm_id, sizeof(mpc_lowcomm_communicator_id_t), remote_leader, tag, &right_comm_id, remote_leader, peer_comm);
	}

	/* Ids are bcast on local comm */
	mpc_lowcomm_bcast(&right_comm_id, sizeof(mpc_lowcomm_communicator_id_t), local_leader, left_comm);

	/* One possibility is that the right comm is locally known thanks to global ids */
	mpc_lowcomm_communicator_t right_comm = mpc_lowcomm_get_communicator_from_id(right_comm_id);

	/* We now want to make sure the comm is known on the whole local comm*/
	int right_comm_missing            = (right_comm == MPC_COMM_NULL);
	int one_local_does_not_know_right = right_comm_missing;

	int i;

	for(i = 0; i < local_comm_size; i++)
	{
		/* Set my flag as I'm root */
		if(i == local_comm_rank)
		{
			one_local_does_not_know_right = right_comm_missing;
		}

		mpc_lowcomm_bcast(&one_local_does_not_know_right, sizeof(int), i, left_comm);

		if(one_local_does_not_know_right)
		{
			break;
		}
	}

	int someone_right_does_not_know_knows_my_local = 0;

	/* Roots exchange remote knowledge */
	if(local_comm_rank == local_leader)
	{
		mpc_lowcomm_sendrecv(&one_local_does_not_know_right,
		                     sizeof(int),
		                     remote_leader,
		                     tag,
		                     &someone_right_does_not_know_knows_my_local,
		                     remote_leader,
		                     peer_comm);
	}

	mpc_lowcomm_bcast(&someone_right_does_not_know_knows_my_local, sizeof(int), local_leader, left_comm);


	mpc_lowcomm_communicator_t ret = MPC_COMM_NULL;

	if(!someone_right_does_not_know_knows_my_local && !one_local_does_not_know_right)
	{
		/* The comm ID is globally known on both sides no need for exchanges */
		/* === DONE */
		/* First create a communicator to become the intercomm */
		ret = __new_communicator(local_comm,
		                         NULL,
		                         left_comm,
		                         right_comm,
		                         0,
		                         0,
		                         intercomm_id);
	}
	else
	{
		/* ########################################
		*  Serialize communicators for right build
		######################################## */

		/* If we are here someone does not know the comm either left of right
		 * we will therefore send our descriptors to the right in order to
		 * allow it to build my local locally "serializing the comm" to do so
		 * we first send our size and then our comm_data allowing the group
		 * to be constructed and eventually the final comm */

		int right_comm_size = 0;

		/* Roots exchange their local sizes */
		if(local_comm_rank == local_leader)
		{
			mpc_lowcomm_sendrecv(&local_comm_size, sizeof(uint32_t), remote_leader, tag, &right_comm_size, remote_leader, peer_comm);
		}

		/* Right comm size is bcast on local comm */
		mpc_lowcomm_bcast(&right_comm_size, sizeof(uint32_t), local_leader, left_comm);
		assume(right_comm_size != 0);

		_mpc_lowcomm_group_rank_descriptor_t *remote_descriptors = sctk_malloc(right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(remote_descriptors != NULL);

		/* Roots exchange their descriptors */
		if(local_comm_rank == local_leader)
		{
			mpc_lowcomm_request_t reqs[2];

			mpc_lowcomm_isend(remote_leader,
			                  left_comm->group->ranks,
			                  local_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
			                  tag,
			                  peer_comm,
			                  &reqs[0]);

			mpc_lowcomm_irecv(remote_leader,
			                  remote_descriptors,
			                  right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
			                  tag,
			                  peer_comm,
			                  &reqs[1]);

			mpc_lowcomm_waitall(2, reqs, MPC_LOWCOMM_STATUS_NULL);
		}

		/* Right descriptors are bcast on local comm */
		mpc_lowcomm_bcast(remote_descriptors, right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t), local_leader, left_comm);

		/* Now we create a group from descriptors */
		mpc_lowcomm_group_t *right_group = _mpc_lowcomm_group_create(right_comm_size, remote_descriptors, 1);

		/* And eventually our comm of interest */
		right_comm = __new_communicator(left_comm,
		                                right_group,
		                                NULL,
		                                NULL,
		                                0,
		                                0,
		                                right_comm_id);


		mpc_lowcomm_group_free(&right_group);

		/* First create a communicator to become the intercomm */
		ret = __new_communicator(local_comm,
		                         NULL,
		                         left_comm,
		                         right_comm,
		                         0,
		                         0,
		                         intercomm_id);
	}

	/****************************************************
	* MANUAL BARRIER ON COMM TO ENSURE GLOBAL CREATION *
	****************************************************/
	mpc_lowcomm_barrier(left_comm);

	int dummy;

	if(local_comm_rank == local_leader)
	{
		mpc_lowcomm_sendrecv(&dummy,
		                     sizeof(int),
		                     remote_leader,
		                     tag,
		                     &dummy,
		                     remote_leader,
		                     peer_comm);
	}

	mpc_lowcomm_barrier(left_comm);

	return ret;
}

int mpc_lowcomm_communicator_in_left_group_rank(const mpc_lowcomm_communicator_t communicator,
                                                int comm_world_rank,
                                                mpc_lowcomm_peer_uid_t uid)
{
	assert(communicator != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(communicator);

	assert(local_comm->left_comm);
	return mpc_lowcomm_group_includes(local_comm->left_comm->group, comm_world_rank, uid);
}

int mpc_lowcomm_communicator_in_left_group(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_in_left_group_rank(communicator,
	                                                   mpc_lowcomm_get_rank(),
	                                                   mpc_lowcomm_monitor_get_uid() );
}

int mpc_lowcomm_communicator_in_right_group_rank(const mpc_lowcomm_communicator_t communicator,
                                                 int comm_world_rank,
                                                 mpc_lowcomm_peer_uid_t uid)
{
	assert(communicator != NULL);
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(communicator);

	assume(local_comm->right_comm != NULL);
	return mpc_lowcomm_group_includes(local_comm->right_comm->group, comm_world_rank, uid);
}

int mpc_lowcomm_communicator_in_right_group(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_in_right_group_rank(communicator,
	                                                    mpc_lowcomm_get_rank(),
	                                                    mpc_lowcomm_monitor_get_uid() );
}

int mpc_lowcomm_communicator_in_master_group(const mpc_lowcomm_communicator_t communicator)
{
	const mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(communicator);

	assume(mpc_lowcomm_communicator_is_intercomm(local_comm) );

	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(local_comm);

	int remote_master_rank = mpc_lowcomm_communicator_world_rank_of(remote_comm, 0);
	int local_master_rank  = mpc_lowcomm_communicator_world_rank_of(local_comm, 0);

	mpc_lowcomm_peer_uid_t local_master_uid  = mpc_lowcomm_communicator_uid(local_comm, 0);
	mpc_lowcomm_peer_uid_t remote_master_uid = mpc_lowcomm_communicator_uid(remote_comm, 0);

	/* If local and remote are in the same set we use the ranks
	 * otherwise we use the set IDs */
	mpc_lowcomm_set_uid_t local_master_set  = mpc_lowcomm_peer_get_set(local_master_uid);
	mpc_lowcomm_set_uid_t remote_master_set = mpc_lowcomm_peer_get_set(remote_master_uid);

	if(local_master_set != remote_master_set)
	{
		if(local_master_set < remote_master_set)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if(local_master_rank < remote_master_rank)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	not_reachable();
	return 0;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local_as(mpc_lowcomm_communicator_t comm,
                                                                 int lookup_cw_rank,
                                                                 mpc_lowcomm_peer_uid_t uid)
{
	assert(comm != NULL);
	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		if(mpc_lowcomm_communicator_in_left_group_rank(comm, lookup_cw_rank, uid) )
		{
			return comm->left_comm;
		}
		else if(mpc_lowcomm_communicator_in_right_group_rank(comm, lookup_cw_rank, uid) )
		{
			return comm->right_comm;
		}
	}
	else
	{
		/* Intracomm local comm is myself */
		return comm;
	}

	not_reachable();
	return comm;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local(mpc_lowcomm_communicator_t comm)
{
	return mpc_lowcomm_communicator_get_local_as(comm, mpc_lowcomm_get_rank(), mpc_lowcomm_monitor_get_uid() );
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote_as(mpc_lowcomm_communicator_t comm,
                                                                  int lookup_cw_rank,
                                                                  mpc_lowcomm_peer_uid_t uid)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	/* Only meaningfull for intercomms */
	if(!mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		return MPC_COMM_NULL;
	}

	mpc_lowcomm_communicator_t local = mpc_lowcomm_communicator_get_local_as(comm, lookup_cw_rank, uid);

	/* The remote is the non-local one */
	if(local == comm->left_comm)
	{
		return comm->right_comm;
	}
	else
	{
		return comm->left_comm;
	}

	not_reachable();
	comm = __mpc_lowcomm_communicator_to_predefined(comm);
	return comm;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote(mpc_lowcomm_communicator_t comm)
{
	return mpc_lowcomm_communicator_get_remote_as(comm, mpc_lowcomm_get_rank(), mpc_lowcomm_monitor_get_uid() );
}

// C'est ici qu'il faut continuer !!
int mpc_lowcomm_communicator_remote_size(const mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(comm);

	return mpc_lowcomm_communicator_size(remote_comm);
}

int mpc_lowcomm_communicator_remote_world_rank(const mpc_lowcomm_communicator_t comm, const int rank)
{
	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(comm);

	return mpc_lowcomm_communicator_world_rank_of(remote_comm, rank);
}

int mpc_lowcomm_communicator_is_intercomm(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	/* Comm self also has a null group
	 * so we need to check it first */
	if(comm->is_comm_self)
	{
		return 0;
	}

	int ret = (comm->group == NULL);

	if(ret)
	{
		assert(comm->left_comm != MPC_COMM_NULL);
		assert(comm->right_comm != MPC_COMM_NULL);
	}

	return ret;
}

/**************
* FROM GROUP *
**************/

#ifdef MPC_Threads

struct __poll_comm
{
	mpc_lowcomm_communicator_id_t searched_id;
	mpc_lowcomm_communicator_t    comm;
	volatile int                  done;
};

static void __poll_comm_avail(void *ppoll_comm)
{
	struct __poll_comm *pc = (struct __poll_comm *)ppoll_comm;

	pc->comm = mpc_lowcomm_get_communicator_from_id(pc->searched_id);

	if(pc->comm)
	{
		pc->done = 1;
	}
}

#endif

int mpc_lowcomm_communicator_create_group(mpc_lowcomm_communicator_t comm, mpc_lowcomm_group_t *group, int tag, mpc_lowcomm_communicator_t *newcomm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		mpc_common_debug_error("Cannot create from group on an intercomm");
		return MPC_LOWCOMM_ERROR;
	}

	int my_rank       = mpc_lowcomm_communicator_rank(comm);
	int my_group_rank = MPC_UNDEFINED;

	mpc_lowcomm_group_t *comm_group = mpc_lowcomm_communicator_group(comm);

	int res = mpc_lowcomm_group_translate_ranks(comm_group, 1, &my_rank, group, &my_group_rank);

	if(res != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("Failed translating ranks 1");
		return MPC_LOWCOMM_ERROR;
	}

	int group_size = mpc_lowcomm_group_size(group);

	*newcomm = MPC_COMM_NULL;

	if(my_group_rank != MPC_UNDEFINED)
	{
		/* Here our main concern is to BCAST the new ID manually */
		mpc_lowcomm_communicator_id_t new_comm_id = 0;

		/* COMPUTE the BTREE in GROUP */

		int parent = -1;
		int lc     = (my_group_rank + 1) * 2 - 1;
		int rc     = (my_group_rank + 1) * 2;

		if(0 < my_group_rank)
		{
			parent = -1 + (my_group_rank + 1) / 2;
		}

		if(group_size <= lc)
		{
			lc = -1;
		}

		if(group_size <= rc)
		{
			rc = -1;
		}

		//mpc_common_debug_error("IN GROUP RANK %d     P:%d L:%d  R:%d (size %d)", my_group_rank, parent, lc, rc, group_size);

		int tmp = MPC_PROC_NULL;

		if(0 <= parent)
		{
			tmp = parent;
			res = mpc_lowcomm_group_translate_ranks(group, 1, &tmp, comm_group, &parent);
			if(res != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("Failed translating ranks 2");
				return MPC_LOWCOMM_ERROR;
			}
		}

		if(0 <= lc)
		{
			tmp = lc;
			res = mpc_lowcomm_group_translate_ranks(group, 1, &tmp, comm_group, &lc);
			if(res != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("Failed translating ranks 3");
				return MPC_LOWCOMM_ERROR;
			}
		}

		if(0 <= rc)
		{
			tmp = rc;
			res = mpc_lowcomm_group_translate_ranks(group, 1, &tmp, comm_group, &rc);
			if(res != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("Failed translating ranks 4");
				return MPC_LOWCOMM_ERROR;
			}
		}

		//mpc_common_debug_error("IN COMM P: %d LC : %d  RC : %d", parent, lc, rc);

		/* At this point we have a btree in comm matching group */

		/* I'm part of the new comm */
		if(my_group_rank == 0)
		{
			/* LEAD */
			new_comm_id = mpc_lowcomm_communicator_gen_local_id();
		}
		else
		{
			/* RECEIVER */
			if(0 <= parent)
			{
				res = mpc_lowcomm_recv(parent, &new_comm_id, sizeof(mpc_lowcomm_communicator_id_t), tag, comm);
				if(res != MPC_LOWCOMM_SUCCESS)
				{
					mpc_common_debug_error("Failed  Receiving");

					return MPC_LOWCOMM_ERROR;
				}
			}
		}

		if(0 < lc)
		{
			res = mpc_lowcomm_send(lc, &new_comm_id, sizeof(mpc_lowcomm_communicator_id_t), tag, comm);
			if(res != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("Failed Sending 1");
				return MPC_LOWCOMM_ERROR;
			}
		}

		if(0 < rc)
		{
			res = mpc_lowcomm_send(rc, &new_comm_id, sizeof(mpc_lowcomm_communicator_id_t), tag, comm);
			if(res != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("Failed Sending 2");
				return MPC_LOWCOMM_ERROR;
			}
		}

		int comm_local_lead = mpc_lowcomm_group_get_local_leader(group);

		if(comm_local_lead == my_group_rank)
		{
			/* HERE BCAST is DONE Proceed to build the comm */
			*newcomm = mpc_lowcomm_communicator_from_group_forced_id(group, new_comm_id);
		}
		else
		{
#ifdef MPC_Threads
			struct __poll_comm pc;
			pc.comm        = MPC_COMM_NULL;
			pc.done        = 0;
			pc.searched_id = new_comm_id;

			mpc_thread_wait_for_value_and_poll(&pc.done, 1, __poll_comm_avail, &pc);

			*newcomm = pc.comm;
#else
			while(1)
			{
				*newcomm = mpc_lowcomm_get_communicator_from_id(new_comm_id);

				if(*newcomm)
				{
					break;
				}
			}
#endif
		}
	}

	mpc_lowcomm_group_free(&comm_group);

	mpc_lowcomm_barrier(comm);

	*newcomm = __mpc_lowcomm_communicator_to_predefined(*newcomm);

	return MPC_LOWCOMM_SUCCESS;
}
