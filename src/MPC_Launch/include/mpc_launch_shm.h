#ifndef MPC_LAUNCH_SHM_H_
#define MPC_LAUNCH_SHM_H_

#include <stddef.h>

/**
 * @brief These are the data exchanges methods which can be used to create the segment
 *
 */
typedef enum
{
	MPC_LAUNCH_SHM_USE_PMI, /**< Synchronize with PMI (Put/Get/Barrier) */
	MPC_LAUNCH_SHM_USE_MPI  /**< Synchronize with MPI (see @ref mpc_launch_shm_exchange_params_t) */
}mpc_launch_shm_exchange_method_t;

/**
 * @brief This defines extra params for synchronization
 *
 */
typedef union
{
	struct {} pmi; /**< No param needed for PMI (NULL can be passed) */

	struct
	{
		int (*rank)(void *pcomm);                          /**< A function returning the rank in the comm */
		int (*bcast)(void *buff, size_t len, void *pcomm); /**< A function broadcasting from 0 on the comm */
		int (*barrier)(void *pcomm);                       /**< A barrier on the comm */
		void *pcomm;                                       /**< A pointer to communicator (stores the actual comm) */
	}         mpi;                                             /**< These parameters are required to create SHM segments with MPI */
}mpc_launch_shm_exchange_params_t;

/**
 * @brief Create an SHM segment using various data exchange methods
 *
 * @param size requested SHM segment size
 * @param method How to proceed to data exchange
 * @param params Extra parameters for data-exchange
 * @return void* pointer to the newly allocated shared segment to be freed with @ref mpc_launch_shm_unmap
 *
 * @warning - With @ref MPC_LAUNCH_SHM_USE_PMI, the PMI will automatically gather participating processes per hostname
 *          - With @ref MPC_LAUNCH_SHM_USE_MPI, the user should provide a communicator spanning on a single node (analogous to MPI_COMM_TYPE_SHARED)
 */
void *mpc_launch_shm_map(size_t size, mpc_launch_shm_exchange_method_t method, mpc_launch_shm_exchange_params_t *params);

/**
 * @brief Free an shm segment previously allocated with @ref mpc_launch_shm_map
 *
 * @param ptr pointer returned by @ref mpc_launch_shm_map
 * @param size size to free as provided to @ref mpc_launch_shm_map
 * @return -1 on error
 */
int mpc_launch_shm_unmap(void *ptr, size_t size);

#endif /* MPC_LAUNCH_SHM_H_ */
