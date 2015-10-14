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

// const used for emulate RDMA_INC and RDMA_DEC, not provided by Portals
static const int inc_buf = 1;
static const int dec_buf = -1;

static ptl_op_t sctk_portals_rdma_determine_operation(RDMA_op op)
{
	switch(op)
	{
		case RDMA_SUM: return PTL_SUM; break;
		case RDMA_MIN: return PTL_MIN; break;
		case RDMA_MAX: return PTL_MAX; break;
		case RDMA_PROD: return PTL_PROD; break;
		case RDMA_LAND: return PTL_LAND; break;
		case RDMA_BAND: return PTL_BAND; break;
		case RDMA_LOR: return PTL_LOR; break;
		case RDMA_BOR: return PTL_BOR; break;
		case RDMA_LXOR: return PTL_LXOR; break;
		case RDMA_BXOR: return PTL_BXOR; break;
		default:
			sctk_error("Type not handled by Portals %d", op);
	}

	not_reachable();
	return 0;
}

static ptl_datatype_t sctk_portals_rdma_determine_type(RDMA_type type)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR:               return PTL_INT8_T; break;
		case RDMA_TYPE_DOUBLE:             return PTL_DOUBLE; break;
		case RDMA_TYPE_FLOAT:              return PTL_FLOAT; break;
		case RDMA_TYPE_INT:                return PTL_INT32_T; break;
		case RDMA_TYPE_LONG:               return PTL_INT64_T; break;
		case RDMA_TYPE_LONG_DOUBLE:        return PTL_LONG_DOUBLE; break;
		case RDMA_TYPE_LONG_LONG:          return PTL_INT64_T; break;
		case RDMA_TYPE_LONG_LONG_INT:      return PTL_INT64_T; break;
		case RDMA_TYPE_SHORT:              return PTL_INT16_T; break;
		case RDMA_TYPE_SIGNED_CHAR:        return PTL_INT8_T; break;
		case RDMA_TYPE_UNSIGNED:           return PTL_UINT32_T; break;
		case RDMA_TYPE_UNSIGNED_CHAR:      return PTL_UINT8_T; break;
		case RDMA_TYPE_UNSIGNED_LONG:      return PTL_UINT32_T; break;
		case RDMA_TYPE_UNSIGNED_LONG_LONG: return PTL_UINT64_T; break;
		case RDMA_TYPE_UNSIGNED_SHORT:     return PTL_UINT16_T; break;
		case RDMA_TYPE_WCHAR:              return PTL_INT16_T; break;
	}

	not_reachable();
	return 0;
}

int sctk_portals_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
	int ret = 1;
	return ret;
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
	//need to pair w/ MPC types
	ptl_datatype_t datatype;
	ptl_op_t operation;
	void * adder = add;

	if(op == RDMA_INC)
	{
		operation = PTL_SUM;
		adder = &inc_buf;
	}
	else if(op == RDMA_DEC)
	{
		operation = PTL_SUM;
		adder = &dec_buf;
	}
	else
	{
		operation = sctk_portals_rdma_determine_operation(op);
	}

	datatype = sctk_portals_rdma_determine_type(type);


	struct sctk_rail_portals_pin_ctx portals = remote_key->pin.portals;
	sctk_portals_list_entry_extra_t *stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RDMA;
	stuff->extra_data = msg;

	sctk_portals_helper_fetchAtomic_request(
		&rail->network.portals.ptable.pending_list,
		adder,	0, fetch_addr,0,  RDMA_type_size(type), (remote_addr - portals.start_addr),
		&rail->network.portals.interface_handler,
		portals.id,
		rail->network.portals.ptable.nb_entries,
		portals.match,
		stuff,
		0,
		operation,
		datatype
	);
}

int sctk_portals_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
	int ret = 1;
	return ret;
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
{
	//need to pair
	ptl_datatype_t datatype;
	ptl_op_t operation;

	struct sctk_rail_portals_pin_ctx portals = remote_key->pin.portals;
	sctk_portals_list_entry_extra_t *stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RDMA;
	stuff->extra_data = msg;

	sctk_portals_helper_swap_request(
		&rail->network.portals.ptable.pending_list,
		new, 0, res_addr, 0, RDMA_type_size(type), (remote_addr - portals.start_addr),
		&rail->network.portals.interface_handler,
		portals.id,
		rail->network.portals.ptable.nb_entries,
		portals.match,
		stuff,
		0,
		comp,
		operation,
		datatype
	);
}

void sctk_portals_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
					 void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
					 void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
					 size_t size )
{
	struct sctk_rail_portals_pin_ctx portals = remote_key->pin.portals;
	sctk_portals_list_entry_extra_t *stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RDMA;
	stuff->extra_data = msg;

	sctk_nodebug("RDMA WRITE TO %lu - %lu !!", portals.id.phys.pid, portals.match);
	sctk_portals_helper_put_request(
		&rail->network.portals.ptable.pending_list,
		src_addr,
		size,
		(dest_addr - portals.start_addr),
		&rail->network.portals.interface_handler,
		portals.id,
		rail->network.portals.ptable.nb_entries,
		portals.match,
		stuff,
		0,
		SCTK_PORTALS_NO_BLOCKING_REQUEST,
		SCTK_PORTALS_ACK_MSG
	);
}

void sctk_portals_rdma_read(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
					 void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
					 void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
					 size_t size )
{
	struct sctk_rail_portals_pin_ctx portals = remote_key->pin.portals;

	sctk_portals_list_entry_extra_t *stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RDMA;
	stuff->extra_data = msg;

	sctk_warning("RDMA READ FROM %lu - %lu (%d) !!", portals.id.phys.pid, portals.match, (src_addr - portals.start_addr));
	sctk_portals_helper_get_request(
		&rail->network.portals.ptable.pending_list,
		dest_addr,
		size,
		(src_addr - portals.start_addr),
		&rail->network.portals.interface_handler,
		portals.id,
		rail->network.portals.ptable.nb_entries,
		portals.match,
		stuff,
		SCTK_PORTALS_NO_BLOCKING_REQUEST
	);

}

void sctk_portals_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	ptl_me_t me;
	ptl_match_bits_t match;

	sctk_portals_rail_info_t* prail = &rail->network.portals;
	int entry = prail->ptable.nb_entries;

	sctk_portals_list_entry_extra_t *stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RDMA;
	stuff->extra_data = NULL;

	sctk_portals_helper_set_bits_from_msg(&match, &prail->ptable.head[entry]->entry_cpt);
	sctk_portals_helper_init_new_entry(&me, &prail->interface_handler, addr, size, match, SCTK_PORTALS_ME_ALL_OPTIONS);

	list->rail_id = rail->rail_number;
	list->pin.portals.me_handler = sctk_portals_helper_register_new_entry(&prail->interface_handler, entry, &me, stuff);
	list->pin.portals.me = me;
	list->pin.portals.match = match;
	list->pin.portals.id = prail->current_id;
	list->pin.portals.start_addr = addr;
	sctk_nodebug("Pinning %lu - %lu !!", list->pin.portals.id.phys.pid, list->pin.portals.match);

}

void sctk_portals_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	assume(list->rail_id == rail->rail_number);
	sctk_portals_helper_destroy_entry(&list->pin.portals.me, &list->pin.portals.me_handler);
}
