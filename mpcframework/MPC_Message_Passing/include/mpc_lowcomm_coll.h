#ifndef MPC_MESSAGE_PASSING_INCLUDE_COLL_H_
#define MPC_MESSAGE_PASSING_INCLUDE_COLL_H_

#include <mpc_lowcomm_types.h>

/* Barrier */

void mpc_lowcomm_barrier ( const mpc_lowcomm_communicator_t communicator );

/* Broadcast */

void mpc_lowcomm_bcast ( void *buffer, const size_t size,
                      const int root, const mpc_lowcomm_communicator_t com_id );

/* Allreduce */

void mpc_lowcomm_allreduce ( const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_number,
                       sctk_Op_f func,
                       const mpc_lowcomm_communicator_t communicator,
                       const mpc_lowcomm_datatype_t data_type );

extern void ( *mpc_lowcomm_coll_init_hook ) ( mpc_lowcomm_communicator_t id );

void mpc_lowcomm_terminaison_barrier (void);


#endif /* MPC_MESSAGE_PASSING_INCLUDE_COLL_H_ */