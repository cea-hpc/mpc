/* Save current context.
   Copyright (C) 2002, 2005, 2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "mpc_context_registers_x86_64.h"

/*  int __getcontext (ucontext_t *ucp)

  Saves the machine context in UCP such that when it is activated,
  it appears as if __getcontext() returned again.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to save anything
  other than the PRESERVED state.  */


.global mpc__getcontext
.type mpc__getcontext,@function
.align 4;
mpc__getcontext:
	/* Save the preserved registers, the registers used for passing
	   args, and the return address.  */
	movq	%rbx, SCTK_oRBX(%rdi)
	movq	%rbp, SCTK_oRBP(%rdi)
	movq	%r12, SCTK_oR12(%rdi)
	movq	%r13, SCTK_oR13(%rdi)
	movq	%r14, SCTK_oR14(%rdi)
	movq	%r15, SCTK_oR15(%rdi)

	movq	%rdi, SCTK_oRDI(%rdi)
	movq	%rsi, SCTK_oRSI(%rdi)
	movq	%rdx, SCTK_oRDX(%rdi)
	movq	%rcx, SCTK_oRCX(%rdi)
	movq	%r8, SCTK_oR8(%rdi)
	movq	%r9, SCTK_oR9(%rdi)

	movq	(%rsp), %rcx
	movq	%rcx, SCTK_oRIP(%rdi)
	leaq	8(%rsp), %rcx		/* Exclude the return address.  */
	movq	%rcx, SCTK_oRSP(%rdi)

	/* We have separate floating-point register content memory on the
	   stack.  We use the __fpregs_mem block in the context.  Set the
	   links up correctly.  */

	leaq	SCTK_oFPREGSMEM(%rdi), %rcx
	movq	%rcx, SCTK_oFPREGS(%rdi)
	/* Save the floating-point environment.  */
	fnstenv	(%rcx)
	fldenv	(%rcx)
	stmxcsr SCTK_oMXCSR(%rdi)

	/* All done, return 0 for success.  */
	xorl	%eax, %eax
	ret
.size mpc__getcontext,.-mpc__getcontext;
