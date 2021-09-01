#ifndef __MPC_THREAD_MPI_OMP_INTEROP_H__
# define __MPC_THREAD_MPI_OMP_INTEROP_H__

# include <mpc_config.h>

# if MPC_OpenMP && MPC_MPI

/** If true, enable MPI/OMP interoperability */
#  define MPC_ENABLE_INTEROP_MPI_OMP 1
#  if MPC_ENABLE_INTEROP_MPI_OMP
#   include <mpi.h>

/**
 * To be called by the MPI runtime when it is about
 * to wait the `n` requests of the `req` array
 */
int mpc_thread_mpi_omp_wait(int n, MPI_Request * req, int * index, MPI_Status * statuses);

#  endif /* MPC_ENABLE_INTEROP_MPI_OMP */
# else /* MPC_OpenMP && MPC_MPI */
#  define MPC_ENABLE_INTEROP_MPI_OMP 0
# endif /* MPC_OpenMP && MPC_MPI */

#endif /* __MPC_THREAD_MPI_OMP_INTEROP_H__ */
