#include "handle_ctx.h"

#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>

#include <mpc_common_spinlock.h>

/**************************
 * CONTEXT ID UNIFICATION *
 **************************/


int mpc_lowcomm_communicator_handle_ctx_unify(mpc_lowcomm_communicator_t comm, mpc_lowcomm_handle_ctx_t hctx)
{
    /* Now all set their ID to the the source communicator */
    hctx->handle_ctx_id = mpc_lowcomm_communicator_id(comm);


    return MPC_LOWCOMM_SUCCESS;;
}

mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_handle_ctx_id(mpc_lowcomm_handle_ctx_t hctx)
{
    if(hctx == MPC_LOWCOMM_HANDLE_CTX_NULL)
    {
        return MPC_LOWCOMM_COMM_NULL_ID;
    }

    return hctx->handle_ctx_id;
}

int mpc_lowcomm_handle_ctx_equal(mpc_lowcomm_handle_ctx_t a, mpc_lowcomm_handle_ctx_t b)
{
    if(a == b)
    {
        return 1;
    }

    mpc_lowcomm_communicator_id_t ida = mpc_lowcomm_communicator_handle_ctx_id(a);
    mpc_lowcomm_communicator_id_t idb = mpc_lowcomm_communicator_handle_ctx_id(b);

    if(ida != idb)
    {
        return 0;
    }

    return 1;
}


/*****************************************
 * HANDLE CONTEXT ALLOCATION AND FREEING *
 *****************************************/

mpc_lowcomm_handle_ctx_t mpc_lowcomm_handle_ctx_new()
{
    mpc_lowcomm_handle_ctx_t ret = sctk_malloc(sizeof(struct mpc_lowcomm_handle_ctx_s));
    assume(ret != NULL);
    memset(ret, 0, sizeof(struct mpc_lowcomm_handle_ctx_s));

    ret->handle_ctx_id = MPC_LOWCOMM_COMM_NULL_ID;

    return ret;
}

int mpc_lowcomm_handle_ctx_free(mpc_lowcomm_handle_ctx_t * hctx)
{
    if(!hctx)
    {
        return 1;
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
