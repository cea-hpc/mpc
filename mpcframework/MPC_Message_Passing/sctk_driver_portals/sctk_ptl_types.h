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

#ifdef MPC_USE_PORTALS
#ifndef SCTK_PTL_TYPES_H_
#define SCTK_PTL_TYPES_H_

#include <portals4.h>
#include "sctk_ht.h"
#include "sctk_io_helper.h"
#include "sctk_atomics.h"

/* match bits */
#define SCTK_PTL_MATCH_INIT (sctk_ptl_matchbits_t) {.data.rank = 0, .data.tag = 0, .data.uid = 0}
#define SCTK_PTL_IGN_TAG  (UINT32_MAX)
#define SCTK_PTL_IGN_RANK  (UINT16_MAX)
#define SCTK_PTL_IGN_UID  (UINT16_MAX)
#define SCTK_PTL_IGN_ALL (sctk_ptl_matchbits_t){.data.rank = SCTK_PTL_IGN_RANK, .data.tag = SCTK_PTL_IGN_TAG, .data.uid = SCTK_PTL_IGN_UID}
#define SCTK_PTL_MATCH_TAG  ((uint32_t)0)
#define SCTK_PTL_MATCH_RANK  ((uint16_t)0)
#define SCTK_PTL_MATCH_UID  ((uint16_t)0)
#define SCTK_PTL_MATCH_ALL  (sctk_ptl_matchbits_t) {.data.rank = SCTK_PTL_MATCH_RANK, .data.tag = SCTK_PTL_MATCH_TAG, .data.uid = SCTK_PTL_MATCH_UID}

/* MEs */
#define sctk_ptl_meh_t ptl_handle_me_t
#define sctk_ptl_me_t ptl_me_t
#define SCTK_PTL_ME_FLAGS PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE
#define SCTK_PTL_ME_PUT_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_PUT
#define SCTK_PTL_ME_GET_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_GET
#define SCTK_PTL_ME_OVERFLOW_FLAGS SCTK_PTL_ME_PUT_FLAGS
#define SCTK_PTL_ME_OVERFLOW_NB 24 /* number of ME in OVERFLOW_LIST (for each PTE) */
#define SCTK_PTL_ONCE PTL_ME_USE_ONCE

#define sctk_ptl_list_t ptl_list_t
#define SCTK_PTL_PRIORITY_LIST PTL_PRIORITY_LIST
#define SCTK_PTL_OVERFLOW_LIST PTL_OVERFLOW_LIST
#define SCTK_PTL_STR_LIST(u) ((u==SCTK_PTL_PRIORITY_LIST) ? "PRIORITY" : "OVERFLOW")

/* MDs */
#define sctk_ptl_mdh_t ptl_handle_md_t
#define sctk_ptl_md_t ptl_md_t
#define SCTK_PTL_MD_FLAGS PTL_MD_EVENT_SEND_DISABLE
#define SCTK_PTL_MD_PUT_FLAGS SCTK_PTL_MD_FLAGS
#define SCTK_PTL_MD_GET_FLAGS SCTK_PTL_MD_FLAGS

/* events */
#define sctk_ptl_event_t ptl_event_t
#define sctk_ptl_cnt_t ptl_ct_event_t
#define sctk_ptl_cnth_t ptl_handle_ct_t
#define sctk_ptl_eq_t ptl_handle_eq_t
#define SCTK_PTL_EQ_PTE_SIZE 10240 /* size of EQ for PTE */
#define SCTK_PTL_EQ_MDS_SIZE 10240 /* size of EQ for MD unique EQ */

/* id */
#define sctk_ptl_id_t ptl_process_t
#define SCTK_PTL_ANY_PROCESS (sctk_ptl_id_t) {.phys.nid = PTL_NID_ANY, .phys.pid = PTL_PID_ANY}

/* RDMA */
#define sctk_ptl_rdma_type_t ptl_datatype_t
#define sctk_ptl_rdma_op_t ptl_op_t

/* Portals entry */
#define SCTK_PTL_PTE_FLAGS PTL_PT_FLOWCTRL
#define SCTK_PTL_PTE_HIDDEN 3
#define SCTK_PTL_PTE_RECOVERY (0)
#define SCTK_PTL_PTE_CM       (1)
#define SCTK_PTL_PTE_RDMA     (2)
#define SCTK_PTL_PTE_ENTRY(table, comm) (MPCHT_get(&table, (comm)+SCTK_PTL_PTE_HIDDEN))
#define SCTK_PTL_PTE_EXIST(table, comm) (SCTK_PTL_PTE_ENTRY(table, comm) != NULL)

/* MISCS */
#define sctk_ptl_nih_t ptl_handle_ni_t
#define sctk_ptl_limits_t ptl_ni_limits_t
#define SCTK_PTL_MAX_RDV_BLOCKS 4

/**
 * How the match_bits is divided to store essential information to 
 * match MPI msg
 */
struct sctk_ptl_bits_content_s
{
	uint16_t uid;     /**< unique per-route ID */
	uint16_t rank;    /**< MPI/MPC rank */
	uint32_t tag;     /**< MPI tag */
};

/** struct to make match_bits management easier
 */
typedef union sctk_ptl_matchbits_t
{
	ptl_match_bits_t raw; /**< raw */
	struct sctk_ptl_bits_content_s data; /**< driver-managed */
} sctk_ptl_matchbits_t;

/**
 * the CM-specfic imm_data.
 * please look at sctk_control_messages.c before updating this struct */
typedef struct sctk_ptl_cm_data_s
{
	char type;
	char subtype;
	char param;
	char rail_id;
	char pad[4];
} sctk_ptl_cm_data_t;

/**
 * RDV-specific imm_data
 */
typedef struct sctk_ptl_std_data_s
{
	int datatype; /**< the datatype, for the matching */
	char pad[3];  /**< padding */
} sctk_ptl_std_data_t;

/**
 * Selector for the 64 bits immediate data
 * contained in every Put() request.
 */
typedef union sctk_ptl_imm_data_s
{
	uint64_t raw;                       /**< the raw */
	struct sctk_ptl_cm_data_s cm;       /**< imm_data for CM */
	struct sctk_ptl_std_data_s std;     /**< imm-data for eager */
} sctk_ptl_imm_data_t;

/**
 * Representing a PT entry in the driver.
 */
typedef struct sctk_ptl_pte_s
{
	ptl_pt_index_t idx; /**< the effective PT index */
	sctk_ptl_eq_t eq; /**< the EQ for this entry */
	//sctk_spinlock_t* taglocks;
} sctk_ptl_pte_t;

/** union to select MD or ME in the user_ptr without dirty casting */
union sctk_ptl_slot_u
{
	sctk_ptl_me_t me; /* request is a ME */
	sctk_ptl_md_t md; /* request is a MD */
};

/** union to select MD or ME in the user_ptr without dirty casting */
union sctk_ptl_slot_h_u
{
	sctk_ptl_meh_t meh; /* request is a ME */
	sctk_ptl_mdh_t mdh; /* request is a MD */
};

/**
 * Structure storing everything we need locally to map 
 * a msg with a PTL request
 */
typedef struct sctk_ptl_local_data_s
{
	union sctk_ptl_slot_u slot;     /* the request (MD or ME) */
	union sctk_ptl_slot_h_u slot_h; /* the request Handle */
	sctk_ptl_list_t list;           /* the list the request issued from */
	sctk_ptl_matchbits_t match;
	char type;                    /* 8 bytes of extra infos (protocol & type) */
	char prot;
	void* msg;                      /* link to the msg */
} sctk_ptl_local_data_t;

#define SCTK_PTL_TYPE_RECOVERY (0)
#define SCTK_PTL_TYPE_CM       (1)
#define SCTK_PTL_TYPE_RDMA     (2)
#define SCTK_PTL_TYPE_STD      (3)
#define SCTK_PTL_TYPE_NONE     (255)

#define SCTK_PTL_PROT_RDV      (0)
#define SCTK_PTL_PROT_EAGER    (1)
#define SCTK_PTL_PROT_NONE     (255)

/**
 * RDMA structure, mapping a Portals window.
 * Please remind that this struct is exchanged with remote through CM.
 */
typedef struct sctk_ptl_rdma_ctx
{
	struct sctk_ptl_local_data_s* me_data; /**< ME infos about the pinned region */
	struct sctk_ptl_local_data_s* md_data; /**< MD infos about the pinned region */
	void* start;                           /**< start address */
	sctk_ptl_matchbits_t match;            /**< unique match_bits (=atomic counter */
	sctk_ptl_id_t origin;
} sctk_ptl_rdma_ctx_t;

/**
 * Portals-specific data stored in msg tail.
 */
typedef struct sctk_ptl_tail_s
{
	struct sctk_ptl_local_data_s* user_ptr; /**< user_ptr, attached to the request */
	int copy;                               /**< true if data has been temporarily copied */
} sctk_ptl_tail_t;


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
	sctk_ptl_eq_t mds_eq;                   /**< EQ for all MDs emited from this NI */
	struct MPCHT pt_table;                  /**< The PT hash table */

	size_t cutoff;                          /**< cutoff for large RDV messages */
	size_t max_mr;                          /**< Max size of a memory region (MD | ME ) */
	size_t eager_limit;                     /**< the max size for an eager msg */
	size_t nb_entries;                      /**< current number of PT entries dedicated to comms */
	sctk_atomics_int rdma_cpt;              /**< RDMA match_bits counter */

	char connection_infos[MAX_STRING_SIZE]; /**< string identifying this rail over the PMI */
	size_t connection_infos_size;           /**< Size of the above string */
} sctk_ptl_rail_info_t;
#endif
#endif
