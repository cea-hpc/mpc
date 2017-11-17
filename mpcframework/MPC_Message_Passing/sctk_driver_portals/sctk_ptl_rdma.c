#ifdef MPC_USE_PORTALS

#include "sctk_types.h"
#include "sctk_route.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_iface.h"
#include "sctk_atomics.h"

int sctk_ptl_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
	not_implemented();
	return 0;
}

void sctk_ptl_rdma_fetch_and_op(  sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void * fetch_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * add,
		RDMA_op op,
		RDMA_type type )
{
	not_implemented();
}

int sctk_ptl_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
	not_implemented();
	return 0;
}

void sctk_ptl_rdma_cas(   sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void *  res_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * comp,
		void * new,
		RDMA_type type )
{
	not_implemented();
}

void sctk_ptl_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size )
{
	sctk_ptl_local_data_t* md = local_key->pin.ptl.md_data;
	sctk_ptl_local_data_t* me = local_key->pin.ptl.me_data;

	sctk_assert(src_addr >= md->slot.md.start);
	sctk_assert(src_addr + size < md->slot.md.start + md->slot.md.length );
	
	sctk_ptl_id_t remote = SCTK_PTL_ANY_PROCESS;
	sctk_ptl_emit_put(md, size, remote , rail->network.ptl.pt_entries + SCTK_PTL_PTE_RDMA, (sctk_ptl_matchbits_t)me->slot.me.match_bits );
}

void sctk_ptl_rdma_read(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size )
{
	sctk_ptl_local_data_t* md = local_key->pin.ptl.md_data;
	sctk_ptl_local_data_t* me = local_key->pin.ptl.me_data;

	sctk_assert(src_addr >= md->slot.md.start);
	sctk_assert(src_addr + size < md->slot.md.start + md->slot.md.length );
	
	sctk_ptl_id_t remote = SCTK_PTL_ANY_PROCESS;
	sctk_ptl_emit_get(md, size, remote , rail->network.ptl.pt_entries + SCTK_PTL_PTE_RDMA, (sctk_ptl_matchbits_t)me->slot.me.match_bits );
}

/* Pinning */
void sctk_ptl_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	sctk_ptl_rail_info_t* srail    = &rail->network.ptl;
	sctk_ptl_local_data_t *md_request , *me_request;
	void  *md_start                   , *me_start;
	size_t md_size                    , me_size;
	int md_flags                      , me_flags;
	sctk_ptl_matchbits_t md_match     , me_match, me_ign;
	sctk_ptl_pte_t *md_pte            , *me_pte;
	sctk_ptl_id_t md_remote          , me_remote;

	md_request = me_request = NULL;
	md_match   = me_match   = me_ign = SCTK_PTL_MATCH_INIT;
	md_pte     = me_pte     = NULL;
	md_remote  = me_remote  = SCTK_PTL_ANY_PROCESS;
	md_start   = me_start   = NULL;
	md_size    = me_size    = 0;
	md_flags   = me_flags   = 0;

	/* Configure the MD */
	md_flags = SCTK_PTL_MD_PUT_FLAGS | SCTK_PTL_MD_GET_FLAGS;
	md_pte = srail->pt_entries + SCTK_PTL_PTE_RDMA;
	md_request = sctk_ptl_md_create(srail, md_start, md_size, md_flags);
	sctk_assert(sizeof(md_request->msg) >= sizeof(srail->id)); /* pre-condition */
	sctk_ptl_md_register(srail, md_request);

	/* configure the ME */
	me_start = addr;
	me_size = size;
	me_flags = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ME_GET_FLAGS;
	me_match.data.tag = sctk_atomics_fetch_and_incr_int(&rail->network.ptl.rdma_cpt);
	me_ign.data.rank = SCTK_PTL_IGN_RANK;
	me_pte = srail->pt_entries + SCTK_PTL_PTE_RDMA;
	me_remote = SCTK_PTL_ANY_PROCESS;
	me_request = sctk_ptl_me_create(me_start, me_size, me_remote, me_match, me_ign, me_flags);
	sctk_ptl_me_register(srail, me_request, me_pte);

	list->rail_id = rail->rail_number;
	list->pin.ptl.me_data = me_request;
	list->pin.ptl.md_data = md_request;

	sctk_error("REGISTER RDMA region (nid/pid=%llu/%llu, idx=%d, match=%llu)", me_remote.phys.nid, me_remote.phys.pid, me_pte->idx, me_match.raw);
}
void sctk_ptl_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	not_implemented();
}
#endif
