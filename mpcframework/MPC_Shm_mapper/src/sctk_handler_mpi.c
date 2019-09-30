#ifndef SCTK_LIB_MODE
#include "mpi.h"
#include "sctk_collective_communications.h"
#include "sctk_debug.h"
#include "sctk_shm_mapper.h"
#include "mpc_common_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

char *sctk_mpi_handler_gen_filename(void *option, __UNUSED__ void *option1) {
  char *filename = (char*) malloc(128*sizeof(char));
  if(filename == NULL){
	  printf("Can't gen filename : filename allocation failedi\n");
  	  }
          sprintf(filename, "mpc-%s_%d_%ld_%d", (char *)option, getpid(), time(0),
                  rand());
          return filename;
  }

/*! \brief Send filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

  bool sctk_mpi_handler_send_filename(const char *filename, void *option,
                                      void *option1) {
    if (option == NULL || filename == NULL) {
      printf("Can't send filename : incorrect key or filename\n");
      return false;
    }

    long long int icomm = (long long int)option1;
    MPI_Comm comm = (MPI_Comm)icomm;

    sctk_broadcast((void *)filename, 128 * sizeof(char), 0, (sctk_communicator_t)comm);
    return true;
  }

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

  char *sctk_mpi_handler_recv_filename(void *option, void *option1) {
    char *filename = (char *)malloc(128 * sizeof(char));
    if (option == NULL || filename == NULL) {
      printf("Can't recv filename : incorrect key or filename allocation "
             "failed\n");
    }

    long long int icomm = (long long int)option1;
    MPI_Comm comm = (MPI_Comm)icomm;

    sctk_broadcast((void *)filename, 128 * sizeof(char), 0, (sctk_communicator_t)comm);

    return filename;
  }

  struct sctk_alloc_mapper_handler_s *
  sctk_shm_mpi_handler_init(sctk_communicator_t comm) {

    char *localhost;
    struct sctk_alloc_mapper_handler_s *mpi_handler;

    mpi_handler = (struct sctk_alloc_mapper_handler_s *)malloc(
        sizeof(struct sctk_alloc_mapper_handler_s));
    localhost = (char *)malloc(64);

    mpi_handler->option = (void *)malloc(128 * sizeof(char));
    assume(mpi_handler->option != NULL);

    int64_t pcomm = comm;
    mpi_handler->option1 = (void *)pcomm;

    mpi_handler->send_handler = sctk_mpi_handler_send_filename;
    mpi_handler->recv_handler = sctk_mpi_handler_recv_filename;
    mpi_handler->gen_filename = sctk_mpi_handler_gen_filename;

    // create key
    // hostname is truncate if too long
    gethostname(localhost, 64 - 1);
    sprintf(mpi_handler->option, "SHM_%s", localhost);
    free(localhost);
    return mpi_handler;
  }

void sctk_shm_mpi_handler_free(struct sctk_alloc_mapper_handler_s* mpi_handler){
	free(mpi_handler->option);
	free(mpi_handler);
	}
#endif
