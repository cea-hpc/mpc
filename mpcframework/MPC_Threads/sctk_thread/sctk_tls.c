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
/* #                                                                      # */
/* ######################################################################## */


#define _GNU_SOURCE
#include "sctk_config.h"
#include "sctk_alloc.h"
#include "sctk_topology.h"
#include "sctk_accessor.h"

#include "sctk_spinlock.h"
#include "sctk_atomics.h"
#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hwloc.h"
#include <utlist.h>
#include <dlfcn.h>

#if defined(SCTK_USE_TLS)
__thread void* sctk_extls_storage = NULL;

struct dyn_sym_s
{
	char * name;
    void * addr;
	struct dyn_sym_s * next;
};


struct dsos_s
{
	char * name;
	struct dsos_s * next;
};

static size_t page_size = 0;
static struct dyn_sym_s * __sctk_dyn_symbols = NULL;
static struct dsos_s * __sctk_dsos = NULL;
static void * __sctk_tls_locate_tls_dyn_initializer_handle = NULL;
sctk_spinlock_t __sctk_tls_locate_tls_dyn_lock = 0;

#if defined (SCTK_USE_OPTIMIZED_TLS)
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 1;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif

#endif

/* This function is called to discover if a given
 * variable comes with a dynamic initializer */
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
		sctk_info("Locating Dyn initalizer for %s ret %p\n", fname, ret );
		void (*fn)() = (void (*)())ret;
		(fn)();
	}
}

int sctk_load_proc_self_maps()
{
	char * maps = calloc( 4 * 1024 * 1024, 1 );
	
	if( !maps )
	{
		return 1;
	}
	
	FILE * self = fopen("/proc/self/maps" ,"r");
	
	size_t ret = fread(maps, 1, 4 * 1024 * 1024 , self);
	
	//printf("READ %d\n", ret );
	
	char * line = maps;
	char * next_line;
	
	int cont = 1;
	
	unsigned long begin, end, size, inode;
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
		
		//printf("%s\n", line );
		
		sscanf(line, "%lx-%lx %4s %s %s %ld %s", &begin, &end, perm,
		skip, dev, &inode, dsoname);
		
		if( (perm[2] == 'x') && !(dsoname[0] == '[') )
		{
			
			//fprintf(stderr,"==>%s (%s)\n", dsoname , perm);
		
			struct dsos_s * new = sctk_malloc( sizeof( struct dsos_s ) );
			
			if( !new )
			{
				return 1;
			}
			
			new->name = strdup( dsoname );
			new->next = __sctk_dsos;
			__sctk_dsos = new;
		
		}
		
		line = next_line;
		
		
	}while( cont );
	
	fclose( self );
	
	free( maps );

	return 0;
}


int sctk_load_wrapper_symbols( char * dso , void * handle )
{
	char command[1000];

	snprintf(command, 1000, "nm  %s 2>&1 | grep \"___mpc_TLS_w\\|_ZTW\"", dso );
	
	
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


inline void* extls_get_context_storage_addr(void)
{
	return (void*)&sctk_extls_storage;
}

#define SCTK_COW_TLS

#if defined(SCTK_COW_TLS)
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 1;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
#endif

void* sctk_align_ptr_to_page(void* ptr){
  if(!page_size)
	  page_size = getpagesize ();
  ptr = (char*)ptr - ((size_t) ptr % page_size);
  return ptr;
}

size_t sctk_align_size_to_page(size_t size){
  if(!page_size)
	  page_size = getpagesize ();
  if(size % page_size == 0){
    return size;
  }
  size += page_size - (size % page_size);
  return size;
}

size_t sctk_extls_size()
{
  size_t size;
  size = extls_get_sz_static_tls_segments();
  return size;
}

void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head)
{
	*head = NULL;
}

void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *))
{
	struct sctk_tls_dtors_s * elt = sctk_malloc(sizeof(struct sctk_tls_dtors_s));
	elt->dtor = func;
	elt->obj = obj;
	
	LL_PREPEND(*head, elt);
}

void sctk_tls_dtors_free(struct sctk_tls_dtors_s ** head)
{
	struct sctk_tls_dtors_s* elt = NULL, *tmp = NULL;
	int count = 0;
	
	LL_FOREACH_SAFE(*head, elt, tmp)
	{
		elt->dtor(elt->obj);
		LL_DELETE(*head, elt);
		sctk_free(elt);
	}
}

#ifdef MPC_DISABLE_HLS
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 1;
}
#endif

#else
#warning "Experimental Ex-TLS support desactivated"

int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif
