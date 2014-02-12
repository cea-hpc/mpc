/* Copyright (C) 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _SYS_SCTK_UCONTEXT_H
#define _SYS_SCTK_UCONTEXT_H	1

#include <stdlib.h>
#include <stdint.h>
#include <sctk_reg.h>

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

void mpc__makecontext (sctk_ucontext_t *ucp, void (*func) (void), int argc, ...); 
int mpc__setcontext (const sctk_ucontext_t *ucp);
int mpc__swapcontext (sctk_ucontext_t *oucp, const sctk_ucontext_t *ucp);
int mpc__getcontext (sctk_ucontext_t *ucp);

#ifdef ENABLE_GEN_COMPATIBILITY
#define makecontext mpc__makecontext
#define setcontext mpc__setcontext
#define getcontext mpc__getcontext
#define swapcontext mpc__swapcontext
#endif

#endif /* sys/ucontext.h */
