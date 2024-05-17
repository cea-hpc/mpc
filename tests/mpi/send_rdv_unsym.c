#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

int main(int argc, char** argv) {
	int process_Rank, size_Of_Cluster;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
	MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);

	assert(size_Of_Cluster == 2);

	int send_size = 8194;
	int recv_size = 16388;
	int *array_to_send = (int *)malloc(send_size*sizeof(int));
	int *array_to_recv = (int *)malloc(recv_size*sizeof(int));
	/* Intializes random number generator */
	time_t t;
	srand((unsigned) time(&t));

	/* Intialize arrays */
	for(int i=0; i<send_size; i++) {
		array_to_send[i] = rand();
	}
	for(int i=0; i<recv_size; i++) {
		array_to_recv[i] = 0;
	}

	if(process_Rank == 0){
		MPI_Send(array_to_send, send_size, MPI_INT, 1, 1, MPI_COMM_WORLD);
		printf("Sent %d octets\n", send_size*4);
	}
	else {
		MPI_Recv(array_to_recv, recv_size, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Received %d octets\n", send_size*4);
	}

	/* Check */
	if (process_Rank == 1) {
		int array_check = 1;
		for (int i=0; i< send_size; i++) {
			if (array_to_recv[i] != array_to_send[i])
				array_check = 0;
		}
		if (!array_check) {
			printf("ERROR: received vector is different");
		}
	}

	free(array_to_recv);
	free(array_to_send);

	MPI_Finalize();
	return 0;
}
