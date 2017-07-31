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
#include "sctk_debug.h"
#include "sctk_ft_types.h"
#include "sctk_multirail.h"

#ifdef MPC_USE_DMTCP
#include <dmtcp.h>
#endif

static inline void __sctk_ft_pre_checkpoint();
static inline void __sctk_ft_post_checkpoint();
static inline void __sctk_ft_post_restart();

static inline void __sctk_ft_set_ckptdir(char * dir)
{
#ifdef MPC_USE_DMTCP
	dmtcp_set_global_ckpt_dir(dir);
	dmtcp_set_coord_ckpt_dir(dir);
#endif
}

static inline void __sctk_ft_set_tmpdir(char * dir)
{
#ifdef MPC_USE_DMTCP
	sctk_warning("DMTCP commented dmtcp_set_tmpdir() func");
	//dmtcp_set_tmpdir(dir);
#endif
}

int sctk_ft_init()
{
#ifdef MPC_USE_DMTCP
	if(dmtcp_is_enabled())
	{
		__sctk_ft_set_ckptdir(".");
		__sctk_ft_set_tmpdir(".");
		if(dmtcp_get_ckpt_signal() == SIGUSR2)
		{
			sctk_error("DMTCP and MPC both set an handler for SIGUSR2");
			sctk_fatal("Signal value: %d", dmtcp_get_ckpt_signal());
		}
	}
#endif
}

int sctk_ft_enabled()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_is_enabled();
#endif
}

static volatile int nb_checkpoints = 0;
static volatile int nb_restarts = 0;
static sctk_ft_state_t __state = MPC_STATE_ERROR;

void sctk_ft_checkpoint_init()
{
#ifdef MPC_USE_DMTCP
	dmtcp_get_local_status(&nb_checkpoints, &nb_restarts);
#endif
}

void sctk_ft_checkpoint()
{
	__sctk_ft_pre_checkpoint();

#ifdef MPC_USE_DMTCP
	dmtcp_checkpoint();
#endif
}

void sctk_ft_checkpoint_finalize()
{
#ifdef MPC_USE_DMTCP
	/* depending on status, deferring the work to the FT system */
	switch(__state)
	{
		case MPC_STATE_CHECKPOINT: __sctk_ft_post_checkpoint(); break;
		case MPC_STATE_RESTART:    __sctk_ft_post_restart(); break;
		default: break;
	}
#endif
}

int sctk_ft_disable()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_disable_ckpt();
#endif
}

int sctk_ft_enable()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_enable_ckpt();
#endif
}

int sctk_ft_finalize()
{
#ifdef MPC_USE_DMTCP
#endif
}

sctk_ft_state_t sctk_ft_checkpoint_wait()
{
#ifdef MPC_USE_DMTCP
	int new_nb_checkpoints, new_nb_restarts;
	do
	{
		dmtcp_get_local_status(&new_nb_checkpoints, &new_nb_restarts);
	}
	while(new_nb_checkpoints == nb_checkpoints && new_nb_restarts == nb_restarts);

	__state = (new_nb_restarts == nb_restarts) ? MPC_STATE_CHECKPOINT : MPC_STATE_RESTART;
	
#endif
	return __state;
}

static inline void __sctk_ft_pre_checkpoint()
{
	size_t nb = sctk_rail_count();
	size_t i;

	for (i = 0; i < nb; ++i) {
		sctk_rail_info_t* rail = sctk_rail_get_by_id(i);
		if(!rail) continue;
#ifdef MPC_USE_DMTCP
		if(sctk_rail_get_type(rail) == SCTK_NET_INFINIBAND)
		{
                        sctk_warning("Disconnect rail %d", i);
			/*sctk_multirail_on_demand_disconnection_rail(rail);*/
		}
#endif
	}
}

static inline void __sctk_ft_post_checkpoint()
{
	sctk_debug("Post-Checkpoint");
}

static inline void __sctk_ft_post_restart()
{
	sctk_debug("Post-Restart");
}

char* sctk_ft_str_status(sctk_ft_state_t s)
{
	static  char * state_names[] = {
		"MPC_STATE_ERROR",
		"MPC_STATE_CHECKPOINT",
		"MPC_STATE_RESTART",
		"MPC_STATE_IGNORE",
		"MPC_STATE_COUNT"
	};

	sctk_assert(s >= 0 && s < MPC_STATE_COUNT);
	return strdup(state_names[s]);
}
