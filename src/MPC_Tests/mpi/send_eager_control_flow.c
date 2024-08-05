#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv) {
        int i;
        int process_Rank, size_Of_Cluster;
        int num_send_requests = 128;

        MPI_Request *req_array;
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
        MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);

        if (process_Rank == 0) {
                req_array = (MPI_Request *)malloc((size_Of_Cluster - 1) * num_send_requests * sizeof(MPI_Request));
        } else {
                req_array = (MPI_Request *)malloc(num_send_requests * sizeof(MPI_Request));
        }

        int size = 1024;
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

        /* Pathological case: rank 0 does some computation while the other is
         * sending a lot of messages. */
        if(process_Rank == 0){
                for (i = 0; i < (size_Of_Cluster -1) * num_send_requests; i++) {
                        MPI_Irecv(array_to_recv, size, MPI_INT,
                                  MPI_ANY_SOURCE, 1, MPI_COMM_WORLD,
                                  &req_array[i]);
                }
        } else {
                for (i = 0; i < num_send_requests; i++) {
                        MPI_Isend(array_to_send, size, MPI_INT,
                                  0, 1, MPI_COMM_WORLD, &req_array[i]);
                }
        }

        if (process_Rank == 0) {
                MPI_Waitall((size_Of_Cluster - 1) * num_send_requests, req_array, MPI_STATUSES_IGNORE);
        } else {
                MPI_Waitall(num_send_requests, req_array, MPI_STATUSES_IGNORE);
        }

        free(array_to_recv);
        free(array_to_send);

        MPI_Finalize();
        return 0;
}
