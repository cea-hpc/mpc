#ifndef SCTK_LIB_MODE
#include <stdlib.h>
#include "sctk_shm_mapper.h"

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

char *sctk_mpi_handler_gen_filename(void * option);

/*! \brief Send filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

bool sctk_mpi_handler_send_filename(char *filename, void* option);

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */
char* sctk_mpi_handler_recv_filename(void* option);


/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */
struct sctk_alloc_mapper_handler_s* sctk_shm_mpi_handler_init(void);

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */
void sctk_shm_mpi_handler_free(struct sctk_alloc_mapper_handler_s* mpi_handler);
#endif
