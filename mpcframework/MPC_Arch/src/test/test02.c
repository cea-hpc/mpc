#include <mpc_arch_context.h>
#include <stdio.h>

void test(int a){
  fprintf(stderr,"%d\n",a);
}

int main(int argc, char** argv){
	sctk_ucontext_t uc;

	mpc__getcontext(&uc);
	printf("OK\n");
	return 0;
}
