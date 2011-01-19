/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
	
#define JB_BX	0
#define JB_SI	1
#define JB_DI	2
#define JB_BP	3
#define JB_SP	4
#define JB_PC	5
#define JB_SIZE 24
#define JMPBUF 4
#define PCOFF 0
#define	VAL 4+4
	
.global mpc__setjmp
.align 4;
mpc__setjmp:	
	movl JMPBUF(%esp), %eax

     	/* Save registers.  */
	movl %ebx, (JB_BX*4)(%eax)
	movl %esi, (JB_SI*4)(%eax)
	movl %edi, (JB_DI*4)(%eax)
	leal JMPBUF(%esp), %ecx	/* Save SP as it will be after we return.  */
     	movl %ecx, (JB_SP*4)(%eax)
	movl PCOFF(%esp), %ecx	/* Save PC we are returning to now.  */
     	movl %ecx, (JB_PC*4)(%eax)
	LEAVE /* pop frame pointer to prepare for tail-call.  */
	movl %ebp, (JB_BP*4)(%eax) /* Save caller's frame pointer.  */
	
	xorl %eax, %eax
	ret


