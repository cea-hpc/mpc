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

#include "mpc_launch_pmi.h"
#include "osc_mpi.h"

#include <assert.h>

#include "mpi_conf.h"
#include <sctk_alloc.h>
#include <communicator.h> //FIXME: is this good include?
#include <mpc_common_progress.h>

extern lcp_context_h lcp_ctx_loc;
static lcp_manager_h osc_mngr = NULL;

int mpc_osc_progress() 
{
        return lcp_progress(osc_mngr);
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
        size_t win_info_len = 0;
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
                win->disp_unit = disp_unit;
        } else {
                win->disp_unit = -1;
                win->disp_units = sctk_malloc(win->comm_size * sizeof(int));
                if (win->disp_units == NULL) {
                        mpc_common_debug_error("WIN: could not allocated displacement "
                                               "table.");
                        rc = -1;
                        goto err;
                }

                win->disp_units[win->comm_rank] = disp_unit;

                PMPI_Allgather(&disp_unit, 1, MPI_INT, 
                               win->disp_units, 1, MPI_INT,
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
                if (rc != LCP_SUCCESS) {
                        goto err;
                }
                
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
                if (rc != LCP_SUCCESS) {
                        goto err;
                }
                
                attr.field_mask = LCP_MEM_ADDR_FIELD |
                        LCP_MEM_SIZE_FIELD;
                rc = lcp_mem_query(w_mem_data, &attr);

                assert(attr.size >= size && attr.address != NULL);
                module->base_data = *base = attr.address;
        } else {
                mpc_common_debug_error("MPI OSC: wrong flavor.");
                goto err;
        }

        module->lkey_data = w_mem_data;
        
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
        if (rc != LCP_SUCCESS) {
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
        rc = lcp_mem_pack(module->mngr, w_mem_state, &key_data_buf, &key_data_len);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto unmap_mem_state;
        }

        /* Second, pack state memory key. */
        rc = lcp_mem_pack(module->mngr, module->lkey_state, &key_state_buf, &key_state_len);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto release_key;
        }

        win_info_len += win->comm_size * sizeof(void *);
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
        win_info_offset += key_state_len;
        memcpy((char *)win_info + win_info_offset, key_state_buf, key_state_len);

        lcp_mem_release_rkey_buf(key_data_buf);
        lcp_mem_release_rkey_buf(key_state_buf);

        /* First send size of local key to all others. */
        win_info_lengths = sctk_malloc(win->comm_size * sizeof(int));
        win_info_disps   = sctk_malloc(win->comm_size * sizeof(int));
        if (win_info_lengths == NULL && win_info_disps == NULL) {
                mpc_common_debug_error("WIN: could not allocate remote keys "
                                       "size table");
                goto release_key;
        }

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

        
        module->rkeys_data  = sctk_malloc(win->comm_size * sizeof(lcp_mem_h));
        module->rkeys_state = sctk_malloc(win->comm_size * sizeof(lcp_mem_h));
        module->base_rdata  = sctk_malloc(win->comm_size * sizeof(void *));
        module->base_rstate = sctk_malloc(win->comm_size * sizeof(void *));
        if (module->rkeys_data == NULL || module->rkeys_state == NULL ||
            module->base_rdata == NULL || module->base_rstate == NULL) {
                mpc_common_debug_error("WIN: could not allocate memory for "
                                       "remote memory keys.");
                goto release_key;
        }

        win_info_offset = 0;
        for (int i = 0; i < win->comm_size; i++) {
                //FIXME: simplify with macro
                memcpy(&module->base_rdata[i], (char *)win_info_buf + win_info_disps[i], sizeof(void *));
                win_info_offset += sizeof(void *);
                memcpy(&module->base_rstate[i], (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(void *));
                win_info_offset += sizeof(void *);
                memcpy(&rkey_data_len, (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(int));
                win_info_offset += sizeof(int);
                memcpy(&rkey_state_len, (char *)win_info_buf + win_info_disps[i] + win_info_offset, sizeof(int));
                win_info_offset += sizeof(int);

                rc = lcp_mem_unpack(module->mngr, &module->rkeys_data[i], 
                               (void *)((char *)win_info_buf + win_info_disps[i] + win_info_offset),
                               rkey_data_len);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto release_key;
                }
                win_info_offset += rkey_data_len;
                rc = lcp_mem_unpack(module->mngr, &module->rkeys_state[i], 
                                    (void *)((char *)win_info_buf + win_info_disps[i] + win_info_offset),
                                    rkey_state_len);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto release_key;
                }
                win_info_offset = 0;
        }

        /* Finally, synchronize with everyone. */
	PMPI_Barrier(comm);

        sctk_free(win_info_lengths);
        sctk_free(win_info_buf);
        sctk_free(win_info_disps);

        return rc;
release_key:
        if (key_data_buf)          lcp_mem_release_rkey_buf(key_data_buf); 
        if (key_state_buf)         lcp_mem_release_rkey_buf(key_state_buf); 
        if (win_info_buf)          sctk_free(win_info_buf);
        if (win_info_disps)        sctk_free(win_info_disps);
        if (win_info_lengths)      sctk_free(win_info_lengths);
unmap_mem_state:
        lcp_mem_deprovision(module->mngr, w_mem_state);
unmap_mem_data:
        if (win->win_module.state) sctk_free(win->win_module.state);
        lcp_mem_deprovision(module->mngr, w_mem_data);
err:
        return rc;
}

static int _win_config(void *base, size_t size, int disp_unit, 
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
        static mpc_common_spinlock_t win_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
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

        mpc_common_spinlock_lock(&win_lock);
        if (osc_mngr == NULL) {
                lcp_manager_param_t mngr_param = {
                        .estimated_eps = win->comm_size,
                        .num_tasks     = win->comm_size,
                        .flags         = LCP_MANAGER_OSC_MODEL,
                        .field_mask    = LCP_MANAGER_ESTIMATED_EPS |
                                LCP_MANAGER_NUM_TASKS              |
                                LCP_MANAGER_COMM_MODEL,
                };
                rc = lcp_manager_create(win->win_module.ctx, &osc_mngr, &mngr_param);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                mpc_common_progress_register(mpc_osc_progress);
        }
        mpc_common_spinlock_unlock(&win_lock);

        win->win_module.mngr = osc_mngr;

        rc = lcp_task_associate(task, win->win_module.mngr);

        win->flavor    = MPI_WIN_FLAVOR_CREATE;
        win->model     = MPI_WIN_SEPARATE;
        win->size      = size;
        win->disp_unit = disp_unit;
        win->info      = info;
        strcpy(win->win_name, "MPI_WIN_LCP");

        win->comm = comm;
        win->group = mpc_lowcomm_communicator_group(comm);
        win->win_module.eps = sctk_malloc(win->comm_size * sizeof(lcp_ep_h));
        if (win->win_module.eps == NULL) {
                mpc_common_debug_error("WIN: could not allocate window endpoint "
                                       "table.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(win->win_module.eps, 0, win->comm_size * sizeof(lcp_ep_h));

        mpc_common_hashtable_init(&win->win_module.outstanding_locks, 1024);
        mpc_list_init_head(&win->win_module.pending_posts);

        /* Exchange windows information with other processes. */ 
        _win_setup(win, base, size, disp_unit, 
                   comm, info, flavor);

        /* Set windows attributes. */
        _win_config(base, size, disp_unit, MPI_WIN_FLAVOR_CREATE, 
                       MPI_WIN_MODEL, win);
        
        *win_p = win;

err:
        return rc;
}

int mpc_win_create(void *base, size_t size, 
                   int disp_unit, MPI_Info info,
                   mpc_lowcomm_communicator_t comm, 
                   mpc_win_t **win_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;

        rc = _win_init(&win, MPI_WIN_FLAVOR_CREATE, comm, &base, disp_unit, size, info);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *win_p = win;

err:
        return rc;
}

int mpc_win_allocate(void *base, size_t size, 
                     int disp_unit, MPI_Info info,
                     mpc_lowcomm_communicator_t comm, 
                     mpc_win_t **win_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_win_t *win;

        rc = _win_init(&win, MPI_WIN_FLAVOR_ALLOCATE, comm, &base, disp_unit, size, info);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *win_p = win;

err:
        return rc;
}

int mpc_win_free(mpc_win_t *win)
{
        int rc;

        /* Deallocate table of displacement units if not all processes had the
         * same. */
        if (win->disp_unit == -1) {
                sctk_free(win->disp_units);
        }

        /* Deregister local key */
        rc = lcp_mem_deprovision(win->win_module.mngr, win->win_module.lkey_data);
        if (rc != LCP_SUCCESS) {
                mpc_common_debug_error("WIN: could not deregister local data memory.");
                goto err;
        }

        rc = lcp_mem_deprovision(win->win_module.mngr, win->win_module.lkey_state);
        if (rc != LCP_SUCCESS) {
                mpc_common_debug_error("WIN: could not deregister local state memory.");
                goto err;
        }

        mpc_common_hashtable_release(&win->win_module.outstanding_locks);

        if (win->win_module.start_grp_ranks) sctk_free(win->win_module.start_grp_ranks);

        sctk_free(win->win_module.rkeys_data);
        sctk_free(win->win_module.rkeys_state);

err:
        return rc;
}

int osc_module_fini()
{
        int rc = MPC_LOWCOMM_SUCCESS;
        if (osc_mngr != NULL) {
                rc = lcp_manager_fini(osc_mngr);
        }
        return rc;
}
