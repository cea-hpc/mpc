/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LOWCOMM_HANDLE_CTX
#define MPC_LOWCOMM_HANDLE_CTX

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************
 * HANDLE CONTEXT TYPES *
 ************************/

/**
 * @brief This file defines the hereditary context to be propagated
 *        through handles in the Session world. It is in lowcomm
 *        as it has to live close to all other handles
 */

/**
 * @brief This defines the opaque context handle
 */
struct mpc_lowcomm_handle_ctx_s;
typedef struct mpc_lowcomm_handle_ctx_s * mpc_lowcomm_handle_ctx_t;

#define MPC_LOWCOMM_HANDLE_CTX_NULL NULL

/*****************************************
 * HANDLE CONTEXT ALLOCATION AND FREEING *
 *****************************************/

/**
 * @brief Create a new Opaque HANDLE CTX
 *
 * @return mpc_lowcomm_handle_ctx_t the new Handle CTX to be attached
 */
mpc_lowcomm_handle_ctx_t mpc_lowcomm_handle_ctx_new();

/**
 * @brief Free a previously allocated HANDLE Ctx
 *
 * @param hctx pointer to the handle ctx to be freed (will be set to MPC_LOWCOMM_HANDLE_CTX_NULL)
 * @return int 0 on success
 */
int mpc_lowcomm_handle_ctx_free(mpc_lowcomm_handle_ctx_t * hctx);

/****************************
 * HANDLE CONTEXT ACCESSORS *
 ****************************/

/**
 * @brief Check if two contexes are equal
 *
 * @param a first context
 * @param b second contex
 * @return int 1 if equal
 */
int mpc_lowcomm_handle_ctx_equal(mpc_lowcomm_handle_ctx_t a, mpc_lowcomm_handle_ctx_t b);


/**
 * @brief Attach a session to the handle context
 *
 * @param hctx the handle context to enrich
 * @param session the session to attach (note we use opaque pointer)
 * @return int 0 on success
 */
int mpc_lowcomm_handle_ctx_set_session(mpc_lowcomm_handle_ctx_t hctx, void * session);

/**
 * @brief Get the session attached to an handle context
 *
 * @param hctx the handle context to querry
 * @return void* the session handle context (opaque) NULL if none or error
 */
void * mpc_lowcomm_handle_ctx_get_session(mpc_lowcomm_handle_ctx_t hctx);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_HANDLE_CTX */
