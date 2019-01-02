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
#include "sctk_config.h"
#include "sctk_alloc.h"
#include "sctk_topology.h"
#include "sctk_accessor.h"
#include "sctk_spinlock.h"
#include "sctk_atomics.h"
#include "hwloc.h"

#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utlist.h>
#include <dlfcn.h>

#if defined(SCTK_USE_TLS)
/** storage slot used by libextls to store its own data.
 * The address of this TLS is provided by extls_get_context_storage_addr()
 */
__thread void* sctk_extls_storage = NULL;
__thread int __mpc_task_rank = -2;
#endif

/**
 * Represent a dynamic symbol for the dynamic initializer interface.
 * Built as a chained list.
 */
struct dyn_sym_s
{
	char * name;              /**< symbol's name */
	void * addr;              /**< current mapped address */
	struct dyn_sym_s * next;  /**< next symbol in the list (if any) */
};

/**
 * Represents a dynamic shared object (like library object) mapped in /proc/self/maps.
 * built and used as a chained list.
 */
struct dsos_s
{
	char * name;          /**< the DSO's name */
	struct dsos_s * next; /**< the next discovered DSO in the list (if any) */
};

/** page size for the current execution (filled with getpagesize() call) */
static size_t page_size = 0;
/** head of dynamic symbol list having a initializer to be called */
static struct dyn_sym_s * __sctk_dyn_symbols = NULL;
/** head of discovered DSOs from /proc/self/maps reading */
static struct dsos_s * __sctk_dsos = NULL;
/** DSO handler pointing to the current binary */
static void * __sctk_tls_locate_tls_dyn_initializer_handle = NULL;
/** Avoiding concurrency over dyn_init lookup */
sctk_spinlock_t __sctk_tls_locate_tls_dyn_lock = 0;

#if defined (SCTK_USE_OPTIMIZED_TLS)
/**
 * Function switched by SCTK_USE_OPTIMIZED_TLS value
 * @returns 1 if OPTIMIZED_TLS has been requested and is supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 1;
}
#else
/**
 * Function switched by SCTK_USE_OPTIMIZED_TLS value
 * @returns 1 if OPTIMIZED_TLS has been requested and is supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif

/** 
 * This function is called to discover if a given variable comes with a dynamic initializer.
 * If found, the callback is called.
 * 
 * @param[in] fname the functino name in string format
 */
void sctk_tls_locate_tls_dyn_initializer( char * fname )
{
	sctk_spinlock_lock( &__sctk_tls_locate_tls_dyn_lock );
	
	if( !__sctk_tls_locate_tls_dyn_initializer_handle )
		 __sctk_tls_locate_tls_dyn_initializer_handle = dlopen( NULL, RTLD_LAZY );

	dlerror();    /* Clear any existing error */

	void * ret = dlsym( __sctk_tls_locate_tls_dyn_initializer_handle, fname );

	char * error = NULL;
	
	if ((error = dlerror()) != NULL)  {
		//fprintf(stderr, "%s\n", error);
		sctk_spinlock_unlock( &__sctk_tls_locate_tls_dyn_lock );
		return;
	}

	sctk_spinlock_unlock( &__sctk_tls_locate_tls_dyn_lock );
	
	/* If we found a dynamic initializer call it */
	if( ret )
	{
          sctk_info("Calling Dyn initalizer for %s ret %p", fname, ret);
          void (*fn)() = (void (*)())ret;
          (fn)();
        }
}

/**
 * Read /proc/self/maps and populate the list with all discovered DSOs.
 * This allow us to know in which DSOs the initializers must be located.
 *
 * @returns 1 if any element list failed to be allocated, 0 otherwise
 */
int sctk_load_proc_self_maps()
{
	char * maps = calloc( 4 * 1024 * 1024, 1 );
	
	if( !maps )
	{
		return 1;
	}
	
	FILE * self = fopen("/proc/self/maps" ,"r");
	
	fread(maps, 1, 4 * 1024 * 1024 , self);
	
	//printf("READ %d\n", ret );
	
	char * line = maps;
	char * next_line;
	
	int cont = 1;
	
	unsigned long begin, end, inode;
	char skip[50], perm[5], dev[50], dsoname[500];
	
	do
	{
		next_line = strstr( line, "\n" );
		
		if( next_line )
		{
			*next_line = '\0';
			next_line++;
		}
		else
		{
			cont = 0;
		}

                if (!strstr(line, "/")) {
                  line = next_line;
                  continue;
                }

                sscanf(line, "%lx-%lx %4s %49s %49s %ld %499s", &begin, &end,
                       perm, skip, dev, &inode, dsoname);

                if ((perm[2] == 'x') && !(dsoname[0] == '[')) {

                  // fprintf(stderr,"==>%s (%s)\n", dsoname , perm);

                  struct dsos_s *new = sctk_malloc(sizeof(struct dsos_s));

                  if (!new) {
                    return 1;
                  }

                  new->name = strdup(dsoname);
                  new->next = __sctk_dsos;
                  __sctk_dsos = new;
                }

                line = next_line;

        } while (cont);

        fclose(self);

        free(maps);

        return 0;
}


/**
 * Look for all wrapper generated by patched GCC in the given DSO
 * @param[in] dso the dynamic object to look up
 * @param[in] handle the associated handler returned by dlopen()
 * @returns 1 if any element list failed to be allocated, 0 otherwise
 */
int sctk_load_wrapper_symbols( char * dso , void * handle )
{
	char command[1000];

	/* because DMTCP waste too much time to track these commands */
	char * ckpt_wrapper = (sctk_checkpoint_mode) ? "dmtcp_nocheckpoint" : "";
	snprintf(command, 1000, "%s nm  %s 2>&1 | %s grep \"___mpc_TLS_w\\|_ZTW\"", ckpt_wrapper, dso, ckpt_wrapper);
	
	
	FILE * wrapper_symbols = popen(command, "r");
	
	while(!feof(wrapper_symbols))
	{
		char buff[500];
		
		if(fgets(buff, 500, wrapper_symbols) == 0)
		{
			break;
		}
		
		//printf("%s\n", buff );
	
		char * wrapp = strstr( buff , "___mpc_TLS_w" );
		
		if( wrapp )
		{
			char *retl = strstr( buff, "\n");
			
			if( retl )
			{
				*retl = '\0';
				
				while( retl != wrapp )
				{
					if( *(retl - 1) == ' ')
					{
						retl--;
						*retl = '\0';
					}
					else
					{
						break;
					}
				}
				
			}
			
			
			struct dyn_sym_s * new = sctk_malloc( sizeof(struct dyn_sym_s) );
			
			if( !new )
			{
				return 1;
			}
			
			new->name = strdup( wrapp );
			new->addr = dlsym(handle, wrapp );
			new->next = __sctk_dyn_symbols;
			__sctk_dyn_symbols = new;
			
			
			sctk_debug("Located dyn initializer : %s", wrapp );
		}
	}
	
	pclose( wrapper_symbols );
	
	return 0;
}

/**
 * Locate all dynamic initializers generated by patched GCC.
 * This function does not call them. This function should be called as early
 * as possible by MPC (probably in sctk_launch.c), before any MPI task
 *
 * @returns 1 if something's gone wrong, 0 otherwise
 */
int sctk_locate_dynamic_initializers()
{
	if( sctk_load_proc_self_maps() )
	{
		fprintf(stderr, "Failled to load /proc/self/maps");
		return 1;
	}
	
	struct dsos_s * current = __sctk_dsos;
	
	void * lib_handle = dlopen( NULL, RTLD_LAZY );
	
	while( current )
	{
		//printf("CHECKING %s\n", current->name );
		if( sctk_load_wrapper_symbols(current->name, lib_handle) )
		{
			return 1;
		}
	
		current = current->next;
	}
	
	dlclose( lib_handle );
	
	return 0;
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
	struct dyn_sym_s * current = __sctk_dyn_symbols;
	
	while( current )
	{
		if( current->addr )
		{
			sctk_debug("TLS : Calling dynamic init %s", current->name);
			((void (*)())current->addr)();
		}
		
		current = current->next;
	}
	
	return 0;
}

#if defined(SCTK_USE_TLS) && defined(Linux_SYS)

/**
 * Function switched by SCTK_USE_OPTIMIZED_TLS value
 * @returns 1 if OPTIMIZED_TLS has been requested and is supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 1;
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
inline void* extls_get_context_storage_addr(void)
{
	return (void*)&sctk_extls_storage;
}

/** Enabling Copy-on-write TLS */
#define SCTK_COW_TLS

#if defined(SCTK_COW_TLS)
/**
 * Function switched depending on SCTK_COW_TLS macro state.
 * @returns 1 if Copy-on-write support is enabled and supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 1;
}
#else
/**
 * Function switched depending on SCTK_COW_TLS macro state.
 * @returns 1 if Copy-on-write support is enabled and supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
#endif

/**
 * initialize the head of registered destructors (C++).
 * This list contains pointers to object which have to be
 * handled by each task, independently.
 * @param[in,out] head the address pointing to the head to initialize
 */
void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head)
{
	*head = NULL;
}

/**
 * register a new destructor to the list (C++).
 * This list contains pointers to object which have to be
 * handled by each task, independently. This should be called
 * each time a new task is started.
 * @param[in,out] head the list address
 * @param[in] obj the object owning the destructor.
 * @param[in] func the function call to destroy the object
 */
void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *))
{
	struct sctk_tls_dtors_s * elt = sctk_malloc(sizeof(struct sctk_tls_dtors_s));
	elt->dtor = func;
	elt->obj = obj;
	
	LL_PREPEND(*head, elt);
}

/**
 * Free the list for the current task by calling all the destructors
 * for the registered objects.
 * @param[in,out] head the pointer to the list to free
 */
void sctk_tls_dtors_free(struct sctk_tls_dtors_s ** head)
{
	struct sctk_tls_dtors_s* elt = NULL, *tmp = NULL;

	
	LL_FOREACH_SAFE(*head, elt, tmp)
	{
		elt->dtor(elt->obj);
		LL_DELETE(*head, elt);
		sctk_free(elt);
	}
}

#ifdef MPC_DISABLE_HLS
/**
 * Determine if the program enables and supports HLS mode.
 * Controled through MPC_DISABLE_HLS macro state.
 * @returns 1 if HLS are enabled, 0 otherwise
 */
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
#else
/**
 * Determine if the program enables and supports HLS mode.
 * Controled through MPC_DISABLE_HLS macro state.
 * @returns 1 if HLS are enabled, 0 otherwise
 */
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 1;
}
#endif

#else
#warning "Experimental Ex-TLS support desactivated"

/**
 * Determine if the program enables and supports HLS mode.
 * Controled through MPC_DISABLE_HLS macro state.
 * @returns 1 if HLS are enabled, 0 otherwise
 */
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
/**
 * Function switched depending on SCTK_COW_TLS macro state.
 * @returns 1 if Copy-on-write support is enabled and supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
/**
 * Function switched by SCTK_USE_OPTIMIZED_TLS value
 * @returns 1 if OPTIMIZED_TLS has been requested and is supported, 0 otherwise.
 */
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif /* SCTK_USE_TLS */

size_t extls_get_sz_static_tls_segments();

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
void* sctk_align_ptr_to_page(void* ptr){
  if(!page_size)
	  page_size = getpagesize ();
  ptr = (char*)ptr - ((size_t) ptr % page_size);
  return ptr;
}



/**
 * Alter the given size to make the value rounded up (value % page_size = 0).
 * @param[in] size the size to round up
 * @returns the rounded up size (can returns the same value than the input)
 */
size_t sctk_align_size_to_page(size_t size){
  if(!page_size)
	  page_size = getpagesize ();
  if(size % page_size == 0){
    return size;
  }
  size += page_size - (size % page_size);
  return size;
}

