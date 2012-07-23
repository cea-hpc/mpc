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
}

#endif
