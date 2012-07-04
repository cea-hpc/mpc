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

#ifndef SCTK_ALLOC_SPY_H
#define SCTK_ALLOC_SPY_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#ifndef _WIN32
	#include <sys/time.h>
#elif defined(_MSC_VER)
//#include <WinSock.h>
#endif
#include "sctk_alloc_lock.h"

/************************** CONSTS *************************/

/************************** MACROS *************************/
/** Define the size of spying buffer for writing. **/
#define SCTK_ALLOC_SPY_BUFFER 256
/**
 * Define the hook macro to wrap all call and definition for allocator spying. It permit to identify
 * and disable them quicly and simply.
**/
#ifdef SCTK_ALLOC_SPY
#define SCTK_ALLOC_SPY_HOOK(x) x
#define SCTK_ALLOC_SPY_COND_HOOK(x,y) if (x) y
#else //SCTK_ALLOC_STAT
#define SCTK_ALLOC_SPY_HOOK(x) /*x*/
#define SCTK_ALLOC_SPY_COND_HOOK(x,y) /*if (x) y*/
#endif //SCTK_ALLOC_STAT

/*************************** ENUM **************************/
/** List of supported event type. **/
enum sctk_alloc_spy_event_type
{
	/** This is mode to signal error when reading the trace, this may not be used readly to write in files. **/
	SCTK_ALLOC_SPY_EVENT_NULL,
	/** Initialization of a new allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_INIT_CHAIN,
	/** Destroy an allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_DEL_CHAIN,
	/** Insert a macro bloc in allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_ADD_MACRO_BLOC,
	/** Remove a macro bloc from allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_FREE_MACRO_BLOC,
	/** Emit when the current chain try to free a distant bloc (to register in RFQ). **/
	SCTK_ALLOC_SPY_EVENT_REG_REMOTE_FREE,
	/** Emit when the current chain try to flush is RFQ (and get content, not called if empty). **/
	SCTK_ALLOC_SPY_EVENT_FLUSH_RFQ,
	/** Emit when an entroy of current RFQ is flushed (it whill imply emission of FREE also). **/
	SCTK_ALLOC_SPY_EVENT_FLUSH_RFQ_ENTRY,
	/** Emit when a spandard allocation was performed by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_ALLOC,
	/** Emit when a standard free was performaned by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_FREE,
	/** Emit when a huge allocation was performed by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_HUGE_ALLOC,
	/** Emit when a huge free was performaned by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_HUGE_FREE,
	/** Emit when a split operation was performaned by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_SPLIT,
	/** Emit when a merge operation was performed by allocation chain. **/
	SCTK_ALLOC_SPY_EVENT_CHAIN_MERGE,
	/** Emit on realloc call before internal malloc call. **/
	SCTK_ALLOC_SPY_EVENT_NEXT_IS_REALLOC,
	//SCTK_ALLOC_SPY_EVENT_MALLOC,
	//SCTK_ALLOC_SPY_EVENT_CALLOC,
	//SCTK_ALLOC_SPY_EVENT_REALLOC,
	//SCTK_ALLOC_SPY_EVENT_FREE,
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_init
{
	struct timeval timestamp;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_add_macro_bloc
{
	struct sctk_alloc_macro_bloc * bloc;
	unsigned long size;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_remote_free
{
	void * ptr;
	struct sctk_alloc_chain * remote_chain;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_flush_rfq
{
	int nb_entries;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_flush_rfq_entry
{
	void * ptr;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_realloc
{
	void * orig_ptr;
	unsigned long new_requested_size;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_chain_alloc
{
	unsigned long request;
	void * ptr;
	unsigned long size;
	void * realloc_orig;
	unsigned long boundary;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_chain_free
{
	void * ptr;
	unsigned long size;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_chain_split
{
	void * base_addr;
	unsigned long chunk_size;
	unsigned long residut_size;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event_chain_merge
{
	void * base_addr;
	unsigned long size;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_event
{
	enum sctk_alloc_spy_event_type type;
	unsigned long rdtsc;
	union {
		struct sctk_alloc_spy_event_init evt_init;
		struct sctk_alloc_spy_event_init evt_destroy;
		struct sctk_alloc_spy_event_add_macro_bloc evt_add_macro_bloc;
		struct sctk_alloc_spy_event_remote_free evt_remote_free;
		struct sctk_alloc_spy_event_flush_rfq evt_flush_rfq;
		struct sctk_alloc_spy_event_flush_rfq_entry evt_flush_rfq_entry;
		struct sctk_alloc_spy_event_chain_alloc evt_chain_alloc;
		struct sctk_alloc_spy_event_chain_free evt_chain_free;
		struct sctk_alloc_spy_event_chain_split evt_chain_split;
		struct sctk_alloc_spy_event_chain_merge evt_chain_merge;
		struct sctk_alloc_spy_event_realloc evt_realloc;
	} infos;
};

/************************** STRUCT *************************/
struct sctk_alloc_spy_chain
{
	int fd;
	int buf_entries;
	struct sctk_alloc_spy_event buffer[SCTK_ALLOC_SPY_BUFFER];
	struct sctk_alloc_spy_chain * next;
	struct sctk_alloc_chain * chain;
	sctk_alloc_spinlock_t lock;
	unsigned char next_is_realloc;
};

/************************* FUNCTION ************************/
void __sctk_alloc_spy_chain_reset(void);
void sctk_alloc_spy_chain_init(struct sctk_alloc_chain * chain);
void sctk_alloc_spy_chain_destroy(struct sctk_alloc_chain * chain);
void sctk_alloc_spy_chain_flush(struct sctk_alloc_spy_chain * spy);
void sctk_alloc_spy_chain_get_fname(struct sctk_alloc_chain * chain,char * fname);
struct sctk_alloc_spy_event * sctk_alloc_spy_chain_get_event(struct sctk_alloc_spy_chain * spy);
void sctk_alloc_spy_chain_setup_event(struct sctk_alloc_spy_event * event,enum sctk_alloc_spy_event_type type);
void sctk_alloc_spy_chain_push_event(struct sctk_alloc_spy_chain * spy,struct sctk_alloc_spy_event * event);

/************************* FUNCTION ************************/
void sctk_alloc_spy_chain_register(struct sctk_alloc_spy_chain * spy);
void sctk_alloc_spy_chain_unregister(struct sctk_alloc_spy_chain * spy);
void sctk_alloc_spy_chain_at_exit(void);

/************************* FUNCTION ************************/
void sctk_alloc_spy_emit_event_init(struct sctk_alloc_chain * chain);
void sctk_alloc_spy_emit_event_destroy(struct sctk_alloc_chain * chain);
void sctk_alloc_spy_emit_event_add_macro_bloc(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc);
void sctk_alloc_spy_emit_event_free_macro_bloc(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * bloc);
void sctk_alloc_spy_emit_event_remote_free(struct sctk_alloc_chain * local_chain,struct sctk_alloc_chain * distant_chain,void * ptr);
void sctk_alloc_spy_emit_event_flush_rfq(struct sctk_alloc_chain * chain,int nb_entries);
void sctk_alloc_spy_emit_event_flush_rfq_entry(struct sctk_alloc_chain * chain,void * ptr);
void sctk_alloc_spy_emit_event_chain_alloc(struct sctk_alloc_chain * chain,unsigned long request,void * ptr,unsigned long size,unsigned long boundary);
void sctk_alloc_spy_emit_event_chain_free(struct sctk_alloc_chain * chain,void * ptr,unsigned long size);
void sctk_alloc_spy_emit_event_chain_huge_alloc(struct sctk_alloc_chain * chain,unsigned long request,void * ptr,unsigned long size,unsigned long boundary);
void sctk_alloc_spy_emit_event_chain_huge_free(struct sctk_alloc_chain * chain,void * ptr,unsigned long size);
void sctk_alloc_spy_emit_event_chain_split(struct sctk_alloc_chain * chain,void * base_addr,unsigned long chunk_size,unsigned long residut_size);
void sctk_alloc_spy_emit_event_chain_merge(struct sctk_alloc_chain * chain, void * base_addr,unsigned long size);
void sctk_alloc_spy_emit_event_next_is_realloc(struct sctk_alloc_chain * chain, void * orig_ptr,unsigned long new_requested_size);

/************************* FUNCTION ************************/
int sctk_alloc_spy_exename_filter_accept(void);

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_SPY_H

