/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_portals_rdma.h>

int sctk_portals_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
}


void sctk_portals_rdma_fetch_and_op(  sctk_rail_info_t *rail,
							sctk_thread_ptp_message_t *msg,
							void * fetch_addr,
							struct  sctk_rail_pin_ctx_list * local_key,
							void * remote_addr,
							struct  sctk_rail_pin_ctx_list * remote_key,
							void * add,
							RDMA_op op,
							RDMA_type type)
{
}

int sctk_portals_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
}

void sctk_portals_rdma_cas(sctk_rail_info_t *rail,
					  sctk_thread_ptp_message_t *msg,
					  void *  res_addr,
					  struct  sctk_rail_pin_ctx_list * local_key,
					  void * remote_addr,
					  struct  sctk_rail_pin_ctx_list * remote_key,
					  void * comp,
					  void * new,
					  RDMA_type type )
{}
void sctk_portals_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
					 void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
					 void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
					 size_t size )
{}

void sctk_portals_rdma_read(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
					 void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
					 void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
					 size_t size )
{}

void sctk_portals_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	ptl_me_t me;
	ptl_match_bits_t match;

	sctk_portals_rail_info_t* prail = &rail->network.portals;
	int entry = prail->ptable.nb_entries - 1;

	//sctk_portals_helper_set_bits_from_msg(&match, &prail->ptable.head[entry]->entry_cpt);
	//sctk_portals_helper_init_new_entry(&me, &prail->interface_handler, addr, size, match, SCTK_PORTALS_ME_ALL_OPTIONS);

	list->rail_id = rail->rail_number;
	list->pin.portals.me_handler = sctk_portals_helper_register_new_entry(&prail->interface_handler, entry, &me, NULL);;
	list->pin.portals.match = match;
}

void sctk_portals_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	assume(list->rail_id == rail->rail_number);
	PtlMEUnlink(list->pin.portals.me_handler);
}
