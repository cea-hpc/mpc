#include "handle_ctx.h"

#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>

/*****************************************
 * HANDLE CONTEXT ALLOCATION AND FREEING *
 *****************************************/

mpc_lowcomm_handle_ctx_t mpc_lowcomm_handle_ctx_new()
{
    mpc_lowcomm_handle_ctx_t ret = sctk_malloc(sizeof(struct mpc_lowcomm_handle_ctx_s));
    assume(ret != NULL);
    memset(ret, 0, sizeof(struct mpc_lowcomm_handle_ctx_s));
    return ret;
}

int mpc_lowcomm_handle_ctx_free(mpc_lowcomm_handle_ctx_t * hctx)
{
    if(!hctx)
    {
        return NULL;
    }

    if(*hctx == MPC_LOWCOMM_HANDLE_CTX_NULL)
    {
        return 1;
    }

    sctk_free(*hctx);
    *hctx = MPC_LOWCOMM_HANDLE_CTX_NULL;

    return 0;
}

/****************************
 * HANDLE CONTEXT ACCESSORS *
 ****************************/

int mpc_lowcomm_handle_ctx_set_session(mpc_lowcomm_handle_ctx_t hctx, void * session)
{
    if(hctx == MPC_LOWCOMM_HANDLE_CTX_NULL)
    {
        return 1;
    }

    hctx->session_ptr = session;

    return 0;
}

void * mpc_lowcomm_handle_ctx_get_session(mpc_lowcomm_handle_ctx_t hctx)
{
    if(hctx == MPC_LOWCOMM_HANDLE_CTX_NULL)
    {
        return NULL;
    }

    return hctx->session_ptr;
}
