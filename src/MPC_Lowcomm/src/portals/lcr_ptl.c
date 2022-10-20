#include "lcr_ptl.h"

#include "sctk_ptl_iface.h"

#include <sctk_alloc.h>

ssize_t lcr_ptl_send_tag_bcopy(_mpc_lowcomm_endpoint_t *ep,
			   lcr_tag_t tag,
			   uint64_t imm,
			   lcr_pack_callback_t pack,
			   void *arg,
			   __UNUSED__ unsigned cflags,
			   lcr_tag_context_t *ctx)
{
	int rc;
	sctk_ptl_local_data_t* request = NULL;
	sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
	void* start                    = NULL;
	size_t size                    = 0;
	int flags                      = 0;
	sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
	sctk_ptl_pte_t* pte            = NULL;
	sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
	_mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
	sctk_ptl_imm_data_t hdr;
	
        //TODO(HIGH PRIORITY): free start...
	start = sctk_malloc(srail->eager_limit);
	size = pack(start, arg);
	
	assert(size <= srail->eager_limit);

	/* prepare the Put() MD */
	flags                  = SCTK_PTL_MD_PUT_FLAGS;
	match.raw              = tag.t;
	hdr.raw                = imm;
	pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
	remote                 = infos->dest;
	request                = sctk_ptl_md_create(srail, start, size, flags);

	assert(request);
	assert(pte);

	/* double-linking */
	request->msg           = ctx;
	request->type          = SCTK_PTL_TYPE_STD;

	/* emit the request */
	rc = lcr_ptl_md_register(srail, request);
	if (rc == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug("LCR PTL: bcopy no resource to %d (iface=%llu, "
                                 "remote=%llu, idx=%d, sz=%llu)", ep->dest, srail->iface, 
                                 remote, pte->idx, size);
		return MPC_LOWCOMM_NO_RESOURCE;
	}
	mpc_common_debug_info("LCR PTL: send bcopy to %d (iface=%llu, remote=%llu, idx=%d, sz=%llu)", 
			 ep->dest, srail->iface, remote, pte->idx, size);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);

	return size;
}

int lcr_ptl_send_tag_zcopy(_mpc_lowcomm_endpoint_t *ep,
			   lcr_tag_t tag,
			   uint64_t imm,
			   const struct iovec *iov,
			   size_t iovcnt,
			   __UNUSED__ unsigned cflags,
			   lcr_tag_context_t *ctx)
{
	int rc;
	sctk_ptl_local_data_t* request = NULL;
	sctk_ptl_rail_info_t* srail    = &ep->rail->network.ptl;
	void* start                    = iov->iov_base;
	size_t size                    = iov->iov_len;
	int flags                      = 0;
	sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
	sctk_ptl_pte_t* pte            = NULL;
	sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
	_mpc_lowcomm_endpoint_info_portals_t* infos   = &ep->data.ptl;
	sctk_ptl_imm_data_t hdr;
	
        //FIXME: support for only one iovec
        assert(iovcnt == 1);

	/* prepare the Put() MD */
	flags                  = SCTK_PTL_MD_PUT_FLAGS;
	match.raw              = tag.t;
	hdr.raw                = imm;
	pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
	remote                 = infos->dest;
	request                = sctk_ptl_md_create(srail, start, size, flags);

	assert(request);
	assert(pte);

	/* double-linking */
	request->msg           = ctx;
	request->type          = SCTK_PTL_TYPE_STD;

	/* emit the request */
	rc = lcr_ptl_md_register(srail, request);
	if (rc == MPC_LOWCOMM_NO_RESOURCE) {
		mpc_common_debug("LCR PTL: zcopy in progress to %d (iface=%llu, remote=%llu, "
                                 "idx=%d, sz=%llu)", ep->dest, srail->iface, remote,  pte->idx, 
                                 size);
		return MPC_LOWCOMM_NO_RESOURCE;
	}
	mpc_common_debug_info("LCR PTL: send zcopy to %d (iface=%llu, remote=%llu, idx=%d, sz=%llu)", 
			      ep->dest, srail->iface, remote, pte->idx, size);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);

	return MPC_LOWCOMM_SUCCESS;
}

int lcr_ptl_recv_tag_zcopy(sctk_rail_info_t *rail,
			   lcr_tag_t tag, lcr_tag_t ign_tag,
			   const struct iovec *iov, 
			   __UNUSED__ size_t iovcnt, /* only one iov supported */
			   lcr_tag_context_t *ctx) 
{
	void* start                     = NULL;
	unsigned flags                  = 0;
	size_t size                     = 0;
	sctk_ptl_matchbits_t match      = SCTK_PTL_MATCH_INIT;
	sctk_ptl_matchbits_t ign        = SCTK_PTL_MATCH_INIT;
	sctk_ptl_pte_t* pte             = NULL;
	sctk_ptl_local_data_t* user_ptr = NULL;
	sctk_ptl_id_t remote            = SCTK_PTL_ANY_PROCESS;
	sctk_ptl_rail_info_t* srail     = &rail->network.ptl;

	start = iov[0].iov_base;
	match.raw = tag.t;
	ign.raw = ign_tag.t;

	/* complete the ME data, this ME will be appended to the PRIORITY_LIST */
	size     = iov[0].iov_len;
	pte      = SCTK_PTL_PTE_ENTRY(srail->pt_table, ctx->comm_id);
	flags    = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
	user_ptr = sctk_ptl_me_create(start, size, remote, match, ign, flags); 

	assert(user_ptr);
	assert(pte);

	user_ptr->msg  = ctx;
	user_ptr->list = SCTK_PTL_PRIORITY_LIST;
	user_ptr->type = SCTK_PTL_TYPE_STD;
	sctk_ptl_me_register(srail, user_ptr, pte);

	mpc_common_debug_info("LCR PTL: post recv zcopy to %p (iface=%llu, idx=%d, "
			      "sz=%llu, buf=%p)", rail, srail->iface, pte->idx, size, start);
	return MPC_LOWCOMM_SUCCESS;
}
