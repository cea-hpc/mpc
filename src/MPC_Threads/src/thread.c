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
	#define sctk_check(res, val)    assume(res == val)
#else
	#define sctk_check(res, val)    (void)(0)
#endif

/***********
* HEADERS *
***********/

#include <mpc_config.h>

#include <mpc_common_helper.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <mpc_conf.h>

#include <mpc_topology.h>
#include <mpc_topology_render.h>

#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_flags.h>


#include "thread.h"

#include "thread_ptr.h"

#include <kthread.h>

#include <tls.h>

#include <futex.h>

#ifdef MPC_Thread_db
	#include <sctk_thread_dbg.h>
#endif

#ifdef MPC_USE_EXTLS
	#include <extls.h>
	#include <extls_hls.h>
#endif

#ifdef MPC_OpenMP
    #include "mpcomp_core.h"
#endif


#define MPC_MODULE "Threads/Threads"

/***************
* LOCAL TYPES *
***************/

typedef unsigned sctk_long_long   sctk_timer_t;

/********************
* GLOBAL VARIABLES *
********************/

volatile int start_func_done = 0;

static volatile long sctk_nb_user_threads = 0;

volatile int sctk_multithreading_initialised = 0;

struct sctk_alloc_chain *mpc_thread_tls = NULL;

/************************
 * THREAD MODULE CONFIG *
 ************************/


struct mpc_thread_config __thread_module_config;

struct mpc_thread_config  * _mpc_thread_config_get(void)
{
	return &__thread_module_config;
}

static inline void __thread_module_config_defaults(void)
{
	/* Here we set default values for thread config */
	snprintf(__thread_module_config.thread_layout, MPC_CONF_STRING_SIZE, "default");
	__thread_module_config.thread_timer_interval = 10;
	__thread_module_config.thread_timer_enabled = 1;
    __thread_module_config.kthread_stack_size = (10 * 1024 * 1024);
	__thread_module_config.ethread_spin_delay = 10;
}


static inline void __init_thread_module_config(void)
{
	__thread_module_config_defaults();

	mpc_conf_config_type_t *common = mpc_conf_config_type_init("common",
														       PARAM("layout", __thread_module_config.thread_layout ,MPC_CONF_STRING, "Layout to be used (default, numa or numa_packed)"),
														       PARAM("timerenabled", &__thread_module_config.thread_timer_enabled, MPC_CONF_BOOL, "Enable/Disable the timer thread in milliseconds"),
														       PARAM("interval", &__thread_module_config.thread_timer_interval, MPC_CONF_INT, "Wakeup interval of the timer thread in milliseconds"),
															   NULL);

    mpc_conf_config_type_t *kthread = mpc_conf_config_type_init("kthread",
														        PARAM("stack", &__thread_module_config.kthread_stack_size ,MPC_CONF_LONG_INT, "Stack size for kernel threads"),
															    NULL);


	mpc_conf_config_type_t *ethread = mpc_conf_config_type_init("ethread",
														        PARAM("spindelay", &__thread_module_config.ethread_spin_delay ,MPC_CONF_LONG_INT, "Number of direct attempts before locking"),
															    NULL);


	/* Aggegation for the thread config */

	mpc_conf_config_type_t *thconf = mpc_conf_config_type_init("thread",
														       PARAM("common", common, MPC_CONF_TYPE, "Common knobs for thread module"),
														       PARAM("kthread", kthread, MPC_CONF_TYPE, "Parameters for regular 'kernel' threads"),
														       PARAM("ethread", ethread, MPC_CONF_TYPE, "Parameters for 'ethread' user level threads"),
															   NULL);

	mpc_conf_root_config_append("mpcframework", thconf, "MPC Thread Configuration");
}


/******************
* THREAD COUNTER *
******************/

/* Register, unregister running thread */
volatile int       sctk_current_local_tasks_nb   = 0;
mpc_thread_mutex_t sctk_current_local_tasks_lock = SCTK_THREAD_MUTEX_INITIALIZER;

void sctk_unregister_task(__UNUSED__ const int i)
{
	mpc_thread_mutex_lock(&sctk_current_local_tasks_lock);
	sctk_current_local_tasks_nb--;
	mpc_thread_mutex_unlock(&sctk_current_local_tasks_lock);
}

void sctk_register_task(__UNUSED__ const int i)
{
	mpc_thread_mutex_lock(&sctk_current_local_tasks_lock);
	sctk_current_local_tasks_nb++;
	mpc_thread_mutex_unlock(&sctk_current_local_tasks_lock);
}

/* Return the number of running tasks (i.e tasks already registered) */
int mpc_thread_get_current_local_tasks_nb()
{
	return sctk_current_local_tasks_nb;
}

/***************************
* VP LAYOUT AND VP LAUNCH *
***************************/

static int sctk_first_local = 0;
static int sctk_last_local  = 0;


// default algorithme
int mpc_thread_get_task_placement_and_count_default(int i, int *nbVp)
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

	cpu_nb             = mpc_topology_get_pu_count(); // number of cpu per process
	total_tasks_number = mpc_common_get_task_count();
	/*   rank_in_node = i - sctk_first_local; */
	/*   mpc_common_nodebug("check for %d(%d) %d cpu",i,rank_in_node,cpu_nb); */
	assume( (sctk_last_local != 0) || (total_tasks_number == 1) || (mpc_common_get_process_rank() == 0) );
	task_nb      = sctk_last_local - sctk_first_local + 1;
	slot_size    = task_nb / cpu_nb;
	cpu_per_task = cpu_nb / task_nb;

	if(cpu_per_task < 1)
	{
		cpu_per_task = 1;
	}

	/* TODO: cpu_per_task=1 => put MPI tasks close */
	// cpu_per_task = 1 ;

	/* Normalize i if i the the global number insted of localnumber*/
	// hmt
	if(i >= task_nb)
	{
		i = i - sctk_first_local;
	}

	mpc_common_nodebug("i normalized=%d", i);
	first = 0;
	mpc_common_nodebug("cpu_per_task %d", cpu_per_task);
	j = 0;

	for(proc = 0; proc < cpu_nb; proc += cpu_per_task)
	{
		int local_slot_size;
		local_slot_size = slot_size;
		mpc_common_nodebug("local_slot_size=%d", local_slot_size);

		if( (task_nb % cpu_nb) > j)
		{
			local_slot_size++;
			mpc_common_nodebug("local_slot_size inside the if=%d", local_slot_size);
		}

		mpc_common_nodebug("%d proc %d slot size", proc, local_slot_size);
		last = first + local_slot_size - 1;
		mpc_common_nodebug("First %d last %d", first, last);

		if( (i >= first) && (i <= last) )
		{
			if( (cpu_nb % task_nb > j) && (cpu_nb > task_nb) )
			{
				*nbVp = cpu_per_task + 1;
			}
			else
			{
				*nbVp = cpu_per_task;
			}

			mpc_common_nodebug("mpc_thread_get_task_placement: Put task %d on VP %d", i, proc);
			return proc;
		}

		first = last + 1;
		j++;

		if( (cpu_nb % task_nb >= j) && (cpu_nb > task_nb) )
		{
			proc++;
		}
	}

	mpc_common_nodebug("mpc_thread_get_task_placement: (After loop) Put task %d on VP %d", i, proc);
	fflush(stdout);
	mpc_common_debug_abort();
	return proc;
}

int mpc_thread_get_task_placement_and_count_numa_packed(int i, int *nbVp)
{
	/*   int rank_in_node; */
	// declaration


	int proc;

	int cpu_nb;

	int nb_numa_node_per_node;

	// initialization
	cpu_nb = mpc_topology_get_pu_count();           // number of cpu per process
	nb_numa_node_per_node =
		mpc_topology_get_numa_node_count();     // number of numa nodes in the node
	// config
	int T = sctk_last_local - sctk_first_local + 1; // number of task per node
	int C = cpu_nb;                                 // number of cpu per node
	int N = nb_numa_node_per_node;                  // number of numa nodes
	int t = T % N;                                  // buffer task
	int cpu_per_numa_node = C / N;                  // cpu per numa node
	int Tnk;                                        // task per numa node
	// task already treated
	int tat = 0;
	*nbVp = 1;
	int k; // numa_node ID

	for(k = 0; k < N; k++)
	{
		// printf("for k=%d\n",k);
		// On ajoute une tache en plus aux premiers noeuds numas
		Tnk = T / N;

		if(k < t)
		{
			Tnk += 1;
			// printf("if [k=%d] < [t=%d] -> Tnk+=1 %d\n", k, t, Tnk);
		}

		// Si  tat <= i < Tnk+tat // est ce qu on est dans ce noeud numa la ?
		if(tat <= i && i < (Tnk + tat) )
		{
			// printf("if [tat=%d] <= [i=%d] < [Tnk=%d+tat=%d] %d\n", tat, i, Tnk,
			// tat, Tnk+tat);

			// update *nbVP
			if(k * (cpu_per_numa_node) + (i - tat) ==
			   k * (cpu_per_numa_node) + Tnk - 1)
			{
				*nbVp = (cpu_per_numa_node - Tnk) + 1;
			}

			// printf("proc=%d, *nbVp=%d\n", k*(cpu_per_numa_node) + (i-tat),*nbVp);
			// return numanode id * (C/N) + (i - tat)
			//      fixe premier cpu du noeud numa     + difference de la tache en
			//      cours
			return k * (cpu_per_numa_node) + (i - tat);
		}

		// Si nous avons une tache en plus
		// printf("else tat=%d += Tnk=%d -> %d\n", tat, Tnk, tat+Tnk);
		tat += Tnk;
	}

	//mpc_common_debug("mpc_thread_get_task_placement: (After loop) Put task %d on VP %d", i, proc);
	mpc_common_debug_abort();
	return proc;
}

int mpc_thread_get_task_placement_and_count_numa(int i, int *nbVp)
{
	int task_nb = mpc_common_get_local_task_count();
	int cpu_nb  = mpc_topology_get_pu_count(); // number of cpu per process
	int numa_node_per_node_nb = mpc_topology_get_numa_node_count();
	int cpu_per_numa_node     = cpu_nb / numa_node_per_node_nb;
	int global_id             = i;
	int numa_node_id          = (global_id * numa_node_per_node_nb) / task_nb;
	int local_id = mpc_common_get_local_task_rank();

	int task_per_numa_node =
		( ( (numa_node_id + 1) * task_nb + numa_node_per_node_nb - 1) /
		  numa_node_per_node_nb) -
		( ( (numa_node_id) * task_nb + numa_node_per_node_nb - 1) /
		  numa_node_per_node_nb);
	int proc_local  = (local_id * cpu_per_numa_node) / task_per_numa_node;
	int proc_global = proc_local + (numa_node_id * cpu_per_numa_node);
	// calculate nbVp value
	int proc_next_global;
	int proc_next_local;

	if(local_id < task_per_numa_node)
	{
		proc_next_global =
			( (local_id + 1) * cpu_per_numa_node) / task_per_numa_node;
		proc_next_local = proc_next_global + numa_node_id * cpu_per_numa_node;
		*nbVp           = proc_next_local - proc_global;
	}
	else
	{
		*nbVp = cpu_per_numa_node - proc_local;
	}

	if(*nbVp < 1)
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

int mpc_thread_get_task_placement_and_count(int i, int *nbVp)
{
	// return mpc_thread_get_task_placement_and_count_default (i, nbVp);
	// return mpc_thread_get_task_placement_and_count_numa (i, nbVp);
	// return mpc_thread_get_task_placement_and_count_numa_packed (i, nbVp);
	// function pointer to get the thread placement policy from the config
	int ( *thread_placement_policy )(int i, int *nbVp) = NULL;

	if(!strcmp(__thread_module_config.thread_layout, "default"))
	{
		thread_placement_policy = mpc_thread_get_task_placement_and_count_default;
	}else if(!strcmp(__thread_module_config.thread_layout, "numa"))
	{
		thread_placement_policy = mpc_thread_get_task_placement_and_count_numa;
	}else if(!strcmp(__thread_module_config.thread_layout, "numa_packed"))
	{
		thread_placement_policy = mpc_thread_get_task_placement_and_count_numa_packed;
	}
	else
	{
		bad_parameter("No such value '%s' for mpc.thread.common.layout\n"
		                       "Supported are 'default', 'numa' and 'numa_packed'", __thread_module_config.thread_layout);
	}

	assume(thread_placement_policy != NULL);
	return thread_placement_policy(i, nbVp);
}

int mpc_thread_get_task_placement(int i)
{
	int dummy;

	return mpc_thread_get_task_placement_and_count(i, &dummy);
}

static inline void __vp_placement_init_data(int *vp_start_thread, int *vp_thread_count)
{
	int total_tasks_number = mpc_common_get_task_count();
	int local_threads      = total_tasks_number / mpc_common_get_process_count();

	if(total_tasks_number % mpc_common_get_process_count() > mpc_common_get_process_rank() )
	{
		local_threads++;
	}

	int start_thread = local_threads * mpc_common_get_process_rank();

	if(total_tasks_number % mpc_common_get_process_count() <= mpc_common_get_process_rank() )
	{
		start_thread += total_tasks_number % mpc_common_get_process_count();
	}

	TODO("THIS GLOBAL IS UGLY");
	sctk_first_local = start_thread;
	sctk_last_local  = start_thread + local_threads - 1;
	*vp_start_thread = start_thread;
	*vp_thread_count = local_threads;
}

/**********************
* THREAD DATA GETTER *
**********************/

//FIXME excess elements in struct initializer
static sctk_thread_data_t sctk_main_datas = SCTK_THREAD_DATA_INIT;

mpc_thread_keys_t stck_task_data;

void _mpc_thread_data_init()
{
	mpc_thread_keys_create(&stck_task_data, NULL);
	mpc_common_nodebug("stck_task_data = %d", stck_task_data);
	_mpc_thread_data_set(&sctk_main_datas);
}

static void _mpc_thread_data_reset()
{
	sctk_thread_data_t *tmp;

	tmp = ( sctk_thread_data_t * )mpc_thread_getspecific(stck_task_data);

	if(tmp == NULL)
	{
		_mpc_thread_data_set(&sctk_main_datas);
	}
}

void _mpc_thread_data_set(sctk_thread_data_t *task_id)
{
	mpc_thread_setspecific(stck_task_data, ( void * )task_id);
}

sctk_thread_data_t *mpc_thread_data_get_disg(int no_disguise)
{
	sctk_thread_data_t *tmp;

	if(sctk_multithreading_initialised == 0)
	{
		tmp = &sctk_main_datas;
	}
	else
	{
		tmp = ( sctk_thread_data_t * )mpc_thread_getspecific(stck_task_data);
	}

	if(tmp)
	{
		if( (tmp->disguise.my_disguisement != NULL) && (no_disguise == 0) )
		{
			return tmp->disguise.my_disguisement;
		}
	}

	return tmp;
}

sctk_thread_data_t *mpc_thread_data_get()
{
	return mpc_thread_data_get_disg(0);
}

void *mpc_thread_mpi_data_get()
{
	sctk_thread_data_t *th = mpc_thread_data_get_disg(0);

	if(!th)
	{
		return NULL;
	}

	return th->mpi_per_thread;
}

void mpc_thread_mpi_data_set(void *mpi_thread_ctx)
{
	sctk_thread_data_t *th = mpc_thread_data_get_disg(0);

	if(!th)
	{
		return;
	}

	th->mpi_per_thread = mpi_thread_ctx;
}

struct mpc_thread_rank_info_s *mpc_thread_rank_info_get(void)
{
	sctk_thread_data_t *data = mpc_thread_data_get();

	if(!data)
	{
		return NULL;
	}

	return &data->mpi_task;
}

int mpc_thread_get_pu(void)
{
	sctk_thread_data_t *data = mpc_thread_data_get();

	if(!data)
	{
		return mpc_topology_get_current_cpu();
	}

	int ret = data->virtual_processor;

	if(ret < 0)
	{
		ret = mpc_topology_get_current_cpu();
	}

	return ret;
}

int mpc_thread_get_thread_id(void)
{
	sctk_thread_data_t *data = mpc_thread_data_get();

	if(!data)
	{
		return -1;
	}

	return data->user_thread;
}

void * mpc_thread_get_parent_mpi_task_ctx(void)
{
	sctk_thread_data_t *data = mpc_thread_data_get();

	if(!data)
	{
		return NULL;
	}

	return data->mpc_mpi_context_data;
}

/************
* DISGUISE *
************/


mpc_thread_mpi_disguise_t *mpc_thread_disguise_get()
{
	sctk_thread_data_t *th = mpc_thread_data_get_disg(1);

	assume(th != NULL);
	return &th->disguise;
}

int mpc_thread_disguise_set(struct sctk_thread_data_s *th_data, void *mpi_ctx)
{
	mpc_thread_mpi_disguise_t *d = mpc_thread_disguise_get();

	d->my_disguisement  = th_data;
	d->ctx_disguisement = mpi_ctx;

	return 0;
}

/****************
* TIMER THREAD *
****************/

volatile sctk_timer_t ___timer_thread_ticks   = 0;
volatile int          ___timer_thread_running = 1;

static void *___timer_thread_main(void *arg)
{
    assert(__thread_module_config.thread_timer_enabled);
	_mpc_thread_ethread_mxn_engine_init_kethread();
	assume(arg == NULL);

	while(___timer_thread_running)
	{
		kthread_usleep(__thread_module_config.thread_timer_interval);
		___timer_thread_ticks++;
	}

	return NULL;
}

void __timer_thread_start(void)
{
	/* Create the timer thread */
	mpc_thread_kthread_t timer_thread;

	_mpc_thread_kthread_create(&timer_thread, ___timer_thread_main, NULL);
}

void __timer_thread_end(void)
{
	___timer_thread_running = 0;
}

/*******************************
* THREAD CLEANUP PUSH AND POP *
*******************************/


static mpc_thread_keys_t ___thread_cleanup_callback_list_key;


void mpc_thread_cleanup_push(struct _sctk_thread_cleanup_buffer *__buffer,
                             void (*__routine)(void *), void *__arg)
{
	struct _sctk_thread_cleanup_buffer **__head;

	__buffer->__routine = __routine;
	__buffer->__arg     = __arg;
	__head = mpc_thread_getspecific(___thread_cleanup_callback_list_key);
	mpc_common_nodebug("%p %p %p", __buffer, __head, *__head);
	__buffer->next = *__head;
	*__head        = __buffer;
	mpc_common_nodebug("%p %p %p", __buffer, __head, *__head);
}

void mpc_thread_cleanup_pop(struct _sctk_thread_cleanup_buffer *__buffer,
                            int __execute)
{
	struct _sctk_thread_cleanup_buffer **__head;

	__head  = mpc_thread_getspecific(___thread_cleanup_callback_list_key);
	*__head = __buffer->next;

	if(__execute)
	{
		__buffer->__routine(__buffer->__arg);
	}
}

static inline void __init_thread_cleanup_callback_key(void)
{
	mpc_thread_keys_create(&___thread_cleanup_callback_list_key, (void (*)(void *) )NULL);
}

static inline void __set_thread_cleanup_callback_key(void)
{
	struct _sctk_thread_cleanup_buffer *ptr_cleanup = NULL;

	mpc_thread_setspecific(___thread_cleanup_callback_list_key, &ptr_cleanup);
}

/*****************************
* VIRTUAL PROCESSORS LAUNCH *
*****************************/

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
#pragma weak MPC_Process_hook
void  MPC_Process_hook(void)
{
	/*This function is used to intercept MPC process's creation when profiling*/
}

#endif

//TODO Je ne suis pas sur qu'on créé vraiment 4 page vides sur GH200 avec cet appel étant donnée qu'on fait des malloc de 16000
//L'autre question que je me pose est de savoir de combien de page on voulait vraiment avoir ?
static inline void __prepare_free_pages(void)
{
	void *p1, *p2, *p3, *p4;

	p1 = sctk_malloc(1024 * 16);
	p2 = sctk_malloc(1024 * 16);
	p3 = sctk_malloc(1024 * 16);
	p4 = sctk_malloc(1024 * 16);
	sctk_free(p1);
	sctk_free(p2);
	sctk_free(p3);
	sctk_free(p4);
}

static inline void __init_brk_for_task(void)
{
	size_t s;
	void * tmp;
	size_t start = 0;
	size_t size;

	sctk_enter_no_alloc_land();
	size = ( size_t )(SCTK_MAX_MEMORY_SIZE);
	tmp  = sctk_get_heap_start();
	mpc_common_nodebug("INIT ADRESS %p", tmp);
	s = ( size_t )tmp /*  + 1*1024*1024*1024 */;
	mpc_common_nodebug("Max allocation %luMo %lu",
	                   ( unsigned long )(size /
	                                     (1024 * 1024 * mpc_common_get_process_count() ) ), s);
	start = s;
	start = start / SCTK_MAX_MEMORY_OFFSET;
	start = start * SCTK_MAX_MEMORY_OFFSET;

	if(s > start)
	{
		start += SCTK_MAX_MEMORY_OFFSET;
	}

	tmp = ( void * )start;
	mpc_common_nodebug("INIT ADRESS REALIGNED %p", tmp);

	if(mpc_common_get_process_count() > 1)
	{
		sctk_mem_reset_heap( ( size_t )tmp, size);
	}

	mpc_common_nodebug("Heap start at %p (%p %p)", sctk_get_heap_start(),
	                   ( void * )s, tmp);
	sctk_leave_no_alloc_land();
}

void mpc_thread_spawn_mpi_tasks(void *(*mpi_task_start_func)(void *), void *arg)
{
	__init_thread_cleanup_callback_key();
	__set_thread_cleanup_callback_key();
	__prepare_free_pages();
	__init_brk_for_task();
	if (__thread_module_config.thread_timer_enabled)
        __timer_thread_start();
#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
	MPC_Process_hook();
#endif

	int start_thread  = -1;
	int local_threads = -1;
	__vp_placement_init_data(&start_thread, &local_threads);

	mpc_thread_t *threads = ( mpc_thread_t * )sctk_malloc(local_threads * sizeof(mpc_thread_t) );

	int i;
	int local_task_id_counter = 0;

	mpc_common_init_trigger("Before Starting VPs");

	for(i = start_thread; i < start_thread + local_threads; i++)
	{
		sctk_register_task(local_task_id_counter);
		_mpc_thread_create_vp(&(threads[local_task_id_counter]), NULL, mpi_task_start_func, arg, ( long )i, local_task_id_counter);
		local_task_id_counter++;
	}

	start_func_done = 1;

	/* Start Waiting for the end of  all VPs */

  //FIXME il me semble qu'on preferait faire un semaphose pour pas garder le thread dans les listes d'ordonnancement
	mpc_thread_wait_for_value_and_poll( ( int * )&sctk_current_local_tasks_nb, 0, NULL, NULL);

	sctk_multithreading_initialised = 0;
	__timer_thread_end();

	mpc_common_init_trigger("After Ending VPs");

	/* Make sure to call desctructors for main thread */
	sctk_tls_dtors_free(sctk_main_datas.dtors_head);
	sctk_free(threads);
}

/************************
* MPI CONTEXT HANDLING *
************************/

static struct mpc_mpi_cl_per_mpi_process_ctx_s *( *_mpc_thread_mpi_ctx_get_trampoline )(void) = NULL;

void mpc_thread_mpi_ctx_set(struct mpc_mpi_cl_per_mpi_process_ctx_s * (*trampoline)(void) )
{
	_mpc_thread_mpi_ctx_get_trampoline = trampoline;
}

static inline struct mpc_mpi_cl_per_mpi_process_ctx_s *mpc_thread_mpi_ctx_get()
{
	if(_mpc_thread_mpi_ctx_get_trampoline)
	{
		return _mpc_thread_mpi_ctx_get_trampoline();
	}

	return NULL;
}

/*********************************************************
* HERE BEGINS THE INTERFACE TO THE OTHER THREAD ENGINES *
*********************************************************/


/*******************
* THREAD CREATION *
*******************/

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
	int        init_cpu = mpc_topology_get_current_cpu();
	int        cpuset_len, i, nb_cpusets;
	cpu_set_t *cpuset;

	mpc_thread_get_task_placement_and_count(mpc_common_get_task_rank(), &cpuset_len);
	init_cpu   = mpc_topology_convert_logical_pu_to_os(init_cpu);
	nb_cpusets = 1;
	cpuset     = sctk_malloc(sizeof(cpu_set_t) * nb_cpusets);
	CPU_ZERO(cpuset);

	for(i = 0; i < cpuset_len; ++i)
	{
		CPU_SET(init_cpu + i, cpuset);
	}

	void *next = dlsym(RTLD_NEXT, "__tbb_init_for_mpc");

	if(next)
	{
		void ( *call )(cpu_set_t *, int) = (void (*)(cpu_set_t *, int) )next;
		call(cpuset, cpuset_len);
	}
	else
	{
		mpc_common_nodebug("Calling fake TBB Initializer");
	}
}

static inline void __tbb_finalize_for_mpc()
{
	void *next = dlsym(RTLD_NEXT, "__tbb_finalize_for_mpc");

	if(next)
	{
		void ( *call )() = (void (*)() )next;
		call();
	}
	else
	{
		mpc_common_nodebug("Calling fake TBB Finalizer");
	}
}


static void *___vp_thread_start_routine(sctk_thread_data_t *__arg)
{
	void *res;

	sctk_thread_data_t tmp = *__arg;

	sctk_free(__arg);

	assume_m(mpc_topology_get_current_cpu() == ( int )tmp.bind_to, "___vp_thread_start_routine Bad Pinning");

	//mark the given TLS as currant thread allocator
	sctk_set_tls(tmp.tls);

	//do NUMA migration if required
	sctk_alloc_posix_numa_migrate();
	__set_thread_cleanup_callback_key();

	// we do not have an MPI
	tmp.mpi_per_thread = NULL;
	// Set no disguise
	tmp.disguise.my_disguisement  = NULL;
	tmp.disguise.ctx_disguisement = NULL;
	tmp.virtual_processor         = mpc_topology_get_current_cpu();

	_mpc_thread_data_set(&tmp);
	sctk_thread_add(&tmp, mpc_thread_self() );


#if defined(MPC_USE_EXTLS)
	/* TLS INTIALIZATION */
	sctk_tls_init();
	sctk_call_dynamic_initializers();
#endif

	sctk_register_thread_initial(task_rank);

  //Here begin the MPI process (that can be a thread): a mpc_task !
  //mpc_threads_generic_kind_mask_add(KIND_MASK_MPI);

	sctk_tls_dtors_init(&(tmp.dtors_head) );

	__tbb_init_for_mpc();

	sctk_report_creation(mpc_thread_self() );

	mpc_common_init_trigger("VP Thread Start");

	res = tmp.__start_routine(tmp.__arg);

#if defined (MPC_OpenMP)
    mpc_omp_exit();
#endif

	mpc_common_init_trigger("VP Thread End");

	sctk_report_death(mpc_thread_self() );

	__tbb_finalize_for_mpc();

	sctk_tls_dtors_free(tmp.dtors_head);
	sctk_unregister_task(tmp.mpi_task.rank);

	sctk_thread_remove(&tmp);
	_mpc_thread_data_set(NULL);

	return res;
}

int _mpc_thread_create_vp(mpc_thread_t *restrict __threadp,
                          const mpc_thread_attr_t *restrict __attr,
                          void *(*__start_routine)(void *), void *restrict __arg,
                          long task_id, long local_task_id)
{
	int res;


	static unsigned int core = 0;

	/* We bind the parent thread to the vp where the child
	* will be created. We must bind before calling
	* __sctk_crete_thread_memory_area(). The child thread
	* will be bound to the same core after its creation */
	int new_binding = mpc_thread_get_task_placement(core);

	core++;
	mpc_common_debug_info("Task %d is bound to core %d", local_task_id, new_binding);
	int previous_binding = mpc_topology_bind_to_cpu(new_binding);

	assert(new_binding == mpc_topology_get_current_cpu() );

	struct sctk_alloc_chain *tls = __sctk_create_thread_memory_area();
	assume(tls != NULL);

	sctk_thread_data_t *tmp = ( sctk_thread_data_t * )__sctk_malloc(sizeof(sctk_thread_data_t), tls);
	assume(tmp != NULL);
	memset(tmp, 0, sizeof(sctk_thread_data_t));

	tmp->tls                 = tls;
	tmp->__arg               = __arg;
	tmp->__start_routine     = __start_routine;
	tmp->user_thread         = 0;
	tmp->bind_to             = new_binding;
	tmp->mpi_task.rank       = sctk_safe_cast_long_int(task_id);
	tmp->mpi_task.local_rank = sctk_safe_cast_long_int(local_task_id);
	tmp->mpc_mpi_context_data = NULL;



#ifdef MPC_USE_EXTLS
	extls_ctx_t * old_ctx = sctk_extls_storage;
	extls_ctx_t **cur_tx  = ( (extls_ctx_t **)sctk_get_ctx_addr() );
	*cur_tx = malloc(sizeof(extls_ctx_t) );
	extls_ctx_herit(old_ctx, *cur_tx, LEVEL_TASK);
	extls_ctx_restore(*cur_tx);
#ifndef MPC_DISABLE_HLS
	extls_ctx_bind(*cur_tx, tmp->bind_to);
#endif
#endif

	// create user thread
	assert(_funcptr_mpc_thread_create != NULL);
	res = _funcptr_mpc_thread_create(__threadp, __attr, (void *(*)(void *) )
	                                 ___vp_thread_start_routine,
	                                 ( void * )tmp);


	mpc_topology_render_notify(task_id);

	/* We reset the binding */
	{
		mpc_topology_bind_to_cpu(previous_binding);
#ifdef MPC_USE_EXTLS
		extls_ctx_restore(old_ctx);
#endif
	}
	sctk_check(res, 0);
	return res;
}

static inline void __thread_base_init(void)
{
#ifdef HAVE_HWLOC
	sctk_alloc_posix_mmsrc_numa_init_phase_numa();
#endif

	mpc_thread_tls = sctk_get_current_alloc_chain();
#ifdef MPC_Allocator
	assert(mpc_thread_tls != NULL);
#endif
#ifdef SCTK_CHECK_CODE_RETURN
	fprintf(stderr, "Thread librarie return code check enable!!\n");
#endif
	_mpc_thread_futex_context_init();
	/*Check all types */
}

static inline void __thread_engine_init(void)
{
	if(mpc_common_get_flags()->thread_library_init != NULL)
	{
		mpc_common_get_flags()->thread_library_init();
	}
	else
	{
		/* assume pthread */
		mpc_thread_pthread_engine_init();
	}
}

#ifdef MPC_USE_EXTLS
static void __extls_thread_init(void)
{
	extls_init();
	extls_set_context_storage_addr( (void *(*)(void) )sctk_get_ctx_addr);
}

static void __extls_thread_start(void)
{
	sctk_call_dynamic_initializers();
}

static void __extls_runtime_start(void)
{
	sctk_locate_dynamic_initializers();
}

static void __extls_runtime_end(void)
{
	extls_fini();
}

#endif


void mpc_thread_module_register() __attribute__( (constructor) );

void mpc_thread_module_register()
{
	MPC_INIT_CALL_ONLY_ONCE
	mpc_common_init_callback_register("Per Thread Init", "Allocator Numa Migrate", sctk_alloc_posix_numa_migrate, 0);

	mpc_common_init_callback_register("Base Runtime Init with Config", "Base init thread", __thread_base_init, 0);
	mpc_common_init_callback_register("Base Runtime Init with Config", "Thread engine init", __thread_engine_init, 1);

	mpc_common_init_callback_register("Config Sources", "Config for MPC_Threads", __init_thread_module_config, 10);

#ifdef MPC_USE_EXTLS
	mpc_common_init_callback_register("Base Runtime Init", "Initialize EXTLS", __extls_thread_init, 0);
	mpc_common_init_callback_register("Base Runtime Finalize", "Finalize EXTLS", __extls_runtime_end, 99);
	mpc_common_init_callback_register("Per Thread Init", "Dynamic Initializers", __extls_thread_start, 1);

	mpc_common_init_callback_register("Base Runtime Init Done", "Extls Runtime Init", __extls_runtime_start, 1);
#endif
}

static void *___nonvp_thread_start_routine(sctk_thread_data_t *__arg)
{
	void *res;
	sctk_thread_data_t tmp;

	memset(&tmp, 0, sizeof(sctk_thread_data_t) );

	while(start_func_done == 0)
	{
		mpc_thread_yield();
	}

	_mpc_thread_data_reset();

	/* FIXME Intel OMP: at some point, in pthread mode, the ptr_cleanup variable seems to
	 * be corrupted. */
	struct _sctk_thread_cleanup_buffer **ptr_cleanup = sctk_malloc(sizeof(struct _sctk_thread_cleanup_buffer *) );
	tmp = *__arg;
	sctk_set_tls(tmp.tls);
	*ptr_cleanup = NULL;
	mpc_thread_setspecific(___thread_cleanup_callback_list_key, ptr_cleanup);
	sctk_free(__arg);
	tmp.virtual_processor = mpc_topology_get_current_cpu();
	mpc_common_nodebug("%d on %d", tmp.task_id, tmp.virtual_processor);
	_mpc_thread_data_set(&tmp);
	sctk_thread_add(&tmp, mpc_thread_self() );
	/** ** **/
	sctk_report_creation(mpc_thread_self() );
	/** **/

  //mpc_threads_generic_kind_mask_add(KIND_MASK_PTHREAD);

	mpc_common_init_trigger("Per Thread Init");
	res = tmp.__start_routine(tmp.__arg);
	mpc_common_init_trigger("Per Thread Release");
	/** ** **/
	sctk_report_death(mpc_thread_self() );
	/** **/
	sctk_free(ptr_cleanup);
	sctk_thread_remove(&tmp);
	_mpc_thread_data_set(NULL);
	return res;
}

#if 0
static hwloc_topology_t topology_option_text;
#endif

int mpc_thread_core_thread_create(mpc_thread_t *restrict __threadp,
                                  const mpc_thread_attr_t *restrict __attr,
                                  void *(*__start_routine)(void *),
                                  void *restrict __arg)
{
	int res;
	sctk_thread_data_t *         tmp;
	sctk_thread_data_t *         tmp_father;
	struct sctk_alloc_chain *    tls;
	static mpc_common_spinlock_t lock = MPC_COMMON_SPINLOCK_INITIALIZER;
	int user_thread;

	mpc_common_spinlock_lock(&lock);
	sctk_nb_user_threads++;
	user_thread = sctk_nb_user_threads;
	mpc_common_spinlock_unlock(&lock);
	tls = __sctk_create_thread_memory_area();
	mpc_common_nodebug("create tls %p attr %p", tls, __attr);
	tmp = ( sctk_thread_data_t * )
	      __sctk_malloc(sizeof(sctk_thread_data_t), tls);
	memset(tmp, 0, sizeof(sctk_thread_data_t) );
	tmp_father               = mpc_thread_data_get();
	tmp->tls                 = tls;
	tmp->__arg               = __arg;
	tmp->__start_routine     = __start_routine;
	tmp->mpi_task.rank       = -1;
	tmp->mpi_task.local_rank = -1;
	tmp->user_thread         = user_thread;
	tmp->mpc_mpi_context_data         = mpc_thread_mpi_ctx_get();

	if(tmp_father)
	{
		tmp->mpi_task.rank       = tmp_father->mpi_task.rank;
		tmp->mpi_task.local_rank = tmp_father->mpi_task.local_rank;
	}

	mpc_common_nodebug("Create Thread with MPI rank %d", tmp->task_id);

	/* Bind the user thread to the whole process if we are using pthreads
	   as otherwise they are stuck to the VP's CPU */
	if(mpc_common_get_flags()->thread_library_init == mpc_thread_pthread_engine_init)
	{
		mpc_topology_bind_to_process_cpuset();
	}


#ifdef MPC_USE_EXTLS
	int scope_init;

	extls_ctx_t * old_ctx = sctk_extls_storage;
	extls_ctx_t **cur_tx  = ( (extls_ctx_t **)sctk_get_ctx_addr() );
	*cur_tx = calloc(1, sizeof(extls_ctx_t) );
	extls_ctx_herit(old_ctx, *cur_tx, LEVEL_THREAD);
	extls_ctx_restore(*cur_tx);
#ifndef MPC_DISABLE_HLS
	if(__attr)
	{
		mpc_thread_attr_getscope(__attr, &scope_init);
	}
	else
	{
		scope_init = SCTK_THREAD_SCOPE_PROCESS;    /* consider not a scope_system */
	}

	/* if the thread is bound and its scope is not SCOPE_SYSTEM */
	if(scope_init != SCTK_THREAD_SCOPE_SYSTEM)
	{
		extls_ctx_bind(*cur_tx, tmp->bind_to);
	}
#endif
#endif
	assert(_funcptr_mpc_thread_user_create != NULL);
	res = _funcptr_mpc_thread_user_create(__threadp, __attr,
	                                      (void *(*)(void *) )
	                                      ___nonvp_thread_start_routine,
	                                      ( void * )tmp);
	sctk_check(res, 0);
#ifdef MPC_USE_EXTLS
	extls_ctx_restore(old_ctx);
	#ifndef NO_INTERNAL_ASSERT
		if(__attr != NULL)
		{
			mpc_thread_attr_getscope(__attr, &scope_init);
			mpc_common_nodebug("Thread created with scope %d", scope_init);
		}
	#endif
#endif
	TODO("THIS CODE IS UGLY ! FIX TO REEENABLE");
#if 0
	/* option graphic placement */
	if(mpc_common_get_flags()->enable_topology_graphic_placement)
	{
		/* to be sure of __arg type to cast */
		if(mpc_common_get_task_rank() != -1)
		{
			struct mpc_omp_mvp_thread_args_s *temp = ( struct mpc_omp_mvp_thread_args_s * )__arg;
			int vp_local_processus = temp->target_vp;
			/* get os ind */
			int master = mpc_topology_render_get_current_binding();
			/* need the logical pu of the master from the total compute node topo computing with the os index */
			int master_logical = mpc_topology_render_get_logical_from_os_id(master);
			/* in the global scope of compute node topology the pu is*/
			int logical_pu = (master_logical + vp_local_processus);
			/* convert logical in os ind in topology_compute_node */
			int os_pu = mpc_topology_render_get_current_binding_from_logical(logical_pu);
			mpc_topology_render_lock();
			/* fill file to communicate between process of the same compute node */
			mpc_topology_render_create(os_pu, master, mpc_common_get_task_rank() );
			mpc_topology_render_unlock();
		}
	}

	/* option text placement */
	if(mpc_common_get_flags()->enable_topology_text_placement)
	{
		/* to be sure of __arg type to cast */
		if(mpc_common_get_task_rank() != -1)
		{
			struct mpc_omp_mvp_thread_args_s *temp = ( struct mpc_omp_mvp_thread_args_s * )__arg;
			int *                    tree_shape;
			mpc_omp_node_t *          root_node;
			mpc_omp_meta_tree_node_t *root = NULL;
			int rank      = temp->rank;
			int target_vp = temp->target_vp;
			root      = temp->array;
			root_node = ( mpc_omp_node_t * )root[0].user_pointer;
			int max_depth = root_node->tree_depth - 1;
			tree_shape = root_node->tree_base + 1;
			int        core_depth;
			static int done_init = 1;
			mpc_topology_render_lock();

			if(done_init)
			{
				hwloc_topology_init(&topology_option_text);
				hwloc_topology_load(topology_option_text);
				done_init = 0;
			}

			mpc_topology_render_unlock();

			if(mpc_common_get_flags()->enable_smt_capabilities)
			{
				core_depth = hwloc_get_type_depth(topology_option_text, HWLOC_OBJ_PU);
			}
			else
			{
				core_depth = hwloc_get_type_depth(topology_option_text, HWLOC_OBJ_CORE);
			}

			int *min_index = ( int * )malloc(sizeof(int) * MPCOMP_AFFINITY_NB);
			min_index = mpc_omp_tree_array_compute_thread_min_rank(tree_shape, max_depth, rank, core_depth);
			/* get os ind */
			int master = mpc_topology_render_get_current_binding();
			// need the logical pu of the master from the total compute node topo computin with the os index to use for origin */
			int master_logical = mpc_topology_render_get_logical_from_os_id(master);
			/* in the global compute node topology the processus is*/
			int logical_pu = (master_logical + target_vp);
			/* convert logical in os ind in topology_compute_node */
			int os_pu = mpc_topology_render_get_current_binding_from_logical(logical_pu);
			mpc_topology_render_lock();
			mpc_topology_render_text(os_pu, master, mpc_common_get_task_rank(), target_vp, 0, min_index, 0);
			mpc_topology_render_unlock();
			free(min_index);
		}
	}
#endif
	return res;
}

/********************************************
* THIS IS THE REDIRECTED PTHREAD INTERFACE *
********************************************/

/* Common runtime init check */
void  mpc_launch_init_runtime();

static inline void __check_mpc_initialized()
{
	static int init_done = 0;

	if(!init_done)
	{
		init_done = 1;
		mpc_launch_init_runtime();
	}
}

/************
* ACTIVITY *
************/

double mpc_thread_getactivity(int i)
{
	__check_mpc_initialized();
	double tmp;

	assert(_funcptr_mpc_thread_get_activity != NULL);
	tmp = _funcptr_mpc_thread_get_activity(i);
	mpc_common_nodebug("mpc_thread_get_activity %d %f", i, tmp);
	return tmp;
}

/***********
* AT EXIT *
***********/

/* At exit generic trampoline */

static int ( *___per_task_atexit_trampoline )(void (*func)(void) ) = NULL;

void mpc_thread_mpi_task_atexit(int (*trampoline)(void (*func)(void) ) )
{
	___per_task_atexit_trampoline = trampoline;
}

static inline int __per_mpi_task_atexit(void (*func)(void) )
{
	if(___per_task_atexit_trampoline)
	{
		return ( ___per_task_atexit_trampoline )(func);
	}

	/* No trampoline */
	return 1;
}

int mpc_thread_atexit(void (*function)(void) )
{
	__check_mpc_initialized();
	/* We may have a TASK context replacing the proces one */
	mpc_common_debug("Calling the MPC atexit function");
	int ret = __per_mpi_task_atexit(function);

	if(ret == 0)
	{
		/* We were in a task and managed to register ourselves */
		return ret;
	}

	/* It failed we may not be in a task then call libc */
	mpc_common_debug("Calling the default atexit function");
	/* We have no task level fallback to libc */
	return atexit(function);
}

/********
* EXIT *
********/

static inline void __run_cleanup_callbacks(struct _sctk_thread_cleanup_buffer **__buffer)
{
	if(__buffer != NULL)
	{
		mpc_common_nodebug("end %p %p", __buffer, *__buffer);

		if(*__buffer != NULL)
		{
			struct _sctk_thread_cleanup_buffer *tmp;
			tmp = *__buffer;

			while(tmp != NULL)
			{
				tmp->__routine(tmp->__arg);
				tmp = tmp->next;
			}
		}
	}
}

void _mpc_thread_exit_cleanup()
{
	sctk_thread_data_t *tmp;
	struct _sctk_thread_cleanup_buffer **__head;

	/** ** **/
	sctk_report_death(mpc_thread_self() );
	/** **/
	tmp = mpc_thread_data_get();
	sctk_thread_remove(tmp);
	__head = mpc_thread_getspecific(___thread_cleanup_callback_list_key);
	__run_cleanup_callbacks(__head);
	mpc_thread_setspecific(___thread_cleanup_callback_list_key, NULL);
	mpc_common_nodebug("%p", tmp);

	if(tmp != NULL)
	{
		mpc_common_nodebug("ici %p %d", tmp, tmp->task_id);
#ifdef MPC_Lowcomm
		if(tmp->mpi_task.rank >= 0 && tmp->user_thread == 0)
		{
			//mpc_common_nodebug ( "mpc_lowcomm_terminaison_barrier" );
			//mpc_lowcomm_terminaison_barrier ();
			//mpc_common_nodebug ( "mpc_lowcomm_terminaison_barrier done" );
			sctk_unregister_task(tmp->mpi_task.rank);
			sctk_net_send_task_end(tmp->mpi_task.rank, mpc_common_get_process_rank() );
		}
#endif
	}
}

void mpc_thread_exit(void *__retval)
{
	__check_mpc_initialized();
	_mpc_thread_exit_cleanup();
	assert(_funcptr_mpc_thread_exit != NULL);
	_funcptr_mpc_thread_exit(__retval);
}

/**********
* DETACH *
**********/

int mpc_thread_detach(mpc_thread_t __th)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_detach != NULL);
	res = _funcptr_mpc_thread_detach(__th);
	sctk_check(res, 0);
	return res;
}

/*********
* EQUAL *
*********/

int mpc_thread_equal(mpc_thread_t __thread1, mpc_thread_t __thread2)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_equal != NULL);
	res = _funcptr_mpc_thread_equal(__thread1, __thread2);
	sctk_check(res, 0);
	return res;
}

/********
* ONCE *
********/

int mpc_thread_once(mpc_thread_once_t *__once_control,
                    void (*__init_routine)(void) )
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_once != NULL);
	res = _funcptr_mpc_thread_once(__once_control, __init_routine);
	sctk_check(res, 0);
	return res;
}

/**********
* ATFORK *
**********/

int mpc_thread_atfork(void (*__prepare)(void), void (*__parent)(void),
                      void (*__child)(void) )
{
	/* OpenMP Compat (use by openmp to reinitialize itself on processus ofrking) */
	return pthread_atfork(__prepare, __parent, __child);
}

/**********
* CANCEL *
**********/

int mpc_thread_cancel(mpc_thread_t __cancelthread)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cancel != NULL);
	res = _funcptr_mpc_thread_cancel(__cancelthread);
	sctk_check(res, 0);
	mpc_thread_yield();
	return res;
}

void mpc_thread_testcancel(void)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
}

/*********
* YIELD *
*********/

int mpc_thread_yield(void)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_yield != NULL);
	return _funcptr_mpc_thread_yield();
}

/********
* JOIN *
********/

int mpc_thread_join(mpc_thread_t __th, void **__thread_return)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_join != NULL);
	res = _funcptr_mpc_thread_join(__th, __thread_return);
	sctk_check(res, 0);
	return res;
}

/********
* SELF *
********/

mpc_thread_t mpc_thread_self(void)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_self != NULL);
	return _funcptr_mpc_thread_self();
}

/*********************
* THREAD ATTRIBUTES *
*********************/

int mpc_thread_getattr_np(mpc_thread_t th, mpc_thread_attr_t *attr)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_getattr_np != NULL);
	return _funcptr_mpc_thread_getattr_np(th, attr);
}

int mpc_thread_attr_destroy(mpc_thread_attr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_destroy != NULL);
	res = _funcptr_mpc_thread_attr_destroy(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getdetachstate(const mpc_thread_attr_t *__attr,
                                   int *__detachstate)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getdetachstate != NULL);
	res = _funcptr_mpc_thread_attr_getdetachstate(__attr, __detachstate);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getguardsize(const mpc_thread_attr_t *restrict __attr,
                                 size_t *restrict __guardsize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getguardsize != NULL);
	res = _funcptr_mpc_thread_attr_getguardsize(__attr, __guardsize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getinheritsched(const mpc_thread_attr_t *
                                    restrict __attr, int *restrict __inherit)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getinheritsched != NULL);
	res = _funcptr_mpc_thread_attr_getinheritsched(__attr, __inherit);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getschedparam(const mpc_thread_attr_t *restrict __attr,
                                  struct sched_param *restrict __param)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getschedparam != NULL);
	res = _funcptr_mpc_thread_attr_getschedparam(__attr, __param);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getschedpolicy(const mpc_thread_attr_t *restrict __attr,
                                   int *restrict __policy)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getschedpolicy != NULL);
	res = _funcptr_mpc_thread_attr_getschedpolicy(__attr, __policy);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setaffinity_np(mpc_thread_t thread, size_t cpusetsize,
                              const mpc_cpu_set_t *cpuset)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setaffinity_np != NULL);
	res = _funcptr_mpc_thread_setaffinity_np(thread, cpusetsize, cpuset);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_getaffinity_np(mpc_thread_t thread, size_t cpusetsize,
                              mpc_cpu_set_t *cpuset)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_getaffinity_np != NULL);
	res = _funcptr_mpc_thread_getaffinity_np(thread, cpusetsize, cpuset);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getscope(const mpc_thread_attr_t *restrict __attr,
                             int *restrict __scope)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getscope != NULL);
	res = _funcptr_mpc_thread_attr_getscope(__attr, __scope);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getstackaddr(const mpc_thread_attr_t *restrict __attr,
                                 void **restrict __stackaddr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getstackaddr != NULL);
	res = _funcptr_mpc_thread_attr_getstackaddr(__attr, __stackaddr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getstack(const mpc_thread_attr_t *restrict __attr,
                             void **restrict __stackaddr,
                             size_t *restrict __stacksize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getstack != NULL);
	res = _funcptr_mpc_thread_attr_getstack(__attr, __stackaddr, __stacksize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getstacksize(const mpc_thread_attr_t *restrict __attr,
                                 size_t *restrict __stacksize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getstacksize != NULL);
	res = _funcptr_mpc_thread_attr_getstacksize(__attr, __stacksize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_init(mpc_thread_attr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_init != NULL);
	res = _funcptr_mpc_thread_attr_init(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setdetachstate(mpc_thread_attr_t *__attr,
                                   int __detachstate)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setdetachstate != NULL);
	res = _funcptr_mpc_thread_attr_setdetachstate(__attr, __detachstate);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setguardsize(mpc_thread_attr_t *__attr,
                                 size_t __guardsize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setguardsize != NULL);
	res = _funcptr_mpc_thread_attr_setguardsize(__attr, __guardsize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setinheritsched(mpc_thread_attr_t *__attr, int __inherit)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setinheritsched != NULL);
	res = _funcptr_mpc_thread_attr_setinheritsched(__attr, __inherit);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setschedparam(mpc_thread_attr_t *restrict __attr,
                                  const struct sched_param *restrict __param)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setschedparam != NULL);
	res = _funcptr_mpc_thread_attr_setschedparam(__attr, __param);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setschedpolicy(mpc_thread_attr_t *__attr, int __policy)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setschedpolicy != NULL);
	res = _funcptr_mpc_thread_attr_setschedpolicy(__attr, __policy);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setscope(mpc_thread_attr_t *__attr, int __scope)
{
	__check_mpc_initialized();
	int res;

	mpc_common_nodebug("thread attr_setscope %d == %d || %d", __scope,
	                   SCTK_THREAD_SCOPE_PROCESS, SCTK_THREAD_SCOPE_SYSTEM);
	assert(_funcptr_mpc_thread_attr_setscope != NULL);
	res = _funcptr_mpc_thread_attr_setscope(__attr, __scope);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setstackaddr(mpc_thread_attr_t *__attr, void *__stackaddr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setstackaddr != NULL);
	res = _funcptr_mpc_thread_attr_setstackaddr(__attr, __stackaddr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setstack(mpc_thread_attr_t *__attr, void *__stackaddr,
                             size_t __stacksize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setstack != NULL);
	res = _funcptr_mpc_thread_attr_setstack(__attr, __stackaddr, __stacksize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setstacksize(mpc_thread_attr_t *__attr,
                                 size_t __stacksize)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setstacksize != NULL);
	res = _funcptr_mpc_thread_attr_setstacksize(__attr, __stacksize);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_setbinding(mpc_thread_attr_t *__attr, int __binding)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_setbinding != NULL);
	res = _funcptr_mpc_thread_attr_setbinding(__attr, __binding);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_attr_getbinding(mpc_thread_attr_t *__attr, int *__binding)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_attr_getbinding != NULL);
	res = _funcptr_mpc_thread_attr_getbinding(__attr, __binding);
	sctk_check(res, 0);
	return res;
}

/***********************
* BARRIER  ATTRIBUTES *
***********************/

int mpc_thread_barrierattr_destroy(mpc_thread_barrierattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrierattr_destroy != NULL);
	res = _funcptr_mpc_thread_barrierattr_destroy(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_barrierattr_init(mpc_thread_barrierattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrierattr_init != NULL);
	res = _funcptr_mpc_thread_barrierattr_init(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_barrierattr_setpshared(mpc_thread_barrierattr_t *__attr,
                                      int __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrierattr_setpshared != NULL);
	res = _funcptr_mpc_thread_barrierattr_setpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_barrierattr_getpshared(const mpc_thread_barrierattr_t *
                                      __attr, int *__pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrierattr_getpshared != NULL);
	res = _funcptr_mpc_thread_barrierattr_getpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

/***********
* BARRIER *
***********/

int mpc_thread_core_barrier_destroy(mpc_thread_barrier_t *__barrier)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrier_destroy != NULL);
	res = _funcptr_mpc_thread_barrier_destroy(__barrier);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_core_barrier_init(mpc_thread_barrier_t *restrict __barrier,
                                 const mpc_thread_barrierattr_t *restrict __attr,
                                 unsigned int __count)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrier_init != NULL);
	res = _funcptr_mpc_thread_barrier_init(__barrier, __attr, __count);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_core_barrier_wait(mpc_thread_barrier_t *__barrier)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_barrier_wait != NULL);
	res = _funcptr_mpc_thread_barrier_wait(__barrier);
	sctk_check(res, 0);
	return res;
}

/****************************
* CONDITIONNALS ATTRIBUTES *
****************************/

int mpc_thread_condattr_destroy(mpc_thread_condattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_destroy != NULL);
	res = _funcptr_mpc_thread_condattr_destroy(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_condattr_getpshared(const mpc_thread_condattr_t *
                                   restrict __attr, int *restrict __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_getpshared != NULL);
	res = _funcptr_mpc_thread_condattr_getpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_condattr_init(mpc_thread_condattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_init != NULL);
	res = _funcptr_mpc_thread_condattr_init(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_condattr_setpshared(mpc_thread_condattr_t *__attr,
                                   int __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_setpshared != NULL);
	res = _funcptr_mpc_thread_condattr_setpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_condattr_setclock(mpc_thread_condattr_t *__attr,
                                 clockid_t __clockid)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_setclock != NULL);
	res = _funcptr_mpc_thread_condattr_setclock(__attr, __clockid);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_condattr_getclock(mpc_thread_condattr_t *__attr,
                                 clockid_t *__clockid)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_condattr_getclock != NULL);
	res = _funcptr_mpc_thread_condattr_getclock(__attr, __clockid);
	sctk_check(res, 0);
	return res;
}

/****************
* CONDITIONALS *
****************/

int mpc_thread_cond_broadcast(mpc_thread_cond_t *__cond)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_broadcast != NULL);
	res = _funcptr_mpc_thread_cond_broadcast(__cond);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_destroy(mpc_thread_cond_t *__cond)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_destroy != NULL);
	res = _funcptr_mpc_thread_cond_destroy(__cond);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_init(mpc_thread_cond_t *restrict __cond,
                         const mpc_thread_condattr_t *restrict __cond_attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_init != NULL);
	res = _funcptr_mpc_thread_cond_init(__cond, __cond_attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_signal(mpc_thread_cond_t *__cond)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_signal != NULL);
	res = _funcptr_mpc_thread_cond_signal(__cond);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_timedwait(mpc_thread_cond_t *restrict __cond,
                              mpc_thread_mutex_t *restrict __mutex,
                              const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_timedwait != NULL);
	res = _funcptr_mpc_thread_cond_timedwait(__cond, __mutex, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_clockwait(mpc_thread_cond_t *restrict __cond,
                              mpc_thread_mutex_t *restrict __mutex,
                              clockid_t __clock_id,
                              const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_clockwait != NULL);
	res = _funcptr_mpc_thread_cond_clockwait(__cond, __mutex, __clock_id, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_cond_wait(mpc_thread_cond_t *restrict __cond,
                         mpc_thread_mutex_t *restrict __mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_cond_wait != NULL);
	res = _funcptr_mpc_thread_cond_wait(__cond, __mutex);
	sctk_check(res, 0);
	return res;
}

/***********
* GETTERS *
***********/

int
mpc_thread_getconcurrency(void)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_getconcurrency != NULL);
	res = _funcptr_mpc_thread_getconcurrency();
	sctk_check(res, 0);
	return res;
}

int
mpc_thread_getcpuclockid(mpc_thread_t __thread_id, clockid_t *__clock_id)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_getcpuclockid != NULL);
	res = _funcptr_mpc_thread_getcpuclockid(__thread_id, __clock_id);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_getpriority_max(int policy)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sched_get_priority_max != NULL);
	res = _funcptr_mpc_thread_sched_get_priority_max(policy);
	return res;
}

int mpc_thread_getpriority_min(int policy)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sched_get_priority_min != NULL);
	res = _funcptr_mpc_thread_sched_get_priority_min(policy);
	return res;
}

int mpc_thread_getschedparam(mpc_thread_t __target_thread,
                             int *restrict __policy,
                             struct sched_param *restrict __param)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_getschedparam != NULL);
	res = _funcptr_mpc_thread_getschedparam(__target_thread, __policy, __param);
	sctk_check(res, 0);
	return res;
}

/********
* KILL *
********/

int mpc_thread_kill(mpc_thread_t thread, int signo)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_kill != NULL);
	res = _funcptr_mpc_thread_kill(thread, signo);
	sctk_check(res, 0);
	mpc_thread_yield();
	return res;
}

int mpc_thread_process_kill(pid_t pid, int sig)
{
	__check_mpc_initialized();
#ifndef WINDOWS_SYS
	int res;
	res = kill(pid, sig);
	sctk_check(res, 0);
	mpc_thread_yield();
	return res;
#else
	not_available();
	return 1;
#endif
}

/***********
* SIGNALS *
***********/

int mpc_thread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sigmask != NULL);
	res = _funcptr_mpc_thread_sigmask(how, newmask, oldmask);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sigwait(const sigset_t *set, int *sig)
{
	__check_mpc_initialized();
	not_implemented();
	assume(set == NULL);
	assume(sig == NULL);
	return 0;
}

int mpc_thread_sigpending(sigset_t *set)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sigpending != NULL);
	res = _funcptr_mpc_thread_sigpending(set);
	sctk_check(res, 0);
	mpc_thread_yield();
	return res;
}

int mpc_thread_sigsuspend(sigset_t *set)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sigsuspend != NULL);
	res = _funcptr_mpc_thread_sigsuspend(set);
	sctk_check(res, 0);
	mpc_thread_yield();
	return res;
}

/*int
* sctk_thread_sigaction( int signum, const struct sigaction* act,
*              struct sigaction* oldact ){
* int res;
*  assert(_funcptr_mpc_thread_sigaction != NULL);
* res = _funcptr_mpc_thread_sigaction( signum, act, oldact );
* return res;
* }*/

/********
* KEYS *
********/

int mpc_thread_keys_create(mpc_thread_keys_t *__key,
                           void (*__destr_function)(void *) )
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_key_create != NULL);
	res = _funcptr_mpc_thread_key_create(__key, __destr_function);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_keys_delete(mpc_thread_keys_t __key)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_key_delete != NULL);
	res = _funcptr_mpc_thread_key_delete(__key);
	sctk_check(res, 0);
	return res;
}

void *mpc_thread_getspecific(mpc_thread_keys_t __key)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_getspecific != NULL);
	return _funcptr_mpc_thread_getspecific(__key);
}

/********************
* MUTEX ATTRIBUTES *
********************/

int mpc_thread_mutexattr_destroy(mpc_thread_mutexattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_destroy != NULL);
	res = _funcptr_mpc_thread_mutexattr_destroy(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_getpshared(const mpc_thread_mutexattr_t *
                                    restrict __attr, int *restrict __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_getpshared != NULL);
	res = _funcptr_mpc_thread_mutexattr_getpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_getprioceiling(const mpc_thread_mutexattr_t *
                                        attr, int *prioceiling)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_getprioceiling != NULL);
	res = _funcptr_mpc_thread_mutexattr_getprioceiling(attr, prioceiling);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_setprioceiling(mpc_thread_mutexattr_t *attr,
                                        int prioceiling)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_setprioceiling != NULL);
	res = _funcptr_mpc_thread_mutexattr_setprioceiling(attr, prioceiling);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_getprotocol(const mpc_thread_mutexattr_t *
                                     attr, int *protocol)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_getprotocol != NULL);
	res = _funcptr_mpc_thread_mutexattr_getprotocol(attr, protocol);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_setprotocol(mpc_thread_mutexattr_t *attr,
                                     int protocol)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_setprotocol != NULL);
	res = _funcptr_mpc_thread_mutexattr_setprotocol(attr, protocol);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_gettype(const mpc_thread_mutexattr_t *
                                 restrict __attr, int *restrict __kind)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_gettype != NULL);
	res = _funcptr_mpc_thread_mutexattr_gettype(__attr, __kind);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_init(mpc_thread_mutexattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_init != NULL);
	res = _funcptr_mpc_thread_mutexattr_init(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_setpshared(mpc_thread_mutexattr_t *__attr,
                                    int __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_setpshared != NULL);
	res = _funcptr_mpc_thread_mutexattr_setpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutexattr_settype(mpc_thread_mutexattr_t *__attr, int __kind)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutexattr_settype != NULL);
	res = _funcptr_mpc_thread_mutexattr_settype(__attr, __kind);
	sctk_check(res, 0);
	return res;
}

/*********
* MUTEX *
*********/

int mpc_thread_mutex_destroy(mpc_thread_mutex_t *__mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_destroy != NULL);
	res = _funcptr_mpc_thread_mutex_destroy(__mutex);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_init(mpc_thread_mutex_t *restrict __mutex,
                          const mpc_thread_mutexattr_t *restrict __mutex_attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_init != NULL);
	res = _funcptr_mpc_thread_mutex_init(__mutex, __mutex_attr);
	sctk_check(res, 0);
	return res;
}

#pragma weak user_mpc_thread_mutex_lock=mpc_thread_mutex_lock
int mpc_thread_mutex_lock(mpc_thread_mutex_t *__mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_lock != NULL);
	res = _funcptr_mpc_thread_mutex_lock(__mutex);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_spinlock(mpc_thread_mutex_t *__mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_spinlock != NULL);
	res = _funcptr_mpc_thread_mutex_spinlock(__mutex);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_timedlock(mpc_thread_mutex_t *restrict __mutex,
                               const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_timedlock != NULL);
	res = _funcptr_mpc_thread_mutex_timedlock(__mutex, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_clocklock(mpc_thread_mutex_t *restrict __mutex,
                               clockid_t __clock_id,
                               const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_clocklock != NULL);
	res = _funcptr_mpc_thread_mutex_clocklock(__mutex, __clock_id, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_trylock(mpc_thread_mutex_t *__mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_trylock != NULL);
	res = _funcptr_mpc_thread_mutex_trylock(__mutex);
	sctk_check(res, 0);
	return res;
}

#pragma weak user_mpc_thread_mutex_unlock=mpc_thread_mutex_unlock
int mpc_thread_mutex_unlock(mpc_thread_mutex_t *__mutex)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_mutex_unlock != NULL);
	res = _funcptr_mpc_thread_mutex_unlock(__mutex);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_mutex_getprioceiling(__UNUSED__ const mpc_thread_mutex_t *restrict mutex,
                                    __UNUSED__ int *restrict prioceiling)
{
	__check_mpc_initialized();
	int res = 0;

	not_implemented();
	return res;
}

int mpc_thread_mutex_setprioceiling(__UNUSED__ mpc_thread_mutex_t *restrict mutex,
                                    __UNUSED__ int prioceiling, __UNUSED__ int *restrict old_ceiling)
{
	__check_mpc_initialized();
	int res = 0;
	not_implemented();
	return res;
}

/**************
* SEMAPHORES *
**************/

int mpc_thread_sem_init(mpc_thread_sem_t *sem, int pshared,
                        unsigned int value)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_init != NULL);
	res = _funcptr_mpc_thread_sem_init(sem, pshared, value);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sem_wait(mpc_thread_sem_t *sem)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_wait != NULL);
	res = _funcptr_mpc_thread_sem_wait(sem);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sem_trywait(mpc_thread_sem_t *sem)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_trywait != NULL);
	res = _funcptr_mpc_thread_sem_trywait(sem);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sem_post(mpc_thread_sem_t *sem)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_post != NULL);
	res = _funcptr_mpc_thread_sem_post(sem);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sem_getvalue(mpc_thread_sem_t *sem, int *sval)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_getvalue != NULL);
	res = _funcptr_mpc_thread_sem_getvalue(sem, sval);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_sem_destroy(mpc_thread_sem_t *sem)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_sem_destroy != NULL);
	res = _funcptr_mpc_thread_sem_destroy(sem);
	sctk_check(res, 0);
	return res;
}

mpc_thread_sem_t *mpc_thread_sem_open(const char *__name, int __oflag, ...)
{
	__check_mpc_initialized();
	mpc_thread_sem_t *tmp;

	if( (__oflag & O_CREAT) )
	{
		va_list ap;
		int     value;
		mode_t  mode;
		va_start(ap, __oflag);
		mode  = va_arg(ap, mode_t);
		value = va_arg(ap, int);
		va_end(ap);
		assert(_funcptr_mpc_thread_sem_open != NULL);
		tmp = _funcptr_mpc_thread_sem_open(__name, __oflag, mode, value);
	}
	else
	{
		assert(_funcptr_mpc_thread_sem_open != NULL);
		tmp = _funcptr_mpc_thread_sem_open(__name, __oflag);
	}

	return tmp;
}

int mpc_thread_sem_close(mpc_thread_sem_t *__sem)
{
	__check_mpc_initialized();
	int res = 0;

	assert(_funcptr_mpc_thread_sem_close != NULL);
	res = _funcptr_mpc_thread_sem_close(__sem);
	return res;
}

int mpc_thread_sem_unlink(const char *__name)
{
	__check_mpc_initialized();
	int res = 0;

	assert(_funcptr_mpc_thread_sem_unlink != NULL);
	res = _funcptr_mpc_thread_sem_unlink(__name);
	return res;
}

int mpc_thread_sem_timedwait(mpc_thread_sem_t *__sem,
                             const struct timespec *__abstime)
{
	__check_mpc_initialized();
	int res = 0;

	assert(_funcptr_mpc_thread_sem_timedwait != NULL);
	res = _funcptr_mpc_thread_sem_timedwait(__sem, __abstime);
	return res;
}

/***************
* RWLOCK ATTR *
***************/

int mpc_thread_rwlockattr_init(mpc_thread_rwlockattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlockattr_init != NULL);
	res = _funcptr_mpc_thread_rwlockattr_init(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlockattr_destroy(mpc_thread_rwlockattr_t *__attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlockattr_destroy != NULL);
	res = _funcptr_mpc_thread_rwlockattr_destroy(__attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlockattr_getpshared(const mpc_thread_rwlockattr_t *
                                     restrict __attr, int *restrict __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlockattr_getpshared != NULL);
	res = _funcptr_mpc_thread_rwlockattr_getpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlockattr_setpshared(mpc_thread_rwlockattr_t *__attr,
                                     int __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlockattr_setpshared != NULL);
	res = _funcptr_mpc_thread_rwlockattr_setpshared(__attr, __pshared);
	sctk_check(res, 0);
	return res;
}

/********************
* READ WRITE LOCKS *
********************/

int mpc_thread_rwlock_destroy(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_destroy != NULL);
	res = _funcptr_mpc_thread_rwlock_destroy(__rwlock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_init(mpc_thread_rwlock_t *restrict __rwlock,
                           const mpc_thread_rwlockattr_t *restrict __attr)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_init != NULL);
	res = _funcptr_mpc_thread_rwlock_init(__rwlock, __attr);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_rdlock(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_rdlock != NULL);
	res = _funcptr_mpc_thread_rwlock_rdlock(__rwlock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_timedrdlock(mpc_thread_rwlock_t *restrict __rwlock,
                                  const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_timedrdlock != NULL);
	res = _funcptr_mpc_thread_rwlock_timedrdlock(__rwlock, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_timedwrlock(mpc_thread_rwlock_t *restrict __rwlock,
                                  const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_timedwrlock != NULL);
	res = _funcptr_mpc_thread_rwlock_timedwrlock(__rwlock, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_clockrdlock(mpc_thread_rwlock_t *restrict __rwlock,
                                  clockid_t __clock_id,
                                  const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_clockrdlock != NULL);
	res = _funcptr_mpc_thread_rwlock_clockrdlock(__rwlock, __clock_id, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_clockwrlock(mpc_thread_rwlock_t *restrict __rwlock,
                                  clockid_t __clock_id,
                                  const struct timespec *restrict __abstime)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_clockwrlock != NULL);
	res = _funcptr_mpc_thread_rwlock_clockwrlock(__rwlock, __clock_id, __abstime);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_tryrdlock(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_tryrdlock != NULL);
	res = _funcptr_mpc_thread_rwlock_tryrdlock(__rwlock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_trywrlock(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_trywrlock != NULL);
	res = _funcptr_mpc_thread_rwlock_trywrlock(__rwlock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_unlock(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_unlock != NULL);
	res = _funcptr_mpc_thread_rwlock_unlock(__rwlock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_rwlock_wrlock(mpc_thread_rwlock_t *__rwlock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_rwlock_wrlock != NULL);
	res = _funcptr_mpc_thread_rwlock_wrlock(__rwlock);
	sctk_check(res, 0);
	return res;
}

/***********
* SETTERS *
***********/

int mpc_thread_setcancelstate(int __state, int *__oldstate)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setcancelstate != NULL);
	res = _funcptr_mpc_thread_setcancelstate(__state, __oldstate);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setcanceltype(int __type, int *__oldtype)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setcanceltype != NULL);
	res = _funcptr_mpc_thread_setcanceltype(__type, __oldtype);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setconcurrency(int __level)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setconcurrency != NULL);
	res = _funcptr_mpc_thread_setconcurrency(__level);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setschedprio(mpc_thread_t __p, int __i)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setschedprio != NULL);
	res = _funcptr_mpc_thread_setschedprio(__p, __i);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setschedparam(mpc_thread_t __target_thread, int __policy,
                             const struct sched_param *__param)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setschedparam != NULL);
	res = _funcptr_mpc_thread_setschedparam(__target_thread, __policy, __param);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_setspecific(mpc_thread_keys_t __key, const void *__pointer)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_setspecific != NULL);
	res = _funcptr_mpc_thread_setspecific(__key, __pointer);
	sctk_check(res, 0);
	return res;
}

/*************
* SPINLOCKS *
*************/

int mpc_thread_spin_destroy(mpc_thread_spinlock_t *__lock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_spin_destroy != NULL);
	res = _funcptr_mpc_thread_spin_destroy(__lock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_spin_init(mpc_thread_spinlock_t *__lock, int __pshared)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_spin_init != NULL);
	res = _funcptr_mpc_thread_spin_init(__lock, __pshared);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_spin_lock(mpc_thread_spinlock_t *__lock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_spin_lock != NULL);
	res = _funcptr_mpc_thread_spin_lock(__lock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_spin_trylock(mpc_thread_spinlock_t *__lock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_spin_trylock != NULL);
	res = _funcptr_mpc_thread_spin_trylock(__lock);
	sctk_check(res, 0);
	return res;
}

int mpc_thread_spin_unlock(mpc_thread_spinlock_t *__lock)
{
	__check_mpc_initialized();
	int res;

	assert(_funcptr_mpc_thread_spin_unlock != NULL);
	res = _funcptr_mpc_thread_spin_unlock(__lock);
	sctk_check(res, 0);
	return res;
}

/*********
* SLEEP *
*********/

typedef struct _mpc_thread_core_sleep_pool_s
{
	sctk_timer_t wake_time;
	int          done;
} _mpc_thread_core_sleep_pool_t;

static void __sleep_pool_poll(_mpc_thread_core_sleep_pool_t *wake_time)
{
	assert(__thread_module_config.thread_timer_enabled);
	if(wake_time->wake_time < ___timer_thread_ticks)
	{
		wake_time->done = 0;
	}
}

unsigned int mpc_thread_sleep(unsigned int seconds)
{
	__check_mpc_initialized();
	assert(__thread_module_config.thread_timer_enabled);
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	_mpc_thread_core_sleep_pool_t wake_time;
	wake_time.done      = 1;
	wake_time.wake_time =
		( ( ( sctk_timer_t )seconds * ( sctk_timer_t )1000) /
		  ( sctk_timer_t )__thread_module_config.thread_timer_interval) + ___timer_thread_ticks + 1;
	mpc_thread_yield();
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	mpc_thread_wait_for_value_and_poll(&(wake_time.done), 0,
	                                   (void (*)(void *) )
	                                   __sleep_pool_poll,
	                                   ( void * )&wake_time);
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	return 0;
}

int mpc_thread_usleep(unsigned int useconds)
{
	__check_mpc_initialized();
	assert(__thread_module_config.thread_timer_enabled);
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	_mpc_thread_core_sleep_pool_t wake_time;
	wake_time.done      = 1;
	wake_time.wake_time =
		( ( ( sctk_timer_t )useconds / ( sctk_timer_t )1000) /
		  ( sctk_timer_t )__thread_module_config.thread_timer_interval) + ___timer_thread_ticks + 1;
	mpc_thread_yield();
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	mpc_thread_wait_for_value_and_poll(&(wake_time.done), 0,
	                                   (void (*)(void *) )
	                                   __sleep_pool_poll,
	                                   ( void * )&wake_time);
	assert(_funcptr_mpc_thread_testcancel != NULL);
	_funcptr_mpc_thread_testcancel();
	return 0;
}

/*on ne prend pas en compte la precision en dessous de la micro-seconde
 * on ne gere pas les interruptions non plus*/
int mpc_thread_nanosleep(const struct timespec *req, struct timespec *rem)
{
	__check_mpc_initialized();

	if(req == NULL)
	{
		return EINVAL;
	}

	if( (req->tv_sec < 0) || (req->tv_nsec < 0) || (req->tv_nsec > 999999999) )
	{
		errno = EINVAL;
		return -1;
	}

	mpc_thread_sleep(req->tv_sec);
	mpc_thread_usleep(req->tv_nsec / 1000);

	if(rem != NULL)
	{
		rem->tv_sec  = 0;
		rem->tv_nsec = 0;
	}

	return 0;
}

/*********************
* MIGRATION SUPPORT *
*********************/

int mpc_thread_migrate(void)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_migrate != NULL);
	return _funcptr_mpc_thread_migrate();
}

int mpc_thread_migrate_to_core(const int cpu)
{
	__check_mpc_initialized();
	int tmp;
	sctk_thread_data_t *tmp_data;

	tmp_data = mpc_thread_data_get();
	/*   assert(tmp_data != NULL); */
	assert(_funcptr_mpc_thread_proc_migration != NULL);
	tmp = _funcptr_mpc_thread_proc_migration(cpu);

	if(0 <= tmp)
	{
		tmp_data->virtual_processor = mpc_topology_get_current_cpu();
	}

	return tmp;
}

/****************
* DUMP SUPPORT *
****************/

int mpc_thread_dump(char *file)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_dump != NULL);
	return _funcptr_mpc_thread_dump(file);
}

int mpc_thread_dump_restore(mpc_thread_t thread, char *type, int vp)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_restore != NULL);
	return _funcptr_mpc_thread_restore(thread, type, vp);
}

int mpc_thread_dump_clean(void)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_dump_clean != NULL);
	return _funcptr_mpc_thread_dump_clean();
}

/********************
* FREEZING SUPPORT *
********************/

void mpc_thread_freeze(mpc_thread_mutex_t *lock, void **list)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_freeze_thread_on_vp != NULL);
	_funcptr_mpc_thread_freeze_thread_on_vp(lock, list);
}

void mpc_thread_wake(void **list)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_wake_thread_on_vp != NULL);
	_funcptr_mpc_thread_wake_thread_on_vp(list);
}

/***************************
* WAIT FOR VALUE AND POLL *
***************************/

void mpc_thread_wait_for_value_and_poll(volatile int *data, int value,
                                        void (*func)(void *), void *arg)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_wait_for_value_and_poll != NULL);
	_funcptr_mpc_thread_wait_for_value_and_poll(data, value, func, arg);
}

int mpc_thread_timed_wait_for_value(volatile int *data, int value, unsigned int max_time_in_usec)
{
	__check_mpc_initialized();
	assert(__thread_module_config.thread_timer_enabled);
	unsigned int end_time = ( ( ( sctk_timer_t )max_time_in_usec / ( sctk_timer_t )1000) /
	                          ( sctk_timer_t )__thread_module_config.thread_timer_interval) + ___timer_thread_ticks + 1;
	unsigned int trials = 0;

	while(*data != value)
	{
		if(end_time < ___timer_thread_ticks)
		{
			/* TIMED OUT */
			return 1;
		}

		int left_to_wait = end_time - ___timer_thread_ticks;

		if( (30 < trials) && (5000 < left_to_wait) )
		{
			mpc_thread_usleep(1000);
		}
		else
		{
			mpc_thread_yield();
		}

		trials++;
	}

	return 0;
}

void mpc_thread_wait_for_value(volatile int *data, int value)
{
	__check_mpc_initialized();
	assert(_funcptr_mpc_thread_wait_for_value_and_poll != NULL);
	_funcptr_mpc_thread_wait_for_value_and_poll(data, value, NULL, NULL);
}

void mpc_thread_kernel_wait_for_value_and_poll(int *data, int value,
                                               void (*func)(void *), void *arg)
{
	__check_mpc_initialized();
	while( (*data) != value)
	{
		if(func != NULL)
		{
			func(arg);
		}

		kthread_usleep(500);
	}
}

/** This overrides a weak function in EXTLS to redirect WFV to MPC
 * as extls is otherwise compiled against pthread */
void extls_wait_for_value(volatile int* addr_val, int threshold)
{
	mpc_thread_wait_for_value_and_poll( (int*)addr_val, threshold, NULL, NULL);
}

/***********
* FUTEXES *
***********/

long  mpc_thread_futex(__UNUSED__ int sysop, void *addr1, int op, int val1,
                       struct timespec *timeout, void *addr2, int val2)
{
	__check_mpc_initialized();
	_mpc_thread_futex_context_init();

	if(_funcptr_mpc_thread_futex == NULL)
	{
		_funcptr_mpc_thread_futex = _mpc_thread_futex;
	}

	return _funcptr_mpc_thread_futex(addr1, op, val1, timeout, addr2, val2);
}

long mpc_thread_futex_with_vaargs(int sysop, ...)
{
	va_list          ap;
	void *           addr1, *addr2;
	int              op, val1, val2;
	struct timespec *timeout;

	va_start(ap, sysop);
	addr1   = va_arg(ap, void *);
	op      = va_arg(ap, int);
	val1    = va_arg(ap, int);
	timeout = va_arg(ap, struct timespec *);
	addr2   = va_arg(ap, void *);
	val2    = va_arg(ap, int);
	va_end(ap);

	return mpc_thread_futex(sysop, &addr1, op, val1, timeout, addr2, val2);
}

/*********************
* ATOMIC ADD HELPER *
*********************/

unsigned long mpc_thread_atomic_add(volatile unsigned long *ptr, unsigned long val)
{
	unsigned long             tmp;
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

	mpc_thread_mutex_lock(&lock);
	tmp  = *ptr;
	*ptr = tmp + val;
	mpc_thread_mutex_unlock(&lock);
	return tmp;
}
