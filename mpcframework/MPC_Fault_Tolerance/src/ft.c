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

#ifdef MPC_USE_DMTCP
#include <dmtcp.h>
#endif

const char * state_names[] = {"MPC_STATE_CHECKPOINT", "MPC_STATE_RESTART", "MPC_STATE_ERROR"};

static inline void sctk_ft_set_ckptdir(char * dir)
{
#ifdef MPC_USE_DMTCP
	dmtcp_set_global_ckpt_dir(dir);
	dmtcp_set_coord_ckpt_dir(dir);
#endif
}

static inline void sctk_ft_set_tmpdir(char * dir)
{
#ifdef MPC_USE_DMTCP
	sctk_warning("DMTCP commented dmtcp_set_tmpdir() func");
	//dmtcp_set_tmpdir(dir);
#endif
}

int sctk_ft_init()
{
#ifdef MPC_USE_DMTCP
	assume_m(dmtcp_is_enabled() == 1, "DMTCP not running but reaching FT interface");
	sctk_ft_set_ckptdir(".");
	sctk_ft_set_tmpdir(".");
	/*if(dmtcp_get_ckpt_signal() != SIGUSR2)*/
	/*{*/
		/*sctk_error("DMTCP and MPC both set an handler for SIGUSR2");*/
		/*sctk_fatal("Signal value: %d", dmtcp_get_ckpt_signal());*/
	/*}*/
#endif
}

int sctk_ft_enabled()
{
#ifdef MPC_USE_DMTCP
	return dmtcp_is_enabled();
#endif
}

int sctk_ft_checkpoint()
{
	sctk_ft_state_t st = MPC_STATE_ERROR;
#ifdef MPC_USE_DMTCP
	int ev = dmtcp_checkpoint();

	/* TODO: Disabling networks (only IB ? ) */
	switch(ev)
	{
		case DMTCP_AFTER_CHECKPOINT: st = MPC_STATE_CHECKPOINT; break;
	        case DMTCP_AFTER_RESTART: st = MPC_STATE_RESTART; break;
		case DMTCP_NOT_PRESENT: sctk_fatal("MPC C/R system does not have any initialized FT layer"); break;
		default: st = MPC_STATE_ERROR; break;
	}
	
	sctk_warning("FT: checkpoint: %s", state_names[st] );

	/* Probably not necessary to re-enable network (delegate it to on-demand support) */
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
