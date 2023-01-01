#include "lcr_ptl_recv.h"

#include "sctk_ptl_iface.h"

#include <utlist.h>

int lcr_ptl_recv_block_init(sctk_ptl_rail_info_t *srail, lcr_ptl_recv_block_t **block_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_recv_block_t *block;

        block = sctk_malloc(sizeof(lcr_ptl_recv_block_t));        
        if (block == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate eager block structure");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        block->size  = 4 * srail->ptl_info.eager_block_size;        
        block->rail  = srail;
        block->start = sctk_malloc(block->size);
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

int lcr_ptl_recv_block_activate(lcr_ptl_recv_block_t *block)
{
        ptl_me_t me;
        sctk_ptl_rail_info_t *srail = block->rail;
        sctk_ptl_matchbits_t match = SCTK_PTL_MATCH_INIT;
        sctk_ptl_matchbits_t ign = SCTK_PTL_IGN_ALL;

        if (block->start == NULL) {
                return MPC_LOWCOMM_ERROR;
        }

        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .ignore_bits = ign.raw,
                .match_id    = SCTK_PTL_ANY_PROCESS,
                .match_bits  = match.raw,
                .min_free    = block->rail->ptl_info.eager_block_size,
                .options     = PTL_ME_OP_PUT | 
                        PTL_ME_MANAGE_LOCAL | 
                        PTL_ME_EVENT_LINK_DISABLE,
                .uid         = PTL_UID_ANY,
                .start       = block->start,
                .length      = block->size
        };

        sctk_ptl_chk(PtlMEAppend(srail->iface,         /* the NI handler */
                                 srail->ptl_info.eager_pt_idx,             /* the targeted PT entry */
                                 &me,   /* the ME to register in the table */
                                 SCTK_PTL_PRIORITY_LIST,       /* in which list the ME has to be appended */
                                 block,             /* usr_ptr: forwarded when polling the event */
                                 &block->meh /* out: the ME handler */
                                ));

        mpc_common_debug("LCR_PTL: activate block");

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_recv_block_enable(sctk_ptl_rail_info_t *srail)
{
        int rc, i;

        for (i=0; i< srail->ptl_info.num_eager_blocks; i++) {
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
                        return rc;
                }
                el->block = block;
                DL_APPEND(srail->ptl_info.block_list, el);
               
                rc = lcr_ptl_recv_block_activate(block);
        }

        return MPC_LOWCOMM_SUCCESS;
}

void lcr_ptl_recv_block_free(lcr_ptl_recv_block_t *block)
{
        if (block->start != NULL) {
                sctk_free(block->start);
        }
        PtlMEUnlink(block->meh);
        sctk_free(block);
}

int lcr_ptl_recv_block_disable(sctk_ptl_rail_info_t *srail)
{
        lcr_ptl_block_list_t *elem = NULL, *tmp = NULL;

        DL_FOREACH_SAFE(srail->ptl_info.block_list, elem, tmp) {
               lcr_ptl_recv_block_free(elem->block);
               DL_DELETE(srail->ptl_info.block_list, elem);
               sctk_free(elem);
        }

        return MPC_LOWCOMM_SUCCESS;
}
