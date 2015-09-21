#include <stdlib.h>
#include <stdio.h>
#include "sctk_shm_mapper.h"
#include <sys/types.h>
#include <unistd.h>
#include "mpi.h" 

/*! \brief Generate filename with localhost and pid  
 * @param option No option implemented 
 */

char *sctk_mpi_handler_gen_filename(void * option){
  char *filename = (char*) malloc(128*sizeof(char));
  if(filename == NULL){
	  printf("Can't gen filename : filename allocation failedi\n");
  	  }
  sprintf(filename, "%s_%d",(char*) option, getpid());
  return filename;
  }

/*! \brief Send filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

bool sctk_mpi_handler_send_filename(const char *filename, void* option){
  if(option == NULL || filename == NULL){
	  printf("Can't send filename : incorrect key or filename\n");
	  return false;
  	  }
  MPI_Bcast((void*)filename, 128, MPI_CHAR, 0,  MPI_COMM_WORLD); 
  MPI_Barrier(MPI_COMM_WORLD);
  return true;
  }

/*! \brief Recv filename via mpi, filename size must be lesser than 64 bytes  
 * @param option No option implemented 
 */

char* sctk_mpi_handler_recv_filename(void* option){
  char* filename = (char *) malloc(128*sizeof(char));
  if(option == NULL || filename == NULL){
	  printf("Can't recv filename : incorrect key or filename allocation failed\n");
  	  }
  MPI_Bcast((void*)filename, 128, MPI_CHAR, 0,  MPI_COMM_WORLD); 
  MPI_Barrier(MPI_COMM_WORLD);
  return filename;
  }


struct sctk_alloc_mapper_handler_s* sctk_shm_mpi_handler_init(void){
  
  char* localhost;
  struct sctk_alloc_mapper_handler_s* mpi_handler;

  mpi_handler = (struct sctk_alloc_mapper_handler_s*) malloc(sizeof(struct sctk_alloc_mapper_handler_s));
  localhost = (char*) malloc(16);
  mpi_handler->option = (void*) malloc(128*sizeof(char));

  mpi_handler->send_handler = sctk_mpi_handler_send_filename;
  mpi_handler->recv_handler = sctk_mpi_handler_recv_filename;
  mpi_handler->gen_filename = sctk_mpi_handler_gen_filename;
  
  // create key
  gethostname(localhost,256);
  sprintf(mpi_handler->option, "SHM_%s",localhost);
  free(localhost);
  return mpi_handler;
  }

void sctk_shm_mpi_handler_free(struct sctk_alloc_mapper_handler_s* mpi_handler){
	free(mpi_handler->option);
	free(mpi_handler);
	}

