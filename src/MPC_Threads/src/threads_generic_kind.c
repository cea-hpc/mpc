#include "threads_generic_kind.h"
#include "sctk_thread_generic.h"
#include <mpc_common_flags.h>

/***********************
 * THREAD KIND SETTERS *
 ***********************/

void _mpc_threads_generic_kind_set_self(sctk_thread_generic_kind_t kind)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(_mpc_threads_generic_self()->sched);
	sched->th->attr.kind = kind;
}

void mpc_threads_generic_kind_mask_self(unsigned int kind_mask)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(_mpc_threads_generic_self()->sched);
	sched->th->attr.kind.mask = kind_mask;
}

void mpc_threads_generic_kind_priority(int priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(_mpc_threads_generic_self()->sched);
		sched->th->attr.kind.priority = priority;
	}
}

void mpc_threads_generic_kind_basic_priority(int basic_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(_mpc_threads_generic_self()->sched);
		sched->th->attr.basic_priority = basic_priority;
	}
}

void mpc_threads_generic_kind_current_priority(int current_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(_mpc_threads_generic_self()->sched);
		sched->th->attr.current_priority = current_priority;
	}
}

/***********************
 * THREAD KIND GETTERS *
 ***********************/

sctk_thread_generic_kind_t _mpc_threads_generic_kind_get()
{
	sctk_thread_generic_kind_t       kind;
	sctk_thread_generic_scheduler_t *sched;

	sched = &(_mpc_threads_generic_self()->sched);
	kind  = sched->th->attr.kind;
	return kind;
}

unsigned int mpc_threads_generic_kind_mask_get()
{
	unsigned int kind_mask;
	sctk_thread_generic_scheduler_t *sched;

	sched     = &(_mpc_threads_generic_self()->sched);
	kind_mask = sched->th->attr.kind.mask;
	return kind_mask;
}

int mpc_threads_generic_kind_basic_priority_get()
{
	int priority;
	sctk_thread_generic_scheduler_t *sched;

	sched    = &(_mpc_threads_generic_self()->sched);
	priority = sched->th->attr.basic_priority;
	return priority;
}

int mpc_threads_generic_kind_current_priority_get()
{
	int priority;
	sctk_thread_generic_scheduler_t *sched;

	sched    = &(_mpc_threads_generic_self()->sched);
	priority = sched->th->attr.kind.priority;
	return priority;
}


int mpc_threads_generic_kind_priority_get()
{
	int priority;
	sctk_thread_generic_scheduler_t *sched;

	sched    = &(_mpc_threads_generic_self()->sched);
	priority = sched->th->attr.kind.priority;
	return priority;
}

void mpc_threads_generic_kind_mask_add(unsigned int kind_mask)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(_mpc_threads_generic_self()->sched);
		sched->th->attr.kind.mask |= kind_mask;
	}
}

void mpc_threads_generic_kind_mask_remove(unsigned int kind_mask)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(_mpc_threads_generic_self()->sched);
	sched->th->attr.kind.mask &= ~kind_mask;
}
