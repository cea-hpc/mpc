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
/* #   - Adam Julien julien.adam.ocre@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef _MSC_VER
#include <windows.h>
#include <cstdio>
#include "../sctk_alloc_posix.h"
#include "mhook-lib/mhook.h"
	
static void * ptr_origin_function_malloc 	= (void *) malloc;
static void * ptr_origin_function_free 		= (void *) free;
static void * ptr_origin_function_realloc 	= (void *) realloc;
static void * ptr_origin_function_calloc 	= (void *) calloc;
static void * ptr_origin_operator_delete = (void *) operator delete[];
static void * ptr_origin_function_memalign 	= (void *) _aligned_malloc;

extern "C" __declspec(dllexport)
void * sctk_alloc_win_memalign(size_t size, size_t alignment)
{
	//test AND to check if alignement is pow of 2
	if((alignment & (alignment - 1)))
		abort();

	return sctk_memalign(alignment, size);
}

extern "C" __declspec(dllexport)
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
	int ok = 1;

	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			ok = (	Mhook_SetHook( (PVOID *)&ptr_origin_function_malloc, 	(PVOID)sctk_malloc)					&&
					Mhook_SetHook( (PVOID *)&ptr_origin_function_free,		(PVOID)sctk_free) 					&&
					Mhook_SetHook( (PVOID *)&ptr_origin_operator_delete,	(PVOID)sctk_free)					&&
					Mhook_SetHook( (PVOID *)&ptr_origin_function_memalign,	(PVOID)sctk_alloc_win_memalign)		&&
					Mhook_SetHook( (PVOID *)&ptr_origin_function_realloc,	(PVOID)sctk_realloc)				&&
					Mhook_SetHook( (PVOID *)&ptr_origin_function_calloc, 	(PVOID)sctk_calloc)					);
			break;
		case DLL_PROCESS_DETACH:
			/*
			Mhook_Unhook((PVOID*)&ptr_origin_function_malloc);
			Mhook_Unhook((PVOID*)&ptr_origin_function_free);
			Mhook_Unhook((PVOID*)&ptr_origin_function_realloc);
			Mhook_Unhook((PVOID*)&ptr_origin_function_calloc);
			Mhook_Unhook((PVOID*)&ptr_origin_operator_delete);
			Mhook_Unhook((PVOID*)&ptr_origin_function_memalign);
			*/
			break;
	}
	return ok;
}

#endif