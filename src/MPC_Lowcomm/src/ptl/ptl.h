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

#ifndef LCR_PTL_H
#define LCR_PTL_H

#include <mpc_mempool.h>
#include <lcr_def.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <stdatomic.h>
#include <list.h>
#include <queue.h>
#include <mpc_common_datastructure.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_helper.h>
#include <lowcomm_config.h>

#include <portals4.h>

//Forward addressing
typedef struct lcr_ptl_block_list lcr_ptl_block_list_t;
typedef struct lcr_ptl_mem lcr_ptl_mem_t;

//FIXME: some of these defines are only for control flow...
#define LCR_PTL_AM_HDR_ID_MASK         0x00000000000000ffULL
#define LCR_PTL_AM_HDR_TX_ID_MASK      0x0000000000ffff00ULL
#define LCR_PTL_AM_HDR_PEND_OPS_MASK   0x00ffffffff000000ULL

#define LCR_PTL_AM_HDR_GET_AM_ID(_hdr) \
        ((uint8_t)(_hdr & LCR_PTL_AM_HDR_ID_MASK))
#define LCR_PTL_AM_HDR_GET_TX_ID(_hdr) \
        ((uint16_t)((_hdr & LCR_PTL_CTRL_TX_ID_MASK) >> 8))
#define LCR_PTL_AM_HDR_GET_PEND_OPS(_hdr) \
        ((uint32_t)((_hdr & LCR_PTL_AM_HDR_PEND_OPS_MASK) >> 24))

#define LCR_PTL_AM_HDR_SET(_hdr, _am_id, _tx_id, _pend_op ) \
        _hdr |= (_pend_op & 0xffffffffull); \
        _hdr  = (_hdr << 16);               \
        _hdr |= (_tx_id & 0xffffull);       \
        _hdr  = (_hdr << 8);                \
        _hdr |= (_am_id & 0xff);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
#define LCR_PTL_CTRL_OP_ID_MASK     0x00000000000000ffull
#define LCR_PTL_CTRL_TX_ID_MASK     0x0000000000ffff00ull
#define LCR_PTL_CTRL_TOKEN_NUM_MASK 0x000000ffff000000ull

#define LCR_PTL_CTRL_HDR_GET_OP_ID(_hdr) \
        ((uint8_t)((_hdr) & LCR_PTL_CTRL_OP_ID_MASK))
#define LCR_PTL_CTRL_HDR_GET_TX_ID(_hdr) \
        ((uint16_t)((_hdr & LCR_PTL_CTRL_TX_ID_MASK) >> 8))
#define LCR_PTL_CTRL_HDR_GET_TOKEN_NUM(_hdr) \
        ((uint16_t)((_hdr & LCR_PTL_CTRL_TOKEN_NUM_MASK) >> 24))

#define LCR_PTL_CTRL_HDR_SET(_hdr, _op_type, _tx_id, _tk_num) \
        _hdr |= (_tk_num & 0xffffull); \
        _hdr  = (_hdr << 16);          \
        _hdr |= (_tx_id & 0xffffull);  \
        _hdr  = (_hdr << 8);           \
        _hdr |= (_op_type & 0xff);
#endif

#define _lcr_ptl_init_op_common(_op, _id, _mdh, _addr, _pti, \
                                _type, _size, _comp, _txq)   \
        (_op)->id   = _id;   \
        (_op)->hdr  = 0;     \
        (_op)->mdh  = _mdh;  \
        (_op)->addr = _addr; \
        (_op)->pti  = _pti;  \
        (_op)->type = _type; \
        (_op)->size = _size; \
        (_op)->comp = _comp; \
        (_op)->txq  = _txq;

#define LCR_PTL_PROCESS_TO_UINT64(_key, _process_id) \
        _key |= (_process_id).phys.nid & 0x00000000ffffffffull; \
        _key  = (_key << 32);                                   \
        _key |= (_process_id).phys.pid & 0x00000000ffffffffull;

/*********************************/
/********** PTL TYPES   **********/
/*********************************/
#define LCR_PTL_PT_NULL (ptl_pt_index_t)-1
#define LCR_PTL_RNDV_MB  0x1000000000000000ULL
#define LCR_PTL_RMA_MB   0x1100000000000000ULL
#define LCR_PTL_MB_MASK  0x0011111111111111ULL
#define LCR_PTL_MAX_TXQUEUES 1024

#define LCR_PTL_WRAP_AROUND_THRESHOLD PTL_SIZE_MAX / 2

#define LCR_PTL_SN_CMP_INF(_a, _b) \
        ( (_a) < (_b) && ( (_a) - (_b) < LCR_PTL_WRAP_AROUND_THRESHOLD ) )

#define PTL_EQ_PTE_SIZE 4096 

/*********************************/
/********** PTL ADDRESS **********/
/*********************************/
typedef struct lcr_ptl_addr {
        ptl_process_t id;
        struct {
                ptl_pt_index_t tk;   /* PT index for Token operations. */
                ptl_pt_index_t am;   /* PT index for AM operations.    */
                ptl_pt_index_t tag;  /* PT index for TAG operations.   */
                ptl_pt_index_t rma;  /* PT index for RMA operations.   */
        } pte;
} lcr_ptl_addr_t;

#define LCR_PTL_PROCESS_ANY \
        (ptl_process_t) { \
                .phys.nid = PTL_NID_ANY, \
                .phys.pid = PTL_PID_ANY, \
                 }

#define LCR_PTL_ADDR_ANY \
        (lcr_ptl_addr_t) { \
                .id       = LCR_PTL_PROCESS_ANY, \
                .pte.tk   = LCR_PTL_PT_NULL,     \
                .pte.am   = LCR_PTL_PT_NULL,     \
                .pte.tag  = LCR_PTL_PT_NULL,     \
                .pte.rma  = LCR_PTL_PT_NULL,     \
        } 

#define LCR_PTL_IS_ANY_PROCESS(a) \
        ((a).id.phys.nid == PTL_NID_ANY && \
         (a).id.phys.pid == PTL_PID_ANY)


typedef struct lcr_ptl_txq {
        int                   idx;
        mpc_list_elem_t       mem_head;
        mpc_common_spinlock_t lock;
        mpc_queue_head_t      queue;
        mpc_mempool_t        *ops_pool;
        lcr_ptl_addr_t        addr;
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        atomic_int_least8_t   is_waiting;
        int32_t               tokens;
        int32_t               num_ops;
#endif
} lcr_ptl_txq_t;

typedef struct lcr_ptl_ep_info {
        lcr_ptl_txq_t        *txq;
} lcr_ptl_ep_info_t;
typedef struct lcr_ptl_ep_info _mpc_lowcomm_endpoint_info_ptl_t;

/*********************************/
/********** PTL RMA  *************/
/*********************************/

typedef struct lcr_ptl_mem {
        void                  *start;          /* Start address of the memory.           */
        uint32_t               muid;           /* Memory Unique Idendifier (for RMA matching). */
        mpc_list_elem_t        elem;           /* Element in list.                       */
        ptl_handle_ct_t        cth;            /* Counter handle.                        */
        ptl_handle_md_t        mdh;            /* Memory Descriptor handle.              */
        ptl_handle_me_t        meh;
        ptl_match_bits_t       match;
        mpc_queue_head_t       pendings;
        size_t                 size;           /* Size of the memory.                    */
        atomic_uint_least64_t *op_count;       /* Sequence number of the last pushed op. */
        ptl_size_t             op_done;
        mpc_common_spinlock_t  lock;
        unsigned               flags;
} lcr_ptl_mem_t;

/*********************************/
/********** PTL OP   *************/
/*********************************/

static ptl_op_t lcr_ptl_atomic_op_table[] = {
        [LCR_ATOMIC_OP_ADD]   = PTL_SUM,
        [LCR_ATOMIC_OP_SWAP]  = PTL_SWAP,
        [LCR_ATOMIC_OP_CSWAP] = PTL_CSWAP,
        [LCR_ATOMIC_OP_AND]   = PTL_BAND,
        [LCR_ATOMIC_OP_OR]    = PTL_BOR,
        [LCR_ATOMIC_OP_XOR]   = PTL_BXOR
};

static inline const char *lcr_ptl_decode_atomic_op(ptl_op_t op_type) {
        switch (op_type) {
        case PTL_SUM: return "PTL_SUM"; break;
        case PTL_SWAP: return "PTL_SWAP"; break;
        case PTL_CSWAP: return "PTL_CSWAP"; break;
        case PTL_BAND: return "PTL_BAND"; break;
        case PTL_BOR: return "PTL_BOR"; break;
        case PTL_BXOR: return "PTL_BXOR"; break;
        default: return "Unknwon PTL op."; break;
        }

        return NULL;
}

/* Operation types. */
typedef enum {
        /* Block operation. */
        LCR_PTL_OP_BLOCK,
        /* Endpoint operations. */
        LCR_PTL_OP_AM_BCOPY,
        LCR_PTL_OP_AM_ZCOPY,
        LCR_PTL_OP_TAG_BCOPY,
        LCR_PTL_OP_TAG_ZCOPY,
        LCR_PTL_OP_TAG_SEARCH,
        LCR_PTL_OP_RMA_PUT,
        LCR_PTL_OP_RMA_GET,
        LCR_PTL_OP_ATOMIC_POST,
        LCR_PTL_OP_ATOMIC_FETCH,
        LCR_PTL_OP_ATOMIC_CSWAP,
        LCR_PTL_OP_RMA_FLUSH,
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        /* Token operations. */
        LCR_PTL_OP_TK_INIT,
        LCR_PTL_OP_TK_REQUEST,
        LCR_PTL_OP_TK_GRANT,
        LCR_PTL_OP_TK_RELEASE,
#endif
} lcr_ptl_op_type_t;

typedef struct lcr_ptl_op {
        ptl_size_t        id;   /* Operation identifier. */
        ptl_handle_md_t   mdh;  /* MD from which to perform the operation. */
        ptl_process_t     addr;
        ptl_hdr_data_t    hdr;
        ptl_pt_index_t    pti;
        lcr_ptl_op_type_t type; /* Type of operation */
        size_t            size; /* Data size of the operation */
        lcr_completion_t *comp; /* Completion callback */
        lcr_ptl_txq_t    *txq;  /* TX Queue where the operation was pushed. */

        union {
                struct {
                        ptl_iovec_t    *iov;
                } iov;
                struct {
                        lcr_tag_context_t *tag_ctx;
                        void              *bcopy_buf;
                        ptl_handle_me_t    meh;
                } tag;
                struct {
                        uint8_t am_id;
                        void   *bcopy_buf;
                } am;
                struct {
                        uint64_t         local_offset;
                        uint64_t         remote_offset;
                        ptl_match_bits_t match;
                        lcr_ptl_mem_t   *lkey;
                        lcr_ptl_mem_t   *rkey;
                } rma;
                struct {
                        uint64_t         remote_offset;
                        ptl_match_bits_t match;
                        lcr_ptl_mem_t   *lkey;
                        lcr_ptl_mem_t   *rkey;
                        ptl_op_t         op;
                        ptl_datatype_t   dt;
                        struct {
                                uint64_t  local_offset;
                        } post;
                        struct {
                                uint64_t        get_local_offset;
                                uint64_t        put_local_offset;
                        } fetch;
                        struct {
                                uint64_t        get_local_offset;
                                uint64_t        put_local_offset;
                                const void     *operand;
                        } cswap;
                } ato;

                struct {
                        ptl_size_t     op_count;
                        ptl_size_t     op_done;
                        lcr_ptl_mem_t *lkey;
                } flush;
        };

        mpc_queue_elem_t  elem;  /* Element in TX queue. */
} lcr_ptl_op_t;

/*********************************/
/********** Operation Context ****/
/*********************************/
/* Context for two-sided operations. */
typedef struct lcr_ptl_ts_ctx {
        ptl_handle_md_t       mdh; /* Eager Memory Descriptor handle. */
        ptl_pt_index_t        pti; /* Portal Table Index. */
        mpc_mempool_t        *block_mp; /* Pool of shaddow buffers. */
        mpc_list_elem_t       bhead; /* Head of block list. */
        atomic_uint_least64_t op_sn;
        atomic_uint_least64_t rma_count;
} lcr_ptl_ts_ctx_t;

/* Context for one-sided operations. */
typedef struct lcr_ptl_os_ctx {
        ptl_handle_md_t       rndv_mdh; /* Rendez-vous Memory Descriptor handle. */
        ptl_handle_me_t       rndv_meh; /* Rendez-vous Memory Entry handle. */
        ptl_handle_ct_t       rndv_cth; /* Rendez-vous Counter handle. */
        ptl_pt_index_t        pti; /* Portals Table Index for RMA. */
        atomic_uint_least32_t mem_uid; /* Unique identifier for RMA registration. */
        atomic_uint_least64_t op_sn; /* Atomic counter for identifying RMA operations. */
        mpc_list_elem_t       mem_head; /* List head of posted Memory. */
        mpc_common_spinlock_t lock;
} lcr_ptl_os_ctx_t;

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
typedef struct lcr_ptl_tk_rsc {
        int                   idx;       /* Resource idx in table. */
        lcr_ptl_addr_t        remote;    /* Full address of remote process. */
        uint16_t              tx_idx;    /* ID of the remote TXQ it is connected to. */
        mpc_queue_elem_t      elem;      /* Element in exhausted queue. */
        int32_t               tk_chunk;  /* Chunck of tokens to grant each time. */
        struct {
                int32_t       requested; /* Number of requested tokens from remote. */
                int32_t       granted;   /* Number of token distributed to remote.  */
                int           scheduled; /* Has grant request been scheduled already? */
        } req;
} lcr_ptl_tk_rsc_t;

typedef struct lcr_ptl_tk_config {
        int max_tokens; /* Maximum number of tokens. */
        int max_chunk;  /* Maximum number of tokens granted per request. */
        int min_chunk;  /* Minimum number of tokens granted per request. */
} lcr_ptl_tk_config_t;

typedef struct lcr_ptl_tk_module {
        lcr_ptl_tk_config_t         config;
        ptl_handle_eq_t             eqh; /* Event Queue. */
        ptl_handle_md_t             mdh; /* Memory Decriptor handle. */
        ptl_pt_index_t              pti; /* Portal Table Index. */
        ptl_handle_me_t             meh; /* Memory Entry handle. */
        mpc_common_spinlock_t       lock;
        struct mpc_common_hashtable rsct; /* Token Resource Hash Table. */
        mpc_mempool_t              *ops;  /* Pool of operations. */
        mpc_queue_head_t            exhausted; /* Queue of exhausted token resources. */
        int32_t                     pool; /* Token pool. */
} lcr_ptl_tk_module_t;
#endif

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
	size_t          max_put;          /**< Max size of a put. */
	size_t          max_get;          /**< Max size of a get. */
        size_t          min_frag_size;    /* Minimum size of a fragment. */
	ptl_ni_limits_t max_limits;       /**< container for Portals thresholds */
} lcr_ptl_iface_config_t;

typedef struct lcr_ptl_rail_info {
        ptl_handle_ni_t             nih;
        lcr_ptl_addr_t              addr; /* Full address of PTL interface. */
        lcr_ptl_iface_config_t      config;
	char                        connection_infos[MPC_COMMON_MAX_STRING_SIZE];
	size_t                      connection_infos_size;
        mpc_mempool_t              *iface_ops; //NOTE: only need for TAG interface.
        lcr_ptl_txq_t              *txqt; /* TX Queue Table. */
        atomic_int_least32_t        num_txqs;
        ptl_handle_eq_t             eqh; /**< Event Queue for Active Message/Tag/RMA errors. */
        mpc_common_spinlock_t       lock;
        lcr_ptl_ts_ctx_t            am;  /* Active Message context. */
        lcr_ptl_ts_ctx_t            tag; /* Tag context. */
        lcr_ptl_os_ctx_t            rma; /* RMA context. */
#if defined(MPC_USE_PORTALS_CONTROL_FLOW)
        lcr_ptl_tk_module_t         tk; /* Token module. */
#endif
        unsigned                    features; /* Instanciated features. */
} lcr_ptl_rail_info_t;
typedef struct lcr_ptl_rail_info _mpc_lowcomm_ptl_rail_info_t;

#if !defined(NDEBUG)
#define lcr_ptl_chk(x) do { int __ret = 0; \
    static int ___env = -1; \
	if(___env == -1) { \
		___env = (getenv("MPC_PTL_DEBUG") != NULL);\
	}\
	if(___env) \
    		mpc_common_debug("%s -> %s (%s:%u)", #x, lcr_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); \
    switch (__ret = x) { \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
        case PTL_OK: break; \
	default: mpc_common_debug_fatal("%s -> %s (%s:%u)", #x, lcr_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); break; \
    } } while (0)
#else
#define lcr_ptl_chk(x) x
#endif

static inline const char * lcr_ptl_event_ni_fail_type_decode(ptl_event_t ev)
{
	switch(ev.ni_fail_type)
	{
        case PTL_NI_OK: return "PTL_NI_OK"; break;
        case PTL_NI_DROPPED: return "PTL_NI_DROPPED"; break;
        case PTL_NI_PERM_VIOLATION: return "PTL_NI_PERM_VIOLATION"; break;
        case PTL_NI_SEGV: return "PTL_NI_SEGV"; break;
        case PTL_NI_UNDELIVERABLE: return "PTL_NI_UNDELIVERABLE"; break;
        case PTL_NI_NO_MATCH: return "PTL_NI_NO_MATCH"; break;
        case PTL_NI_TARGET_INVALID: return "PTL_NI_TARGET_INVALID"; break;
        case PTL_NI_OP_VIOLATION: return "PTL_NI_OP_VIOLATION"; break;
        default:
		return "Portals Event Ni Failed Type not known"; break;
        }
}

static inline const char * lcr_ptl_event_decode(ptl_event_t ev)
{
	switch(ev.type)
	{
	case PTL_EVENT_GET: return "PTL_EVENT_GET"; break;
	case PTL_EVENT_GET_OVERFLOW: return "PTL_EVENT_GET_OVERFLOW"; break;
	
	case PTL_EVENT_PUT: return "PTL_EVENT_PUT"; break;
	case PTL_EVENT_PUT_OVERFLOW: return "PTL_EVENT_PUT_OVERFLOW"; break;
	
	case PTL_EVENT_ATOMIC: return "PTL_EVENT_ATOMIC"; break;
	case PTL_EVENT_ATOMIC_OVERFLOW: return "PTL_EVENT_ATOMIC_OVERFLOW"; break;
	
	case PTL_EVENT_FETCH_ATOMIC: return "PTL_EVENT_FETCH_ATOMIC"; break;
	case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: return "PTL_EVENT_FETCH_ATOMIC_OVERFLOW"; break;
	case PTL_EVENT_REPLY: return "PTL_EVENT_REPLY"; break;
	case PTL_EVENT_SEND: return "PTL_EVENT_SEND"; break;
	case PTL_EVENT_ACK: return "PTL_EVENT_ACK"; break;
	
	case PTL_EVENT_PT_DISABLED: return "PTL_EVENT_PT_DISABLED"; break;
	case PTL_EVENT_LINK: return "PTL_EVENT_LINK"; break;
	case PTL_EVENT_AUTO_UNLINK: return "PTL_EVENT_AUTO_UNLINK"; break;
	case PTL_EVENT_AUTO_FREE: return "PTL_EVENT_AUTO_FREE"; break;
	case PTL_EVENT_SEARCH: return "PTL_EVENT_SEARCH"; break;
	
	default:
		return "Portals Event not known"; break;
	}

	return NULL;
}

static inline const char * lcr_ptl_op_decode(lcr_ptl_op_t *op) 
{
        switch(op->type) {
        case LCR_PTL_OP_BLOCK: return "LCR_PTL_OP_BLOCK"; break;
        case LCR_PTL_OP_AM_BCOPY: return "LCR_PTL_OP_AM_BCOPY"; break;
        case LCR_PTL_OP_AM_ZCOPY: return "LCR_PTL_OP_AM_ZCOPY"; break;
        case LCR_PTL_OP_RMA_PUT: return "LCR_PTL_OP_RMA_PUT"; break;
        case LCR_PTL_OP_RMA_GET: return "LCR_PTL_OP_RMA_GET"; break;
        case LCR_PTL_OP_RMA_FLUSH: return "LCR_PTL_OP_RMA_FLUSH"; break;
        case LCR_PTL_OP_TAG_ZCOPY: return "LCR_PTL_OP_TAG_ZCOPY"; break;
        case LCR_PTL_OP_TAG_BCOPY: return "LCR_PTL_OP_TAG_BCOPY"; break;
        case LCR_PTL_OP_TAG_SEARCH: return "LCR_PTL_OP_TAG_SEARCH"; break;
#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
        case LCR_PTL_OP_TK_INIT: return "LCR_PTL_OP_TK_INIT"; break;
        case LCR_PTL_OP_TK_REQUEST: return "LCR_PTL_OP_TK_REQUEST"; break;
        case LCR_PTL_OP_TK_GRANT: return "LCR_PTL_OP_TK_GRANT"; break;
        case LCR_PTL_OP_TK_RELEASE: return "LCR_PTL_OP_TK_RELEASE"; break;
#endif
        default:
                return "PTL OP no known."; break;
        }

        return NULL;
}

static inline const char * lcr_ptl_rc_decode(int rc)
{
	switch(rc)
	{
		case PTL_OK: return "PTL_OK"; break;
		case PTL_FAIL: return "PTL_FAIL"; break;
		case PTL_IN_USE: return "PTL_IN_USE"; break;
		case PTL_NO_INIT: return "PTL_NO_INIT"; break;
		case PTL_IGNORED: return "PTL_IGNORED"; break;
		case PTL_PID_IN_USE: return "PTL_PID_IN_USE"; break;
		case PTL_INTERRUPTED: return "PTL_INTERRUPTED"; break;
		
		case PTL_NO_SPACE: return "PTL_NO_SPACE"; break;
		case PTL_ARG_INVALID: return "PTL_ARG_INVALID"; break;
		
		case PTL_PT_IN_USE: return "PTL_PT_IN_USE"; break;
		case PTL_PT_FULL: return "PTL_PT_FULL"; break;
		case PTL_PT_EQ_NEEDED: return "PTL_PT_EQ_NEEDED"; break;
		
		case PTL_LIST_TOO_LONG: return "PTL_LIST_TOO_LONG"; break;
		case PTL_EQ_EMPTY: return "PTL_EQ_EMPTY"; break;
		case PTL_EQ_DROPPED: return "PTL_EQ_DROPPED"; break;
		case PTL_CT_NONE_REACHED: return "PTL_CT_NONE_REACHED"; break;
		default:
		{
			char* buf = (char*)sctk_malloc(sizeof(char) * 40);
			snprintf(buf, 40, "Portals return code not known: %d", rc); 
			return buf;
			break;
		}
	}
	return NULL;
};

/**
 * De-serialize an object an map it into its base struct.
 *
 * \param[in] inval the serialized data, as a string or NULL if the outvallen is not large enough
 * \param[out] outval the effective struct to fill
 * \param[in] outvallen size of the final struct
 * \return Size of the string if serialization succeeded, -1 otherwise
 */
static inline int lcr_ptl_data_deserialize ( const char *inval, void *outval, unsigned int outvallen )
{
    size_t i;
    char *ret = ( char * ) outval;

    if ( outvallen != strlen ( inval ) / 2 )
    {
        return -1;
    }

    for ( i = 0 ; i < outvallen ; ++i )
    {
        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] = *inval - '0';
        }
        else
        {
            ret[i] = *inval - 'a' + 10;
        }

        inval++;

        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] |= ( ( *inval - '0' ) << 4 );
        }
        else
        {
            ret[i] |= ( ( *inval - 'a' + 10 ) << 4 );
        }

        inval++;
    }

    return outvallen;
}

/**
 * Serialize a object/struct into a string.
 * \param[in] inval the object to serialize
 * \param[in] invallen input object size
 * \param[out] outval the string version of the object
 * \param[in] outvallen the max string length
 * \return the effective string size if succeeded, -1 otherwise 
 */
static inline int lcr_ptl_data_serialize ( const void *inval, size_t invallen, char *outval, size_t outvallen )
{
    static unsigned char encodings[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', \
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    size_t i;
	if ( invallen * 2 + 1 > outvallen )
    {
        return -1;
    }

    for ( i = 0; i < invallen; i++ )
    {
        outval[2 * i] = encodings[ ( ( unsigned char * ) inval ) [i] & 0xf];
        outval[2 * i + 1] = encodings[ ( ( unsigned char * ) inval ) [i] >> 4];
    }

    outval[invallen * 2] = '\0';
    return (invallen * 2);
}

static inline char * __ptl_get_rail_callback_name(int rail_number, char * buff, int bufflen)
{
	snprintf(buff, bufflen, "portals-callback-%d", rail_number);
	return buff;
}

static inline int lcr_ptl_do_op(lcr_ptl_op_t *op) {

        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_common_debug("LCR PTL: op type=%s, nid=%llu, pid=%llu, pti=%d, "
                         "id=%d, mdh=%llu, txq=%p, size=%llu, hdr=0x%08x.", 
                         lcr_ptl_op_decode(op), op->addr.phys.nid,
                         op->addr.phys.pid, op->pti, op->id,
                         *(uint64_t*)&op->mdh, op->txq, op->size, op->hdr);

        switch(op->type) {
        case LCR_PTL_OP_AM_BCOPY:
                lcr_ptl_chk(PtlPut(op->mdh,
                                   (ptl_size_t)op->am.bcopy_buf, /* local offset */
                                   op->size,
                                   PTL_ACK_REQ,
                                   op->addr,
                                   op->pti,
                                   0,
                                   0, /* remote offset */
                                   op, 
                                   op->hdr	
                                  ));
                break;
        case LCR_PTL_OP_AM_ZCOPY:
                lcr_ptl_chk(PtlPut(op->mdh,
                                   0,
                                   op->size,
                                   PTL_ACK_REQ,
                                   op->addr,
                                   op->pti,
                                   0,
                                   0,
                                   op,
                                   op->hdr	
                                  ));
                break;
        case LCR_PTL_OP_RMA_PUT: 
                mpc_common_debug("LCR PTL: local key. lkey size=%llu, addr=%p, "
                                 "local offset=%llu, remote offset=%llu, "
                                 "cth=%llu, mdh=%llu.", op->rma.lkey->size, 
                                 op->rma.lkey->start, op->rma.local_offset,
                                 op->rma.remote_offset,
                                 *(uint64_t*)&op->rma.lkey->cth,
                                 *(uint64_t*)&op->rma.lkey->mdh);
                mpc_common_debug("LCR PTL: remote key. addr=%p, match=%llu", op->rma.rkey->start,
                                 (uint64_t)op->rma.match);
                lcr_ptl_chk(PtlPut(op->rma.lkey->mdh, 
                                   op->rma.local_offset, 
                                   op->size, 
                                   PTL_ACK_REQ,
                                   op->addr,
                                   op->pti, 
                                   op->rma.match,
                                   op->rma.remote_offset,
                                   op,
                                   0
                                  ));
                break;
        case LCR_PTL_OP_RMA_GET: 
                mpc_common_debug("LCR PTL: local key.  lkey size=%llu, addr=%p, "
                                 "local offset=%llu, remote offset=%llu, "
                                 "cth=%llu, mdh=%llu.", op->rma.lkey->size, 
                                 op->rma.lkey->start, op->rma.local_offset,
                                 op->rma.remote_offset,
                                 *(uint64_t*)&op->rma.lkey->cth,
                                 *(uint64_t*)&op->rma.lkey->mdh);
                mpc_common_debug("LCR PTL: remote key. addr=%p, match=%llu", 
                                 op->rma.rkey->start, (uint64_t)op->rma.match);
                lcr_ptl_chk(PtlGet(op->rma.lkey->mdh,
                                   op->rma.local_offset,
                                   op->size,
                                   op->addr,
                                   op->pti,
                                   op->rma.match,
                                   op->rma.remote_offset,
                                   op	
                                  ));
                break;
        case LCR_PTL_OP_RMA_FLUSH:
                if (op->flush.lkey->op_done < op->flush.op_count) {
                        rc = MPC_LOWCOMM_IN_PROGRESS;
                        break;
                }
                break;
        case LCR_PTL_OP_ATOMIC_POST:
                mpc_common_debug("LCR PTL: atomic post operation. type=%s, lkey "
                                 "size=%llu, addr=%p, local offset=%llu, "
                                 "remote offset=%llu, cth=%llu, mdh=%llu, "
                                 "match=%llu.", 
                                 lcr_ptl_decode_atomic_op(lcr_ptl_atomic_op_table[op->ato.op]),
                                 op->ato.lkey->size, op->ato.lkey->start, op->ato.post.local_offset,
                                 op->ato.remote_offset, *(uint64_t*)&op->ato.lkey->cth,
                                 *(uint64_t*)&op->ato.lkey->mdh, (uint64_t)op->ato.match);
                lcr_ptl_chk(PtlAtomic(op->mdh, 
                                      op->ato.post.local_offset, 
                                      op->size, 
                                      PTL_ACK_REQ, 
                                      op->addr, 
                                      op->pti, 
                                      op->ato.match, 
                                      op->ato.remote_offset, 
                                      op, 
                                      0, 
                                      op->ato.op, 
                                      op->ato.dt));
                break;
        case LCR_PTL_OP_ATOMIC_FETCH:
                mpc_common_debug("LCR PTL: atomic fetch operation. type=%s, lkey "
                                 "size=%llu, addr=%p, get local offset=%llu, "
                                 "put local offset=%llu, remote offset=%llu, cth=%llu, "
                                 "mdh=%llu, match=%llu.", 
                                 lcr_ptl_decode_atomic_op(lcr_ptl_atomic_op_table[op->ato.op]),
                                 op->ato.lkey->size, op->ato.lkey->start, 
                                 op->ato.fetch.get_local_offset, op->ato.fetch.put_local_offset,
                                 op->ato.remote_offset, *(uint64_t*)&op->ato.lkey->cth,
                                 *(uint64_t*)&op->ato.lkey->mdh, (uint64_t)op->ato.match);
                lcr_ptl_chk(PtlFetchAtomic(op->mdh, 
                                           op->ato.fetch.get_local_offset, 
                                           op->mdh, 
                                           op->ato.fetch.put_local_offset,
                                           op->size, 
                                           op->addr, 
                                           op->pti, 
                                           op->ato.match, 
                                           op->ato.remote_offset, 
                                           op, 
                                           0, 
                                           op->ato.op, 
                                           op->ato.dt));
                break;
        case LCR_PTL_OP_ATOMIC_CSWAP:
                mpc_common_debug("LCR PTL: atomic cswap operation. type=%s, lkey "
                                 "size=%llu, addr=%p, get local offset=%llu, "
                                 "put local offset=%llu, remote offset=%llu, cth=%llu, "
                                 "mdh=%llu, match=%llu.", 
                                 lcr_ptl_decode_atomic_op(lcr_ptl_atomic_op_table[op->ato.op]),
                                 op->ato.lkey->size, op->ato.lkey->start, 
                                 op->ato.cswap.get_local_offset, op->ato.cswap.put_local_offset,
                                 op->ato.remote_offset, *(uint64_t*)&op->ato.lkey->cth,
                                 *(uint64_t*)&op->ato.lkey->mdh, (uint64_t)op->ato.match);
                lcr_ptl_chk(PtlSwap(op->mdh, 
                                    op->ato.cswap.get_local_offset, 
                                    op->mdh, 
                                    op->ato.cswap.put_local_offset,
                                    op->size, 
                                    op->addr, 
                                    op->pti, 
                                    op->ato.match, 
                                    op->ato.remote_offset, 
                                    op, 
                                    0, 
                                    op->ato.cswap.operand,
                                    op->ato.op, 
                                    op->ato.dt));
                break;
#if defined (MPC_USE_PORTALS_CONTROL_FLOW) 
        case LCR_PTL_OP_TK_INIT: 
        case LCR_PTL_OP_TK_GRANT: 
        case LCR_PTL_OP_TK_REQUEST: 
        case LCR_PTL_OP_TK_RELEASE: 
                lcr_ptl_chk(PtlPut(op->mdh, 
                                   0, /* No local offset */
                                   0, /* Size is 0 */
                                   PTL_ACK_REQ,
                                   op->addr,
                                   op->pti, 
                                   0,
                                   0,
                                   op,
                                   op->hdr
                                  ));
                break;
#endif
        default:
                mpc_common_debug_error("LCR PTL: invalid operation.");
                break;
        }

        return rc;
}

static inline int lcr_ptl_post_rma_resources(lcr_ptl_rail_info_t *srail,
                                             ptl_match_bits_t match,
                                             ptl_addr_t start,
                                             ptl_size_t length,
                                             ptl_handle_ct_t *cth,
                                             ptl_handle_md_t *mdh,
                                             ptl_handle_me_t *meh)
{
        ptl_md_t md;
        ptl_me_t me;

        lcr_ptl_chk(PtlCTAlloc(srail->nih, cth));

        md = (ptl_md_t) {
                .ct_handle = *cth,
                .eq_handle = srail->eqh,
                .length = length,
                .start  = start,
                .options = PTL_MD_EVENT_SUCCESS_DISABLE |
                        PTL_MD_EVENT_SEND_DISABLE       |
                        PTL_MD_EVENT_CT_ACK             | 
                        PTL_MD_EVENT_CT_REPLY,
        };
        lcr_ptl_chk(PtlMDBind(srail->nih, &md, mdh));

        me = (ptl_me_t) {
                .ct_handle = PTL_CT_NONE,
                .ignore_bits = 0,
                .match_bits  = match,
                .match_id    = {
                        .phys.nid = PTL_NID_ANY, 
                        .phys.pid = PTL_PID_ANY
                },
                .uid         = PTL_UID_ANY,
                .min_free    = 0,
                .options     = PTL_ME_OP_PUT        | 
                        PTL_ME_OP_GET               |
                        PTL_ME_EVENT_LINK_DISABLE   | 
                        PTL_ME_EVENT_UNLINK_DISABLE |
                        PTL_ME_EVENT_SUCCESS_DISABLE,
                .start       = start,
                .length      = length 
        };

        lcr_ptl_chk(PtlMEAppend(srail->nih,
                                srail->rma.pti,
                                &me,
                                PTL_PRIORITY_LIST,
                                srail,
                                meh
                               ));

        return MPC_LOWCOMM_SUCCESS;
}


#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
static inline int lcr_ptl_txq_needs_tokens(lcr_ptl_txq_t *txq, int token_num) {
        int_least8_t zero = 0;
        return (token_num <= 0 && atomic_compare_exchange_strong(&txq->is_waiting, &zero, 1));
}

static inline int lcr_ptl_create_token_request(lcr_ptl_rail_info_t *srail,
                                               lcr_ptl_txq_t *txq, 
                                               lcr_ptl_op_t **op_p) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_op_t *op;

        op = mpc_mpool_pop(srail->tk.ops);
        if (op == NULL) {
                mpc_common_debug("LCR PTL: could not allocate "
                                 "token operation.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }

        _lcr_ptl_init_op_common(op, 0, srail->tk.mdh, 
                                txq->addr.id, txq->addr.pte.tk, 
                                LCR_PTL_OP_TK_REQUEST, 0, NULL, 
                                txq);

        LCR_PTL_CTRL_HDR_SET(op->hdr, op->type, 
                             txq->idx, 
                             txq->num_ops);

        *op_p = op;
err:
        return rc;
}

static inline int lcr_ptl_create_token_grant(lcr_ptl_rail_info_t *srail,
                                             lcr_ptl_tk_rsc_t *rsc, 
                                             lcr_ptl_op_t **op_p) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_ptl_op_t *op;

        op = mpc_mpool_pop(srail->tk.ops);
        if (op == NULL) {
                mpc_common_debug("LCR PTL: could not allocate "
                                 "token operation.");
                rc = MPC_LOWCOMM_NO_RESOURCE;
                goto err;
        }

        _lcr_ptl_init_op_common(op, 0, srail->tk.mdh, 
                                rsc->remote.id, 
                                rsc->remote.pte.tk, 
                                LCR_PTL_OP_TK_GRANT, 
                                0, NULL, NULL);

        LCR_PTL_CTRL_HDR_SET(op->hdr, op->type, 
                             rsc->tx_idx, 
                             rsc->req.granted);

        *op_p = op;
err:
        return rc;
}

static inline int lcr_ptl_post(lcr_ptl_txq_t *txq, 
                               lcr_ptl_op_t *op, 
                               int *token_num)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        mpc_common_spinlock_lock(&txq->lock);
        if (txq->tokens <= 0 || !mpc_queue_is_empty(&txq->queue)) {
                *token_num = txq->tokens;
                txq->num_ops++;
                mpc_queue_push(&txq->queue, &op->elem);

                goto txq_unlock;
        }
        txq->tokens--;

        rc = lcr_ptl_do_op(op);

txq_unlock:
        mpc_common_spinlock_unlock(&txq->lock);

        return rc;
}
#endif

static inline void lcr_ptl_complete_op(lcr_ptl_op_t *op) {
        mpc_common_debug("LCR PTL: complete op. id=%llu, size=%llu, "
                         "nid=%lu, pid=%lu, pti=%d, type=%s, hdr=0x%08x.",
                         op->id, op->size, op->addr.phys.nid,
                         op->addr.phys.pid, op->pti, lcr_ptl_op_decode(op),
                         op->hdr);

        mpc_mpool_push(op);
}

ptl_size_t lcr_ptl_poll_mem(lcr_ptl_mem_t *mem);

int lcr_ptl_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr);

int lcr_ptl_iface_is_reachable(sctk_rail_info_t *rail, uint64_t uid);

int lcr_ptl_iface_init(sctk_rail_info_t *rail, unsigned flags);

void lcr_ptl_iface_fini(sctk_rail_info_t* rail);

ssize_t lcr_ptl_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep, 
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              unsigned flags);

int lcr_ptl_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                          uint8_t id,
                          const void *header,
                          unsigned header_length,
                          const struct iovec *iov,
                          size_t iovcnt,
                          unsigned flags,
                          lcr_completion_t *comp);

ssize_t lcr_ptl_send_tag_bcopy(_mpc_lowcomm_endpoint_t *ep,
                               lcr_tag_t tag,
                               uint64_t imm,
                               lcr_pack_callback_t pack,
                               void *arg,
                               unsigned cflags);

int lcr_ptl_send_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_tag_t tag,
                           uint64_t imm,
                           const struct iovec *iov,
                           size_t iovcnt,
                           unsigned cflags,
                           lcr_completion_t *ctx);

int lcr_ptl_post_tag_zcopy(sctk_rail_info_t *rail,
                           lcr_tag_t tag, lcr_tag_t ign_tag,
                           const struct iovec *iov, 
                           size_t iovcnt, /* only one iov supported */
                           unsigned flags,
                           lcr_tag_context_t *ctx); 

int lcr_ptl_unpost_tag_zcopy(sctk_rail_info_t *rail,
                             lcr_tag_t tag);

int lcr_ptl_send_put_bcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_pack_callback_t pack,
                           void *arg,
                           uint64_t remote_addr,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key);

int lcr_ptl_send_get_bcopy(_mpc_lowcomm_endpoint_t *ep,
                           lcr_pack_callback_t pack,
                           void *arg,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key);

int lcr_ptl_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *ctx);

int lcr_ptl_send_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *ctx);

int lcr_ptl_get_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
                          lcr_tag_t tag,
                          uint64_t local_offset,
                          uint64_t remote_offset,
                          size_t size,
                          lcr_completion_t *ctx);

int lcr_ptl_atomic_post(_mpc_lowcomm_endpoint_t *ep,
                        uint64_t local_offset,
                        uint64_t remote_offset,
                        lcr_atomic_op_t op_type,
                        lcr_memp_t *local_key,
                        lcr_memp_t *remote_key,
                        size_t size,
                        lcr_completion_t *comp);

int lcr_ptl_atomic_fetch(_mpc_lowcomm_endpoint_t *ep,
                         uint64_t get_local_offset,
                         uint64_t put_local_offset,
                         uint64_t remote_offset,
                         lcr_atomic_op_t op_type,
                         lcr_memp_t *local_key,
                         lcr_memp_t *remote_key,
                         size_t size,
                         lcr_completion_t *comp);

int lcr_ptl_atomic_cswap(_mpc_lowcomm_endpoint_t *ep,
                         uint64_t get_local_offset,
                         uint64_t put_local_offset,
                         uint64_t remote_offset,
                         lcr_atomic_op_t op_type,
                         lcr_memp_t *local_key,
                         lcr_memp_t *remote_key,
                         uint64_t compare,
                         size_t size,
                         lcr_completion_t *comp);

int lcr_ptl_ep_flush(_mpc_lowcomm_endpoint_t *ep,
                     unsigned flags);

void lcr_ptl_connect_on_demand(struct sctk_rail_info_s *rail, 
                               uint64_t dest);

int lcr_ptl_mem_register(struct sctk_rail_info_s *rail, 
                          struct sctk_rail_pin_ctx_list *list, 
                          void * addr, size_t size, unsigned flags);
int lcr_ptl_mem_unregister(struct sctk_rail_info_s *rail, 
                            struct sctk_rail_pin_ctx_list *list);
int lcr_ptl_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest);

int lcr_ptl_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest);

int lcr_ptl_iface_progress(sctk_rail_info_t *rail);

#if defined (MPC_USE_PORTALS_CONTROL_FLOW)
int lcr_ptl_tk_progress_pending_ops(lcr_ptl_rail_info_t *srail,
                                    lcr_ptl_txq_t *txq, int *count);

int lcr_ptl_tk_release_rsc_token(lcr_ptl_tk_module_t *tk,
                                 ptl_process_t remote, 
                                 int32_t num_pendings);

int lcr_ptl_iface_progress_tk(lcr_ptl_rail_info_t *srail);

int lcr_ptl_tk_distribute_tokens(lcr_ptl_rail_info_t *srail);

int lcr_ptl_tk_init(ptl_handle_ni_t nih, lcr_ptl_tk_module_t *tk, 
                    struct _mpc_lowcomm_config_struct_net_driver_portals *ptl_driver_config);

int lcr_ptl_tk_fini(ptl_handle_ni_t nih, lcr_ptl_tk_module_t *tk);
#endif

#endif
