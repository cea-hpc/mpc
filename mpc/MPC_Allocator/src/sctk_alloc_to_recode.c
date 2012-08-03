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

#if !defined(MPC_Common) || defined(MPC_PosixAllocator)

/************************** HEADERS ************************/
#include <sctk_config.h>
#include <sctk_spinlock.h>
#include "sctk_alloc_posix.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_posix.h"
#include "sctk_alloc_chain.h"
#include <sys/mman.h>

#ifdef MPC_Threadss
#include "sctk_context.h"
#endif

/************************* FUNCTION ************************/
void * sctk_user_mmap (void *start, size_t length, int prot, int flags,int fd, off_t offset)
{
	return mmap(start,length,prot,flags,fd,offset);
}

/************************* FUNCTION ************************/
struct sctk_alloc_chain * __sctk_create_thread_memory_area(void)
{
	return sctk_alloc_posix_create_new_tls_chain();
}

/************************* FUNCTION ************************/
void sctk_set_tls(struct sctk_alloc_chain * tls)
{
	sctk_alloc_posix_set_default_chain(tls);
};

/************************* FUNCTION ************************/
void * sctk_get_heap_start(void)
{
	//now fully use mmap, so no strict heap definition
	return NULL;
};

/************************* FUNCTION ************************/
size_t sctk_get_heap_size(void)
{
	//now fully use mmap, so no strict heap definition
	return 0;
};

/************************* FUNCTION ************************/
void * __sctk_malloc_new(size_t size,struct sctk_alloc_chain * chain)
{
	return sctk_alloc_chain_alloc(chain,size);
};

/************************* FUNCTION ************************/
void * __sctk_malloc (size_t size,struct sctk_alloc_chain * chain)
{
	return sctk_alloc_chain_alloc(chain,size);
};

/************************* FUNCTION ************************/
char * sctk_alloc_mode (void)
{
	return "MPC allocator";
};

/************************* FUNCTION ************************/
void __sctk_free(void * ptr,struct sctk_alloc_chain * chain)
{
	sctk_alloc_chain_free(chain,ptr);
};

#endif //!defined(MPC_Common) || defined(MPC_PosixAllocator)
