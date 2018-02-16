#ifndef __ARPC_H_
#define __ARPC_H_
#include <string.h>

struct sctk_arpc_context
{
	int dest;
	int rpcode;
	int srvcode;
};
typedef struct sctk_arpc_context sctk_arpc_context_t;

int arpc_emit_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void* response, size_t* resp_size);
int arpc_recv_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void* response, size_t* resp_size);

int arpc_polling_request(sctk_arpc_context_t* ctx);
#endif /* ifndef __ARPC_H_ */
