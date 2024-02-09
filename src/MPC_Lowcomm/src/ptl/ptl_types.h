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

#ifndef LCR_PTL_TYPES_H
#define LCR_PTL_TYPES_H

#include <utlist.h>
#include <list.h>
#include <queue.h>
#include <uthash.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_datastructure.h>
#include <mpc_mempool.h>

#include "lcr_def.h"
#include "mpc_common_bit.h"

#include <mpc_common_helper.h>

#include <portals4.h>

//Forward addressing
typedef struct lcr_ptl_block_list lcr_ptl_block_list_t;

/*********************************/
/********** PTL TYPES   **********/
/*********************************/
#define LCR_PTL_PT_NULL (ptl_pt_index_t)-1

#define SCTK_PTL_MATCH_INIT (sctk_ptl_matchbits_t) {.data.rank = SCTK_PTL_MATCH_RANK, .data.tag = SCTK_PTL_MATCH_TAG, .data.uid = SCTK_PTL_MATCH_UID, .data.type = SCTK_PTL_MATCH_TYPE}

#define PTL_EQ_PTE_SIZE 10240
/*********************************/
/********** PTL ADDRESS **********/
/*********************************/
typedef struct lcr_ptl_addr {
        ptl_process_t id;
        struct {
                ptl_pt_index_t am;  /* PT index for AM operations.  */
                ptl_pt_index_t tag; /* PT index for TAG operations. */
                ptl_pt_index_t rma; /* PT index for RMA operations. */
        } pts;
} lcr_ptl_addr_t;
#define LCR_PTL_ADDR_ANY \
        (lcr_ptl_addr_t) { \
                .id = SCTK_PTL_ANY_PROCESS, \
                .pts.am  = LCR_PTL_PT_NULL,  \
                .pts.tag = LCR_PTL_PT_NULL,  \
                .pts.rma = LCR_PTL_PT_NULL,  \
        } 
#define LCR_PTL_IS_ANY_PROCESS(a) ((a).id.phys.nid == PTL_NID_ANY && (a).id.phys.pid == PTL_PID_ANY)

typedef struct lcr_ptl_ep_info {
        lcr_ptl_addr_t addr;
} lcr_ptl_ep_info_t;
typedef struct lcr_ptl_ep_info _mpc_lowcomm_endpoint_info_ptl_t;

/*********************************/
/********** PTL RMA  *************/
/*********************************/

typedef struct lcr_ptl_rma_handle {
        void             *start;         /* Start address of the memory. */
        mpc_list_elem_t   elem;          /* Element in list.             */
        ptl_handle_ct_t   cth;           /* Counter handle.              */
        ptl_handle_md_t   mdh;           /* Memory Descriptor handle.    */
        size_t            size;          /* Size of the memory.          */
        int               completed_ops; /* Number of already completed OP. */
        mpc_queue_head_t  ops;           /* Active operations on handle. */
        mpc_common_spinlock_t ops_lock;  /* Memory lock.                 */
} lcr_ptl_rma_handle_t;

typedef struct lcr_ptl_persistant_post {
        UT_hash_handle hh;

        uint64_t tag_key;
        ptl_handle_me_t meh;
} lcr_ptl_persistant_post_t;


/*********************************/
/********** PTL OP   *************/
/*********************************/

/* Operation types. */
typedef enum {
        LCR_PTL_OP_BLOCK,
        LCR_PTL_OP_AM_BCOPY,
        LCR_PTL_OP_AM_ZCOPY,
        LCR_PTL_OP_TAG_BCOPY,
        LCR_PTL_OP_TAG_ZCOPY,
        LCR_PTL_OP_TAG_SEARCH,
        LCR_PTL_OP_RMA_PUT,
        LCR_PTL_OP_RMA_GET,
} lcr_ptl_op_type_t;

typedef struct lcr_ptl_op {
        lcr_ptl_op_type_t type; /* Type of operation */
        size_t            size; /* Data size of the operation */
        lcr_completion_t *comp; /* Completion callback */

        union {
                struct {
                        ptl_handle_md_t iovh;
                        ptl_iovec_t    *iov;
                } iov;
                struct {
                        lcr_tag_context_t *tag_ctx;
                        void *bcopy_buf;
                } tag;
                struct {
                        void *bcopy_buf;
                } am;
                struct {
                        int sz;
                } rma;
        };

        mpc_queue_elem_t elem;
} lcr_ptl_op_t;

/*********************************/
/********** Operation Context ****/
/*********************************/

/* Context for two-sided operations. */
typedef struct lcr_ptl_ts_ctx {
        ptl_handle_eq_t       eqh; /**< EQ for all MEs received on this NI */
        ptl_handle_md_t       mdh;
        ptl_pt_index_t        pti;
        mpc_common_spinlock_t eq_lock;
        lcr_ptl_block_list_t *blist;
} lcr_ptl_ts_ctx_t;

/* Context for one-sided operations. */
typedef struct lcr_ptl_os_ctx {
        ptl_handle_md_t       mdh;
        ptl_pt_index_t        pti;
        ptl_handle_me_t       rma_meh;
        mpc_list_elem_t       rmah_head;
        mpc_common_spinlock_t rmah_lock;
} lcr_ptl_os_ctx_t;

/*********************************/
/********** FEATURES *************/
/*********************************/
enum {
        LCR_PTL_FEATURE_AM  = MPC_BIT(0),
        LCR_PTL_FEATURE_TAG = MPC_BIT(1),
        LCR_PTL_FEATURE_RMA = MPC_BIT(2),
};

typedef struct lcr_ptl_iface_config {
        size_t          eager_limit;
        int             num_eager_blocks;
        int             eager_block_size;
        int             max_iovecs;
	size_t          max_mr;           /**< Max size of a memory region (MD | ME ) */
	size_t          max_put;          /**< Max size of a put */
	size_t          max_get;          /**< Max size of a get */
        size_t          min_frag_size;
	ptl_ni_limits_t max_limits;       /**< container for Portals thresholds */
} lcr_ptl_iface_config_t;


typedef struct lcr_ptl_rail_info {
        ptl_handle_ni_t             nih;
        lcr_ptl_addr_t              addr;
        lcr_ptl_iface_config_t      config;
	char                        connection_infos[MPC_COMMON_MAX_STRING_SIZE];
	struct mpc_common_hashtable ranks_ids_map; /**< each cell maps to the portals process object */
	size_t                      connection_infos_size;
        lcr_ptl_ts_ctx_t            am_ctx;
        lcr_ptl_ts_ctx_t            tag_ctx;
        lcr_ptl_os_ctx_t            os_ctx;
        unsigned                    features; /* Instanciated features. */
        lcr_ptl_persistant_post_t  *persistant_ht;
        mpc_mempool_t              *ops_pool;
} lcr_ptl_rail_info_t;
typedef struct lcr_ptl_rail_info _mpc_lowcomm_ptl_rail_info_t;


#endif
