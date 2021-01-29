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
/* #   - ADAM Julien <adamj@paratools.com>                                # */
/* #                                                                      # */
/* ######################################################################## */

#include <assert.h>
#include <signal.h>
#include "mpc_common_debug.h"
#include <mpc_launch_pmi.h>
#include <mpc_launch.h>
#include <mpc_common_flags.h>
#include "sctk_ft_types.h"

#include "sctk_rail.h"

#ifdef MPC_USE_DMTCP
#include <dmtcp.h>


/** Number of checkpoints done since the application starts */
static int nb_checkpoints = 0;
/** Number of restarts done since the application starts */
static int nb_restarts = 0;


#endif

/** Last C/R state */
static mpc_lowcomm_checkpoint_state_t __state = MPC_STATE_NO_SUPPORT;

/** Lock to create a critical section for C/R */
static mpc_common_rwlock_t checkpoint_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
/** Number of threads blocking the C/R to be started */
__thread int sctk_ft_critical_section = 0;


int dmtcp_get_ckpt_signal()
{
	mpc_common_debug_fatal("This function should be called outside of running wrapped by DMTPC")
	return 2;
}


/**
 * Routine called only when the application is in post-checkpoint state.
 * There are currently nothing to do.
 */
static inline void __sctk_ft_post_checkpoint()
{
	mpc_common_debug("Post-Checkpoint");
}

/**
 * Routine called only when the application restarts from a checkpoint.
 * We only re-print the MPC banner.
 */
static inline void __sctk_ft_post_restart()
{
	mpc_launch_print_banner(__state == MPC_STATE_RESTART);
}

/**
 * Initialize DMTCP checkpoint directory.
 */
static inline void __sctk_ft_set_ckptdir()
{
#ifdef MPC_USE_DMTCP
        char path[256], *dummy=NULL;
        memset(path, 0, 256);
        dummy = getcwd(path, 256);
        assert(dummy);

	dmtcp_set_global_ckpt_dir(path);
	dmtcp_set_coord_ckpt_dir(path);
	mpc_common_debug("DMTCP commented dmtcp_set_tmpdir() func");
	//dmtcp_set_tmpdir(dir);
#endif
}

/**
 * Initialize DMTCP and C/R module.
 * \return always 1.
 */
void sctk_ft_init()
{
#ifdef MPC_USE_DMTCP
	if(dmtcp_is_enabled())
	{
		__sctk_ft_set_ckptdir();

		if(dmtcp_get_ckpt_signal() == SIGUSR1)
		{
			mpc_common_debug_error("DMTCP and MPC both set an handler for SIGUSR1");
			mpc_common_debug_fatal("Signal value: %d", dmtcp_get_ckpt_signal());
		}

		mpc_common_get_flags()->checkpoint_model = "C/R (DMTCP) ENABLED";
	}
	else
	{
		mpc_common_get_flags()->checkpoint_model = "C/R (DMTCP) Disabled";
	}
#endif
}

/**
 * Check if C/R is enabled for the running application.
 * \return 1 if DMTCP is enabled, 0 otherwise.
 */
int sctk_ft_enabled()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_is_enabled();
#else
	return 0;
#endif
}

/**
 * Initialize a new checkpoint.
 */
void sctk_ft_checkpoint_init()
{
        /* IMPORTANT: After this function, the calling thread
         * should never call a function leading to a critical section.
         * In this case, mpc_thread_yield (for ethread_mxn) should not
         * be called for waiting. This thread should not be preempted
         * (leading to deadlock to acquire read after write).
         */
	mpc_common_spinlock_write_lock_yield(&checkpoint_lock);
#ifdef MPC_USE_DMTCP
	/* retrieve old value (we will wait for their increment to simulate a barrier) */
	dmtcp_get_local_status(&nb_checkpoints, &nb_restarts);
#endif
}

/**
 * Prepare for a checkpoint.
 *
 * This is different from the initialization, because the MPI layer needs to do some
 * stuff AFTER the init but BEFORE we close the networks.
 */
void sctk_ft_checkpoint_prepare()
{
	size_t nb = sctk_rail_count();
	size_t i;

	/* iterate over each rail */
	for (i = 0; i < nb; ++i) {
		sctk_rail_info_t* rail = sctk_rail_get_by_id(i);
		if(!rail) continue;
#ifdef MPC_USE_DMTCP
		/* condition (that could be extended (Portals?) */
                if(SCTK_RAIL_TYPE(rail) == SCTK_RTCFG_net_driver_infiniband)
		{
			sctk_rail_disable(rail);
		}
#endif
	}
}

/**
 * Attempt to create a critical region, blocking any later checkpoint requests.
 *
 * This function does not necessarily create a critical section. It tries to do it but
 * can return 0 in case of failure. Its purpose is to be called in `sctk_notify_...` function
 * and avoid two things:
 * 1. Avoid these functions to be called when a checkpoint is pending
 * 2. Avoid a checkpoint to be triggered if at least one thread is in a critical section.
 *
 * This is mandatory because you don't want an idle thread trying to poll queue you closed before checkpointing.
 * \return 1 if there is a critical section, 0 otherwise.
 */
int sctk_ft_no_suspend_start()
{

        int old = sctk_ft_critical_section++;
        
	/* if i'm the first to request the critical section */
        if(old == 0)
        {
		/* And I cannot take the lock --> skip to create the section */
                if(mpc_common_spinlock_read_trylock(&checkpoint_lock) != 0)
                {
                        sctk_ft_critical_section--;
                        return 0;
                }
        }
        return 1;
}

/**
 * Free the critical section.
 *
 * NOTE: Don't call this function if a critical section was not created first.
 * A typical way to call these two functions is:
 * ```{c}
 * if(sctk_ft_no_suspend_start())
 * {
 *     // do things
 *     sctk_ft_no_suspend_end();
 * }
 * ```
 */
void sctk_ft_no_suspend_end()
{
        int nw = --sctk_ft_critical_section;

        if(nw == 0)
        {
                mpc_common_spinlock_read_unlock(&checkpoint_lock);
        }
}

/**
 * emit a DMTCP checkpoint request to the coordinator.
 * Should be called only once for the whole application. Each call will trigger a new checkpoint.
 * If N proceses call it, you will have N checkpoints (instead of one).
 */
void sctk_ft_checkpoint()
{
#ifdef MPC_USE_DMTCP
	/* don't care about the return value because we have our own 'uniform' way to retrieve the state */
	dmtcp_checkpoint();
#endif
}

/**
 * Finalize the checkpoint step --> restart closed networks.
 *
 * This function also called state-specific handlers.
 */
void sctk_ft_checkpoint_finalize()
{
#ifdef MPC_USE_DMTCP
	/* we have to reinit PMI from MPC, each time */
 mpc_launch_pmi_init();

	/* depending on status, deferring the work to the FT system */
	switch(__state)
	{
		case MPC_STATE_CHECKPOINT: __sctk_ft_post_checkpoint(); break;
		case MPC_STATE_RESTART:    __sctk_ft_post_restart(); break;
		default: break;
	}
#endif
	
	unsigned int i;
	size_t nb = sctk_rail_count();
	for (i = 0; i < nb; ++i) {
		sctk_rail_info_t* rail = sctk_rail_get_by_id(i);
		if(!rail) continue;
#ifdef MPC_USE_DMTCP
                if(SCTK_RAIL_TYPE(rail) == SCTK_RTCFG_net_driver_infiniband)
		{
			sctk_rail_enable(rail);
		}
#endif
	}
	/* recall driver init function & update mpc_common_get_flags()->sctk_network_description_string string */
        sctk_rail_commit();
	
	mpc_common_spinlock_write_unlock(&checkpoint_lock);
}

/**
 * Disable the C/R system until the peer function is called.
 * \see sctk_ft_enable.
 * \return 1 in case of success, 0 otherwise.
 */
int sctk_ft_disable()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_disable_ckpt();
#else
	return 0;
#endif
}

/**
 * Enable the C/R system, until the peer function is called.
 * \see sctk_ft_disable
 * \return 1 in case of success, 0 otherwise
 */
int sctk_ft_enable()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_enable_ckpt();
#else
	return 0;
#endif
}

/**
 * Free C/R resources.
 * \return always 1
 */
int sctk_ft_finalize()
{
#ifdef MPC_USE_DMTCP
#endif
	return 1;
}

/**
 * Wait for checkpoint completion.
 * Called by all involved proceses.
 * \return the enum representing the state (CHECKPOINT,IGNORED,RESTART)
 */
mpc_lowcomm_checkpoint_state_t sctk_ft_checkpoint_wait()
{
#ifdef MPC_USE_DMTCP
	int new_nb_checkpoints, new_nb_restarts;
	/* old values have been loaded when calling _init() */
	do
	{
		dmtcp_get_local_status(&new_nb_checkpoints, &new_nb_restarts);
	}
	while(new_nb_checkpoints == nb_checkpoints && new_nb_restarts == nb_restarts);

	__state = (new_nb_restarts == nb_restarts) ? MPC_STATE_CHECKPOINT : MPC_STATE_RESTART;
	
#endif
	return __state;
}

/**
 * Stringify the C/R state to human-readable format.
 * \return the string to print
 */
char* sctk_ft_str_status(mpc_lowcomm_checkpoint_state_t s)
{
	static  char * state_names[] = {
		"MPC_STATE_ERROR",
		"MPC_STATE_CHECKPOINT",
		"MPC_STATE_RESTART",
		"MPC_STATE_IGNORE",
		"MPC_STATE_COUNT"
	};

	assert(s >= 0 && s < MPC_STATE_COUNT);
	return strdup(state_names[s]);
}




void mpc_cl_ft_init() __attribute__((constructor));

void mpc_cl_ft_init()
{
        MPC_INIT_CALL_ONLY_ONCE

	/* Runtime start */
        mpc_common_init_callback_register("Base Runtime Init Done",
                                          "Init Profiling keys",
                                          sctk_ft_init, 128);

}
