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
/* #                                                                      # */
/* ######################################################################## */

#include "lcp.h"
#include "mpc_lowcomm_communicator.h"
#include "mpc_mpi.h"
#include "osc_mpi.h"

#include <assert.h>

#include "osc_win_attr.h"
#include <sctk_alloc.h>
#include <communicator.h> //FIXME: is this good include?
#include <mpc_common_progress.h>

extern lcp_context_h lcp_ctx_loc;
static lcp_manager_h osc_mngr = NULL;
static mpc_common_spinlock_t osc_win_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

int mpc_osc_progress() 
{
        return lcp_progress(osc_mngr);
}

void mpc_win_get_attr(mpc_win_t *win, int win_keyval, void *attr_val, 
                     int *flag)
{
	switch(win_keyval)
	{
		case MPI_WIN_BASE:
			*flag = 1;
			*( (void **)attr_val) = win->win_module.base_data;
                        break;

		case MPI_WIN_SIZE:
			*flag = 1;
			*( (void **)attr_val) = (void *)&win->size;
                        break;

		case MPI_WIN_DISP_UNIT:
			*flag = 1;
                        if (win->win_module.disp_unit == -1) {
                                *( (void **)attr_val) = (void *)&win->win_module.disp_units[win->comm_rank];
                        } else {
                                *( (void **)attr_val) = (void *)&win->win_module.disp_unit;
                        }
                        break;

		case MPI_WIN_CREATE_FLAVOR:
			*flag = 1;
			*( (void **)attr_val) = (void *)&win->flavor;
                        break;

		case MPI_WIN_MODEL:
			*flag = 1;
			*( (void **)attr_val) = (void *)&win->model;
                        break;
		default:
			assume("Unreachable");
                        break;
	}

}

static int _win_setup(mpc_win_t *win, void **base, size_t size,
                      int disp_unit, mpc_lowcomm_communicator_t comm,
                      MPI_Info info, int flavor)
{
        UNUSED(info);
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        long values[2];
        lcp_mem_h w_mem_data, w_mem_state;
        lcp_mem_attr_t attr;
        void *win_info = NULL;
        size_t win_info_len;
        int win_info_offset = 0;
        void *key_data_buf = NULL, *key_state_buf = NULL;
        int key_data_len, key_state_len;
        int rkey_data_len, rkey_state_len;
        int *win_info_lengths = NULL;
        int *win_info_disps = NULL;
        int total_win_info_lengths;
        char *win_info_buf = NULL;

        if (flavor == MPI_WIN_FLAVOR_SHARED) {
                rc = -1;
                goto err;
        }

        /* Share displacement units. */
        values[0] = disp_unit;
        values[1] = -disp_unit;

        PMPI_Allreduce(MPI_IN_PLACE, values, 2, MPI_LONG, MPI_MIN, comm); 

        if (values[0] == -values[1]) { /* Every one has the same disp_unit, store only one. */
                module->disp_unit = disp_unit;
        } else {
                module->disp_unit = -1;
                module->disp_units = sctk_malloc(win->comm_size * sizeof(int));
                if (module->disp_units == NULL) {
                        mpc_common_debug_error("WIN: could not allocated displacement "
                                               "table.");
                        rc = -1;
                        goto err;
                }

                module->disp_units[win->comm_rank] = disp_unit;

                PMPI_Allgather(&disp_unit, 1, MPI_INT, 
                               module->disp_units, 1, MPI_INT,
                               comm);
        }

        /* Register memory according to flavor. */ 
        if (flavor == MPI_WIN_FLAVOR_CREATE) {
                lcp_mem_param_t param = {
                        .field_mask = LCP_MEM_ADDR_FIELD |
                                LCP_MEM_SIZE_FIELD,
                        .flags   = LCP_MEM_REGISTER_CREATE |
                                LCP_MEM_REGISTER_STATIC,
                        .address = *base,
                        .size    = size
                };

                rc = lcp_mem_provision(module->mngr, &w_mem_data, &param); 
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI OSC: could not provision "
                                               "created data memory.");
                        goto err;
                }
                module->lkey_data = w_mem_data;
                
                attr.field_mask = LCP_MEM_ADDR_FIELD |
                        LCP_MEM_SIZE_FIELD;
                rc = lcp_mem_query(w_mem_data, &attr);

                assert(attr.size >= size && attr.address == *base);
                module->base_data = *base;
        } else if (flavor == MPI_WIN_FLAVOR_ALLOCATE) {
                /* Memory is already allocated, just register it. */ 
                lcp_mem_param_t param = {
                        .field_mask = LCP_MEM_ADDR_FIELD |
                                LCP_MEM_SIZE_FIELD,
                        .flags   = LCP_MEM_REGISTER_ALLOCATE |
                                LCP_MEM_REGISTER_STATIC,
                        .address = NULL,
                        .size    = size
                };

                rc = lcp_mem_provision(module->mngr, &w_mem_data, &param); 
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("MPI OSC: could not provision "
                                               "allocated data memory.");
                        goto err;
                }
                module->lkey_data = w_mem_data;
                
                attr.field_mask = LCP_MEM_ADDR_FIELD |
                        LCP_MEM_SIZE_FIELD;
                rc = lcp_mem_query(w_mem_data, &attr);

                assert(attr.size >= size);
                module->base_data = *base = attr.address;
        } else if (flavor == MPI_WIN_FLAVOR_DYNAMIC) {
                //nothing to do.
        } else {
                mpc_common_debug_error("MPI OSC: wrong flavor.");
                goto err;
        }

        /* Allocate state for remote synchronization. */
        module->state = sctk_malloc(sizeof(mpc_osc_module_state_t));
        if (module->state == NULL) {
                mpc_common_debug_error("MPI OSC: could not allocate module "
                                       "state.");
                rc = MPC_LOWCOMM_ERROR;
                goto unmap_mem_data;
        }
        memset(module->state, 0, sizeof(mpc_osc_module_state_t));

        lcp_mem_param_t mem_state_param = {
                .field_mask = LCP_MEM_ADDR_FIELD |
                        LCP_MEM_SIZE_FIELD,
                .flags   = LCP_MEM_REGISTER_CREATE |
                        LCP_MEM_REGISTER_STATIC,
                .address = module->state,
                .size    = sizeof(mpc_osc_module_state_t), 
        };

        rc = lcp_mem_provision(module->mngr, &w_mem_state, &mem_state_param); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("MPI OSC: could not provision "
                                       "state memory.");
                goto unmap_mem_data;
        }
        attr.field_mask = LCP_MEM_ADDR_FIELD |
                LCP_MEM_SIZE_FIELD;
        rc = lcp_mem_query(w_mem_state, &attr);

        assert(attr.size >= sizeof(mpc_osc_module_state_t) && attr.address != NULL);
        module->base_state = attr.address;
        module->lkey_state = w_mem_state;

        /* Exchange the memory key with every other process. 
         * First, pack state memory key. */
        if (flavor == MPI_WIN_FLAVOR_CREATE || flavor == MPI_WIN_FLAVOR_ALLOCATE) {
                rc = lcp_mem_pack(module->mngr, module->lkey_data,
                                  &key_data_buf, &key_data_len);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto unmap_mem_state;
                }
        } else {
                key_data_len = 0;
        }

        /* Second, pack state memory key. */
        rc = lcp_mem_pack(module->mngr, module->lkey_state, &key_state_buf,
                          &key_state_len);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto release_key;
        }

        win_info_len  = 0;
        win_info_len += 2 * sizeof(void *);
        win_info_len += 2 * sizeof(int);
        win_info_len += key_data_len + key_state_len;
        win_info = sctk_malloc(win_info_len);
        if (win_info == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate buffer for both "
                                       "memory keys.");
                rc = MPC_LOWCOMM_ERROR;
                goto release_key;
        }
        //FIXME: simplify with macro
        memcpy((char *)win_info, &module->base_data, sizeof(void *)); 
        win_info_offset += sizeof(void *);
        memcpy((char *)win_info + win_info_offset, &module->base_state, sizeof(void *)); 
        win_info_offset += sizeof(void *);
        memcpy((char *)win_info + win_info_offset, &key_data_len, sizeof(int)); 
        win_info_offset += sizeof(int);
        memcpy((char *)win_info + win_info_offset, &key_state_len, sizeof(int)); 
        win_info_offset += sizeof(int);
        memcpy((char *)win_info + win_info_offset, key_data_buf, key_data_len);
        win_info_offset += key_data_len;
        memcpy((char *)win_info + win_info_offset, key_state_buf, key_state_len);

        /* First send size of local key to all others. */
        win_info_lengths = sctk_malloc(win->comm_size * sizeof(int));
        win_info_disps   = sctk_malloc(win->comm_size * sizeof(int));
        if (win_info_lengths == NULL || win_info_disps == NULL) {
                mpc_common_debug_error("WIN: could not allocate remote keys "
                                       "size table");
                goto release_key;
        }
        memset(win_info_lengths, 0, win->comm_size * sizeof(int));
        memset(win_info_disps, 0, win->comm_size * sizeof(int));

        PMPI_Allgather(&win_info_len, 1, MPI_INT, win_info_lengths, 1, MPI_INT, comm);

        total_win_info_lengths = 0;
        for (int i = 0; i < win->comm_size; i++) {
                win_info_disps[i]       = total_win_info_lengths; 
                total_win_info_lengths += win_info_lengths[i];
        }

        win_info_buf = sctk_malloc(total_win_info_lengths);
        if (win_info_buf == NULL) {
                mpc_common_debug_error("WIN: could not allocate memory for "
                                       "rkeys buffer.");
                goto release_key;
        }
        PMPI_Allgatherv(win_info, win_info_len, MPI_BYTE, (void *)win_info_buf,
                        win_info_lengths, win_info_disps, MPI_BYTE, comm);

        
        module->rdata_win_info  = sctk_malloc(win->comm_size * sizeof(mpc_osc_win_info_t));
        module->rstate_win_info  = sctk_malloc(win->comm_size * sizeof(mpc_osc_win_info_t));
        if (module->rdata_win_info == NULL || module->rstate_win_info == NULL) {
                mpc_common_debug_error("OSC WIN: could not allocate memory for "
                                       "remote memory keys.");
                goto release_key;
        }

        win_info_offset = 0;
        for (int i = 0; i < win->comm_size; i++) {
                //FIXME: simplify with macro
                memcpy(&module->rdata_win_info[i].addr, (char *)win_info_buf + win_info_disps[i], sizeof(void *));
                win_info_offset += sizeof(void *);
                memcpy(&module->rstate_win_info[i].addr, (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(void *));
                win_info_offset += sizeof(void *);
                memcpy(&rkey_data_len, (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(int));
                win_info_offset += sizeof(int);
                memcpy(&rkey_state_len, (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(int));
                win_info_offset += sizeof(int);

                module->rdata_win_info[i].rkey_init = 0;
                if (rkey_data_len > 0 && (flavor == MPI_WIN_FLAVOR_CREATE ||
                                          flavor == MPI_WIN_FLAVOR_ALLOCATE)) {
                        rc = lcp_mem_unpack(module->mngr, &module->rdata_win_info[i].rkey, 
                                            (void *)((char *)win_info_buf + 
                                                     win_info_disps[i] + win_info_offset),
                                            rkey_data_len);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                mpc_common_debug_error("OSC WIN: could not unpack memory for "
                                                       "remote memory keys.");
                                goto release_key;
                        }
                        win_info_offset += rkey_data_len;
                        module->rdata_win_info[i].rkey_init = 1;
                }
                
                rc = lcp_mem_unpack(module->mngr, &module->rstate_win_info[i].rkey, 
                                    (void *)((char *)win_info_buf + win_info_disps[i] + 
                                             win_info_offset),
                                    rkey_state_len);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto release_key;
                }
                module->rstate_win_info[i].rkey_init = 1;
                win_info_offset = 0;
        }

        /* Finally, synchronize with everyone. */
	PMPI_Barrier(comm);

        if (flavor == MPI_WIN_FLAVOR_CREATE || flavor == MPI_WIN_FLAVOR_ALLOCATE) {
                lcp_mem_release_rkey_buf(key_data_buf);
        }
        lcp_mem_release_rkey_buf(key_state_buf);

        mpc_common_debug("MPI OSC: win setup. state=%p", module->base_state);

        sctk_free(win_info_lengths);
        sctk_free(win_info_buf);
        sctk_free(win_info_disps);

        return rc;

release_key:
        if (win_info_buf)          sctk_free(win_info_buf);
        if (win_info_disps)        sctk_free(win_info_disps);
        if (win_info_lengths)      sctk_free(win_info_lengths);
        if (key_data_buf)          lcp_mem_release_rkey_buf(key_data_buf); 
        if (key_state_buf)         lcp_mem_release_rkey_buf(key_state_buf); 
unmap_mem_state:
        lcp_mem_deprovision(module->mngr, w_mem_state);
unmap_mem_data:
        if (win->win_module.state) sctk_free(win->win_module.state);
        lcp_mem_deprovision(module->mngr, w_mem_data);
err:
        return rc;
}

static int _win_config(void **base, size_t size, int disp_unit, 
                       int flavor, int model, mpc_win_t *win)
{
        UNUSED(base);
        UNUSED(size);
        UNUSED(disp_unit);
        UNUSED(flavor);
        UNUSED(model);
        UNUSED(win);
        TODO("WIN: set windows attributes");

        return 0;
}

static int _win_init(mpc_win_t **win_p, int flavor, mpc_lowcomm_communicator_t comm,
                     void **base, int disp_unit, size_t size, MPI_Info info)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;
        lcp_task_h task;
        int tid;

        /* Allocate the window and initialize fields. */
        win = sctk_malloc(sizeof(mpc_win_t));
        if (win == NULL) {
                rc = -1;
                goto err;
        }
        memset(win, 0, sizeof(mpc_win_t));

        win->comm_size      = mpc_lowcomm_communicator_size(comm);
        win->win_module.ctx = lcp_ctx_loc;

        tid = mpc_common_get_task_rank();
        task = lcp_context_task_get(win->win_module.ctx, tid);

        mpc_common_spinlock_lock(&osc_win_lock);
        if (osc_mngr == NULL) {
                lcp_manager_param_t mngr_param = {
                        .estimated_eps = win->comm_size,
                        .flags         = LCP_MANAGER_OSC_MODEL,
                        .field_mask    = LCP_MANAGER_ESTIMATED_EPS |
                                LCP_MANAGER_COMM_MODEL,
                };
                rc = lcp_manager_create(win->win_module.ctx, &osc_mngr, &mngr_param);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                mpc_common_progress_register(mpc_osc_progress);
        }
        mpc_common_spinlock_unlock(&osc_win_lock);

        win->win_module.mngr = osc_mngr;

        rc = lcp_task_associate(task, win->win_module.mngr);

        win->flavor    = flavor;
        win->model     = MPI_WIN_SEPARATE;
        win->size      = size;
        win->win_module.disp_unit = disp_unit;
        win->win_module.lock_count = 0;
        win->info      = info;
        strcpy(win->win_name, "MPI_WIN_LCP");

        win->comm = mpc_lowcomm_communicator_dup(comm);
        win->group = mpc_lowcomm_communicator_group(comm);
        win->win_module.eps = sctk_malloc(win->comm_size * sizeof(lcp_ep_h));
        if (win->win_module.eps == NULL) {
                mpc_common_debug_error("WIN: could not allocate window endpoint "
                                       "table.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(win->win_module.eps, 0, win->comm_size * sizeof(lcp_ep_h));

        if (mpc_lowcomm_communicator_get_process_count(win->comm) > 1 && 
            mpc_lowcomm_communicator_local_task_count(win->comm) > 1) {
                win->win_module.ep_flags  = LCP_EP_REQUIRE_NET_ATOMICS;
                win->win_module.ato_flags = LCP_REQUEST_USE_NET_ATOMICS;

                mpc_common_debug("OSC WIN: using network atomics only.");
        }

        mpc_common_hashtable_init(&win->win_module.outstanding_locks, 1024);
        mpc_list_init_head(&win->win_module.pending_posts);
	mpc_common_hashtable_init(&win->attrs, 16);

        /* Exchange windows information with other processes. */ 
        rc = _win_setup(win, base, size, disp_unit, 
                        comm, info, flavor);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Set windows attributes. */
        _win_config(base, size, disp_unit, flavor, MPI_WIN_SEPARATE, win);

        *win_p = win;

        return rc;

err:
        mpc_common_debug_fatal("MPI OSC: could not initialize window.");
        return rc;
}

int mpc_osc_find_attached_region_position(mpc_osc_dynamic_win_t *regions,
                                          int min_idx, int max_idx, uint64_t base,
                                          size_t len, int *insert_idx)
{
        int mid_idx = (min_idx + max_idx) >> 1;

        if (min_idx > max_idx) {
                *insert_idx = min_idx;
                return -1;
        }

        if (base < regions[mid_idx].base) {
                return mpc_osc_find_attached_region_position(regions, min_idx, mid_idx-1, 
                                                             base, len, insert_idx);
        } else if (base + len < regions[mid_idx].base + regions[mid_idx].size) {
                return mid_idx;
        } else {
                return mpc_osc_find_attached_region_position(regions, mid_idx+1, max_idx, 
                                                             base, len, insert_idx);
        }
}

int mpc_osc_win_attach(mpc_win_t *win, void *base, size_t len)
{
        int rc = MPI_SUCCESS;
        int my_rank = mpc_lowcomm_communicator_rank_fast(win->comm);
        int insert_idx;
        int found_idx;
        int memkey_length;
        void *memkey_buf;
        lcp_task_h task;
        lcp_ep_h ep;
        mpc_osc_module_t *module = &win->win_module;
        lcp_mem_attr_t attr;

        if (module->state->dynamic_win_count >= OSC_DYNAMIC_WIN_ATTACH_MAX) {
                return MPI_ERR_INTERN;
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
        rc = mpc_osc_get_comm_info(module, my_rank, win->comm, &ep);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = mpc_osc_start_exclusive(module, task, my_rank);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        if (module->state->dynamic_win_count > 0) {
                found_idx = mpc_osc_find_attached_region_position((mpc_osc_dynamic_win_t *)module->state->dynamic_wins, 0, 
                                                                  module->state->dynamic_win_count - 1,
                                                                  (uint64_t)base, len, &insert_idx);

                if (found_idx != -1) {
                        module->local_dynamic_win_infos[found_idx].refcnt++;
                        rc = mpc_osc_end_exclusive(module, task, my_rank);
                        return rc;
                }

                assert(insert_idx >= 0 && insert_idx <= (int)module->state->dynamic_win_count);

                memmove((void *)&module->local_dynamic_win_infos[insert_idx+1],
                        (void *)&module->local_dynamic_win_infos[insert_idx],
                        (OSC_DYNAMIC_WIN_ATTACH_MAX - (insert_idx+1))*sizeof(mpc_osc_local_dynamic_win_info_t));
                memmove((void *)&module->state->dynamic_wins[insert_idx+1],
                        (void *)&module->state->dynamic_wins[insert_idx],
                        (OSC_DYNAMIC_WIN_ATTACH_MAX - (insert_idx+1))*sizeof(mpc_osc_dynamic_win_t));
        } else {
                insert_idx = 0;
        }

        lcp_mem_param_t param = {
                .field_mask = LCP_MEM_ADDR_FIELD |
                        LCP_MEM_SIZE_FIELD,
                .flags   = LCP_MEM_REGISTER_CREATE |
                        LCP_MEM_REGISTER_STATIC,
                .address = base,
                .size    = len 
        };

        rc = lcp_mem_provision(module->mngr, 
                               &(module->local_dynamic_win_infos[insert_idx].memh),
                               &param); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unlock_dyn;
        }

        attr.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD;
        rc = lcp_mem_query(module->local_dynamic_win_infos[insert_idx].memh,
                           &attr);

        assert(attr.size >= len && attr.address == base);

        module->state->dynamic_wins[insert_idx].base = (uint64_t)base;
        module->state->dynamic_wins[insert_idx].size = len;

        rc = lcp_mem_pack(module->mngr,
                          module->local_dynamic_win_infos[insert_idx].memh, 
                          &memkey_buf,
                          &memkey_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unlock_dyn;
        }

        assert(memkey_length <= OCS_LCP_RKEY_BUF_SIZE_MAX);

        module->state->dynamic_wins[insert_idx].rkey_len = memkey_length;
        memcpy((void *)module->state->dynamic_wins[insert_idx].rkey_buf, 
               memkey_buf, memkey_length);

        module->local_dynamic_win_infos[insert_idx].refcnt++;
        module->state->dynamic_win_count++;

        mpc_common_debug("MPI OSC: dyn win count. count=%d", 
                         module->state->dynamic_win_count);

        lcp_mem_release_rkey_buf(memkey_buf);

        mpc_common_debug("MPI OSC: attached memory. base=%p.", base);

unlock_dyn:
        mpc_osc_end_exclusive(module, task, my_rank);
err:
        return rc;
}

int mpc_osc_win_detach(mpc_win_t *win, const void *base)
{
        int rc = MPI_SUCCESS;
        int my_rank = mpc_lowcomm_communicator_rank_fast(win->comm);
        int region_idx, insert_idx;
        lcp_task_h task;
        mpc_osc_module_t *module = &win->win_module;

        task = lcp_context_task_get(win->win_module.ctx, mpc_common_get_task_rank());

        rc = mpc_osc_start_exclusive(module, task, my_rank);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        region_idx
                = mpc_osc_find_attached_region_position((mpc_osc_dynamic_win_t*)module->state->dynamic_wins,
                                                        0,
                                                        module->state->dynamic_win_count-1,
                                                        (uint64_t)base, 1,
                                                        &insert_idx);

        if (region_idx == -1) {
                rc = mpc_osc_end_exclusive(module, task, my_rank);
                return rc;
        }

        module->local_dynamic_win_infos[region_idx].refcnt--;
        if (module->local_dynamic_win_infos[region_idx].refcnt == 0) {
                lcp_mem_deprovision(module->mngr, module->local_dynamic_win_infos[region_idx].memh);

                memmove((void *)&module->local_dynamic_win_infos[region_idx],
                        (void *)&module->local_dynamic_win_infos[region_idx+1],
                        (OSC_DYNAMIC_WIN_ATTACH_MAX - (region_idx+1))*sizeof(mpc_osc_local_dynamic_win_info_t));
                memmove((void *)&module->state->dynamic_wins[region_idx],
                        (void *)&module->state->dynamic_wins[region_idx+1],
                        (OSC_DYNAMIC_WIN_ATTACH_MAX - (region_idx+1))*sizeof(mpc_osc_dynamic_win_t));

                module->state->dynamic_win_count--;
                mpc_common_debug("MPI OSC: dyn win count. count=%d", 
                                 module->state->dynamic_win_count);
        }
        rc = mpc_osc_end_exclusive(module, task, my_rank);

err:
        return rc;
}

int mpc_win_create_dynamic(MPI_Info info,
                           mpc_lowcomm_communicator_t comm, 
                           mpc_win_t **win_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;

        rc = _win_init(&win, MPI_WIN_FLAVOR_DYNAMIC, comm, NULL, 1,
                       0, info);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_common_debug("MPI OSC: dynamic window creation.");

        *win_p = win;

err:
        return rc;
}

int mpc_win_create(void **base, size_t size, 
                   int disp_unit, MPI_Info info,
                   mpc_lowcomm_communicator_t comm, 
                   mpc_win_t **win_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;

        rc = _win_init(&win, MPI_WIN_FLAVOR_CREATE, comm, base, disp_unit,
                       size, info);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_common_debug("MPI OSC: create window creation. base=%p size=%lu", 
                         *base, size);

        *win_p = win;

err:
        return rc;
}

int mpc_win_allocate(void **base, size_t size, 
                     int disp_unit, MPI_Info info,
                     mpc_lowcomm_communicator_t comm, 
                     mpc_win_t **win_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;

        rc = _win_init(&win, MPI_WIN_FLAVOR_ALLOCATE, comm, base, disp_unit,
                       size, info);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_common_debug("MPI OSC: allocate window creation. base=%p size=%lu", 
                         *base, size);

        *win_p = win;

err:
        return rc;
}

int mpc_win_free(mpc_win_t *win)
{
        int rc = MPI_SUCCESS, i;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task = lcp_context_task_get(win->win_module.ctx, 
                                               mpc_common_get_task_rank());

        assert(module->lock_count == 0);

        rc = mpc_osc_perform_flush_op(module, task, NULL, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        PMPI_Barrier(win->comm);

        /* Deallocate table of displacement units if not all processes had the
         * same. */
        if (module->disp_unit == -1) {
                sctk_free(module->disp_units);
        }

        /* Deregister local key */
        if (win->flavor == MPI_WIN_FLAVOR_CREATE || win->flavor == MPI_WIN_FLAVOR_ALLOCATE) {
                rc = lcp_mem_deprovision(win->win_module.mngr, win->win_module.lkey_data);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("WIN: could not deregister local data memory.");
                        goto err;
                }

                for (i = 0; i < win->comm_size; i++) {
                        rc = lcp_mem_deprovision(win->win_module.mngr, 
                                                 win->win_module.rdata_win_info[i].rkey);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        } 

        /* Detach all attached memory. */
        for (i = 0; i < (int)win->win_module.state->dynamic_win_count; i++) {
                rc = lcp_mem_deprovision(win->win_module.mngr, 
                                         win->win_module.local_dynamic_win_infos[i].memh);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }
        win->win_module.state->dynamic_win_count = 0;

        rc = lcp_mem_deprovision(win->win_module.mngr, win->win_module.lkey_state);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("WIN: could not deregister local state memory.");
                goto err;
        }
        mpc_osc_win_attr_ht_release(&win->attrs);

        mpc_common_hashtable_release(&win->win_module.outstanding_locks);

        sctk_free(win->win_module.rdata_win_info);
        sctk_free(win->win_module.rstate_win_info);

err:
        return rc;
}

int osc_module_fini()
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int tid;
        lcp_task_h task;

        tid = mpc_common_get_task_rank();
        task = lcp_context_task_get(lcp_ctx_loc, tid);

        rc = lcp_task_dissociate(task, osc_mngr);

        //NOTE: make sure all task have been dissociated before finishing the
        //      manager.
        PMPI_Barrier(MPI_COMM_WORLD);

        mpc_common_spinlock_lock(&osc_win_lock);
        if (osc_mngr != NULL) {
                mpc_common_progress_unregister(mpc_osc_progress);

                rc = lcp_manager_fini(osc_mngr);
                osc_mngr = NULL;
        }
        mpc_common_spinlock_unlock(&osc_win_lock);
        return rc;
}
