/* Save current context and install the given one.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
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


/* int __swapcontext (ucontext_t *oucp, const ucontext_t *ucp);

  Saves the machine context in oucp such that when it is activated,
  it appears as if __swapcontextt() returned again, restores the
  machine context in ucp and thereby resumes execution in that
  context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to save anything
  other than the PRESERVED state.  */

.global mpc__swapcontext
.type mpc__swapcontext,@function
.align 4;
mpc__swapcontext:
	/* Save the preserved registers, the registers used for passing args,
	   and the return address.  */
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
	stmxcsr SCTK_oMXCSR(%rdi)

	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	movq	SCTK_oFPREGS(%rsi), %rcx
	fldenv	(%rcx)
	ldmxcsr SCTK_oMXCSR(%rsi)

	/* Load the new stack pointer and the preserved registers.  */
	movq	SCTK_oRSP(%rsi), %rsp
	movq	SCTK_oRBX(%rsi), %rbx
	movq	SCTK_oRBP(%rsi), %rbp
	movq	SCTK_oR12(%rsi), %r12
	movq	SCTK_oR13(%rsi), %r13
	movq	SCTK_oR14(%rsi), %r14
	movq	SCTK_oR15(%rsi), %r15

	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	SCTK_oRIP(%rsi), %rcx
	pushq	%rcx

	/* Setup registers used for passing args.  */
	movq	SCTK_oRDI(%rsi), %rdi
	movq	SCTK_oRDX(%rsi), %rdx
	movq	SCTK_oRCX(%rsi), %rcx
	movq	SCTK_oR8(%rsi), %r8
	movq	SCTK_oR9(%rsi), %r9

	/* Setup finally  %rsi.  */
	movq	SCTK_oRSI(%rsi), %rsi

	/* Clear rax to indicate success.  */
	xorl	%eax, %eax

	ret
.size mpc__swapcontext,.-mpc__swapcontext;
