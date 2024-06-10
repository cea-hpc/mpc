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

#ifndef SCTK_PTL_AM_IFACE_H_
#define SCTK_PTL_AM_IFACE_H_

#ifdef MPC_USE_PORTALS

#include <portals4.h>
#include <stddef.h>
#include <mpc_common_types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sctk_alloc.h>
#include "sctk_ptl_am_types.h"
#include "mpc_common_debug.h"

#if !defined(NDEBUG)
#define sctk_ptl_chk(x) do { int __ret = 0; \
    static int ___env = -1; \
	if(___env == -1) { \
		___env = (getenv("MPC_PTL_DEBUG") != NULL);\
	}\
	if(___env) \
    		mpc_common_debug("%s (%s:%u)", #x, __FILE__, (unsigned int)__LINE__); \
    switch (__ret = x) { \
	case PTL_EQ_DROPPED: \
		mpc_common_debug_warning("At least one event has been dropped. This is a major concern !"); \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
    case PTL_OK: break; \
	default: mpc_common_debug_error("%s -> %s (%s:%u)", #x, sctk_ptl_am_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); MPC_CRASH(); break; \
    } } while (0)
#else
#define sctk_ptl_chk(x) x
#endif

/**************************************************************/
/************************** FUNCTIONS *************************/
/**************************************************************/

/* Hardware-related init */
sctk_ptl_id_t sctk_ptl_am_map_id(sctk_ptl_am_rail_info_t* srail, int dest);
sctk_ptl_am_rail_info_t sctk_ptl_am_hardware_init();
void sctk_ptl_am_hardware_fini();

/* Software-related init */
void sctk_ptl_am_software_init(sctk_ptl_am_rail_info_t*, int);
void sctk_ptl_am_software_fini(sctk_ptl_am_rail_info_t*);

/* Portals table management */
void sctk_ptl_am_pte_create(sctk_ptl_am_rail_info_t* srail, size_t key);

/* ME management */
sctk_ptl_am_local_data_t* sctk_ptl_am_me_create(void*, size_t, sctk_ptl_id_t, sctk_ptl_am_matchbits_t, sctk_ptl_am_matchbits_t, int);
sctk_ptl_am_local_data_t* sctk_ptl_am_me_create_with_cnt(sctk_ptl_am_rail_info_t* srail, void*, size_t, sctk_ptl_id_t, sctk_ptl_am_matchbits_t, sctk_ptl_am_matchbits_t, int);
void sctk_ptl_am_me_register(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_local_data_t* user_ptr, sctk_ptl_am_pte_t* pte);
void sctk_ptl_am_me_release(sctk_ptl_am_local_data_t*);
void sctk_ptl_am_me_free(sctk_ptl_am_local_data_t*, int);
void sctk_ptl_am_me_feed(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_pte_t* pte, size_t me_size, int nb, int list, char type, char protocol);

/* event management */
void sctk_ptl_ct_free(sctk_ptl_cnth_t cth);

/* Request management */
int sctk_ptl_am_emit_get(sctk_ptl_am_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_am_pte_t*, sctk_ptl_am_matchbits_t, size_t, size_t, void*);
int sctk_ptl_am_emit_put(sctk_ptl_am_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_am_pte_t*, sctk_ptl_am_matchbits_t, size_t, size_t, sctk_ptl_am_imm_data_t, void*);

int sctk_ptl_am_incoming_lookup(sctk_ptl_am_rail_info_t* srail);
int sctk_ptl_am_outgoing_lookup(sctk_ptl_am_rail_info_t* srail);

sctk_ptl_am_msg_t* sctk_ptl_am_send_request(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, const void* start_in, size_t sz_in, void** start_out, size_t* sz_out, sctk_ptl_am_msg_t*);
void sctk_ptl_am_send_response(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, void* start, size_t sz, int remote, sctk_ptl_am_msg_t*);
void sctk_ptl_am_register_process( sctk_ptl_am_rail_info_t *srail );
void sctk_ptl_am_wait_response(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_msg_t* msg);
int sctk_ptl_am_free_response(void* addr);
/**************************************************************/
/*************************** HELPERS **************************/
/**************************************************************/
sctk_ptl_id_t sctk_ptl_am_self(sctk_ptl_am_rail_info_t* srail);

/**
 * @brief function to check if the given Portals id struct can match any process
 *
 * @param r the struct to check
 * @return 1 if true, 0 otherwise
 */
static inline int __sctk_ptl_am_id_undefined(sctk_ptl_id_t r)
{
	// mpc_common_debug_warning("check %d/%d", r.phys.nid, r.phys.pid);
	return (r.phys.nid == PTL_NID_ANY && r.phys.pid == PTL_PID_ANY);
}

/**
 * @brief Decode the return code provided by Portals code, to display them as string
 *
 * @param rc the rc to decode
 * @return const char * the actual string
 */
static inline const char * sctk_ptl_am_rc_decode(int rc)
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

/**
 * @brief decode an event to display its type as a string.
 *
 * @param ev the event to stringify
 * @return const char  * the actual string.
 */
static inline const char * sctk_ptl_am_event_decode(sctk_ptl_event_t ev)
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

/**
 * De-serialize an object an map it into its base struct.
 *
 * \param[in] inval the serialized data, as a string or NULL if the outvallen is not large enough
 * \param[out] outval the effective struct to fill
 * \param[in] outvallen size of the final struct
 * \return Size of the string if serialization succeeded, -1 otherwise
 */
static inline int sctk_ptl_am_data_deserialize ( const char *inval, void *outval, unsigned int outvallen )
{
    unsigned int i;
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
static inline int sctk_ptl_am_data_serialize ( const void *inval, int invallen, char *outval, int outvallen )
{
    static unsigned char encodings[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', \
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    int i;
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
static inline int sctk_ptl_am_eq_poll_md(sctk_ptl_am_rail_info_t* srail, sctk_ptl_event_t* ev)
{
	int ret;

	unsigned int id = -1;

	assert(ev);
	ret = PtlEQPoll(&srail->mds_eq, 1, SCTK_PTL_AM_EQ_TIMEOUT, ev, &id);
	sctk_ptl_chk(ret);
	assert(ret == PTL_EQ_EMPTY || id == 0);

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
static inline int sctk_ptl_am_eq_poll_me(sctk_ptl_am_rail_info_t* srail, sctk_ptl_event_t* ev, unsigned int* idx)
{
	int ret;

	assert(ev && srail);
	ret = PtlEQPoll(srail->meqs_table, srail->nb_entries, SCTK_PTL_AM_EQ_TIMEOUT, ev, idx);
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
static inline const char * __sctk_ptl_am_match_str(char*buf, size_t s, ptl_match_bits_t m)
{
	sctk_ptl_am_matchbits_t m2;
	m2.raw = m;
	snprintf(buf, s, "[S%u/R%u]{T%u} -> D%u-R%u-L%u", m2.data.srvcode, m2.data.rpcode, m2.data.tag, m2.data.inc_data & 0x1, m2.data.is_req & 0x1, m2.data.is_large & 0x1);

/* 	int i = 0;
	for (i = 63; i >= 0; i--)
	{
		snprintf(buf+(63-i), s-(63-i), "%d", (m2.raw >> i) & 0x1);
	} */
	return buf;
}

static inline const char * __sctk_ptl_am_ign_str(char*buf, size_t s, ptl_match_bits_t m)
{
	sctk_ptl_am_matchbits_t m2;
	m2.raw = m;
	snprintf(buf, s, "[S?%u/R?%u]{T?%u} -> D?%u-R?%u-L?%u",
		m2.data.srvcode == SCTK_PTL_AM_IGN_SRV,
		m2.data.rpcode == SCTK_PTL_AM_IGN_RPC,
		m2.data.tag == SCTK_PTL_AM_IGN_TAG,
		m2.data.inc_data & SCTK_PTL_AM_IGN_DATA,
		m2.data.is_req & SCTK_PTL_AM_IGN_TYPE,
		m2.data.is_large & SCTK_PTL_AM_IGN_LARGE);

/* 	int i = 0;
	for (i = 63; i >= 0; i--)
	{
		snprintf(buf+(63-i), s-(63-i), "%d", (m2.raw >> i) & 0x1);
	} */
	return buf;
}

#endif /* MPC_USE_PORTALS */
#endif
