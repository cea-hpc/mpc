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

#ifndef LCR_PTL_IFACE_H
#define LCR_PTL_IFACE_H

#include "lcr_def.h"
#include "ptl_types.h"
#include <endpoint.h> //FIXME: try to use remove this include
#include "mpc_lowcomm_monitor.h"

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


int lcr_ptl_iface_init(sctk_rail_info_t *rail, unsigned flags);
int lcr_ptl_iface_fini(sctk_rail_info_t* rail);
void lcr_ptl_add_route(mpc_lowcomm_peer_uid_t dest, lcr_ptl_addr_t id, 
                       sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, 
                       _mpc_lowcomm_endpoint_state_t state);

#endif
