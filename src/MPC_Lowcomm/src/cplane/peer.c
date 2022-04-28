#include "peer.h"

#include <mpc_common_datastructure.h>
#include <mpc_common_debug.h>
#include <sctk_alloc.h>
#include <mpc_common_helper.h>

static struct mpc_common_hashtable __peer_ht;

int _mpc_lowcomm_peer_setup()
{
	mpc_common_hashtable_init(&__peer_ht, 1024);
	return 0;
}

int _mpc_lowcomm_peer_teardown()
{
	_mpc_lowcomm_peer_t *peer = NULL;

	MPC_HT_ITER(&__peer_ht, peer)
	{
		_mpc_lowcomm_peer_free(peer);
	}
	MPC_HT_ITER_END(&__peer_ht)

	mpc_common_hashtable_release(&__peer_ht);

	return 0;
}

_mpc_lowcomm_peer_t *_mpc_lowcomm_peer_register(mpc_lowcomm_peer_uid_t uid, uint64_t local_task_count, char *uri, int is_local)
{
	static mpc_common_spinlock_t __peer_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	_mpc_lowcomm_peer_t *ret = NULL;

	/* Make sure there is no race on the peer registration */
	mpc_common_spinlock_lock_yield( &__peer_lock );

	ret = _mpc_lowcomm_peer_get( uid );

	if(ret != NULL)
	{
		mpc_common_spinlock_unlock( &__peer_lock );
		return ret;
	}

	mpc_common_spinlock_unlock( &__peer_lock );

	_mpc_lowcomm_peer_t *new = sctk_malloc(sizeof(_mpc_lowcomm_peer_t) );
	assume(new != NULL);
	new->infos.uid = uid;
	new->is_local = is_local;
	new->infos.local_task_count = local_task_count;
	snprintf(new->infos.uri, MPC_LOWCOMM_PEER_URI_SIZE, uri);

	mpc_common_hashtable_set(&__peer_ht, uid, new);

	return new;
}

int _mpc_lowcomm_peer_free(_mpc_lowcomm_peer_t *peer)
{
	sctk_free(peer);
	return 0;
}

int _mpc_lowcomm_peer_remove(uint64_t uid)
{
	_mpc_lowcomm_peer_t *to_del = _mpc_lowcomm_peer_get(uid);

	if(to_del)
	{
		return 1;
	}

	mpc_common_hashtable_delete(&__peer_ht, uid);

	_mpc_lowcomm_peer_free(to_del);

	return 0;
}

_mpc_lowcomm_peer_t *_mpc_lowcomm_peer_get(mpc_lowcomm_peer_uid_t uid)
{
	return (_mpc_lowcomm_peer_t *)mpc_common_hashtable_get(&__peer_ht, uid);
}

int mpc_lowcomm_peer_closer(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_peer_uid_t current, mpc_lowcomm_peer_uid_t candidate)
{
	//mpc_common_debug_error("Dest is %lu CURRENT %lu CANDIDATE %lu", dest, current, candidate);

	/* No candidate */
	if(candidate == 0)
	{
		return 0;
	}

	/* Not in the same set */
	if(mpc_lowcomm_peer_get_set(dest) != mpc_lowcomm_peer_get_set(candidate))
	{
		return 0;
	}

	/* No reference pick it if in set */
	if(current == 0)
	{
		return 1;
	}

	int current_rank = mpc_lowcomm_peer_get_rank(current);
	int candidate_rank = mpc_lowcomm_peer_get_rank(candidate);
	int dest_rank = mpc_lowcomm_peer_get_rank(dest);

	int d_cand_d = mpc_common_abs(dest_rank - candidate_rank);
	int d_cur_d = mpc_common_abs(dest_rank - current_rank);
	//mpc_common_debug_error("Dest is %u current %u D %d candidate %u D %d", dest_rank, current, d_cur_d, candidate_rank, d_cand_d);

	if(d_cand_d < d_cur_d)
	{
		return 1;
	}

	return 0;
}

char * mpc_lowcomm_peer_format_r(mpc_lowcomm_peer_uid_t uid, char * buff, int len)
{
	if(!buff)
	{
		return buff;
	}

	snprintf(buff, len, "(%u,%u)", mpc_lowcomm_peer_get_set(uid), mpc_lowcomm_peer_get_rank(uid));

	return buff;
}


char * mpc_lowcomm_peer_format(mpc_lowcomm_peer_uid_t uid)
{
	static char peer_name[128];
	return mpc_lowcomm_peer_format_r(uid, peer_name, 128);
}
