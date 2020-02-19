#define _GNU_SOURCE
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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #   - BOUHROUR Stephane stephane.bouhrour@exascale-computing.eu        # */
/* #                                                                      # */
/* ######################################################################## */

/**********
 * MACROS *
 **********/

#define SCTK_DONOT_REDEFINE_KILL

#undef sleep
#undef usleep


/* #define SCTK_CHECK_CODE_RETURN */
#ifdef SCTK_CHECK_CODE_RETURN
	#define sctk_check(res,val) assume(res == val)
#else
	#define sctk_check(res,val) (void)(0)
#endif

/***********
 * HEADERS *
 ***********/

#include <mpc_config.h>
#include <mpcthread_config.h>
#include <mpc_runtime_config.h>
#include <mpc_common_helper.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <mpc_topology.h>
#include <mpc_topology_render.h>

#include <sctk_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_flags.h>


#include <sctk_thread.h>
#include <sctk_posix_thread.h>
#include <sctk_internal_thread.h>
#include <sctk_kernel_thread.h>
#include <sctk_thread_generic.h>

#include <sctk_tls.h>

#include <sctk_futex.h>

#ifdef MPC_Debugger
	#include <sctk_thread_dbg.h>
#endif

#ifdef MPC_USE_EXTLS
	#include <extls.h>
#endif

/***************
 * LOCAL TYPES *
 ***************/

typedef unsigned sctk_long_long sctk_timer_t;

/********************
 * GLOBAL VARIABLES *
 ********************/

volatile int start_func_done = 0;

static volatile long sctk_nb_user_threads = 0;

volatile int sctk_multithreading_initialised = 0;

struct sctk_alloc_chain *sctk_thread_tls = NULL;


/******************
 * THREAD COUNTER *
 ******************/

/* Register, unregister running thread */
volatile int sctk_current_local_tasks_nb = 0;
sctk_thread_mutex_t sctk_current_local_tasks_lock = SCTK_THREAD_MUTEX_INITIALIZER;

void sctk_unregister_task( const int i )
{
	sctk_thread_mutex_lock ( &sctk_current_local_tasks_lock );
	sctk_current_local_tasks_nb--;
	sctk_thread_mutex_unlock ( &sctk_current_local_tasks_lock );
}

void sctk_register_task( const int i )
{
	sctk_thread_mutex_lock ( &sctk_current_local_tasks_lock );
	sctk_current_local_tasks_nb++;
	sctk_thread_mutex_unlock ( &sctk_current_local_tasks_lock );
}

/* Return the number of running tasks (i.e tasks already registered) */
int sctk_thread_get_current_local_tasks_nb()
{
	return sctk_current_local_tasks_nb;
}

/***************************
 * VP LAYOUT AND VP LAUNCH *
 ***************************/

static int sctk_first_local = 0;
static int sctk_last_local = 0;


// default algorithme
int sctk_get_init_vp_and_nbvp_default( int i, int *nbVp )
{
	/*   int rank_in_node; */
	int slot_size;
	int task_nb;
	int proc = -1;
	int first;
	int last;
	int cpu_nb;
	int total_tasks_number;
	int cpu_per_task;
	int j;
	cpu_nb = mpc_topology_get_pu_count(); // number of cpu per process
	total_tasks_number = mpc_common_get_task_count();
	/*   rank_in_node = i - sctk_first_local; */
	/*   sctk_nodebug("check for %d(%d) %d cpu",i,rank_in_node,cpu_nb); */
	assume( ( sctk_last_local != 0 ) || ( total_tasks_number == 1 ) || ( mpc_common_get_process_rank() == 0 ) );
	task_nb = sctk_last_local - sctk_first_local + 1;
	slot_size = task_nb / cpu_nb;
	cpu_per_task = cpu_nb / task_nb;

	if ( cpu_per_task < 1 )
	{
		cpu_per_task = 1;
	}

	/* TODO: cpu_per_task=1 => put MPI tasks close */
	// cpu_per_task = 1 ;

	/* Normalize i if i the the global number insted of localnumber*/
	// hmt
	if ( i >= task_nb )
	{
		i = i - sctk_first_local;
	}

	sctk_nodebug( "i normalized=%d", i );
	first = 0;
	sctk_nodebug( "cpu_per_task %d", cpu_per_task );
	j = 0;

	for ( proc = 0; proc < cpu_nb; proc += cpu_per_task )
	{
		int local_slot_size;
		local_slot_size = slot_size;
		sctk_nodebug( "local_slot_size=%d", local_slot_size );

		if ( ( task_nb % cpu_nb ) > j )
		{
			local_slot_size++;
			sctk_nodebug( "local_slot_size inside the if=%d", local_slot_size );
		}

		sctk_nodebug( "%d proc %d slot size", proc, local_slot_size );
		last = first + local_slot_size - 1;
		sctk_nodebug( "First %d last %d", first, last );

		if ( ( i >= first ) && ( i <= last ) )
		{
			if ( ( cpu_nb % task_nb > j ) && ( cpu_nb > task_nb ) )
			{
				*nbVp = cpu_per_task + 1;
			}
			else
			{
				*nbVp = cpu_per_task;
			}

			sctk_nodebug( "sctk_get_init_vp: Put task %d on VP %d", i, proc );
			return proc;
		}

		first = last + 1;
		j++;

		if ( ( cpu_nb % task_nb >= j ) && ( cpu_nb > task_nb ) )
		{
			proc++;
		}
	}

	sctk_nodebug( "sctk_get_init_vp: (After loop) Put task %d on VP %d", i, proc );
	fflush( stdout );
	sctk_abort();
	return proc;
}

int sctk_get_init_vp_and_nbvp_numa_packed( int i, int *nbVp )
{
	/*   int rank_in_node; */
	// declaration
	int slot_size;
	int task_nb;
	int proc;
	int first;
	int last;
	int cpu_nb;
	int total_tasks_number;
	int cpu_per_task;
	int j;
	int nb_numa_node_per_node;
	int nb_cpu_per_numa_node;
	int nb_task_per_numa_node;
	// initialization
	cpu_nb = mpc_topology_get_pu_count(); // number of cpu per process
	nb_numa_node_per_node =
	    mpc_topology_get_numa_node_count(); // number of numa nodes in the node
	// config
	int T = sctk_last_local - sctk_first_local + 1; // number of task per node
	int C = cpu_nb;									// number of cpu per node
	int N = nb_numa_node_per_node;					// number of numa nodes
	int t = T % N;									// buffer task
	int cpu_per_numa_node = C / N;					// cpu per numa node
	int Tnk; // task per numa node
	// task already treated
	int tat = 0;
	*nbVp = 1;
	int k; // numa_node ID

	for ( k = 0; k < N; k++ )
	{
		// printf("for k=%d\n",k);
		// On ajoute une tache en plus aux premiers noeuds numas
		Tnk = T / N;

		if ( k < t )
		{
			Tnk += 1;
			// printf("if [k=%d] < [t=%d] -> Tnk+=1 %d\n", k, t, Tnk);
		}

		// Si  tat <= i < Tnk+tat // est ce qu on est dans ce noeud numa la ?
		if ( tat <= i && i < ( Tnk + tat ) )
		{
			// printf("if [tat=%d] <= [i=%d] < [Tnk=%d+tat=%d] %d\n", tat, i, Tnk,
			// tat, Tnk+tat);

			// update *nbVP
			if ( k * ( cpu_per_numa_node ) + ( i - tat ) ==
			     k * ( cpu_per_numa_node ) + Tnk - 1 )
			{
				*nbVp = ( cpu_per_numa_node - Tnk ) + 1;
			}

			// printf("proc=%d, *nbVp=%d\n", k*(cpu_per_numa_node) + (i-tat),*nbVp);
			// return numanode id * (C/N) + (i - tat)
			//      fixe premier cpu du noeud numa     + difference de la tache en
			//      cours
			return k * ( cpu_per_numa_node ) + ( i - tat );
		}

		// Si nous avons une tache en plus
		// printf("else tat=%d += Tnk=%d -> %d\n", tat, Tnk, tat+Tnk);
		tat += Tnk;
	}

	//mpc_common_debug("sctk_get_init_vp: (After loop) Put task %d on VP %d", i, proc);
	sctk_abort();
	return proc;
}

int sctk_get_init_vp_and_nbvp_numa( int i, int *nbVp )
{
	int task_nb = mpc_common_get_local_task_count();
	int cpu_nb = mpc_topology_get_pu_count(); // number of cpu per process
	int numa_node_per_node_nb = mpc_topology_get_numa_node_count();
	int cpu_per_numa_node = cpu_nb / numa_node_per_node_nb;
	int global_id = i;
	int numa_node_id = ( global_id * numa_node_per_node_nb ) / task_nb;
	int local_id = mpc_common_get_local_task_rank();
	global_id - ( ( ( numa_node_id * task_nb ) + numa_node_per_node_nb - 1 ) /
	              numa_node_per_node_nb );
	int task_per_numa_node =
	    ( ( ( numa_node_id + 1 ) * task_nb + numa_node_per_node_nb - 1 ) /
	      numa_node_per_node_nb ) -
	    ( ( ( numa_node_id ) * task_nb + numa_node_per_node_nb - 1 ) /
	      numa_node_per_node_nb );
	int proc_local = ( local_id * cpu_per_numa_node ) / task_per_numa_node;
	int proc_global = proc_local + ( numa_node_id * cpu_per_numa_node );
	// calculate nbVp value
	int proc_next_global;
	int proc_next_local;

	if ( local_id < task_per_numa_node )
	{
		proc_next_global =
		    ( ( local_id + 1 ) * cpu_per_numa_node ) / task_per_numa_node;
		proc_next_local = proc_next_global + numa_node_id * cpu_per_numa_node;
		*nbVp = proc_next_local - proc_global;
	}
	else
	{
		*nbVp = cpu_per_numa_node - proc_local;
	}

	if ( *nbVp < 1 )
	{
		*nbVp = 1;
	}

	// DEBUG
	// char hostname[1024];
	// gethostname(hostname,1024);
	// FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
	// fprintf(hostname_fd,"INIT        task_nb %d cpu_per_node %d
	// numa_node_per_node_nb %d numa_node_id %d task_per_numa_node %d local_id %d
	// global_id %d proc_global %d current_cpu %d mpc_common_get_local_task_count %d
	// nbVp %d\n",
	//        task_nb,
	//        cpu_nb ,
	//        numa_node_per_node_nb,
	//        numa_node_id ,
	//        task_per_numa_node,
	//        local_id  ,
	//        global_id ,
	//        proc_global,
	//        mpc_topology_get_current_cpu(),
	//        mpc_common_get_local_task_count(),
	//        *nbVp
	//       );
	// fflush(hostname_fd);
	// fclose(hostname_fd);
	// ENDDEBUG
	return proc_global;
}

int sctk_get_init_vp_and_nbvp( int i, int *nbVp )
{
	// return sctk_get_init_vp_and_nbvp_default (i, nbVp);
	// return sctk_get_init_vp_and_nbvp_numa (i, nbVp);
	// return sctk_get_init_vp_and_nbvp_numa_packed (i, nbVp);
	// function pointer to get the thread placement policy from the config
	int ( *thread_placement_policy )( int i, int *nbVp );
	thread_placement_policy = ( int ( * )( int, int * ) ) sctk_runtime_config_get()
	                          ->modules.thread.placement_policy.value;
	return thread_placement_policy( i, nbVp );
}

int sctk_get_init_vp( int i )
{
	int dummy;
	return sctk_get_init_vp_and_nbvp( i, &dummy );
}

static inline void __vp_placement_init_data( int *vp_start_thread, int *vp_thread_count )
{
	int total_tasks_number = mpc_common_get_task_count();
	int local_threads = total_tasks_number / mpc_common_get_process_count();

	if ( total_tasks_number % mpc_common_get_process_count() > mpc_common_get_process_rank() )
	{
		local_threads++;
	}

	int start_thread = local_threads * mpc_common_get_process_rank();

	if ( total_tasks_number % mpc_common_get_process_count() <= mpc_common_get_process_rank() )
	{
		start_thread += total_tasks_number % mpc_common_get_process_count();
	}

	TODO( "THIS GLOBAL IS UGLY" );
	sctk_first_local = start_thread;
	sctk_last_local = start_thread + local_threads - 1;
	*vp_start_thread = start_thread;
	*vp_thread_count = local_threads;
}

/**********************
 * THREAD DATA GETTER *
 **********************/

static sctk_thread_data_t sctk_main_datas = SCTK_THREAD_DATA_INIT;

sctk_thread_key_t stck_task_data;

void sctk_thread_data_init ()
{
	sctk_thread_key_create ( &stck_task_data, NULL );
	sctk_nodebug ( "stck_task_data = %d", stck_task_data );
	sctk_thread_data_set ( &sctk_main_datas );
}

static void sctk_thread_data_reset()
{
	sctk_thread_data_t *tmp;
	tmp = ( sctk_thread_data_t * )sctk_thread_getspecific( stck_task_data );

	if ( tmp == NULL )
	{
		sctk_thread_data_set( &sctk_main_datas );
	}
}

void sctk_thread_data_set( sctk_thread_data_t *task_id )
{
	sctk_thread_setspecific ( stck_task_data, ( void * ) task_id );
}

sctk_thread_data_t *__sctk_thread_data_get ( int no_disguise )
{
	sctk_thread_data_t *tmp;

	if ( sctk_multithreading_initialised == 0 )
	{
		tmp = &sctk_main_datas;
	}
	else
	{
		tmp = ( sctk_thread_data_t * ) sctk_thread_getspecific ( stck_task_data );
	}

	if ( tmp )
	{
		if ( ( tmp->my_disguisement != NULL ) && ( no_disguise == 0 ) )
		{
			return tmp->my_disguisement;
		}
	}

	return tmp;
}


sctk_thread_data_t *sctk_thread_data_get ()
{
	return __sctk_thread_data_get( 0 );
}

/****************
 * TIMER THREAD *
 ****************/

volatile sctk_timer_t ___timer_thread_ticks = 0;
volatile int ___timer_thread_running = 1;

static void *___timer_thread_main ( void *arg )
{
	sctk_ethread_mxn_init_kethread ();
	assume ( arg == NULL );

	while ( ___timer_thread_running )
	{
		kthread_usleep ( sctk_time_interval * 1000 );
		___timer_thread_ticks++;
	}

	return NULL;
}


void __timer_thread_start( void )
{
	/* Create the timer thread */
	kthread_t timer_thread;
	kthread_create ( &timer_thread, ___timer_thread_main, NULL );
}

void __timer_thread_end( void )
{
	___timer_thread_running = 0;
}


/*******************************
 * THREAD CLEANUP PUSH AND POP *
 *******************************/


static sctk_thread_key_t ___thread_cleanup_callback_list_key;


void _sctk_thread_cleanup_push ( struct _sctk_thread_cleanup_buffer *__buffer,
                                 void ( *__routine ) ( void * ), void *__arg )
{
	struct _sctk_thread_cleanup_buffer **__head;
	__buffer->__routine = __routine;
	__buffer->__arg = __arg;
	__head = sctk_thread_getspecific ( ___thread_cleanup_callback_list_key );
	sctk_nodebug ( "%p %p %p", __buffer, __head, *__head );
	__buffer->next = *__head;
	*__head = __buffer;
	sctk_nodebug ( "%p %p %p", __buffer, __head, *__head );
}

void _sctk_thread_cleanup_pop ( struct _sctk_thread_cleanup_buffer *__buffer,
                                int __execute )
{
	struct _sctk_thread_cleanup_buffer **__head;
	__head = sctk_thread_getspecific ( ___thread_cleanup_callback_list_key );
	*__head = __buffer->next;

	if ( __execute )
	{
		__buffer->__routine ( __buffer->__arg );
	}
}

static inline void __init_thread_cleanup_callback_key( void )
{
	sctk_thread_key_create ( &___thread_cleanup_callback_list_key, ( void ( * )( void * ) ) NULL );
}


static inline void __set_thread_cleanup_callback_key( void )
{
	struct _sctk_thread_cleanup_buffer *ptr_cleanup = NULL;
	sctk_thread_setspecific ( ___thread_cleanup_callback_list_key, &ptr_cleanup );
}




/*****************************
 * VIRTUAL PROCESSORS LAUNCH *
 *****************************/

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#pragma weak MPC_Process_hook
void  MPC_Process_hook( void )
{
	/*This function is used to intercept MPC process's creation when profiling*/
}
#endif

static inline void __prepare_free_pages( void )
{
	void *p1, *p2, *p3, *p4;
	p1 = sctk_malloc ( 1024 * 16 );
	p2 = sctk_malloc ( 1024 * 16 );
	p3 = sctk_malloc ( 1024 * 16 );
	p4 = sctk_malloc ( 1024 * 16 );
	sctk_free ( p1 );
	sctk_free ( p2 );
	sctk_free ( p3 );
	sctk_free ( p4 );
}

static inline void __init_brk_for_task( void )
{
	size_t s;
	void *tmp;
	size_t start = 0;
	size_t size;
	sctk_enter_no_alloc_land ();
	size = ( size_t ) ( SCTK_MAX_MEMORY_SIZE );
	tmp = sctk_get_heap_start ();
	sctk_nodebug ( "INIT ADRESS %p", tmp );
	s = ( size_t ) tmp /*  + 1*1024*1024*1024 */ ;
	sctk_nodebug ( "Max allocation %luMo %lu",
	               ( unsigned long ) ( size /
	                                   ( 1024 * 1024 * mpc_common_get_process_count() ) ), s );
	start = s;
	start = start / SCTK_MAX_MEMORY_OFFSET;
	start = start * SCTK_MAX_MEMORY_OFFSET;

	if ( s > start )
	{
		start += SCTK_MAX_MEMORY_OFFSET;
	}

	tmp = ( void * ) start;
	sctk_nodebug ( "INIT ADRESS REALIGNED %p", tmp );

	if ( mpc_common_get_process_count() > 1 )
	{
		sctk_mem_reset_heap ( ( size_t ) tmp, size );
	}

	sctk_nodebug ( "Heap start at %p (%p %p)", sctk_get_heap_start (),
	               ( void * ) s, tmp );
	sctk_leave_no_alloc_land();
}

void mpc_thread_spawn_virtual_processors ( void *( *run ) ( void * ), void *arg )
{
	__init_thread_cleanup_callback_key();
        __set_thread_cleanup_callback_key();
	__prepare_free_pages();
	__init_brk_for_task();
	__timer_thread_start();
#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
	MPC_Process_hook();
#endif

	int start_thread = -1;
	int local_threads = -1;
	__vp_placement_init_data( &start_thread, &local_threads );

	sctk_thread_t *threads = ( sctk_thread_t * ) sctk_malloc( local_threads * sizeof( sctk_thread_t ) );

	int i;
	int local_task_id_counter = 0;

	mpc_common_init_trigger( "Before Starting VPs" );

	for ( i = start_thread; i < start_thread + local_threads; i++ )
	{
		sctk_register_task( local_task_id_counter );
		mpc_thread_create_vp_thread( &( threads[local_task_id_counter] ), NULL, run, arg, ( long ) i, local_task_id_counter );
		local_task_id_counter++;
	}

	start_func_done = 1;

	/* Start Waiting for the end of  all VPs */

	sctk_thread_wait_for_value_and_poll( ( int * )&sctk_current_local_tasks_nb, 0, NULL, NULL );

	sctk_multithreading_initialised = 0;
	__timer_thread_end();

	mpc_common_init_trigger( "After Ending VPs" );

	/* Make sure to call desctructors for main thread */
	sctk_tls_dtors_free( &( sctk_main_datas.dtors_head ) );
	sctk_free( threads );
}

/******************
 * INITIALIZATION *
 ******************/

void sctk_thread_init ( void )
{
	sctk_thread_tls = sctk_get_current_alloc_chain();
#ifdef MPC_Allocator
	assert( sctk_thread_tls != NULL );
#endif
#ifdef SCTK_CHECK_CODE_RETURN
	fprintf ( stderr, "Thread librarie return code check enable!!\n" );
#endif
	sctk_futex_context_init();
	/*Check all types */
}

static void _sctk_thread_cleanup_end ( struct _sctk_thread_cleanup_buffer **__buffer )
{
	if ( __buffer != NULL )
	{
		sctk_nodebug ( "end %p %p", __buffer, *__buffer );

		if ( *__buffer != NULL )
		{
			struct _sctk_thread_cleanup_buffer *tmp;
			tmp = *__buffer;

			while ( tmp != NULL )
			{
				tmp->__routine ( tmp->__arg );
				tmp = tmp->next;
			}
		}
	}
}

void sctk_thread_exit_cleanup ()
{
	sctk_thread_data_t *tmp;
	struct _sctk_thread_cleanup_buffer **__head;
	/** ** **/
	sctk_report_death ( sctk_thread_self() );
	/** **/
	tmp = sctk_thread_data_get ();
	sctk_thread_remove ( tmp );
	__head = sctk_thread_getspecific ( ___thread_cleanup_callback_list_key );
	_sctk_thread_cleanup_end ( __head );
	sctk_thread_setspecific ( ___thread_cleanup_callback_list_key, NULL );
	sctk_nodebug ( "%p", tmp );

	if ( tmp != NULL )
	{
		sctk_nodebug ( "ici %p %d", tmp, tmp->task_id );
#ifdef MPC_Message_Passing

		if ( tmp->task_id >= 0 && tmp->user_thread == 0 )
		{
			//sctk_nodebug ( "mpc_lowcomm_terminaison_barrier" );
			//mpc_lowcomm_terminaison_barrier ();
			//sctk_nodebug ( "mpc_lowcomm_terminaison_barrier done" );
			sctk_unregister_task ( tmp->task_id );
			sctk_net_send_task_end ( tmp->task_id, mpc_common_get_process_rank() );
		}

#endif
	}
}

void
sctk_thread_exit ( void *__retval )
{
	sctk_thread_exit_cleanup ();
	__sctk_ptr_thread_exit ( __retval );
}

/*******************
 * THREAD CREATION *
 *******************/


#include <dlfcn.h>

static inline void __tbb_init_for_mpc()
{
	/* BEGIN TBB SETUP */
	/**
	* #define macros are not used for TBB code injections
	* avoiding MPC recompilation when the user code wants to load TBB
	* Instead, we define two functions :
	*    - __tbb_init_for_mpc ( called around sctk_thread.c:643)
	*    - __tbb_finalize_for_mpc (called around sctk_thread.c:658)
	*
	* These functions are set via #pragma weak, empty when TBB is not
	* loaded and bound to our patchs in TBB when loaded.
	*
	* TODO: due to some issues, weak functions are replaced by dlsym accesses for now
	*/
	int init_cpu = mpc_topology_get_current_cpu();
	int cpuset_len, i, nb_cpusets;
	cpu_set_t *cpuset;
	sctk_get_init_vp_and_nbvp( mpc_common_get_task_rank(), &cpuset_len );
	init_cpu = mpc_topology_convert_logical_pu_to_os( init_cpu );
	nb_cpusets = 1;
	cpuset = sctk_malloc( sizeof( cpu_set_t ) * nb_cpusets );
	CPU_ZERO( cpuset );

	for ( i = 0; i < cpuset_len; ++i )
	{
		CPU_SET( init_cpu + i, cpuset );
	}

	void *next = dlsym( RTLD_NEXT, "__tbb_init_for_mpc" );

	if ( next )
	{
		void( *call )( cpu_set_t *, int ) = ( void( * )( cpu_set_t *, int ) )next;
		call( cpuset, cpuset_len );
	}
	else
	{
		sctk_nodebug( "Calling fake TBB Initializer" );
	}
}

static inline void __tbb_finalize_for_mpc()
{
	void *next = dlsym( RTLD_NEXT, "__tbb_finalize_for_mpc" );

	if ( next )
	{
		void( *call )() = ( void( * )() )next;
		call();
	}
	else
	{
		sctk_nodebug( "Calling fake TBB Finalizer" );
	}
}

static inline void __scheduler_set_thread_kind_mpi(void)
{
	// thread priority
	if ( mpc_common_get_flags()->new_scheduler_engine_enabled )
	{
		sctk_thread_generic_addkind_mask_self( KIND_MASK_MPI );
		sctk_thread_generic_set_basic_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority );
		sctk_thread_generic_setkind_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority );
		sctk_thread_generic_set_current_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.mpi_basic_priority );
	}
}

static void * ___vp_thread_start_routine( sctk_thread_data_t *__arg )
{
	void *res;

	sctk_thread_data_t tmp = *__arg;
	sctk_free( __arg );

	assume_m( mpc_topology_get_current_cpu() == ( int )tmp.bind_to, "___vp_thread_start_routine Bad Pinning");

	/* Bind the thread to the right core if we are using pthreads */
	if ( mpc_common_get_flags()->thread_library_init == sctk_pthread_thread_init )
	{
		mpc_topology_bind_to_cpu( tmp.bind_to );
	}

	//mark the given TLS as currant thread allocator
	sctk_set_tls( tmp.tls );
	//do NUMA migration if required
	sctk_alloc_posix_numa_migrate();
        __set_thread_cleanup_callback_key();

	// we do not have an MPI
	tmp.mpi_per_thread = NULL;
	// Set no disguise
	tmp.my_disguisement = NULL;
	tmp.ctx_disguisement = NULL;
	tmp.virtual_processor = sctk_thread_get_vp();

	sctk_thread_data_set( &tmp );
	sctk_thread_add( &tmp, sctk_thread_self() );

	/* Initialization of the profiler */
#ifdef MPC_Profiler
	sctk_internal_profiler_init();
#endif

#if defined( MPC_USE_EXTLS )
        /* TLS INTIALIZATION */
        sctk_tls_init();
        sctk_call_dynamic_initializers();
#endif

        sctk_register_thread_initial( task_rank );

        __scheduler_set_thread_kind_mpi();

#if defined( MPC_USE_CUDA )
	sctk_thread_yield();
	sctk_accl_cuda_init_context();
#endif
	sctk_tls_dtors_init( &( tmp.dtors_head ) );

	__tbb_init_for_mpc();

	sctk_report_creation( sctk_thread_self() );

        mpc_common_init_trigger("VP Thread Start");

	res = tmp.__start_routine( tmp.__arg );

        mpc_common_init_trigger("VP Thread End");


	assume_m( mpc_topology_get_current_cpu() == ( int )tmp.bind_to, "___vp_thread_start_routine Bad Pinning");


	sctk_report_death( sctk_thread_self() );

        __tbb_finalize_for_mpc();
	sctk_tls_dtors_free( &( tmp.dtors_head ) );
        sctk_unregister_task(tmp.task_id);

	sctk_thread_remove( &tmp );
	sctk_thread_data_set( NULL );

	return res;
}

int
mpc_thread_create_vp_thread ( sctk_thread_t *restrict __threadp,
                     const sctk_thread_attr_t *restrict __attr,
                     void *( *__start_routine ) ( void * ), void *restrict __arg,
                     long task_id, long local_task_id )
{
	int res;


	static unsigned int core = 0;

	/* We bind the parent thread to the vp where the child
	 * will be created. We must bind before calling
	 * __sctk_crete_thread_memory_area(). The child thread
	 * will be bound to the same core after its creation */
	int new_binding = sctk_get_init_vp ( core );
	core++;
	int previous_binding = mpc_topology_bind_to_cpu ( new_binding );

	assert( new_binding == mpc_topology_get_current_cpu() );

	struct sctk_alloc_chain *tls = __sctk_create_thread_memory_area ();
	assume ( tls != NULL );

	sctk_thread_data_t * tmp = ( sctk_thread_data_t * )__sctk_malloc ( sizeof ( sctk_thread_data_t ), tls );
        assume(tmp!=NULL);

	tmp->tls = tls;
	tmp->__arg = __arg;
	tmp->__start_routine = __start_routine;
	tmp->user_thread = 0;
	tmp->bind_to = new_binding;
	tmp->task_id = sctk_safe_cast_long_int ( task_id );
	tmp->local_task_id = sctk_safe_cast_long_int ( local_task_id );


#ifdef MPC_USE_EXTLS
	extls_ctx_t *old_ctx = sctk_extls_storage;
  extls_ctx_t** cur_tx = ((extls_ctx_t**)sctk_get_ctx_addr());
	*cur_tx = malloc( sizeof( extls_ctx_t ) );
	extls_ctx_herit( old_ctx, *cur_tx, LEVEL_TASK );
	extls_ctx_restore( *cur_tx );
#ifndef MPC_DISABLE_HLS

	if ( tmp->bind_to >= 0 ) /* if the thread is bound -> HLS */
	{
		extls_ctx_bind( *cur_tx, tmp->bind_to );
	}

#endif
#endif

	// create user thread
	res = __sctk_ptr_thread_create ( __threadp, __attr, ( void *( * )( void * ) )
	                                 ___vp_thread_start_routine,
	                                 ( void * ) tmp );


	mpc_topology_render_notify( task_id );

	/* We reset the binding */
	{
		mpc_topology_bind_to_cpu( previous_binding );
#ifdef MPC_USE_EXTLS
		extls_ctx_restore( old_ctx );
#endif
	}
	sctk_check ( res, 0 );
	return res;
}




void mpc_thread_per_thread_init_function() __attribute__( ( constructor ) );

void mpc_thread_per_thread_init_function()
{
	MPC_INIT_CALL_ONLY_ONCE
	mpc_common_init_callback_register( "Per Thread Init", "Allocator Numa Migrate", sctk_alloc_posix_numa_migrate, 0 );
#ifdef MPC_USE_EXTLS
	mpc_common_init_callback_register( "Per Thread Init", "Dynamic Initializers", sctk_call_dynamic_initializers, 1 );
#endif
}

static void sctk_thread_data_reset();
static void *
sctk_thread_create_tmp_start_routine_user( sctk_thread_data_t *__arg )
{
	void *res;
	sctk_thread_data_t tmp;
	memset( &tmp, 0, sizeof( sctk_thread_data_t ) );

	while ( start_func_done == 0 )
	{
		sctk_thread_yield();
	}

	sctk_thread_data_reset();
	/* FIXME Intel OMP: at some point, in pthread mode, the ptr_cleanup variable seems to
	* be corrupted. */
	struct _sctk_thread_cleanup_buffer **ptr_cleanup = malloc( sizeof( struct _sctk_thread_cleanup_buffer * ) );
	tmp = *__arg;
	sctk_set_tls( tmp.tls );
	*ptr_cleanup = NULL;
	sctk_thread_setspecific( ___thread_cleanup_callback_list_key, ptr_cleanup );
	sctk_free( __arg );
	tmp.virtual_processor = sctk_thread_get_vp();
	sctk_nodebug( "%d on %d", tmp.task_id, tmp.virtual_processor );
	sctk_thread_data_set( &tmp );
	sctk_thread_add( &tmp, sctk_thread_self() );
	/** ** **/
	sctk_report_creation( sctk_thread_self() );
	/** **/

	if ( mpc_common_get_flags()->new_scheduler_engine_enabled )
	{
		sctk_thread_generic_addkind_mask_self( KIND_MASK_PTHREAD );
		sctk_thread_generic_set_basic_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.posix_basic_priority );
		sctk_thread_generic_setkind_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.posix_basic_priority );
		sctk_thread_generic_set_current_priority_self(
		    sctk_runtime_config_get()->modules.scheduler.posix_basic_priority );
	}

	mpc_common_init_trigger( "Per Thread Init" );
	res = tmp.__start_routine( tmp.__arg );
	mpc_common_init_trigger( "Per Thread Release" );
	/** ** **/
	sctk_report_death( sctk_thread_self() );
	/** **/
	sctk_free( ptr_cleanup );
	sctk_thread_remove( &tmp );
	sctk_thread_data_set( NULL );
	return res;
}

/*
 * Sylvain: the following code was use for creating Intel OpenMP threads with MPC.
 * The TLS variables are automatically recopied into the OpenMP threads
 * */
#if 0
#include <dlfcn.h>
#define LIB "/lib64/libpthread.so.0"

int sctk_real_pthread_create ( pthread_t  *thread,
                               __const pthread_attr_t *attr, void *( *start_routine )( void * ), void *arg )
{
	static void *handle = NULL;
	static int ( *real_pthread_create )( pthread_t *,
	                                     __const pthread_attr_t *,
	                                     void *( * )( void * ), void * ) = NULL;

	if ( real_pthread_create == NULL )
	{
		handle = dlopen( LIB, RTLD_LAZY );

		if ( handle == NULL )
		{
			fprintf( stderr, "dlopen(%s) failed\n", LIB );
			exit( 1 );
		}

		real_pthread_create = dlsym( handle, "pthread_create" );

		if ( real_pthread_create == NULL )
		{
			fprintf( stderr, "dlsym(%s) failed\n", "pthread_create" );
			exit( 1 );
		}
	}

	real_pthread_create( thread, attr, start_routine, arg );
}

__thread pthread_t *sctk_pthread_bypass_thread = NULL;
int sctk_pthread_bypass_create( pthread_t  *thread,
                                __const pthread_attr_t *attr,
                                void *( *start_routine )( void * ), void *arg )
{
	int ret;
	int recursive = 0;

	if ( sctk_pthread_bypass_thread == thread )
	{
		recursive = 1;
	}

	sctk_pthread_bypass_thread = thread;
	/* Check if we are trying to create a thread previously passed here */
	sctk_warning( "PTHREAD CREATE %p!!", thread );

	if ( sctk_thread_data_get()->task_id == -1 || recursive )
	{
		sctk_warning( "NOT MPI task creates a thread", sctk_thread_data_get()->task_id );
		ret = sctk_real_pthread_create( thread, attr, start_routine, arg );
	}
	else
	{
		sctk_net_set_mode_hybrid();
		sctk_warning( "MPI task %d creates a thread", sctk_thread_data_get()->task_id );
		//	  sctk_thread_attr_setscope (attr, SCTK_THREAD_SCOPE_SYSTEM);
		ret = sctk_user_thread_create ( thread, attr, start_routine, arg );
	}

	sctk_pthread_bypass_thread = NULL;
	return ret;
}
#endif

static struct mpc_mpi_cl_per_mpi_process_ctx_s *( *___get_mpi_process_ctx_trampoline )( void ) = NULL;

void mpc_thread_get_mpi_process_ctx_set_trampoline( struct mpc_mpi_cl_per_mpi_process_ctx_s * ( *trampoline )( void ) )
{
	___get_mpi_process_ctx_trampoline = trampoline;
}

static inline struct mpc_mpi_cl_per_mpi_process_ctx_s *__get_mpi_process_ctx()
{
	if ( ___get_mpi_process_ctx_trampoline )
	{
		return ___get_mpi_process_ctx_trampoline();
	}

	return NULL;
}




static hwloc_topology_t topology_option_text;

int sctk_user_thread_create ( sctk_thread_t *restrict __threadp,
                              const sctk_thread_attr_t *restrict __attr,
                              void *( *__start_routine ) ( void * ),
                              void *restrict __arg )
{
	int res;
	sctk_thread_data_t *tmp;
	sctk_thread_data_t *tmp_father;
	struct sctk_alloc_chain *tls;
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	int user_thread;
	int scope_init;
	mpc_common_spinlock_lock ( &lock );
	sctk_nb_user_threads++;
	user_thread = sctk_nb_user_threads;
	mpc_common_spinlock_unlock ( &lock );
	tls = __sctk_create_thread_memory_area ();
	sctk_nodebug ( "create tls %p attr %p", tls, __attr );
	tmp = ( sctk_thread_data_t * )
	      __sctk_malloc ( sizeof ( sctk_thread_data_t ), tls );
	memset( tmp, 0, sizeof( sctk_thread_data_t ) );
	tmp_father = sctk_thread_data_get ();
	tmp->tls = tls;
	tmp->__arg = __arg;
	tmp->__start_routine = __start_routine;
	tmp->task_id = -1;
	tmp->user_thread = user_thread;
	tmp->father_data = __get_mpi_process_ctx();

	if ( tmp_father )
	{
		tmp->task_id = tmp_father->task_id;
	}

	sctk_nodebug( "Create Thread with MPI rank %d", tmp->task_id );
#if 0

	/* Must be disabled because it unbind midcro VPs */
	if ( __attr != NULL )
	{
		sctk_thread_attr_getscope ( __attr, &scope_init );
		sctk_nodebug ( "Thread to create with scope %d ", scope_init );

		if ( scope_init == SCTK_THREAD_SCOPE_SYSTEM )
		{
			mpc_topology_bind_to_process_cpuset();
		}
	}
	else
	{
		mpc_topology_bind_to_process_cpuset();
	}

#endif
#ifdef MPC_USE_EXTLS
	extls_ctx_t *old_ctx = sctk_extls_storage;
  extls_ctx_t** cur_tx = ((extls_ctx_t**)sctk_get_ctx_addr());
	*cur_tx = calloc( 1, sizeof( extls_ctx_t ) );
	extls_ctx_herit( old_ctx, *cur_tx, LEVEL_THREAD );
	extls_ctx_restore( *cur_tx );
#ifndef MPC_DISABLE_HLS

	if ( __attr )
	{
		sctk_thread_attr_getscope( __attr, &scope_init );
	}
	else
	{
		scope_init = SCTK_THREAD_SCOPE_PROCESS;    /* consider not a scope_system */
	}

	/* if the thread is bound and its scope is not SCOPE_SYSTEM */
	if ( tmp->bind_to >= 0 && scope_init != SCTK_THREAD_SCOPE_SYSTEM )
	{
		extls_ctx_bind( *cur_tx, tmp->bind_to );
	}

#endif
#endif
	res = __sctk_ptr_thread_user_create ( __threadp, __attr,
	                                      ( void *( * )( void * ) )
	                                      sctk_thread_create_tmp_start_routine_user,
	                                      ( void * ) tmp );
	sctk_check ( res, 0 );
#ifdef MPC_USE_EXTLS
	extls_ctx_restore( old_ctx );
#endif
#ifndef NO_INTERNAL_ASSERT

	if ( __attr != NULL )
	{
		sctk_thread_attr_getscope ( __attr, &scope_init );
		sctk_nodebug ( "Thread created with scope %d", scope_init );
	}

#endif
	TODO( "THIS CODE IS UGLY ! FIX TO REEENABLE" );
#if 0


	/* option graphic placement */
	if ( mpc_common_get_flags()->enable_topology_graphic_placement )
	{
		/* to be sure of __arg type to cast */
		if ( mpc_common_get_task_rank() != -1 )
		{
			struct mpcomp_mvp_thread_args_s *temp = ( struct mpcomp_mvp_thread_args_s * )__arg;
			int vp_local_processus = temp->target_vp;
			/* get os ind */
			int master = mpc_topology_render_get_current_binding();
			/* need the logical pu of the master from the total compute node topo computing with the os index */
			int master_logical = mpc_topology_render_get_logical_from_os_id( master );
			/* in the global scope of compute node topology the pu is*/
			int logical_pu = ( master_logical + vp_local_processus );
			/* convert logical in os ind in topology_compute_node */
			int os_pu = mpc_topology_render_get_current_binding_from_logical( logical_pu );
			mpc_topology_render_lock();
			/* fill file to communicate between process of the same compute node */
			mpc_topology_render_create( os_pu, master, mpc_common_get_task_rank() );
			mpc_topology_render_unlock();
		}
	}

	/* option text placement */
	if ( mpc_common_get_flags()->enable_topology_text_placement )
	{
		/* to be sure of __arg type to cast */
		if ( mpc_common_get_task_rank() != -1 )
		{
			struct mpcomp_mvp_thread_args_s *temp = ( struct mpcomp_mvp_thread_args_s * )__arg;
			int *tree_shape;
			mpcomp_node_t *root_node;
			mpcomp_meta_tree_node_t *root = NULL;
			int rank = temp->rank;
			int target_vp = temp->target_vp;
			root = temp->array;
			root_node = ( mpcomp_node_t * ) root[0].user_pointer;
			int max_depth = root_node->tree_depth - 1;
			tree_shape = root_node->tree_base + 1;
			int core_depth;
			static int done_init = 1;
			mpc_topology_render_lock();

			if ( done_init )
			{
				hwloc_topology_init( &topology_option_text );
				hwloc_topology_load( topology_option_text );
				done_init = 0;
			}

			mpc_topology_render_unlock();

			if ( mpc_common_get_flags()->enable_smt_capabilities )
			{
				core_depth = hwloc_get_type_depth( topology_option_text, HWLOC_OBJ_PU );
			}
			else
			{
				core_depth = hwloc_get_type_depth( topology_option_text, HWLOC_OBJ_CORE );
			}

			int *min_index = ( int * )malloc( sizeof( int ) * MPCOMP_AFFINITY_NB );
			min_index = mpc_omp_tree_array_compute_thread_min_rank( tree_shape, max_depth, rank, core_depth );
			/* get os ind */
			int master = mpc_topology_render_get_current_binding();
			// need the logical pu of the master from the total compute node topo computin with the os index to use for origin */
			int master_logical = mpc_topology_render_get_logical_from_os_id( master );
			/* in the global compute node topology the processus is*/
			int logical_pu = ( master_logical + target_vp );
			/* convert logical in os ind in topology_compute_node */
			int os_pu = mpc_topology_render_get_current_binding_from_logical( logical_pu );
			mpc_topology_render_lock();
			mpc_topology_render_text( os_pu, master, mpc_common_get_task_rank(), target_vp, 0, min_index, 0 );
			mpc_topology_render_unlock();
			free( min_index );
		}
	}

#endif
	return res;
}


/*****************
 * PTHREAD IFACE *
 *****************/


int
sctk_thread_atfork ( void ( *__prepare ) ( void ), void ( *__parent ) ( void ),
                     void ( *__child ) ( void ) )
{
	int res;
	res = __sctk_ptr_thread_atfork ( __prepare, __parent, __child );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_destroy ( sctk_thread_attr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_attr_destroy ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getdetachstate ( const sctk_thread_attr_t *__attr,
                                  int *__detachstate )
{
	int res;
	res = __sctk_ptr_thread_attr_getdetachstate ( __attr, __detachstate );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getguardsize ( const sctk_thread_attr_t *restrict __attr,
                                size_t *restrict __guardsize )
{
	int res;
	res = __sctk_ptr_thread_attr_getguardsize ( __attr, __guardsize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getinheritsched ( const sctk_thread_attr_t *
                                   restrict __attr, int *restrict __inherit )
{
	int res;
	res = __sctk_ptr_thread_attr_getinheritsched ( __attr, __inherit );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getschedparam ( const sctk_thread_attr_t *restrict __attr,
                                 struct sched_param *restrict __param )
{
	int res;
	res = __sctk_ptr_thread_attr_getschedparam ( __attr, __param );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getschedpolicy ( const sctk_thread_attr_t *restrict __attr,
                                  int *restrict __policy )
{
	int res;
	res = __sctk_ptr_thread_attr_getschedpolicy ( __attr, __policy );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getscope ( const sctk_thread_attr_t *restrict __attr,
                            int *restrict __scope )
{
	int res;
	res = __sctk_ptr_thread_attr_getscope ( __attr, __scope );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getstackaddr ( const sctk_thread_attr_t *restrict __attr,
                                void **restrict __stackaddr )
{
	int res;
	res = __sctk_ptr_thread_attr_getstackaddr ( __attr, __stackaddr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getstack ( const sctk_thread_attr_t *restrict __attr,
                            void **restrict __stackaddr,
                            size_t *restrict __stacksize )
{
	int res;
	res = __sctk_ptr_thread_attr_getstack ( __attr, __stackaddr, __stacksize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getstacksize ( const sctk_thread_attr_t *restrict __attr,
                                size_t *restrict __stacksize )
{
	int res;
	res = __sctk_ptr_thread_attr_getstacksize ( __attr, __stacksize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_init ( sctk_thread_attr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_attr_init ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setdetachstate ( sctk_thread_attr_t *__attr,
                                  int __detachstate )
{
	int res;
	res = __sctk_ptr_thread_attr_setdetachstate ( __attr, __detachstate );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setguardsize ( sctk_thread_attr_t *__attr,
                                size_t __guardsize )
{
	int res;
	res = __sctk_ptr_thread_attr_setguardsize ( __attr, __guardsize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setinheritsched ( sctk_thread_attr_t *__attr, int __inherit )
{
	int res;
	res = __sctk_ptr_thread_attr_setinheritsched ( __attr, __inherit );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setschedparam ( sctk_thread_attr_t *restrict __attr,
                                 const struct sched_param *restrict __param )
{
	int res;
	res = __sctk_ptr_thread_attr_setschedparam ( __attr, __param );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setschedpolicy ( sctk_thread_attr_t *__attr, int __policy )
{
	int res;
	res = __sctk_ptr_thread_attr_setschedpolicy ( __attr, __policy );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setscope ( sctk_thread_attr_t *__attr, int __scope )
{
	int res;
	sctk_nodebug ( "thread attr_setscope %d == %d || %d", __scope,
	               SCTK_THREAD_SCOPE_PROCESS, SCTK_THREAD_SCOPE_SYSTEM );
	res = __sctk_ptr_thread_attr_setscope ( __attr, __scope );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setstackaddr ( sctk_thread_attr_t *__attr, void *__stackaddr )
{
	int res;
	res = __sctk_ptr_thread_attr_setstackaddr ( __attr, __stackaddr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setstack ( sctk_thread_attr_t *__attr, void *__stackaddr,
                            size_t __stacksize )
{
	int res;
	res = __sctk_ptr_thread_attr_setstack ( __attr, __stackaddr, __stacksize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setstacksize ( sctk_thread_attr_t *__attr,
                                size_t __stacksize )
{
	int res;
	res = __sctk_ptr_thread_attr_setstacksize ( __attr, __stacksize );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_setbinding ( sctk_thread_attr_t *__attr, int __binding )
{
	int res;
	res = __sctk_ptr_thread_attr_setbinding ( __attr, __binding );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_attr_getbinding ( sctk_thread_attr_t *__attr, int *__binding )
{
	int res;
	res = __sctk_ptr_thread_attr_getbinding ( __attr, __binding );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrierattr_destroy ( sctk_thread_barrierattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_barrierattr_destroy ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrierattr_init ( sctk_thread_barrierattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_barrierattr_init ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrierattr_setpshared ( sctk_thread_barrierattr_t *__attr,
                                     int __pshared )
{
	int res;
	res = __sctk_ptr_thread_barrierattr_setpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrierattr_getpshared ( const sctk_thread_barrierattr_t *
                                     __attr, int *__pshared )
{
	int res;
	res = __sctk_ptr_thread_barrierattr_getpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrier_destroy ( sctk_thread_barrier_t *__barrier )
{
	int res;
	res = __sctk_ptr_thread_barrier_destroy ( __barrier );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrier_init ( sctk_thread_barrier_t *restrict __barrier,
                           const sctk_thread_barrierattr_t *restrict __attr,
                           unsigned int __count )
{
	int res;
	res = __sctk_ptr_thread_barrier_init ( __barrier, __attr, __count );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_barrier_wait ( sctk_thread_barrier_t *__barrier )
{
	int res;
	res = __sctk_ptr_thread_barrier_wait ( __barrier );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cancel ( sctk_thread_t __cancelthread )
{
	int res;
	res = __sctk_ptr_thread_cancel ( __cancelthread );
	sctk_check ( res, 0 );
	sctk_thread_yield ();
	return res;
}

int
sctk_thread_condattr_destroy ( sctk_thread_condattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_condattr_destroy ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_condattr_getpshared ( const sctk_thread_condattr_t *
                                  restrict __attr, int *restrict __pshared )
{
	int res;
	res = __sctk_ptr_thread_condattr_getpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_condattr_init ( sctk_thread_condattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_condattr_init ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_condattr_setpshared ( sctk_thread_condattr_t *__attr,
                                  int __pshared )
{
	int res;
	res = __sctk_ptr_thread_condattr_setpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_condattr_setclock ( sctk_thread_condattr_t *__attr,
                                clockid_t __clockid )
{
	int res;
	res = __sctk_ptr_thread_condattr_setclock ( __attr, __clockid );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_condattr_getclock ( sctk_thread_condattr_t *__attr,
                                clockid_t *__clockid )
{
	int res;
	res = __sctk_ptr_thread_condattr_getclock ( __attr, __clockid );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_broadcast ( sctk_thread_cond_t *__cond )
{
	int res;
	res = __sctk_ptr_thread_cond_broadcast ( __cond );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_destroy ( sctk_thread_cond_t *__cond )
{
	int res;
	res = __sctk_ptr_thread_cond_destroy ( __cond );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_init ( sctk_thread_cond_t *restrict __cond,
                        const sctk_thread_condattr_t *restrict __cond_attr )
{
	int res;
	res = __sctk_ptr_thread_cond_init ( __cond, __cond_attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_signal ( sctk_thread_cond_t *__cond )
{
	int res;
	res = __sctk_ptr_thread_cond_signal ( __cond );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_timedwait ( sctk_thread_cond_t *restrict __cond,
                             sctk_thread_mutex_t *restrict __mutex,
                             const struct timespec *restrict __abstime )
{
	int res;
	res = __sctk_ptr_thread_cond_timedwait ( __cond, __mutex, __abstime );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_cond_wait ( sctk_thread_cond_t *restrict __cond,
                        sctk_thread_mutex_t *restrict __mutex )
{
	int res;
	res = __sctk_ptr_thread_cond_wait ( __cond, __mutex );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_get_vp ()
{
	return __sctk_ptr_thread_get_vp ();
}

int
sctk_thread_detach ( sctk_thread_t __th )
{
	int res;
	res = __sctk_ptr_thread_detach ( __th );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_equal ( sctk_thread_t __thread1, sctk_thread_t __thread2 )
{
	int res;
	res = __sctk_ptr_thread_equal ( __thread1, __thread2 );
	sctk_check ( res, 0 );
	return res;
}

/* At exit generic trampoline */

static int ( *___per_task_atexit_trampoline )( void ( *func )( void ) ) = NULL;

void mpc_thread_per_mpi_task_atexit_set_trampoline( int ( *trampoline )( void ( *func )( void ) ) )
{
	___per_task_atexit_trampoline = trampoline;
}

static inline int __per_mpi_task_atexit( void ( *func )( void ) )
{
	if ( ___per_task_atexit_trampoline )
	{
		return ( ___per_task_atexit_trampoline )( func );
	}

	/* No trampoline */
	return 1;
}

int sctk_atexit( void ( *function )( void ) )
{
	/* We may have a TASK context replacing the proces one */
	mpc_common_debug( "Calling the MPC atexit function" );
	int ret  = __per_mpi_task_atexit( function );

	if ( ret == 0 )
	{
		/* We were in a task and managed to register ourselves */
		return ret;
	}

	/* It failed we may not be in a task then call libc */
	mpc_common_debug( "Calling the default atexit function" );
	/* We have no task level fallback to libc */
	return atexit( function );
}



/* Futex Generic Trampoline */

int  sctk_thread_futex( void *addr1, int op, int val1,
                        struct timespec *timeout, void *addr2, int val3 )
{
	return __sctk_ptr_thread_futex( addr1, op, val1, timeout, addr2, val3 );
}


int
sctk_thread_getconcurrency ( void )
{
	int res;
	res = __sctk_ptr_thread_getconcurrency ();
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_getcpuclockid ( sctk_thread_t __thread_id, clockid_t *__clock_id )
{
	int res;
	res = __sctk_ptr_thread_getcpuclockid ( __thread_id, __clock_id );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sched_get_priority_max ( int policy )
{
	int res;
	res = __sctk_ptr_thread_sched_get_priority_max ( policy );
	return res;
}

int
sctk_thread_sched_get_priority_min ( int policy )
{
	int res;
	res = __sctk_ptr_thread_sched_get_priority_min ( policy );
	return res;
}

int
sctk_thread_getschedparam ( sctk_thread_t __target_thread,
                            int *restrict __policy,
                            struct sched_param *restrict __param )
{
	int res;
	res = __sctk_ptr_thread_getschedparam ( __target_thread, __policy, __param );
	sctk_check ( res, 0 );
	return res;
}

void *
sctk_thread_getspecific ( sctk_thread_key_t __key )
{
	return __sctk_ptr_thread_getspecific ( __key );
}

int
sctk_thread_join ( sctk_thread_t __th, void **__thread_return )
{
	int res;
	res = __sctk_ptr_thread_join ( __th, __thread_return );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_kill ( sctk_thread_t thread, int signo )
{
	int res;
	res = __sctk_ptr_thread_kill ( thread, signo );
	sctk_check ( res, 0 );
	sctk_thread_yield ();
	return res;
}

int
sctk_thread_process_kill ( pid_t pid, int sig )
{
#ifndef WINDOWS_SYS
	int res;
	res = kill ( pid, sig );
	sctk_check ( res, 0 );
	sctk_thread_yield ();
	return res;
#else
	not_available ();
	return 1;
#endif
}

int
sctk_thread_sigmask ( int how, const sigset_t *newmask, sigset_t *oldmask )
{
	int res;
	res = __sctk_ptr_thread_sigmask ( how, newmask, oldmask );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sigwait ( const sigset_t *set, int *sig )
{
	not_implemented ();
	assume ( set == NULL );
	assume ( sig == NULL );
	return 0;
}

int
sctk_thread_sigpending ( sigset_t *set )
{
	int res;
	res = __sctk_ptr_thread_sigpending ( set );
	sctk_check ( res, 0 );
	sctk_thread_yield ();
	return res;
}

int
sctk_thread_sigsuspend ( sigset_t *set )
{
	int res;
	res = __sctk_ptr_thread_sigsuspend ( set );
	sctk_check ( res, 0 );
	sctk_thread_yield ();
	return res;
}

/*int
sctk_thread_sigaction( int signum, const struct sigaction* act,
		struct sigaction* oldact ){
  int res;
  res = __sctk_ptr_thread_sigaction( signum, act, oldact );
  return res;
}*/

int
sctk_thread_key_create ( sctk_thread_key_t *__key,
                         void ( *__destr_function ) ( void * ) )
{
	int res;
	res = __sctk_ptr_thread_key_create ( __key, __destr_function );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_key_delete ( sctk_thread_key_t __key )
{
	int res;
	res = __sctk_ptr_thread_key_delete ( __key );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_destroy ( sctk_thread_mutexattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_destroy ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_getpshared ( const sctk_thread_mutexattr_t *
                                   restrict __attr, int *restrict __pshared )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_getpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_getprioceiling ( const sctk_thread_mutexattr_t *
                                       attr, int *prioceiling )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_getprioceiling ( attr, prioceiling );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_setprioceiling ( sctk_thread_mutexattr_t *attr,
                                       int prioceiling )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_setprioceiling ( attr, prioceiling );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_getprotocol ( const sctk_thread_mutexattr_t *
                                    attr, int *protocol )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_getprotocol ( attr, protocol );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_setprotocol ( sctk_thread_mutexattr_t *attr,
                                    int protocol )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_setprotocol ( attr, protocol );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_gettype ( const sctk_thread_mutexattr_t *
                                restrict __attr, int *restrict __kind )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_gettype ( __attr, __kind );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_init ( sctk_thread_mutexattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_init ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_setpshared ( sctk_thread_mutexattr_t *__attr,
                                   int __pshared )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_setpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutexattr_settype ( sctk_thread_mutexattr_t *__attr, int __kind )
{
	int res;
	res = __sctk_ptr_thread_mutexattr_settype ( __attr, __kind );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_destroy ( sctk_thread_mutex_t *__mutex )
{
	int res;
	res = __sctk_ptr_thread_mutex_destroy ( __mutex );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_init ( sctk_thread_mutex_t *restrict __mutex,
                         const sctk_thread_mutexattr_t *restrict __mutex_attr )
{
	int res;
	res = __sctk_ptr_thread_mutex_init ( __mutex, __mutex_attr );
	sctk_check ( res, 0 );
	return res;
}

#pragma weak user_sctk_thread_mutex_lock=sctk_thread_mutex_lock
int
sctk_thread_mutex_lock ( sctk_thread_mutex_t *__mutex )
{
	int res;
	res = __sctk_ptr_thread_mutex_lock ( __mutex );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_spinlock ( sctk_thread_mutex_t *__mutex )
{
	int res;
	res = __sctk_ptr_thread_mutex_spinlock ( __mutex );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_timedlock ( sctk_thread_mutex_t *restrict __mutex,
                              const struct timespec *restrict __abstime )
{
	int res;
	res = __sctk_ptr_thread_mutex_timedlock ( __mutex, __abstime );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_trylock ( sctk_thread_mutex_t *__mutex )
{
	int res;
	res = __sctk_ptr_thread_mutex_trylock ( __mutex );
	sctk_check ( res, 0 );
	return res;
}

#pragma weak user_sctk_thread_mutex_unlock=sctk_thread_mutex_unlock
int
sctk_thread_mutex_unlock ( sctk_thread_mutex_t *__mutex )
{
	int res;
	res = __sctk_ptr_thread_mutex_unlock ( __mutex );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_mutex_getprioceiling ( const sctk_thread_mutex_t *restrict mutex,
                                   int *restrict prioceiling )
{
	int res = 0;
	not_implemented ();
	return res;
}

int
sctk_thread_mutex_setprioceiling ( sctk_thread_mutex_t *restrict mutex,
                                   int prioceiling, int *restrict old_ceiling )
{
	int res = 0;
	not_implemented ();
	return res;
}

int
sctk_thread_sem_init ( sctk_thread_sem_t *sem, int pshared,
                       unsigned int value )
{
	int res;
	res = __sctk_ptr_thread_sem_init ( sem, pshared, value );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sem_wait ( sctk_thread_sem_t *sem )
{
	int res;
	res = __sctk_ptr_thread_sem_wait ( sem );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sem_trywait ( sctk_thread_sem_t *sem )
{
	int res;
	res = __sctk_ptr_thread_sem_trywait ( sem );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sem_post ( sctk_thread_sem_t *sem )
{
	int res;
	res = __sctk_ptr_thread_sem_post ( sem );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sem_getvalue ( sctk_thread_sem_t *sem, int *sval )
{
	int res;
	res = __sctk_ptr_thread_sem_getvalue ( sem, sval );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_sem_destroy ( sctk_thread_sem_t *sem )
{
	int res;
	res = __sctk_ptr_thread_sem_destroy ( sem );
	sctk_check ( res, 0 );
	return res;
}

sctk_thread_sem_t *
sctk_thread_sem_open ( const char *__name, int __oflag, ... )
{
	sctk_thread_sem_t *tmp;

	if ( ( __oflag & O_CREAT ) )
	{
		va_list ap;
		int value;
		mode_t mode;
		va_start ( ap, __oflag );
		mode = va_arg ( ap, mode_t );
		value = va_arg ( ap, int );
		va_end ( ap );
		tmp = __sctk_ptr_thread_sem_open ( __name, __oflag, mode, value );
	}
	else
	{
		tmp = __sctk_ptr_thread_sem_open ( __name, __oflag );
	}

	return tmp;
}

int
sctk_thread_sem_close ( sctk_thread_sem_t *__sem )
{
	int res = 0;
	res = __sctk_ptr_thread_sem_close ( __sem );
	return res;
}

int
sctk_thread_sem_unlink ( const char *__name )
{
	int res = 0;
	res = __sctk_ptr_thread_sem_unlink ( __name );
	return res;
}

int
sctk_thread_sem_timedwait ( sctk_thread_sem_t *__sem,
                            const struct timespec *__abstime )
{
	int res = 0;
	res = __sctk_ptr_thread_sem_timedwait( __sem, __abstime );
	return res;
}


int
sctk_thread_once ( sctk_thread_once_t *__once_control,
                   void ( *__init_routine ) ( void ) )
{
	int res;
	res = __sctk_ptr_thread_once ( __once_control, __init_routine );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlockattr_destroy ( sctk_thread_rwlockattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_rwlockattr_destroy ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlockattr_getpshared ( const sctk_thread_rwlockattr_t *
                                    restrict __attr, int *restrict __pshared )
{
	int res;
	res = __sctk_ptr_thread_rwlockattr_getpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlockattr_init ( sctk_thread_rwlockattr_t *__attr )
{
	int res;
	res = __sctk_ptr_thread_rwlockattr_init ( __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlockattr_setpshared ( sctk_thread_rwlockattr_t *__attr,
                                    int __pshared )
{
	int res;
	res = __sctk_ptr_thread_rwlockattr_setpshared ( __attr, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_destroy ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_destroy ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_init ( sctk_thread_rwlock_t *restrict __rwlock,
                          const sctk_thread_rwlockattr_t *restrict __attr )
{
	int res;
	res = __sctk_ptr_thread_rwlock_init ( __rwlock, __attr );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_rdlock ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_rdlock ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_timedrdlock ( sctk_thread_rwlock_t *restrict __rwlock,
                                 const struct timespec *restrict __abstime )
{
	int res;
	res = __sctk_ptr_thread_rwlock_timedrdlock ( __rwlock, __abstime );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_timedwrlock ( sctk_thread_rwlock_t *restrict __rwlock,
                                 const struct timespec *restrict __abstime )
{
	int res;
	res = __sctk_ptr_thread_rwlock_timedwrlock ( __rwlock, __abstime );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_tryrdlock ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_tryrdlock ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_trywrlock ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_trywrlock ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_unlock ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_unlock ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_rwlock_wrlock ( sctk_thread_rwlock_t *__rwlock )
{
	int res;
	res = __sctk_ptr_thread_rwlock_wrlock ( __rwlock );
	sctk_check ( res, 0 );
	return res;
}

sctk_thread_t
sctk_thread_self ( void )
{
	return __sctk_ptr_thread_self ();
}

sctk_thread_t
sctk_thread_self_check ( void )
{
	return __sctk_ptr_thread_self_check ();
}

int
sctk_thread_setcancelstate ( int __state, int *__oldstate )
{
	int res;
	res = __sctk_ptr_thread_setcancelstate ( __state, __oldstate );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_setcanceltype ( int __type, int *__oldtype )
{
	int res;
	res = __sctk_ptr_thread_setcanceltype ( __type, __oldtype );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_setconcurrency ( int __level )
{
	int res;
	res = __sctk_ptr_thread_setconcurrency ( __level );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_setschedprio ( sctk_thread_t __p, int __i )
{
	int res;
	res = __sctk_ptr_thread_setschedprio ( __p, __i );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_setschedparam ( sctk_thread_t __target_thread, int __policy,
                            const struct sched_param *__param )
{
	int res;
	res = __sctk_ptr_thread_setschedparam ( __target_thread, __policy, __param );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_setspecific ( sctk_thread_key_t __key, const void *__pointer )
{
	int res;
	res = __sctk_ptr_thread_setspecific ( __key, __pointer );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_spin_destroy ( sctk_thread_spinlock_t *__lock )
{
	int res;
	res = __sctk_ptr_thread_spin_destroy ( __lock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_spin_init ( sctk_thread_spinlock_t *__lock, int __pshared )
{
	int res;
	res = __sctk_ptr_thread_spin_init ( __lock, __pshared );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_spin_lock ( sctk_thread_spinlock_t *__lock )
{
	int res;
	res = __sctk_ptr_thread_spin_lock ( __lock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_spin_trylock ( sctk_thread_spinlock_t *__lock )
{
	int res;
	res = __sctk_ptr_thread_spin_trylock ( __lock );
	sctk_check ( res, 0 );
	return res;
}

int
sctk_thread_spin_unlock ( sctk_thread_spinlock_t *__lock )
{
	int res;
	res = __sctk_ptr_thread_spin_unlock ( __lock );
	sctk_check ( res, 0 );
	return res;
}

void
sctk_thread_testcancel ( void )
{
	__sctk_ptr_thread_testcancel ();
}

int
sctk_thread_yield ( void )
{
	return __sctk_ptr_thread_yield ();
}

typedef struct sctk_thread_sleep_pool_s
{
	sctk_timer_t wake_time;
	int done;
} sctk_thread_sleep_pool_t;

static void
sctk_thread_sleep_pool ( sctk_thread_sleep_pool_t *wake_time )
{
	if ( wake_time->wake_time < ___timer_thread_ticks )
	{
		wake_time->done = 0;
	}
}

unsigned int
sctk_thread_sleep ( unsigned int seconds )
{
	__sctk_ptr_thread_testcancel ();
	sctk_thread_sleep_pool_t wake_time;
	wake_time.done = 1;
	wake_time.wake_time =
	    ( ( ( sctk_timer_t ) seconds * ( sctk_timer_t ) 1000 ) /
	      ( sctk_timer_t ) sctk_time_interval ) + ___timer_thread_ticks + 1;
	sctk_thread_yield ();
	__sctk_ptr_thread_testcancel ();
	sctk_thread_wait_for_value_and_poll ( &( wake_time.done ), 0,
	                                      ( void ( * )( void * ) )
	                                      sctk_thread_sleep_pool,
	                                      ( void * ) &wake_time );
	__sctk_ptr_thread_testcancel ();
	return 0;
}

int
sctk_thread_usleep ( unsigned int useconds )
{
	__sctk_ptr_thread_testcancel ();
	sctk_thread_sleep_pool_t wake_time;
	wake_time.done = 1;
	wake_time.wake_time =
	    ( ( ( sctk_timer_t ) useconds / ( sctk_timer_t ) 1000 ) /
	      ( sctk_timer_t ) sctk_time_interval ) + ___timer_thread_ticks + 1;
	sctk_thread_yield ();
	__sctk_ptr_thread_testcancel ();
	sctk_thread_wait_for_value_and_poll ( &( wake_time.done ), 0,
	                                      ( void ( * )( void * ) )
	                                      sctk_thread_sleep_pool,
	                                      ( void * ) &wake_time );
	__sctk_ptr_thread_testcancel ();
	return 0;
}

/*on ne prend pas en compte la precision en dessous de la micro-seconde
on ne gere pas les interruptions non plus*/
int
sctk_thread_nanosleep ( const struct timespec *req, struct timespec *rem )
{
	if ( req == NULL )
	{
		return EINVAL;
	}

	if ( ( req->tv_sec < 0 ) || ( req->tv_nsec < 0 ) || ( req->tv_nsec > 999999999 ) )
	{
		errno = EINVAL;
		return -1;
	}

	sctk_thread_sleep ( req->tv_sec );
	sctk_thread_usleep ( req->tv_nsec / 1000 );

	if ( rem != NULL )
	{
		rem->tv_sec = 0;
		rem->tv_nsec = 0;
	}

	return 0;
}

double
sctk_thread_get_activity ( int i )
{
	double tmp;
	tmp = __sctk_ptr_thread_get_activity ( i );
	sctk_nodebug ( "sctk_thread_get_activity %d %f", i, tmp );
	return tmp;
}

int
sctk_thread_dump ( char *file )
{
	return __sctk_ptr_thread_dump ( file );
}

int
sctk_thread_migrate ( void )
{
	return __sctk_ptr_thread_migrate ();
}

int
sctk_thread_restore ( sctk_thread_t thread, char *type, int vp )
{
	return __sctk_ptr_thread_restore ( thread, type, vp );
}

int
sctk_thread_dump_clean ( void )
{
	return __sctk_ptr_thread_dump_clean ();
}

void
sctk_thread_wait_for_value_and_poll ( volatile int *data, int value,
                                      void ( *func ) ( void * ), void *arg )
{
	__sctk_ptr_thread_wait_for_value_and_poll ( data, value, func, arg );
}

int
sctk_thread_timed_wait_for_value ( volatile int *data, int value, unsigned int max_time_in_usec )
{
	unsigned int end_time = ( ( ( sctk_timer_t ) max_time_in_usec / ( sctk_timer_t ) 1000 ) /
	                          ( sctk_timer_t ) sctk_time_interval ) + ___timer_thread_ticks + 1;
	unsigned int trials = 0;

	while ( *data != value )
	{
		if ( end_time < ___timer_thread_ticks )
		{
			/* TIMED OUT */
			return 1;
		}

		int left_to_wait = end_time - ___timer_thread_ticks;

		if ( ( 30 < trials ) && ( 5000 < left_to_wait ) )
		{
			sctk_thread_usleep( 1000 );
		}
		else
		{
			sctk_thread_yield();
		}

		trials++;
	}

	return 0;
}

void extls_wait_for_value( volatile int *data, int value )
{
	__sctk_ptr_thread_wait_for_value_and_poll ( data, value, NULL, NULL );
}

void
sctk_thread_wait_for_value ( volatile int *data, int value )
{
	__sctk_ptr_thread_wait_for_value_and_poll ( data, value, NULL, NULL );
}


void
sctk_thread_freeze_thread_on_vp ( sctk_thread_mutex_t *lock, void **list )
{
	__sctk_ptr_thread_freeze_thread_on_vp ( lock, list );
}

void
sctk_thread_wake_thread_on_vp ( void **list )
{
	__sctk_ptr_thread_wake_thread_on_vp ( list );
}

int
sctk_thread_getattr_np ( sctk_thread_t th, sctk_thread_attr_t *attr )
{
	return __sctk_ptr_thread_getattr_np ( th, attr );
}



#if 0
static void
sctk_net_poll ( void *arg )
{
#ifdef MPC_Message_Passing

	/*   static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER; */
	/*   if(sctk_thread_mutex_trylock(&lock) == 0){ */
	if ( sctk_net_adm_poll != NULL )
	{
		sctk_net_adm_poll ( arg );
	}

	if ( sctk_net_ptp_poll != NULL )
	{
		sctk_net_ptp_poll ( arg );
	}

	/*     sctk_thread_mutex_unlock(&(lock)); */
	/*   } */
#endif
}
#endif

void
sctk_kthread_wait_for_value_and_poll ( int *data, int value,
                                       void ( *func ) ( void * ), void *arg )
{
	while ( ( *data ) != value )
	{
		if ( func != NULL )
		{
			func ( arg );
		}

		kthread_usleep ( 500 );
	}
}

int
sctk_thread_proc_migration ( const int cpu )
{
	int tmp;
	sctk_thread_data_t *tmp_data;
	tmp_data = sctk_thread_data_get ();
	/*   sctk_assert(tmp_data != NULL); */
	tmp = __sctk_ptr_thread_proc_migration ( cpu );
	tmp_data->virtual_processor = sctk_thread_get_vp ();
	return tmp;
}

unsigned long
sctk_thread_atomic_add ( volatile unsigned long *ptr, unsigned long val )
{
	unsigned long tmp;
	static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	sctk_thread_mutex_lock ( &lock );
	tmp = *ptr;
	*ptr = tmp + val;
	sctk_thread_mutex_unlock ( &lock );
	return tmp;
}

/* Used by GCC to bypass TLS destructor calls */
int __cxa_thread_mpc_atexit( void( *dfunc )( void * ), void *obj, void *dso_symbol )
{
	sctk_thread_data_t *th;
	th = sctk_thread_data_get();
	sctk_tls_dtors_add( &( th->dtors_head ), obj, dfunc );
	return 0;
}

