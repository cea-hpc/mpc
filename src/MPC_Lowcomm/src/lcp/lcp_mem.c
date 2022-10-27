#include "lcp_mem.h"

#include "lcp_context.h"
#include <sctk_alloc.h>

int lcp_mem_register(lcp_context_h ctx, lcp_mem_h *mem_p, void *buffer, size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        size_t max_req = SIZE_MAX;
        size_t max_msg_size = SIZE_MAX;
        int max_used_ifaces = 0;
	int num_msg;
        sctk_rail_info_t *iface = NULL;
        void *p = buffer;
        size_t reg_size;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory domain");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mem->length     = length;

        /* Get interface attributes to define maximum memory size 
         * that can be registered */
        //FIXME: resource must be registerable, make sure looping over
        //       ctx->num_resources is ok
        for (i=0; i<ctx->num_resources; i++) {
                lcr_rail_attr_t attr; 
                iface = ctx->resources[i].iface;
               
                iface->iface_get_attr(iface, &attr);

                max_req = LCP_MIN(max_req, attr.mem.cap.max_reg);
                max_msg_size = LCP_MIN(max_msg_size, attr.iface.cap.rndv.max_put_zcopy);
        }
        assert(max_req > length);

	/* compute number of interface to use for registration based on msg length */
	num_msg = length % max_msg_size > 0 ? length / max_msg_size + 1 : length / max_msg_size;
        max_used_ifaces = LCP_MIN(ctx->num_resources, num_msg);
        mem->num_ifaces = max_used_ifaces; 

        mem->mems = sctk_malloc(mem->num_ifaces * sizeof(struct lcp_memp));
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        
        /* Pin the memory and create memory handles */
        reg_size = length/max_used_ifaces + length % max_used_ifaces;
        for (i=0; i< max_used_ifaces; i++) {
                iface = ctx->resources[i].iface;

                mem->mems[i].memp = sctk_malloc(sizeof(lcr_memp_t));
                if (mem->mems[i].memp == NULL) {
                        mpc_common_debug_error("LCP: could not allocate memory pins");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                iface->rail_pin_region(iface, mem->mems[i].memp, p, reg_size);

                mem->mems[i].base_addr = (uint64_t)p;
                mem->mems[i].len       = reg_size;

                p = p + reg_size;
                reg_size = length/max_used_ifaces;
        }

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_deregister(lcp_context_h ctx, lcp_mem_h mem)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        for (i=0; i<mem->num_ifaces; i++) {
                iface = ctx->resources[i].iface;
                iface->rail_unpin_region(iface, mem->mems[i].memp);
                sctk_free(mem->mems[i].memp);
        }

        sctk_free(mem->mems);
        sctk_free(mem); 

        return rc;
}
