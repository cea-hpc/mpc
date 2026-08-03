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

/**@file
 * INTERNAL MPI Window Attribute support
 */

#include <mpc_common_rank.h>
#include <mpc_lowcomm_one_sided.h>

#include "osc_mpi.h"
#include "osc_mpi_internal.h"

/**@ingroup MPI_OSC_INTERNAL
 * Internal structure of an osc keyval
 */
struct osc_win_keyval_s
{
	uint64_t                      keyval;      /**< Unique identifier of the keyval */
	MPI_Win_copy_attr_function *  copy_fn;     /**< Copy function */
	MPI_Win_delete_attr_function *delete_fn;   /**< Deletion function */
	void *                        extra_state; /**< Additional information */
};

/**@ingroup MPI_OSC_INTERNAL
 * Internal structure of an osc attribute
 */
struct osc_win_attr_s
{
	int                     keyval;    /**< Unique identifier of the attribute */
	void *                  value;     /**< Information stored in the attribute */
	struct osc_win_keyval_s keyval_pl; /**< Associated keyval  structure */
	mpc_mpi_win_t *         win;       /**< Associated window */
};

/**@ingroup MPI_OSC_INTERNAL
 * Shared keyval hashtable
 */
static struct mpc_common_hashtable __osc_win_keyval_ht;

/**@ingroup MPI_OSC_INTERNAL
 * Global keyval hashtable lock
 */
static mpc_common_spinlock_t __osc_win_keyval_ht_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/**@ingroup MPI_OSC_INTERNAL
 * State of the keyval hashtable (1 for initialized)
 */
static int __osc_win_keyval_ht_init_done = 0;

/**@ingroup MPI_OSC_INTERNAL
 * @brief Initiates the global keyval hashtable is not done already
 */
static inline void __osc_win_keyval_ht_init_once()
{
	mpc_common_spinlock_lock(&__osc_win_keyval_ht_lock);

	if (!__osc_win_keyval_ht_init_done)
	{
		mpc_common_hashtable_init(&__osc_win_keyval_ht, 16);
		__osc_win_keyval_ht_init_done = 1;
	}

	mpc_common_spinlock_unlock(&__osc_win_keyval_ht_lock);
}

/**@ingroup MPI_OSC_INTERNAL
 * @brief Retrieves the per rank keyval
 *
 * @param[in] keyval Global keyval identifier
 *
 * @return           Keyval for the current task
 */
static inline uint64_t __osc_get_per_rank_keyval_key(const int keyval)
{
	uint64_t rank = mpc_common_get_task_rank();
	uint64_t ret  = 0;

	ret  = (rank << 32);
	ret |= keyval;

	return ret;
}

/* Attr definition */

/**@ingroup MPI_OSC_INTERNAL
 * Global attributes counter
 */
static OPA_int_t __win_attr_counter;

/**@ingroup MPI_OSC_INTERNAL
 * @brief Allocates and initializes a new keyval
 *
 * @param[in] copy_fn     Copy function to associate with the keyval
 * @param[in] delete_fn   Deletion function to associate with the keyval
 * @param[in] extra_state Additional information stored in the keyval
 *
 * @return                Allocated and initialized keyval
 */
static struct osc_win_keyval_s * __osc_win_keyval_init(MPI_Win_copy_attr_function *copy_fn,
                                                       MPI_Win_delete_attr_function *delete_fn, void *extra_state)
{
	struct osc_win_keyval_s *ret =
		sctk_calloc(1, sizeof(struct osc_win_keyval_s));

	if (!ret)
	{
		mpc_common_debug_error("Failed to allocate a keyval");
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

/**@ingroup MPI_OSC_INTERNAL
 * @brief Allocates and initializes a new attribute
 *
 * @param[in] keyval    Unique identifier of the attribute
 * @param[in] value     Value stored in the attribute
 * @param[in] keyval_pl Keyval used by this attribute
 * @param[in] win       Window associated with the attribute
 *
 * @return              Allocated and initialized attribute
 */
static struct osc_win_attr_s * __osc_win_attr_init(int keyval, void *value,
                                                   struct osc_win_keyval_s *keyval_pl, mpc_mpi_win_t *win)
{
	struct osc_win_attr_s *ret = sctk_calloc(1, sizeof(struct osc_win_attr_s));

	if (ret == NULL)
	{
		mpc_common_debug_error("Failed to allocate an attribute");
		return ret;
	}

	ret->keyval = keyval;
	ret->value  = value;
	memcpy(&ret->keyval_pl, keyval_pl, sizeof(struct osc_win_keyval_s));
	ret->win = win;

	return ret;
}

int osc_win_attr_ht_release(struct mpc_common_hashtable *atht)
{
	void *pattr = NULL;

	int found = -1;

	do
	{
		found = -1;

		struct osc_win_attr_s *attr = NULL;
		MPC_HT_ITER(atht, pattr)

		if (found < 0)
		{
			attr = (struct osc_win_attr_s *)pattr;
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

int mpc_mpi_osc_win_set_attr(mpc_mpi_win_t *win, const int keyval, void *const attr_val)
{
	__osc_win_keyval_ht_init_once();

	if (!attr_val)
	{
		return MPI_ERR_ARG;
	}

	if (!win)
	{
		return MPI_ERR_WIN;
	}

	/* Check keyval */

	uint64_t per_rank_keyval = __osc_get_per_rank_keyval_key(keyval);

	struct osc_win_keyval_s *kv =
		(struct osc_win_keyval_s *)mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!kv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* If we are here ready to set */
	struct osc_win_attr_s *attr = (struct osc_win_attr_s *)mpc_common_hashtable_get(&win->attrs, keyval);

	if (!attr)
	{
		attr = __osc_win_attr_init(keyval, attr_val, kv, win);
		mpc_common_hashtable_set(&win->attrs, keyval, attr);
	}
	else
	{
		attr->value = attr_val;
	}

	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_get_attr(mpc_mpi_win_t *win, const int keyval, void *attr_val, int *flag)
{
	__osc_win_keyval_ht_init_once();

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

	uint64_t per_rank_keyval = __osc_get_per_rank_keyval_key(keyval);

	void *pkv = mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!pkv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* Now try to retrieve attr */
	struct osc_win_attr_s *attr = (struct osc_win_attr_s *)mpc_common_hashtable_get(&win->attrs, keyval);

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

	uint64_t per_rank_keyval = __osc_get_per_rank_keyval_key(keyval);

	struct osc_win_keyval_s *kv =
		(struct osc_win_keyval_s *)mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!kv)
	{
		return MPI_ERR_KEYVAL;
	}

	/* Now try to retrieve attr */
	struct osc_win_attr_s *attr = (struct osc_win_attr_s *)mpc_common_hashtable_get(&win->attrs, keyval);

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
	__osc_win_keyval_ht_init_once();

	struct osc_win_keyval_s *new = __osc_win_keyval_init(copy_fn, delete_fn, extra_state);

	if (!new)
	{
		return MPI_ERR_INTERN;
	}

	/* Now add to the ATTR HT */
	uint64_t per_rank_keyval = __osc_get_per_rank_keyval_key(new->keyval);

	mpc_common_hashtable_set(&__osc_win_keyval_ht, per_rank_keyval, new);

	/* SET the return keyval */
	*keyval = new->keyval;

	return MPI_SUCCESS;
}

int mpc_mpi_osc_win_free_keyval(const int keyval)
{
	__osc_win_keyval_ht_init_once();

	uint64_t per_rank_keyval = __osc_get_per_rank_keyval_key(keyval);

	void *pkv = mpc_common_hashtable_get(&__osc_win_keyval_ht, per_rank_keyval);

	if (!pkv)
	{
		return MPI_ERR_KEYVAL;
	}

	mpc_common_hashtable_delete(&__osc_win_keyval_ht, per_rank_keyval);
	sctk_free(pkv);

	return MPI_SUCCESS;
}
