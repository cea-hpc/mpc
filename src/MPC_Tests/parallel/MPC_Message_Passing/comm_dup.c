#include <mpc.h>

int main (int argc, char** argv){
mpc_lowcomm_communicator_t comm;
MPC_Comm_dup(MPC_COMM_WORLD,&comm);
MPC_Comm_dup(MPC_COMM_WORLD,&comm);
return 0;
}
