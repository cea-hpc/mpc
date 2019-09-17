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
#include <stdio.h>
#include "sctk_generic.h"
#include <signal.h>
#include <setjmp.h>


int
sctk_specific_optim ()
{
  return 0;
}


void
sctk_longjmp (jmp_buf env, int val)
{
/*   fprintf (stderr, "longjmp %p\n", env->__jmpbuf); */
  longjmp (env, val);
}

int
sctk_setjmp (jmp_buf env)
{
  int tmp;
/*   fprintf (stderr, "setjmp %p\n", env->__jmpbuf); */
  tmp = setjmp (env);
/*   fprintf (stderr, "setjmp return %d %p\n", tmp, env->__jmpbuf); */
  return tmp;
}
