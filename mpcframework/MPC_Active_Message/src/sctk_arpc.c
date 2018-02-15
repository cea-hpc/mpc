#include <arpc.h>
#include <sctk_debug.h>

void* arpc_emit_call(int dest, int code, const void* input)
{
	sctk_warning("Emit an RPC call w/ code %d", code);
}

void* arpc_recv_call(int dest, int code, const void* input)
{
	sctk_warning("Ready to call remote RPC w/ code %d", code);
}
