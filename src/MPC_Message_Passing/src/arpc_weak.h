#ifndef ARPC_WEAK_H
#define ARPC_WEAK_H

#include <mpc_lowcomm_types.h>

int arpc_c_to_cxx_converter( sctk_arpc_context_t* ctx,  const void * request,  size_t req_size,  void** response,  size_t* resp_size);


#endif