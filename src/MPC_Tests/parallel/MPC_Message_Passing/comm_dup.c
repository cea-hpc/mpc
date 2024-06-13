#include <mpi.h>

int main (){
mpc_lowcomm_communicator_t comm;
MPI_Comm_dup(MPI_COMM_WORLD,&comm);
MPI_Comm_dup(MPI_COMM_WORLD,&comm);
return 0;
}
