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
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_HELPER_H_
#define __SCTK_PORTALS_HELPER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <portals4.h>
#include <sctk_spinlock.h>
#include <utlist.h>
#include <stdlib.h>
#include <string.h>

#define SCTK_PORTALS_BITS_INIT 0UL
#define SCTK_PORTALS_BITS_FIRST_VALUE 1UL
#define SCTK_PORTALS_BITS_HEADER 0UL
//#define SCTK_PORTALS_BITS_CONTROL_MESSAGE (~(0UL)) 
#define SCTK_PORTALS_BITS_EAGER_SLOT (~(0UL) - 1UL)
#define SCTK_PORTALS_BITS_OPEN_CONNECTION (~(0UL) - 2UL)

//is entry specific or not (=header or not)
#define SCTK_PORTALS_NOT_SPECIFIC 0
#define SCTK_PORTALS_SPECIFIC 1

//activate complete polling for idle task
#define SCTK_PORTALS_POLL_ALL -1

#define SCTK_PORTALS_EVENTS_QUEUE_SIZE 64
#define SCTK_PORTALS_HEADERS_ME_SIZE (SCTK_PORTALS_EVENTS_QUEUE_SIZE + 4)

#define SCTK_PORTALS_ME_OPTIONS PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW | PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE
#define SCTK_PORTALS_ME_PUT_OPTIONS SCTK_PORTALS_ME_OPTIONS | PTL_ME_OP_PUT
#define SCTK_PORTALS_ME_GET_OPTIONS SCTK_PORTALS_ME_OPTIONS | PTL_ME_OP_GET

#define SCTK_PORTALS_MD_OPTIONS 0
#define SCTK_PORTALS_MD_PUT_OPTIONS SCTK_PORTALS_MD_OPTIONS | PTL_MD_EVENT_CT_SEND
#define SCTK_PORTALS_MD_GET_OPTIONS SCTK_PORTALS_MD_OPTIONS | PTL_MD_EVENT_CT_REPLY

#define sctk_portals_process_id_t ptl_process_t
#define sctk_portals_interface_handler_t ptl_handle_ni_t
#define sctk_portals_limits_t ptl_ni_limits_t
//#define sctk_portals_assume(x) x
#define sctk_portals_assume(x) do { int ret; \
    switch (ret = x) { \
	case PTL_IGNORED: \
        case PTL_OK: break; \
        case PTL_FAIL: sctk_fatal("=> %s returned PTL_FAIL (line %u)\n", #x, (unsigned int)__LINE__); break; \
        case PTL_NO_SPACE: sctk_fatal( "=> %s returned PTL_NO_SPACE (line %u)\n", #x, (unsigned int)__LINE__); break; \
        case PTL_ARG_INVALID: sctk_fatal( "=> %s returned PTL_ARG_INVALID (line %u)\n", #x, (unsigned int)__LINE__); break; \
        case PTL_NO_INIT: sctk_fatal( "=> %s returned PTL_NO_INIT (line %u)\n", #x, (unsigned int)__LINE__); break; \
		case PTL_PT_IN_USE: sctk_fatal( "=> %s returned PTL_PT_IN_USE (line %u)\n", #x, (unsigned int)__LINE__); break; \
        case PTL_IN_USE: sctk_fatal( "=> %s returned PTL_IN_USE (line %u)\n", #x, (unsigned int)__LINE__); break; \
        default: sctk_fatal( "=> %s returned failcode %i (line %u)\n", #x, ret, (unsigned int)__LINE__); break; \
    } } while (0)

typedef struct sctk_portals_table_entry_s
{
	ptl_pt_index_t                     index;
	ptl_handle_eq_t                    *event_list;
	sctk_spinlock_t                    lock;
	int                                entry_cpt;
} sctk_portals_table_entry_t;

typedef struct sctk_portals_table_s
{
	size_t nb_entries;
	sctk_spinlock_t					table_lock;
	struct sctk_portals_table_entry_s** head;
	
} sctk_portals_table_t;

void sctk_portals_helper_lib_init(sctk_portals_interface_handler_t *interface, sctk_portals_process_id_t* id, sctk_portals_table_t *ptable);

void sctk_portals_helper_init_new_entry(ptl_me_t* me, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, ptl_match_bits_t match, unsigned int option);
void sctk_portals_helper_init_table_entry(sctk_portals_table_entry_t* entry, sctk_portals_interface_handler_t *interface, int ind);
void sctk_portals_helper_set_bits_from_msg(ptl_match_bits_t* match, void*atomic);
void sctk_portals_helper_init_memory_descriptor(ptl_md_t* md, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, unsigned int option);
int sctk_portals_helper_from_str ( const char *inval, void *outval, int outvallen );
int sctk_portals_helper_to_str ( const void *inval, int invallen, char *outval, int outvallen );
void sctk_portals_helper_bind_to(sctk_portals_interface_handler_t*interface, ptl_process_t remote);
void sctk_portals_helper_get_request(void* start, size_t size, ptl_handle_ni_t* handler, ptl_process_t remote, ptl_pt_index_t index, ptl_match_bits_t match, void* ptr);
void sctk_portals_helper_put_request(void* start, size_t size, ptl_handle_ni_t* handler, ptl_process_t remote, ptl_pt_index_t index, ptl_match_bits_t match, void* ptr, ptl_hdr_data_t extra);
inline void sctk_portals_helper_register_new_entry(ptl_handle_ni_t* handler, ptl_pt_index_t index, ptl_me_t* slot, void* ptr);

#endif
#ifdef __cplusplus
}
#endif
#endif
