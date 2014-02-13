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
#include "mpc.h"
#include "mpcthread.h"
#include <stdio.h>
#include "sctk_config.h"
#include <assert.h>
#include "sctk_debug.h"
#include "sctk_launch.h"

#define __MPC_F_H__FORCE

#undef SCTK_USER_FORTRAN_EXT_
#include "mpc_f.h"
#include "mpcf_c.h"

#define SCTK_USER_FORTRAN_EXT_
#include "mpc_f.h"
#include "mpcf_c.h"


/* int mpc_user_main (int argc, char **argv) */
/* { */
/*   void (*func)() = NULL; */
/*   func = dlsym(NULL, "mpc_user_main__"); */
/*   if(func == NULL){ */
/*     func = dlsym(NULL, "mpc_user_main_"); */
/*   } */
/*   func(); */

/*   return 0; */
/*   assume (argc == 0); */
/*   assume (argv == NULL); */
/* } */
