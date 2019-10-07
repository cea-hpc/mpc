#ifndef MPC_MESSAGE_PASSING_INCLUDE_COLL_H_
#define MPC_MESSAGE_PASSING_INCLUDE_COLL_H_

#include <sctk_types.h>

/* Barrier */

void mpc_mp_barrier ( const mpc_mp_communicator_t communicator );

/* Broadcast */

void mpc_mp_bcast ( void *buffer, const size_t size,
                      const int root, const mpc_mp_communicator_t com_id );

/* Allreduce */

void mpc_mp_allreduce ( const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_number,
                       sctk_Op_f func,
                       const mpc_mp_communicator_t communicator,
                       const mpc_mp_datatype_t data_type );

extern void ( *mpc_mp_coll_init_hook ) ( mpc_mp_communicator_t id );

void mpc_mp_terminaison_barrier (void);


#endif /* MPC_MESSAGE_PASSING_INCLUDE_COLL_H_ */