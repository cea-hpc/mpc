#ifdef MPC_USE_PORTALS
#ifndef SCTK_PTL_TYPES_H_
#define SCTK_PTL_TYPES_H_

#include <portals4.h>
#include "sctk_io_helper.h"
#include "sctk_atomics.h"

/* match bits */
#define SCTK_PTL_MATCH_INIT (sctk_ptl_matchbits_t) {.data.rank = 0, .data.tag = 0}
#define SCTK_PTL_IGN_TAG  (UINT32_MAX)
#define SCTK_PTL_IGN_RANK  (UINT32_MAX)
#define SCTK_PTL_IGN_ALL (sctk_ptl_matchbits_t){.data.rank = SCTK_PTL_IGN_RANK, .data.tag = SCTK_PTL_IGN_TAG}
#define SCTK_PTL_MATCH_TAG  ((uint32_t)0)
#define SCTK_PTL_MATCH_RANK  ((uint32_t)0)
#define SCTK_PTL_MATCH_ALL  (sctk_ptl_matchbits_t) {.data.rank = SCTK_PTL_MATCH_RANK, .data.tag = SCTK_PTL_MATCH_TAG}

/* MEs */
#define sctk_ptl_meh_t ptl_handle_me_t
#define sctk_ptl_me_t ptl_me_t
#define SCTK_PTL_ME_FLAGS PTL_ME_EVENT_LINK_DISABLE
#define SCTK_PTL_ME_PUT_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_PUT
#define SCTK_PTL_ME_GET_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_GET
#define SCTK_PTL_ME_OVERFLOW_FLAGS SCTK_PTL_ME_PUT_FLAGS
#define SCTK_PTL_ME_OVERFLOW_NB 16 /* number of ME in OVERFLOW_LIST (for each PTE) */

#define sctk_ptl_list_t ptl_list_t
#define SCTK_PTL_PRIORITY_LIST PTL_PRIORITY_LIST
#define SCTK_PTL_OVERFLOW_LIST PTL_OVERFLOW_LIST
#define SCTK_PTL_STR_LIST(u) ((u==SCTK_PTL_PRIORITY_LIST) ? "PRIORITY_LIST" : "OVERFLOW_LIST")

/* MDs */
#define sctk_ptl_mdh_t ptl_handle_md_t
#define sctk_ptl_md_t ptl_md_t
#define SCTK_PTL_MD_FLAGS 0
#define SCTK_PTL_MD_PUT_FLAGS SCTK_PTL_MD_FLAGS
#define SCTK_PTL_MD_GET_FLAGS SCTK_PTL_MD_FLAGS

/* events */
#define sctk_ptl_event_t ptl_event_t
#define sctk_ptl_cnt_t ptl_ct_event_t
#define sctk_ptl_cnth_t ptl_handle_ct_t
#define sctk_ptl_eq_t ptl_handle_eq_t
#define SCTK_PTL_EQ_PTE_SIZE 512 /* size of EQ for PTE */
#define SCTK_PTL_EQ_MDS_SIZE 128 /* size of EQ for MD unique EQ */

/* id */
#define sctk_ptl_id_t ptl_process_t
#define SCTK_PTL_ANY_PROCESS (sctk_ptl_id_t) {.phys.nid = PTL_NID_ANY, .phys.pid = PTL_PID_ANY}

/* RDMA */
#define sctk_ptl_rdma_type_t ptl_datatype_t
#define sctk_ptl_rdma_op_t ptl_op_t

/* Portals entry */
#define SCTK_PTL_PTE_FLAGS PTL_PT_FLOWCTRL
#define SCTK_PTL_PTE_HIDDEN 3
#define SCTK_PTL_PTE_RECOVERY ( -3)
#define SCTK_PTL_PTE_CM       ( -2)
#define SCTK_PTL_PTE_RDMA     ( -1)
#define SCTK_PTL_PTE_RECOVERY_IDX (0)
#define SCTK_PTL_PTE_CM_IDX       (1)
#define SCTK_PTL_PTE_RDMA_IDX     (2)

/* MISCS */
#define sctk_ptl_nih_t ptl_handle_ni_t
#define sctk_ptl_limits_t ptl_ni_limits_t



struct sctk_ptl_bits_content_s
{
	uint32_t rank;    /**< MPI/MPC rank */
	uint32_t tag;     /**< MPI tag */
};

/* should all be sized to raw 64 bits */
typedef union sctk_ptl_matchbits_t
{
	ptl_match_bits_t raw;
	struct sctk_ptl_bits_content_s data;
} sctk_ptl_matchbits_t;

typedef struct sctk_ptl_pte_s
{
	ptl_pt_index_t idx;
	sctk_ptl_eq_t eq;
	sctk_ptl_meh_t uniq_meh;
} sctk_ptl_pte_t;

union sctk_ptl_slot_u
{
	sctk_ptl_me_t me;
	sctk_ptl_md_t md;
};

union sctk_ptl_slot_h_u
{
	sctk_ptl_meh_t meh;
	sctk_ptl_mdh_t mdh;
};

typedef struct sctk_ptl_local_data_s
{
	union sctk_ptl_slot_u slot;
	union sctk_ptl_slot_h_u slot_h;
	sctk_ptl_list_t list;
	void* msg;
} sctk_ptl_local_data_t;

typedef struct sctk_ptl_rdma_ctx
{
	struct sctk_ptl_local_data_s* me_data;
	struct sctk_ptl_local_data_s* md_data;
	sctk_ptl_id_t origin;
} sctk_ptl_rdma_ctx_t;

/**
 * Portals-specific information describing a route.
 * Is contained by route.h
 */
typedef struct sctk_ptl_route_info_s
{
	sctk_ptl_id_t dest; /**< the remote process the route is connected to */
}
sctk_ptl_route_info_t;

/**
 * Portals-specific information specializing a rail.
 */
typedef struct sctk_ptl_rail_info_s
{
	sctk_ptl_limits_t max_limits;           /**< container for Portals thresholds */
	sctk_ptl_nih_t iface;                   /**< Interface handler for the device */
	sctk_ptl_id_t id;                       /**< Local id identifying this rail */
	sctk_ptl_pte_t* pt_entries;             /**< Portals table for this rail */
	sctk_ptl_eq_t mds_eq;                   /**< EQ for all MDs emited from this NI */


	size_t eager_limit;
	size_t nb_entries;
	sctk_thread_t async_tid;
	sctk_atomics_int rdma_cpt;

	char connection_infos[MAX_STRING_SIZE]; /**< string identifying this rail over the PMI */
	size_t connection_infos_size;           /**< Size of the above string */
} sctk_ptl_rail_info_t;
#endif
#endif
