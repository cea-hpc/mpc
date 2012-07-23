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
#include "mhook-lib/mhook.h"

/******************* STRUCT ********************/
struct sctk_alloc_function {
	//String with origin function name
	char * sctk_alloc_origin_function;
	//a pointer on function to hooking
	void * sctk_alloc_function_ptr;
};

//import functions from allocator libraries to hooking windows symbols
extern "C" __declspec(dllimport) void sctk_free(void *);
extern "C" __declspec(dllimport) void * sctk_malloc(size_t);
extern "C" __declspec(dllimport) void * sctk_calloc(size_t, size_t);
extern "C" __declspec(dllimport) void * sctk_realloc(size_t);
extern "C" __declspec(dllimport) void * sctk_memalign(size_t, size_t);
extern "C" __declspec(dllexport) void * sctk_alloc_win_memalign(size_t size, size_t alignment); //function defined here to be used in const array.
extern "C" void sctk_alloc_posix_base_init(void);
extern "C" void sctk_alloc_tls_chain_local_reset(void);
extern "C" void sctk_alloc_posix_setup_tls_chain(void);

/****************** CONSTS ****************************/
static const char* sctk_alloc_libraries_name[]= 
{"MSVCR71.DLL", "MSVCR71d.DLL",	"MSVCM71.DLL",	"MSVCR80.DLL" ,	"MSVCR80d.DLL",	"MSVCM80.DLL",
 "MSVCR90.DLL",	"MSVCR90d.DLL",	"MSVCM90.DLL",	"MSVCR100.DLL", "MSVCR100d.DLL","MSVCM100.DLL"};

static const struct sctk_alloc_function sctk_alloc_hook_functions[]={{ "malloc" 			, sctk_malloc 				},
																	 { "free"				, sctk_free					},
																	 { "_aligned_malloc"	, sctk_alloc_win_memalign	},
																	 { "realloc"			, sctk_realloc				},
																	 { "calloc"				, sctk_calloc				},
																	 { "??3@YAXPEAX@Z"		, sctk_free					}, //delete operator symbol
																	 { "??_V@YAXPEAX@Z"		, sctk_free					}, //delete[] operator symbol*/
																	 { "??2@YAPEAX_K@Z"		, sctk_malloc				}, //new operator symbol
																	 { "??_U@YAPEAX_K@Z"	, sctk_malloc				}  //new[] operator symbol
													};

extern "C" __declspec(dllexport)
void * sctk_alloc_win_memalign(size_t size, size_t alignment)
{
	//test AND to check if alignement is pow of 2
	if((alignment & (alignment - 1)))
		abort();

	return sctk_memalign(alignment, size);
}

#ifdef SCTK_ALLOC_DEBUG
#define debug_puts(x) puts(x)
#define debug_printf(x,...) printf((x),__VA_ARGS__)
#else //SCTK_ALLOC_DEBUG
#define debug_printf(x,...) do {} while(0)
#define debug_puts(x) do {} while(0)
#endif //SCTK_ALLOC_DEBUG

static int sctk_alloc_erasing_symbols(HMODULE module)
{
	int ok = 1;
	for(int i = 0; i < (sizeof(sctk_alloc_hook_functions)/sizeof(struct sctk_alloc_function)); i++)
	{
		//contains pointer on windows function
		void * tmp_addr = GetProcAddress(module,sctk_alloc_hook_functions[i].sctk_alloc_origin_function);
		debug_printf("\tFUNCTION %s : ", sctk_alloc_hook_functions[i].sctk_alloc_origin_function);
		//Hook each functions		
		
		if(tmp_addr){
			debug_printf("%p\n",  tmp_addr);
			//printf("sctk_%s address: %p\n", sctk_alloc_hook_functions[i].sctk_alloc_origin_function,sctk_alloc_hook_functions[i].sctk_alloc_function_ptr);
			ok = ok && (Mhook_SetHook( 	(PVOID *)&tmp_addr,(PVOID)sctk_alloc_hook_functions[i].sctk_alloc_function_ptr));
			if (ok) debug_puts("OK"); else debug_puts("ERROR");
		} else {
			printf("NOT FOUND\n");
		}
	}
	return ok;
}

extern "C" __declspec(dllexport)
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
	int ok = 1, i = 0;
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			//init the alloca chaine
			//puts("base init");
			sctk_alloc_posix_base_init();
			sctk_free(NULL);

			for(int i = 0; i < (sizeof(sctk_alloc_libraries_name)/sizeof(char*)); i++)
			{
				debug_printf("LIB %s : \t",sctk_alloc_libraries_name[i]);
									 
				HMODULE sctk_module = GetModuleHandle(sctk_alloc_libraries_name[i]);
				
				//Hook functions for each library
				if(sctk_module){
					debug_printf("\n");
					ok = ok && sctk_alloc_erasing_symbols(sctk_module);
				} else {
					debug_printf("NOT FOUND\n");
				}
			}
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
			debug_puts("process detach");
			break;
		 case DLL_THREAD_ATTACH: 
			 debug_puts("thread attach");
			 /*sctk_alloc_tls_chain_local_reset();
			 puts("middle thread attach");
			 sctk_alloc_posix_setup_tls_chain();
			 puts("end thread attach");
			 free(malloc(16));*/
            break; 
         case DLL_THREAD_DETACH: 
			//sctk_alloc_chain_mark_for_destroy(....);
			 break;
	}
	return ok;
}

#endif