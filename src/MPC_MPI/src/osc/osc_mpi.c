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
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdbool.h>

#include <mpc_common_progress.h>
#include <mpc_common_rank.h>
#include <mpc_lowcomm.h>
#include <mpc_lowcomm_one_sided.h>

#include "osc_mpi.h"
#include "mpitypes.h"
#include "osc_mpi_internal.h"
#include "mpc_mpi.h"

#include <sctk_alloc.h>

static void _min_op_long(const void *in, void *out, size_t length, mpc_lowcomm_datatype_t dt)
{
	UNUSED(dt);

	const long *const typed_in  = in;
	long *            typed_out = out;

	for (size_t i = 0; i < length; i++)
	{
		if (typed_out[i] > typed_in[i])
		{
			typed_out[i] = typed_in[i];
		}
	}
}

static int osc_win_setup_after_creation(mpc_mpi_win_t *win, const size_t size, int disp_unit,
                                        MPI_Info info, const int flavor)
{
	long values[2];

	/* Share displacement units. */
	values[0] = disp_unit;
	values[1] = -disp_unit;

	long tmp[2] = { values[0], values[1] };

	const mpc_lowcomm_communicator_t comm = mpc_lowcomm_osc_win_get_comm(win->win_module);
	const int comm_size = mpc_lowcomm_communicator_size(comm);
	const int comm_rank = mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank());

	mpc_lowcomm_allreduce(values, tmp, sizeof(*values), 2, _min_op_long, comm, MPC_LOWCOMM_LONG);

	values[0] = tmp[0];
	values[1] = tmp[1];

	if (values[0] == -values[1]) /* Every one has the same disp_unit, store only one. */
	{
		win->disp_unit = disp_unit;
	}
	else
	{
		win->disp_unit  = -1;
		win->disp_units = sctk_malloc(comm_size * sizeof(int));
		if (win->disp_units == NULL)
		{
			mpc_common_debug_error("WIN: could not allocated displacement table.");
			return MPI_ERR_INTERN;
		}

		win->disp_units[comm_rank] = disp_unit;

		mpc_lowcomm_allgather(&disp_unit, win->disp_units, sizeof(int), comm);
	}


	win->epoch.access   = NONE_EPOCH;
	win->epoch.exposure = NONE_EPOCH;
	win->flavor         = flavor;
	OPA_store_int(&win->lock_count, 0);
	win->info  = info;
	win->model = MPI_WIN_SEPARATE;
	win->size  = size;
	strcpy(win->win_name, "MPI_WIN_DEFAULT_NAME");
	mpc_common_hashtable_init(&win->attrs, 16);

	return MPI_SUCCESS;
}

/* Window Management */
int mpc_mpi_osc_win_create(void **base, size_t size, int disp_unit, MPI_Info info,
                           mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p)
{
	int rc = MPI_SUCCESS;

	mpc_mpi_win_t *win = *win_p = sctk_malloc(sizeof(mpc_mpi_win_t));

	if (win == NULL)
	{
		mpc_common_debug_error("Unable to allocate the required window");
		return MPI_ERR_INTERN;
	}
	memset(win, 0, sizeof(mpc_mpi_win_t));

	rc = mpc_lowcomm_window_create(base, size, comm,
		MPC_LOWCOMM_WINDOW_FLAG_CREATE, &(win->win_module));

	osc_win_setup_after_creation(win, size, disp_unit, info, MPI_WIN_FLAVOR_CREATE);

	return rc;
}

int mpc_mpi_osc_win_allocate(void **base, size_t size, int disp_unit, MPI_Info info,
                             mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p)
{
	int rc = MPI_SUCCESS;

	mpc_mpi_win_t *win = *win_p = sctk_malloc(sizeof(mpc_mpi_win_t));

	if (win == NULL)
	{
		mpc_common_debug_error("Unable to allocate the required window");
		return MPI_ERR_INTERN;
	}
	memset(win, 0, sizeof(mpc_mpi_win_t));

	rc = mpc_lowcomm_window_create(base, size, comm, MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE, &(win->win_module));

	osc_win_setup_after_creation(win, size, disp_unit, info, MPI_WIN_FLAVOR_ALLOCATE);

	return rc;
}

int mpc_mpi_osc_win_create_dynamic(MPI_Info info, mpc_lowcomm_communicator_t comm, mpc_mpi_win_t **win_p)
{
	int rc = MPI_SUCCESS;

	mpc_mpi_win_t *win = *win_p = sctk_malloc(sizeof(mpc_mpi_win_t));

	if (win == NULL)
	{
		mpc_common_debug_error("Unable to allocate the required window");
		return MPI_ERR_INTERN;
	}
	memset(win, 0, sizeof(mpc_mpi_win_t));

	rc = mpc_lowcomm_window_create(NULL, 0, comm,
		MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC, &(win->win_module));

	osc_win_setup_after_creation(win, 0, 1, info, MPI_WIN_FLAVOR_DYNAMIC);

	return rc;
}

int mpc_mpi_osc_win_attach(mpc_mpi_win_t *win, void *base, size_t len)
{
	int rc = MPI_SUCCESS;

	if (win->flavor != MPI_WIN_FLAVOR_DYNAMIC)
	{
		return MPI_ERR_RMA_FLAVOR;
	}

	rc = mpc_lowcomm_window_attach(win->win_module, base, len);

	return rc;
}

int mpc_mpi_osc_win_detach(mpc_mpi_win_t *win, const void *base)
{
	int rc = MPI_SUCCESS;

	if (win->flavor != MPI_WIN_FLAVOR_DYNAMIC)
	{
		return MPI_ERR_RMA_FLAVOR;
	}

	rc = mpc_lowcomm_window_detach(win->win_module, base);

	return rc;
}

int mpc_mpi_osc_win_free(mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	assert(OPA_load_int(&win->lock_count) == 0);

	mpc_mpi_osc_win_attr_ht_release(&win->attrs);

	rc = mpc_lowcomm_window_free(&win->win_module);

	/* Deallocate table of displacement units if not all processes had the
	 * same. */
	if (win->disp_unit == -1)
	{
		sctk_free(win->disp_units);
	}

	memset(win, 0, sizeof(mpc_mpi_win_t));

	return rc;
}

static int mpc_osc_discontig_common(const void *origin_addr, int origin_count,
                                    _mpc_lowcomm_general_datatype_t *origin_dt,
                                    int is_orig_dt_contig, const int target,
                                    uint64_t target_disp, int target_count,
                                    _mpc_lowcomm_general_datatype_t *target_dt,
                                    int is_target_dt_contig, bool is_get,
                                    mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *request);

static int mpc_osc_put_common(const void *origin_addr,
                              int origin_count,
                              _mpc_lowcomm_general_datatype_t *origin_dt,
                              int target,
                              ptrdiff_t target_disp,
                              int target_count,
                              _mpc_lowcomm_general_datatype_t *target_dt,
                              mpc_mpi_win_t *win,
                              mpc_lowcomm_request_t *request)
{
	int    rc = MPC_LOWCOMM_SUCCESS;
	size_t origin_len;
	int    is_orig_dt_contiguous   = 0;
	int    is_target_dt_contiguous = 0;
	long   origin_lb, target_lb;
	long   origin_extent, target_extent;

	if (!origin_count || !target_count)
	{
		return rc;
	}

	origin_dt = _mpc_dt_get_datatype(origin_dt);
	target_dt = _mpc_dt_get_datatype(target_dt);

	is_orig_dt_contiguous   = _mpc_dt_is_contig_mem(origin_dt);
	is_target_dt_contiguous = _mpc_dt_is_contig_mem(target_dt);

	_mpc_cl_type_get_true_extent(origin_dt, &origin_lb, &origin_extent);
	_mpc_cl_type_get_true_extent(target_dt, &target_lb, &target_extent);

	if (is_orig_dt_contiguous && is_target_dt_contiguous)
	{
		origin_len = origin_extent * origin_count;
		const ptrdiff_t new_target_disp = target_disp * OSC_GET_DISP(win, target) + target_lb;

		rc = mpc_lowcomm_iput(origin_addr + origin_lb, origin_len, target, new_target_disp, win->win_module, request);
	}
	else
	{
		rc = mpc_osc_discontig_common(
			origin_addr + origin_lb, origin_count, origin_dt, is_orig_dt_contiguous, target,
			target_disp * OSC_GET_DISP(win, target) + target_lb, target_count, target_dt, is_target_dt_contiguous,
			false, win->win_module, request);
	}

	return rc;
}

static int mpc_osc_get_common(void *origin_addr, int origin_count, _mpc_lowcomm_general_datatype_t *origin_dt,
                              int target, ptrdiff_t target_disp,
                              int target_count, _mpc_lowcomm_general_datatype_t *target_dt,
                              mpc_mpi_win_t *win, mpc_lowcomm_request_t *request)
{
	int    rc = MPC_SUCCESS;
	size_t origin_len;
	int    is_orig_dt_contiguous   = 0;
	int    is_target_dt_contiguous = 0;
	long   origin_lb, target_lb;
	long   origin_extent, target_extent;

	origin_dt = _mpc_dt_get_datatype(origin_dt);
	target_dt = _mpc_dt_get_datatype(target_dt);

	is_orig_dt_contiguous   = _mpc_dt_is_contig_mem(origin_dt);
	is_target_dt_contiguous = _mpc_dt_is_contig_mem(target_dt);

	_mpc_cl_type_get_true_extent(origin_dt, &origin_lb, &origin_extent);
	_mpc_cl_type_get_true_extent(target_dt, &target_lb, &target_extent);

	if (is_orig_dt_contiguous && is_target_dt_contiguous)
	{
		origin_len = origin_extent * origin_count;
		const ptrdiff_t new_target_disp = target_disp * OSC_GET_DISP(win, target) + target_lb;

		rc = mpc_lowcomm_iget(origin_addr + origin_lb, origin_len, target, new_target_disp, win->win_module, request);
	}
	else
	{
		rc = mpc_osc_discontig_common(
			origin_addr + origin_lb, origin_count, origin_dt, is_orig_dt_contiguous, target,
			target_disp * OSC_GET_DISP(win, target) + target_lb, target_count, target_dt, is_target_dt_contiguous,
			true, win->win_module, request);
	}
	return rc;
}

static int mpc_mpi_osc_create_dt_blocks(const void *typebuf, int count,
                                        mpc_lowcomm_datatype_t dt,
                                        int **blocks_p,
                                        intptr_t **disps_p,
                                        int *intblockct_p)
{
	int       rc = MPC_LOWCOMM_SUCCESS;
	size_t    tmp_size;
	int *     blocks;
	int       intblockct;
	intptr_t *disps;
	intptr_t  blockct;

	if (!MPIT_Type_is_initialized(dt))
	{
		MPIT_Type_init(dt);
	}

	rc = _mpc_cl_type_size(dt, &tmp_size);
	if (rc != MPI_SUCCESS)
	{
		goto err;
	}
	tmp_size *= count;

	rc = MPIT_Type_blockct(count, dt, 0, tmp_size, &blockct);
	if (rc != MPI_SUCCESS)
	{
		goto err;
	}

	disps = sctk_malloc(blockct * sizeof(MPI_Aint));
	if (disps == NULL)
	{
		mpc_common_debug_error("MPI OSC: could not allocate iovec displacements.");
		rc = MPI_ERR_INTERN;
		goto err;
	}

	blocks = sctk_malloc(blockct * sizeof(int));
	if (blocks == NULL)
	{
		mpc_common_debug_error("MPI OSC: could not allocate iovec blocks.");
		rc = MPI_ERR_INTERN;
		goto err;
	}

	intblockct = blockct;
	rc         = MPIT_Type_flatten((void *)typebuf, count, dt, 0, tmp_size, disps, blocks, &intblockct);
	if (rc != MPI_SUCCESS)
	{
		goto err;
	}
	assert(intblockct == blockct);

	*blocks_p     = blocks;
	*disps_p      = disps;
	*intblockct_p = intblockct;
err:
	return rc;
}

static int mpc_osc_discontig_common(const void *origin_addr, int origin_count,
                                    _mpc_lowcomm_general_datatype_t *origin_dt,
                                    int is_orig_dt_contig, const int target,
                                    uint64_t target_disp, int target_count,
                                    _mpc_lowcomm_general_datatype_t *target_dt,
                                    int is_target_dt_contig, bool is_get,
                                    mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *request)
{
	int       rc = MPI_SUCCESS;
	int *     orig_blocks, *target_blocks;
	MPI_Aint *orig_disps, *target_disps;
	int       orig_intblockct, target_intblockct;
	int       orig_blockidx = 0, target_blockidx = 0;

	if (!is_orig_dt_contig)
	{
		rc = mpc_mpi_osc_create_dt_blocks(origin_addr, origin_count, origin_dt,
			&orig_blocks, &orig_disps, &orig_intblockct);
		if (rc != MPI_SUCCESS)
		{
			goto err;
		}
	}

	if (!is_target_dt_contig)
	{
		rc = mpc_mpi_osc_create_dt_blocks((void *)target_disp, target_count,
			target_dt, &target_blocks, &target_disps, &target_intblockct);
		if (rc != MPI_SUCCESS)
		{
			goto err;
		}
	}

	// FIXME: We should definitely make use of iovec to try bundle the comms
	if (!is_orig_dt_contig && !is_target_dt_contig)
	{
		size_t curr_len;
		while (orig_blockidx < orig_intblockct)
		{
			curr_len = mpc_common_min(orig_blocks[orig_blockidx], target_blocks[target_blockidx]);

			if (is_get)
			{
				rc = mpc_lowcomm_iget((void *)orig_disps[orig_blockidx],
					curr_len, target, target_disps[target_blockidx], win, request);
			}
			else
			{
				rc = mpc_lowcomm_iput((void *)orig_disps[orig_blockidx],
					curr_len, target, target_disps[target_blockidx], win, request);
			}
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			orig_disps[orig_blockidx]  += curr_len;
			orig_blocks[orig_blockidx] -= curr_len;
			if (orig_blocks[orig_blockidx] == 0)
			{
				orig_blockidx++;
			}
			target_disps[target_blockidx]  += curr_len;
			target_blocks[target_blockidx] -= curr_len;
			if (target_blocks[target_blockidx] == 0)
			{
				target_blockidx++;
			}
		}
	}
	else if (!is_orig_dt_contig)
	{
		size_t curr_len = 0;
		while (orig_blockidx < orig_intblockct)
		{
			if (is_get)
			{
				rc = mpc_lowcomm_iget((void *)orig_disps[orig_blockidx],
					orig_blocks[orig_blockidx], target, target_disp + curr_len, win, request);
			}
			else
			{
				rc = mpc_lowcomm_iput((void *)orig_disps[orig_blockidx],
					orig_blocks[orig_blockidx], target, target_disp + curr_len, win, request);
			}
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			curr_len += orig_blocks[orig_blockidx];
			orig_blockidx++;
		}
	}
	else
	{
		size_t curr_len = 0;
		while (target_blockidx < target_intblockct)
		{
			if (is_get)
			{
				rc = mpc_lowcomm_iget((char *)origin_addr + curr_len,
					target_blocks[target_blockidx], target, target_disps[target_blockidx], win, request);
			}
			else
			{
				rc = mpc_lowcomm_iput((char *)origin_addr + curr_len,
					target_blocks[target_blockidx], target, target_disps[target_blockidx], win, request);
			}
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			curr_len += target_blocks[target_blockidx];
			target_blockidx++;
		}
	}

err:
	return rc;
}

int mpc_mpi_osc_put(const void *origin_addr, int origin_count,
                    _mpc_lowcomm_general_datatype_t *origin_dt,
                    int target, ptrdiff_t target_disp, int target_count,
                    _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win)
{
	return mpc_osc_put_common(origin_addr, origin_count, origin_dt, target,
		target_disp, target_count, target_dt, win, &MPC_REQUEST_NULL);
}

int mpc_mpi_osc_rput(const void *origin_addr,
                     int origin_count,
                     _mpc_lowcomm_general_datatype_t *origin_dt,
                     int target,
                     ptrdiff_t target_disp,
                     int target_count,
                     _mpc_lowcomm_general_datatype_t *target_dt,
                     mpc_mpi_win_t *win,
                     MPI_Request *request)
{
	int rc = MPI_SUCCESS;

	*request = MPI_REQUEST_NULL;

	MPI_internal_request_t *req         = *request = mpc_mpi_alloc_request();
	mpc_lowcomm_request_t * lowcomm_req = _mpc_cl_get_lowcomm_request(req);

	// We do not need to do the comm if target == MPC_PROC_NULL but we have to complete the request
	if (target != MPC_PROC_NULL)
	{
		rc = mpc_osc_put_common(origin_addr, origin_count, origin_dt, target,
			target_disp, target_count, target_dt, win, lowcomm_req);
	}
	else
	{
		mpc_lowcomm_osc_request_complete(MPI_SUCCESS, lowcomm_req, origin_count);
	}

	return rc;
}

int mpc_mpi_osc_get(void *origin_addr, int origin_count,
                    _mpc_lowcomm_general_datatype_t *origin_dt,
                    int target, ptrdiff_t target_disp, int target_count,
                    _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win)
{
	return mpc_osc_get_common(origin_addr, origin_count, origin_dt, target,
		target_disp, target_count, target_dt, win, &MPC_REQUEST_NULL);
}

int mpc_mpi_osc_rget(void *origin_addr, int origin_count,
                     _mpc_lowcomm_general_datatype_t *origin_dt,
                     int target, ptrdiff_t target_disp, int target_count,
                     _mpc_lowcomm_general_datatype_t *target_dt, mpc_mpi_win_t *win,
                     MPI_Request *request)
{
	int rc = MPI_SUCCESS;

	*request = MPI_REQUEST_NULL;

	MPI_internal_request_t *req         = *request = mpc_mpi_alloc_request();
	mpc_lowcomm_request_t * lowcomm_req = _mpc_cl_get_lowcomm_request(req);

	// We do not need to do the comm if target == MPC_PROC_NULL but we have to complete the request
	if (target != MPC_PROC_NULL)
	{
		rc = mpc_osc_get_common(origin_addr, origin_count, origin_dt, target,
			target_disp, target_count, target_dt, win, lowcomm_req);
	}
	else
	{
		mpc_lowcomm_osc_request_complete(MPI_SUCCESS, lowcomm_req, origin_count);
	}

	return rc;
}

int mpc_mpi_osc_accumulate(const void *origin_addr, int origin_count,
                           _mpc_lowcomm_general_datatype_t *origin_dt,
                           int target, ptrdiff_t target_disp, int target_count,
                           _mpc_lowcomm_general_datatype_t *target_dt,
                           MPI_Op op, mpc_mpi_win_t *win)
{
	int   rc = MPI_SUCCESS;
	void *tmp_result_addr = NULL;

	// TODO: check sync state

	if (op == MPI_NO_OP || origin_addr == NULL || origin_count == 0 || target_count == 0)
	{
		return rc;
	}
	bool need_to_unlock = false;

	rc = mpc_lowcomm_osc_lock(MPC_LOWCOMM_WINDOW_LOCK_ATOMIC, win->win_module, target, false);
	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		need_to_unlock = true;
	}


	if (op == MPI_REPLACE)
	{
		rc = mpc_mpi_osc_put(origin_addr, origin_count, origin_dt, target,
			target_disp, target_count, target_dt, win);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}
	}
	else
	{
		mpc_lowcomm_datatype_t tmp_dt;
		int    tmp_dt_count;
		size_t tmp_len, tmp_dt_size;
		long   tmp_lb, tmp_extent;
		int    is_orig_dt_contiguous = mpc_mpi_cl_type_is_contiguous(origin_dt);

		if (mpc_lowcomm_datatype_is_common(target_dt))
		{
			tmp_dt       = target_dt;
			tmp_dt_count = target_count;
		}
		else
		{
			rc = _mpc_cl_type_get_primitive_type_info(target_dt, &tmp_dt, &tmp_dt_count);
			if (rc != MPI_SUCCESS)
			{
				mpc_common_debug_error("MPI OSC: no single primitive type in acc");
				goto unlock;
			}
			tmp_dt_count *= target_count;
		}
		rc = _mpc_cl_type_get_true_extent(tmp_dt, &tmp_lb, &tmp_extent);
		if (rc != MPC_SUCCESS)
		{
			goto unlock;
		}
		tmp_len = tmp_dt_count * tmp_extent;

		tmp_result_addr = sctk_malloc(tmp_len);
		if (tmp_result_addr == NULL)
		{
			mpc_common_debug_error("MPI OSC: could not allocate accumulate temp buffer.");
			goto unlock;
		}

		rc = mpc_mpi_osc_get(tmp_result_addr, tmp_dt_count, tmp_dt, target,
			target_disp, target_count, target_dt, win);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}

		mpc_lowcomm_request_t *lowcomm_req = _mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());
		mpc_lowcomm_status_t   lowcomm_status;

		rc = mpc_lowcomm_iflush(target, win->win_module, lowcomm_req);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}

		rc = mpc_lowcomm_wait(lowcomm_req, &lowcomm_status);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}

		mpc_lowcomm_request_free(lowcomm_req);

		rc = lowcomm_status.MPC_ERROR;
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}

		/* Get mpc operation and perform the reduction. */
		sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
		sctk_Op_f  func   = sctk_get_common_function(tmp_dt, mpc_op->op);

		if (is_orig_dt_contiguous)
		{
			func((void *)origin_addr, tmp_result_addr, tmp_dt_count, tmp_dt);
		}
		else
		{
			int *     blocks;
			int       intblockct;
			MPI_Aint *disps;
			int       orig_blockidx = 0;

			// NOTE: We want to retrieve the layout of the datatype
			rc = mpc_mpi_osc_create_dt_blocks(origin_addr, origin_count, origin_dt, &blocks, &disps, &intblockct);
			if (rc != MPI_SUCCESS)
			{
				goto unlock;
			}

			if (mpc_mpi_cl_type_is_contiguous(tmp_dt))
			{
				_mpc_cl_type_size(tmp_dt, &tmp_dt_size);

				char *curr_tmp_addr = tmp_result_addr;
				while (orig_blockidx < intblockct)
				{
					func((void *)disps[orig_blockidx],
						curr_tmp_addr,
						blocks[orig_blockidx] / tmp_dt_size,
						tmp_dt);

					curr_tmp_addr += blocks[orig_blockidx];
					orig_blockidx++;
				}
			}
			else
			{
				mpc_common_debug_error("MPI OSC: non contiguous predefined datatype not implemented.");
				not_implemented();
			}
		}

		rc = mpc_mpi_osc_put(tmp_result_addr, tmp_dt_count, tmp_dt, target, target_disp, target_count, target_dt, win);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}
	}

	mpc_lowcomm_request_t *lowcomm_req = _mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());
	mpc_lowcomm_status_t   lowcomm_status;

	rc = mpc_lowcomm_iflush(target, win->win_module, lowcomm_req);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

	rc = mpc_lowcomm_wait(lowcomm_req, &lowcomm_status);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}
	mpc_lowcomm_request_free(lowcomm_req);


	rc = lowcomm_status.MPC_ERROR;
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

unlock:
	if (tmp_result_addr)
	{
		sctk_free(tmp_result_addr);
	}
	if ((need_to_unlock))
	{
		mpc_lowcomm_osc_unlock(win->win_module, target, true);
	}

	return rc;
}

int mpc_mpi_osc_raccumulate(const void *origin_addr, int origin_count,
                            _mpc_lowcomm_general_datatype_t *origin_dt,
                            int target, ptrdiff_t target_disp, int target_count,
                            _mpc_lowcomm_general_datatype_t *target_dt, MPI_Op op,
                            mpc_mpi_win_t *win, MPI_Request *request)
{
	int rc = MPI_SUCCESS;

	MPI_internal_request_t *mpi_request     = *request = mpc_mpi_alloc_request();
	mpc_lowcomm_request_t * lowcomm_request = _mpc_cl_get_lowcomm_request(mpi_request);

	mpc_lowcomm_request_init(lowcomm_request, 1, NULL, NULL, 0);
	mpc_lowcomm_request_set_null(lowcomm_request, 0);
	lowcomm_request->request_type    = REQUEST_SEND;
	lowcomm_request->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;


	// We do not need to do the comm if target == MPC_PROC_NULL but we still need to complete the request
	if (target != MPC_PROC_NULL)
	{
		rc = mpc_mpi_osc_accumulate(origin_addr, origin_count, origin_dt, target,
			target_disp, target_count, target_dt, op, win);
	}
	mpc_lowcomm_osc_request_complete(rc, lowcomm_request, origin_count);

	return rc;
}

int mpc_mpi_osc_get_accumulate(const void *origin_addr, int origin_count,
                               _mpc_lowcomm_general_datatype_t *origin_dt,
                               void *result_addr, int result_count,
                               _mpc_lowcomm_general_datatype_t *result_dt,
                               int target, ptrdiff_t target_disp,
                               int target_count, _mpc_lowcomm_general_datatype_t *target_dt,
                               MPI_Op op, mpc_mpi_win_t *win)
{
	int   rc = MPI_SUCCESS;
	void *tmp_result_addr = NULL;

	bool need_to_unlock = false;

	rc = mpc_lowcomm_osc_lock(MPC_LOWCOMM_WINDOW_LOCK_ATOMIC, win->win_module, target, false);
	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		need_to_unlock = true;
	}

	rc = mpc_mpi_osc_get(result_addr, result_count, result_dt, target, target_disp, target_count, target_dt, win);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

	/* Make sure the previous get operation is completed and data is arrived at destination. */

	mpc_lowcomm_request_t *lowcomm_req = _mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());
	mpc_lowcomm_status_t   lowcomm_status;

	rc = mpc_lowcomm_iflush(target, win->win_module, lowcomm_req);

	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

	rc = mpc_lowcomm_wait(lowcomm_req, &lowcomm_status);

	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

	mpc_lowcomm_request_free(lowcomm_req);

	rc = lowcomm_status.MPC_ERROR;

	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}

	if (op != MPI_NO_OP)
	{
		if (op == MPI_REPLACE)
		{
			rc = mpc_mpi_osc_put(origin_addr, origin_count, origin_dt, target, target_disp, target_count, target_dt,
				win);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto unlock;
			}
		}
		else
		{
			mpc_lowcomm_datatype_t tmp_dt;
			int    tmp_dt_count;
			size_t tmp_len, tmp_dt_size;
			long   tmp_lb, tmp_extent;
			int    is_orig_dt_contiguous = mpc_lowcomm_datatype_is_common(origin_dt)
			                               || mpc_mpi_cl_type_is_contiguous(origin_dt);

			if (mpc_lowcomm_datatype_is_common_predefined(target_dt))
			{
				tmp_dt       = target_dt;
				tmp_dt_count = target_count;
			}
			else
			{
				rc = _mpc_cl_type_get_primitive_type_info(target_dt, &tmp_dt, &tmp_dt_count);
				if (rc != MPI_SUCCESS)
				{
					mpc_common_debug_error("MPI OSC: no single primitive type in acc");
					goto unlock;
				}
				tmp_dt_count *= target_count;
			}
			rc = _mpc_cl_type_get_true_extent(tmp_dt, &tmp_lb, &tmp_extent);
			if (rc != MPC_SUCCESS)
			{
				goto unlock;
			}
			tmp_len = tmp_dt_count * tmp_extent;

			tmp_result_addr = sctk_malloc(tmp_len);
			if (tmp_result_addr == NULL)
			{
				mpc_common_debug_error("MPI OSC: could not allocate accumulate temp buffer.");
				goto unlock;
			}

			memcpy(tmp_result_addr, result_addr, tmp_len);

			/* Get mpc operation and perform the reduction. */
			sctk_op_t *mpc_op = sctk_convert_to_mpc_op(op);
			sctk_Op_f  func   = sctk_get_common_function(tmp_dt, mpc_op->op);
			if (is_orig_dt_contiguous)
			{
				func((void *)origin_addr, tmp_result_addr, tmp_dt_count, tmp_dt);
			}
			else
			{
				int *     blocks;
				int       intblockct;
				MPI_Aint *disps;
				int       orig_blockidx = 0;

				rc = mpc_mpi_osc_create_dt_blocks(origin_addr, origin_count, origin_dt, &blocks, &disps, &intblockct);
				if (rc != MPI_SUCCESS)
				{
					goto unlock;
				}

				if (mpc_mpi_cl_type_is_contiguous(tmp_dt))
				{
					_mpc_cl_type_size(tmp_dt, &tmp_dt_size);

					char *curr_tmp_addr = tmp_result_addr;
					while (orig_blockidx < intblockct)
					{
						func((void *)disps[orig_blockidx], curr_tmp_addr,
							blocks[orig_blockidx] / tmp_dt_size,
							tmp_dt);

						curr_tmp_addr += blocks[orig_blockidx];
						orig_blockidx++;
					}
				}
				else
				{
					mpc_common_debug_error("MPI OSC: non contiguous predefined datatype not implemented.");
					not_implemented();
				}
			}

			rc = mpc_mpi_osc_put(tmp_result_addr, tmp_dt_count,
				tmp_dt, target, target_disp,
				target_count, target_dt, win);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto unlock;
			}
		}

		lowcomm_req = _mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());

		rc = mpc_lowcomm_iflush(target, win->win_module, lowcomm_req);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}

		rc = mpc_lowcomm_wait(lowcomm_req, &lowcomm_status);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}
		mpc_lowcomm_request_free(lowcomm_req);

		rc = lowcomm_status.MPC_ERROR;
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unlock;
		}
	}

unlock:
	if (tmp_result_addr)
	{
		sctk_free(tmp_result_addr);
	}

	if ((need_to_unlock))
	{
		mpc_lowcomm_osc_unlock(win->win_module, target, true);
	}

	return rc;
}

int mpc_mpi_osc_rget_accumulate(const void *origin_addr, int origin_count,
                                _mpc_lowcomm_general_datatype_t *origin_datatype,
                                void *result_addr, int result_count,
                                _mpc_lowcomm_general_datatype_t *result_datatype,
                                int target, ptrdiff_t target_disp, int target_count,
                                _mpc_lowcomm_general_datatype_t *target_datatype,
                                MPI_Op op, mpc_mpi_win_t *win,
                                MPI_Request *request)
{
	int rc = MPI_SUCCESS;

	MPI_internal_request_t *mpi_request     = *request = mpc_mpi_alloc_request();
	mpc_lowcomm_request_t * lowcomm_request = _mpc_cl_get_lowcomm_request(mpi_request);

	// We do not need to do the comm if target == MPC_PROC_NULL but we still need to complete the request
	if (target != MPC_PROC_NULL)
	{
		rc = mpc_mpi_osc_get_accumulate(origin_addr, origin_count, origin_datatype,
			result_addr, result_count, result_datatype,
			target, target_disp, target_count,
			target_datatype, op, win);
	}

	mpc_lowcomm_osc_request_complete(rc, lowcomm_request, origin_count);

	return rc;
}

int mpc_mpi_osc_compare_and_swap(const void *origin_addr, const void *compare_addr,
                                 void *result_addr, _mpc_lowcomm_general_datatype_t *dt,
                                 int target, ptrdiff_t target_disp,
                                 mpc_mpi_win_t *win)
{
	mpc_lowcomm_request_t *req = _mpc_cl_get_lowcomm_request(mpc_mpi_alloc_request());
	const bool             dt_is_atomic_compatible = mpc_lowcomm_atomic_dt_is_compatible(dt);

	if ((!dt_is_atomic_compatible))
	{
		return MPI_ERR_TYPE;
	}

	const ptrdiff_t new_disp = target_disp * OSC_GET_DISP(win, target);

	mpc_lowcomm_iatomic(origin_addr, result_addr, compare_addr, dt, target, new_disp,
		LOWCOMM_ATOMIC_OP_CSWAP, win->win_module, req);

	// HACK: We have to wait atomic as the AM are not flushable
	mpc_lowcomm_status_t lowcomm_status = {};
	mpc_lowcomm_wait(req, &lowcomm_status);
	mpc_lowcomm_request_free(req);
	return lowcomm_status.MPC_ERROR;
}

int mpc_mpi_osc_fetch_and_op(const void *origin_addr, void *result_addr,
                             _mpc_lowcomm_general_datatype_t *dt, int target,
                             ptrdiff_t target_disp, MPI_Op op,
                             mpc_mpi_win_t *win)
{
	static const uint64_t no_op_value = 0ull;
	int         op_code    = -1;
	const void *new_origin = origin_addr;

	if (op < MPI_OP_NULL || op > MPI_NO_OP)
	{
		return MPI_ERR_OP;
	}
	// FIXME: Other datatypes not supported for now
	if (mpc_lowcomm_datatype_is_common_predefined(dt) == false)
	{
		return MPI_ERR_TYPE;
	}
	const bool dt_is_atomic_compatible = mpc_lowcomm_atomic_dt_is_compatible(dt);

	if ((dt_is_atomic_compatible))
	{
		switch (op)
		{
		case MPI_SUM:
			op_code = LOWCOMM_ATOMIC_OP_FETCH_ADD;
			break;

		case MPI_BAND:
			op_code = LOWCOMM_ATOMIC_OP_FETCH_AND;
			break;

		case MPI_BOR:
			op_code = LOWCOMM_ATOMIC_OP_FETCH_OR;
			break;

		case MPI_BXOR:
			op_code = LOWCOMM_ATOMIC_OP_FETCH_XOR;
			break;

		case MPI_REPLACE:
			op_code = LOWCOMM_ATOMIC_OP_SWAP;
			break;

		case MPI_NO_OP:
			op_code    = LOWCOMM_ATOMIC_OP_FETCH_ADD;
			new_origin = &no_op_value;
			break;

		default:
			op_code = -1;
			break;
		}
	}


	if (op_code != -1)
	{
		mpc_lowcomm_request_t * req            = _mpc_cl_get_lowcomm_request(mpc_mpi_alloc_request());
		mpc_lowcomm_atomic_op_t atomic_op_code = (mpc_lowcomm_atomic_op_t)op_code;
		const ptrdiff_t         new_disp       = target_disp * OSC_GET_DISP(win, target);

		mpc_lowcomm_iatomic(new_origin, result_addr, NULL, dt, target, new_disp,
			atomic_op_code, win->win_module, req);

		// HACK: We have to wait atomic as the AM are not flushable
		mpc_lowcomm_status_t lowcomm_status = {};
		mpc_lowcomm_wait(req, &lowcomm_status);
		mpc_lowcomm_request_free(req);
		return lowcomm_status.MPC_ERROR;
	}
	else
	{
		return mpc_mpi_osc_get_accumulate(origin_addr, 1, dt, result_addr, 1, dt,
			target, target_disp, 1, dt, op, win);
	}
}

/* Passive Target */
int mpc_mpi_osc_flush(int target, mpc_mpi_win_t *win)
{
	if (win->epoch.access != PASSIVE_ALL_EPOCH && win->epoch.access != PASSIVE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	int rc = MPI_SUCCESS;
	mpc_lowcomm_request_t *lowcomm_req =
		(mpc_lowcomm_request_t *)_mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());

	rc = mpc_lowcomm_iflush(target, win->win_module, lowcomm_req);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		return MPI_ERR_INTERN;
	}

	rc = mpc_lowcomm_wait(lowcomm_req, MPC_LOWCOMM_STATUS_NULL);

	mpc_lowcomm_request_free(lowcomm_req);

	return rc;
}

int mpc_mpi_osc_flush_all(mpc_mpi_win_t *win)
{
	if (win->epoch.access != PASSIVE_ALL_EPOCH && win->epoch.access != PASSIVE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	int rc = MPI_SUCCESS;
	mpc_lowcomm_request_t *lowcomm_req =
		(mpc_lowcomm_request_t *)_mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());

	rc = mpc_lowcomm_iflush(MPC_ANY_SOURCE, win->win_module, lowcomm_req);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		return MPI_ERR_INTERN;
	}

	rc = mpc_lowcomm_wait(lowcomm_req, MPC_LOWCOMM_STATUS_NULL);

	mpc_lowcomm_request_free(lowcomm_req);

	return rc;
}

int mpc_mpi_osc_sync(mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	mpc_common_progress();
	if (win->epoch.access != PASSIVE_ALL_EPOCH && win->epoch.access != PASSIVE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}
	mpc_lowcomm_request_t *lowcomm_req =
		(mpc_lowcomm_request_t *)_mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());

	rc = mpc_lowcomm_iflush(MPC_ANY_SOURCE, win->win_module, lowcomm_req);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		return MPI_ERR_INTERN;
	}

	rc = mpc_lowcomm_wait(lowcomm_req, MPC_LOWCOMM_STATUS_NULL);

	mpc_lowcomm_request_free(lowcomm_req);

	return rc;
}

int mpc_mpi_osc_lock(int lock_type, int target, int mpi_assert, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (OPA_load_int(&win->lock_count) == 0)
	{
		if (win->epoch.access != NONE_EPOCH && win->epoch.access != PASSIVE_EPOCH)
		{
			return MPI_ERR_RMA_SYNC;
		}
	}

	const bool is_no_check = ((mpi_assert & MPI_MODE_NOCHECK) != 0);

	mpc_lowcomm_osc_lock_type_t lowcomm_lock;
	switch (lock_type)
	{
	case MPI_LOCK_EXCLUSIVE:
		lowcomm_lock = MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE;
		break;

	case MPI_LOCK_SHARED:
		lowcomm_lock = MPC_LOWCOMM_WINDOW_LOCK_SHARED;
		break;

	default:
		return MPI_ERR_LOCKTYPE;
	}

	rc = mpc_lowcomm_osc_lock(lowcomm_lock, win->win_module, target, is_no_check);
	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		if (OPA_fetch_and_incr_int(&win->lock_count) == 0)
		{
			win->epoch.access = PASSIVE_EPOCH;
		}
	}

	return rc;
}

int mpc_mpi_osc_unlock(int target, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (OPA_load_int(&win->lock_count) == 0)
	{
		return MPI_ERR_RMA_SYNC;
	}
	else if (win->epoch.access != PASSIVE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	rc = mpc_lowcomm_osc_unlock(win->win_module, target, false);
	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		if (OPA_fetch_and_decr_int(&win->lock_count) == 1)
		{
			win->epoch.access = NONE_EPOCH;
		}
	}

	return rc;
}

int mpc_mpi_osc_lock_all(int mpi_assert, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.access != NONE_EPOCH && win->epoch.access != PASSIVE_ALL_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}
	win->epoch.access = PASSIVE_ALL_EPOCH;

	const bool is_no_check = ((mpi_assert & MPI_MODE_NOCHECK) != 0);

	rc = mpc_lowcomm_osc_lock_all(win->win_module, is_no_check);

	return rc;
}

int mpc_mpi_osc_unlock_all(mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.access != PASSIVE_ALL_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}
	win->epoch.access = PASSIVE_ALL_EPOCH;

	rc = mpc_lowcomm_osc_unlock_all(win->win_module);

	return rc;
}

/* Active Target */
int mpc_mpi_osc_fence(int mpi_assert, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.access != NONE_EPOCH && win->epoch.access != FENCE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	if (mpi_assert & MPI_MODE_NOSUCCEED)
	{
		win->epoch.access = NONE_EPOCH;
	}
	else
	{
		win->epoch.access = FENCE_EPOCH;
	}

	if (!(mpi_assert & MPI_MODE_NOPRECEDE))
	{
		mpc_lowcomm_request_t *lowcomm_req =
			(mpc_lowcomm_request_t *)_mpc_cl_get_lowcomm_request(mpc_lowcomm_request_alloc());

		rc = mpc_lowcomm_iflush(MPC_ANY_SOURCE, win->win_module, lowcomm_req);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			return MPI_ERR_INTERN;
		}

		rc = mpc_lowcomm_wait(lowcomm_req, MPC_LOWCOMM_STATUS_NULL);

		mpc_lowcomm_request_free(lowcomm_req);
	}

	mpc_lowcomm_communicator_t comm = mpc_lowcomm_osc_win_get_comm(win->win_module);

	PMPI_Barrier(comm);

	return rc;
}

int mpc_mpi_osc_start(mpc_lowcomm_group_t *group, int mpi_assert, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.access != NONE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	const bool is_no_check = ((mpi_assert & MPI_MODE_NOCHECK) != 0);

	rc = mpc_lowcomm_osc_start(group, win->win_module, is_no_check);

	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		win->epoch.access = START_COMPLETE_EPOCH;
	}

	return rc;
}

int mpc_mpi_osc_complete(mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.access != START_COMPLETE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	rc = mpc_lowcomm_osc_complete(win->win_module);

	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		win->epoch.access = NONE_EPOCH;
	}

	return rc;
}

int mpc_mpi_osc_post(mpc_lowcomm_group_t *group, int mpi_assert, mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.exposure != NONE_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	const bool is_no_check = ((mpi_assert & MPI_MODE_NOCHECK) != 0);

	rc = mpc_lowcomm_osc_post(group, win->win_module, is_no_check);

	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		win->epoch.exposure = POST_WAIT_EPOCH;
	}

	return rc;
}

int mpc_mpi_osc_wait(mpc_mpi_win_t *win)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.exposure != POST_WAIT_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	rc = mpc_lowcomm_osc_wait(win->win_module);

	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		win->epoch.exposure = NONE_EPOCH;
	}

	return rc;
}

int mpc_mpi_osc_test(mpc_mpi_win_t *win, int *flag)
{
	int rc = MPI_SUCCESS;

	if (win->epoch.exposure != POST_WAIT_EPOCH)
	{
		return MPI_ERR_RMA_SYNC;
	}

	bool is_finalized = false;

	rc = mpc_lowcomm_osc_test(win->win_module, &is_finalized);

	if (rc == MPC_LOWCOMM_SUCCESS && is_finalized == true)
	{
		win->epoch.exposure = NONE_EPOCH;
		*flag = 1;
	}

	return rc;
}

/* Special */
MPI_Comm mpc_mpi_osc_win_get_comm(const mpc_mpi_win_t *const win)
{
	return (MPI_Comm)mpc_lowcomm_osc_win_get_comm(win->win_module);
}

MPI_Group mpc_mpi_osc_win_get_group(const mpc_mpi_win_t *const win)
{
	return (MPI_Group)mpc_lowcomm_osc_win_get_group(win->win_module);
}
