/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#define _GNU_SOURCE

#include <hwloc.h>

#include "sctk_alloc.h"
#include "mpc_topology.h"
#include "mpc_thread_accessor.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_asm.h"


#include "thread.h"

#include <unistd.h>
#include <tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utlist.h>
#include <dlfcn.h>

#if MPC_USE_EXTLS
#include <extls.h>
#include <extls_dynamic.h>
#endif

/** storage slot used by libextls to store its own data.
 * The address of this TLS is provided by sctk_get_ctx_addr()
 */
__thread void *sctk_extls_storage = NULL;

/**
 * Locate all dynamic initializers generated by patched GCC.
 * This function does not call them. This function should be called as early
 * as possible by MPC (probably in sctk_launch.c), before any MPI task
 *
 * @returns 1 if something's gone wrong, 0 otherwise
 */
int sctk_locate_dynamic_initializers()
{
#ifdef MPC_USE_EXTLS
	char *      ckpt_wrapper = (mpc_common_get_flags()->checkpoint_enabled) ? "dmtcp_nocheckpoint" : "";
	extls_ret_t ret          = extls_locate_dynamic_initializers(ckpt_wrapper);
	return (ret == EXTLS_SUCCESS) ? 0 : 1;

#else
	return 0;
#endif
}

/**
 * Call all dynamic initializers previously discovered.
 * This function erases current values and replaces with the initial value
 * For instance, for task-level initialization, this function should be called
 * during task instanciation, in order to have one init copy per task
 * @returns 1 if something's gone wrong, 0 othewise.
 */
int sctk_call_dynamic_initializers()
{
#ifdef MPC_USE_EXTLS
	extls_ret_t ret = extls_call_dynamic_initializers();

	if(ret != EXTLS_SUCCESS)
	{
		return 1;
	}

	ret = extls_call_static_constructors();

	return (ret == EXTLS_SUCCESS) ? 0 : 1;

#else
	return 0;
#endif
}

/**
 * Function switched by SCTK_USE_OPTIMIZED_TLS value
 * @returns 1 if OPTIMIZED_TLS has been requested and is supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED()
{
#if MPC_USE_EXTLS
	return 1;

#else
	return 0;
#endif
}

/**
 * Exposed for Extls usage, returns the MPC-managed address where per-VP context address
 * will be stored.
 *
 * This address needs to be propagated among each swap and herited contexts.
 *
 * If this function is not defined, Extls fallbacks to a similar default mode
 * (__thread level storage duration)
 * @returns the address to the dedicated memory space.
 */
void *sctk_get_ctx_addr(void)
{
	return (void *)&sctk_extls_storage;
}

/**
 * initialize the head of registered destructors (C++).
 * This list contains pointers to object which have to be
 * handled by each task, independently.
 * @param[in,out] head the address pointing to the head to initialize
 */

void sctk_tls_dtors_init(struct sctk_tls_dtors_s **head)
{
	*head = NULL;
}

static mpc_common_spinlock_t dtors_lock = MPC_COMMON_SPINLOCK_INITIALIZER;


/**
 * register a new destructor to the list (C++).
 * This list contains pointers to object which have to be
 * handled by each task, independently. This should be called
 * each time a new task is started.
 * @param[in,out] head the list address
 * @param[in] obj the object owning the destructor.
 * @param[in] func the function call to destroy the object
 */
void sctk_tls_dtors_add(struct sctk_tls_dtors_s **head, void *obj, void (*func)(void *) )
{
	struct sctk_tls_dtors_s *elt = sctk_malloc(sizeof(struct sctk_tls_dtors_s) );

	elt->dtor = func;
	elt->obj  = obj;

	mpc_common_spinlock_lock(&dtors_lock);
	LL_PREPEND(*head, elt);
	mpc_common_spinlock_unlock(&dtors_lock);

}

/**
 * Free the list for the current task by calling all the destructors
 * for the registered objects.
 * @param[in,out] head the pointer to the list to free
 */
void sctk_tls_dtors_free(struct sctk_tls_dtors_s *head)
{
	struct sctk_tls_dtors_s *elt = NULL, *tmp = NULL;

	mpc_common_spinlock_lock(&dtors_lock);

	LL_FOREACH_SAFE(head, elt, tmp)
	{
		elt->dtor(elt->obj);
		LL_DELETE(head, elt);
		sctk_free(elt);
	}

	mpc_common_spinlock_unlock(&dtors_lock);
}

/**
 * Interface call to get the TLS block size for the current program.
 * This size contains the sum of all static TLS segment sizes discovered at
 * the beginning of the program
 * @returns the size as a size_t
 */
size_t sctk_extls_size()
{
	size_t size;

#ifdef MPC_USE_EXTLS
	size = extls_get_sz_static_tls_segments();
#else
	/* no extra space allocated for TLS when libextls is not present */
	size = getpagesize();
#endif
	return size;
}

static size_t page_size = 0;

/**
 * alter the given address to align the pointer to the beginning of the page.
 * @param[in] ptr the address to align
 * @returns the aligned address (can returns the same value as the input)
 */
void *sctk_align_ptr_to_page(void *ptr)
{
	if(!page_size)
	{
		page_size = getpagesize();
	}
	ptr = (char *)ptr - ( (size_t)ptr % page_size);
	return ptr;
}

/**
 * Alter the given size to make the value rounded up (value % page_size = 0).
 * @param[in] size the size to round up
 * @returns the rounded up size (can returns the same value than the input)
 */
size_t sctk_align_size_to_page(size_t size)
{
	if(!page_size)
	{
		page_size = getpagesize();
	}
	if(size % page_size == 0)
	{
		return size;
	}
	size += page_size - (size % page_size);
	return size;
}

/* Used by GCC to bypass TLS destructor calls */
int __cxa_thread_mpc_atexit(void (*dfunc)(void *), void *obj, __UNUSED__ void *dso_symbol)
{
	sctk_thread_data_t *th;

	th = mpc_thread_data_get();
	sctk_tls_dtors_add(&(th->dtors_head), obj, dfunc);
	return 0;
}
