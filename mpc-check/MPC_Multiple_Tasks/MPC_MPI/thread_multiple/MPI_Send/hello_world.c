/* ############################# MPC License ############################## */
/* # Thu Apr 24 18:48:27 CEST 2008                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* #                                                                      # */
/* # This file is part of the MPC Library.                                # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <mpi.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#define NB_COMM 10
#define BUFF_SIZE 10

int send_buffer[BUFF_SIZE];
int recv_buffer[BUFF_SIZE];

void* send_thread(void* arg){
  int rank, size;
  int i;
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  if((rank == 0) || (size-1 == rank)){
    int dest; 

    fprintf(stderr,"Launch send thread for %d\n",rank);

    dest = 0;
    if(rank == 0){
      dest = size-1;
    }
    for(i = 0; i < NB_COMM; i++){
/*       fprintf(stderr,"PREPARE SEND from %d to %d message %d\n",rank,dest,i); */
      assert(MPI_Send( send_buffer, BUFF_SIZE, MPI_INT, dest,
		       i, MPI_COMM_WORLD ) == MPI_SUCCESS);
      fprintf(stderr,"SEND from %d to %d message %d\n",rank,dest,i);
    }
  }
  return NULL;
}

void* recv_thread(void* arg){
  int rank, size;
  int i;
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  if((rank == 0) || (size-1 == rank)){
    int dest; 
    fprintf(stderr,"Launch recv thread for %d\n",rank);
    dest = 0;
    if(rank == 0){
      dest = size-1;
    }
    for(i = 0; i < NB_COMM; i++){
      MPI_Status status;
/*       fprintf(stderr,"PREPARE RECV from %d to %d message %d\n",dest,rank,i); */
      memset(recv_buffer,0,BUFF_SIZE*sizeof(int));
      assert(MPI_Recv( recv_buffer, BUFF_SIZE, MPI_INT, dest,
		       i, MPI_COMM_WORLD, &status ) == MPI_SUCCESS);
      assert(memcmp(send_buffer,recv_buffer,BUFF_SIZE*sizeof(int)) == 0);
      fprintf(stderr,"RECV from %d to %d message %d\n",dest,rank,i);
    }
  }
  return NULL;
}

int
main (int argc, char **argv)
{
  int rank, size;
  int level;
  pthread_t pids[2];
  char name[1024];
  MPI_Init_thread (&argc, &argv,MPI_THREAD_MULTIPLE,&level);
  assert(level == MPI_THREAD_MULTIPLE);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  assert(rank < size);
  gethostname (name, 1023);
  printf ("Hello world from process %d of %d %s\n", rank, size, name);

  pthread_create(&(pids[0]), NULL,send_thread,NULL);
  pthread_create(&(pids[1]), NULL,recv_thread,NULL);

  pthread_join(pids[0],NULL);
  pthread_join(pids[1],NULL);

  MPI_Finalize ();
  return 0;
}
