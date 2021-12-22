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

#include <mpc_common_debug.h>
#include <mpc_common_flags.h>
#include <mpc_launch.h>
#include <string.h>

#ifndef __SCTK__IB_TOOLKIT_H_
#define __SCTK__IB_TOOLKIT_H_

#include "mpc_common_helper.h"
#include "mpc_conf.h"

#define SMALL_BUFFER_SIZE (4*1024)
#define HAVE_SHELL_COLORS

#ifndef MPC_LOWCOMM_IB_MODULE_NAME
#define MPC_LOWCOMM_IB_MODULE_NAME "NONE"
#endif

void sctk_ib_toolkit_print_backtrace ( void );

#ifdef MPC_LOWCOMM_IB_MODULE_DEBUG
__UNUSED__ static void sctk_ib_debug ( const char *fmt, ... )
{
	va_list ap;
	char buff[SMALL_BUFFER_SIZE];

	if ( mpc_common_get_flags()->verbosity >= 2 )
	{
		va_start ( ap, fmt );
#ifdef HAVE_SHELL_COLORS
		snprintf ( buff, SMALL_BUFFER_SIZE,
		           " "MPC_COLOR_RED_BOLD ( [ %5s] ) " %s\n",
		           MPC_LOWCOMM_IB_MODULE_NAME, fmt );
#else
		snprintf ( buff, SMALL_BUFFER_SIZE,
		           " [%5s] %s\n",
		           MPC_LOWCOMM_IB_MODULE_NAME_STR ), fmt );
#endif

		mpc_common_io_noalloc_vfprintf ( stderr, buff, ap );
		va_end ( ap );
	}
}
#else
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define sctk_ib_debug(fmt,...) (void)(0)
#else
static inline void sctk_ib_debug ( const char *fmt, ... )
{
}
#endif
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define sctk_ib_nodebug(fmt,...) (void)(0)
#else
static inline void sctk_ib_nodebug ( const char *fmt, ... )
{
}
#endif
#define LOAD_RAIL(x) sctk_ib_rail_info_t *rail_ib = &(x)->network.ib;
#define LOAD_CONFIG(x) struct _mpc_lowcomm_config_struct_net_driver_infiniband *config = (x)->config;
#define LOAD_MMU(x)    _mpc_lowcomm_ib_mmu_t* mmu = (x)->mmu;
#define LOAD_DEVICE(x)    sctk_ib_device_t* device = (x)->device;
#define LOAD_POOL(x)    _mpc_lowcomm_ib_ibuf_poll_t* pool = (x)->pool_buffers;
#define LOAD_CP(x)    _mpc_lowcomm_ib_cp_ctx_t* cp = (x)->cp;
#define LOAD_PROFILER(x)    sctk_ib_prof_t* profiler = (x)->profiler;


/* Error handler */
#define MPC_LOWCOMM_IB_ABORT_WITH_ERRNO(...)                           \
		mpc_common_debug_error(__VA_ARGS__"(errno: %s)", strerror(errno));      \
		mpc_common_debug_abort();

#define MPC_LOWCOMM_IB_ABORT(...)        \
		mpc_common_debug_error(__VA_ARGS__);      \
		mpc_common_debug_abort();

#define ALIGN_ON(x, align) ( (x + (align-1)) & (~(align-1)) )

char sctk_network_is_ib_used();
void sctk_network_set_ib_used();
void sctk_network_set_ib_unused();

#endif
