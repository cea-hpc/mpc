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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef _MSC_VER
#include <windows.h>
#include <stdio.h>

//linker options
#if defined(_WIN64)
#pragma comment(linker, "/include:mpc_allocator_win_load")
#else
#pragma comment(linker, "/include:_mpc_allocator_win_load")
#endif

//used to force library loading
__declspec(dllimport) void sctk_free(void *);
  
void mpc_allocator_win_load(void){
	LoadLibraryA ("mpc_win_allocator.dll");
	sctk_free(NULL);
}
#endif
