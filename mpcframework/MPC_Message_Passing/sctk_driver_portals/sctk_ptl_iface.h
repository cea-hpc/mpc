#ifdef MPC_USE_PORTALS
#ifndef SCTK_PTL_IFACE_H_
#define SCTK_PTL_IFACE_H_

#include <portals4.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "sctk_ptl_types.h"
#include "sctk_debug.h"

#if !defined(NDEBUG)
#define sctk_ptl_chk(x) do { int __ret = 0; \
    static int ___env = -1; \
	if(___env) { \
		___env = (getenv("MPC_PTL_DEBUG") != NULL);\
	}\
	if(___env) \
    		sctk_debug("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); \
    switch (__ret = x) { \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
        case PTL_OK: break; \
	default: sctk_fatal("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); break; \
    } } while (0)
#else
#define sctk_ptl_chk(x) x
#endif

/**************************************************************/
/************************** FUNCTIONS *************************/
/**************************************************************/

/* Hardware-related init */
sctk_ptl_rail_info_t sctk_ptl_hardware_init();
void sctk_ptl_hardware_fini();

/* Software-related init */
void sctk_ptl_software_init(sctk_ptl_rail_info_t*, int);
void sctk_ptl_software_fini(sctk_ptl_rail_info_t*);

/* Portals table management */
void sctk_ptl_pte_create(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, size_t key);

/* ME management */
sctk_ptl_local_data_t* sctk_ptl_me_create(void*, size_t, sctk_ptl_id_t, sctk_ptl_matchbits_t, sctk_ptl_matchbits_t, int);
sctk_ptl_local_data_t* sctk_ptl_me_create_with_cnt(sctk_ptl_rail_info_t* srail, void*, size_t, sctk_ptl_id_t, sctk_ptl_matchbits_t, sctk_ptl_matchbits_t, int);
void sctk_ptl_me_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t*, sctk_ptl_pte_t*);
void sctk_ptl_me_release(sctk_ptl_local_data_t*);
void sctk_ptl_me_free(sctk_ptl_local_data_t*, int);
void sctk_ptl_me_feed(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, size_t me_size, int nb, int list, char type, char protocol);

/* event management */
void sctk_ptl_ct_free(sctk_ptl_cnth_t cth);

/* MD management */
sctk_ptl_local_data_t* sctk_ptl_md_create(sctk_ptl_rail_info_t* srail, void*, size_t, int);
void sctk_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t*);
void sctk_ptl_md_release(sctk_ptl_local_data_t*);

/* Request management */
int sctk_ptl_emit_get(sctk_ptl_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_pte_t*, sctk_ptl_matchbits_t, size_t, size_t, void*);
int sctk_ptl_emit_put(sctk_ptl_local_data_t*, size_t, sctk_ptl_id_t, sctk_ptl_pte_t*, sctk_ptl_matchbits_t, size_t, size_t, size_t, void*);

int sctk_ptl_emit_atomic(
		sctk_ptl_local_data_t* user,
		size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match,
		sctk_ptl_rdma_op_t op, size_t local_off, size_t remote_off, sctk_ptl_rdma_type_t type);

int sctk_ptl_emit_fetch_atomic(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user,
		size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match,
		size_t local_getoff, size_t local_putoff, size_t remote_off,
		sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type);


int sctk_ptl_emit_swap(
		sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user,
		size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match,
		size_t local_getoff, size_t local_putoff, size_t remote_off,
		const void* cmp, sctk_ptl_rdma_type_t type);

int sctk_ptl_emit_triggeredGet(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold);

/**************************************************************/
/*************************** HELPERS **************************/
/**************************************************************/
sctk_ptl_id_t sctk_ptl_self(sctk_ptl_rail_info_t* srail);

static inline const char const * sctk_ptl_rc_decode(int rc)
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
			char* buf = sctk_malloc(sizeof(char) * 40);
			snprintf(buf, 40, "Portals return code not known: %d", rc); 
			return buf;
			break;
		}
	}
	return NULL;
}

static inline const char const * sctk_ptl_event_decode(sctk_ptl_event_t ev)
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
static inline int sctk_ptl_data_deserialize ( const char *inval, void *outval, int outvallen )
{
    int i;
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
static inline int sctk_ptl_data_serialize ( const void *inval, int invallen, char *outval, int outvallen )
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
static inline const char const* __sctk_ptl_match_str(char*buf, size_t s, ptl_match_bits_t m)
{
	sctk_ptl_matchbits_t m2 = (sctk_ptl_matchbits_t)m;
	snprintf(buf, s, "%u:%d[%u]", m2.data.rank, m2.data.tag, m2.data.uid);
	return buf;
}


#endif
#endif
