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

#ifndef SCTK_PTL_IFACE_H_
#define SCTK_PTL_IFACE_H_

#include <portals4.h>
#include <mpc_common_types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "sctk_ptl_types.h"
#include "mpc_common_debug.h"
#include "rail.h"
#include <sctk_alloc.h>
#include <mpc_lowcomm_types.h>

#if !defined(NDEBUG)
#define SCTK_PTL_STR_PROT(i) __sctk_ptl_str_prot[i]
#define SCTK_PTL_STR_TYPE(i) __sctk_ptl_str_type[i]

/** global static array to stringify type ID (through SCTK_PTL_STR_TYPE macro) */
static __UNUSED__ char * __sctk_ptl_str_type[] = {
	"SCTK_PTL_TYPE_RECOVERY", 
	"SCTK_PTL_TYPE_CM",
	"SCTK_PTL_TYPE_RDMA",
	"SCTK_PTL_TYPE_STD",
	"SCTK_PTL_TYPE_OFFCOLL",
	"SCTK_PTL_TYPE_NONE",
	"SCTK_PTL_TYPE_NB",
};

/** global static array to stringify protocol ID (through SCTK_PTL_STR_PROT macro) */
static __UNUSED__ char * __sctk_ptl_str_prot[] = {

	"SCTK_PTL_PROT_EAGER",
	"SCTK_PTL_PROT_RDV",
	"SCTK_PTL_PROT_NONE",
	"SCTK_PTL_PROT_NB"
};

#define sctk_ptl_chk(x) do { int __ret = 0; \
    static int ___env = -1; \
	if(___env == -1) { \
		___env = (getenv("MPC_PTL_DEBUG") != NULL);\
	}\
	if(___env) \
    		mpc_common_debug("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); \
    switch (__ret = x) { \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
        case PTL_OK: break; \
	default: mpc_common_debug_fatal("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); break; \
    } } while (0)
#else
#define sctk_ptl_chk(x) x
#define SCTK_PTL_STR_PROT(i) "`OPT-OUT`"
#define SCTK_PTL_STR_TYPE(i) "`OPT-OUT`"
#endif

/**************************************************************/
/************************** FUNCTIONS *************************/
/**************************************************************/

/* Hardware-related init */
sctk_ptl_rail_info_t sctk_ptl_hardware_init();
void sctk_ptl_hardware_fini(sctk_ptl_rail_info_t *srail);

/* Software-related init */
void sctk_ptl_software_init(sctk_ptl_rail_info_t*, size_t);
void sctk_ptl_software_fini(sctk_ptl_rail_info_t*);
#ifdef MPC_LOWCOMM_PROTOCOL
int lcr_ptl_software_init(sctk_ptl_rail_info_t* srail, size_t comm_dims);
int lcr_ptl_software_minit(sctk_ptl_rail_info_t* srail, size_t comm_dims);
void lcr_ptl_software_fini(sctk_ptl_rail_info_t* srail);
void lcr_ptl_software_mfini(sctk_ptl_rail_info_t* srail);
#endif

/* Portals table management */
#ifdef MPC_LOWCOMM_PROTOCOL
void lcr_ptl_pte_idx_register(sctk_ptl_rail_info_t* srail, ptl_pt_index_t idx, mpc_lowcomm_communicator_id_t comm_id);
sctk_ptl_pte_t *lcr_ptl_pte_idx_to_pte(sctk_ptl_rail_info_t *srail, ptl_pt_index_t idx);
#endif
void sctk_ptl_pte_create(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, ptl_pt_index_t requested_index, size_t key);
void sctk_ptl_pte_release(sctk_ptl_rail_info_t* srail, ptl_pt_index_t requested_index);
mpc_lowcomm_communicator_id_t sctk_ptl_pte_idx_to_comm_id(sctk_ptl_rail_info_t* srail, ptl_pt_index_t idx);

/* ME management */
sctk_ptl_local_data_t* sctk_ptl_me_create(void*, size_t, sctk_ptl_id_t, sctk_ptl_matchbits_t, sctk_ptl_matchbits_t, int);
sctk_ptl_local_data_t* sctk_ptl_me_create_with_cnt(sctk_ptl_rail_info_t* srail, void*, size_t, sctk_ptl_id_t, sctk_ptl_matchbits_t, sctk_ptl_matchbits_t, int);
void sctk_ptl_me_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t*, sctk_ptl_pte_t*);
void sctk_ptl_me_release(sctk_ptl_local_data_t*);
void sctk_ptl_me_free(sctk_ptl_local_data_t*, int);
void sctk_ptl_me_feed(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, size_t me_size, int nb, int list, char type, char protocol);
#define SCTK_PTL_ME_PROBE_ONLY PTL_SEARCH_ONLY
#define SCTK_PTL_ME_PROBE_DEL PTL_SEARCH_DELETE
int sctk_ptl_me_emit_probe(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, sctk_ptl_local_data_t* user, int probe_level );


/* event management */
void sctk_ptl_ct_alloc(sctk_ptl_rail_info_t* srail, sctk_ptl_cnth_t* cnth_ptr);
void sctk_ptl_ct_free(sctk_ptl_cnth_t cth);
void sctk_ptl_ct_wait_thrs(sctk_ptl_cnth_t cth, size_t thrs, sctk_ptl_cnt_t* ev);
size_t sctk_ptl_ct_get(sctk_ptl_cnth_t cth);

/* MD management */
sctk_ptl_local_data_t* sctk_ptl_md_create(sctk_ptl_rail_info_t* srail, void*, size_t, int);
sctk_ptl_local_data_t* sctk_ptl_md_create_with_cnt(sctk_ptl_rail_info_t* srail, void* start, size_t length, int flags);
void sctk_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t*);
#ifdef MPC_LOWCOMM_PROTOCOL
int lcr_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t*);
#endif
void sctk_ptl_md_release(sctk_ptl_local_data_t*);

/* Request management */
int sctk_ptl_emit_get(sctk_ptl_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_pte_t*, sctk_ptl_matchbits_t, size_t, size_t, void*);
int sctk_ptl_emit_put(sctk_ptl_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_pte_t*, sctk_ptl_matchbits_t, size_t, size_t, size_t, void*);
int sctk_ptl_emit_atomic(
		sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote,
		sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, 
		size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr);
int sctk_ptl_emit_fetch_atomic(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user,
		size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match,
		size_t local_getoff, size_t local_putoff, size_t remote_off,
		sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr);
int sctk_ptl_emit_swap(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user,
		size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match,
		size_t local_getoff, size_t local_putoff, size_t remote_off,
		const void* cmp, sctk_ptl_rdma_type_t type, void* user_ptr);
int sctk_ptl_emit_cnt_incr(sctk_ptl_cnth_t target_cnt, size_t incr);
int sctk_ptl_emit_cnt_set(sctk_ptl_cnth_t target_cnt, size_t val);

/* triggered request management */
int sctk_ptl_emit_trig_get(
		sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte,
		sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr,
		sctk_ptl_cnth_t cnt, size_t threshold);
int sctk_ptl_emit_trig_put(
		sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte,
		sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, size_t extra,
		void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold);
int sctk_ptl_emit_trig_atomic(
		sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte,
		sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, sctk_ptl_rdma_op_t op,
		sctk_ptl_rdma_type_t type, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold);
int sctk_ptl_emit_trig_fetch_atomic(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, 
		sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, 
		size_t local_putoff, size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type,
		void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold);
int sctk_ptl_emit_trig_swap(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote,
		sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, size_t local_putoff, size_t remote_off,
		const void* cmp, sctk_ptl_rdma_type_t type, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold);

/* local triggered ops (counters) */
int sctk_ptl_emit_trig_cnt_incr(sctk_ptl_cnth_t target_cnt, size_t incr, sctk_ptl_cnth_t tracked, size_t threshold);
int sctk_ptl_emit_trig_cnt_set(sctk_ptl_cnth_t target_cnt, size_t val, sctk_ptl_cnth_t tracked, size_t threshold);

/**************************************************************/
/*************************** HELPERS **************************/
/**************************************************************/
sctk_ptl_id_t sctk_ptl_self(sctk_ptl_rail_info_t* srail);

static inline const char * sctk_ptl_rc_decode(int rc)
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
}

static inline const char * sctk_ptl_event_decode(sctk_ptl_event_t ev)
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

static inline const char * sctk_ptl_ni_fail_decode(sctk_ptl_event_t ev)
{
	switch(ev.ni_fail_type)
	{
		case PTL_NI_OK: return "PTL_NI_OK"; break;
		case PTL_NI_UNDELIVERABLE: return "PTL_NI_UNDELIVERABLE"; break;
		case PTL_NI_PT_DISABLED: return "PTL_NI_PT_DISABLED"; break;
		case PTL_NI_DROPPED: return "PTL_NI_DROPPED"; break;
		case PTL_NI_PERM_VIOLATION: return "PTL_NI_PERM_VIOLATION"; break;
		case PTL_NI_OP_VIOLATION: return "PTL_NI_OP_VIOLATION"; break;
		case PTL_NI_SEGV: return "PTL_NI_SEGV"; break;
		case PTL_NI_NO_MATCH: return "PTL_NI_NO_MATCH"; break;
		default:
			return "Portals NI code not known"; break;
	}
	return NULL;
}

/**
 * De-serialize an object an map it into its base struct.
 *
 * \param[in] inval the serialized data, as a string or NULL if the outvallen is not large enough
 * \param[out] outval the effective struct to fill
 * \param[in] outvallen size of the final struct
 * \return Size of the string if serialization succeeded, -1 otherwise
 */
static inline int sctk_ptl_data_deserialize ( const char *inval, void *outval, unsigned int outvallen )
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
static inline int sctk_ptl_data_serialize ( const void *inval, size_t invallen, char *outval, size_t outvallen )
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

/**
 * Attempt to progress pending requests by getting the next MD-related event.
 *
 * \param[out] ev the event to fill
 * \return <ul>
 * <li><b>PTL_OK</b> if an event has been found with no error</li>
 * <li><b>PTL_EQ_EMPTY</b> if not event are present in the queue </li>
 * <li><b>PTL_EQ_DROPPED</b> if a event has been polled but at least one event has been dropped due to a lack of space in the queue</lib>
 * <li><b>any other code</b> is an error</li>
 * </ul>
 */
static inline int sctk_ptl_eq_poll_md(sctk_ptl_rail_info_t* srail, sctk_ptl_event_t* ev)
{
	int ret;
	
	assert(ev);
	ret = PtlEQGet(srail->mds_eq, ev);
	sctk_ptl_chk(ret);
	
	return ret;
}

/**
 * Poll asynchronous notification from the NIC, about registered memory regions.
 *
 * \param[in] pte the Portals entry which contains the EQ to process
 * \param[out] ev the event to fill
 * \return <ul>
 * <li><b>PTL_OK</b> if an event has been found with no error</li>
 * <li><b>PTL_EQ_EMPTY</b> if not event are present in the queue </li>
 * <li><b>PTL_EQ_DROPPED</b> if a event has been polled but at least one event has been dropped due to a lack of space in the queue</lib>
 * <li><b>any other code</b> is an error</li>
 * </ul>
 */
static inline int sctk_ptl_eq_poll_me(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, sctk_ptl_event_t* ev)
{
	int ret;
	
	assert(ev && pte && srail);
	ret = PtlEQGet(pte->eq, ev);
	sctk_ptl_chk(ret);

	return ret;
}

/**
 * Format the match_bits in a human-readable form and stores in a pre-allocated buffer.
 * \param[out] buf the string buffer (also returned)
 * \param[in] s the buffer max size
 * \param[in] m the match_bits to format
 * \return buf (first parameter)
 */
static inline const char * __sctk_ptl_match_str(char*buf, size_t s, ptl_match_bits_t m)
{
	sctk_ptl_matchbits_t m2;
	m2.raw = m;

	switch(m2.data.type)
	{
		case MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE:
		case MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE:
			snprintf(buf, s, "it:%u[%u]:%u", (unsigned int)m2.offload.iter, (int)m2.offload.dir, (int)m2.offload.type);
			break;
		default:
			snprintf(buf, s, "%u:%d[%u]:%d", m2.data.rank, m2.data.tag, (int)m2.data.uid, (int)m2.data.type);
			break;
	}

	return buf;
}

/**
 * Static function (only for large protocols) in charge of cutting a GET request if it exceeed a given size.
 *
 * Especially, this allow use to handle msg larger than Portals ME capacity.
 * This function tries to load-balance requests. For exaple, if we have a msg size = MAX_SIZE + 4,
 * the driver should create two chunks of (MAX_SIZE/2)+2 bytes, instead of one with MAX_SIZE and one with 4 bytes.
 *
 * That is why we need the rest. It contains number of bytes that can't be equally distributed between
 * the chunks. It is the responsability of the called to increment by one any chunk size when emiting GET requests.
 * The condition (num_chunk < num_rest) ? sz+1 : sz is enought for that.
 *
 * \param[in] srail the Portals rail
 * \param[in] msg the mesage (used for the size contained in it)
 * \param[out] sz_out a chunk size, for one GET request
 * \param[out] nb_out number of computed chunks
 * \param[out] rest_out number of bytes not spread between chunks
 */
static inline void sctk_ptl_compute_chunks(sctk_ptl_rail_info_t* srail, size_t data_sz, size_t* sz_out, size_t* nb_out, size_t* rest_out)
{
	size_t total = data_sz;
	size_t size = 0;
	size_t nb = 1;

	assert(srail);
	assert(sz_out);
	assert(nb_out);
	assert(rest_out);

	/* first, if the msg size reached the cufoff, the chunk size if a simple division */
	size = (total > srail->cutoff) ? (size_t)(total / SCTK_PTL_MAX_RDV_BLOCKS) : total;
	nb   = (size < total) ? SCTK_PTL_MAX_RDV_BLOCKS : 1;
		
	/* if sub-blocks are larger than the largest possible ME/MD
	 * Set dimension to the system one.
	 * NOTE: This can lead to resource exhaustion if srail->max_mr << MSG_SIZE
	 */
	if(size >= srail->max_mr)
	{
		/* compute how many chunks we need (add one if necessary if not well-balanced) */
		nb = (size_t)(total / srail->max_mr) + ((total % srail->max_mr == 0) ? 0 : 1);
		assert(nb > 0);
		/* compute the effective size per chunk (load-balancing if an extra-chunk is needed */
		size = (size_t)(total / nb);
	}

	assert(nb > 0);

	*sz_out   = size;
	*nb_out   = nb;
	*rest_out = (total > 0) ? total % size : 0; /* special care (very rare) where data_sz equals to zero */
}
#endif
