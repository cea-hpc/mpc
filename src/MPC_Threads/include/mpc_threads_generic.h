#ifndef MPC_THREADS_GENERIC
#define MPC_THREADS_GENERIC

#include <mpc_config.h>
#include <mpc_common_flags.h>

/****************
 * KIND SUPPORT *
 ****************/

// KIND_MASK should be a power of 2 (1,2,4,8,16,...)
#define KIND_MASK_MPI                   (1 << 0)
#define KIND_MASK_OMP                   (1 << 1)
#define KIND_MASK_PTHREAD               (1 << 2)
#define KIND_MASK_PROGRESS_THREAD       (1 << 3)
#define KIND_MASK_MPC_POLLING_THREAD    (1 << 4)
// TODO add all kinds

#define KIND_MASK_MPI_OMP               (KIND_MASK_MPI | KIND_MASK_OMP)
#define KIND_MASK_MPI_PTHREAD           (KIND_MASK_MPI | KIND_MASK_PTHREAD)

#define KIND_MASK_MPI_OMP_PTHREAD \
	(KIND_MASK_MPI | KIND_MASK_OMP | KIND_MASK_PTHREAD)


/* THREAD KIND GETTERS */

/**
 * @brief get.priority
 *
 * @return get priority for current thread
 */
int mpc_threads_generic_kind_current_priority_get();

/**
 * @brief get kind.priority
 *
 * @return a copy of the current thread's kind.priority
 */
int mpc_threads_generic_kind_priority_get();

/**
 * @brief get kind.priority
 *
 * @return a copy of the current thread's kind.basic priority
 */
int mpc_threads_generic_kind_basic_priority_get();

/**
 * @brief get kind.mask
 *
 * @return a copy of the current thread's kind.mask
 */
unsigned int mpc_threads_generic_kind_mask_get();


/**
 * @brief add kind.mask to the current mask
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void mpc_threads_generic_kind_mask_add(unsigned int kind_mask);

/**
 * @brief remove kind.mask to the current mask
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void mpc_threads_generic_kind_mask_remove(unsigned int kind_mask);

/* THREAD KIND SETTERS */

/**
 * @brief set kind.priority to the current thread
 *
 * @param priority scheduling priority of the current thread
 */
void mpc_threads_generic_kind_priority(int priority);

/**
 * @brief set basic_priority
 *
 * @param basic_priority scheduling priority of beginning
 */
void mpc_threads_generic_kind_basic_priority(int basic_priority);

/**
 * @brief set current_priority
 *
 * @param current_priority currrent priority of the thread
 */
void mpc_threads_generic_kind_current_priority(int current_priority);


/**
 * @brief set kind.mask to the current thread
 *
 * @param kind_mask KIND_MASK_* are defined in the top of this file
 */
void mpc_threads_generic_kind_mask_self(unsigned int kind_mask);

/**
 * @brief Increment all priorities for current thread
 *
 * @param increment what to add to priorities
 */
static inline void mpc_threads_generic_kind_priorities_incr(int increment)
{
	int prio = mpc_threads_generic_kind_current_priority_get();
	int basic = mpc_threads_generic_kind_basic_priority_get();

	mpc_threads_generic_kind_current_priority(prio + increment);
	mpc_threads_generic_kind_basic_priority(basic + increment);
}


#define MPC_GENERIC_THREAD_ENTER_PROGRESS(kind, priority) int __new_sched = mpc_common_get_flags()->new_scheduler_engine_enabled; \
							  int ___prio_save = priority; \
							  unsigned int ___mask_save = 0; \
							  do{\
								if( __new_sched ) { \
									mpc_threads_generic_kind_priorities_incr(___prio_save); \
									___mask_save = mpc_threads_generic_kind_mask_get(); \
									mpc_threads_generic_kind_mask_add(kind); \
								}\
							  } while(0)

#define MPC_GENERIC_THREAD_END_PROGRESS() do{\
						if( __new_sched ) { \
							mpc_threads_generic_kind_mask_self(___mask_save); \
							mpc_threads_generic_kind_priorities_incr(-___prio_save);\
						} \
					  } while(0)


/*******************
 * SCHEDULER HOOKS *
 *******************/

/* These are perfectly ugly and should be reworked */

extern void (*_mpc_threads_generic_scheduler_sched_init_ptr)();
extern void (*_mpc_threads_generic_scheduler_increase_prio_ptr)();
extern void (*_mpc_threads_generic_scheduler_decrease_prio_ptr)();
extern void (*_mpc_threads_generic_scheduler_task_decrease_prio_ptr)(int core);
extern void (*_mpc_threads_generic_scheduler_task_increase_prio_ptr)(int core);

static inline void mpc_threads_generic_scheduler_sched_init()
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled && _mpc_threads_generic_scheduler_sched_init_ptr)
	{
		_mpc_threads_generic_scheduler_sched_init_ptr();
	}
}

static inline void mpc_threads_generic_scheduler_increase_prio()
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled && _mpc_threads_generic_scheduler_increase_prio_ptr)
	{
		_mpc_threads_generic_scheduler_increase_prio_ptr();
	}
}

static inline void mpc_threads_generic_scheduler_decrease_prio()
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled && _mpc_threads_generic_scheduler_decrease_prio_ptr)
	{
		_mpc_threads_generic_scheduler_decrease_prio_ptr();
	}
}

static inline void mpc_threads_generic_scheduler_task_increase_prio(int core)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled && _mpc_threads_generic_scheduler_task_increase_prio_ptr)
	{
		_mpc_threads_generic_scheduler_task_increase_prio_ptr(core);
	}	
}

static inline void mpc_threads_generic_scheduler_task_decrease_prio(int core)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled && _mpc_threads_generic_scheduler_task_decrease_prio_ptr)
	{
		_mpc_threads_generic_scheduler_task_decrease_prio_ptr(core);
	}	
}




#endif /* MPC_THREADS_GENERIC */