#include <arpc.h>
#include <sctk_debug.h>

void* arpc_emit_call(sctk_arpc_context_t* ctx, const void* input)
{
	sctk_warning("Emit an RPC call");

	/* cheat for now */
	return arpc_recv_call(ctx, input);
}

void* arpc_recv_call(sctk_arpc_context_t* ctx, const void* input)
{
	return NULL;
}
