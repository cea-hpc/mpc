#include "mpc_ofi_helpers.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

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


