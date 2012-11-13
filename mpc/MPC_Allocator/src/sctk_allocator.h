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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_H
#define SCTK_ALLOC_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#if defined(_WIN32)
	#include <windows.h>
	#ifdef _MSC_VER
		//used for _open and _write functions with VCC
		#include <io.h>
	#endif
#else	
	#include <sys/mman.h>
#endif
#include "sctk_alloc_common.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_mpscf_queue.h"

#include "sctk_alloc_chunk.h"
#include "sctk_alloc_thread_pool.h"
#include "sctk_alloc_mmsrc.h"
#include "sctk_alloc_mmsrc_default.h"
#include "sctk_alloc_rfq.h"
#include "sctk_alloc_chain.h"
#include "sctk_alloc_region.h"

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_H
