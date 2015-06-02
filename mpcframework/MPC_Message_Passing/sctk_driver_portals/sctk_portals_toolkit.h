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
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_TOOLKIT_H_
#define __SCTK_PORTALS_TOOLKIT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_route.h>
#include <portals4.h>
#include <sctk_spinlock.h>

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
} sctk_portals_message_t;


typedef struct sctk_Event_s
{
	unsigned used;
	//int vp;
	ptl_pt_index_t pt_index;
	sctk_portals_message_t msg;
	sctk_message_to_copy_t ptrmsg;
} sctk_Event_t;

typedef struct sctk_EventL_s
{
	unsigned 			    nb_elems;
	unsigned 			    nb_elems_headers;
	sctk_Event_t 			events[SIZE_QUEUE_EVENTS];
	struct sctk_EventL_s 	*next;
} sctk_EventL_t;


typedef struct sctk_EventQ_s
{
	unsigned SizeMsgList;
	sctk_EventL_t ListMsg;

} sctk_EventQ_t;

typedef struct sctk_ProcsL_s
{
	unsigned             nb_elems;
	ptl_process_t        Procs[SIZE_QUEUE_PROCS];
	struct sctk_ProcsL_s *next;
} sctk_ProcsL_t;


typedef struct sctk_ProcsQ_s
{
	unsigned        SizeList;
	sctk_ProcsL_t   List;

} sctk_ProcsQ_t;

void sctk_network_init_portals_all ( sctk_rail_info_t *rail );
void portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process );
#ifdef __cplusplus
}
#endif
#endif
#endif
