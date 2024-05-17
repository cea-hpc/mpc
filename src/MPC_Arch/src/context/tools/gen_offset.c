#include <mpc_arch_context.h>
#include <sctk_reg.h>
#include <stdio.h>

/*
#define REG_RAX 0
#define REG_RBX 1
#define REG_RCX 2
#define REG_RDX 3
#define REG_RSI 4
#define REG_RDI 5
#define REG_RBP 6
#define REG_R8 7
#define REG_R9 8
#define REG_R10 9
#define REG_R11 10
#define REG_R12 11
#define REG_R13 12
#define REG_R14 13
#define REG_R15 14
#define REG_RSP 15
#define REG_RIP 16
#define REG_EFL 17

*/


#define print_reg(reg) printf("#define o%s 0x%x\n",__STRING(reg),(size_t)(&(uc.uc_mcontext.gregs[REG_##reg])) - (size_t)(&uc))

int main(int argc,char**argv){
	ucontext_t uc;

        print_reg(RAX);
        print_reg(RBX);
	print_reg(RCX);
        print_reg(RDX);
        print_reg(RSI);
        print_reg(RDI);
        print_reg(RBP);
        print_reg(R8);
        print_reg(R9);
        print_reg(R10);
        print_reg(R11);
        print_reg(R12);
        print_reg(R13);
        print_reg(R14);
        print_reg(R15);
        print_reg(RSP);
        print_reg(RIP);
        print_reg(EFL);

	printf("#define o%s 0x%x\n",__STRING(FPREGS),(size_t)(&(uc.uc_mcontext.fpregs)) - (size_t)(&uc));
	printf("#define o%s 0x%x\n",__STRING(FPREGSMEM),(size_t)(&(uc.__fpregs_mem)) - (size_t)(&uc));
	printf("#define o%s 0x%x\n",__STRING(MXCSR),(size_t)(&(uc.__fpregs_mem.mxcsr)) - (size_t)(&uc));
}
