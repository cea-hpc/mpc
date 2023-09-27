#include "mpc_ofi_helpers.h"

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <sctk_alloc.h>


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/*****************
 * ALLOC ALIGNED *
 *****************/

mpc_ofi_aligned_mem_t mpc_ofi_alloc_aligned(size_t size)
{
	mpc_ofi_aligned_mem_t ret;

	ret.orig = sctk_malloc(size + MPC_COMMON_PAGE_SIZE);

	if(!ret.orig)
	{
		ret.ret = NULL;
	}
	else
	{
		ret.ret = (void*)((uint64_t)(ret.orig + (MPC_COMMON_PAGE_SIZE - 1)) & -MPC_COMMON_PAGE_SIZE);
	}

	return ret;
}
void mpc_ofi_free_aligned(mpc_ofi_aligned_mem_t * mem)
{
	sctk_free(mem->orig);
}


int mpc_ofi_decode_mr_mode(uint64_t flags)
{
	(void)fprintf(stderr, "MR MODE %ld =", flags);

	if(flags & FI_MR_LOCAL)
	{
		(void)fprintf(stderr, "FI_MR_LOCAL ");
	}

	if(flags & FI_MR_RAW)
	{
		(void)fprintf(stderr, "FI_MR_RAW ");
	}

	if(flags & FI_MR_VIRT_ADDR)
	{
		(void)fprintf(stderr, "FI_MR_VIRT_ADDR ");
	}

	if(flags & FI_MR_ALLOCATED)
	{
		(void)fprintf(stderr, "FI_MR_ALLOCATED ");
	}

	if(flags & FI_MR_PROV_KEY)
	{
		(void)fprintf(stderr, "FI_MR_PROV_KEY ");
	}

	if(flags & FI_MR_MMU_NOTIFY)
	{
		(void)fprintf(stderr, "FI_MR_MMU_NOTIFY ");
	}

	if(flags & FI_MR_RMA_EVENT)
	{
		(void)fprintf(stderr, "FI_MR_RMA_EVENT ");
	}

	if(flags & FI_MR_ENDPOINT)
	{
		(void)fprintf(stderr, "FI_MR_ENDPOINT ");
	}

	if(flags & FI_MR_HMEM)
	{
		(void)fprintf(stderr, "FI_MR_HMEM ");
	}

	if(flags & FI_MR_COLLECTIVE)
	{
		(void)fprintf(stderr, "FI_MR_COLLECTIVE ");
	}

	(void)fprintf(stderr, "\n");
	return 0;
}

const char * const mpc_ofi_decode_endpoint_type(enum fi_ep_type type)
{
	switch (type)
	{
		case FI_EP_UNSPEC:
			return "FI_EP_UNSPEC";
		case FI_EP_MSG:
			return "FI_EP_MSG";
		case FI_EP_DGRAM:
			return "FI_EP_DGRAM";
		case FI_EP_RDM:
			return "FI_EP_RDM";
		case FI_EP_SOCK_STREAM:
			return "FI_EP_SOCK_STREAM";
		case FI_EP_SOCK_DGRAM:
			return "FI_EP_SOCK_DGRAM";
	}

	return "UNKNOWN Endpoint type";
}

enum fi_ep_type mpc_ofi_encode_endpoint_type(const char * type)
{
	if(!strcmp(type, "FI_EP_UNSPEC"))
	{
		return FI_EP_UNSPEC;
	}

	if(!strcmp(type, "FI_EP_MSG"))
	{
		return FI_EP_MSG;
	}
	
	if(!strcmp(type, "FI_EP_DGRAM"))
	{
		return FI_EP_DGRAM;
	}

	if(!strcmp(type, "FI_EP_RDM"))
	{
		return FI_EP_RDM;
	}

	if(!strcmp(type, "FI_EP_SOCK_STREAM"))
	{
		return FI_EP_SOCK_STREAM;
	}

	if(!strcmp(type, "FI_EP_SOCK_DGRAM"))
	{
		return FI_EP_SOCK_DGRAM;
	}

	mpc_common_debug_fatal("Failed to decode OFI endpoint type from '%s'", type);
}


int mpc_ofi_decode_cq_flags(uint64_t flags)
{
	(void)fprintf(stderr, "CQ FLAG %ld =", flags);

	if(flags & FI_SEND)
	{
		(void)fprintf(stderr, "FI_SEND ");
	}

	if(flags & FI_RECV)
	{
		(void)fprintf(stderr, "FI_RECV ");
	}

	if(flags & FI_RMA)
	{
		(void)fprintf(stderr, "FI_RMA ");
	}

	if(flags & FI_ATOMIC)
	{
		(void)fprintf(stderr, "FI_ATOMIC ");
	}

	if(flags & FI_MSG)
	{
		(void)fprintf(stderr, "FI_MSG ");
	}

	if(flags & FI_TAGGED)
	{
		(void)fprintf(stderr, "FI_TAGGED ");
	}

	if(flags & FI_READ)
	{
		(void)fprintf(stderr, "FI_READ ");
	}

	if(flags & FI_WRITE)
	{
		(void)fprintf(stderr, "FI_WRITE ");
	}

	if(flags & FI_REMOTE_READ)
	{
		(void)fprintf(stderr, "FI_REMOTE_READ ");
	}

	if(flags & FI_REMOTE_WRITE)
	{
		(void)fprintf(stderr, "FI_REMOTE_WRITE ");
	}

	if(flags & FI_REMOTE_CQ_DATA)
	{
		(void)fprintf(stderr, "FI_REMOTE_CQ_DATA ");
	}

	if(flags & FI_MULTI_RECV)
	{
		(void)fprintf(stderr, "FI_MULTI_RECV ");
	}

	(void)fprintf(stderr, "\n");
	return 0;
}


struct fi_info * mpc_ofi_get_requested_hints(const char * provider, const char *endpoint_type)
{
	struct fi_info * hints = fi_allocinfo();

   if(!hints)
   {
      mpc_common_errorpoint("Failed to allocate hints");
      return NULL;
   }

	/* Config handles provider as empty string */
	if(provider)
	{
		if(!strlen(provider))
		{
			provider = NULL;
		}
	}

	hints->mode = FI_CONTEXT | FI_LOCAL_MR;
	hints->caps = FI_MSG | FI_RMA;
	hints->ep_attr->type          = mpc_ofi_encode_endpoint_type(endpoint_type);
	hints->fabric_attr->prov_name = provider?strdup(provider):NULL;
	hints->domain_attr->threading = FI_THREAD_DOMAIN;
	hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;
	hints->domain_attr->control_progress = FI_PROGRESS_MANUAL;

	return hints;
}
