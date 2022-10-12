#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

int main(int argc, char** argv) {
	int process_Rank, size_Of_Cluster;
	int i, j;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
	MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);

	assert(size_Of_Cluster <= 4);

	int size = 32776;
	int *array_to_send = (int *)malloc(size*sizeof(int));
	int *array_to_recv = (int *)malloc(size*sizeof(int));
	/* Intializes random number generator */
	time_t t;
	srand((unsigned) time(&t));

	/* Intialize arrays */
	for(int i=0; i<size; i++) {
		array_to_send[i] = rand();
		array_to_recv[i] = 0;
	}

	for (i=0; i<size_Of_Cluster; i++) {
		if (process_Rank == i) {
			for (j=0; j< size_Of_Cluster; j++) {
				if (j != i)
					MPI_Send(array_to_send, size, MPI_INT, j, 1, MPI_COMM_WORLD);
			}
			printf("Process %d send %d to others\n", process_Rank, size);
		} else {
			MPI_Recv(array_to_recv, size, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf("Process %d recved %d to others÷\n", process_Rank, size);
		}
	}

	free(array_to_recv);
	free(array_to_send);

	MPI_Finalize();
	return 0;
}
