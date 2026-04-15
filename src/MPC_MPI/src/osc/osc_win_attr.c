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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #   - BESNARD Jean-Baptiste                                            # */
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

/************************************************************************/
/* INTERNAL MPI Window Attribute support                                */
/************************************************************************/

/* Keyval low-level storage */
#include "osc_mpi.h"
#include <mpc_common_rank.h>
#include "osc_mpi_internal.h"
#include <mpc_lowcomm_one_sided.h>

struct mpc_mpi_osc_win_keyval
{
	uint64_t                      keyval;
	MPI_Win_copy_attr_function *  copy_fn;
	MPI_Win_delete_attr_function *delete_fn;
	void *                        extra_state;
};

struct mpc_mpi_osc_win_attr
{
	int                           keyval;
	void *                        value;
	struct mpc_mpi_osc_win_keyval keyval_pl;
	mpc_mpi_win_t *               win;
};

static struct mpc_common_hashtable __osc_win_keyval_ht;
static mpc_common_spinlock_t       __osc_win_keyval_ht_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static int __osc_win_keyval_ht_init_done = 0;

static inline void win_keyval_ht_init_once()
{
	mpc_common_spinlock_lock(&__osc_win_keyval_ht_lock);

	if (!__osc_win_keyval_ht_init_done)
	{
		mpc_common_hashtable_init(&__osc_win_keyval_ht, 16);
		__osc_win_keyval_ht_init_done = 1;
	}

	mpc_common_spinlock_unlock(&__osc_win_keyval_ht_lock);
}

static inline uint64_t get_per_rank_keyval_key(int keyval)
{
	uint64_t rank = mpc_common_get_task_rank();
	uint64_t ret  = 0;

	ret  = (rank << 32);
	ret |= keyval;

	return ret;
}

/* Attr definition */

static OPA_int_t __win_attr_counter;

struct mpc_mpi_osc_win_keyval *
mpc_mpi_osc_win_keyval_init(MPI_Win_copy_attr_function *copy_fn,
                            MPI_Win_delete_attr_function *delete_fn,
                            void *extra_state)
{
	struct mpc_mpi_osc_win_keyval *ret =
		sctk_calloc(1, sizeof(struct mpc_mpi_osc_win_keyval));

	if (!ret)
	{
		perror("calloc");
		return NULL;
	}

	ret->copy_fn     = copy_fn;
	ret->delete_fn   = delete_fn;
	ret->extra_state = extra_state;

	int my_id = OPA_fetch_and_add_int(&__win_attr_counter, 1);

	/* Make sure to skip the compulsory attributes  (WIN_BASE, & co ) */
	my_id += 50;

	ret->keyval = my_id;

	return ret;
}

/* Attr storage */

struct mpc_mpi_osc_win_attr *
mpc_mpi_osc_win_attr_init(int keyval, void *value, struct mpc_mpi_osc_win_keyval *keyval_pl, mpc_mpi_win_t *win)
{
	struct mpc_mpi_osc_win_attr *ret = sctk_calloc(1, sizeof(struct mpc_mpi_osc_win_attr));

	if (ret == NULL)
	{
		perror("Calloc");
		return ret;
	}

	ret->keyval = keyval;
	ret->value  = value;
	memcpy(&ret->keyval_pl, keyval_pl, sizeof(struct mpc_mpi_osc_win_keyval));
	ret->win = win;

	return ret;
}

int mpc_mpi_osc_win_attr_ht_init(struct mpc_common_hashtable *atht)
{
	mpc_common_hashtable_init(atht, 16);
	return 0;
}

int mpc_mpi_osc_win_attr_ht_release(struct mpc_common_hashtable *atht)
{
	void *pattr = NULL;

	int found = -1;

	do
	{
		found = -1;

		struct mpc_mpi_osc_win_attr *attr = NULL;
		MPC_HT_ITER(atht, pattr)

		if (found < 0)
		{
			attr = (struct mpc_mpi_osc_win_attr *)pattr;
			if (attr)
			{
				found = attr->keyval;
			}
		}

		MPC_HT_ITER_END(atht)

		if (found != -1)
		{
			if (attr)
			{
				if (attr->keyval_pl.delete_fn != NULL)
				{
					(attr->keyval_pl.delete_fn)((MPI_Win)attr->win, found, attr->value, attr->keyval_pl.extra_state);
				}
			}

			mpc_common_hashtable_delete(atht, found);
			sctk_free(attr);
		}
	} while (found != -1);

	mpc_common_hashtable_release(atht);

	return 0;
}

/* MPI Attr handling */

int mpc_mpi_osc_win_set_attr(mpc_mpi_win_t *win, int keyval, void *attr_val)
{
	win_keyval_ht_init_once();

	if (!attr_val)
	{
		return MPI_ERR_ARG;
	}

	if (!win)
	{
		return MPI_ERR_WIN;
	}

	/* Check keyval */

	uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

	struct mpc_mpi_osc_win_keyval *kv =
		(struct mpc_mpi_osc_win_keyval *)mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!kv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* If we are here ready to set */
	struct mpc_mpi_osc_win_attr *attr = (struct mpc_mpi_osc_win_attr *)mpc_common_hashtable_get(&win->attrs, keyval);

	if (!attr)
	{
		attr = mpc_mpi_osc_win_attr_init(keyval, attr_val, kv, win);
		mpc_common_hashtable_set(&win->attrs, keyval, attr);
	}
	else
	{
		attr->value = attr_val;
	}

	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_get_attr(mpc_mpi_win_t *win, int keyval, void *attr_val, int *flag)
{
	win_keyval_ht_init_once();

	*flag = 0;
	if (keyval == MPI_KEYVAL_INVALID)
	{
		return MPI_ERR_KEYVAL;
	}

	mpc_lowcomm_osc_win_attr_t module_attr = mpc_lowcomm_osc_win_get_attr(win->win_module);

	/* First handle special values */
	switch (keyval)
	{
	case MPI_WIN_BASE:
		*flag = 1;
		*((void **)attr_val) = module_attr.base_data;
		goto out;
		break;

	case MPI_WIN_SIZE:
		*flag = 1;
		*((void **)attr_val) = (void *)&win->size;
		goto out;
		break;

	case MPI_WIN_DISP_UNIT:
		*flag = 1;
		if (win->disp_unit == -1)
		{
			const int comm_rank = mpc_lowcomm_communicator_rank(mpc_lowcomm_osc_win_get_comm(win->win_module));
			*((void **)attr_val) = (void *)&win->disp_units[comm_rank];
		}
		else
		{
			*((void **)attr_val) = (void *)&win->disp_unit;
		}
		goto out;
		break;

	case MPI_WIN_CREATE_FLAVOR:
		*flag = 1;
		*((void **)attr_val) = (void *)&win->flavor;
		goto out;
		break;

	case MPI_WIN_MODEL:
		*flag = 1;
		*((void **)attr_val) = (void *)&win->model;
		goto out;
		break;
	}

	/* If we are here it is an user-defined ATTR */

	/* First check keyval */

	uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

	void *pkv = mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!pkv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* Now try to retrieve attr */
	struct mpc_mpi_osc_win_attr *attr = (struct mpc_mpi_osc_win_attr *)mpc_common_hashtable_get(&win->attrs, keyval);

	if (!attr)
	{
		*flag = 0;
	}
	else
	{
		*flag = 1;
		*((void **)attr_val) = attr->value;
	}

out:
	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_delete_attr(mpc_mpi_win_t *const win, const int keyval)
{
	/* Check keyval */

	uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

	struct mpc_mpi_osc_win_keyval *kv =
		(struct mpc_mpi_osc_win_keyval *)mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!kv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* Now try to retrieve attr */
	struct mpc_mpi_osc_win_attr *attr = (struct mpc_mpi_osc_win_attr *)mpc_common_hashtable_get(&win->attrs, keyval);

	if (attr)
	{
		if (kv->delete_fn != NULL)
		{
			(kv->delete_fn)((MPI_Win)win, keyval, attr->value, kv->extra_state);
		}

		mpc_common_hashtable_delete(&win->attrs, keyval);
		sctk_free(attr);
	}

	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_create_keyval(MPI_Win_copy_attr_function *copy_fn,
                                  MPI_Win_delete_attr_function *delete_fn,
                                  int *keyval, void *extra_state)
{
	win_keyval_ht_init_once();

	struct mpc_mpi_osc_win_keyval *new = mpc_mpi_osc_win_keyval_init(copy_fn, delete_fn, extra_state);

	if (!new)
	{
		return MPI_ERR_INTERN;
	}

	/* Now add to the ATTR HT */
	uint64_t per_rank_keyval = get_per_rank_keyval_key(new->keyval);

	mpc_common_hashtable_set(&__osc_win_keyval_ht, per_rank_keyval, new);

	/* SET the return keyval */
	*keyval = new->keyval;

	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_free_keyval(const int keyval)
{
	win_keyval_ht_init_once();

	uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

	void *pkv = mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!pkv)
	{
		return MPI_ERR_KEYVAL;
	}

	mpc_common_hashtable_delete(&__osc_win_keyval_ht, per_rank_keyval);
	sctk_free(pkv);

	return MPI_SUCCESS;
}
