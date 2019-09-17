#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "sctk_shm_mapper.h"
#include "sctk_pmi.h"

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */



char *sctk_pmi_handler_gen_filename(void *option, __UNUSED__ void *option1) {
  char *filename = sctk_malloc(sctk_pmi_get_max_key_len());
  if (filename == NULL) {
    sctk_nodebug("Can't gen filename : filename allocation failed");
  }
  sprintf(filename, "%s_%d", (char *)option, getpid());
  sctk_nodebug("SHM filename generated: %s", filename);
  return filename;
}

/*! \brief Send filename via pmi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

bool sctk_pmi_handler_send_filename(const char *filename, void *option,
                                    __UNUSED__ void *option1) {
  if(option == NULL || filename == NULL){
	  sctk_nodebug("Can't send filename : incorrect key or filename");
	  return 0;
  	  }
  sctk_nodebug("opt : %s\n", (char*) option);
  sctk_nodebug("filename : %s\n", filename);
  sctk_pmi_put_connection_info_str((void*)filename, (char*) option); 
  sctk_pmi_barrier();
  return 1;
  }

/*! \brief Recv filename via pmi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

  char *sctk_pmi_handler_recv_filename(void *option, __UNUSED__ void *option1) {
    char *filename = sctk_malloc(sctk_pmi_get_max_key_len());
    if (option == NULL || filename == NULL) {
      assume_m(
          0,
          "Can't recv filename : incorrect key or filename allocation failed");
    }
    sctk_nodebug("opt : %s\n", (char *)option);
    sctk_pmi_barrier();
    sctk_pmi_get_connection_info_str(filename, 64, (char *)option);
    sctk_nodebug("filename : %s\n", filename);
    return filename;
  }


struct sctk_alloc_mapper_handler_s* sctk_shm_pmi_handler_init(char* option){
  
  char* localhost;
  struct sctk_alloc_mapper_handler_s* pmi_handler;

  assume_m(pmi_handler = (struct sctk_alloc_mapper_handler_s*) sctk_malloc(sizeof(struct sctk_alloc_mapper_handler_s)),"failed mpi_handler");
  assume_m(localhost = sctk_malloc(256), "failed localhost name allocation");
  assume_m(pmi_handler->option = sctk_malloc(sctk_pmi_get_max_key_len()),"failed key allocation");

  pmi_handler->send_handler = sctk_pmi_handler_send_filename;
  pmi_handler->recv_handler = sctk_pmi_handler_recv_filename;
  pmi_handler->gen_filename = sctk_pmi_handler_gen_filename;
  
  // create key
  gethostname(localhost,256);
  sprintf(pmi_handler->option, "SHM_%s_%s",localhost, (char*) option);
  sctk_free(localhost);
  return pmi_handler;
  }

  void
  sctk_shm_pmi_handler_free(struct sctk_alloc_mapper_handler_s *pmi_handler) {
    sctk_free(pmi_handler->option);
    sctk_free(pmi_handler);
  }
