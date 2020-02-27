#include <mpc_lowcomm.h>
#include <mpc_keywords.h>

#pragma weak arpc_c_to_cxx_converter
int arpc_c_to_cxx_converter( __UNUSED__ sctk_arpc_context_t *ctx,
                             __UNUSED__ const void *request,
			     __UNUSED__ size_t req_size,
			     __UNUSED__ void **response,
			     __UNUSED__ size_t *resp_size )
{
	return 0;
}