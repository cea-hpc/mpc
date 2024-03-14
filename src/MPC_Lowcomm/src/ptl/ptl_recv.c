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
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include "ptl_recv.h"

#include "ptl_iface.h"

#include <mpc_common_debug.h>
#include <mpc_lowcomm_types.h>

#include <utlist.h>

int lcr_ptl_recv_block_init(lcr_ptl_rail_info_t *srail, lcr_ptl_recv_block_t **block_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_recv_block_t *block;

        block = sctk_malloc(sizeof(lcr_ptl_recv_block_t));
        if (block == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate eager block structure");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        block->size      = 4 * srail->config.eager_block_size;
        block->rail      = srail;
        block->start     = sctk_malloc(block->size);
        block->op.type   = LCR_PTL_OP_BLOCK;

        if (block->start == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate eager block");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        block->meh = PTL_INVALID_HANDLE;

        *block_p = block;
err:
        return rc;
}

int lcr_ptl_recv_block_activate(lcr_ptl_recv_block_t *block, 
                                ptl_pt_index_t pte, 
                                ptl_list_t list)
{
        ptl_me_t me;
        ptl_match_bits_t match = 0;
        ptl_match_bits_t ign   = ~0;
        lcr_ptl_rail_info_t *srail = block->rail;

        if (block->start == NULL) {
                return MPC_LOWCOMM_ERROR;
        }

        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .match_bits  = match,
                .ignore_bits = ign,
                .match_id    = {
                        .phys.nid = PTL_NID_ANY,
                        .phys.pid = PTL_PID_ANY,
                },
                .min_free    = block->rail->config.eager_block_size,
                .options     = PTL_ME_OP_PUT       | 
                        PTL_ME_MANAGE_LOCAL        | 
                        PTL_ME_MAY_ALIGN           |
                        PTL_ME_EVENT_LINK_DISABLE,
                .uid         = PTL_UID_ANY,
                .start       = block->start,
                .length      = block->size
        };

        lcr_ptl_chk(PtlMEAppend(srail->nih,
                                 pte,
                                 &me,
                                 list,
                                 &block->op,
                                 &block->meh
                                ));

        return MPC_LOWCOMM_SUCCESS;
}

//FIXME: switch to memory pools
int lcr_ptl_recv_block_enable(lcr_ptl_rail_info_t *srail, 
                              ptl_pt_index_t pte, 
                              ptl_list_t list)
{
        int rc = MPC_LOWCOMM_SUCCESS, i;

        for (i=0; i< srail->config.num_eager_blocks; i++) {
                lcr_ptl_recv_block_t *block = NULL;
                rc = lcr_ptl_recv_block_init(srail, &block);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCR PTL: could not allocate block");
                        return rc;
                }

                lcr_ptl_block_list_t *el = sctk_malloc(sizeof(lcr_ptl_block_list_t));
                if (el == NULL) {
                        mpc_common_debug_error("LCR PTL: could not allocate list elem");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                el->block = block;
                switch(list) {
                case PTL_PRIORITY_LIST:
                        DL_APPEND(srail->am_ctx.blist, el);
                        break;
                case PTL_OVERFLOW_LIST:
                        DL_APPEND(srail->tag_ctx.blist, el);
                        break;
                default:
                        mpc_common_debug_error("LCR PTL: unknown list");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                        break;
                }

                rc = lcr_ptl_recv_block_activate(block, pte, list);
        }

err:
        return rc;
}

void lcr_ptl_recv_block_free(lcr_ptl_recv_block_t *block)
{
        if (block->start != NULL) {
                sctk_free(block->start);
        }
        PtlMEUnlink(block->meh);
        sctk_free(block);
}

int lcr_ptl_recv_block_disable(lcr_ptl_block_list_t *list)
{
        lcr_ptl_block_list_t *elem = NULL, *tmp = NULL;

        DL_FOREACH_SAFE(list, elem, tmp) {
               lcr_ptl_recv_block_free(elem->block);
               DL_DELETE(list, elem);
               sctk_free(elem);
        }

        return MPC_LOWCOMM_SUCCESS;
}

#endif /* MPC_USE_PORTALS */
