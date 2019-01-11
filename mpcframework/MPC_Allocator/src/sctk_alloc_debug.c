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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#ifndef _MSC_VER
	#include <unistd.h>
#else
	#include <process.h>
#endif
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "sctk_alloc_chunk.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_inlined.h"
#include "sctk_alloc_chain.h"
#include "sctk_alloc_hooks.h"

/************************** CONSTS *************************/
static const char * SCTK_ALLOC_STATE_NAME[] = {"free","allocated"};
static const char * SCTK_ALLOC_TYPE_NAME[] = {"small","large"};
/** PTRACE output file. Use '-' for stderr. You can use %d to get the PID in filename. **/
// static const char SCTK_ALLOC_TRACE_FILE[] = "-";
#ifdef ENABLE_TRACE
static const char SCTK_ALLOC_TRACE_FILE[] = "alloc-trace-%d.txt";
#endif //ENABLE_TRACE

/*********************** PORTABILITY ***********************/
#ifndef _WIN32
	#define OPEN_FILE_PERMISSIONS O_TRUNC|O_WRONLY|O_CREAT,S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR
#else
	#define OPEN_FILE_PERMISSIONS O_TRUNC|O_WRONLY|O_CREAT
	//primitives functions on Windows
	#define write _write
	#define open _open
	#define close _close
	#define getpid() _getpid()
#endif

//usage of safe write in MPC
#ifdef MPC_Common
	#include "sctk_io_helper.h"
#else //MPC_Common
	#define sctk_safe_write(fd,buf,count) write((fd),(buf),(count))
#endif //MPC_Common

/************************* GLOBALS *************************/
/** File descriptor for PTRACE output file. **/
#ifdef ENABLE_TRACE
	static int SCTK_ALLOC_TRACE_FD = -1;
#endif

/************************* GLOBALS *************************/
#ifdef HAVE_MEMCHECK_H
/**
 * Permit to disable registration of chunk into memcheck for internal usage as we never free the
 * one from init steps (hwloc....).
 * Registration is enabled it 0 and disable if > 0.
 */
__thread unsigned int sctk_alloc_gbl_notify_memcheck = 0;
#endif //HAVE_MEMCHECK_H

/************************* FUNCTION ************************/
/**
 * If SCTK_ALLOC_TRACE_FD is set to -1, setup the output file descriptor for ptrace mode.
**/
#ifdef ENABLE_TRACE
void sctk_alloc_ptrace_init(void )
{
	char fname[2*sizeof(SCTK_ALLOC_TRACE_FILE)];
	//nothing to do
	if (SCTK_ALLOC_TRACE_FD != -1)
		return;

	if (SCTK_ALLOC_TRACE_FILE[0] == '-' && SCTK_ALLOC_TRACE_FILE[1] == '\0')
	{
		SCTK_ALLOC_TRACE_FD = STDERR_FILENO;
	} else {
		sctk_alloc_sprintf(fname,2*sizeof(SCTK_ALLOC_TRACE_FILE),SCTK_ALLOC_TRACE_FILE,getpid());
		puts("Open debug file.......");
		puts(fname);
		SCTK_ALLOC_TRACE_FD = open(fname,OPEN_FILE_PERMISSIONS);
		if (SCTK_ALLOC_TRACE_FD == -1)
		{
			perror(fname);
			abort();
		}
	}
}
#endif

/************************* FUNCTION ************************/
/**
 * Provide a non buffered printf for debugging as we must avoid to call the allocator in debugging
 * methods.
**/
void sctk_alloc_pdebug (const char * format,...)
{
	char tmp[4096];
	char tmp2[4096];
	va_list param;
	sctk_alloc_sprintf (tmp,4096, "SCTK_ALLOC_DEBUG : %s\n", format);
	va_start (param, format);
	sctk_alloc_vsprintf (tmp2,4096, tmp, param);
	va_end (param);
	sctk_safe_write(STDERR_FILENO,tmp2,(unsigned int)strlen(tmp2));
}

/************************* FUNCTION ************************/
#ifdef ENABLE_TRACE
/**
 * Provide a non buffered printf for debugging as we must avoid to call the allocator in debugging
 * methods.
**/
void sctk_alloc_ptrace (const char * format,...)
{
	char tmp[4096];
	char tmp2[4096];
	va_list param;

	if (SCTK_ALLOC_TRACE_FD == -1)
		sctk_alloc_ptrace_init();
	
	sctk_alloc_sprintf (tmp,4096, "SCTK_ALLOC_TRACE : %s\n", format);
	va_start (param, format);
	sctk_alloc_vsprintf (tmp2,4096, tmp, param);
	va_end (param);
	sctk_safe_write(SCTK_ALLOC_TRACE_FD,tmp2,(unsigned int)strlen(tmp2));
	fflush(stderr);
}
#endif

/************************* FUNCTION ************************/
void sctk_alloc_fprintf(int fd,const char * format,...)
{
	char tmp[4096];
	va_list param;
	va_start (param, format);
	sctk_alloc_vsprintf (tmp,4096, format, param);
	va_end (param);
	sctk_safe_write(fd,tmp,(unsigned int)strlen(tmp));
}

/************************* FUNCTION ************************/
void sctk_alloc_debug_dump_segment(int fd,void* base_addr, void* end_addr)
{
	sctk_alloc_vchunk vchunk;
	
	sctk_alloc_fprintf(fd,"============ SEGMENT %p - %p ============\n",base_addr,end_addr);

	//get first chunk of segment
	/** @todo it work only for large blocs **/
	vchunk = sctk_alloc_get_chunk((sctk_addr_t)base_addr + sizeof(struct sctk_alloc_chunk_header_large));

	while (((sctk_addr_t)sctk_alloc_get_large(vchunk)) + sctk_alloc_get_size(vchunk) < (sctk_addr_t)end_addr)
	{
		vchunk = sctk_alloc_get_next_chunk(vchunk);
		sctk_alloc_fprintf(fd,"Bloc %p (align = %2d) %12ld [%s] in state %s\n",sctk_alloc_get_ptr(vchunk),sctk_alloc_get_addr(vchunk)%32,sctk_alloc_get_size(vchunk),
		                   SCTK_ALLOC_TYPE_NAME[vchunk->type],SCTK_ALLOC_STATE_NAME[vchunk->state]);
	}

	sctk_alloc_fprintf(fd,"\n");
}

/************************* FUNCTION ************************/
void sctk_alloc_debug_dump_free_lists(__AL_UNUSED__ int fd, __AL_UNUSED__ struct sctk_alloc_free_chunk* free_lists)
{
	/*int i;
	struct sctk_alloc_free_chunk * fchunk = NULL;
	
	sctk_alloc_fprintf(fd,"=========================== FREE LISTS ===========================\n");
	
	for ( i = 0 ; i < SCTK_ALLOC_NB_FREE_LIST ; ++i)
	{
		sctk_alloc_fprintf(fd,"-------------------------- %10ld ----------------------------\n",SCTK_ALLOC_FREE_SIZES[i]);
		fchunk = free_lists[i].next;
		while (fchunk != free_lists+i)
		{
			sctk_alloc_fprintf(fd,"Bloc %p (align = %2d) %12ld [%s] in state %s\n",fchunk,((sctk_addr_t)fchunk)%32,sctk_alloc_get_chunk_header_large_size(&fchunk->header),
			                   SCTK_ALLOC_TYPE_NAME[sctk_alloc_get_chunk_header_large_info(&fchunk->header)->type],SCTK_ALLOC_STATE_NAME[sctk_alloc_get_chunk_header_large_info(&fchunk->header)->state]);
			fchunk = fchunk->next;
		}
	}
	sctk_alloc_fprintf(fd,"\n");*/
}

/************************* FUNCTION ************************/
void sctk_alloc_debug_dump_thread_pool(__AL_UNUSED__ int fd, __AL_UNUSED__ struct sctk_thread_pool* pool)
{
	//sctk_alloc_debug_dump_free_lists(fd,pool->free_lists);
}

/************************* FUNCTION ************************/
void sctk_alloc_debug_dump_alloc_chain(__AL_UNUSED__ struct sctk_alloc_chain* chain)
{
	/** @todo  Not thread safe **/
	/*static int id = 0;
	int fd;
	char fname[1024];

	//calc filename
	sctk_alloc_sprintf(fname,1024,"alloc-dump-%04d.txt",id);
// 	id++;

	//open output filei
	fd = open(fname,OPEN_FILE_PERMISSIONS);
	if (fd == -1)
	{
		perror(fname);
		return;
	}

	//dump segment
	sctk_alloc_debug_dump_segment(fd,chain->base_addr,chain->end_addr);
	//dump pool
	sctk_alloc_debug_dump_thread_pool(fd,&chain->pool);

	//close
	#ifndef _WIN32
		sync();
	#endif
	close(fd);*/
}

/************************* FUNCTION ************************/
/*#if defined(SCTK_ALLOC_DEBUG) && !defined(_WIN32)
void sctk_alloc_debug_setgault_handler(int signal, siginfo_t *si, void *arg)
{
	sctk_alloc_perror("SEGMENTATION FAULT");
	sctk_alloc_crash_dump();
}
#endif*/

/************************* FUNCTION ************************/
/*#if defined(SCTK_ALLOC_DEBUG) && !defined(_WIN32)
void sctk_alloc_debug_setup_sighandler()
{
	int res;
	static struct sigaction sa;
	memset(&sa,0,sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sctk_alloc_debug_setgault_handler;
	sa.sa_flags = SA_SIGINFO;

	res = sigaction(SIGSEGV,&sa,NULL);
	if ( res != 0 )
	{
		perror("Error while setting up the segfault handler");
		exit(1);
	}
}
#endif*/

/************************* FUNCTION ************************/
#ifdef SCTK_ALLOC_DEBUG
void sctk_alloc_debug_init(void )
{
	/*#ifndef _WIN32
		sctk_alloc_debug_setup_sighandler();
	#endif*/
}
#endif
