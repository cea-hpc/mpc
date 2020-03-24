#include "sctk_thread_generic_kind.h"
#include "sctk_thread_generic.h"
#include <mpc_common_flags.h>

///////////////////////////////////
// setters
//

void sctk_thread_generic_setkind_self(sctk_thread_generic_kind_t kind)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(sctk_thread_generic_self()->sched);
	sched->th->attr.kind = kind;
}

void sctk_thread_generic_setkind_mask_self(unsigned int kind_mask)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(sctk_thread_generic_self()->sched);
	sched->th->attr.kind.mask = kind_mask;
}

void sctk_thread_generic_setkind_priority_self(int priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(sctk_thread_generic_self()->sched);
		sched->th->attr.kind.priority = priority;
	}
}

void sctk_thread_generic_set_basic_priority_self(int basic_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(sctk_thread_generic_self()->sched);
		sched->th->attr.basic_priority = basic_priority;
	}
}

void sctk_thread_generic_set_current_priority_self(int current_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(sctk_thread_generic_self()->sched);
		sched->th->attr.current_priority = current_priority;
	}
}

///////////////////////////////////
// getters
//

sctk_thread_generic_kind_t sctk_thread_generic_getkind_self()
{
	sctk_thread_generic_kind_t       kind;
	sctk_thread_generic_scheduler_t *sched;

	sched = &(sctk_thread_generic_self()->sched);
	kind  = sched->th->attr.kind;
	return kind;
}

unsigned int sctk_thread_generic_getkind_mask_self()
{
	unsigned int kind_mask;
	sctk_thread_generic_scheduler_t *sched;

	sched     = &(sctk_thread_generic_self()->sched);
	kind_mask = sched->th->attr.kind.mask;
	return kind_mask;
}

int sctk_thread_generic_getkind_priority_self()
{
	int priority;
	sctk_thread_generic_scheduler_t *sched;

	sched    = &(sctk_thread_generic_self()->sched);
	priority = sched->th->attr.kind.priority;
	return priority;
}

///////////////////////////////////
// functions
//
void sctk_thread_generic_addkind_mask_self(unsigned int kind_mask)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		sctk_thread_generic_scheduler_t *sched;
		sched = &(sctk_thread_generic_self()->sched);
		sched->th->attr.kind.mask |= kind_mask;
	}
}

void sctk_thread_generic_removekind_mask_self(unsigned int kind_mask)
{
	sctk_thread_generic_scheduler_t *sched;

	sched = &(sctk_thread_generic_self()->sched);
	sched->th->attr.kind.mask &= ~kind_mask;
}
