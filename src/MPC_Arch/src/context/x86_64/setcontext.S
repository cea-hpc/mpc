/* Install given context.
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
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

/*  int __setcontext (const ucontext_t *ucp)

  Restores the machine context in UCP and thereby resumes execution
  in that context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to restore anything
  other than the PRESERVED state.  */

.global mpc__setcontext
.type mpc__setcontext,@function
.align 4;
mpc__setcontext:
	/* Save argument since syscall will destroy it.  */
	pushq	%rdi

	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	movq	SCTK_oFPREGS(%rdi), %rcx
	fldenv	(%rcx)
	ldmxcsr SCTK_oMXCSR(%rdi)

	movq	SCTK_oRSP(%rdi), %rsp
	movq	SCTK_oRBX(%rdi), %rbx
	movq	SCTK_oRBP(%rdi), %rbp
	movq	SCTK_oR12(%rdi), %r12
	movq	SCTK_oR13(%rdi), %r13
	movq	SCTK_oR14(%rdi), %r14
	movq	SCTK_oR15(%rdi), %r15

	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	SCTK_oRIP(%rdi), %rcx
	pushq	%rcx

	movq	SCTK_oRSI(%rdi), %rsi
	movq	SCTK_oRDX(%rdi), %rdx
	movq	SCTK_oRCX(%rdi), %rcx
	movq	SCTK_oR8(%rdi), %r8
	movq	SCTK_oR9(%rdi), %r9

	/* Setup finally  %rdi.  */
	movq	SCTK_oRDI(%rdi), %rdi

	/* Clear rax to indicate success.  */
	xorl	%eax, %eax

	ret
.size mpc__setcontext,.-mpc__setcontext;
