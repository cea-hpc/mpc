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

#ifndef SCTK_PTL_TYPES_H_
#define SCTK_PTL_TYPES_H_

#ifdef MPC_USE_PORTALS
#include <portals4.h>
#include <stddef.h>
#include <mpc_keywords.h>
#include "mpc_common_datastructure.h"
#include "mpc_common_helper.h"
#include "mpc_common_asm.h"

#include "lcr/lcr_def.h"

#include <utlist.h>
#include <uthash.h>

/** Helper to find the struct base address, based on the address on a given member */
#ifndef container_of
#define container_of(ptr, type, member) ({ \
                                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                                (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

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
typedef enum {
	SCTK_PTL_PTE_RECOVERY,
	SCTK_PTL_PTE_CM,
	SCTK_PTL_PTE_RDMA,
	SCTK_PTL_PTE_HIDDEN,
	SCTK_PTL_PTE_HIDDEN_NB = SCTK_PTL_PTE_HIDDEN
} sctk_ptl_pte_id_t;

typedef enum {
        LCR_PTL_PTE_IDX_TAG_EAGER,
        LCR_PTL_PTE_IDX_AM_EAGER,
        LCR_PTL_PTE_IDX_RMA
} lcr_ptl_pte_id_t;

/** Translate the communicator ID to the PT entry object */
#define SCTK_PTL_PTE_ENTRY(table, comm) (mpc_common_hashtable_get(&table, (comm)+SCTK_PTL_PTE_HIDDEN))
/** Check if a given comm already has corresponding PT entry */
#define SCTK_PTL_PTE_EXIST(table, comm) (SCTK_PTL_PTE_ENTRY(table, comm) != NULL)
typedef enum {
	SCTK_PTL_TYPE_RECOVERY,
	SCTK_PTL_TYPE_CM,
	SCTK_PTL_TYPE_RDMA,
	SCTK_PTL_TYPE_STD,
	SCTK_PTL_TYPE_OFFCOLL,
	SCTK_PTL_TYPE_PROBE,
	SCTK_PTL_TYPE_NONE,
	SCTK_PTL_TYPE_NB,
} sctk_ptl_mtype_t;
typedef enum {
	SCTK_PTL_PROT_EAGER,
	SCTK_PTL_PROT_RDV,
	SCTK_PTL_PROT_NONE,
	SCTK_PTL_PROT_NB
} sctk_ptl_protocol_t;
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
#define sctk_ptl_interface_t ptl_interface_t  /**< Portals interface number for hardware init */
/** default number of chunks when RDV protocol wants to split big messages */
#define SCTK_PTL_MAX_RDV_BLOCKS 4

#define SCTK_PTL_MAX_TAGS  (( size_t)(~ (( uint32_t)0)))
#define SCTK_PTL_MAX_RANKS (( size_t)(~ (( uint16_t)0)))
#define SCTK_PTL_MAX_UIDS  (( size_t)(~ (( uint8_t)0)))
#define SCTK_PTL_MAX_TYPES (( size_t)(~ (( uint8_t)0)))

enum {
        PTL_MODE_NO_MATCH = 0,
        PTL_MODE_MATCH
};

/**
 * How the match_bits is divided to store essential information to
 * match MPI msg.
 * Valid for P2P messages and non-offloaded collectives
 */
typedef struct sctk_ptl_std_content_s
{
	uint32_t tag;  /**< MPI tag */
	uint16_t rank; /**< MPI/MPC rank */
	uint8_t uid;   /**< unique per-route ID */
	uint8_t type;  /**< message type, redundant in case of CM */
} sctk_ptl_std_content_t;

typedef struct sctk_ptl_offload_content_s
{
	uint32_t iter;
	uint16_t pad2;
	uint8_t dir; /* direction, if necessary */
	uint8_t type; /* this field should not be changed */
} sctk_ptl_offload_content_t;

/** struct to make match_bits management easier.
 * Used for std P2P msgs and non-offloaded collectives.
 * Should rename the field 'data' but too much changes could be dangerous.
 */
typedef union sctk_ptl_matchbits_t
{
	ptl_match_bits_t raw;                      /**< raw */
	struct sctk_ptl_std_content_s data;        /**< driver-managed */
	struct sctk_ptl_offload_content_s offload; /**< collective-offload related */
} sctk_ptl_matchbits_t;

/**
 * the CM-specfic imm_data.
 * please look at sctk_control_messages.c before updating this struct */
typedef struct sctk_ptl_cm_data_s
{
	char type;    /**< CM type, redudant w/ matching value */
} sctk_ptl_cm_data_t;

/**
 * RDV-specific imm_data
 */
typedef struct sctk_ptl_std_data_s
{
	int msg_seq_nb; /**< the MSG_NUMBER, for the matching */
	char putsz:1; /**< Does the payload is the message size ? */
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
	uint64_t raw;                           /**< the raw */
	struct sctk_ptl_cm_data_s cm;           /**< imm_data for CM */
	struct sctk_ptl_std_data_s std;         /**< imm-data for regular msgs */
	struct sctk_ptl_offload_data_s offload; /**< imm-data for offload */
} sctk_ptl_imm_data_t;

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

/* data used to make information persistent between posted probing request & notification */
typedef struct sctk_ptl_probing_data_s
{
	OPA_int_t found; /**< does the probing successful ? */
	size_t size;            /**< if so, size of the message */
	int rank;               /**< if so, rank of the remote */
	int tag;                /**< if so, tag of the message */
} sctk_ptl_probing_data_t;

/**
 * Structure storing everything we need locally to map
 * a msg with a PTL request
 */
typedef struct sctk_ptl_local_data_s
{
	union sctk_ptl_slot_u slot;     /**< the request (MD or ME) */
	union sctk_ptl_slot_h_u slot_h; /**< the request Handle */
	int msg_seq_nb;                 /**< mesage sequence number (not truncated) */
	OPA_int_t cnt_frag;      /**< number of chunks before being released */
	void* msg;                      /**< link to the msg */
	sctk_ptl_list_t list;           /**< the list the request issued from */
	sctk_ptl_protocol_t prot;       /**< request protocol */
	sctk_ptl_mtype_t type;          /**< request type */
	sctk_ptl_probing_data_t probe;  /**< related to probing */
	sctk_ptl_matchbits_t match;     /**< request match bits (RDV-spec) */
	size_t req_sz;                  /**< message size intented to be retrieved (RDV spec) */

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
typedef struct mpc_lowcomm_ptl_tail_s
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
_mpc_lowcomm_endpoint_info_portals_t;

/**
 * Offcoll-barrier specific information, stored in a local_data obj
 */
typedef struct sctk_ptl_offcoll_barrier_s
{
	sctk_ptl_cnth_t* cnt_hb_up;   /**< counter handling propagation of the first half barrier */
	sctk_ptl_cnth_t* cnt_hb_down; /**< counter handling propagation  of the second balf barrier */
} sctk_ptl_offcoll_barrier_t;

/**
 * offcoll-bcast specific infors, stored, in a local_data obj
 */
typedef struct sctk_ptl_offcoll_bcast_s
{
	sctk_ptl_local_data_t* large_puts; /**< the large_puts ME handler */
} sctk_ptl_offcoll_bcast_t;

/**
 * generification of the offcoll-specific info, stored into a local_data obj.
 */
typedef union sctk_ptl_offcoll_spec_u
{
	struct sctk_ptl_offcoll_barrier_s barrier; /**< data hosts barrier infos */
	struct sctk_ptl_offcoll_bcast_s bcast;     /**< data hosts bcas infos */
} sctk_ptl_offcoll_spec_t;

/**
 * Represend node infos from a temporary tree, used to map process communication when running offloaded collectives.
 * This tree is stored per collective, recomputed each time the root process need to be changed (a bit dirty...).
 */
typedef struct sctk_ptl_offcoll_tree_node_s
{
        mpc_common_spinlock_t lock;   /**< the lock */
	OPA_int_t iter; /**< iteration number (because PTL does not whant us to decr a counter) */
	sctk_ptl_id_t parent;               /**< the PTL id for the parent of the current process in this given tree */
	sctk_ptl_id_t* children;            /**< PTS id array of all children for the current process */
	size_t nb_children;                 /**< size of the array above */
	int root;                           /**< is this process the root of the tree ? */
	int leaf;                           /**< is this process a leaf of the tree ? (these two fields could be merged...) */
	union sctk_ptl_offcoll_spec_u spec; /**< info stored for the specific collective to process */
} sctk_ptl_offcoll_tree_node_t;

/**
 * Specific type of collective currently handled by this implementation */
typedef enum sctk_ptl_offcoll_type_e
{
	SCTK_PTL_OFFCOLL_BARRIER,
	SCTK_PTL_OFFCOLL_BCAST,
	SCTK_PTL_OFFCOLL_REDUCE,
	SCTK_PTL_OFFCOLL_NB
} sctk_ptl_offcoll_type_t;

/**
 * Representing a PT entry in the driver.
 */
typedef struct sctk_ptl_pte_s
{
	ptl_pt_index_t idx;                                            /**< the effective PT index */
	sctk_ptl_eq_t eq;                                              /**< the EQ for this entry */
	struct sctk_ptl_offcoll_tree_node_s node[SCTK_PTL_OFFCOLL_NB]; /**< what is necessary to optimise collectives for this entry */
} sctk_ptl_pte_t;

typedef enum {
        LCR_PTL_COMP_BLOCK,
        LCR_PTL_COMP_AM_BCOPY,
        LCR_PTL_COMP_AM_ZCOPY,
        LCR_PTL_COMP_TAG_BCOPY,
        LCR_PTL_COMP_TAG_ZCOPY,
        LCR_PTL_COMP_TAG_SEARCH,
        LCR_PTL_COMP_RMA_PUT
} lcr_ptl_comp_type_t;

typedef struct lcr_ptl_send_comp {
        lcr_ptl_comp_type_t type;
        sctk_ptl_mdh_t iov_mdh;
        ptl_iovec_t *iov;
        lcr_completion_t *comp;
        lcr_tag_context_t *tag_ctx;
        void *bcopy_buf;
} lcr_ptl_send_comp_t;

typedef struct lcr_ptl_recv_block lcr_ptl_recv_block_t;

typedef struct lcr_ptl_block_list {
        lcr_ptl_recv_block_t *block;
        struct lcr_ptl_block_list *prev, *next;
} lcr_ptl_block_list_t;

typedef struct lcr_ptl_persistant_post {
        UT_hash_handle hh;

        uint64_t tag_key;
        ptl_handle_me_t meh;
} lcr_ptl_persistant_post_t;

typedef struct lcr_ptl_info_s {
        ptl_handle_md_t mdh;
        int max_iovecs;
	sctk_ptl_eq_t eqh;                   /**< EQ for all MEs received on this NI */
        sctk_ptl_pte_id_t tag_pte;
        sctk_ptl_pte_id_t am_pte;
        sctk_ptl_pte_id_t rma_pte;
        ptl_handle_me_t rma_meh;
        int num_eager_blocks;                   /**< number of eager block */
        size_t eager_block_size;                /**< size of recv block for eager */
        lcr_ptl_block_list_t *tag_block_list;
        lcr_ptl_block_list_t *am_block_list;
        lcr_ptl_persistant_post_t *persistant_ht;
        mpc_common_spinlock_t poll_lock;
} lcr_ptl_info_t;

/**
 * Portals-specific information specializing a rail.
 */
typedef struct sctk_ptl_rail_info_s
{
	sctk_ptl_limits_t max_limits;           /**< container for Portals thresholds */
	sctk_ptl_nih_t iface;                   /**< Interface handler for the device */
	sctk_ptl_id_t id;                       /**< Local id identifying this rail */
	sctk_ptl_eq_t mds_eq;                   /**< EQ for all MDs emited from this NI */
	struct mpc_common_hashtable ranks_ids_map; /**< each cell maps to the portals process object */
	struct mpc_common_hashtable pt_table;         /**< The COMM => PT hash table */
	struct mpc_common_hashtable reverse_pt_table; /**< The PT => COMM hash table */
	size_t cutoff;                          /**< cutoff for large RDV messages */
        lcr_ptl_info_t ptl_info;
	size_t max_mr;                          /**< Max size of a memory region (MD | ME ) */
	size_t max_put;                          /**< Max size of a put */
	size_t max_get;                          /**< Max size of a get */
        size_t min_frag_size;
	size_t eager_limit;                     /**< the max size for an eager msg */
	size_t nb_entries;                      /**< current number of PT entries dedicated to comms */
	size_t connection_infos_size;           /**< Size of the above string */
	OPA_int_t rdma_cpt;              /**< RDMA match_bits counter */
	int offload_support;
	char connection_infos[MPC_COMMON_MAX_STRING_SIZE]; /**< string identifying this rail over the PMI */
} sctk_ptl_rail_info_t;

#endif /* MPC_USE_PORTALS */
#endif
