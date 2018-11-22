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
#include <stddef.h>
#include "sctk_ht.h"
#include "sctk_io_helper.h"
#include "sctk_atomics.h"

/** Helper to find the struct base address, based on the address on a given member */
#define container_of(ptr, type, member) ({ \
                                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                                (type *)( (char *)__mptr - offsetof(type,member) );})

/*********************************/
/********** MATCH BITS ***********/
/*********************************/
/** Set the 'tag' member to be ignored during the matching */
#define SCTK_PTL_IGN_TAG  ((uint32_t)UINT32_MAX)
/** Set the 'rank' member to be ignored during the matching */
#define SCTK_PTL_IGN_RANK  ((uint16_t)UINT16_MAX)
/** Set the 'usage ID' member to be ignored during the matching */
#define SCTK_PTL_IGN_UID  ((uint8_t)UINT8_MAX)
/** Set the message_type member to be ignored during the matching */
#define SCTK_PTL_IGN_TYPE ((uint8_t)UINT8_MAX)
/** Set the 'tag' to be used for the matching step */
#define SCTK_PTL_MATCH_TAG  ((uint32_t)0)
/** Set the 'rank' to be used for the matching step */
#define SCTK_PTL_MATCH_RANK  ((uint16_t)0)
/** Set the 'usage ID' to be used for the matching step */
#define SCTK_PTL_MATCH_UID  ((uint8_t)0)
/** Set the 'Message type' to be used for the matching step */
#define SCTK_PTL_MATCH_TYPE  ((uint8_t)0)
#define SCTK_PTL_MAX_TYPES (1 << 7)
#define SCTK_PTL_TYPE_RDV_SET(a) do { a |= (1<<7);}while(0)
#define SCTK_PTL_TYPE_RDV_UNSET(a) do { a &= (~(1<<7));}while(0)

/** default struct to initialize a new sctk_ptl_matchbits_t */
#define SCTK_PTL_MATCH_INIT (sctk_ptl_matchbits_t) {.data.rank = SCTK_PTL_MATCH_RANK, .data.tag = SCTK_PTL_MATCH_TAG, .data.uid = SCTK_PTL_MATCH_UID, .data.type = SCTK_PTL_MATCH_TYPE}
/** A combination of SCTK_PTL_MATCH_{TAG,RANK,UID} */
#define SCTK_PTL_MATCH_ALL  (sctk_ptl_matchbits_t) {.data.rank = SCTK_PTL_MATCH_RANK, .data.tag = SCTK_PTL_MATCH_TAG, .data.uid = SCTK_PTL_MATCH_UID, .data.type = SCTK_PTL_MATCH_TYPE}
/** A combination of SCTK_PTL_IGN_{TAG,RANK,UID} */
#define SCTK_PTL_IGN_ALL (sctk_ptl_matchbits_t){.data.rank = SCTK_PTL_IGN_RANK, .data.tag = SCTK_PTL_IGN_TAG, .data.uid = SCTK_PTL_IGN_UID, .data.type = SCTK_PTL_IGN_TYPE}

/*********************************/
/******* MATCHING ENTRIES ********/
/*********************************/
/* typedefs */
#define sctk_ptl_meh_t ptl_handle_me_t /**< ME handler */
#define sctk_ptl_me_t ptl_me_t         /**< ME */
/** Default flags for new added ME: No EVENT_LINK/UNLINK */
#define SCTK_PTL_ME_FLAGS PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE
/** Default flags for new PUT-ME */
#define SCTK_PTL_ME_PUT_FLAGS (SCTK_PTL_ME_FLAGS | PTL_ME_OP_PUT)
/** Default flags for new GET-ME */
#define SCTK_PTL_ME_GET_FLAGS (SCTK_PTL_ME_FLAGS | PTL_ME_OP_GET)
/** default flags for OVERFLOW ME --> unexpected PUTs */
#define SCTK_PTL_ME_OVERFLOW_FLAGS (SCTK_PTL_ME_PUT_FLAGS)
/** Macro to automatically unlink an ME when a match occurs (not always used, ex: RDMA) */
#define SCTK_PTL_ONCE (PTL_ME_USE_ONCE)
/** Number of slots to maintain in the OVERFLOW_LIST of each PT entry */
#define SCTK_PTL_ME_OVERFLOW_NB 128


/*********************************/
/****** MEMORY DESCRIPTORS *******/
/*********************************/
/* typedefs */
#define sctk_ptl_mdh_t ptl_handle_md_t /**< MD handler */
#define sctk_ptl_md_t ptl_md_t         /**< MD */
/** default flags for new adde MD --> no EVENT_SEND (not used to ATOMICS ops w/ RDMA) */
#define SCTK_PTL_MD_FLAGS (PTL_MD_EVENT_SEND_DISABLE)
#define SCTK_PTL_MD_PUT_FLAGS (SCTK_PTL_MD_FLAGS) /**< no effect yet */
#define SCTK_PTL_MD_GET_FLAGS (SCTK_PTL_MD_FLAGS) /**< no effect yet */
/** default flags for RDMA requests --> here we need the EVENT_SEND flag */
#define SCTK_PTL_MD_ATOMICS_FLAGS (0)

/*********************************/
/************ EVENTS *************/
/*********************************/
/* typedefs */
#define sctk_ptl_event_t ptl_event_t    /**< event */
#define sctk_ptl_cnt_t ptl_ct_event_t   /**< counting event */
#define sctk_ptl_cnth_t ptl_handle_ct_t /**< couting event handler */
#define sctk_ptl_eq_t ptl_handle_eq_t   /**< event queue handler */
/** default size for the EQ associated with an PT entry */
#define SCTK_PTL_EQ_PTE_SIZE 10240
/** default size for the EQ associated with each MD registered by the process */
#define SCTK_PTL_EQ_MDS_SIZE 10240

/*********************************/
/******** IDENTIFICATION *********/
/*********************************/
/** a Portals identifier */
#define sctk_ptl_id_t ptl_process_t
/** Refer to any physical process */
#define SCTK_PTL_ANY_PROCESS (sctk_ptl_id_t) {.phys.nid = PTL_NID_ANY, .phys.pid = PTL_PID_ANY}
#define SCTK_PTL_IS_ANY_PROCESS(a) (a.phys.nid == PTL_NID_ANY && a.phys.pid == PTL_PID_ANY)

/*********************************/
/******* PORTALS ENTRIES *********/
/*********************************/
/** default flags for allocating a new PT entry */
#define SCTK_PTL_PTE_FLAGS PTL_PT_FLOWCTRL
/** number of PT in addition of one per comm */
#define SCTK_PTL_PTE_HIDDEN 3
/** one extra PT for the recovery system */
#define SCTK_PTL_PTE_RECOVERY (0)
/** one extra PT for the CM management */
#define SCTK_PTL_PTE_CM       (1)
/** one extra PT for RDMA management */
#define SCTK_PTL_PTE_RDMA     (2)
/** Translate the communicator ID to the PT entry object */
#define SCTK_PTL_PTE_ENTRY(table, comm) (MPCHT_get(&table, (comm)+SCTK_PTL_PTE_HIDDEN))
/** Check if a given comm already has corresponding PT entry */
#define SCTK_PTL_PTE_EXIST(table, comm) (SCTK_PTL_PTE_ENTRY(table, comm) != NULL)
/** 'RECOVERY' value in request type member */
#define SCTK_PTL_TYPE_RECOVERY (0)
/** 'CM' value in request type member */
#define SCTK_PTL_TYPE_CM       (1)
/** 'RDMA' value in request type member */
#define SCTK_PTL_TYPE_RDMA     (2)
/** 'STANDARD' value in request type member */
#define SCTK_PTL_TYPE_STD      (3)
/** 'INVALID' value in request type member */
#define SCTK_PTL_TYPE_NONE     (255)
/** 'RDV' value in request protocol member */
#define SCTK_PTL_PROT_RDV      (0)
/** 'EAGER' value in request protocol member */
#define SCTK_PTL_PROT_EAGER    (1)
/** 'INVALID' value in request protocol member */
#define SCTK_PTL_PROT_NONE     (255)
/** Typedef for a Portals list type */
#define sctk_ptl_list_t ptl_list_t
/** Typedef for a Portals priority_list type */
#define SCTK_PTL_PRIORITY_LIST PTL_PRIORITY_LIST
/** typedef for a Portals overflow8list type */
#define SCTK_PTL_OVERFLOW_LIST PTL_OVERFLOW_LIST
/** Translate a Portals list type into a human-readable string */
#define SCTK_PTL_STR_LIST(u) ((u==SCTK_PTL_PRIORITY_LIST) ? "PRIORITY" : "OVERFLOW")

/*********************************/
/************* RDMA **************/
/*********************************/
/* typedefs */
#define sctk_ptl_rdma_type_t ptl_datatype_t /**< RDMA data type */
#define sctk_ptl_rdma_op_t ptl_op_t         /**< RDMA operation */

/*********************************/
/********** OFFLOADING ***********/
/*********************************/
/* the config field is an int */
#define SCTK_PTL_OFFLOAD_NONE_FLAG (0x0)
#define SCTK_PTL_OFFLOAD_OD_FLAG (0x1)
#define SCTK_PTL_OFFLOAD_COLL_FLAG (0x2)
#define SCTK_PTL_IS_OFFLOAD_OD(u) ((u & SCTK_PTL_OFFLOAD_OD_FLAG) >> 0)
#define SCTK_PTL_IS_OFFLOAD_COLL(u) ((u & SCTK_PTL_OFFLOAD_COLL_FLAG) >> 1)


/*********************************/
/**** OTHER USEFUL CONSTANTS  ****/
/*********************************/
/* typedefs */
#define sctk_ptl_nih_t ptl_handle_ni_t    /**< NIC handler */
#define sctk_ptl_limits_t ptl_ni_limits_t /**< Portals NIC limits */
/** default number of chunks when RDV protocol wants to split big messages */
#define SCTK_PTL_MAX_RDV_BLOCKS 4
#ifndef UNUSED
#define UNUSED(a) (void*)&a
#endif

/**
 * How the match_bits is divided to store essential information to 
 * match MPI msg.
 * Valid for P2P messages and non-offloaded collectives
 */
struct sctk_ptl_std_content_s
{
	uint32_t tag;     /**< MPI tag */
	uint16_t rank;    /**< MPI/MPC rank */
	uint8_t uid;     /**< unique per-route ID */
	uint8_t type;    /**< message type, redundant in case of CM */
};

struct sctk_ptl_offload_content_s
{
	uint32_t pad1;
	uint16_t pad2;
	uint8_t dir; /* direction, if necessary */
	uint8_t type; /* this field should not be changed */
};

/** struct to make match_bits management easier.
 * Used for std P2P msgs and non-offloaded collectives.
 * Should rename the field 'data' but too much changes could be dangerous.
 */
typedef union sctk_ptl_matchbits_t
{
	ptl_match_bits_t raw;                /**< raw */
	struct sctk_ptl_std_content_s data; /**< driver-managed */
	struct sctk_ptl_offload_content_s offload;
} sctk_ptl_matchbits_t;

/**
 * the CM-specfic imm_data.
 * please look at sctk_control_messages.c before updating this struct */
typedef struct sctk_ptl_cm_data_s
{
	char type;    /**< CM type, redudant w/ matching value */
	char subtype; /**< CM subtype */
	char param;   /**< CM param */
	char rail_id; /**< referent rail ID */
	char pad[4];  /**< aligment */
} sctk_ptl_cm_data_t;

/**
 * RDV-specific imm_data
 */
typedef struct sctk_ptl_std_data_s
{
	int datatype; /**< the datatype, for the matching */
	char pad[4];  /**< padding */
} sctk_ptl_std_data_t;

typedef struct sctk_ptl_offload_data_s
{
	uint64_t pad;
} sctk_ptl_offload_data_t;

/**
 * Selector for the 64 bits immediate data
 * contained in every Put() request.
 */
typedef union sctk_ptl_imm_data_s
{
	uint64_t raw;                   /**< the raw */
	struct sctk_ptl_cm_data_s cm;   /**< imm_data for CM */
	struct sctk_ptl_std_data_s std; /**< imm-data for eager */
	struct sctk_ptl_offload_data_s offload; /**< imm-data for offload */
} sctk_ptl_imm_data_t;

typedef struct sctk_ptl_coll_opts_s
{
        sctk_ptl_id_t parent;
        sctk_ptl_id_t* children;
        size_t nb_children;
        char leaf;
	sctk_ptl_cnth_t* cnt_hb_up;
	sctk_ptl_cnth_t* cnt_hb_down;
} sctk_ptl_coll_opts_t;

/**
 * Representing a PT entry in the driver.
 */
typedef struct sctk_ptl_pte_s
{
	ptl_pt_index_t idx; /**< the effective PT index */
	sctk_ptl_eq_t eq;   /**< the EQ for this entry */
        sctk_ptl_coll_opts_t coll; /**< what is necessary to optimise collectives for this entry */
        sctk_spinlock_t lock; /**< lock for this PTE */
} sctk_ptl_pte_t;

/** union to select MD or ME in the user_ptr without dirty casting */
union sctk_ptl_slot_u
{
	sctk_ptl_me_t me; /**< request is a ME */
	sctk_ptl_md_t md; /**< request is a MD */
};

/** union to select MD or ME in the user_ptr without dirty casting */
union sctk_ptl_slot_h_u
{
	sctk_ptl_meh_t meh; /**< request is a ME */
	sctk_ptl_mdh_t mdh; /**< request is a MD */
};

/**
 * Structure storing everything we need locally to map 
 * a msg with a PTL request
 */
typedef struct sctk_ptl_local_data_s
{
	union sctk_ptl_slot_u slot;     /**< the request (MD or ME) */
	union sctk_ptl_slot_h_u slot_h; /**< the request Handle */
	sctk_ptl_list_t list;           /**< the list the request issued from */
	sctk_ptl_matchbits_t match;     /**< request match bits */
	char type;                      /**< request type */
	char prot;                      /**< request protocol */
	void* msg;                      /**< link to the msg */
} sctk_ptl_local_data_t;

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
	sctk_ptl_id_t origin;                  /**< the process that initiated the new region */
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
	int offload_support;
} sctk_ptl_rail_info_t;

#endif
#endif
