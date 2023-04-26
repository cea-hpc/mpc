#ifndef MPC_LOWCOMM_AM
#define MPC_LOWCOMM_AM

#include <mpc_lowcomm.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************
 * DATATYPE DEFINITIONS *
 ************************/

/**
 * @brief These are the types supported in RPCs
 * 
 */
typedef enum
{
	MPC_LOWCOMM_AM_NULL = 0,
	MPC_LOWCOMM_AM_INT,
	MPC_LOWCOMM_AM_FLOAT,
	MPC_LOWCOMM_AM_DOUBLE
}mpc_lowcomm_am_type_t;

/********************
 * INIT AND RELEASE *
 ********************/

/**
 * @brief Forward declaration for the internal opaque AM context
 *
 */
struct mpc_lowcomm_am_ctx_s;

/**
 * @brief Opaque Context for active messages
 *
 */
typedef struct mpc_lowcomm_am_ctx_s * mpc_lowcomm_am_ctx_t;

/**
 * @brief Initalize RPC engine
 * @warning Requires mpc_lowcomm_init to be called
 *
 * @return mpc_lowcomm_am_ctx_t Opaque context handle
 *         needs to be freed with mpc_lowcomm_am_finalize
 */
mpc_lowcomm_am_ctx_t mpc_lowcomm_am_init();

/**
 * @brief Release the RPC engine while ensuring completion
 * @warning Needs to be called *before* mpc_lowcomm_release
 *
 * @param ctx RPC context handle to be freed (initialized with mpc_lowcomm_am_init)
 * @return int 0 on success
 */
int mpc_lowcomm_am_release(mpc_lowcomm_am_ctx_t * ctx);

/****************************
 * ACTIVE MESSAGE INTERFACE *
 ****************************/

/**
 * @brief Remotely call a function without blocking (i)
 *
 * @warning The function needs to be resolvable with dlsym
 *          (you may need to compile with -rdynamic)
 *
 * @param ctx Opaque AM context (from mpc_lowcomm_am_init)
 * @param buffers Array of pointer to individual parameters
 * @param datatypes Types for respective parameters
 * @param sendcount Number of arguments to send
 * @param recvbuf Receive buffer (one element)
 * @param recvtype Receive type for the recvbuf
 * @param fname Remote function name to be called
 * @param dest Destination rank
 * @param req Request to be waited with mpc_lowcomm_wait*
 * @return int 0 on success
 */
int mpc_lowcomm_iam(mpc_lowcomm_am_ctx_t ctx,
                    void **buffers,
                    mpc_lowcomm_am_type_t *datatypes,
                    int sendcount,
                    void *recvbuf,
                    mpc_lowcomm_am_type_t recvtype,
                    char *fname,
                    int dest,
                    mpc_lowcomm_request_t *req);

/**
 * @brief Remotely call a function (blocking version)
 *
 * @warning The function needs to be resolvable with dlsym
 *          (you may need to compile with -rdynamic)
 *
 * @param ctx Opaque AM context (from mpc_lowcomm_am_init)
 * @param buffers Array of pointer to individual parameters
 * @param datatypes Types for respective parameters
 * @param sendcount Number of arguments to send
 * @param recvbuf Receive buffer (one element)
 * @param recvtype Receive type for the recvbuf
 * @param fname Remote function name to be called
 * @param dest Destination rank
 * @return int 0 on success
 */
int mpc_lowcomm_am(mpc_lowcomm_am_ctx_t ctx,
                   void **buffers,
                   mpc_lowcomm_am_type_t *datatypes,
                   int sendcount,
                   void *recvbuf,
                   mpc_lowcomm_am_type_t recvtype,
                   char *fname,
                   int dest);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_AM */
