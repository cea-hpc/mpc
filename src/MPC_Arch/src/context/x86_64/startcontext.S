/* Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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


/* This is the helper code which gets called if a function which is
   registered with 'makecontext' returns.  In this case we have to
   install the context listed in the uc_link element of the context
   'makecontext' manipulated at the time of the 'makecontext' call.
   If the pointer is NULL the process must terminate.  */

#include "mpc_context_registers_x86_64.h"

.global mpc__start_context
.type mpc__start_context,@function
.align 4;
mpc__start_context:


        /* This removes the parameters passed to the function given to
           'makecontext' from the stack.  RBX contains the address
           on the stack pointer for the next context.  */
        movq    %rbx, %rsp

        popq    %rdi                    /* This is the next context.  */
        testq   %rdi, %rdi
        je      2f                      /* If it is zero exit.  */

        call    mpc__setcontext@PLT

        /* If this returns (which can happen if the syscall fails) we'll
           exit the program with the return error value (-1).  */

2:      movq    %rax,%rdi
        call    exit@PLT
        /* The 'exit' call should never return.  In case it does cause
           the process to terminate.  */
        hlt
.size mpc__start_context,.-mpc__start_context;
