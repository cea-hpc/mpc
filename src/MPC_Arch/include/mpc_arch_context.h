#include <ucontext.h>

#ifndef MPC_ARCH_CONTEXT
#define MPC_ARCH_CONTEXT

/* Include needed to export SCTK_*_ARCH_SCTK */
#include <mpc_arch.h>

#include <stdlib.h>
#include <stdint.h>

#if defined(MPC_I686_ARCH) || defined(MPC_X86_64_ARCH)

/*****************************
 * CONTEXT SWITCH DEFINITION *
 *****************************/

/* Type for general register.  */
typedef long int sctk_greg_t;

/* Number of general registers.  */
#define SCTK_NGREG	23

/* Container for all general registers.  */
typedef sctk_greg_t sctk_gregset_t[SCTK_NGREG];

struct sctk__libc_fpxreg
{
	unsigned short int significand[4];
	unsigned short int exponent;
	unsigned short int padding[3];
};

struct sctk__libc_xmmreg
{
	uint32_t	element[4];
};

typedef struct
{
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
} sctk_stack_t;

struct sctk__libc_fpstate
{
	/* 64-bit FXSAVE format.  */
	uint16_t		cwd;
	uint16_t		swd;
	uint16_t		ftw;
	uint16_t		fop;
	uint64_t		rip;
	uint64_t		rdp;
	uint32_t		mxcsr;
	uint32_t		mxcr_mask;
	struct sctk__libc_fpxreg	_st[8];
	struct sctk__libc_xmmreg	_xmm[16];
	uint32_t		padding[24];
};

/* Structure to describe FPU registers.  */
typedef struct sctk__libc_fpstate *sctk_fpregset_t;

/* Context to describe whole processor state.  */
typedef struct
{
	sctk_gregset_t gregs;
	/* Note that fpregs is a pointer.  */
	sctk_fpregset_t fpregs;
	unsigned long __reserved1 [8];
} sctk_mcontext_t;

/* Userlevel context.  */
typedef struct sctk_ucontext
{
	unsigned long int uc_flags;
	struct sctk_ucontext *uc_link;
	sctk_stack_t uc_stack;
	sctk_mcontext_t uc_mcontext;
	struct sctk__libc_fpstate __fpregs_mem;
} sctk_ucontext_t;

void mpc__makecontext ( sctk_ucontext_t *ucp, void ( *func ) ( void ), int argc, ... );
int mpc__setcontext ( const sctk_ucontext_t *ucp );
int mpc__swapcontext ( sctk_ucontext_t *oucp, const sctk_ucontext_t *ucp );
int mpc__getcontext ( sctk_ucontext_t *ucp );


#else

/* Generic Implementation */

typedef ucontext_t sctk_ucontext_t;

#define mpc__makecontext makecontext
#define mpc__setcontext setcontext
#define mpc__getcontext getcontext
#define mpc__swapcontext swapcontext

#endif /* CONTEXT SWITCH ARCHITECTURE */

#endif /* MPC_ARCH_CONTEXT */
