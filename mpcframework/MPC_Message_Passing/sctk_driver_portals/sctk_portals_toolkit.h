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

#ifndef __SCTK_PORTALS_TOOLKIT_H_
#define __SCTK_PORTALS_TOOLKIT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_portals_helper.h>

# define NI_TYPE  PTL_NI_MATCHING
# define OPTIONS  (PTL_ME_OP_GET | PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW | PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE | PTL_ME_EVENT_COMM_DISABLE)
# define OPTIONS2 (PTL_ME_OP_PUT | PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW | PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE)
# define OPTIONS_HEADER  (PTL_ME_OP_PUT | PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW | PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE)
# define APPEND   PtlMEAppend
# define UNLINK   PtlMEUnlink

#define SCTK_PORTALS_SIZE_EVENTS		64
//#define SIZE_QUEUE_PROCS		16
#define SCTK_PORTALS_SIZE_HEADERS		16
#define SCTK_PORTALS_SIZE_HEADERS_MIN	8

#define MSG_ARRAY	0
#define OVER_ARRAY	1

#define READ	2
#define	WRITE	1

#define EVENT_EVENT 		0
#define EVENT_ENTRY 		1
#define EVENT_DESCRIPTOR 	2

#define TAG_INIT    0x1000000000000000
#define TAG_IGN 	0x00000000FFFFFFFF
#define SOURCE_IGN 	0x0000FFFF00000000
#define FLAG_REQ	0x0001000000000000
#define REQ_IGN		0xFFFEFFFFFFFFFFFF
#define ANY_TAG 	-1
#define ANY_SOURCE 	-1

/*		|  ME  |  MEH |  MD  |  MDH |				*/
//#define MDH_ALLOCATED		1
#define MD_ALLOCATED		2
//#define MEH_ALLOCATED		4
#define ME_ALLOCATED		8

#define IDLE				0
#define	RESERVED			1
#define IN_USE				2

#define TRYLOCK_SUCCESS	    0

   
typedef struct portals_message_s
{
	ptl_md_t md;
	ptl_handle_md_t md_handle;
	ptl_me_t me;
	ptl_handle_me_t me_handle;

	uint8_t  allocs;
	ptl_process_t peer;
	ptl_match_bits_t match_bits;
	ptl_match_bits_t ignore_bits;
	void *buffer;
	unsigned peer_idThread;
	unsigned my_idThread;
	int type;
	int tag;
	int append_pos;
	int append_list;
    char init_message;
} sctk_portals_message_t;

typedef struct sctk_portals_connection_context_s
{
	sctk_portals_process_id_t from;
	int to;
    ptl_pt_index_t entry;
} sctk_portals_connection_context_t;

typedef enum
{
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC
} sctk_portals_control_message_t;

typedef struct sctk_portals_event_item_s
{
	unsigned used;
	sctk_portals_message_t msg;
	sctk_message_to_copy_t ptrmsg;
} sctk_portals_event_item_t;

void sctk_network_init_portals_all ( sctk_rail_info_t *rail );
void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process );
#endif
#ifdef __cplusplus
}
#endif // MPC_USE_PORTALS
#endif // end .h file
