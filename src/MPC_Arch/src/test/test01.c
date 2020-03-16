#include <mpc_arch_context.h>
#include <stdio.h>

void test(int a){
  fprintf(stderr,"%d\n", a);
}

int main(int argc, char** argv){
	printf("OK\n");
	return 0;
}
