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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <infiniband/verbs.h>
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "mpc_common_asm.h"
#include "utlist.h"
#include "sctk_route.h"
#include "sctk_ib_prof.h"
#include "sctk_thread.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "PROF"
#include "sctk_ib_toolkit.h"

/* Reference clock */
static __thread volatile double reference_clock = -1;

#ifdef SCTK_IB_PROF
static OPA_int_t sctk_ib_prof_glob_counters[IB_PROF_GLOB_COUNTERS_MAX];
#endif

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ib_prof_init()
{
	sctk_nodebug ( "VP number: %d", mpc_topology_get_pu_count() );

	/* Initialize QP usage profiling */
#ifdef SCTK_IB_QP_PROF
	sctk_ib_prof_qp_init();
#endif
}

#ifdef SCTK_IB_PROF
double sctk_ib_prof_get_time_stamp()
{
	return sctk_get_time_stamp() - reference_clock;
}

void sctk_ib_prof_init_reference_clock()
{
	if ( reference_clock == -1 )
	{
		reference_clock = sctk_get_time_stamp();
	}
}
#endif

void sctk_ib_prof_init_task ( __UNUSED__ int rank, __UNUSED__ int vp )
{
	sctk_nodebug ( "Initialization with %d rails for rank %d", sctk_rail_count(), rank );
	sctk_ib_prof_qp_init_task ( rank, vp );
}


void sctk_ib_prof_finalize ( __UNUSED__ sctk_ib_rail_info_t *rail_ib )
{
#ifdef SCTK_IB_PROF

#ifdef SCTK_IB_MEM_PROF
	sctk_ib_prof_mem_finalize ( rail_ib );
#endif

#if 1
	char line[1024];
	char line_res[1024] = "\0";
	sprintf ( line, "%d ", mpc_common_get_process_rank() );
	strcat ( line_res, line );

	int i;

	for ( i = 0; i < IB_PROF_GLOB_COUNTERS_MAX; ++i )
	{
		sprintf ( line, "%s %d ",
		          sctk_ib_prof_glob_counters_name[i],
		          OPA_load_int ( &sctk_ib_prof_glob_counters[i] ) );
		strcat ( line_res, line );
	}

	sprintf ( line, "\n" );
	strcat ( line_res, line );
	fprintf ( stderr, "%s", line_res );
#endif
#endif
}


#ifdef SCTK_IB_QP_PROF
/*-----------------------------------------------------------
 *  QP profiling
 *----------------------------------------------------------*/
// TODO: should add a filter to record only a part of events

/* Size of the buffer */
#define QP_PROF_BUFF_SIZE 40000000
/* Path where profile files are stored */
#define QP_PROF_DIR "prof/node_%d"
/* Name of files */
#define QP_PROF_OUTPUT_FILE "qp_prof_%d"
static char QP_PROF_DONE = 0;

struct sctk_ib_prof_qp_buff_s
{
	int proc;
	size_t size;
	double ts;
	char from;
};

struct sctk_ib_prof_qp_s
{
	struct sctk_ib_prof_qp_buff_s buff[QP_PROF_BUFF_SIZE];
	int head;
	int fd;
	int task_id;
};

struct sctk_ib_prof_qp_s *qp_prof = NULL;

struct sctk_ib_prof_qp_s *sctk_ib_prof_qp_get ( int vp )
{
	return &qp_prof[vp];
}

/* Process initialization */
void sctk_ib_prof_qp_init()
{
	char dirname[256];

	sprintf ( dirname, QP_PROF_DIR, sctk_get_process_rank() );
	sctk_debug ( "Creating dirname %s", dirname );
	mkdir ( dirname, S_IRWXU );
}


/* Task initialization */
void sctk_ib_prof_qp_init_task ( int task_id, int vp )
{
	char pathname[256];
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	struct sctk_ib_prof_qp_s *tmp;

	sctk_debug ( "Task initialization" );

	mpc_common_spinlock_lock ( &lock );

	if ( qp_prof == NULL )
	{
		int vp_number;
		vp_number = mpc_topology_get_pu_count();
		assume ( vp_number >= 1 );
		qp_prof = sctk_malloc ( vp_number * sizeof ( struct sctk_ib_prof_qp_s ) );
		assume ( qp_prof );
		memset ( qp_prof, 0, vp_number * sizeof ( struct sctk_ib_prof_qp_s ) );
	}

	mpc_common_spinlock_unlock ( &lock );

	tmp = sctk_ib_prof_qp_get ( vp );

	sprintf ( pathname, QP_PROF_DIR"/"QP_PROF_OUTPUT_FILE, sctk_get_process_rank(), task_id );
	tmp->fd = open ( pathname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );
	assume ( tmp->fd != -1 );

	tmp->task_id = task_id;
	tmp->head = 0;

	/* Tasks synchronization */
	mpc_mp_terminaison_barrier ( task_id );
	mpc_mp_terminaison_barrier ( task_id );
	mpc_mp_terminaison_barrier ( task_id );
	mpc_mp_terminaison_barrier ( task_id );
	sctk_ib_prof_qp_write ( -1, 0, sctk_get_time_stamp(), PROF_QP_SYNC );
}

/*
 * Flush the buffer to the file
 */
void sctk_ib_prof_qp_flush()
{
	struct sctk_ib_prof_qp_s *tmp;
	tmp = sctk_ib_prof_qp_get ( mpc_topology_get_pu() );

	sctk_nodebug ( "Dumping file with %lu elements", tmp->head );
	write ( tmp->fd, tmp->buff,
	        tmp->head * sizeof ( struct sctk_ib_prof_qp_buff_s ) );
	tmp->head = 0;
}

/* Write a new prof line */
void sctk_ib_prof_qp_write ( int proc, size_t size, double ts, char from )
{

	struct sctk_ib_prof_qp_s *tmp;
	tmp = sctk_ib_prof_qp_get ( mpc_topology_get_pu() );
	assume ( tmp );

	/* We flush */
	if ( tmp->head > ( QP_PROF_BUFF_SIZE - 1 ) )
	{
		sctk_ib_prof_qp_flush();
	}


	if ( from == PROF_QP_CREAT )
	{
		sctk_debug ( "CREATION FOUND for rank %d on vp %d!!!", proc, mpc_topology_get_pu() );
	}

	tmp->buff[tmp->head].proc = proc;
	tmp->buff[tmp->head].size = size;
	tmp->buff[tmp->head].ts = ts;
	tmp->buff[tmp->head].from = from;
	tmp->head ++;
}

/* Finalize a task */
void sctk_ib_prof_qp_finalize_task ( int task_id )
{
	struct sctk_ib_prof_qp_s *tmp;
	tmp = sctk_ib_prof_qp_get ( mpc_topology_get_pu() );
	/* End marker */
	sctk_ib_prof_qp_write ( -1, 0, sctk_get_time_stamp(), PROF_QP_SYNC );
	sctk_nodebug ( "End of task %d (head:%d)", task_id, tmp->head );
	sctk_ib_prof_qp_flush();
	sctk_nodebug ( "FLUSHED End of task %d (head:%d)", task_id, tmp->head );
	close ( tmp->fd );
}
#endif


#ifdef SCTK_IB_MEM_PROF
/*-----------------------------------------------------------
 *  Memory profiling
 *----------------------------------------------------------*/
/* Size of the buffer */
#define MEM_PROF_BUFF_SIZE 40000000
/* Path where profile files are stored */
#define MEM_PROF_DIR "prof/node_%d"
/* Name of files */
#define MEM_PROF_OUTPUT_FILE "mem_prof"

struct sctk_ib_prof_mem_buff_s
{
	double ts;
	double mem;
};

struct sctk_ib_prof_mem_s
{
	struct sctk_ib_prof_mem_buff_s *buff;
	int head;
	int fd;
};

struct sctk_ib_prof_mem_s *mem_prof;

/*
 * Flush the buffer to the file
 */
void sctk_ib_prof_mem_flush()
{
	sctk_nodebug ( "Dumping file with %lu elements", mem_prof->head );
	write ( mem_prof->fd, mem_prof->buff,
	        mem_prof->head * sizeof ( struct sctk_ib_prof_mem_buff_s ) );
	mem_prof->head = 0;
}

/* Write a new prof line */
void sctk_ib_prof_mem_write ( double ts, double mem )
{

	/* We flush */
	if ( mem_prof->head > ( MEM_PROF_BUFF_SIZE - 1 ) )
	{
		sctk_ib_prof_mem_flush();
	}

	if ( mpc_common_get_process_rank() == 0 )
	{
		sctk_warning ( "[%d] Memory used: %fMB", mpc_common_get_process_rank(), mem / 1024.0 / 1024.0 );
	}

	mem_prof->buff[mem_prof->head].ts = ts;
	mem_prof->buff[mem_prof->head].mem = mem;
	mem_prof->head ++;
}

void *__mem_thread ( void *arg )
{
  mpc_topology_bind_to_cpu(-1);
	sctk_ib_rail_info_t *rail_ib = ( sctk_ib_rail_info_t * ) arg;

	while ( 1 )
	{
		sctk_ib_prof_mem_write ( sctk_get_time_stamp(), sctk_profiling_get_dataused() );
		sleep ( 1 );
	}
}

/* Process initialization */
void sctk_ib_prof_mem_init ( sctk_ib_rail_info_t *rail_ib )
{
	char pathname[256];
	sctk_thread_t pidt;
	sctk_thread_attr_t attr;

	mem_prof = sctk_malloc ( sizeof ( struct sctk_ib_prof_mem_s ) );
	mem_prof->buff = sctk_malloc ( MEM_PROF_BUFF_SIZE * sizeof ( struct sctk_ib_prof_mem_buff_s ) );

#if 1
	char dirname[256];

	sprintf ( dirname, MEM_PROF_DIR, sctk_get_process_rank() );
	sctk_debug ( "Creating dirname %s", dirname );
	mkdir ( dirname, S_IRWXU );
#endif

	sprintf ( pathname, MEM_PROF_DIR"/"MEM_PROF_OUTPUT_FILE, sctk_get_process_rank() );
	mem_prof->fd = open ( pathname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );
	assume ( mem_prof->fd != -1 );


	mem_prof->head = 0;

	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );
	sctk_user_thread_create ( &pidt, &attr, __mem_thread, ( void * ) rail_ib );
}

/* Process finalization */
void sctk_ib_prof_mem_finalize ( sctk_ib_rail_info_t *rail_ib )
{
	/* End marker */
	sctk_ib_prof_mem_write ( sctk_get_time_stamp(), PROF_QP_SYNC );
	sctk_ib_prof_mem_flush();
	close ( mem_prof->fd );
}

#endif
#endif
