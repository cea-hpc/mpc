#include "set.h"

#include <string.h>

#include <mpc_common_debug.h>
#include <mpc_common_datastructure.h>
#include <sctk_alloc.h>
#include <mpc_lowcomm.h>
#include "monitor.h"
#include "uid.h"


static struct mpc_common_hashtable __set_ht;

/*****************
* SETUP TEADOWN *
*****************/

int _mpc_lowcomm_set_setup()
{
	_mpc_lowcomm_peer_setup();
	mpc_common_hashtable_init(&__set_ht, 512);
	return 0;
}

int _mpc_lowcomm_set_teardown()
{
	mpc_common_hashtable_release(&__set_ht);
	_mpc_lowcomm_peer_teardown();
	return 0;
}

/**********************
* REGISTER INTERFACE *
**********************/

_mpc_lowcomm_set_t *_mpc_lowcomm_set_init(mpc_lowcomm_set_uid_t uid,
                                          char *name,
                                          uint64_t total_task_count,
                                          uint64_t *peers_uids,
                                          uint64_t peer_count,
                                          int is_lead,
										  mpc_lowcomm_peer_uid_t local_peer)
{
	assume(_mpc_lowcomm_set_get(uid) == NULL);

	_mpc_lowcomm_set_t *new = sctk_malloc(sizeof(_mpc_lowcomm_set_t) );
	assume(new != NULL);

	new->is_lead = is_lead;
	new->local_peer = local_peer;

	new->uid = uid;
	snprintf(new->name, MPC_LOWCOMM_SET_NAME_LEN, name);

	uint64_t i;
	new->total_task_count = 0;

	new->peers = sctk_malloc(sizeof(_mpc_lowcomm_peer_t *) * peer_count);
	assume(new->peers != NULL);
	new->peer_count = peer_count;

	for(i = 0; i < peer_count; i++)
	{
		_mpc_lowcomm_peer_t *peer = _mpc_lowcomm_peer_get(peers_uids[i]);

		if(!peer)
		{
			bad_parameter("No such peer %ld", peers_uids[i]);
		}

		new->peers[i] = peer;
	}

	new->total_task_count = total_task_count;

	mpc_common_hashtable_set(&__set_ht, uid, new);

	return new;
}

int _mpc_lowcomm_set_free(_mpc_lowcomm_set_t *set)
{
	sctk_free(set->peers);
	set->peers = NULL;

	memset(set, 0, sizeof(_mpc_lowcomm_set_t) );

	sctk_free(set);

	return 0;
}


static inline int __load_set_from_uid(uint32_t uid, char * path)
{
	/* Now load information for each set */
	struct _mpc_lowcomm_uid_descriptor_s sd;

	if( _mpc_lowcomm_uid_descriptor_load(&sd, path) != 0 )
	{
		mpc_common_debug_warning("Failed to load set at %s", path);
		return -1;
	}

	/* Is the set already known ? */
	if(_mpc_lowcomm_set_get(sd.set_uid))
	{
		/* Set is already known just drop */
		return 0;
	}


	/* If we have a set we need to register its main peer for later connections
	   if it is not already known */
	mpc_lowcomm_peer_uid_t main_peer = _mpc_lowcomm_peer_uid(sd.set_uid, 0);

	if(!_mpc_lowcomm_peer_get(main_peer))
	{
		_mpc_lowcomm_peer_register(main_peer,
								0,
								sd.leader_uri,
								0);
	}

	/* First of all we make sure the peer is reachable */
	int failed = 0;
	if(!mpc_lowcomm_monitor_peer_reachable_directly(main_peer))
	{
		/* These info could not be retrieved consider the set as lost */
		_mpc_lowcomm_uid_clear(sd.set_uid);
		return 0;
	}

	/* Note that we don't create the set for now we need its data */

	mpc_lowcomm_monitor_retcode_t command_ret;
	mpc_lowcomm_monitor_response_t set_info =  mpc_lowcomm_monitor_get_set_info(main_peer,
															                    &command_ret);

	if(!set_info)
	{
		/* These info could not be retrieved consider the set as lost */
		_mpc_lowcomm_uid_clear(sd.set_uid);
		return 0;
	}

	/* We have the set info we can create the set */
	_mpc_lowcomm_monitor_command_register_set_info(mpc_lowcomm_monitor_response_get_content(set_info));

	mpc_lowcomm_monitor_response_free(set_info);


	return 0;
}


int _mpc_lowcomm_set_load_from_system(void)
{
	return _mpc_lowcomm_uid_scan(__load_set_from_uid);
}

/*******************
* QUERY INTERFACE *
*******************/

_mpc_lowcomm_set_t *_mpc_lowcomm_set_get(mpc_lowcomm_set_uid_t uid)
{
	return (_mpc_lowcomm_set_t *)mpc_common_hashtable_get(&__set_ht, uid);
}

int _mpc_lowcomm_set_iterate(int (*set_cb)(_mpc_lowcomm_set_t *set, void *arg), void *arg)
{
	_mpc_lowcomm_set_t *set = NULL;

	if(!set_cb)
	{
		return 1;
	}

	MPC_HT_ITER(&__set_ht, set)
	{
		int ret = (set_cb)(set, arg);

		if(ret != 0)
		{
			break;
		}
	}
	MPC_HT_ITER_END

	return 0;
}


int _mpc_lowcomm_set_contains(_mpc_lowcomm_set_t * set, mpc_lowcomm_peer_uid_t peer_uid)
{
	if(!set)
	{
		return 0;
	}

	uint64_t i;

	for(i = 0 ; i < set->peer_count; i++)
	{
		if(set->peers[i]->infos.uid == peer_uid)
		{
			return 1;
		}
	}

	return 0;
}