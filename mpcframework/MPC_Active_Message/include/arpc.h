#ifndef __ARPC_H_
#define __ARPC_H_

struct sctk_arpc_context
{
	int dest;
};
typedef struct sctk_arpc_context sctk_arpc_context_t;

void* arpc_emit_call(sctk_arpc_context_t* ctx, const void* input);
void* arpc_recv_call(sctk_arpc_context_t* ctx, const void* input);

#endif /* ifndef __ARPC_H_ */
