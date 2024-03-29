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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */


#include <mpc_common_helper.h>
#include <mpc_common_flags.h>
#include <mpc_thread.h>
#include <mpc_common_profiler.h>

#include "microthread.h"
#include "sctk_alloc.h"
#include "sctk_context.h"
#include "mpc_thread.h"
#include "mpc_topology.h"
#include "mpc_common_debug.h"

static mpc_thread_once_t sctk_microthread_key_is_initialized = PTHREAD_ONCE_INIT;

/* Key corresponding to the main micro-thread instance structure */
mpc_thread_keys_t sctk_microthread_key;


/* Function initializing a 'microthread_vp' structure */
static inline void sctk_microthread_init_microthread_vp_t
        (sctk_microthread_vp_t *self)
{
	/* Nothing to do */
	self->to_do_list      = 0;
	self->to_do_list_next = 0;
	/* This vp is ready to accept microthreads */
	self->enable = 1;

	self->to_run = 0;

	/* TODO remove this part (should be replaced by 'levels') */
	self->barrier      = 0;
	self->barrier_done = 0;
}

/* Scheduler for the micro VP 'self'
 * Basically, it iterates through the 'to_do' list and execute/schedule each of
 * the micro-thread.
 */
static inline void
sctk_microthread_scheduler(sctk_microthread_vp_t *self)
{
	long i;
	long to_do;

	/* Grab the number of operations to perform */
	to_do = self->to_do_list;

	/* Iterate on the TO DO list and call the corresponding function */
	for(i = 0; i < to_do; i++)
	{
		mpc_common_nodebug("sctk_microthread_scheduler: BEGIN func %d", i);

		/* Execute the i-th function */
		self->op_list[i].func( (void *)self->op_list[i].arg);

		mpc_common_nodebug("sctk_microthread_scheduler: END func %d", i);
	}

	mpc_common_nodebug("sctk_microthread_scheduler: exit scheduler (vp=%p)", self);

	/* The list is now empty */
	self->to_do_list = 0;
}

/* Main function for every slave micro-vp.
 * This slave micro-vp dies when exiting this function
 */
static void *
sctk_microthread_slave_vp(void *arg)
{
	sctk_microthread_vp_t *self;

	mpc_common_nodebug("sctk_microthread_slave_vp: START");

	/* Grab the information on the VP */
	self = (sctk_microthread_vp_t *)arg;

	/* While this VP is available */
	while(self->enable)
	{
		mpc_thread_wait_for_value_and_poll( (int *)&(self->to_run), 1, NULL, NULL);
		self->to_run = 0;
		sctk_microthread_scheduler(self);
#if 0
		/* Is there anything to do? */
		if(self->to_do_list)
		{
			/* Run the microthread scheduler */
			mpc_common_nodebug
			        ("sctk_microthread_slave_vp: running the scheduler (vp=%p)",
			        self);
			sctk_microthread_scheduler(self);
		}
		else
		{
			/* Fall back to the main scheduler */
			mpc_thread_yield();
		}
#endif
	}

	mpc_common_nodebug("sctk_microthread_slave_vp: END");

	return NULL;
}

/* Add an operation to perform by the micro-VP 'vp' */
int
sctk_microthread_add_task(void *(*func)(void *), void *arg,
                          long vp, long *step, sctk_microthread_t *task,
                          int val)
{
	sctk_microthread_vp_t *self;
	long i;

	mpc_common_nodebug("sctk_microthread_add_task: VP=%d", vp);

	/* Grab the information on the currently scheduled VP */
	self = &(task->__list[vp]);

	/* Enqueue in the 'next' to-do list */
	i = self->to_do_list_next;
	self->op_list[i].func = MPC_MICROTHREAD_FUNC_T func;
	self->op_list[i].arg  = arg;
	*step = i;

	i++;

	/*
	 * i -> If the number of op is known ('val' param), switch the 'to-do' to
	 * re-activate the thread execution
	 * MPC_MICROTHREAD_LAST_TASK -> idem if this add task is the last one
	 */
	if( (val == i) || (val == MPC_MICROTHREAD_LAST_TASK) )
	{
		mpc_common_nodebug("sctk_microthread_add_task: to_do switch forced");

		self->to_do_list_next = 0;
		self->to_do_list      = i;
		self->to_run          = 1;
	}
	else
	{
		mpc_common_nodebug("sctk_microthread_add_task: no to_do switch = %d", i);

		self->to_do_list_next = i;
	}

	return 0;
}

static void *
sctk_microthread_first_task(void *arg)
{
	sctk_microthread_t *self = (sctk_microthread_t *)arg;

	assert(self != NULL);

	assume(mpc_thread_setspecific(sctk_microthread_key, self) == 0);

	return NULL;
}

/* Initialization of the key named 'sctk_microthread_key' */
void
sctk_microthread_key_init()
{
	mpc_thread_keys_create(&sctk_microthread_key, NULL);
}

/* Initialization of a microthread instance */
int
sctk_microthread_init(long nb_vp, sctk_microthread_t *self)
{
	long i;
	__UNUSED__ int  res;
	int  current_mpc_vp;
	mpc_thread_attr_t __attr;
	int *order;

	/* Initialization of the sctk_microthread key (only once) */
	assume(mpc_thread_once(&sctk_microthread_key_is_initialized,
	                       sctk_microthread_key_init) == 0);

	mpc_common_nodebug("sctk_microthread_init: Ask for %d VP(s)", nb_vp);

	/* Memory allocation for all VPs */
	self->__list = (sctk_microthread_vp_t *)sctk_malloc(nb_vp * sizeof
	                                                    (sctk_microthread_vp_t) );
	assert(self->__list != NULL);

	mpc_common_nodebug("sctk_microthread_init: Ready to enter loop...");

	TODO("Put a warning when the #microVP > #cores")

	/* Update the number of VPs */
	self->__nb_vp = nb_vp;

	/* Grab the current VP this thread is already binded to */
	current_mpc_vp = mpc_topology_get_current_cpu();

	order = sctk_malloc(mpc_topology_get_pu_count() * sizeof(int) );
	assert(order != NULL);

	/* Grab the right order to allocate microVPs */
	mpc_topology_get_pu_neighborhood(current_mpc_vp, mpc_topology_get_pu_count(), order);

	/* Create all VPs */
	for(i = 0; i < nb_vp; i++)
	{
		mpc_common_nodebug("sctk_microthread_init: %d %d %p %ld", i,
		             sizeof(sctk_mctx_t), &( (&(self->__list[i]) )->vp_context),
		             ( (long)&( (&(self->__list[i]) )->vp_context) ) % 32);

		/* Get the set of registers */
		sctk_getcontext(&( (&(self->__list[i]) )->vp_context) );
		/* Initialize the corresponding VP */
		sctk_microthread_init_microthread_vp_t( (&(self->__list[i]) ) );

		mpc_common_nodebug("sctk_microthread_init: %d done", i);
	}



	mpc_common_nodebug("sctk_microthread_init: Ready to enter loop2...");

	/* Initialize slave VPs */
	for(i = 1; i < nb_vp; i++)
	{
		int    target_vp; /* Where the microVP has to be created */
		size_t stack_size;

		mpc_common_nodebug("sctk_microthread_init: Before attr init (microVP #%d)...", i);

		mpc_thread_attr_init(&__attr);

		mpc_common_nodebug("sctk_microthread_init: Before computing target vp");

		TODO("Placement policy of microVPs is not optimal when #microVPs > #VPs")
		// target_vp = (current_mpc_vp + i) % (mpc_topology_get_pu_count ()) ;
		target_vp = order[i % mpc_topology_get_pu_count()];

		mpc_common_nodebug("Putting microVP %d on VP %d", i, target_vp);

		mpc_thread_attr_setbinding(&__attr, target_vp);

		if(mpc_common_get_flags()->is_fortran == 1)
		{
			stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
		}
		else
		{
			stack_size = SCTK_ETHREAD_STACK_SIZE;
		}

		mpc_thread_attr_setstacksize(&__attr, stack_size);

		res =
		        mpc_thread_core_thread_create(&(self->__list[i].pid), &__attr,
		                                      sctk_microthread_slave_vp,
		                                      &(self->__list[i]) );
		assert(res == 0);

		mpc_thread_attr_destroy(&__attr);

		mpc_common_nodebug("sctk_microthread_init: After user thread create...");
	}

	sctk_free(order);

	/* Update the PID for the master VP */
	self->__list[0].pid = mpc_thread_self();

	mpc_common_nodebug("sctk_microthread_init: Ready to enter loop3...");


	/* Schedule a storage of the global structure 'sctk_microthread_t' */
	for(i = 1; i < nb_vp; i++)
	{
		long step;      /* Storing the current 'step' (index of current op) is not
		                 * necessary for this first execution */

		sctk_microthread_add_task(sctk_microthread_first_task, self, i, &step,
		                          self, MPC_MICROTHREAD_NO_TASK_INFO);
	}

	mpc_common_nodebug("sctk_microthread_init: Ready to exec...");

	/* Launch the master execution of this micro thread instance */
	sctk_microthread_parallel_exec(self, MPC_MICROTHREAD_WAKE_VPS);

	return 0;
}

int
sctk_microthread_parallel_exec(sctk_microthread_t *task, int val)
{
	long i;
	sctk_microthread_t *   old; /* In case of nested microthread, store the old 'context' */
	sctk_microthread_vp_t *self;
	long running = 1;
	long __nb_vp;
	sctk_microthread_vp_t *__list;

	/* Store the old microthread context in case of nested microthreading */
	old = (sctk_microthread_t *)mpc_thread_getspecific(sctk_microthread_key);

	mpc_thread_setspecific(sctk_microthread_key, task);

	self    = &(task->__list[0]);
	__nb_vp = task->__nb_vp;
	__list  = task->__list;

	/* If needed, launch the other VPs by switching 'to_do' and 'to_do_next' */
	if(val == MPC_MICROTHREAD_WAKE_VPS)
	{
		mpc_common_nodebug("Wake %d VPs", __nb_vp);

		for(i = 0; i < __nb_vp; i++)
		{
			__list[i].to_do_list      = __list[i].to_do_list_next;
			__list[i].to_do_list_next = 0;
			__list[i].to_run          = 1;
		}
	}

	/* Execute the operations of the current (master) VP */
	mpc_common_nodebug
	        ("sctk_microthread_parallel_exec: running the scheduler (vp=%p)", self);
	sctk_microthread_scheduler(self);

	SCTK_PROFIL_START(__sctk_microthread_parallel_exec__last_barrier);

	/* Wait for everyone (implicit barrier) */
	while(running)
	{
		running = 0;

		for(i = 1; i < __nb_vp; i++)
		{
			running = running | (__list[i].to_do_list);

			/*
			 * if ( running ) {
			 * break ;
			 * }
			 */
		}

		/* When someone is still working, a context switch is forced */
		if(running)
		{
			mpc_thread_yield();
		}
	}

	SCTK_PROFIL_END(__sctk_microthread_parallel_exec__last_barrier);

	/* Restore the previous microthread context in case of nested microthreading */
	mpc_thread_setspecific(sctk_microthread_key, old);

	return 0;
}
