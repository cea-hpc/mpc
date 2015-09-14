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
/* #   - ADAM Julien julien.adam@cea.fr                                   # */
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

#define SCTK_PORTALS_BITS_INIT 0x0000000000000000
#define SCTK_PORTALS_SRC_IGN  0x0000FFFF00000000
#define SCTK_PORTALS_TAG_IGN  0x00000000FFFFFFFF
#define SCTK_PORTALS_WILD_FLAG 0x123456789ABCDEF0
#define SCTK_PORTALS_WILD_IGN 0x0000000000000000

#define SCTK_PORTALS_EVENTS_QUEUE_SIZE 64

#define SCTK_PORTALS_ME_OPTIONS PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW | PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE
#define SCTK_PORTALS_ME_PUT_OPTIONS SCTK_PORTALS_ME_OPTIONS | PTL_ME_OP_PUT
#define SCTK_PORTALS_ME_GET_OPTIONS SCTK_PORTALS_ME_OPTIONS | PTL_ME_OP_GET

#define SCTK_PORTALS_MD_OPTIONS 0
#define SCTK_PORTALS_MD_PUT_OPTIONS SCTK_PORTALS_MD_OPTIONS | PTL_MD_EVENT_CT_SEND
#define SCTK_PORTALS_MD_GET_OPTIONS SCTK_PORTALS_MD_OPTIONS | PTL_MD_EVENT_CT_REPLY

#define sctk_portals_process_id_t ptl_process_t
#define sctk_portals_interface_handler_t ptl_handle_ni_t
#define sctk_portals_limits_t ptl_ni_limits_t
#define sctk_portals_assume(x) x
#define CHECK_RETURNVAL(x) do { int ret; \
    switch (ret = x) { \
	case PTL_IGNORED: \
        case PTL_OK: break; \
        case PTL_FAIL: fprintf(stderr, "=> %s returned PTL_FAIL (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
        case PTL_NO_SPACE: fprintf(stderr, "=> %s returned PTL_NO_SPACE (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
        case PTL_ARG_INVALID: fprintf(stderr, "=> %s returned PTL_ARG_INVALID (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
        case PTL_NO_INIT: fprintf(stderr, "=> %s returned PTL_NO_INIT (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
		case PTL_PT_IN_USE: fprintf(stderr, "=> %s returned PTL_PT_IN_USE (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
        case PTL_IN_USE: fprintf(stderr, "=> %s returned PTL_IN_USE (line %u)\n", #x, (unsigned int)__LINE__); abort(); break; \
        default: fprintf(stderr, "=> %s returned failcode %i (line %u)\n", #x, ret, (unsigned int)__LINE__); abort(); break; \
    } } while (0)

ptl_ct_event_t counter_reset;

typedef struct sctk_portals_table_entry_s
{
	ptl_pt_index_t                     index;
	ptl_handle_eq_t                    *event_list;
	sctk_spinlock_t                    *lock;
	struct sctk_portals_table_entry_s  *next;
} sctk_portals_table_entry_t;

typedef struct sctk_portals_table_s
{
	size_t nb_entries;
	struct sctk_portals_table_entry_s* head;
	
} sctk_portals_table_t;

void sctk_portals_helper_lib_init(sctk_portals_interface_handler_t *interface, sctk_portals_process_id_t* id, sctk_portals_limits_t *maxs, sctk_portals_table_t *ptable);

void sctk_portals_helper_init_list_entry(ptl_me_t* me, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, unsigned int option);
void sctk_portals_helper_init_table_entry(sctk_portals_table_entry_t* entry, sctk_portals_interface_handler_t *interface, int ind);

#endif
#ifdef __cplusplus
}
#endif
#endif
