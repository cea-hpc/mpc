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

#include "sctk_alloc_posix.h"
#include "sctk_config.h"
#include "sctk_spinlock.h"
#include "sctk_allocator.h"

#ifdef MPC_Threadss
#include "sctk_context.h"
#endif

void * sctk_user_mmap (void *start, size_t length, int prot, int flags,int fd, off_t offset)
{
	return mmap(start,length,prot,flags,fd,offset);
}

sctk_alloc_chain_t * __sctk_create_thread_memory_area(void)
{
	return sctk_alloc_posix_create_new_tls_chain();
}

void sctk_set_tls(sctk_alloc_chain_t * tls)
{
	sctk_alloc_posix_set_default_chain(tls);
};

void * sctk_get_heap_start(void)
{
	return (void*)SCTK_ALLOC_HEAP_BASE;
};

size_t sctk_get_heap_size(void)
{
	return SCTK_ALLOC_HEAP_SIZE;
};

void * __sctk_malloc_new(size_t size,sctk_alloc_chain_t * chain)
{
	return sctk_alloc_chain_alloc(chain,size);
};

void * __sctk_malloc (size_t size,sctk_alloc_chain_t * chain)
{
	return sctk_alloc_chain_alloc(chain,size);
};

char * sctk_alloc_mode (void)
{
	return "MPC allocator";
};

void __sctk_free(void * ptr,sctk_alloc_chain_t * chain)
{
	sctk_alloc_chain_free(chain,ptr);
};
