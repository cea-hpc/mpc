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
/* #   - Adam Julien julien.adam.ocre@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include "sctk_alloc_spy.h"
#include "sctk_allocator.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_rdtsc.h"
#include "sctk_alloc_lock.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef SCTK_ALLOC_SPY

/************************* GLOBALS *************************/
#warning Move to a global struct to avoid getting many global variables
/** Global list for automatic deletion and flush at exit. **/
static struct sctk_alloc_spy_chain * sctk_alloc_global_spy_list = NULL;
/** Mutex to protect the global list for automatic deletion. **/
static SCTK_ALLOC_INIT_LOCK_TYPE sctk_alloc_global_spy_mutex = SCTK_ALLOC_INIT_LOCK_INITIALIZER;
/** Check if atexit as been setup or not. **/
static bool sctk_alloc_glob_spy_as_atexit = false;


/********************** PORTABILITY ************************/
#ifndef WIN32
	#define OPEN_FILE_PERMISSIONS O_TRUNC|O_WRONLY|O_CREAT,S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR
#else
	#define OPEN_FILE_PERMISSIONS O_TRUNC|O_WRONLY|O_CREAT
	#define PTHREAD_PROCESS_PRIVATE 0
#endif


/************************* FUNCTION ************************/
void sctk_alloc_spy_get_exename(char * buffer,unsigned int max_size)
{
	char tmp[256];
	int tmp2;
	int res;
	int i = 0;
	//error
	assert(buffer != NULL);
	assert(max_size <= 256);

	//open /proc/self/stat
	int fd = open("/proc/self/stat",O_RDONLY);
	assume(fd != -1,"Can't open file /proc/self/stat");

	//read in local buffer to avoid malloc call by fscanf
	res = read(fd,tmp,sizeof(tmp));
	tmp[res] = '\0';
	close(fd);

	//scan to extract chain
	res = sscanf(tmp,"%d (%s) ",&tmp2,buffer);
	assume(res == 2,"Error while reading /proc/self/stat, bad format");

	//???? why it let a ')' at then end ???????
	while( buffer[i] != '\0' )
	{
		if (buffer[i] == ')')
			buffer[i] = '\0';
		i++;
	}

	SCTK_PDEBUG("SPY : Filename : %s",buffer);
}

/************************* FUNCTION ************************/
/**
 * This is just to be replaced in unit tests by /dev/null to avoid writing readl files.
 * @param chain Define the allocation chain for which to open the file.
 * @param fname Define the output pointer on which to write the filename.
**/
void sctk_alloc_spy_chain_get_fname(struct sctk_alloc_chain * chain,char * fname)
{
	char dirname[256];
	char exename[128];
	sctk_alloc_spy_get_exename(exename,sizeof(exename));
	sprintf(dirname,"mpc-alloc-spy-%s-%06d",exename,getpid());
	sprintf(fname,"%s/alloc-chain-%p.raw",dirname,chain);
	
	#ifndef WIN32
		mkdir(dirname,S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IXUSR|S_IXGRP|S_IXOTH);
	#else
		_mkdir(dirname);
	#endif
}

/************************* FUNCTION ************************/
int sctk_alloc_spy_exename_filter_accept(void)
{
	char exename[128];
	char * filter = getenv("SCTK_ALLOC_SPY_EXENAME");

	//get exename
	#warning Avoid to call this another time
	sctk_alloc_spy_get_exename(exename,sizeof(exename));

	//trivial
	if (filter == NULL || filter[0] == '\0')
		return 1;
	else
		return strcmp(filter,exename) == 0;
}

/************************* FUNCTION ************************/
/**
 * Method used to init the spy member of given allocation chain.
 * @param chain Define the allcoation chain to init.
**/
void sctk_alloc_spy_chain_init(struct sctk_alloc_chain* chain)
{
	char fname[256];
	
	//check errors
	assert(chain != NULL);

	SCTK_PDEBUG("SPY : sctk_alloc_spy_chain_init");

	//if first to register
	if (!sctk_alloc_glob_spy_as_atexit)
	{
		sctk_alloc_glob_spy_as_atexit = true;
		atexit(sctk_alloc_spy_chain_at_exit);
	}

	//check if enabled
	if (sctk_alloc_spy_exename_filter_accept())
	{
		//calc filename
		sctk_alloc_spy_chain_get_fname(chain,fname);
		SCTK_PDEBUG("Init spy file : %s for chain %p",fname,chain);

		//open file
		chain->spy.fd = open(fname,OPEN_FILE_PERMISSIONS);
		if (chain->spy.fd == -1)
		{
			perror(fname);
			abort();
		}
	} else {
		chain->spy.fd = -1;
	}

	//setup buffer
	memset(chain->spy.buffer,0,sizeof(chain->spy.buffer));
	chain->spy.buf_entries = 0;

	//setup other fields
	chain->spy.next = NULL;
	chain->spy.chain = chain;
	chain->spy.next_is_realloc = NULL;

	//setup lock
	sctk_alloc_spinlock_init(&chain->spy.lock,PTHREAD_PROCESS_PRIVATE);

	//register in auto-deletion list
	sctk_alloc_spy_chain_register(&chain->spy);

	//call first event
	sctk_alloc_spy_emit_event_init(chain);
}

/************************* FUNCTION ************************/
/**
 * This is a way to reset spy chain methods for testing purpose. It reset the global list.
**/
void __sctk_alloc_spy_chain_reset(void)
{
	//lock mutex
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_global_spy_mutex);
	sctk_alloc_global_spy_list = NULL;
	//lock mutex
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_global_spy_mutex);
}

/************************* FUNCTION ************************/
/**
 * Flush the buffer of the given allocation spy.
 * @param spy Define the spy structure to flush.
**/
void sctk_alloc_spy_chain_flush(struct sctk_alloc_spy_chain * spy)
{
	//error
	assert(spy != NULL);
	assert(spy->buf_entries <= SCTK_ALLOC_SPY_BUFFER);

	//nothing to do
	if (spy->buf_entries == 0)
		return;

	//error on FD, this may be due to filter as it only disable file opening
	if (spy->fd == -1)
	{
		//warning("Spy try to flush on closed file.");
		return;
	}

	//flush to file
	const int size = spy->buf_entries * sizeof(struct sctk_alloc_spy_event);
	const int wsize = write(spy->fd,spy->buffer,size);
	if (wsize != size)
	{
		perror("Flushing spy buffer.");
		abort();
	}

	//reset spy buffer
	spy->buf_entries = 0;
}

/************************* FUNCTION ************************/
/**
 * Remove the spyer of given allocation chain. It will flush the buffer and close the file.
 * @param chain Define the chain for which to close the spyer.
**/
void sctk_alloc_spy_chain_destroy(struct sctk_alloc_chain* chain)
{
	//error
	assert(chain != NULL);

	//remove from list
	sctk_alloc_spy_chain_unregister(&chain->spy);

	//emit destroy event
	sctk_alloc_spy_emit_event_destroy(chain);

	//debug
	SCTK_PDEBUG("Closing spy file for chain %p",chain);

	//flush if needed
	sctk_alloc_spy_chain_flush(&chain->spy);

	//closing file
	if (chain->spy.fd != 0)
	{
		close(chain->spy.fd);
		chain->spy.fd = 0;
	}
}

/************************* FUNCTION ************************/
/**
 * Setup common properties of an event.
 * @param type Define the type of event to setup.
 * @return Return the event after init.
**/
void sctk_alloc_spy_chain_setup_event(struct sctk_alloc_spy_event * event,enum sctk_alloc_spy_event_type type)
{
	//setup default values
	sctk_alloc_fill_rdtsc(&event->rdtsc);

	//setup type
	event->type = type;
}

/************************* FUNCTION ************************/
/**
 * Push an event into the log system.
 * @param spy Define the spy chain on which to push the event.
 * @param event Define the event to push.
**/
void sctk_alloc_spy_chain_push_event(struct sctk_alloc_spy_chain * spy,struct sctk_alloc_spy_event * event)
{
	assert(spy != NULL);
	assert(event != NULL);
	sctk_alloc_spinlock_lock(&spy->lock);
	struct sctk_alloc_spy_event * target = sctk_alloc_spy_chain_get_event(spy);
	*target = *event;
	sctk_alloc_spinlock_unlock(&spy->lock);
}

/************************* FUNCTION ************************/
/**
 * Allocate an event in the buffer an return it. It the buffer get full, the function will flush
 * it before returning.
 * @param spy Define the spy strut in which to allocate the buffer.
**/
struct sctk_alloc_spy_event * sctk_alloc_spy_chain_get_event(struct sctk_alloc_spy_chain * spy)
{
	//error
	assert(spy != NULL);
	
	//check if need to flush
	if (spy->buf_entries >= SCTK_ALLOC_SPY_BUFFER)
		sctk_alloc_spy_chain_flush(spy);

	//get current entry
	struct sctk_alloc_spy_event * event = spy->buffer + spy->buf_entries;
	//only increment if enabled for current process
	if (spy->fd != -1)
		spy->buf_entries ++;
	assert(spy->buf_entries <= SCTK_ALLOC_SPY_BUFFER);
	
	return event;
}

/************************* FUNCTION ************************/
/**
 * Emit INIT event on the given allocation chain.
 * No need of lock as it's sequential by definition, a chain was init or destroy one time only.
 * @param chain Define the allocation chain for which to emit the event.
**/
void sctk_alloc_spy_emit_event_init(struct sctk_alloc_chain * chain)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_INIT_CHAIN);

	//fill it
	gettimeofday(&event.infos.evt_init.timestamp,NULL);

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}


/************************* FUNCTION ************************/
/**
 * Emit DESTROY event on the given allocation chain.
 * No need of lock as it's sequential by definition, a chain was init or destroy one time only.
 * @param chain Define the allocation chain for which to emit the event.
**/
void sctk_alloc_spy_emit_event_destroy(struct sctk_alloc_chain * chain)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_DEL_CHAIN);

	//fill it
	gettimeofday(&event.infos.evt_init.timestamp,NULL);

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit ADD_MACRO_BLOC signal when a macro bloc is requested by allocation chain to the memory source.
 * No need of lock as we got shared and protected chain or TLS chain so get only sequential access.
 * @param chain Define the allocation chain for which to emit the event.
 * @param bloc Define the related macro bloc.
**/
void sctk_alloc_spy_emit_event_add_macro_bloc(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_ADD_MACRO_BLOC);

	//fill it
	event.infos.evt_add_macro_bloc.bloc = bloc;
	if (bloc != NULL)
		event.infos.evt_add_macro_bloc.size = bloc->header.size;
	else
		event.infos.evt_add_macro_bloc.size = 0;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit FREE_MACRO_BLOC signal when a macro bloc is freed by allocation chain to the memory source.
 * No need of lock as we got shared and protected chain or TLS chain so get only sequential access.
 * @param chain Define the allocation chain for which to emit the event.
 * @param bloc Define the related macro bloc.
**/
void sctk_alloc_spy_emit_event_free_macro_bloc(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_FREE_MACRO_BLOC);

	//fill it
	event.infos.evt_add_macro_bloc.bloc = bloc;
	event.infos.evt_add_macro_bloc.size = bloc->header.size;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit REMOTE_FREE signal when a macro bloc is freed by allocation chain to the memory source.
 * CAUTION, it take place in a non protected region so we need to lock the chain if it was shared.
 * @param local_chain Define the local allocation chain for which to emit the event.
 * @param remote_chain Define the remote chain in whitch to register the segment.
 * @param ptr Define the pointer to free.
**/
void sctk_alloc_spy_emit_event_remote_free(struct sctk_alloc_chain * local_chain,struct sctk_alloc_chain * remote_chain,void * ptr)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_REG_REMOTE_FREE);

	//fill it
	event.infos.evt_remote_free.ptr = ptr;
	event.infos.evt_remote_free.remote_chain = remote_chain;

	//push it
	sctk_alloc_spy_chain_push_event(&local_chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit FLUSH_RFQ signal the current allocation chain start to flush his RFQ and get a non empty
 * list.
 * RFQ flush is used only on non shared chain, so it's thread safe by definition, no need of lock.
 * @param chain Define the local allocation chain for which to emit the event.
 * @param nb_entries Define the number of entries in the list.
**/
void sctk_alloc_spy_emit_event_flush_rfq(struct sctk_alloc_chain * chain,int nb_entries)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_FLUSH_RFQ);

	//fill it
	event.infos.evt_flush_rfq.nb_entries = nb_entries;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit FLUSH_RFQ_ENTRY signal when the current allocation chain flush an EFQ entry. It will also
 * imply a futur emition of free as it call the free method of allocation chain.
 * RFQ flush is used only on non shared chain, so it's thread safe by definition, no need of lock.
 * @param chain Define the local allocation chain for which to emit the event.
 * @param ptr Define the pointer to free.
**/
void sctk_alloc_spy_emit_event_flush_rfq_entry(struct sctk_alloc_chain * chain,void * ptr)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_FLUSH_RFQ_ENTRY);

	//fill it
	event.infos.evt_flush_rfq_entry.ptr = ptr;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_ALLOC signal when the current allocation chain get a standard bloc request (not huge onces).
 * As alloc method is thread safe we didn't need to lock the spy struct.
 * @param chain Define the local allocatio nchain for which to emit the event.
 * @param request Define the requested size.
 * @param ptr Define the body address returned by allocator.
 * @param size Define the real allocated size (with bloc header)
 * @param boundary Define the requested boundary to fulfill (this is for memalign profiling).
**/
#warning Declare sctk_size_t instead of unsigned long
void sctk_alloc_spy_emit_event_chain_alloc(struct sctk_alloc_chain * chain,unsigned long request,void * ptr,unsigned long size,unsigned long boundary)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_ALLOC);

	//fill it
	event.infos.evt_chain_alloc.ptr = ptr;
	event.infos.evt_chain_alloc.request = request;
	event.infos.evt_chain_alloc.size = size;
	event.infos.evt_chain_alloc.realloc_orig = chain->spy.next_is_realloc;
	event.infos.evt_chain_alloc.boundary = boundary;

	//unset global alloc
	chain->spy.next_is_realloc = NULL;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_FREE signal when the current allocation chain get a standard bloc free (not huge onces).
 * As free method is thread safe we didn't need to lock the spy struct.
 * @param chain Define the local allocatio nchain for which to emit the event.
 * @param ptr Define the body address of the bloc to free.
 * @param size Define the real allocated size (with bloc header)
**/
#warning Declare sctk_size_t instead of unsigned long
void sctk_alloc_spy_emit_event_chain_free(struct sctk_alloc_chain * chain,void * ptr,unsigned long size)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_FREE);

	//fill it
	event.infos.evt_chain_free.ptr = ptr;
	event.infos.evt_chain_free.size = size;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_HUGE_ALLOC signal when the current allocation chain get a huge bloc request.
 * CAUTION, their is no mutex to protect huge allocation so if chain is shared, we must lock.
 * @param chain Define the local allocatio nchain for which to emit the event.
 * @param request Define the requested size.
 * @param ptr Define the body address returned by allocator.
 * @param size Define the real allocated size (with bloc header)
 * @param boundary Define the requested boundary to fulfill (this is for memalign profiling).
**/
#warning Declare sctk_size_t instead of unsigned long
void sctk_alloc_spy_emit_event_chain_huge_alloc(struct sctk_alloc_chain * chain,unsigned long request,void * ptr,unsigned long size,unsigned long boundary)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_HUGE_ALLOC);

	//fill it
	event.infos.evt_chain_alloc.ptr = ptr;
	event.infos.evt_chain_alloc.request = request;
	event.infos.evt_chain_alloc.size = size;
	event.infos.evt_chain_alloc.realloc_orig = chain->spy.next_is_realloc;
	event.infos.evt_chain_alloc.boundary = boundary;

	//unset global alloc
	chain->spy.next_is_realloc = NULL;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_HUGE_FREE signal when the current allocation chain get a huge bloc free.
 * CAUTION, their is no mutex to protect huge allocation so if chain is shared, we must lock.
 * @param chain Define the local allocatio nchain for which to emit the event.
 * @param ptr Define the body address of the bloc to free.
 * @param size Define the real allocated size (with bloc header)
**/
#warning Declare sctk_size_t instead of unsigned long
void sctk_alloc_spy_emit_event_chain_huge_free(struct sctk_alloc_chain * chain,void * ptr,unsigned long size)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_HUGE_FREE);

	//fill it
	event.infos.evt_chain_free.ptr = ptr;
	event.infos.evt_chain_free.size = size;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_SPLIT signal when the current allocation chain finish a split operation for allocation.
 * This is thread safe as allocation chain was already thread protected (or used by a single thread.)
 * @param chain Define the allocation which done the split.
 * @param base_addr Define the base address of splitted segement.
 * @param chunk_size Define the size of first chunk (to fit the allocation request). Full size with headers.
 * @param residut_size Define the size of second chunk (the residut). Full size with headers.
**/
void sctk_alloc_spy_emit_event_chain_split(struct sctk_alloc_chain * chain,void * base_addr,unsigned long chunk_size,unsigned long residut_size)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_SPLIT);

	//fill it
	event.infos.evt_chain_split.base_addr = base_addr;
	event.infos.evt_chain_split.chunk_size = chunk_size;
	event.infos.evt_chain_split.residut_size = residut_size;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Emit CHAIN_MERGE signal when current allocation chain finish to merge some free chunks.
 * This is thread safe as allocation chain was already thread protected (or used by a single thread.)
 * @param chain Define the allocation which done the split.
 * @param base_addr Define the base address of the merged segement.
 * @param size Define the full size of merged segment (with header).
**/
void sctk_alloc_spy_emit_event_chain_merge(struct sctk_alloc_chain * chain, void * base_addr,unsigned long size)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_CHAIN_MERGE);

	//fill it
	event.infos.evt_chain_merge.base_addr = base_addr;
	event.infos.evt_chain_merge.size = size;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Update the global TLS which permit to know that we are in a local alloc or realloc
 * @param chain Define the allocation chain which will done the malloc.
 * @param orig_ptr The original pointer to free.
 * @param new_requested_size Define the new size requested by user.
**/
void sctk_alloc_spy_emit_event_next_is_realloc(struct sctk_alloc_chain * chain, void * orig_ptr,sctk_size_t new_requested_size)
{
	//get entry
	struct sctk_alloc_spy_event event;
	sctk_alloc_spy_chain_setup_event(&event,SCTK_ALLOC_SPY_EVENT_NEXT_IS_REALLOC);

	//fill it
	event.infos.evt_realloc.orig_ptr = orig_ptr;
	event.infos.evt_realloc.new_requested_size = new_requested_size;

	//setup global info for next event
	chain->spy.next_is_realloc = orig_ptr;

	//push it
	sctk_alloc_spy_chain_push_event(&chain->spy,&event);
}

/************************* FUNCTION ************************/
/**
 * Register the spy chain in global list for automatic deletion at exit.
 * @param spy Define the spy chain to register.
**/
void sctk_alloc_spy_chain_register(struct sctk_alloc_spy_chain * spy)
{
	//error
	assert(spy != NULL);

	//lock mutex
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_global_spy_mutex);

	//register
	spy->next = sctk_alloc_global_spy_list;
	sctk_alloc_global_spy_list = spy;

	//unlock mutex
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_global_spy_mutex);
}

/************************* FUNCTION ************************/
/**
 * Unregister the given spy chain from global list for automatic delection at exit.
 * @param spy Define the spy chain to remove.
**/
void sctk_alloc_spy_chain_unregister(struct sctk_alloc_spy_chain * spy)
{
	//error
	assert(spy != NULL);

	//lock mutex
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_global_spy_mutex);

	//search position in list
	struct sctk_alloc_spy_chain * cur = sctk_alloc_global_spy_list;
	struct sctk_alloc_spy_chain * prev = NULL;
	while (cur != spy && cur != NULL)
	{
		prev = cur;
		cur = cur->next;
	}

	//error
	assume(cur == spy,"Can't find the given spy in global list.");

	//remove
	if (prev == NULL)
		sctk_alloc_global_spy_list = spy->next;
	else
		prev->next = spy->next;

	//unlock mutex
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_global_spy_mutex);
}

/************************* FUNCTION ************************/
/**
 * At exit, automatically flush and delete all spy structs.
**/
void sctk_alloc_spy_chain_at_exit(void )
{
	while (sctk_alloc_global_spy_list != NULL)
		sctk_alloc_spy_chain_destroy(sctk_alloc_global_spy_list->chain);
}

#endif //SCTK_ALLOC_SPY
