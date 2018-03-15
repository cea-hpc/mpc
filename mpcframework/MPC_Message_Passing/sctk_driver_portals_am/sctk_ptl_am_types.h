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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#if defined(MPC_USE_PORTALS) && defined(MPC_Active_Message)
#ifndef SCTK_PTL_AM_TYPES_H_
#define SCTK_PTL_AM_TYPES_H_

#include <portals4.h>
#include "sctk_spinlock.h"
#include "sctk_io_helper.h"
#include "sctk_atomics.h"

/*********************************/
/********** MATCH BITS ***********/
/*********************************/
#define SCTK_PTL_AM_MATCH_INIT (sctk_ptl_am_matchbits_t) { \
	.data.srvcode = 0,\
	.data.rpcode = 0, \
	.data.tag = 0, \
	.data.inc_data = 0, \
	.data.is_req = 0, \
	.data.is_large = 0, \
}

#define SCTK_PTL_AM_IGN_TAG  (UINT32_MAX)
#define SCTK_PTL_AM_IGN_RPC  (UINT16_MAX)
#define SCTK_PTL_AM_IGN_SRV  (UINT8_MAX)
#define SCTK_PTL_AM_IGN_LARGE  (0x1)
#define SCTK_PTL_AM_IGN_TYPE (0x1)
#define SCTK_PTL_AM_IGN_DATA (0x1)

#define SCTK_PTL_AM_IGN_ALL (sctk_ptl_am_matchbits_t){\
	.data.srvcode = SCTK_PTL_AM_IGN_SRV,\
	.data.rpcode = SCTK_PTL_AM_IGN_RPC, \
	.data.tag = SCTK_PTL_AM_IGN_TAG, \
	.data.inc_data = SCTK_PTL_AM_IGN_DATA, \
	.data.is_req = SCTK_PTL_AM_IGN_TYPE, \
	.data.is_large = SCTK_PTL_AM_IGN_LARGE \
}

#define SCTK_PTL_AM_MATCH_TAG  ((uint32_t)0)
#define SCTK_PTL_AM_MATCH_RPC  ((uint16_t)0)
#define SCTK_PTL_AM_MATCH_SRV  ((uint8_t)0)
#define SCTK_PTL_AM_MATCH_LARGE  (0x0)
#define SCTK_PTL_AM_MATCH_TYPE (0x0)
#define SCTK_PTL_AM_MATCH_DATA (0x0)

#define SCTK_PTL_MATCH_BUILD(a,b,c,d,e,f) (sctk_ptl_am_matchbits_t){\
	.data.srvcode = a, \
	.data.rpcode = b, \
	.data.tag = c, \
	.data.inc_data = d, \
	.data.is_req = e, \
	.data.is_large = f \
}


/******* HELPERS *******/
#define SCTK_PTL_IGN_FOR_REQ (sctk_ptl_am_matchbits_t) {\
	.data.srvcode = SCTK_PTL_AM_MATCH_SRV,\
	.data.rpcode = SCTK_PTL_AM_IGN_RPC, \
	.data.tag = SCTK_PTL_AM_IGN_TAG, \
	.data.inc_data = SCTK_PTL_AM_IGN_DATA, \
	.data.is_req = SCTK_PTL_AM_MATCH_TYPE, \
	.data.is_large = SCTK_PTL_AM_MATCH_LARGE \
}

#define SCTK_PTL_IGN_FOR_REP (sctk_ptl_am_matchbits_t) {\
	.data.srvcode = SCTK_PTL_AM_MATCH_SRV,\
	.data.rpcode = SCTK_PTL_AM_IGN_RPC, \
	.data.tag = SCTK_PTL_AM_MATCH_TAG, \
	.data.inc_data = SCTK_PTL_AM_IGN_DATA, \
	.data.is_req = SCTK_PTL_AM_MATCH_TYPE, \
	.data.is_large = SCTK_PTL_AM_MATCH_LARGE \
}

#define SCTK_PTL_IGN_FOR_LARGE (sctk_ptl_am_matchbits_t) {\
	.data.srvcode = SCTK_PTL_AM_MATCH_SRV,\
	.data.rpcode = SCTK_PTL_AM_MATCH_RPC, \
	.data.tag = SCTK_PTL_AM_MATCH_TAG, \
	.data.inc_data = SCTK_PTL_AM_MATCH_DATA, \
	.data.is_req = SCTK_PTL_AM_MATCH_TYPE, \
	.data.is_large = SCTK_PTL_AM_MATCH_LARGE \
}

/*********************************/
/******* MATCHING ENTRIES ********/
/*********************************/
/* typedefs */
#define sctk_ptl_meh_t ptl_handle_me_t /**< ME handler */
#define sctk_ptl_me_t ptl_me_t         /**< ME */
#define SCTK_PTL_AM_ME_FLAGS PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_NO_TRUNCATE
#define SCTK_PTL_AM_ME_PUT_FLAGS SCTK_PTL_AM_ME_FLAGS | PTL_ME_OP_PUT
#define SCTK_PTL_AM_ME_GET_FLAGS SCTK_PTL_AM_ME_FLAGS | PTL_ME_OP_GET
#define SCTK_PTL_ONCE PTL_ME_USE_ONCE
#define SCTK_PTL_AM_ME_NO_OFFSET (-1)


/*********************************/
/****** MEMORY DESCRIPTORS *******/
/*********************************/
/* typedefs */
#define sctk_ptl_mdh_t ptl_handle_md_t /**< MD handler */
#define sctk_ptl_md_t ptl_md_t         /**< MD */
#define SCTK_PTL_AM_MD_FLAGS PTL_MD_EVENT_SEND_DISABLE
#define SCTK_PTL_AM_MD_PUT_FLAGS SCTK_PTL_AM_MD_FLAGS /**< no effect yet */
#define SCTK_PTL_AM_MD_GET_FLAGS SCTK_PTL_AM_MD_FLAGS /**< no effect yet */
#define SCTK_PTL_AM_EQ_MDS_SIZE 10240

/*********************************/
/************ EVENTS *************/
/*********************************/
/* typedefs */
#define sctk_ptl_event_t ptl_event_t    /**< event */
#define sctk_ptl_cnt_t ptl_ct_event_t   /**< counting event */
#define sctk_ptl_cnth_t ptl_handle_ct_t /**< couting event handler */
#define sctk_ptl_eq_t ptl_handle_eq_t   /**< event queue handler */

/*********************************/
/******** IDENTIFICATION *********/
/*********************************/
#define sctk_ptl_id_t ptl_process_t
#define SCTK_PTL_ANY_PROCESS (sctk_ptl_id_t) {.phys.nid = PTL_NID_ANY, .phys.pid = PTL_PID_ANY}

/*********************************/
/******* PORTALS ENTRIES *********/
/*********************************/
#define SCTK_PTL_AM_PTE_FLAGS PTL_PT_FLOWCTRL
#define sctk_ptl_list_t ptl_list_t
#define SCTK_PTL_PRIORITY_LIST PTL_PRIORITY_LIST
#define SCTK_PTL_AM_EQ_PTE_SIZE 10240
#define SCTK_PTL_AM_PMI_TAG 42

/*********************************/
/**** OTHER USEFUL CONSTANTS  ****/
/*********************************/
/* typedefs */
#define sctk_ptl_nih_t ptl_handle_ni_t    /**< NIC handler */
#define sctk_ptl_limits_t ptl_ni_limits_t /**< Portals NIC limits */
#define SCTK_PTL_AM_CHUNK_SZ (128 * 1024) /**< 128 KiB */
#define SCTK_PTL_AM_REQ_NB_DEF 8
#define SCTK_PTL_AM_REP_NB_DEF 1
#define SCTK_PTL_AM_REP_CELL_SZ 128
#define SCTK_PTL_AM_MD_NB_DEF 4

#define SCTK_PTL_AM_REQ_TYPE 0
#define SCTK_PTL_AM_REP_TYPE 1
#define SCTK_PTL_AM_OP_SMALL 0
#define SCTK_PTL_AM_OP_LARGE 1
struct sctk_ptl_am_bits_content_s
{
	uint32_t   tag;        /**< Identify an operation                       */
	uint16_t   rpcode;     /**< identify a RPC for a given service          */
	uint8_t    srvcode;    /**< identify a service                          */
	char       inc_data:1; /**< are data embedded by the operation ?        */
	char       is_req:1;   /**< is the operation a request or a response ?  */
	char       is_large:1; /**< is the current slot a large memory buffer ? */
	char       not_used:5; /**< Not used (yet)                              */
};

typedef union sctk_ptl_am_matchbits_s
{
	ptl_match_bits_t raw;                /**< raw */
	struct sctk_ptl_am_bits_content_s data; /**< driver-managed */
} sctk_ptl_am_matchbits_t;

#define SCTK_PTL_NO_IMM_DATA (sctk_ptl_am_imm_data_t) {.raw = 0}
typedef union sctk_ptl_am_imm_data_s
{
	uint64_t raw;                   /**< the raw */
	struct __imm_data_s
	{
		uint32_t offset;
		uint32_t not_used;
	} data;
} sctk_ptl_am_imm_data_t;

union sctk_ptl_am_slot_u
{
	sctk_ptl_me_t me; /**< request is a ME */
	sctk_ptl_md_t md; /**< request is a MD */
};

union sctk_ptl_am_slot_h_u
{
	sctk_ptl_meh_t meh; /**< request is a ME */
	sctk_ptl_mdh_t mdh; /**< request is a MD */
};

typedef struct sctk_ptl_am_local_data_s
{
	union sctk_ptl_am_slot_u slot;     /**< the request (MD or ME) */
	union sctk_ptl_am_slot_h_u slot_h; /**< the request Handle */
	sctk_ptl_am_matchbits_t match;     /**< request match bits */
} sctk_ptl_am_local_data_t;

typedef struct sctk_ptl_am_chunk_s
{
	sctk_atomics_int refcnt;
	sctk_atomics_int noff;
	int tag;
	struct sctk_ptl_am_chunk_s* next;
	sctk_ptl_am_local_data_t* uptr;
	char buf[SCTK_PTL_AM_CHUNK_SZ];
} sctk_ptl_am_chunk_t;


/**
 * Representing a PT entry in the driver.
 */
typedef struct sctk_ptl_am_pte_s
{
	ptl_pt_index_t idx; /**< the effective PT index */
	sctk_ptl_eq_t eq;   /**< the EQ for this entry */
	sctk_spinlock_t pte_lock;
	sctk_atomics_int next_tag;
	
	/**TODO: List of MEs for this PTE */
	sctk_ptl_am_chunk_t* req_head;
	sctk_ptl_am_chunk_t* rep_head;

} sctk_ptl_am_pte_t;

typedef struct sctk_ptl_am_msg_s
{
	sctk_ptl_id_t remote;
	size_t offset;
	uint32_t tag;
} sctk_ptl_am_msg_t;

/**
 * Portals-specific information specializing a rail.
 */
typedef struct sctk_ptl_am_rail_info_s
{
	sctk_ptl_limits_t max_limits;           /**< container for Portals threshold */
	sctk_ptl_nih_t iface;                   /**< Interface handler for the device */
	sctk_ptl_id_t id;                       /**< Local id identifying this rail */

	/**TODO: List of MD chunks */
	sctk_ptl_eq_t mds_eq;
	sctk_ptl_am_local_data_t md_slot;

	sctk_ptl_am_pte_t** pte_table; /**< The PT hash table */
	size_t nb_entries;                      /**< current number of PT entries */
	size_t eager_limit;                     /**< the max size for a small payload */
	
	char connection_infos[MAX_STRING_SIZE]; /**< string identifying this rail over the PMI */
	size_t connection_infos_size;           /**< Size of the above string */
} sctk_ptl_am_rail_info_t;
#endif
#endif
