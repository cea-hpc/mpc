#include <stdlib.h>
#include "mpc_common_types.h"
#include "sctk_shm_mapper.h"

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

char *sctk_pmi_handler_gen_filename(void * option);

/*! \brief Send filename via pmi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

short int sctk_pmi_handler_send_filename(const char *filename, void* option);

/*! \brief Recv filename via pmi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

char* sctk_pmi_handler_recv_filename(void* option);

struct sctk_alloc_mapper_handler_s* sctk_shm_pmi_handler_init(char* option, int master_rank);

void sctk_shm_pmi_handler_free(struct sctk_alloc_mapper_handler_s* pmi_handler);

