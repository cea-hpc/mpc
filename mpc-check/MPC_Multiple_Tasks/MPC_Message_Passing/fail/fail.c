#include <stdio.h>
#include <mpc.h>
#include <assert.h>

int main(int argc, char** argv){
  int rank;
  MPC_Init(&argc,&argv);
  MPC_Comm_rank(MPC_COMM_WORLD,&rank);
  fprintf(stderr,"Hello World %d\n",rank);
  if(rank == 0){
    fprintf(stderr,"Assertion on %d\n",rank);
    assert(0);	
  }
  fprintf(stderr,"Hello World %d\n",rank);
  MPC_Finalize();
  return 0;
}
