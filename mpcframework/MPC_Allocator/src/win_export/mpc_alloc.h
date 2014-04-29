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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_WIN_ALLOCATOR_H
#define MPC_WIN_ALLOCATOR_H

//import functions from allocator libraries to hooking windows symbols
extern "C"
{
	__declspec(dllimport) void sctk_free(void *);
	__declspec(dllimport) void * sctk_malloc(size_t);
	__declspec(dllimport) void * sctk_calloc(size_t, size_t);
	__declspec(dllimport) void * sctk_realloc(size_t);
	__declspec(dllimport) void * sctk_memalign(size_t, size_t);
	__declspec(dllimport) void * sctk_alloc_posix_numa_migrate(void);
	__declspec(dllimport) void sctk_alloc_posix_chain_print_stat(void);
}

#endif
